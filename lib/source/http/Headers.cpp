#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <format>
#include <iostream>
#include <iterator>
#include <new>
#include <sstream>
#include <stdexcept>
#include <string.h>

#include <Statme/http/Headers.h>

#ifndef _WIN32
#define strtok_s strtok_r
#endif

using namespace HTTP::Header;

static const char* SPEC = "_ :;.,\\/\"'?!(){}[]@<>=-+*#$&`|~^%";

namespace
{
  // Field names are ASCII tokens, and the lookups must not depend on the
  // process locale, so this is deliberately not ::tolower().
  inline char AsciiLower(char c)
  {
    return (c >= 'A' && c <= 'Z') ? char(c + ('a' - 'A')) : c;
  }

  // FNV-1a over the lowercased name: the same value for every spelling of a
  // field name, computed without making a lowercase copy of it.
  uint32_t HashKey(std::string_view key)
  {
    uint32_t h = 2166136261u;
    for (char c : key)
    {
      h ^= uint8_t(AsciiLower(c));
      h *= 16777619u;
    }

    return h;
  }

  // Nodes are carved from the blocks at 8-byte boundaries, each one taking a
  // whole number of such slots, so the room needed for N nodes is the sum of
  // their slots whatever sizeof(Field)/sizeof(FieldValue) are on the target
  // (they are not multiples of 8 on 32-bit).
  constexpr size_t NodeAlignment = 8;

  constexpr size_t NodeSlot(size_t size)
  {
    return (size + NodeAlignment - 1) & ~(NodeAlignment - 1);
  }

  static_assert(alignof(Field) <= NodeAlignment && alignof(FieldValue) <= NodeAlignment);

  // Marks a table slot whose field was deleted: probing goes on past it.
  Field* Tombstone()
  {
    return reinterpret_cast<Field*>(uintptr_t(1));
  }

  uint32_t CapacityOf(size_t size)
  {
    return uint32_t(std::min<size_t>(size, UINT32_MAX));
  }

  bool EqualsNoCase(std::string_view a, std::string_view b)
  {
    if (a.size() != b.size())
    {
      return false;
    }

    for (size_t i = 0; i < a.size(); ++i)
    {
      if (AsciiLower(a[i]) != AsciiLower(b[i]))
      {
        return false;
      }
    }

    return true;
  }
}

struct alignas(16) Headers::Block
{
  Block* Prev;
  // The memory handed out by NewBlock() follows the block header.
};

Headers::Headers(bool lowerCase)
  : Size(0)
  , LowerCase(lowerCase)
  , MixedLineEndings(false)
  , LineEnding('\r\n')
  , Blocks(nullptr)
  , Cur(nullptr)
  , End(nullptr)
  , Tombstones(0)
{
}

Headers::Headers(const Headers& other)
  : Size(other.Size)
  , ReqRes(other.ReqRes)
  , Body(other.Body)
  , LowerCase(other.LowerCase)
  , MixedLineEndings(other.MixedLineEndings)
  , LineEnding(other.LineEnding)
  , Blocks(nullptr)
  , Cur(nullptr)
  , End(nullptr)
  , Tombstones(0)
{
  AssignFields(other);
}

Headers& Headers::operator=(const Headers& other)
{
  if (this == &other)
  {
    return *this;
  }

  Size = other.Size;
  ReqRes = other.ReqRes;
  Body = other.Body;
  LowerCase = other.LowerCase;
  MixedLineEndings = other.MixedLineEndings;
  LineEnding = other.LineEnding;

  AssignFields(other);

  return *this;
}

Headers::~Headers()
{
  ClearFields();
}

bool Headers::Empty() const
{
  return ReqRes.empty();
}

void Headers::Clear()
{
  ClearFields();
  ReqRes.clear();
  Body.clear();
  Size = 0;
}

void Headers::ClearFields()
{
  for (Block* block = Blocks; block;)
  {
    Block* prev = block->Prev;
    ::operator delete(block);
    block = prev;
  }

  Blocks = nullptr;
  Cur = nullptr;
  End = nullptr;

  std::vector<Field*>().swap(Table);
  Tombstones = 0;

  Header.First = nullptr;
  Header.Last = nullptr;
  Header.Count = 0;
}

