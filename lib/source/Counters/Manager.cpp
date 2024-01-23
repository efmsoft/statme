#include <cassert>

#include <Logme/Logme.h>

#include <Syncme/Logger/Log.h>
#include <Syncme/SetThreadName.h>
#include <Syncme/Sockets/API.h>
#include <Syncme/Sockets/SocketPair.h>
#include <Syncme/ThreadPool/ThreadList.h>
#include <Syncme/TickCount.h>

#include <Statme/Counters/Counter.h>
#include <Statme/Counters/Manager.h>

using namespace Syncme;
using namespace Counters;

Manager::Manager(Syncme::ThreadPool::Pool& pool, HEvent& stopEvent)
  : Modified(0)
  , Pool(pool)
  , StopEvent(stopEvent)
  , Socket(-1)
  , ListenerThread(nullptr)
  , WorkerThread(nullptr)
  , Exiting(false)
{
}

Manager::~Manager()
{
  Deleted.clear();

  assert(ListenerThread == nullptr);
  assert(WorkerThread == nullptr);
  assert(Counters.empty());
  
  CloseSocket();
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

void Manager::CloseSocket()
{
  Exiting = true;

  if (Socket != -1)
  {
    shutdown(Socket, SD_RECEIVE);

    closesocket(Socket);
    Socket = -1;
  }
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

      if (!Exiting)
        Deleted.push_back(counter);

      break;
    }
  }
}

bool Manager::Start(int port)
{
  struct addrinfo hints{};
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = IPPROTO_TCP;
  hints.ai_flags = AI_PASSIVE;

  std::string port_str = std::to_string(port);

  struct addrinfo* addr = nullptr;
  int rc = getaddrinfo("127.0.0.1", port_str.c_str(), &hints, &addr);
  if (rc)
  {
    LogosE("getaddrinfo() failed");
    return false;
  }

  struct sockaddr_in* sockAddr_ipv4 = nullptr;
  sockAddr_ipv4 = (struct sockaddr_in*)addr->ai_addr;
  LogI(
    "Counters server IPv4 Address : %s:%i"
    , inet_ntoa(sockAddr_ipv4->sin_addr)
    , ntohs(sockAddr_ipv4->sin_port)
  );

  Socket = (int)socket(AF_INET, SOCK_STREAM, 0);
  if (Socket == -1)
  {
    LogosE("WSASocket() failed");

    freeaddrinfo(addr);
    return false;
  }

  rc = bind(Socket, addr->ai_addr, (int)addr->ai_addrlen);
  if (rc)
  {
    LogosE("bind() failed");

    CloseSocket();
    freeaddrinfo(addr);
    return false;
  }

  freeaddrinfo(addr);

  if (listen(Socket, SOMAXCONN) == -1)
  {
    LogosE("listen() failed");

    CloseSocket();
    return false;
  }

  ListenerThread = Pool.Run(std::bind(&Manager::Listener, this));
  if (ListenerThread == nullptr)
  {
    LogE("ThreadPool::Run() failed");

    CloseSocket();
    return false;
  }

  WorkerThread = Pool.Run(std::bind(&Manager::Worker, this));
  if (WorkerThread == nullptr)
  {
    LogE("ThreadPool::Run() failed");

    CloseSocket();
    return false;
  }

  return true;
}

void Manager::Stop()
{
  CloseSocket();

  if (WorkerThread)
  {
    auto rc = WaitForSingleObject(WorkerThread, FOREVER);
    assert(rc == WAIT_RESULT::OBJECT_0);

    CloseHandle(WorkerThread);
  }

  if (ListenerThread)
  {
    auto rc = WaitForSingleObject(ListenerThread, FOREVER);
    assert(rc == WAIT_RESULT::OBJECT_0);

    CloseHandle(ListenerThread);
  }
}

void Manager::Worker()
{
  //SET_CUR_THREAD_NAME_EX2(this, "Counters worker");

  for (;;)
  {
    auto rc = WaitForSingleObject(StopEvent, 1000);
    if (rc == WAIT_RESULT::OBJECT_0)
      break;

    auto t = GetTimeInMillisec();

    std::lock_guard<std::mutex> guard(Lock);
    for (bool cont = true; cont;)
    {
      cont = false;
      for (auto it = Deleted.begin(); it != Deleted.end(); ++it)
      {
        auto& c = *it;
        auto d = t - c->Deleted;

        if (d > DELETE_AFTER)
        {
          cont = true;
          Deleted.erase(it);
          SetDirty();
          break;
        }
      }
    }
  }

  std::lock_guard<std::mutex> guard(Lock);
  Deleted.clear();
}

void Manager::Listener()
{
  //SET_CUR_THREAD_NAME_EX2(this, "Counters listener");

  ThreadsList threads;
  for (;;)
  {
    auto rc = WaitForSingleObject(StopEvent, 0);
    if (rc == WAIT_RESULT::OBJECT_0)
      break;

    int accept_sock = (int)accept(Socket, nullptr, nullptr);
    if (accept_sock == -1)
    {
      if (!Exiting)
        LogosE("accept() failed");

      break;
    }

    HEvent h = Pool.Run(std::bind(&Manager::ConnectionWorker, this, accept_sock));
    if (h == nullptr)
    {
      LogE("ThreadPool::Run() failed");

      closesocket(accept_sock);
      continue;
    }

    threads.Add(h);
  }
}

void Manager::ConnectionWorker(int socket)
{
  Logme::ID ch = CH;
  SocketPair pair(ch, StopEvent, Config);

  pair.Client = pair.CreateBIOSocket();
  pair.Client->Attach(socket);
  pair.Client->Configure();

  uint64_t lastReq = GetTimeInMillisec();

  for (;;)
  {
    auto rc = WaitForSingleObject(StopEvent, 0);
    if (rc == WAIT_RESULT::OBJECT_0)
      break;

    std::vector<char> buffer(64 * 1024);
    int n = pair.Client->Read(buffer, 500);
    if (n < 0 || pair.Client->Peer.Disconnected)
      break;

    if (n == 0)
    {
      auto silence = GetTimeInMillisec() - lastReq;

      if (silence > 60000)
      {
        Json::Value res;
        res["ok"] = false;
        res["descr"] = "wake up";
        pair.Client->WriteStr(Json::FastWriter().write(res));

        lastReq = GetTimeInMillisec();
      }

      continue;
    }
    
    std::string cmd(&buffer[0], n);
    lastReq = GetTimeInMillisec();

    Json::Value root;
    Json::Reader reader;
    if (!reader.parse(cmd, root))
      continue;

    std::string command;
    uint64_t timestamp = 0;

    if (root["cmd"].isString())
      command = root["cmd"].asString();

    if (root["timestamp"].isUInt64())
      timestamp = root["timestamp"].asUInt64();

    if (command == "exit")
    {
      pair.Close();
      break;
    }

    if (command == "get")
    {
      std::string json = GrabStat(timestamp);
      
      char buf[64];
      snprintf(buf, sizeof(buf) -1, "Content-Length: %zu\r\n", json.size());
      
      pair.Client->WriteStr(buf);
      pair.Client->WriteStr(json);
      continue;
    }

    Json::Value res;
    res["ok"] = false;
    res["descr"] = "unsupported command";
    pair.Client->WriteStr(Json::FastWriter().write(res));
  }
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
