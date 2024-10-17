#pragma once

#include <stdint.h>

#include <Logme/Logme.h>
#include <Statme/Duration/Calculator.h>
#include <Statme/Duration/Printer.h>
#include <Statme/Duration/ThreadData.h>
#include <Statme/Duration/Tracer.h>
#include <Statme/Macros.h>
#include <Syncme/CritSection.h>

namespace Duration
{
  class Meter
  {
    static std::mutex ThreadMapLock;
    static std::map<uint64_t, Meter*> ThreadMap;

    CritSection Lock;
    ThreadDataMap Map;

  public:
    STATMELNK Meter();

    STATMELNK CalculatorPtr StartMeasurement(DurationCounter& dc, const char* name, uint64_t* pthreadid = nullptr);
    STATMELNK static CalculatorPtr StartThreadMeasurement(DurationCounter& dc, const char* name, uint64_t* pthreadid = nullptr);

    STATMELNK void PrintResults(const Logme::ID& ch, const std::string& title, uint64_t* pthreadid = nullptr);

    struct Handle
    {
      CalculatorPtr Calculator;
    };
    STATMELNK Handle* StartMeasurement2(DurationCounter& dc, const char* name);

    STATMELNK static Meter* SetThreadObject(Meter* o);
    STATMELNK static Meter* GetThreadObject(uint64_t* pthreadid = nullptr);

    struct ThreadObjectMapGuard
    {
      Meter* Prev;
      Meter* Object;

      ThreadObjectMapGuard(Meter* o)
        : Prev(nullptr)
        , Object(o)
      {
        if (Object)
          Prev = Duration::Meter::SetThreadObject(Object);
      }

      ~ThreadObjectMapGuard()
      {
        if (Object)
          Duration::Meter::SetThreadObject(Prev);
      }
    };
  private:
    friend Calculator;
    void StopMeasurement(uint64_t id, uint64_t duration, MeasurementPtr parent);

    ThreadDataPtr PopThreadData(uint64_t* pthreadid = nullptr);
    void AppendData(std::string& str, const std::string& align, MeasurementPtr m);
  };
}

#define THREAD_DURATION_METER_OBJECT(meter) \
  Duration::Meter::ThreadObjectMapGuard _tomGuard(meter)

#define DURATION_METER() \
  static DurationCounter _dc(__FUNCTION__); \
  Duration::CalculatorPtr _dcalc = DurationMeter.StartMeasurement(_dc, __FUNCTION__)

#define DURATION_METER_WITH_NAME(x) \
  static const std::string _tag = std::string(__FUNCTION__) + '_' + x; \
  static DurationCounter _dc(_tag.c_str()); \
  Duration::CalculatorPtr _dcalc = DurationMeter.StartMeasurement(_dc, _tag.c_str())

#define THREAD_DURATION_METER() \
  static DurationCounter _dc(__FUNCTION__); \
  Duration::CalculatorPtr _dcalc = Duration::Meter::StartThreadMeasurement(_dc, __FUNCTION__)

#define THREAD_DURATION_METER_WITH_NAME(x) \
  static const std::string _tag = std::string(__FUNCTION__) + '_' + x; \
  static DurationCounter _dc(_tag.c_str()); \
  Duration::CalculatorPtr _dcalc = Duration::Meter::StartThreadMeasurement(_dc, _tag.c_str())

#define DURATION_METER_RESULTS(t) \
  DurationMeter.PrintResults(CH, t)

#define DURATION_METER_AND_RESULTS(t) \
  static DurationCounter _dc(__FUNCTION__); \
  Duration::Printer _dprinter(DurationMeter, CH, t); \
  Duration::CalculatorPtr _dcalc = DurationMeter.StartMeasurement(_dc, __FUNCTION__)

#define DURATION_TRACER(...) \
  Duration::Tracer _tracer(__FUNCTION__, __VA_ARGS__)
