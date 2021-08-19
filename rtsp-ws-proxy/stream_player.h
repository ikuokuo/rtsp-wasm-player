#pragma once

#include <condition_variable>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>

#include "common/media/stream.h"
#include "common/gl/glfw_frame.h"

class StreamPlayer {
 public:
  using stream_ops_t =
      std::unordered_map<AVMediaType, std::shared_ptr<StreamOp>>;

  explicit StreamPlayer(const std::string &id);
  ~StreamPlayer();

  void Start();
  void Stop();

  void Send(const std::string &id,
            const std::shared_ptr<Stream> &stream,
            const AVMediaType &type,
            AVPacket *packet);

 private:
  std::string id_;

  stream_ops_t ops_;

  bool ui_ok_;
  std::shared_ptr<GlfwFrame> ui_;
  GlfwInitParams ui_params_;
  std::mutex ui_mutex_;
  std::condition_variable ui_cond_;
  std::thread ui_thread_;
};
