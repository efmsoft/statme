#pragma once

#include <Statme/Counters/Counter.h>
#include <Statme/Macros.h>

namespace Counters
{
  struct Holder
  {
    CounterPtr Ptr;

  public:
    STATMELNK Holder(CounterPtr counter);
    STATMELNK ~Holder();

    STATMELNK operator CounterPtr();
  };
}