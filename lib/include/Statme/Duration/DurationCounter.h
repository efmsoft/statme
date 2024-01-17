#pragma once

#include <atomic>
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

class STATMELNK DurationCounter
{
  static std::atomic<DurationCounter*> Head;

  const char* Name;
  std::atomic<DurationCounter*> Next;

  uint64_t Min;
  bool MinSet;
  uint64_t Max;
  uint64_t Sum;
  uint64_t Num;

  std::atomic<long> Lock;

public:
  DurationCounter(const char* name);

  void Reset();
  void Add(uint64_t duration);

  uint64_t Minimal() const;
  uint64_t Maximal() const;
  uint64_t Average() const;
  uint64_t Sampling() const;

  const char* GetName() const;
  operator uint64_t();
  operator DurationCounterValue();

  static void PrintStatistics(const Logme::ID& channel);
};
