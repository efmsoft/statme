#pragma once

#include <string>

#include <Logme/Logme.h>
#include <Statme/Macros.h>

namespace Duration
{
  class Meter;
  class Printer
  {
    Meter& Target;
    const Logme::ID& CH;
    std::string Title;

  public:
    STATMELNK Printer(Meter& target, const Logme::ID& ch, const std::string& title);
    STATMELNK ~Printer();
  };
}