char* Headers::NewBlock(size_t bytes)
{
  void* memory = ::operator new(sizeof(Block) + bytes);
  Block* block = new (memory) Block{Blocks};

  char* data = reinterpret_cast<char*>(block + 1);

  Blocks = block;
  Cur = data;
  End = data + bytes;

  return data;
}

char* Headers::NewLayout(size_t nodeBytes, size_t textBytes)
{
  // One block laid out as [nodes][text]. Cur/End are left covering the node
  // area only: whatever the nodes do not use is what AddHeader/SetHeader can
  // still carve from, while the text area is full by construction.
  char* data = NewBlock(nodeBytes + textBytes);
  End = data + nodeBytes;

  return End;
}

void Headers::Reserve(size_t bytes)
{
  if (Cur && size_t(End - Cur) >= bytes)
  {
    return;
  }

  NewBlock(std::max(ArenaSize - sizeof(Block), bytes));
}

void* Headers::TakeNode(size_t size)
{
  const uintptr_t start = (uintptr_t(Cur) + NodeAlignment - 1) & ~uintptr_t(NodeAlignment - 1);
  const size_t slot = NodeSlot(size);

  // Reserve()/NewLayout() size the room for everything carved from it, so
  // this is a sizing bug and not an out-of-memory condition; writing past
  // the block would corrupt the heap, hence a check that stays in release.
  if (!Cur || start + slot > uintptr_t(End))
  {
    throw std::length_error("HTTP::Header::Headers: arena overflow");
  }

  Cur = reinterpret_cast<char*>(start + slot);
  return reinterpret_cast<void*>(start);
}

std::string_view Headers::StoreText(std::string_view text, bool lower)
{
  char* p = Cur;
  if (!p || p + text.size() + 1 > End)
  {
    throw std::length_error("HTTP::Header::Headers: arena overflow");
  }

  if (lower)
  {
    for (size_t i = 0; i < text.size(); ++i)
    {
      p[i] = AsciiLower(text[i]);
    }
  }
  else if (!text.empty())
  {
    memcpy(p, text.data(), text.size());
  }

  p[text.size()] = '\0';
  Cur = p + text.size() + 1;

  return std::string_view(p, text.size());
}

Field* Headers::Find(std::string_view key, uint32_t hash) const
{
  if (!Table.empty())
  {
    const size_t mask = Table.size() - 1;
    for (size_t i = hash & mask; Table[i]; i = (i + 1) & mask)
    {
      Field* field = Table[i];
      if (field != Tombstone() && field->Hash == hash && EqualsNoCase(field->Key, key))
      {
        return field;
      }
    }

    return nullptr;
  }

  for (Field* field = Header.First; field; field = field->Next)
  {
    if (field->Hash == hash && EqualsNoCase(field->Key, key))
    {
      return field;
    }
  }

  return nullptr;
}

Field* Headers::NewField(std::string_view storedKey, uint32_t hash)
{
  Field* field = new (TakeNode(sizeof(Field))) Field();
  field->Key = storedKey;
  field->Hash = hash;

  return field;
}

void Headers::AppendValue(Field* field, std::string_view storedValue)
{
  FieldValue* value = new (TakeNode(sizeof(FieldValue))) FieldValue();
  value->Text = storedValue;

  if (!field->Values.First)
  {
    field->Capacity = CapacityOf(storedValue.size());
  }

  field->Values.Append(value);
}

void Headers::LinkField(Field* field)
{
  field->Prev = Header.Last;
  field->Next = nullptr;

  if (Header.Last)
  {
    Header.Last->Next = field;
  }
  else
  {
    Header.First = field;
  }

  Header.Last = field;
  ++Header.Count;

  try
  {
    IndexField(field);
  }
  catch (...)
  {
    // The table is only an accelerator; an empty one sends lookups to the
    // list scan, which is always correct. Not being able to grow it must
    // not fail the add that has already been done.
    Table.clear();
    Tombstones = 0;
  }
}

