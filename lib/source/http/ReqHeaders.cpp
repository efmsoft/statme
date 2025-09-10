#include <algorithm>
#include <array>
#include <cstring>
#include <string_view>

#include <Statme/http/Headers.h>

#pragma warning(disable : 6255)

using namespace HTTP::Header;

namespace
{
  HEADER_ERROR MatchVerb(std::string_view str, bool allowPartial)
  {
    static std::array<std::string, 9> validVerbs = {
      "GET"
      , "HEAD"
      , "POST"
      , "PUT"
      , "DELETE"
      , "CONNECT"
      , "OPTIONS"
      , "TRACE"
      , "PATCH"
    };

    for (const auto& v : validVerbs)
    {
      auto matchSize = std::min(str.size(), v.size());
      bool matched{true};
      for (size_t i{}; i < matchSize; ++i)
      {
        if (v[i] != ::toupper((unsigned char)str[i]))
        {
          matched = false;
          break;
        }
      }

      if (!matched)
        continue;

      return str.size() >= v.size()
        ? HEADER_ERROR::NONE
        : (allowPartial ? HEADER_ERROR::NOT_COMPLETED : HEADER_ERROR::INVALID);
    }

    return HEADER_ERROR::INVALID;
  }

  bool VersionValid(HTTP::Version version)
  {
    return (version.Major == 0 && version.Minor == 9)
      || (version.Major == 1 && version.Minor == 0)
      || (version.Major == 1 && version.Minor == 1)
      || (version.Major == 2 && version.Minor == 0)
      || (version.Major == 3 && version.Minor == 0);
  }
}

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
  std::transform(Method.begin(), Method.end(), Method.begin(), [](char c) { return ::toupper((unsigned char)c); });
  Uri = sstrtok(nullptr, " ", &ctx1);
  std::string protocol = sstrtok(nullptr, "", &ctx1);

  // protocol can be empty for HTTP/0.9
  if (Method.empty() || Uri.empty())
    return HEADER_ERROR::INVALID;

  Protocol = Version::Parse(protocol);

  if (type == Verification::Strict)
  {
    if (MatchVerb(Method, false) != HEADER_ERROR::NONE)
    {
      return HEADER_ERROR::INVALID;
    }

    if (!VersionValid(Protocol))
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

void ReqHeaders::CopyTo(ReqHeaders& to) const
{
  Headers::CopyTo(to);

  to.Method = Method;
  to.Uri = Uri;
  to.Protocol = Protocol;
}


HEADER_ERROR ReqHeaders::TryParse(std::string_view data)
{
  if (data.empty())
    return HEADER_ERROR::ZERO_CHAR;

  if (data.size() > MAX_HEADER_SIZE)
    return HEADER_ERROR::TOO_LARGE;

  auto eol = data.find("\r\n");
  // first line is partial
  if (eol == std::string::npos)
  {
    auto firstSpace = data.find(' ');
    if (firstSpace == std::string::npos)
    {
      auto result = MatchVerb(data, true);
      return result == HEADER_ERROR::NONE ? HEADER_ERROR::NOT_COMPLETED : result;
    }
    else
    {
      auto result = MatchVerb(data.substr(0, firstSpace), false);
      if (result != HEADER_ERROR::NONE)
        return result;
    }

    // we may or may not find the second space
    ++firstSpace;
    auto secondSpace = data.find(' ', firstSpace);
    if (secondSpace == std::string::npos)
      return HEADER_ERROR::NOT_COMPLETED;

    // there must not be the third space on the first line
    ++secondSpace;
    auto thirdSpace = data.find(' ', secondSpace);
    return thirdSpace == std::string::npos ? HEADER_ERROR::NOT_COMPLETED : HEADER_ERROR::INVALID;
  }

  auto firstLine = data.substr(0, eol);

  // we must find the first space
  auto firstSpace = firstLine.find(' ');
  if (firstSpace == std::string::npos)
    return HEADER_ERROR::INVALID;

  // we must mutch verb completely
  if (MatchVerb(firstLine.substr(0, firstSpace), false) != HEADER_ERROR::NONE)
    return HEADER_ERROR::INVALID;

  // check version if we find the second space
  ++firstSpace;
  auto secondSpace = firstLine.find(' ', firstSpace);
  if (secondSpace != std::string::npos)
  {
    auto versionStr = firstLine.substr(secondSpace + 1);
    auto version = Version::Parse(std::string(versionStr));
    if (!VersionValid(version))
      return HEADER_ERROR::INVALID_CHAR;
  }

  auto crlf2Pos = Find2CRLF(data.data(), data.size());
  if (crlf2Pos > data.size())
    return (HEADER_ERROR)crlf2Pos;

  return HEADER_ERROR::NONE;
}
