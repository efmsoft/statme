#include <algorithm>
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
#include <Statme/http/Headers.h>
#include <Statme/Runtime/Broker.h>

#if defined(_WIN32) || defined(_WIN64)
/* We are on Windows */
# define strtok_r strtok_s
#else
#include <string.h>
#endif 

using namespace Runtime;
using namespace Syncme;

Broker* Broker::Instance = nullptr;

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

static bool AcceptsHtml(HTTP::Header::ReqHeaders& req)
{
  auto arr = req.GetHeader("accept");
  if (arr == nullptr)
    return false;

  for (auto& a : *arr)
  {
    std::string s(a);
    std::transform(s.begin(), s.end(), s.begin(), ::tolower);

    if (s == "text/html")
      return true;
  }
  return false;
}

void Broker::ConnectionWorker(int socket)
{
  Logme::ID ch = CH;
  SocketPair pair(ch, StopEvent, Config);

  pair.Client = pair.CreateBIOSocket();
  pair.Client->Attach(socket);
  pair.Client->Configure();

  while (WaitForSingleObject(StopEvent, 0) != WAIT_RESULT::OBJECT_0)
  {
    std::vector<char> buffer(64 * 1024);
    int n = pair.Client->Read(buffer);
    if (n <= 0 || pair.Client->Peer.Disconnected)
      break;

    std::string rdata(&buffer[0], buffer.size());
    size_t pos = Find2CRLF(rdata);
    if (pos <= 0)
      break;

    HTTP::Header::ReqHeaders req;
    if (req.Parse(rdata.c_str(), rdata.size()) != HEADER_ERROR::NONE)
      break;

    if (req.Method != "GET" || req.Protocol.Major != 1)
      break;

    int code = 200;
    std::stringstream ss;
    bool html = AcceptsHtml(req);
    if (req.Uri == "/topics" || req.Uri == "/topics/")
    {
      std::lock_guard lock(Lock);
      ss << "HTTP/1.1 200 OK\r\n\r\n";

      for (auto& t : Topics)
      {
        if (html)
          ss << "<a href=\"" << "/topics/" << t->Name << "\">" << t->Name << "</a><br/>";
        else
          ss << t->Name << "\n";
      }
    }
    else if (req.Uri.starts_with("/topics/"))
    {
      std::string uri(req.Uri);

      char* nt = nullptr;
      char* p = strtok_s(&uri[0], "/", &nt);
      char* t = strtok_s(nullptr, "/", &nt);
      char* a = strtok_s(nullptr, "/", &nt);

      std::string topic(t ? t : "");
      std::string arg(a ? a : "");

      std::lock_guard lock(Lock);
      for (auto& t : Topics)
      {
        if (t->Name == topic)
        {
          ss << "HTTP/1.1 200 OK\r\n\r\n";
          ss << t->Print(topic, arg);
          break;
        }
      }

      if (ss.str().empty())
        ss << "HTTP/1.1 404 NotFound\r\n\r\n";
    }
    else
      ss << "HTTP/1.1 404 NotFound\r\n\r\n";

    pair.Client->WriteStr(ss.str());
    break;
  }
}
