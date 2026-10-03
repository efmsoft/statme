#include <algorithm>
#include <cassert>
#include <format>
#include <iostream>
#include <iterator>
#include <sstream>
#include <string.h>

#include <Statme/http/Headers.h>

#ifndef _WIN32
#define strtok_s strtok_r
#endif

using namespace HTTP::Header;

static const char* SPEC = "_ :;.,\\/\"'?!(){}[]@<>=-+*#$&`|~^%";

Headers::Headers(bool lowerCase)
  : Size(0)
  , LowerCase(lowerCase)
  , MixedLineEndings(false)
  , LineEnding('\r\n')
{
}

Headers::Headers(const Headers& other)
  : Size(other.Size)
  , ReqRes(other.ReqRes)
  , Header(other.Header)
  , Body(other.Body)
  , LowerCase(other.LowerCase)
  , MixedLineEndings(other.MixedLineEndings)
  , LineEnding(other.LineEnding)
{
  RebuildIndex();
}

Headers& Headers::operator=(const Headers& other)
{
  if (this == &other)
    return *this;

  Size = other.Size;
  ReqRes = other.ReqRes;
  Header = other.Header;
  Body = other.Body;
  LowerCase = other.LowerCase;
  MixedLineEndings = other.MixedLineEndings;
  LineEnding = other.LineEnding;

  RebuildIndex();

  return *this;
}

Headers::~Headers()
{
}

bool Headers::Empty() const
{
  return ReqRes.empty();
}

void Headers::Clear()
{
  Header.clear();
  Index.clear();
  ReqRes.clear();
  Body.clear();
  Size = 0;
}

std::string Headers::NormalizeKey(const std::string& key)
{
  std::string k(key);
  std::transform(k.begin(), k.end(), k.begin(), ::tolower);
  return k;
}

void Headers::RebuildIndex()
{
  // Every Field already carries its own NormalizedKey (set once at
  // AddHeader/SetHeader time and copied along with the rest of the field
  // by whatever copied Header before calling this) -- no need to
  // recompute it here.
  Index.clear();
  for (auto it = Header.begin(); it != Header.end(); ++it)
    Index[it->NormalizedKey] = it;
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

bool Headers::Printable(const std::string& str)
{
  for (auto c : str)
  {
    if (c < ' ' || c > '~')
      return false;
  }
  return true;
}

size_t Headers::CalcPrintable(const std::string& str)
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

bool Headers::ValidKey(const std::string& key)
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

bool Headers::ValidValue(const std::string& val)
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

bool Headers::ValidHeaderBuf(const std::string& buf)
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
  Header.clear();
  Index.clear();

  Size = SizeOfHeader(data, length);
  if (long(Size) < 0)
    return (HEADER_ERROR)Size;

  std::vector<char> buf(Size + 1);
  memcpy(&buf[0], data, Size);
  buf[Size] = '\0';

  int eol = 0;
  char* ctx1 = nullptr;
  char* line = ExtractHeaderLine(&buf[0], ctx1, eol);
  for (int i = 0; line; line = ExtractHeaderLine(nullptr, ctx1, eol), ++i)
  {
    if (*line == '\0')
      break;

    if (!i)
    {
      ReqRes = line;
      continue;
    }

    const char* p = strchr(line, ':');
    if (!p)
      return HEADER_ERROR::INVALID;

    // p already found the first ':' -- building name/val directly from it
    // avoids strtok_s() re-scanning the same line a second time just to
    // find the colon it was already given.
    std::string name(line, p - line);
    std::string val(p + 1);

    name.erase(0, name.find_first_not_of(" "));
    name.erase(name.find_last_not_of(" ") + 1);

    val.erase(0, val.find_first_not_of(" "));
    val.erase(val.find_last_not_of(" ") + 1);

    if (type == Verification::Strict)
    {
      if (name.empty())
        return HEADER_ERROR::EMPTY_KEY;

      if (!ValidKey(name) || !ValidValue(val))
        return HEADER_ERROR::INVALID_CHAR;
    }

    if (name.empty())
      continue;

    AddHeader(name, val);
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
  const std::string& field
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
      return def;

    if (!lowercase)
      return it->Values[0];

    std::string v(it->Values[0]);
    std::transform(v.begin(), v.end(), v.begin(), ::tolower);
    return v;
  }

  StringArrayPtr arr = GetHeader(field, lowercase, splitter);
  if (arr == nullptr || arr->empty())
    return def;

  return (*arr)[0];
}

