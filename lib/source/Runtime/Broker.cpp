#include <algorithm>
#include <cassert>

#include <Logme/Logme.h>

#include <Syncme/Logger/Log.h>
#include <Syncme/ProcessThreadId.h>
#include <Syncme/SetThreadName.h>
#include <Syncme/Sockets/API.h>
#include <Syncme/Sockets/SocketPair.h>
#include <Syncme/Sockets/SSLHelpers.h>
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
  , SocketSSL(-1)
  , ListenerThread(nullptr)
  , ListenerThreadSSL(nullptr)
  , WorkerThread(nullptr)
  , WorkerThreadSSL(nullptr)
  , Exiting(false)
  , SSLCert(0)
  , SSLKey(0)
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

void Broker::CloseSocketSSL()
{
  if (SocketSSL != -1)
  {
    shutdown(SocketSSL, SD_RECEIVE);

    closesocket(SocketSSL);
    SocketSSL = -1;
  }
}

void Broker::SetLoginData(const std::string& login, const std::string& pass)
{
  Login = login;
  Pass = pass;
}

int Broker::PrepareSocket(const std::string& ip, int port)
{
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
    return -1;
  }

  struct sockaddr_in* sockAddr_ipv4 = nullptr;
  sockAddr_ipv4 = (struct sockaddr_in*)addr->ai_addr;
  LogI(
    "RT server IPv4 Address : %s:%i"
    , inet_ntoa(sockAddr_ipv4->sin_addr)
    , ntohs(sockAddr_ipv4->sin_port)
  );

  int sock = (int)socket(AF_INET, SOCK_STREAM, 0);
  if (sock == -1)
  {
    LogosE("WSASocket() failed");

    freeaddrinfo(addr);
    return -1;
  }
  
  int reuse = 1;
  if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuse, sizeof(reuse)) < 0)
  {
    LogosE("setsockopt(SO_REUSEADDR) failed");
    
    freeaddrinfo(addr);
    return -1;
  }

  rc = bind(sock, addr->ai_addr, (int)addr->ai_addrlen);
  if (rc)
  {
    LogosE("bind() failed");

    shutdown(sock, SD_RECEIVE);

    closesocket(sock);

    freeaddrinfo(addr);
    return -1;
  }

  freeaddrinfo(addr);

  return sock;
}

bool Broker::Start(const std::string& ip /* 127.0.0.1 */, int port)
{
  Socket = PrepareSocket(ip, port);
  if (Socket == -1)
  {
    LogosE("WSASocket() failed");
    return false;
  }

  if (listen(Socket, SOMAXCONN) == -1)
  {
    LogosE("listen() failed");

    CloseSocket();
    return false;
  }

  ListenerThread = Pool.Run(std::bind(&Broker::Listener, this, Socket));
  if (ListenerThread == nullptr)
  {
    LogE("ThreadPool::Run() failed");

    CloseSocket();
    return false;
  }

  LogI("http server started, socket %d", Socket);
  return true;
}

bool Broker::StartSSL(const std::string& ip /* 0.0.0.0 */, int port, X509* cert, EVP_PKEY* key)
{
  SSLCert = cert;
  SSLKey = key;

  if (!SSLCert || !SSLKey)
  {
    LogosE("Incorrect arguments");
    return false;
  }

  SocketSSL = PrepareSocket(ip, port);
  if (SocketSSL == -1)
  {
    LogosE("WSASocket() failed");
    return false;
  }

  if (listen(SocketSSL, SOMAXCONN) == -1)
  {
    LogosE("listen() failed");

    CloseSocketSSL();
    return false;
  }

  ListenerThreadSSL = Pool.Run(std::bind(&Broker::Listener, this, SocketSSL));
  if (ListenerThreadSSL == nullptr)
  {
    LogE("ThreadPool::Run() failed");

    CloseSocketSSL();
    return false;
  }

  LogI("https server started, socket %d", SocketSSL);
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
  CloseSocketSSL();

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

  if (WorkerThreadSSL)
  {
    auto rc = WaitForSingleObject(WorkerThreadSSL, FOREVER);
    assert(rc == WAIT_RESULT::OBJECT_0);

    CloseHandle(WorkerThreadSSL);
  }

  if (ListenerThreadSSL)
  {
    auto rc = WaitForSingleObject(ListenerThreadSSL, FOREVER);
    assert(rc == WAIT_RESULT::OBJECT_0);

    CloseHandle(ListenerThreadSSL);
  }

}

