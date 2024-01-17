#pragma once

#include <map>
#include <memory>
#include <stdint.h>

#include <Statme/Duration/Measurement.h>

namespace Duration
{
  struct ThreadData
  {
    MeasurementPtr Root;
    MeasurementPtr Pos;
  };

  typedef std::shared_ptr<ThreadData> ThreadDataPtr;
  typedef std::map<uint64_t, ThreadDataPtr> ThreadDataMap;
}