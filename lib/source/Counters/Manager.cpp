#include <cassert>
#include <zlib.h>

#include <Logme/Logme.h>
#include <Syncme/Logger/Log.h>
#include <Syncme/TickCount.h>

#include <Statme/Counters/Manager.h>
#include <Statme/http/Base64.hpp>

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

std::string Manager::EncodeMessage(const std::string& data)
{
  std::string gzip = Compress(data);
  return base64::to_base64(gzip);
}

std::string Manager::DecodeMessage(const std::string& msg)
{
  std::string str = base64::from_base64(msg);

  z_stream zs;
  memset(&zs, 0, sizeof(zs));

  if (inflateInit(&zs) != Z_OK)
    return std::string();

  zs.next_in = (Bytef*)str.data();
  zs.avail_in = str.size();

  int ret = 0;
  std::string outstring;
  static char outbuffer[16 * 1024] = { 0 };

  do 
  {
    zs.next_out = reinterpret_cast<Bytef*>(outbuffer);
    zs.avail_out = sizeof(outbuffer);

    ret = inflate(&zs, 0);

    if (outstring.size() < zs.total_out) 
    {
      outstring.append(
        outbuffer
        , zs.total_out - outstring.size()
      );
    }

  } while (ret == Z_OK);

  inflateEnd(&zs);

  if (ret != Z_STREAM_END)
    return std::string();

  return outstring;
}

std::string Manager::Compress(const std::string& str)
{
  int compressionlevel = Z_BEST_COMPRESSION;

  z_stream zs;                        // z_stream is zlib's control structure
  memset(&zs, 0, sizeof(zs));

  if (deflateInit(&zs, compressionlevel) != Z_OK)
    return std::string();

  zs.next_in = (Bytef*)str.data();
  zs.avail_in = str.size();           // set the z_stream's input

  int ret = 0;
  std::string outstring;
  static char outbuffer[16 * 1024] = { 0 };

  do 
  {
    zs.next_out = reinterpret_cast<Bytef*>(outbuffer);
    zs.avail_out = sizeof(outbuffer);

    ret = deflate(&zs, Z_FINISH);

    if (outstring.size() < zs.total_out) 
    {
      // append the block to the output string
      outstring.append(
        outbuffer
        , zs.total_out - outstring.size()
      );
    }

  } while (ret == Z_OK);

  deflateEnd(&zs);

  if (ret != Z_STREAM_END)
    return std::string();

  return outstring;
}

void Manager::OnOpen(const WebSocketChannelPtr& channel, const HttpRequestPtr& req)
{
}

void Manager::OnMessage(const WebSocketChannelPtr& channel, const std::string& msg)
{
  std::string decoded(DecodeMessage(msg));

  Json::Value root;
  Json::Reader reader;
  if (!reader.parse(decoded, root))
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
    channel->send(EncodeMessage(stat));
  }
}

void Manager::OnClose(const WebSocketChannelPtr& channel)
{
}
