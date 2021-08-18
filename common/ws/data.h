#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <libavformat/avformat.h>

#ifdef __cplusplus
}
#endif

#include <iostream>
#include <sstream>
#include <string>

inline
std::ostream &operator<<(std::ostream &os, AVCodecParameters *codecpar) {
  return os << "{"
    << "\"codec_type\": " << static_cast<int>(codecpar->codec_type)
    << ", \"codec_id\": " << static_cast<int>(codecpar->codec_id)
    << "}";
}

inline
std::string to_string(AVCodecParameters *codecpar) {
  std::stringstream ss;
  ss << codecpar;
  return ss.str();
}
