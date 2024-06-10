#include <algorithm>
#include <cstring>

#include <Statme/http/Headers.h>

#pragma warning(disable : 6255)

using namespace HTTP::Header;

ReqHeaders::ReqHeaders(bool lowerCase)
  : Headers(lowerCase)
  , Protocol(0, 9)
{
}

HEADER_ERROR ReqHeaders::Parse(const StreamData& data, Verification type)
{
  return Parse(data, data.size(), type);
}

HEADER_ERROR ReqHeaders::Parse(
  const char* data
  , size_t length
  , Verification type
)
{
  HEADER_ERROR e = Headers::Parse(data, length, type);
  if (e != HEADER_ERROR::NONE)
    return e;

  return ParseReqLine(type);
}

HEADER_ERROR ReqHeaders::ParseReqLine(Verification type)
{
  char* line = (char*)alloca(ReqRes.size() + 1);
  strcpy(line, ReqRes.c_str());

  char* ctx1 = nullptr;
  Method = sstrtok(line, " ", &ctx1);
  Uri = sstrtok(nullptr, " ", &ctx1);
  std::string protocol = sstrtok(nullptr, "", &ctx1);

  // protocol can be empty for HTTP/0.9
  if (Method.empty() || Uri.empty())
    return HEADER_ERROR::INVALID;

  Protocol = Version::Parse(protocol);

  if (type == Verification::Strict)
  {
    std::string method(Method);
    std::transform(method.begin(), method.end(), method.begin(), ::toupper);

    if (method != "GET"
      && method != "HEAD"
      && method != "POST"
      && method != "PUT"
      && method != "DELETE"
      && method != "CONNECT"
      && method != "OPTIONS"
      && method != "TRACE"
      && method != "PATCH"
    )
    {
      return HEADER_ERROR::INVALID;
    }

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

bool ReqHeaders::IsHeadRequest() const
{
  return Method == "HEAD";
}
