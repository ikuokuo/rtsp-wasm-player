#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <utility>

#include "ws_server.h"
#include "common/media/stream.h"
#include "common/util/blocking_queue.h"

class WsStreamServer : public WsServer {
 public:
  struct Data {
    AVPacket *packet;
    Stream::stream_sub_t stream;

    Data(AVPacket *p, Stream::stream_sub_t s)
      : packet(av_packet_clone(p)), stream(std::move(s)) {
    }
    ~Data() {
      av_packet_free(&packet);
    }
  };

  using data_queue_t = BlockingQueue<std::shared_ptr<Data>>;

  explicit WsStreamServer(const WsServerOptions &options);
  ~WsStreamServer() override;

  void Push(const std::string &id,
            const Stream::stream_sub_t &stream,
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

  std::unordered_map<std::string, std::shared_ptr<data_queue_t>> datas_map_;
};
