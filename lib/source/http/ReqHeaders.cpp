#include <cstring>

#include <Statme/http/Headers.h>

#pragma warning(disable : 6255)

using namespace HTTP::Header;

ReqHeaders::ReqHeaders()
  : Protocol(0, 9)
{
}

HEADER_ERROR ReqHeaders::Parse(const StreamData& data)
{
  return Parse(data, data.size());
}

HEADER_ERROR ReqHeaders::Parse(const char* data, size_t length)
{
  HEADER_ERROR e = Headers::Parse(data, length);
  if (e != HEADER_ERROR::NONE)
    return e;

  return ParseReqLine();
}

HEADER_ERROR ReqHeaders::ParseReqLine()
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

  return HEADER_ERROR::NONE;
}

bool ReqHeaders::IsHeadRequest() const
{
  return Method == "HEAD";
}
