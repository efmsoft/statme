#include <Syncme/TickCount.h>
#include <Syncme/ProcessThreadId.h>

#include <Statme/Duration/Calculator.h>
#include <Statme/Duration/Meter.h>

using namespace Syncme;
using namespace Duration;

Calculator::Calculator(
  Meter* meter
  , MeasurementPtr parent
  , uint64_t* pthread_id
)
  : Owner(meter)
  , Parent(parent)
  , Start(GetTimeInMillisec())
  , ThreadID(pthread_id ? *pthread_id : GetCurrentThreadId())
{
}

Calculator::~Calculator()
{
  auto d = GetTimeInMillisec() - Start;
  Owner->StopMeasurement(ThreadID, d, Parent);
}
