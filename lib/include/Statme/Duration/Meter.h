#pragma once

#include <stdint.h>

#include <Logme/Logme.h>
#include <Statme/Duration/Calculator.h>
#include <Statme/Duration/Printer.h>
#include <Statme/Duration/ThreadData.h>
#include <Statme/Macros.h>
#include <Syncme/CritSection.h>

namespace Duration
{
  class STATMELNK Meter
  {
    CritSection Lock;
    ThreadDataMap Map;

  public:
    Meter();

    CalculatorPtr StartMeasurement(DurationCounter& dc, const char* name);
    void PrintResults(const Logme::ID& ch, const std::string& title);

    struct Handle
    {
      CalculatorPtr Calculator;
    };
    Handle* StartMeasurement2(DurationCounter& dc, const char* name);

  private:
    friend Calculator;
    void StopMeasurement(uint64_t id, uint64_t duration, MeasurementPtr parent);

    ThreadDataPtr PopThreadData();
    void AppendData(std::string& str, const std::string& align, MeasurementPtr m);
  };
}

#define DURATION_METER() \
  static DurationCounter _dc(__FUNCTION__); \
  Duration::CalculatorPtr _dcalc = DurationMeter.StartMeasurement(_dc, __FUNCTION__)

#define DURATION_METER_RESULTS(t) \
  DurationMeter.PrintResults(CH, t)

#define DURATION_METER_AND_RESULTS(t) \
  static DurationCounter _dc(__FUNCTION__); \
  Duration::Printer _dprinter(DurationMeter, CH, t); \
  Duration::CalculatorPtr _dcalc = DurationMeter.StartMeasurement(_dc, __FUNCTION__)

