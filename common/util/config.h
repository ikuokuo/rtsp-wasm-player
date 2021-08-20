#pragma once

#include <yaml-cpp/yaml.h>

#include <string>

#ifndef UTIL_CONFIG_STREAM_IGNORE

#include "common/media/stream_def.h"

namespace YAML {

template <>
struct convert<StreamVideoOptions> {
  static Node encode(const StreamVideoOptions &opts) {
    Node node;
    node["sws_enable"] = opts.sws_enable;
    node["sws_dst_width"] = opts.sws_dst_width;
    node["sws_dst_height"] = opts.sws_dst_height;
    node["sws_dst_pix_fmt"] = PixelFormatToString(opts.sws_dst_pix_fmt);
    node["sws_flags"] = opts.sws_flags;
    return node;
  }

  static bool decode(const Node &node, StreamVideoOptions &opts) {  // NOLINT
    if (!node.IsMap()) {
      return false;
    }
    if (node["sws_enable"])
      opts.sws_enable = node["sws_enable"].as<bool>();
    if (node["sws_dst_width"])
      opts.sws_dst_width = node["sws_dst_width"].as<int>();
    if (node["sws_dst_height"])
      opts.sws_dst_height = node["sws_dst_height"].as<int>();
    if (node["sws_dst_pix_fmt"])
      opts.sws_dst_pix_fmt =
        PixelFormatFromString(node["sws_dst_pix_fmt"].as<std::string>());
    if (node["sws_flags"])
      opts.sws_flags = node["sws_flags"].as<int>();
    return true;
  }
};

template <>
struct convert<StreamOptions> {
  static Node encode(const StreamOptions &opts) {
    Node node;
    node["method"] = StreamMethodToString(opts.method);
    node["input_url"] = opts.input_url;

    node["input_format"] = opts.input_format;
    node["width"] = opts.width;
    node["height"] = opts.height;
    node["framerate"] = opts.framerate;
    node["pixel_format"] = PixelFormatToString(opts.pixel_format);

    node["rtbufsize"] = opts.rtbufsize;

    node["video"] = opts.video;
    return node;
  }

  static bool decode(const Node &node, StreamOptions &opts) {  // NOLINT
    if (!node.IsMap()) {
      return false;
    }

    opts.method = StreamMethodFromString(node["method"].as<std::string>());
    opts.input_url = node["input_url"].as<std::string>();

    if (node["input_format"])
      opts.input_format = node["input_format"].as<std::string>();
    if (node["width"])
      opts.width = node["width"].as<int>();
    if (node["height"])
      opts.height = node["height"].as<int>();
    if (node["framerate"])
      opts.framerate = node["framerate"].as<int>();
    if (node["pixel_format"])
      opts.pixel_format =
        PixelFormatFromString(node["pixel_format"].as<std::string>());

    if (node["rtbufsize"])
      opts.rtbufsize = node["rtbufsize"].as<int>();

    if (node["video"])
      opts.video = node["video"].as<StreamVideoOptions>();
    return true;
  }
};

}  // namespace YAML

#endif  // UTIL_CONFIG_STREAM_IGNORE

#ifndef UTIL_CONFIG_GLOG_IGNORE

#include <glog/logging.h>

namespace config {

inline
void InitGoogleLoggingFlags() {
  FLAGS_logtostderr = true;
  FLAGS_alsologtostderr = false;
  FLAGS_colorlogtostderr = true;

  FLAGS_minloglevel = google::INFO;
  FLAGS_v = 0;

  FLAGS_log_prefix = true;

  FLAGS_log_dir = ".";
  FLAGS_max_log_size = 8;
  FLAGS_stop_logging_if_full_disk = true;

  /*
  namespace gg = google;
  for (gg::LogSeverity s = gg::GLOG_INFO; s < gg::NUM_SEVERITIES; s++) {
    gg::SetLogDestination(s, );
    gg::SetLogSymlink(s, );
  }
  */
}

inline
void InitGoogleLoggingFlags(const YAML::Node &node) {
  if (!node) return;
  if (node["logtostderr"])
    FLAGS_logtostderr = node["logtostderr"].as<bool>();
  if (node["alsologtostderr"])
    FLAGS_alsologtostderr = node["alsologtostderr"].as<bool>();
  if (node["colorlogtostderr"])
    FLAGS_colorlogtostderr = node["colorlogtostderr"].as<bool>();

  if (node["minloglevel"])
    FLAGS_minloglevel = node["minloglevel"].as<int>();
  if (node["v"])
    FLAGS_v = node["v"].as<int>();

  if (node["log_prefix"])
    FLAGS_log_prefix = node["log_prefix"].as<bool>();

  if (node["log_dir"])
    FLAGS_log_dir = node["log_dir"].as<std::string>();
  if (node["max_log_size"])
    FLAGS_max_log_size = node["max_log_size"].as<int>();
  if (node["stop_logging_if_full_disk"])
    FLAGS_stop_logging_if_full_disk = node["stop_logging_if_full_disk"].as<bool>();
}

}  // namespace config

#endif  // UTIL_CONFIG_GLOG_IGNORE
