#pragma once

#include <functional>
#include <stdint.h>

#include <Logme/Logme.h>
#include <Syncme/TickCount.h>

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
}