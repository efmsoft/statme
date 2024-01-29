#include <cassert>

#include <Logme/Logme.h>

#include <Syncme/Logger/Log.h>
#include <Syncme/ProcessThreadId.h>
#include <Syncme/SetThreadName.h>
#include <Syncme/Sockets/API.h>
#include <Syncme/Sockets/SocketPair.h>
#include <Syncme/ThreadPool/ThreadList.h>
#include <Syncme/TickCount.h>

#include <Statme/Counters/Counters.h>
#include <Statme/Runtime/Broker.h>

using namespace Runtime;
using namespace Syncme;

Broker::Broker(Syncme::ThreadPool::Pool& pool, HEvent& stopEvent)
  : Pool(pool)
  , StopEvent(stopEvent)
  , Socket(-1)
  , ListenerThread(nullptr)
  , Exiting(false)
{
  Instance = this;
}

Broker::~Broker()
{
  CloseSocket();
}

BrokerPtr Broker::GetInstance()
{
  return Instance->shared_from_this();
}

void Broker::SetSocketConfig(Syncme::ConfigPtr config)
{
  Config = config;
}

Cookie Broker::RegisterTopic(
  const char* name
  , TPrint print
  , const StringList& subtopics
)
{
  std::lock_guard lock(Lock);

  for (auto& t : Topics)
  {
    if (t->Name == name)
    {
      assert(!"topic is already registered!");
      return nullptr;
    }
  }

  TopicPtr topic = std::make_shared<Topic>(name, print, subtopics);
  Topics.push_back(topic);

  return topic.get();
}

void Broker::UnregisterTopic(Cookie cookie)
{
  std::lock_guard lock(Lock);

  for (auto it = Topics.begin(); it != Topics.end(); ++it)
  {
    auto& t = *it;

    if (t.get() == cookie)
    {
      Topics.erase(it);
      return;
    }
  }
}

void Broker::CloseSocket()
{
  Exiting = true;

  if (Socket != -1)
  {
    shutdown(Socket, SD_RECEIVE);

    closesocket(Socket);
    Socket = -1;
  }
}

bool Broker::Start(int port)
{
  struct addrinfo hints {};
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

  ListenerThread = Pool.Run(std::bind(&Broker::Listener, this));
  if (ListenerThread == nullptr)
  {
    LogE("ThreadPool::Run() failed");

    CloseSocket();
    return false;
  }

  return true;
}

void Broker::Stop()
{
  if (GetEventState(StopEvent) == STATE::NOT_SIGNALLED)
  {
    // !?!?!
    SetEvent(StopEvent);
  }

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

void Broker::Listener()
{
  SET_CUR_THREAD_NAME_EX("Counters listener");

  ThreadsList threads;
  while (WaitForSingleObject(StopEvent, 0) != WAIT_RESULT::OBJECT_0)
  {
    int accept_sock = (int)accept(Socket, nullptr, nullptr);
    if (accept_sock == -1)
    {
      if (!Exiting)
        LogosE("accept() failed");

      break;
    }

    HEvent h = Pool.Run(std::bind(&Broker::ConnectionWorker, this, accept_sock));
    if (h == nullptr)
    {
      LogE("ThreadPool::Run() failed");

      closesocket(accept_sock);
      continue;
    }

    threads.Add(h);
  }
}

void Broker::ConnectionWorker(int socket)
{
  Logme::ID ch = CH;
  SocketPair pair(ch, StopEvent, Config);

  pair.Client = pair.CreateBIOSocket();
  pair.Client->Attach(socket);
  pair.Client->Configure();

  uint64_t lastReq = GetTimeInMillisec();

  while (WaitForSingleObject(StopEvent, 0) != WAIT_RESULT::OBJECT_0)
  {
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

    Json::Value res;
    res["ok"] = false;
    res["descr"] = "unsupported command";
    pair.Client->WriteStr(Json::FastWriter().write(res));
  }
}
