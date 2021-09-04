#include "ws_stream_server.h"

#include <utility>
#include <vector>

#define NET_JSON_STREAM_INFO_IGNORE
#include "common/net/json.h"
#include "common/net/packet.h"
#include "common/util/log.h"

#include "ws_stream_room.h"
#include "ws_stream_session.h"

WsStreamServer::WsStreamServer(const WsServerOptions &options)
  : WsServer(options),
    cors_(options.cors.enabled
        ? std::make_shared<net::Cors<>>(options.cors)
        : nullptr),
    room_(std::make_shared<WsStreamRoom>()) {
  VLOG(2) << __func__;
}

WsStreamServer::~WsStreamServer() {
  VLOG(2) << __func__;
}

void WsStreamServer::Send(
    const std::string &id,
    const std::shared_ptr<Stream> &stream,
    const AVMediaType &type,
    AVPacket *packet) {
  (void)stream;
  if (stream_map_.find(id) == stream_map_.end()) {
    stream_map_.emplace(id, stream);
    LOG(INFO) << "Stream[" << id << "] start";
  } else {
    // need update if stream loop
    stream_map_[id] = stream;
  }
  auto data = std::make_shared<std::vector<uint8_t>>();
  net::Data(type, packet).ToBytes(*data);
  room_->Send(id, data);
}

void WsStreamServer::DoSessionWebSocket(
    beast::websocket::stream<beast::tcp_stream> &&ws,
    boost::optional<http_req_t> &&http_req) {
  assert(http_req.has_value());

  auto req = http_req.get();
  auto target = req.target();
  if (!target.starts_with(options_.stream.ws_target_prefix)) {
    LOG(ERROR) << "ws stream denied: " << target;
    return;
  }

  static auto stream_path_len = options_.stream.ws_target_prefix.size();
  auto stream_id = std::string(target.substr(stream_path_len));
  LOG(INFO) << "ws stream granted, id=" << stream_id;
  LOG(INFO) << " client, ip=" << ws.next_layer().socket().remote_endpoint();

  if (stream_map_.find(stream_id) == stream_map_.end()) {
    LOG(WARNING) << "ws stream not found, id=" << stream_id;
    return;
  }

  auto s = std::make_shared<WsStreamSession>(
      std::move(ws), std::move(req), stream_id, room_);
  s->SetEventCallback(net::NET_EVENT_FAIL,
      [this](const std::shared_ptr<WsStreamSession::event_t> &event) {
        auto e = std::dynamic_pointer_cast<net::NetFailEvent>(event);
        OnFail(e->ec, e->what.c_str());
      });
  s->Run();
}

bool WsStreamServer::OnHandleHttpRequest(
    http_req_t &req,
    net::send_lambda &send) {
  (void)send;

  auto target = req.target();
  if (target == options_.stream.http_target) {
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
