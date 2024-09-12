#pragma once

#include <memory>
#include <mutex>
#include <stdint.h>
#include <vector>

#include <openssl/ssl.h>

#include <Syncme/Config/Config.h>
#include <Syncme/Sync.h>
#include <Syncme/ThreadPool/Pool.h>

#include <Statme/http/Headers.h>
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

    std::string Login;
    std::string Pass;
    std::vector<uint8_t> Key;

    Syncme::ThreadPool::Pool& Pool;
    HEvent& StopEvent;
    Syncme::ConfigPtr Config;

    int Socket;
    int SocketSSL;
    HEvent ListenerThread;
    HEvent ListenerThreadSSL;
    HEvent WorkerThread;
    HEvent WorkerThreadSSL;
    bool Exiting;

    X509* SSLCert;
    EVP_PKEY* SSLKey;

  public:
    STATMELNK Broker(Syncme::ThreadPool::Pool& pool, HEvent& stopEvent);
    STATMELNK ~Broker();

    STATMELNK static BrokerPtr GetInstance();
    STATMELNK Cookie RegisterTopic(const char* name, TPrint print, const StringList& subtopics = StringList());
    STATMELNK void UnregisterTopic(Cookie);

    STATMELNK void SetSocketConfig(Syncme::ConfigPtr config);
    STATMELNK void SetLoginData(const std::string& login, const std::string& pass);

    STATMELNK bool Start(const std::string& ip, int port);
    STATMELNK bool StartSSL(const std::string& ip, int port, X509* cert, EVP_PKEY* key);

    STATMELNK void Stop();

  private:
    void Listener(int socket);
    void CloseSocket();
    void CloseSocketSSL();
    void ConnectionWorker(int socket, int server_socket);

    bool AcceptsHtml(const HTTP::Header::ReqHeaders& req) const;
    std::string ProcessRequest(
      const HTTP::Header::ReqHeaders& req
      , const std::string& peerIP
    );
    std::string CreateToken(const std::string& peerIP) const;
    bool VerifyToken(const std::string& token, const std::string& peerIP) const;
    bool VerifyAuthorization(const std::string& auth) const;
    std::string GetToken(const HTTP::Header::ReqHeaders& req);

    typedef std::vector<std::string> StringArray;
    StringArray SplitUrl(const std::string& url);
    TopicPtr GetTopic(const StringArray& uri);

    int PrepareSocket(const std::string& ip, int port);

    void GenerateKey();
  };
}

#define RUNTIME_TOPIC_REGISTER(n, p, s) \
  (Runtime::Broker::GetInstance() ? Runtime::Broker::GetInstance()->RegisterTopic(n, p, s) : nullptr)

#define RUNTIME_TOPIC_UNREGISTER(c) \
  { if (Runtime::Broker::GetInstance() && c) { Runtime::Broker::GetInstance()->UnregisterTopic(c); } }