void Broker::Listener(int sock)
{
  const char* tName = "RT listener";
  if (sock != Socket)
    tName = "RT SSL listener";

  SET_CUR_THREAD_NAME_EX(tName);

  ThreadsList threads;
  while (WaitForSingleObject(StopEvent, 0) != WAIT_RESULT::OBJECT_0)
  {
    int accept_sock = (int)accept(sock, nullptr, nullptr);
    if (accept_sock == -1)
    {
      if (!Exiting)
        LogosE("accept() failed");

      break;
    }

    HEvent h = Pool.Run(std::bind(&Broker::ConnectionWorker, this, accept_sock, sock));
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

bool Broker::ProcessRequest(
  const HTTP::Header::ReqHeaders& req
  , const std::string& peerIP
  , std::string_view reqBody
  , std::string& resBody
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
        resBody = unauthorized.Data();
        return true;
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

    resBody = redirect.Data();
    return true;
  }

  StringArray url = SplitUrl(req.Uri);
  if (url.empty())
  {
    REDIRECT redirect("./home");
    if (!token.empty())
      redirect.Headers.SetHeader("Set-Cookie", cookie);

    resBody = redirect.Data();
    return true;
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
    f->AddTOCItem(true, "&#127968;", ".");

    for (auto& t : Topics)
    {
      if (t != topic)
        f->AddTOCItem(false, t->Name, "./" + t->Name);
    }

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
    {
      resBody = NotFound.Data();
      return false;
    }

    std::string arg1 = uri.size() > 1 ? uri[1] : "";
    std::string arg2 = uri.size() > 2 ? uri[2] : "";
    
    std::string rel = arg1.empty() ? "./" : "../";
    f->Rel = rel;

    f->AddTOCItem(false, "&#127968;", rel + "home");
    f->AddTOCItem(arg1.empty(), topic->Name, rel + topic->Name);

    for (auto& s : topic->Subtopics)
      f->AddTOCItem(s == arg1, s, rel + topic->Name + "/" + s);
    
    try
    {
      if (!topic->Print(*f, arg1, arg2))
      {
        resBody = InternalServerError.Data();
        return true;
      }
    }
    catch (const std::exception& ex)
    {
      auto text = Model::IParagraph::Create(std::string("Server Exception:\n") + ex.what());
      f->GetMainPage().AddContent(text);
    }
  }

  if (!token.empty())
    ok.Headers.SetHeader("Set-Cookie", cookie);
  
  resBody = ok.Data();
  return true;
}

static SSL_CTX* CreateClientContext(X509* cert, EVP_PKEY* key)
{
  SSL_CTX* ctx = SSL_CTX_new((SSL_METHOD*)TLS_server_method());
  if (ctx == nullptr)
  {
    LogE("SSL_CTX_new() failed, %s", GetBioError().c_str());
    return 0;
  }

  int64_t options = SSL_CTX_get_options(ctx);
  SSL_CTX_set_options(
    ctx
    , options 
    | SSL_OP_ALL
    | SSL_OP_IGNORE_UNEXPECTED_EOF 
    | SSL_OP_LEGACY_SERVER_CONNECT
  );

  int mode = SSL_CTX_get_mode(ctx);
  SSL_CTX_set_mode(ctx, mode | SSL_MODE_ACCEPT_MOVING_WRITE_BUFFER);

    /* Set the key and cert */
  if (SSL_CTX_use_certificate(ctx, cert) != 1)
  {
    LogE("SSL_CTX_use_certificate() failed, %s", GetBioError().c_str());
    SSL_CTX_free(ctx);
    return 0;
  }

  if (SSL_CTX_use_PrivateKey(ctx, key) != 1)
  {
    LogE("SSL_CTX_use_PrivateKey() failed, %s", GetBioError().c_str());
    SSL_CTX_free(ctx);
    return 0;
  }

  if (SSL_CTX_check_private_key(ctx) != 1)
  {
    LogE("SSL_check_private_key() failed, %s", GetBioError().c_str());
    SSL_CTX_free(ctx);
    return 0;
  }

  return ctx;
}

static SSL* CreateClientSSL(SSL_CTX* ctx, int fd)
{
  SSL* ssl = SSL_new(ctx);
  if (ssl == nullptr)
  {
    LogE("SSL_new() failed, %s", GetBioError().c_str());
    return 0;
  }

  if (SSL_set_fd(ssl, fd) != 1)
  {
    LogE("SSL_set_fd() failed, %s", GetBioError().c_str());
    SSL_free(ssl);
    return 0;
  }

  if (SSL_accept(ssl) != 1)
  {
    LogE("SSL_accept() failed, %s", GetBioError().c_str());
    SSL_free(ssl);
    return 0;
  }

  return ssl;
}

void Broker::ConnectionWorker(int socket, int server_socket)
{
  using namespace HTTP::Response;

  const char* tName = "RT connection";
  if (server_socket != Socket)
    tName = "RT SSL connection";

  SET_CUR_THREAD_NAME_EX(tName);

  SSL_CTX* clientContext = 0;
  SSL* clientSSL = 0;

  Logme::ID ch = CH;
  SocketPair pair(ch, StopEvent, Config);

  if (server_socket != Socket)
  {
    clientContext = CreateClientContext(SSLCert, SSLKey);
    if (!clientContext)
    {
      LogE("%s: failed to create client context (%d, %d)", tName, socket, server_socket);
      return;
    }
    clientSSL = CreateClientSSL(clientContext, socket);
    if (!clientSSL)
    {
      LogE("%s: failed to create client SSL (%d, %d)", tName, socket, server_socket);
      SSL_CTX_free(clientContext);
      return;
    }

    pair.Client = pair.CreateSSLSocket(clientSSL);
  }
  else
  {
    pair.Client = pair.CreateBIOSocket();
  }

  if (!pair.Client->Attach(socket))
  {
    LogE("%s: failed to attach socket (%d, %d)", tName, socket, server_socket);
    if (clientSSL)
      SSL_free(clientSSL);

    if (clientContext)
      SSL_CTX_free(clientContext);
    return;
  }

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
    auto vtype = HTTP::Header::Verification::NotStrict;
    if (req.Parse(rdata.c_str(), rdata.size(), vtype) != HEADER_ERROR::NONE)
      break;

    std::string res;
    std::string_view reqBody(rdata);
    reqBody.remove_prefix(pos);
    
    bool processed = false;
    if (req.Method == "GET" && req.Protocol.Major == 1)
    {
      processed = ProcessRequest(req, pair.Client->Peer.IP, reqBody, res);
    }

    if (!processed)
    {
      if (UnprocessedPrint && !UnprocessedPrint(req, reqBody, res))
        res = NotFound.Data();
    }
    pair.Client->WriteStr(res);
  }

  if (clientSSL)
    SSL_free(clientSSL);

  if (clientContext)
    SSL_CTX_free(clientContext);

}

void Broker::RegisterUnprocessedTopic(TUnprocessedPrint print)
{
  UnprocessedPrint = std::move(print);
}

void Broker::UnregisterUnprocessedTopic()
{
  UnprocessedPrint = nullptr;
}
