#pragma once

#include <memory>
#include <string>

class HttpRequest;

namespace hv
{
  class WebSocketChannel;
  struct WebSocketService;
  class WebSocketServer;
}

namespace Counters
{
  typedef std::shared_ptr<HttpRequest> HttpRequestPtr;
  typedef std::shared_ptr<hv::WebSocketChannel> WebSocketChannelPtr;

  class WebsocketServer
  {
    std::shared_ptr<hv::WebSocketService> Service;
    std::shared_ptr<hv::WebSocketServer> Server;
    bool Started;

  public:
    WebsocketServer();
    virtual ~WebsocketServer();

    bool Start(int port);
    void Stop();

  protected:
    virtual void OnOpen(const WebSocketChannelPtr& channel, const HttpRequestPtr& req) = 0;
    virtual void OnMessage(const WebSocketChannelPtr& channel, const std::string& msg) = 0;
    virtual void OnClose(const WebSocketChannelPtr& channel) = 0;
  };
}