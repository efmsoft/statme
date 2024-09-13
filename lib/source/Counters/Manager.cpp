#include <cassert>

#include <Logme/Logme.h>

#include <Syncme/Logger/Log.h>
#include <Syncme/ProcessThreadId.h>
#include <Syncme/SetThreadName.h>
#include <Syncme/Sockets/API.h>
#include <Syncme/Sockets/SocketPair.h>
#include <Syncme/ThreadPool/ThreadList.h>
#include <Syncme/TickCount.h>

#include <Statme/Counters/Counter.h>
#include <Statme/Counters/Counters.h>
#include <Statme/Counters/Holder.h>
#include <Statme/Counters/Manager.h>

#include "WebSocketServerEx.h"

using namespace Syncme;
using namespace Counters;

Manager* Manager::Instance;

Manager::Manager(Syncme::ThreadPool::Pool& pool, HEvent& stopEvent)
  : Modified(0)
  , DeletedEvent(CreateSynchronizationEvent())
  , WorkerTimeout(FOREVER)
  , Pool(pool)
  , StopEvent(stopEvent)
{
  Instance = this;
}

Manager::~Manager()
{
  Deleted.clear();
  CloseHandle(DeletedEvent);
}

ManagerPtr Manager::GetInstance()
{
  if (Instance == nullptr)
    return ManagerPtr();

  return Instance->shared_from_this();
}

std::mutex& Manager::GetLock()
{
  return Lock;
}

void Manager::SetSocketConfig(ConfigPtr config)
{
  Config = config;
}

void Manager::SetDirty()
{
  Modified = GetTimeInMillisec();
}

CounterPtr Manager::AddCounter(
  const std::string& name
  , const char* category
)
{
  return AddCounter(name.c_str(), category);
}

CounterPtr Manager::AddCounter(
  const char* name
  , const char* category
)
{
  std::lock_guard<std::mutex> guard(Lock);

  CounterPtr counter = std::make_shared<Counter>(this, name, category);
  Counters.push_back(counter);
  
  SetDirty();

  return counter;
}

void Manager::DeleteCounter(CounterPtr counter)
{
  std::lock_guard<std::mutex> guard(Lock);

  for (auto it = Counters.begin(); it != Counters.end(); ++it)
  {
    if (counter.get() == it->get())
    {
      Counters.erase(it);
      counter->Deleted = GetTimeInMillisec();

      SetEvent(DeletedEvent);
      break;
    }
  }
}

bool Manager::Start(int port)
{
  return WebsocketServer::Start(port);
}

void Manager::Stop()
{
  WebsocketServer::Stop();
}

std::string Manager::GrabStat(uint64_t timestamp)
{
  Json::Value root;
  root["ok"] = true;
  root["timestamp"] = Modified;
  root["descr"] = "";

  if (timestamp < Modified)
  {
    Json::Value item(Json::arrayValue);
    std::lock_guard<std::mutex> guard(Lock);

    for (auto& c : Counters)
      item.append(c->Get());

    for (auto& c : Deleted)
      item.append(c->Get());

    root["item"] = item;
  }
  else
  {
    root["ok"] = false;
    root["descr"] = "up to date";
  }

  return Json::FastWriter().write(root);
}

void Manager::OnOpen(const WebSocketChannelPtr& channel, const HttpRequestPtr& req)
{
}

void Manager::OnMessage(const WebSocketChannelPtr& channel, const std::string& msg)
{
  Json::Value root;
  Json::Reader reader;
  if (!reader.parse(msg, root))
    return;

  std::string command;
  uint64_t timestamp = 0;

  if (root["cmd"].isString())
    command = root["cmd"].asString();

  if (root["timestamp"].isUInt64())
    timestamp = root["timestamp"].asUInt64();

  if (command == "get")
  {
    std::string stat = GrabStat(timestamp);
    channel->send(stat);
  }
}

void Manager::OnClose(const WebSocketChannelPtr& channel)
{
}
