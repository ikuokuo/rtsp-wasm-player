#include "ws_stream_client.h"

#include <glog/logging.h>

#include <algorithm>

WsStreamClient::WsStreamClient(const WsClientOptions &options)
  : WsClient(options), packet_(av_packet_alloc()) {
}

WsStreamClient::~WsStreamClient() {
  av_packet_free(&packet_);
}

bool WsStreamClient::OnRead(boost::beast::flat_buffer *buffer) {
  // type size data, ...
  // 1    4    -
  auto data = buffer->data();
  auto bytes = reinterpret_cast<char *>(data.data());

  auto type = static_cast<AVMediaType>(*(bytes));
  auto size = ((*(bytes + 1) & 0xff) << 24) |
      ((*(bytes + 2) & 0xff) << 16) |
      ((*(bytes + 3) & 0xff) << 8) |
      ((*(bytes + 4) & 0xff));
  buffer->consume(5);

  if (type == AVMEDIA_TYPE_VIDEO) {
    auto data = static_cast<uint8_t *>(av_malloc(size));
    if (data) {
      std::copy(bytes, bytes + size, data);
      av_packet_from_data(packet_, data, size);
      OnPacket(type, packet_);
      av_packet_unref(packet_);
    } else {
      LOG(ERROR) << "av_malloc fail, size=" << size;
    }
  }
  buffer->consume(size);
  return true;
}

void WsStreamClient::OnPacket(const AVMediaType &type, AVPacket *packet) {
  VLOG(1) << "packet type=" << type << ", size=" << packet->size;
}
