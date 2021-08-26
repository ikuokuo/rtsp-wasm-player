#include "ws_stream_server.h"

#ifdef __cplusplus
extern "C" {
#endif

#include <libavformat/avformat.h>

#ifdef __cplusplus
}
#endif

#include <glog/logging.h>

#include <utility>

#define NET_JSON_STREAM_INFO_IGNORE
#include "common/net/json.h"

namespace asio = boost::asio;
namespace beast = boost::beast;
namespace http = beast::http;
namespace websocket = beast::websocket;
using tcp = boost::asio::ip::tcp;

WsStreamServer::WsStreamServer(const WsServerOptions &options)
  : WsServer(options),
    cors_(options.cors.enabled
        ? std::make_shared<net::Cors<>>(options.cors)
        : nullptr) {
}

WsStreamServer::~WsStreamServer() {
}

void WsStreamServer::Send(
    const std::string &id,
    const std::shared_ptr<Stream> &stream,
    const AVMediaType &type,
    AVPacket *packet) {
  (void)stream;
  if (stream_map_.find(id) == stream_map_.end()) {
    stream_map_.emplace(id, stream);
    datas_map_.emplace(id, std::make_shared<data_queue_t>(1));
    LOG(INFO) << "Stream[" << id << "] start";
  } else {
    // need update if stream loop
    stream_map_[id] = stream;
  }
  datas_map_[id]->Put(std::make_shared<data_t>(type, packet));
}

bool WsStreamServer::OnHandleHttpRequest(
    http_req_t &req,
    ws_ext::send_lambda &send) {
  (void)send;

  auto target = req.target();
  if (target == "/streams") {
    LOG(INFO) << "http req: " << target;
    http::response<http::string_body> res{
        http::status::ok, req.version()};
    if (cors_ && cors_->Handle(req, res)) {
      send(std::move(res));
      return true;
    }
    res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
    res.set(http::field::content_type, "application/json");
    res.keep_alive(req.keep_alive());
    res.body() = net::to_string(stream_map_);
    res.prepare_payload();
    send(std::move(res));
    return true;
  }

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
  bool ws_ok;
  for (;;) {
    ws_ok = true;
    for (auto &&data : data_queue->TakeAll()) {
      if (!SendData(ws, stream_id, data, ec, yield)) {
        ws_ok = false;
        break;
      }
    }
    if (!ws_ok) break;
  }

  return true;
}

bool WsStreamServer::SendData(
    boost::beast::websocket::stream<boost::beast::tcp_stream> &ws,
    const std::string &id,
    const data_p &data,
    boost::beast::error_code &ec,
    boost::asio::yield_context yield) {
  VLOG(1) << "Stream[" << id << "] packet size=" << data->packet->size;

  ws.binary(true);
  ws.async_write(asio::buffer(data->ToBytes()), yield[ec]);

  if (ec == websocket::error::closed) return false;
  if (ec) {
    OnFail(ec, "write");
    return false;
  }

  return true;
}
