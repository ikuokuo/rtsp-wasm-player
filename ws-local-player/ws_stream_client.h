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
#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <vector>

#include "ws_client.h"
#include "common/media/stream.h"
#include "common/gl/glfw_frame.h"

class WsStreamClient : public WsClient<std::vector<uint8_t>> {
 public:
  using stream_ops_t =
      std::unordered_map<AVMediaType, std::shared_ptr<StreamOp>>;

  WsStreamClient(asio::io_context &ioc, const WsClientOptions &options,
                 const StreamInfo &info, int ui_wait_secs,
                 std::function<void()> on_ui_exit = nullptr);
  ~WsStreamClient() override;

 protected:
  void Run();
  void OnEventRecv(beast::flat_buffer &buffer, std::size_t bytes_n) override;

  StreamInfo info_;
  stream_ops_t ops_;

  int ui_wait_secs_;
  bool ui_ok_;
  std::shared_ptr<GlfwFrame> ui_;
  GlfwInitParams ui_params_;
  std::mutex ui_mutex_;
  std::condition_variable ui_cond_;
  std::thread ui_thread_;
  std::function<void()> on_ui_exit_;
};
