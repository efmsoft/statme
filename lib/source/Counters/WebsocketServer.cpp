#include <hv/WebsocketServer.h>
#include <hv/HttpMessage.h>

#include <Statme/Counters/WebsocketServer.h>

using namespace Counters;

WebsocketServer::WebsocketServer()
  : Service{std::make_shared<hv::WebSocketService>()}
  , Server{std::make_shared<hv::WebSocketServer>()}
  , Started{false}
{
  Service->onopen = std::bind_front(&WebsocketServer::OnOpen, this);
  Service->onmessage = std::bind_front(&WebsocketServer::OnMessage, this);
  Service->onclose = std::bind_front(&WebsocketServer::OnClose, this);
}

bool WebsocketServer::Start(int port)
{
  Server->port = port;
  Server->registerWebSocketService(Service.get());

  if (Server->start() != 0)
    return false;

  Started = true;
  return true;
}

void WebsocketServer::Stop()
{
  if (!Started)
    return;

  Server->stop();
  Started = false;
}

WebsocketServer::~WebsocketServer()
{
  Stop();
}
