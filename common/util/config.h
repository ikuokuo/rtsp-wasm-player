#pragma once

#include <yaml-cpp/yaml.h>

#include <string>
#include <vector>

#include "log.h"

namespace config {

#ifdef UTIL_LOGGER_ENABLE

inline
void InitLogging(const char* argv0) {
  (void)argv0;
}

inline
void InitLoggingFlags(const YAML::Node &node) {
  if (!node) return;
  if (node["log_prefix"])
    UTIL_LOG_PREFIX = node["log_prefix"].as<bool>();
  if (node["minloglevel"])
    UTIL_LOG_MINLEVEL = node["minloglevel"].as<int>();
  if (node["v"])
    UTIL_LOG_V = node["v"].as<int>();
}

#else

inline
void InitLogging(const char* argv0) {
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

  google::InitGoogleLogging(argv0);
}

inline
void InitLoggingFlags(const YAML::Node &node) {
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

#endif  // UTIL_LOGGER_ENABLE

}  // namespace config

#ifdef UTIL_CONFIG_STREAM

#include "common/media/stream_def.h"

namespace YAML {

template <>
struct convert<StreamVideoOptions> {
  static Node encode(const StreamVideoOptions &opts) {
    Node node;
    node["dec_name"] = opts.dec_name;
    node["dec_thread_count"] = opts.dec_thread_count;
    node["dec_thread_type"] = opts.dec_thread_type;

    node["sws_enable"] = opts.sws_enable;
    node["sws_dst_width"] = opts.sws_dst_width;
    node["sws_dst_height"] = opts.sws_dst_height;
    node["sws_dst_pix_fmt"] = PixelFormatToString(opts.sws_dst_pix_fmt);
    node["sws_flags"] = opts.sws_flags;
    return node;
  }

  static bool decode(const Node &node, StreamVideoOptions &opts) {
    if (!node.IsMap()) {
      return false;
    }
    if (node["dec_name"])
      opts.dec_name = node["dec_name"].as<std::string>();
    if (node["dec_thread_count"])
      opts.dec_thread_count = node["dec_thread_count"].as<int>();
    if (node["dec_thread_type"])
      opts.dec_thread_type = node["dec_thread_type"].as<int>();

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
    node["max_delay"] = opts.max_delay;

    node["rtsp_transport"] = opts.rtsp_transport;
    node["stimeout"] = opts.stimeout;

    node["dump_format"] = opts.dump_format;

    node["video"] = opts.video;
    return node;
  }

  static bool decode(const Node &node, StreamOptions &opts) {
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
    if (node["max_delay"])
      opts.max_delay = node["max_delay"].as<int>();

    if (node["rtsp_transport"])
      opts.rtsp_transport = node["rtsp_transport"].as<std::string>();
    if (node["stimeout"])
      opts.stimeout = node["stimeout"].as<int>();

    if (node["dump_format"])
      opts.dump_format = node["dump_format"].as<bool>();

    if (node["video"])
      opts.video = node["video"].as<StreamVideoOptions>();
    return true;
  }
};

}  // namespace YAML

#endif  // UTIL_CONFIG_STREAM

#ifdef UTIL_CONFIG_CORS

#include "common/net/cors.h"

namespace YAML {

template <>
struct convert<net::Options> {
  static bool decode(const Node &node, net::Options &opts) {
    auto parseAsStrings = [](const std::string &k,
                             const YAML::Node &n,
                             std::vector<std::string> &v) {
      if (!n) return;
      if (n.IsScalar()) {
        v.push_back(n.as<std::string>());
      } else if (n.IsSequence()) {
        for (auto i = n.begin(); i != n.end(); ++i) {
          v.push_back(i->as<std::string>());
        }
      } else {
        std::stringstream ss;
        ss << "cors." << k << " not a string value or list, ignored";
        throw YAML::Exception(n.Mark(), ss.str());
      }
    };
    if (node["enabled"])
      opts.enabled = node["enabled"].as<bool>();
    parseAsStrings("allowed_origins", node["allowed_origins"],
                    opts.allowed_origins);
    parseAsStrings("allowed_methods", node["allowed_methods"],
                    opts.allowed_methods);
    parseAsStrings("allowed_headers", node["allowed_headers"],
                    opts.allowed_headers);
    parseAsStrings("exposed_headers", node["exposed_headers"],
                    opts.exposed_headers);
    if (node["allowed_credentials"])
      opts.allowed_credentials = node["allowed_credentials"].as<bool>();
    if (node["max_age"])
      opts.max_age = node["max_age"].as<int>();
    if (node["debug"])
      opts.debug = node["debug"].as<bool>();
    return true;
  }
};

}  // namespace YAML

#endif  // UTIL_CONFIG_CORS
