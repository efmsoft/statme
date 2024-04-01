#pragma once

#include <mutex>
#include <stdint.h>

#include <Logme/Logme.h>
#include <Statme/Macros.h>

#pragma pack(push, 1)

struct STATMELNK DurationCounterValue
{
  DurationCounterValue();

  const char* Name;
  uint64_t Min;
  uint64_t Max;
  uint64_t Avg;
  uint64_t Sampling;
};
#pragma pack(pop)

class DurationCounter
{
  static std::mutex HeadLock;
  static DurationCounter* Head;

  const char* Name;
  DurationCounter* Next;

  uint64_t Min;
  bool MinSet;
  uint64_t Max;
  uint64_t Sum;
  uint64_t Num;

  std::atomic<long> Lock;

public:
  STATMELNK DurationCounter(const char* name);
  STATMELNK ~DurationCounter();

  STATMELNK void Reset();
  STATMELNK void Add(uint64_t duration);

  STATMELNK uint64_t Minimal() const;
  STATMELNK uint64_t Maximal() const;
  STATMELNK uint64_t Average() const;
  STATMELNK uint64_t Sampling() const;

  STATMELNK const char* GetName() const;
  STATMELNK operator uint64_t();
  STATMELNK operator DurationCounterValue();

  STATMELNK static void PrintStatistics(const Logme::ID& channel);
};
