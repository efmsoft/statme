#pragma once

#include <string>

#include <Logme/Logme.h>
#include <Statme/Macros.h>

namespace Duration
{
  class Meter;
  class STATMELNK Printer
  {
    Meter& Target;
    const Logme::ID& CH;
    std::string Title;

  public:
    Printer(Meter& target, const Logme::ID& ch, const std::string& title);
    ~Printer();
  };
}