void Headers::UnlinkField(Field* field)
{
  if (field->Prev)
  {
    field->Prev->Next = field->Next;
  }
  else
  {
    Header.First = field->Next;
  }

  if (field->Next)
  {
    field->Next->Prev = field->Prev;
  }
  else
  {
    Header.Last = field->Prev;
  }

  --Header.Count;
}

void Headers::IndexField(Field* field)
{
  // A table is built when the list outgrows IndexThreshold; once there is
  // one it has to take every field that is linked, however few are left.
  // The builds below cover the field that has just been linked.
  if (Table.empty())
  {
    if (Header.Count > IndexThreshold)
    {
      BuildTable();
    }

    return;
  }

  if ((Header.Count + Tombstones) * 2 > Table.size())
  {
    BuildTable();
    return;
  }

  const size_t mask = Table.size() - 1;
  size_t i = field->Hash & mask;
  while (Table[i] && Table[i] != Tombstone())
  {
    i = (i + 1) & mask;
  }

  if (Table[i] == Tombstone())
  {
    --Tombstones;
  }

  Table[i] = field;
}

void Headers::BuildTable()
{
  // Load stays between 1/4 (right after a build) and 1/2 (when the next
  // add rebuilds), so a probe always reaches an empty slot.
  size_t capacity = 64;
  while (capacity < Header.Count * 4)
  {
    capacity <<= 1;
  }

  std::vector<Field*> table(capacity, nullptr);
  const size_t mask = capacity - 1;

  for (Field* field = Header.First; field; field = field->Next)
  {
    size_t i = field->Hash & mask;
    while (table[i])
    {
      i = (i + 1) & mask;
    }

    table[i] = field;
  }

  Table.swap(table);
  Tombstones = 0;
}

void Headers::AddNew(std::string_view field, uint32_t hash, std::string_view value)
{
  Reserve(NodeSlot(sizeof(Field)) + NodeSlot(sizeof(FieldValue)) + field.size() + value.size() + 2 + NodePadding);

  // The key is stored the way this object keeps all of them: lowercased for
  // LowerCase headers, as given otherwise.
  std::string_view key = StoreText(field, LowerCase);
  std::string_view stored = StoreText(value, false);

  Field* created = NewField(key, hash);
  AppendValue(created, stored);
  LinkField(created);
}

void Headers::AddStored(std::string_view storedKey, std::string_view storedValue)
{
  const uint32_t hash = HashKey(storedKey);

  Field* field = Find(storedKey, hash);
  const bool isNew = field == nullptr;
  if (isNew)
  {
    field = NewField(storedKey, hash);
  }

  AppendValue(field, storedValue);

  if (isNew)
  {
    LinkField(field);
  }
}

void Headers::AssignFields(const Headers& other)
{
  ClearFields();

  if (other.Header.empty())
  {
    return;
  }

  size_t nodeBytes = 0;
  size_t textBytes = 0;
  for (const Field& field : other.Header)
  {
    nodeBytes += NodeSlot(sizeof(Field)) + field.Values.size() * NodeSlot(sizeof(FieldValue));
    textBytes += field.Key.size() + 1;

    for (std::string_view value : field.Values)
    {
      textBytes += value.size() + 1;
    }
  }

  // One block, exactly sized: nodes first, then the text they point at.
  char* text = NewLayout(nodeBytes, textBytes);

  auto store = [&text](std::string_view s)
  {
    if (!s.empty())
    {
      memcpy(text, s.data(), s.size());
    }

    text[s.size()] = '\0';

    std::string_view stored(text, s.size());
    text += s.size() + 1;
    return stored;
  };

  for (const Field& source : other.Header)
  {
    Field* field = NewField(store(source.Key), source.Hash);

    for (std::string_view value : source.Values)
    {
      AppendValue(field, store(value));
    }

    // Appends to the list; the table is built once at the end instead of
    // being grown field by field.
    field->Prev = Header.Last;
    if (Header.Last)
    {
      Header.Last->Next = field;
    }
    else
    {
      Header.First = field;
    }

    Header.Last = field;
    ++Header.Count;
  }

  if (Header.Count > IndexThreshold)
  {
    try
    {
      BuildTable();
    }
    catch (...)
    {
      // Lookups scan the list instead, see LinkField().
    }
  }
}

