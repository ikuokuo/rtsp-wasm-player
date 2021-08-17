#include "ws_stream_server.h"

#include <glog/logging.h>

namespace asio = boost::asio;
namespace beast = boost::beast;
namespace http = beast::http;
namespace websocket = beast::websocket;
using tcp = boost::asio::ip::tcp;

WsStreamServer::WsStreamServer(const WsServerOptions &options)
  : WsServer(options) {
}

WsStreamServer::~WsStreamServer() {
}

void WsStreamServer::Push(
    const std::string &id,
    const Stream::stream_sub_t &stream,
    AVPacket *packet) {
  (void)stream;
  if (datas_map_.find(id) == datas_map_.end()) {
    datas_map_.emplace(id, std::make_shared<data_queue_t>(1));
    LOG(INFO) << "stream start, id=" << id;
  }
  datas_map_[id]->Put(std::make_shared<Data>(packet, stream));
}

bool WsStreamServer::OnHandleHttpRequest(
    http_req_t &req,
    ws_ext::send_lambda &send) {
  (void)send;

  auto target = req.target();
  LOG(INFO) << "http req: " << target;

  return false;
}

bool WsStreamServer::OnHandleWebSocket(
    boost::beast::websocket::stream<boost::beast::tcp_stream> &ws,
    boost::optional<http_req_t> &http_req,
    boost::beast::error_code &ec,
    boost::asio::yield_context yield) {
  assert(http_req.has_value());

  auto req = http_req.get();
  auto target = req.target();
  if (!target.starts_with("/stream/")) {
    LOG(ERROR) << "ws stream denied: " << target;
    return true;
  }

  static auto stream_path_len = std::string("/stream/").size();
  auto stream_id = std::string(target.substr(stream_path_len));
  LOG(INFO) << "ws stream granted, id=" << stream_id;
  LOG(INFO) << " client, ip=" << ws.next_layer().socket().remote_endpoint();

  if (datas_map_.find(stream_id) == datas_map_.end()) {
    LOG(WARNING) << "ws stream not found, id=" << stream_id;
    return true;
  }

  auto &&data_queue = datas_map_[stream_id];

  for (;;) {
    auto datas = data_queue->TakeAll();

    for (auto &&data : datas) {
      LOG(INFO) << "Stream[" << stream_id << "] packet size="
          << data->packet->size;
    }

    // This buffer will hold the incoming message
    beast::flat_buffer buffer;

    // Read a message
    ws.async_read(buffer, yield[ec]);

    // This indicates that the session was closed
    if (ec == websocket::error::closed) break;

    if (ec) {
      OnFail(ec, "read");
      break;
    }

    // Echo the message back
    ws.text(ws.got_text());
    ws.async_write(buffer.data(), yield[ec]);
    if (ec) {
      OnFail(ec, "write");
      break;
    }
  }

  return true;
}
