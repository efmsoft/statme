#include <Statme/Counters/Holder.h>
#include <Statme/Counters/Manager.h>

using namespace Counters;

Holder::Holder(CounterPtr counter)
  : Ptr(counter)
{
}

Holder::~Holder()
{
  if (Ptr)
    Ptr->Owner->DeleteCounter(Ptr);
}

Holder::operator CounterPtr()
{
  return Ptr;
}
