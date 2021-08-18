#pragma once

#include <memory>
#include <string>
#include <unordered_map>

#include "ws_server.h"
#include "common/media/stream.h"
#include "common/util/blocking_queue.h"

class WsStreamServer : public WsServer {
 public:
  struct Data {
    AVMediaType type;
    AVPacket *packet;

    Data(AVMediaType type, AVPacket *p)
      : type(type), packet(av_packet_clone(p)) {
    }
    ~Data() {
      av_packet_free(&packet);
    }
  };

  using data_queue_t = BlockingQueue<std::shared_ptr<Data>>;

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
      boost::beast::websocket::stream<boost::beast::tcp_stream> &ws,
      boost::optional<http_req_t> &req,
      boost::beast::error_code &ec,
      boost::asio::yield_context yield) override;

  bool SendData(
      boost::beast::websocket::stream<boost::beast::tcp_stream> &ws,
      const std::string &id,
      const std::shared_ptr<Data> &data,
      boost::beast::error_code &ec,
      boost::asio::yield_context yield);

  std::unordered_map<std::string, std::shared_ptr<Stream>> stream_map_;
  std::unordered_map<std::string, std::shared_ptr<data_queue_t>> datas_map_;
};
