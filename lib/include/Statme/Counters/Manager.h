#pragma once

#include <memory>
#include <mutex>

#include <Syncme/Config/Config.h>
#include <Syncme/Sync.h>
#include <Syncme/ThreadPool/Pool.h>

#include <Statme/Counters/Counter.h>
#include <Statme/Macros.h>

#include <Statme/Counters/WebsocketServer.h>

namespace Counters
{
  class Manager;
  typedef std::shared_ptr<Manager> ManagerPtr;

  class Manager 
    : public std::enable_shared_from_this<Manager>
    , Counters::WebsocketServer
  {
    static Manager* Instance;

    std::mutex Lock;
    uint64_t Modified;
    CounterArray Counters;

    CounterArray Deleted;
    HEvent DeletedEvent;
    uint32_t WorkerTimeout;

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

    STATMELNK static ManagerPtr GetInstance();

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

  protected:
    void OnOpen(const WebSocketChannelPtr& channel, const HttpRequestPtr& req) override;
    void OnMessage(const WebSocketChannelPtr& channel, const std::string& msg) override;
    void OnClose(const WebSocketChannelPtr& channel) override;

  private:
    void Listener();
    void Worker();
    void CloseSocket();
    void ConnectionWorker(int socket);

    std::string GrabStat(uint64_t timestamp);
  };
}
