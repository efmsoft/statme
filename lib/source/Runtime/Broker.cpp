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
#include <Statme/http/Response/Generator.h>
#include <Statme/Runtime/Broker.h>

#if defined(_WIN32) || defined(_WIN64)
/* We are on Windows */
# define strtok_r strtok_s
#else
#include <string.h>
#endif 

#include <FaviconArray.hpp>
#include <StyleArray.hpp>

using namespace Runtime;
using namespace Syncme;

Broker* Broker::Instance = nullptr;

Broker::Broker(Syncme::ThreadPool::Pool& pool, HEvent& stopEvent)
  : Key(32)
  , Pool(pool)
  , StopEvent(stopEvent)
  , Socket(-1)
  , ListenerThread(nullptr)
  , Exiting(false)
{
  Instance = this;

  GenerateKey();
}

Broker::~Broker()
{
  CloseSocket();
}

BrokerPtr Broker::GetInstance()
{
  if (Instance == nullptr)
    return BrokerPtr();

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

bool Broker::Start(
  const std::string& ip
  , int port
  , const std::string& login
  , const std::string& pass
)
{
  Login = login;
  Pass = pass;

  struct addrinfo hints {};
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = IPPROTO_TCP;
  hints.ai_flags = AI_PASSIVE;

  std::string port_str = std::to_string(port);

  struct addrinfo* addr = nullptr;
  int rc = getaddrinfo(ip.c_str(), port_str.c_str(), &hints, &addr);
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
  SET_CUR_THREAD_NAME_EX("RT listener");

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

bool Broker::AcceptsHtml(const HTTP::Header::ReqHeaders& req) const
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

Broker::StringArray Broker::SplitUrl(const std::string& url)
{
  StringArray arr;
  std::string uri(url);

  char* nt = nullptr;
  for (char* p = strtok_r(&uri[0], "/", &nt); p; p = strtok_r(nullptr, "/", &nt))
    arr.push_back(p);

  return arr;
}

TopicPtr Broker::GetTopic(const StringArray& uri)
{
  if (uri.empty())
    return TopicPtr();

  std::lock_guard lock(Lock);

  auto& topic = uri[0];
  for (auto& t : Topics)
  {
    if (t->Name == topic)
      return t;
  }
  return TopicPtr();
}

std::string Broker::GetToken(const HTTP::Header::ReqHeaders& req)
{
  auto h = req.GetHeader("Cookie", false, "; ");
  if (h == nullptr)
    return std::string();

  for (std::string v : *h)
  {
    char* ctx = nullptr;
    char* p0 = strtok_r(&v[0], "=", &ctx);
    char* p1 = strtok_r(nullptr, " ", &ctx);
    if (p0 == nullptr || p1 == nullptr)
      continue;

    std::string k(p0);
    std::transform(k.begin(), k.end(), k.begin(), ::tolower);
    if (k != "token")
      continue;

    return p1;
  }

  return std::string();
}

std::string Broker::ProcessRequest(
  const HTTP::Header::ReqHeaders& req
  , const std::string& peerIP
)
{
  using namespace HTTP::Response;
  
  std::string token;
  if (!Login.empty() || !Pass.empty())
  {
    token = GetToken(req);
    if (token.empty() || !VerifyToken(token, peerIP))
    {
      token.clear();

      std::string auth = req.GetFirstValue("Authorization", false);
      if (auth.empty() || !VerifyAuthorization(auth))
      {
        UNAUTHORIZED unauthorized;
        unauthorized.Headers.SetHeader("WWW-Authenticate", "Basic realm=\"Authorization Required\"");
        unauthorized.Headers.SetHeader("Content-Length", "0");
        return unauthorized.Data();
      }

      if (token.empty())
        token = CreateToken(peerIP);
    }
  }

  std::string cookie;
  if (!token.empty())
    cookie = "token=" + token + "; Max-Age=" + std::to_string(24LL * 60 * 60);

  if (req.Uri.size() > 1 && req.Uri.ends_with('/'))
  {
    REDIRECT redirect(req.Uri.substr(0, req.Uri.size() - 1));

    if (!token.empty())
      redirect.Headers.SetHeader("Set-Cookie", cookie);

    return redirect.Data();
  }

  StringArray url = SplitUrl(req.Uri);
  if (url.empty())
  {
    REDIRECT redirect("./home");
    if (!token.empty())
      redirect.Headers.SetHeader("Set-Cookie", cookie);

    return redirect.Data();
  }

  OK ok;
  FormatterPtr f = AcceptsHtml(req) ?
    std::make_shared<HtmlFormatter>()
    : (FormatterPtr)std::make_shared<TextFormatter>();

  ok.SetFormatter(f);

  if (url[0] == "home")
  {
    StringArray arr;
    arr.push_back("home");
    TopicPtr topic = GetTopic(arr);

    std::lock_guard lock(Lock);
    f->AddTOCItem(true, "home", ".");

    for (auto& t : Topics)
      f->AddTOCItem(false, t->Name, "./" + t->Name);

    if (topic)
      topic->Print(*f, "", "");
  }
  else if (url[0] == "favicon.ico")
  {
    ok.SetFormatter(
      std::make_shared<StaticFormatter>(
        "image/vnd.microsoft.icon"
        , &Favicon[0]
        , Favicon.size()
      )
    );
  }
  else if (url[0] == "statme.css")
  {
    ok.SetFormatter(
      std::make_shared<StaticFormatter>(
        "text/css"
        , &Style[0]
        , Style.size()
      )
    );
  }
  else
  {
    auto uri = SplitUrl(req.Uri);
    
    TopicPtr topic = GetTopic(uri);
    if (topic == nullptr)
      return NotFound.Data();

    std::string arg1 = uri.size() > 1 ? uri[1] : "";
    std::string arg2 = uri.size() > 2 ? uri[2] : "";
    
    std::string rel = arg1.empty() ? "./" : "../";
    f->Rel = rel;

    f->AddTOCItem(false, "home", rel + "home");
    f->AddTOCItem(arg1.empty(), topic->Name, rel + topic->Name);

    for (auto& s : topic->Subtopics)
      f->AddTOCItem(s == arg1, s, rel + topic->Name + "/" + s);
    
    if (!topic->Print(*f, arg1, arg2))
      return InternalServerError.Data();
  }

  if (!token.empty())
    ok.Headers.SetHeader("Set-Cookie", cookie);
  
  return ok.Data();
}

void Broker::ConnectionWorker(int socket)
{
  SET_CUR_THREAD_NAME_EX("RT connection");

  Logme::ID ch = CH;
  SocketPair pair(ch, StopEvent, Config);

  pair.Client = pair.CreateBIOSocket();
  pair.Client->Attach(socket);
  pair.Client->Configure();
  pair.Client->InitPeer();

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

    std::string res = ProcessRequest(req, pair.Client->Peer.IP);
    pair.Client->WriteStr(res);
  }
}
