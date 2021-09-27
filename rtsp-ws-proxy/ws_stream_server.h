#pragma once

#include <memory>
#include <string>
#include <unordered_map>

#include "common/media/stream.h"

#include "ws_server.h"

class WsStreamRoom;

class WsStreamServer : public WsServer {
 public:
  explicit WsStreamServer(const WsServerOptions &options);
  ~WsStreamServer() override;

  void Send(const std::string &id,
            const std::shared_ptr<Stream> &stream,
            const AVMediaType &type,
            AVPacket *packet);

 protected:
  void DoSessionWebSocket(
      ws_stream_t &&ws, boost::optional<http_req_t> &&req) override;

  bool OnHandleHttpRequest(
      http_req_t &req, send_lambda_t &send) override;

  std::shared_ptr<net::Cors<>> cors_;
  std::shared_ptr<WsStreamRoom> room_;
  std::unordered_map<std::string, std::shared_ptr<Stream>> stream_map_;
};