const char* Headers::sstrtok(
  char* str,
  const char* delimiters,
  char** context
)
{
  char* ret = strtok_s(str, delimiters, context);
  return ret ? ret : "";
}

HEADER_ERROR Headers::Parse(const StreamData& data, Verification type)
{
  return Parse(data, data.size(), type);
}

bool Headers::Printable(std::string_view str)
{
  for (auto c : str)
  {
    if (c < ' ' || c > '~')
      return false;
  }
  return true;
}

size_t Headers::CalcPrintable(std::string_view str)
{
  size_t n = 0;
  for (auto c : str)
  {
    if (c < ' ' || c > '~')
      break;

    ++n;
  }
  return n;
}

bool Headers::ValidKey(std::string_view key)
{
  // Key:
  // Alphanumeric characters : a - z, A - Z, and 0 - 9
  // The following special characters : - and _
  for (auto& c : key)
  {
    if (isalnum(uint8_t(c)) || c == '-' || c == '_')
      continue;

    return false;
  }

  return true;
}

bool Headers::ValidValue(std::string_view val)
{
  // Value:
  // The value of the HTTP request header you want to set can only contain:
  // Alphanumeric characters : a - z, A - Z, and 0 - 9
  // The following special characters : _:; ., \ / "'?!(){}[]@<>=-+*#$&`|~^%
  for (auto& c : val)
  {
    if (isalnum(c) || c == '-' || c == '_')
      continue;

    if (strchr(SPEC, c))
      continue;

    return false;
  }

  return true;
}

bool Headers::ValidHeaderBuf(std::string_view buf)
{
  // Allow all characters as in Value plus CR LF
  for (auto& c : buf)
  {
    if (isalnum(uint8_t(c)) || c == '-' || c == '_')
      continue;

    if (strchr(SPEC, c))
      continue;

    if (c == '\r' || c == '\n')
      continue;

    return false;
  }

  return true;
}

std::string Headers::Reparse()
{
  std::string h = ToString();

  auto e = Parse(h.c_str(), h.size(), Verification::NotStrict);
  if (e != HEADER_ERROR::NONE)
    return std::string();

  return h;
}

int Headers::SenseType(char* buffer)
{
  int n = 0;
  int rn = 0;

  while (*buffer)
  {
    char ch = *buffer++;
    if (ch == '\r' && *buffer == '\n')
    {
      buffer++;
      rn++;
      continue;
    }

    if (ch == '\n')
      n++;
  }

  LineEnding = rn >= 2 || rn >= n ? '\r\n' : '\n';
  MixedLineEndings = rn && n;

  return LineEnding;
}

char* Headers::ExtractHeaderLine(
  char* buffer
  , char*& context
  , int& type
)
{
  if (buffer == nullptr)
  {
    if (context == nullptr)
      return nullptr;

    buffer = context;
  }

  if (type == 0)
    type = SenseType(buffer);

  if (type == '\n')
  {
    char* p = strchr(buffer, '\n');
    if (p == nullptr)
    {
      context = nullptr;
      return buffer;
    }

    *p = '\0';
    context = p + 1;
    return buffer;
  }

  assert(type == '\r\n');

  char* p = strstr(buffer, "\r\n");
  if (p == nullptr)
  {
    context = nullptr;
    return buffer;
  }

  context = p + 2;

  p[0] = '\0';
  p[1] = '\0';

  return buffer;
}

