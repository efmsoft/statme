#pragma once

#include <memory>
#include <mutex>

#include <Syncme/Config/Config.h>
#include <Syncme/Sync.h>
#include <Syncme/ThreadPool/Pool.h>

#include <Statme/Macros.h>
#include <Statme/Runtime/Topic.h>

namespace Runtime
{
  class Broker;
  typedef std::shared_ptr<Broker> BrokerPtr;
  typedef void* Cookie;

  class Broker : public std::enable_shared_from_this<Broker>
  {
    static Broker* Instance;

    std::mutex Lock;
    TopicList Topics;

    Syncme::ThreadPool::Pool& Pool;
    HEvent& StopEvent;
    Syncme::ConfigPtr Config;

    int Socket;
    HEvent ListenerThread;
    HEvent WorkerThread;
    bool Exiting;

  public:
    STATMELNK Broker(Syncme::ThreadPool::Pool& pool, HEvent& stopEvent);
    STATMELNK ~Broker();

    STATMELNK static BrokerPtr GetInstance();
    STATMELNK Cookie RegisterTopic(const char* name, TPrint print, const StringList& subtopics = StringList());
    STATMELNK void UnregisterTopic(Cookie);

    STATMELNK void SetSocketConfig(Syncme::ConfigPtr config);

    STATMELNK bool Start(int port);
    STATMELNK void Stop();

  private:
    void Listener();
    void CloseSocket();
    void ConnectionWorker(int socket);
  };
}

#define RUNTIME_TOPIC_REGISTER(n, p, s) \
  Runtime::Broker::GetInstance()->RegisterTopic(n, p, s)

#define RUNTIME_TOPIC_UNREGISTER(c) \
  Runtime::Broker::GetInstance()->UnregisterTopic(c)
