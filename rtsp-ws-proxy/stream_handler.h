#pragma once

#include <functional>
#include <string>
#include <memory>
#include <vector>

#include "common/media/stream_thread.h"

#include "stream_filter.h"

class StreamHandler {
 public:
  using packet_callback_t = std::function<void(
      const std::shared_ptr<Stream> &, const AVMediaType &, AVPacket *)>;

  StreamHandler(const std::string &id,
                const StreamOptions &options,
                const std::vector<StreamFilterOptions> &filters_options,
                int get_frequency,
                packet_callback_t cb = nullptr);
  ~StreamHandler();

  void Start();
  void Stop();

 private:
  void OnEvent(const std::shared_ptr<StreamEvent> &e);
  void OnRunning(const std::shared_ptr<StreamThread> &t,
                 const std::shared_ptr<Stream> &s);
  void DoFilter(
      const std::vector<std::shared_ptr<StreamFilter>> &filters,
      const std::vector<std::shared_ptr<StreamFilter>>::iterator &filter,
      AVPacket *pkt,
      std::function<void(AVPacket *pkt)> on_recv);
  void InitVideoFilters(const std::shared_ptr<Stream::stream_sub_t> &video);

  std::string id_;
  StreamOptions options_;
  std::vector<StreamFilterOptions> filters_options_;
  int get_frequency_;
  packet_callback_t packet_cb_;

  std::string log_id_;

  std::shared_ptr<StreamThread> stream_;
  bool video_filters_inited_;
  std::vector<std::shared_ptr<StreamFilter>> video_filters_;
  AVPacket *packet_recv_;
};
