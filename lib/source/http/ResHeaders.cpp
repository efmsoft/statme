#include <cstring>

#include <Statme/http/Headers.h>

#pragma warning(disable : 6255)

using namespace HTTP::Header;

ResHeaders::ResHeaders(bool lowerCase)
  : Headers(lowerCase)
  , Protocol(0, 9)
  , Status(0)
{
}

HEADER_ERROR ResHeaders::Parse(const StreamData& data, Verification type)
{
  return Parse(data, data.size(), type);
}

HEADER_ERROR ResHeaders::Parse(
  const char* data
  , size_t length
  , Verification type
)
{
  HEADER_ERROR e = Headers::Parse(data, length, type);
  if (e != HEADER_ERROR::NONE)
    return e;

  return ParseResLine(type);
}

HEADER_ERROR ResHeaders::ParseResLine(Verification type)
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

  if (type == Verification::Strict)
  {
    if (Status < 100)
      return HEADER_ERROR::INVALID;

    if (!(Protocol.Major == 0 && Protocol.Minor == 9)
      && !(Protocol.Major == 1 && Protocol.Minor == 0)
      && !(Protocol.Major == 1 && Protocol.Minor == 1)
      && !(Protocol.Major == 2 && Protocol.Minor == 0)
      && !(Protocol.Major == 3 && Protocol.Minor == 0)
      )
    {
      return HEADER_ERROR::INVALID;
    }
  }

  return HEADER_ERROR::NONE;
}
