#pragma once

#include <memory>
#include <string>
#include <unordered_map>

#include "common/media/stream.h"
#include "common/net/packet.h"
#include "common/util/blocking_queue.h"

#include "ws_server.h"

class WsStreamServer : public WsServer {
 public:
  using data_t = net::Data;
  using data_p = std::shared_ptr<data_t>;
  using data_queue_t = BlockingQueue<data_p>;

  explicit WsStreamServer(const WsServerOptions &options);
  ~WsStreamServer() override;

  void Send(const std::string &id,
            const std::shared_ptr<Stream> &stream,
            const AVMediaType &type,
            AVPacket *packet);

 protected:
  bool OnHandleHttpRequest(
      http_req_t &req,
      ws_ext::send_lambda &send) override;

  bool OnHandleWebSocket(
      beast::websocket::stream<beast::tcp_stream> &ws,
      boost::optional<http_req_t> &req,
      beast::error_code &ec,
      asio::yield_context yield) override;

  bool SendData(
      beast::websocket::stream<beast::tcp_stream> &ws,
      const std::string &id,
      const data_p &data,
      beast::error_code &ec,
      asio::yield_context yield);

  std::unordered_map<std::string, std::shared_ptr<Stream>> stream_map_;
  std::unordered_map<std::string, std::shared_ptr<data_queue_t>> datas_map_;

  std::shared_ptr<net::Cors<>> cors_;
};
