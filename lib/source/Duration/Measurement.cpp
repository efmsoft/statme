#include <Statme/Duration/Measurement.h>

using namespace Duration;

Measurement::Measurement(DurationCounter& dc, const char* name)
  : Duration(0)
  , Name(name)
  , Counter(dc)
{
}