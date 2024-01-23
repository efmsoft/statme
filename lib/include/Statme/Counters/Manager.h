#pragma once

#include <mutex>

#include <Syncme/Config/Config.h>
#include <Syncme/Sync.h>
#include <Syncme/ThreadPool/Pool.h>

#include <Statme/Counters/Counter.h>
#include <Statme/Macros.h>

namespace Counters
{
  class Manager
  {
    std::mutex Lock;
    CounterArray Counters;
    CounterArray Deleted;
    uint64_t Modified;

    Syncme::ThreadPool::Pool& Pool;
    HEvent& StopEvent;
    Syncme::ConfigPtr Config;

    int Socket;
    HEvent ListenerThread;
    HEvent WorkerThread;
    bool Exiting;

  public:
    STATMELNK Manager(Syncme::ThreadPool::Pool& pool, HEvent& stopEvent);
    STATMELNK ~Manager();

    STATMELNK void SetSocketConfig(Syncme::ConfigPtr config);

    STATMELNK CounterPtr AddCounter(
      const char* name
      , const char* category
    );

    STATMELNK CounterPtr AddCounter(
      const std::string& name
      , const char* category
    );

    STATMELNK void DeleteCounter(CounterPtr counter);

    STATMELNK std::mutex& GetLock();

    STATMELNK bool Start(int port);
    STATMELNK void Stop();
    STATMELNK void SetDirty();

  private:
    void Listener();
    void Worker();
    void CloseSocket();
    void ConnectionWorker(int socket);

    std::string GrabStat(uint64_t timestamp);
  };
}
