#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <libavcodec/packet.h>
#include <libavutil/avutil.h>

#ifdef __cplusplus
}
#endif

#include "ws_client.h"

class WsStreamClient : public WsClient {
 public:
  explicit WsStreamClient(const WsClientOptions &options);
  ~WsStreamClient() override;

 protected:
  bool OnRead(boost::beast::flat_buffer *buffer) override;

  virtual void OnPacket(const AVMediaType &type, AVPacket *packet);

  AVPacket *packet_;
};
