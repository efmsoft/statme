#pragma once

#include <string>

#include <Statme/Macros.h>

namespace HTTP
{
  struct Version
  {
    int Major;
    int Minor;

    STATMELNK Version(int major, int minor);

    STATMELNK bool operator<(const Version& v);
    STATMELNK bool operator>(const Version& v);
    STATMELNK bool operator<=(const Version& v);
    STATMELNK bool operator>=(const Version& v);

    STATMELNK operator int() const;

    STATMELNK static Version Parse(const std::string& ver);
  };
}