#include <regex>

#include <Statme/http/Version.h>

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

  std::smatch m;
  static std::regex r("HTTP/(\\d+)\\.(\\d+)", std::regex_constants::icase);
  if (std::regex_match(ver, m, r))
  {
    major = atoi(m[1].str().c_str());
    minor = atoi(m[2].str().c_str());
  }

  return Version(major, minor);
}
