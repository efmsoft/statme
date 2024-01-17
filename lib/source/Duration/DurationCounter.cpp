#include <algorithm>
#include <cassert>
#include <cstring>

#include <Statme/Duration/DurationCounter.h>
#include <Syncme/Sleep.h>

using namespace Syncme;

std::atomic<DurationCounter*> DurationCounter::Head;

DurationCounterValue::DurationCounterValue()
{
  memset(this, 0, sizeof(DurationCounterValue));
}

DurationCounter::DurationCounter(const char* name)
  : Name(name)
  , Next(nullptr)
  , Lock(0)
{
  Reset();
  Next = std::atomic_exchange(&Head, this);
}

void DurationCounter::Reset()
{
  Min = 0;
  MinSet = false;
  Max = 0;
  Sum = 0;
  Num = 0;
}

void DurationCounter::Add(uint64_t spent)
{
  for (long i = 0;; Sleep(1), i++)
  {
    if (!atomic_exchange(&Lock, true))
    {
      Min = MinSet ? std::min(Min, spent) : spent;
      MinSet = true;
      Max = std::max(Max, spent);
      Sum += spent;
      Num++;

      atomic_exchange(&Lock, false);
      break;
    }
    assert(i < 100);
  }
}

uint64_t DurationCounter::Minimal() const
{
  return Min;
}

uint64_t DurationCounter::Maximal() const
{
  return Max;
}

uint64_t DurationCounter::Average() const
{
  uint64_t sum = Sum;
  uint64_t div = Num;

  if (!div)
    return 0;

  return sum / div;
}

uint64_t DurationCounter::Sampling() const
{
  return Num;
}

DurationCounter::operator uint64_t()
{
  return Num;
}

DurationCounter::operator DurationCounterValue()
{
  DurationCounterValue v;
  v.Name = Name;
  v.Min = Minimal();
  v.Max = Maximal();
  v.Avg = Average();
  v.Sampling = Sampling();
  return v;
}

const char* DurationCounter::GetName() const
{
  return Name;
}

void DurationCounter::PrintStatistics(const Logme::ID& channel)
{
  std::vector<DurationCounterValue> arr;
  for (DurationCounter* p = Head; p; p = p->Next)
  {
    const char* n = p->GetName();
    if (n == nullptr || *n == '\0')
      continue;

    arr.push_back(*p);
  }

  if (arr.empty())
    return;

  std::qsort(
    &arr[0]
    , arr.size()
    , sizeof(DurationCounterValue)
    , [](const void* a, const void* b) {
      DurationCounterValue* p1 = (DurationCounterValue*)a;
      DurationCounterValue* p2 = (DurationCounterValue*)b;
      return int(p2->Avg - p1->Avg);
    }
  );

  std::string str(
    "\n"
    "Duration of execution (min/max/average)\n"
    "---------------------------------------\n");

  for (auto& o : arr)
  {
    str += o.Name;
    str += "(): ";
    str += std::to_string(o.Min);
    str += " / ";
    str += std::to_string(o.Max);
    str += " / ";
    str += std::to_string(o.Avg);
    str += " [";
    str += std::to_string(o.Sampling);
    str += "]\n";
  }

  Logme::Override ovr;
  ovr.Remove.Method = true;
  ovr.Remove.ErrorPrefix = true;

  LogmeI(channel, ovr, "\n%s", str.c_str());
}