#include <cstring>

#include <Statme/http/Headers.h>

#pragma warning(disable : 6255)

using namespace HTTP::Header;

ResHeaders::ResHeaders()
  : Protocol(0, 9)
  , Status(0)
{
}

HEADER_ERROR ResHeaders::Parse(const StreamData& data)
{
  return Parse(data, data.size());
}

HEADER_ERROR ResHeaders::Parse(const char* data, size_t length)
{
  HEADER_ERROR e = Headers::Parse(data, length);
  if (e != HEADER_ERROR::NONE)
    return e;

  return ParseResLine();
}

HEADER_ERROR ResHeaders::ParseResLine()
{
  char* line = (char*)alloca(ReqRes.size() + 1);
  strcpy(line, ReqRes.c_str());

  char* ctx1 = nullptr;
  std::string protocol = sstrtok(line, " ", &ctx1);
  std::string status = sstrtok(nullptr, " ", &ctx1);
  Reason = sstrtok(nullptr, "", &ctx1);

  if (protocol.empty() || status.empty())
    return HEADER_ERROR::INVALID;

  Protocol = Version::Parse(protocol);

  char* e = nullptr;
  Status = (int)strtoll(status.c_str(), &e, 10);
  if (*e != '\0')
    return HEADER_ERROR::INVALID;

  return HEADER_ERROR::NONE;
}
