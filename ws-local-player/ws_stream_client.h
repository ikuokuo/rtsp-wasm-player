#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <libavcodec/packet.h>
#include <libavutil/avutil.h>

#ifdef __cplusplus
}
#endif

#include <condition_variable>
#include <memory>
#include <mutex>
#include <unordered_map>

#include "ws_client.h"
#include "common/media/stream.h"
#include "common/gl/glfw_frame.h"

class WsStreamClient : public WsClient {
 public:
  using stream_ops_t =
      std::unordered_map<AVMediaType, std::shared_ptr<StreamOp>>;

  WsStreamClient(const WsClientOptions &options,
      const StreamInfo &info, int ui_wait_secs);
  ~WsStreamClient() override;

  void Run() override;

 protected:
  bool OnRead(beast::flat_buffer *buffer) override;

  StreamInfo info_;
  stream_ops_t ops_;

  int ui_wait_secs_;
  bool ui_ok_;
  std::shared_ptr<GlfwFrame> ui_;
  GlfwInitParams ui_params_;
  std::mutex ui_mutex_;
  std::condition_variable ui_cond_;
};