HEADER_ERROR Headers::Parse(
  const char* data
  , size_t length
  , Verification type
)
{
  ReqRes.clear();
  ClearFields();

  Size = SizeOfHeader(data, length);
  if (long(Size) < 0)
  {
    return (HEADER_ERROR)Size;
  }

  // Every line of the block can become one field with one value, so the
  // number of '\n' bounds the number of nodes (the request/status line and
  // repeated names only make that an overestimate).
  size_t lines = 1;
  for (const char* p = data; (p = (const char*)memchr(p, '\n', data + Size - p)) != nullptr; ++p)
  {
    ++lines;
  }

  // The block is copied once; everything below only writes NULs into the
  // copy and points the fields at it. The nodes are built in the same
  // allocation, in front of the text.
  char* buf = NewLayout(lines * (NodeSlot(sizeof(Field)) + NodeSlot(sizeof(FieldValue))), Size + 1);
  memcpy(buf, data, Size);
  buf[Size] = '\0';

  int eol = 0;
  char* ctx1 = nullptr;
  char* line = ExtractHeaderLine(buf, ctx1, eol);
  for (int i = 0; line; line = ExtractHeaderLine(nullptr, ctx1, eol), ++i)
  {
    if (*line == '\0')
    {
      break;
    }

    if (!i)
    {
      ReqRes = line;
      continue;
    }

    char* colon = strchr(line, ':');
    if (!colon)
    {
      return HEADER_ERROR::INVALID;
    }

    // Only blanks around the name and the value are dropped, nothing else.
    char* nameBegin = line;
    while (nameBegin < colon && *nameBegin == ' ')
    {
      ++nameBegin;
    }

    char* nameEnd = colon;
    while (nameEnd > nameBegin && nameEnd[-1] == ' ')
    {
      --nameEnd;
    }

    char* valueBegin = colon + 1;
    while (*valueBegin == ' ')
    {
      ++valueBegin;
    }

    char* valueEnd = valueBegin + strlen(valueBegin);
    while (valueEnd > valueBegin && valueEnd[-1] == ' ')
    {
      --valueEnd;
    }

    std::string_view name(nameBegin, nameEnd - nameBegin);
    std::string_view val(valueBegin, valueEnd - valueBegin);

    if (type == Verification::Strict)
    {
      if (name.empty())
      {
        return HEADER_ERROR::EMPTY_KEY;
      }

      if (!ValidKey(name) || !ValidValue(val))
      {
        return HEADER_ERROR::INVALID_CHAR;
      }
    }

    if (name.empty())
    {
      continue;
    }

    // Terminate both in place (the colon and the line end are already there
    // for the cases where nothing was trimmed).
    *nameEnd = '\0';
    *valueEnd = '\0';

    if (LowerCase)
    {
      for (char* c = nameBegin; c < nameEnd; ++c)
      {
        *c = AsciiLower(*c);
      }
    }

    AddStored(name, val);
  }

  Body = std::string(data + Size, length - Size);

  return HEADER_ERROR::NONE;
}

std::string Headers::ToString(
  const char* indent
  , bool dropTermination
  , bool appendBody
) const
{
  std::string str;

  if (ReqRes.empty() == false)
  {
    str += indent;
    str += ReqRes;
    str += "\r\n";
  }

  for (auto& header : Header)
  {
    for (auto& v : header.Values)
    {
      str += indent;
      str += header.Key;
      str += ": ";
      str += v;
      str += "\r\n";
    }
  }

  str += "\r\n";

  if (appendBody == false || Body.empty() || (dropTermination && !Printable(Body)))
  {
    if (dropTermination)
      str.resize(str.length() - 4);
  }
  else if (*indent == '\0')
    str += Body;
  else
  {
    std::istringstream stream(Body);
    for (std::string line; std::getline(stream, line);)
    {
      str += indent;
      str += line;
      str += "\r\n";
    }

    if (dropTermination && str.length() >= 2)
      str.resize(str.length() - 2);
  }

  if (appendBody == false && Body.empty() == false)
    str += "\n";

  return str;
}