bool Headers::HasHeader(const std::string& field) const
{
  return FindHeader(field) != Header.end();
}

FieldList::const_iterator Headers::FindHeader(
  const std::string& field
) const
{
  // 4.2 Message Headers
  // HTTP header fields, which include general - header(section 4.5), request -
  // header(section 5.3), response - header(section 6.2), and entity - header(section 7.1)
  // fields, follow the same generic format as that given in Section 3.1 of RFC 822[9].
  // Each header field consists of a name followed by a colon(":") and the field value.
  // Field names are -----> case-insensitive <-------.
  //
  // Index (name -> iterator into Header, kept up to date by every mutator)
  // turns this into an O(1) lookup; without it, AddHeader-during-Parse was
  // O(n^2) in the number of header lines, and there is no cap on header
  // count under the 48KB header block limit (VTune, 2026-09).
  auto it = Index.find(NormalizeKey(field));
  if (it == Index.end())
    return Header.end();

  return it->second;
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
  const std::string& field
  , bool lowercase
  , const char* splitter
) const
{
  auto it = FindHeader(field);
  if (it == Header.end())
    return StringArrayPtr();

  StringArrayPtr arr = std::make_shared<StringArray>();
  for (std::string v : it->Values)
  {
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
      PushValue(arr, v, lowercase);
  }

  return arr;
}

void Headers::DeleteHeader(const std::string& field)
{
  auto it = FindHeader(field);
  if (it == Header.end())
    return;

  Index.erase(NormalizeKey(field));
  Header.erase(it);
}

void Headers::SetHeader(
  const std::string& field
  , const std::string& value
)
{
  std::string key(field);

  if (LowerCase)
    std::transform(key.begin(), key.end(), key.begin(), ::tolower);

  // Normalize once and reuse it for both the lookup and the insert below,
  // instead of going through FindHeader() (which normalizes field for the
  // lookup) and then normalizing field again for Index[...] -- two
  // allocation+tolower passes over the same string on every new header
  // (VTune, 2026-09).
  std::string normalized = NormalizeKey(field);
  auto it = Index.find(normalized);

  if (it != Index.end())
  {
    Field& f = *it->second;
    f.Values.clear();
    f.Values.push_back(value);
  }
  else
  {
    Field header;
    header.Key = key;
    header.NormalizedKey = normalized;
    header.Values.push_back(value);
    Header.push_back(std::move(header));
    Index[std::move(normalized)] = std::prev(Header.end());
  }
}

void Headers::AddHeader(const std::string& field, const std::string& value)
{
  std::string key(field);

  if (LowerCase)
    std::transform(key.begin(), key.end(), key.begin(), ::tolower);

  // See SetHeader(): normalize once, reuse for both the lookup and the
  // insert (VTune, 2026-09).
  std::string normalized = NormalizeKey(field);
  auto it = Index.find(normalized);

  if (it != Index.end())
    it->second->Values.push_back(value);
  else
  {
    Field header;
    header.Key = key;
    header.NormalizedKey = normalized;
    header.Values.push_back(value);
    Header.push_back(std::move(header));
    Index[std::move(normalized)] = std::prev(Header.end());
  }
}

void Headers::CopyTo(Headers& to) const
{
  to.Size = Size;
  to.ReqRes = ReqRes;
  to.Body = Body;
  to.LowerCase = LowerCase;
  to.LineEnding = LineEnding;
  to.MixedLineEndings = MixedLineEndings;

  to.Header = Header;
  to.RebuildIndex();
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
