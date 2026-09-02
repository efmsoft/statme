#pragma once

#include <functional>
#include <stdint.h>
#include <string>
#include <utility>

#include <Logme/Logme.h>
#include <Syncme/TickCount.h>
#include <Statme/Duration/DurationCounter.h>

namespace Duration
{
  typedef std::function<std::string()> TracerPrefix;

  struct Tracer
  {
    uint64_t Timeout;
    uint64_t Start;
    const char* Function;
    TracerPrefix Prefix;
    
    Logme::ID CH;
    Logme::ChannelPtr PCH;

    Tracer(const char* name, uint64_t timeout, const Logme::ID& ch, TracerPrefix prefix = nullptr)
      : Timeout(timeout)
      , CH(ch)
      , Start(Syncme::GetTimeInMillisec())
      , Function(name)
      , Prefix(prefix)
    {
    }

    Tracer(const char* name, uint64_t timeout, Logme::ChannelPtr pch, TracerPrefix prefix = nullptr)
      : Timeout(timeout)
      , CH{}
      , PCH(pch)
      , Start(Syncme::GetTimeInMillisec())
      , Function(name)
      , Prefix(prefix)
    {
    }

    ~Tracer()
    {
      auto d = Syncme::GetTimeInMillisec() - Start;
      if (d >= Timeout)
      {
        Logme::Override ovr;
        ovr.Remove.Method = true;

        Logme::ShortenerContext context;
        const char* p = Function;

        if (PCH)
        {
          Logme::ChannelPtr link = PCH->GetLinkPtr();
          if (link)
            p = link->ShortenerRun(Function, context);
          else
            p = PCH->ShortenerRun(Function, context);
        }

        std::string prefix;

        if (Prefix)
          prefix = Prefix();

        if (PCH)
        {
          LogmeW(PCH, ovr, "%s():%s took %i ms", p, prefix.c_str(), int(d));
        }
        else
          LogmeW(CH, ovr, "%s():%s took %i ms", p, prefix.c_str(), int(d));
      }
    }
  };

  // Shared "<name>():<prefix> took N ms" warning (method-name shortening +
  // parent-channel link), used by Guard below.
  inline void ReportSlow(
    const char* function
    , uint64_t ms
    , const Logme::ID& ch
    , Logme::ChannelPtr pch
    , const std::string& prefix)
  {
    Logme::Override ovr;
    ovr.Remove.Method = true;

    Logme::ShortenerContext context;
    const char* p = function;

    if (pch)
    {
      Logme::ChannelPtr link = pch->GetLinkPtr();
      p = link ? link->ShortenerRun(function, context)
               : pch->ShortenerRun(function, context);
    }

    if (pch)
      LogmeW(pch, ovr, "%s():%s took %i ms", p, prefix.c_str(), int(ms));
    else
      LogmeW(ch, ovr, "%s():%s took %i ms", p, prefix.c_str(), int(ms));
  }

  // Scoped timer for a single unit of work:
  //   * always feeds `counter` (global min/max/avg/count) when non-null;
  //   * logs one LogmeW when `warnMs != 0` and the scope ran >= warnMs.
  // `Prefix` is any callable returning std::string, invoked only when the
  // warning fires - no cost on the fast path.
  template <class Prefix>
  struct Guard
  {
    const char*       Name;
    uint64_t          WarnMs;
    DurationCounter*  Counter;
    Logme::ChannelPtr PCH;
    Logme::ID         CH;
    Prefix            MakePrefix;
    uint64_t          Start;

    Guard(const char* name, uint64_t warnMs, DurationCounter* counter,
          Logme::ChannelPtr pch, Prefix prefix)
      : Name(name), WarnMs(warnMs), Counter(counter), PCH(pch), CH{}
      , MakePrefix(std::move(prefix)), Start(Syncme::GetTimeInMillisec()) {}

    Guard(const char* name, uint64_t warnMs, DurationCounter* counter,
          const Logme::ID& ch, Prefix prefix)
      : Name(name), WarnMs(warnMs), Counter(counter), PCH{}, CH(ch)
      , MakePrefix(std::move(prefix)), Start(Syncme::GetTimeInMillisec()) {}

    Guard(const Guard&) = delete;
    Guard& operator=(const Guard&) = delete;

    ~Guard()
    {
      const uint64_t d = Syncme::GetTimeInMillisec() - Start;

      if (Counter)
        Counter->Add(d);

      if (WarnMs != 0 && d >= WarnMs)
        ReportSlow(Name, d, CH, PCH, MakePrefix());
    }
  };

  // Runs work(), times it (feeds counter / warns), returns work()'s result.
  template <class Prefix, class Work>
  decltype(auto) Timed(const char* name, uint64_t warnMs, DurationCounter* counter,
                       Logme::ChannelPtr pch, Prefix prefix, Work&& work)
  {
    Guard<Prefix> guard(name, warnMs, counter, pch, std::move(prefix));
    return std::forward<Work>(work)();
  }

  template <class Prefix, class Work>
  decltype(auto) Timed(const char* name, uint64_t warnMs, DurationCounter* counter,
                       const Logme::ID& ch, Prefix prefix, Work&& work)
  {
    Guard<Prefix> guard(name, warnMs, counter, ch, std::move(prefix));
    return std::forward<Work>(work)();
  }
}