std::string Headers::BodyToString(
  const char* indent
  , bool InitialLF
  , size_t limit
  , size_t split
) const
{
  if (Body.empty())
    return std::string();

  std::string str;
  if (InitialLF)
    str += "\n";

  size_t cb = CalcPrintable(Body);
  if (!cb)
  {
    str += std::format("[non printable body: {} chars]", Body.size());
    return str;
  }

  auto n = std::min(limit, cb);

  static constexpr std::string_view kBreakChars = " \t,.;:!?/\\-|)]}";

  size_t pos = 0;
  str += indent;

  while (pos < n)
  {
    size_t chunkLen;
    if (split > 0)
      chunkLen = std::min(split, n - pos);
    else
      chunkLen = n - pos;

    size_t useLen = chunkLen;

    if (split > 0 && chunkLen > 1)
    {
      size_t lastBreak = SIZE_MAX;
      for (size_t i = chunkLen; i-- > 1; )
      {
        const char ch = Body[pos + i];
        if (kBreakChars.find(ch) != std::string_view::npos)
        {
          lastBreak = i;
          break;
        }
      }

      if (lastBreak != SIZE_MAX)
        useLen = lastBreak + 1;
    }

    str.append(Body, pos, useLen);
    pos += useLen;

    if (split > 0 && pos < n)
    {
      str.push_back('\n');
      str += indent;
    }
  }

  return str;
}

std::string Headers::GetFirstValue(
  std::string_view field
  , bool lowercase
  , const char* splitter
  , const std::string& def
) const
{
  // The splitter-less case (every real call site in the codebase omits it)
  // only ever wants Values[0], but routing it through GetHeader() allocates
  // a whole StringArrayPtr and copies every value into it via PushValue()
  // just to discard everything past index 0. Read the field's own Values
  // directly instead -- this is the hot path (~20 call sites, at least
  // once per request/response), so the allocation this skips is real.
  if (!splitter)
  {
    auto it = FindHeader(field);
    if (it == Header.end() || it->Values.empty())
    {
      return def;
    }

    std::string v(it->Values.front());
    if (lowercase)
    {
      std::transform(v.begin(), v.end(), v.begin(), ::tolower);
    }

    return v;
  }

  StringArrayPtr arr = GetHeader(field, lowercase, splitter);
  if (arr == nullptr || arr->empty())
  {
    return def;
  }

  return (*arr)[0];
}

bool Headers::HasHeader(std::string_view field) const
{
  return Find(field, HashKey(field)) != nullptr;
}

FieldList::const_iterator Headers::FindHeader(
  std::string_view field
) const
{
  // 4.2 Message Headers
  // HTTP header fields, which include general - header(section 4.5), request -
  // header(section 5.3), response - header(section 6.2), and entity - header(section 7.1)
  // fields, follow the same generic format as that given in Section 3.1 of RFC 822[9].
  // Each header field consists of a name followed by a colon(":") and the field value.
  // Field names are -----> case-insensitive <-------.
  //
  // Compared case-insensitively in place: neither a lowercase copy of the
  // name nor any other allocation is made for a lookup. Up to IndexThreshold
  // fields this is a scan of the list (one integer compare per field),
  // beyond that a hash probe -- there is no cap on header count under the
  // 48KB header block limit, and AddHeader during Parse looks every name up.
  return FieldList::const_iterator(Find(field, HashKey(field)));
}

void Headers::PushValue(StringArrayPtr arr, const std::string& value, bool lowercase)
{
  if (lowercase == false)
  {
    arr->push_back(value);
    return;
  }

  std::string v(value);
  std::transform(v.begin(), v.end(), v.begin(), ::tolower);
  arr->push_back(v);
}

StringArrayPtr Headers::GetHeader(
  std::string_view field
  , bool lowercase
  , const char* splitter
) const
{
  auto it = FindHeader(field);
  if (it == Header.end())
  {
    return StringArrayPtr();
  }

  StringArrayPtr arr = std::make_shared<StringArray>();
  for (std::string_view value : it->Values)
  {
    std::string v(value);

    if (splitter)
    {
      char* ctx1 = nullptr;
      char* line = strtok_s(&v[0], splitter, &ctx1);
      for (int i = 0; line; line = strtok_s(nullptr, splitter, &ctx1), ++i)
      {
        std::string str(line);
        str.erase(0, str.find_first_not_of(" "));
        str.erase(str.find_last_not_of(" ") + 1);

        PushValue(arr, str, lowercase);
      }
    }
    else
    {
      PushValue(arr, v, lowercase);
    }
  }

  return arr;
}

