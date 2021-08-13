#pragma once

#include <functional>
#include <string>
#include <memory>

#include "common/media/stream_thread.h"

#ifdef __cplusplus
extern "C" {
#endif

struct AVBSFContext;
struct AVPacket;

#ifdef __cplusplus
}
#endif

class StreamHandler {
 public:
  using packet_callback_t =
      std::function<void(Stream::stream_sub_t, AVPacket *)>;

  StreamHandler(const std::string &id,
                const StreamOptions &options,
                int get_frequency,
                packet_callback_t cb = nullptr);
  ~StreamHandler();

  void Start();
  void Stop();

 private:
  void OnEvent(std::shared_ptr<StreamEvent> e);
  void OnEventPacket(std::shared_ptr<StreamPacketEvent> e);

  std::string id_;
  StreamOptions options_;
  int get_frequency_;
  packet_callback_t packet_cb_;

  std::shared_ptr<StreamThread> stream_;

  AVBSFContext *bsf_ctx_;
  AVPacket *bsf_packet_;
};
