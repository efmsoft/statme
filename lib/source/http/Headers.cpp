#include <algorithm>
#include <cassert>
#include <format>
#include <iostream>
#include <sstream>
#include <string.h>

#include <Statme/http/Headers.h>

#ifndef _WIN32
#define strtok_s strtok_r
#else
#define strcasecmp _stricmp
#endif

using namespace HTTP::Header;

static const char* SPEC = "_ :;.,\\/\"'?!(){}[]@<>=-+*#$&`|~^%";

Headers::Headers(bool lowerCase)
  : Size(0)
  , LowerCase(lowerCase)
{
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
  ReqRes.clear();
  Body.clear();
  Size = 0;
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

static char* ExtractHeaderLine(
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
  {
    char* p = strchr(buffer, '\n');
    if (p == nullptr)
    {
      context = nullptr;
      return buffer;
    }

    *p = '\0';
    context = p + 1;

    int len = p - buffer;
    if (len && p[-1] == '\r')
    {
      p[-1] = '\0';
      type = '\r\n';
    }
    else
      type = '\n';
    
    return buffer;
  }

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

    char* ctx2 = nullptr;
    std::string name = sstrtok(line, ":", &ctx2);
    std::string val = sstrtok(nullptr, "", &ctx2);

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
) const
{
  std::string str;
  
  if (ReqRes.empty() == false)
  {
    str += indent;
    str += ReqRes;
    str += "\r\n";
  }

  for (auto it = Header.begin(); it != Header.end(); ++it)
  {
    auto& header = *it;
    for (auto& v : header->Values)
    {
      str += indent;
      str += header->Key;
      str += ": ";
      str += v;
      str += "\r\n";
    }
  }

  str += "\r\n";

  if (Body.empty() || (dropTermination && !Printable(Body)))
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
  return str;
}

std::string Headers::BodyToString(
  const char* indent
  , bool InitialLF
  , size_t limit
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

  str += Body.substr(0, std::min(limit, cb));
  return str;
}

std::string Headers::GetFirstValue(
  const std::string& field
  , bool lowercase
  , const char* splitter
  , const std::string& def
) const
{
  StringArrayPtr arr = GetHeader(field, lowercase, splitter);
  if (arr == nullptr || arr->empty())
    return def;

  return (*arr)[0];
}

bool Headers::HasHeader(const std::string& field)
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
  for (auto it = Header.begin(); it != Header.end(); ++it)
  {
    const std::string& key = (*it)->Key;
    if (!strcasecmp(field.c_str(), key.c_str()))
      return it;
  }
  return Header.end();
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
  for (std::string v : (*it)->Values)
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

  auto it = FindHeader(field);

  if (it != Header.end())
  {
    (*it)->Values.clear();
    (*it)->Values.push_back(value);
  }
  else
  {
    FieldPtr header = std::make_shared<Field>();
    header->Key = key;
    header->Values.push_back(value);
    Header.push_back(header);
  }
}

void Headers::AddHeader(const std::string& field, const std::string& value)
{
  std::string key(field);

  if (LowerCase)
    std::transform(key.begin(), key.end(), key.begin(), ::tolower);

  auto it = FindHeader(field);

  if (it != Header.end())
    (*it)->Values.push_back(value);
  else
  {
    FieldPtr header = std::make_shared<Field>();
    header->Key = key;
    header->Values.push_back(value);
    Header.push_back(header);
  }
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