void Headers::DeleteHeader(std::string_view field)
{
  Field* found = Find(field, HashKey(field));
  if (!found)
  {
    return;
  }

  UnlinkField(found);

  // The unlinked field stays in its block (nothing is freed before the
  // object goes away); only the table has to forget it.
  if (Table.empty())
  {
    return;
  }

  if (Header.Count <= IndexThreshold)
  {
    // Small again: scanning the list is as fast, and the table is released.
    std::vector<Field*>().swap(Table);
    Tombstones = 0;
    return;
  }

  // Leave a tombstone instead of rebuilding: probing runs past it, and the
  // next build or growth clears them. No allocation per delete.
  const size_t mask = Table.size() - 1;
  size_t i = found->Hash & mask;
  for (size_t probes = 0; probes < Table.size() && Table[i] != found; ++probes)
  {
    i = (i + 1) & mask;
  }

  if (Table[i] != found)
  {
    // Cannot happen while the table holds every linked field; if it ever
    // did, the list scan is the safe way to carry on.
    std::vector<Field*>().swap(Table);
    Tombstones = 0;
    return;
  }

  Table[i] = Tombstone();
  ++Tombstones;
}

void Headers::SetHeader(
  std::string_view field
  , std::string_view value
)
{
  const uint32_t hash = HashKey(field);
  Field* found = Find(field, hash);

  if (!found)
  {
    AddNew(field, hash, value);
    return;
  }

  // Replace all the values by the new one. The first node is reused, and
  // the old text as well when the new value fits into it, so setting a
  // header over and over does not keep consuming arena.
  FieldValue* first = found->Values.First;
  if (!first)
  {
    Reserve(NodeSlot(sizeof(FieldValue)) + value.size() + 1 + NodePadding);
    AppendValue(found, StoreText(value, false));
    return;
  }

  if (value.size() <= found->Capacity)
  {
    // Fits into the room the first value was given, whatever its length is
    // right now. value may point into this very buffer, hence memmove, and
    // may be a default-constructed (null) view, hence the check.
    char* dst = const_cast<char*>(first->Text.data());
    if (!value.empty())
    {
      memmove(dst, value.data(), value.size());
    }

    dst[value.size()] = '\0';
    first->Text = std::string_view(dst, value.size());
  }
  else
  {
    Reserve(value.size() + 1);
    first->Text = StoreText(value, false);
    found->Capacity = CapacityOf(value.size());
  }

  first->Next = nullptr;
  found->Values.Last = first;
  found->Values.Count = 1;
}

void Headers::AddHeader(std::string_view field, std::string_view value)
{
  const uint32_t hash = HashKey(field);
  Field* found = Find(field, hash);

  if (found)
  {
    Reserve(NodeSlot(sizeof(FieldValue)) + value.size() + 1 + NodePadding);
    AppendValue(found, StoreText(value, false));
    return;
  }

  AddNew(field, hash, value);
}

void Headers::CopyTo(Headers& to) const
{
  if (this == &to)
  {
    return;
  }

  to.Size = Size;
  to.ReqRes = ReqRes;
  to.Body = Body;
  to.LowerCase = LowerCase;
  to.LineEnding = LineEnding;
  to.MixedLineEndings = MixedLineEndings;

  to.AssignFields(*this);
}

bool Headers::Complete(const std::vector<char>& data)
{
  return Complete(&data[0], data.size());
}

bool Headers::Complete(const char* data, size_t length)
{
  return long(Find2CRLF(data, length)) > 0;
}

size_t Headers::SizeOfHeader(const char* data, size_t length)
{
  return Find2CRLF(data, length);
}
