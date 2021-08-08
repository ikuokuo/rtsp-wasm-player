#pragma once

#include <glog/logging.h>
#include <yaml-cpp/yaml.h>

#include <string>

#include "common/media/stream_def.h"

namespace YAML {

template<>
struct convert<StreamOptions> {
  static Node encode(const StreamOptions& opts) {
    Node node;
    node["method"] = StreamMethodToString(opts.method);
    node["input_url"] = opts.input_url;

    node["input_format"] = opts.input_format;
    node["width"] = opts.width;
    node["height"] = opts.height;
    node["framerate"] = opts.framerate;
    node["pixel_format"] = PixelFormatToString(opts.pixel_format);

    node["rtbufsize"] = opts.rtbufsize;

    node["sws_enable"] = opts.sws_enable;
    node["sws_dst_width"] = opts.sws_dst_width;
    node["sws_dst_height"] = opts.sws_dst_height;
    node["sws_dst_pix_fmt"] = PixelFormatToString(opts.sws_dst_pix_fmt);
    node["sws_flags"] = opts.sws_flags;
    return node;
  }

  static bool decode(const Node& node, StreamOptions& opts) {  // NOLINT
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

}  // namespace YAML

inline
bool StreamOptionsParse(const YAML::Node &node, StreamOptions *opts,
                        const std::string &key = "stream") {
  try {
    *opts = node[key].as<StreamOptions>();
    return true;
  } catch (const std::exception &e) {
    LOG(ERROR) << "StreamOptions parse fail: " << e.what();
    return false;
  }
}

inline
bool StreamOptionsParse(const std::string &path, StreamOptions *opts) {
  return StreamOptionsParse(YAML::LoadFile(path), opts);
}
