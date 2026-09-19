#include <cctype>
#include <cstdlib>
#include <cstring>

#include <Statme/http/Version.h>

#ifdef _WIN32
#define strncasecmp _strnicmp
#endif

using namespace HTTP;

Version::Version()
  : Major(1)
  , Minor(1)
{
}

Version::Version(int major, int minor)
  : Major(major)
  , Minor(minor)
{
}

Version::operator int() const
{
  return (Major << 8) + Minor;
}

bool Version::operator<(const Version& v)
{
  return int(*this) < int(v);
}

bool Version::operator>(const Version& v)
{
  return int(*this) > int(v);
}
bool Version::operator<=(const Version& v)
{
  return int(*this) <= int(v);
}
bool Version::operator>=(const Version& v)
{
  return int(*this) >= int(v);
}

Version Version::Parse(const std::string& ver)
{
  // The initial version of HTTP had no version number; it was later 
  // called 0.9 to differentiate it from later versions
  if (ver.empty())
    return Version(0, 9);

  int major = 0;
  int minor = 0;

  // Replaces a std::regex_match against "HTTP/(\d+)\.(\d+)" (case
  // insensitive, full-string match). Every caller only ever passes the
  // already-isolated version token (see ReqHeaders::ParseReqLine /
  // ResHeaders::ParseResLine, which split the request/status line before
  // calling this), so this always runs on a short, fixed-shape string --
  // std::regex's NFA/capture-group machinery is real overhead here for
  // something this simple, and it runs at least twice per request. Falls
  // back to (0, 0), exactly like a non-matching regex did.
  static const size_t prefixLen = 5; // strlen("HTTP/")
  do
  {
    if (ver.size() <= prefixLen)
      break;

    if (strncasecmp(ver.c_str(), "HTTP/", prefixLen) != 0)
      break;

    size_t pos = prefixLen;
    size_t majorStart = pos;
    while (pos < ver.size() && isdigit((unsigned char)ver[pos]))
      ++pos;

    if (pos == majorStart || pos >= ver.size() || ver[pos] != '.')
      break;

    ++pos;
    size_t minorStart = pos;
    while (pos < ver.size() && isdigit((unsigned char)ver[pos]))
      ++pos;

    if (pos == minorStart || pos != ver.size())
      break;

    major = atoi(ver.c_str() + majorStart);
    minor = atoi(ver.c_str() + minorStart);
  } while (false);

  return Version(major, minor);
}
