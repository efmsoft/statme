#include <cassert>

#include <Statme/Duration/Meter.h>

using namespace Duration;

Printer::Printer(
  Meter& target
  , const Logme::ID& ch
  , const std::string& title
) 
  : Target(target)
  , CH(ch)
  , Title(title)
{
}

Printer::~Printer()
{
  Target.PrintResults(CH, Title);
}


