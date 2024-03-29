#pragma once

#include <memory>
#include <vector>

#include <Statme/Duration/DurationCounter.h>
#include <Statme/Macros.h>

namespace Duration
{
  struct Measurement;
  typedef std::shared_ptr<Measurement> MeasurementPtr;
  typedef std::vector<MeasurementPtr> MeasurementArray;

  struct Measurement
  {
    int64_t Duration;
    const char* Name;
    DurationCounter& Counter;
    MeasurementArray Child;

  public:
    STATMELNK Measurement(DurationCounter& dc, const char* name);
  };
}