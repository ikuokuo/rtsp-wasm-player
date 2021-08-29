#pragma once

#include <vector>

#ifdef __cplusplus
extern "C" {
#endif

#include <libavcodec/packet.h>
#include <libavutil/avutil.h>

#ifdef __cplusplus
}
#endif

#include <boost/asio/buffer.hpp>

#include "common/util/bytes.h"
#include "common/util/log.h"

namespace net {

class Data {
 public:
  AVMediaType type;
  AVPacket *packet;

  Data()
    : type(AVMEDIA_TYPE_UNKNOWN), packet(av_packet_alloc()) {
  }
  Data(AVMediaType type, AVPacket *p)
    : type(type), packet(av_packet_clone(p)) {
  }
  ~Data() {
    av_packet_free(&packet);
  }

  enum Status {
    OK,
    ERROR_NOT_ENOUGH,
    ERROR_ALLOC_FAIL,
  };

  std::vector<uint8_t> ToBytes();
  int ToBytes(std::vector<uint8_t> &bytes);
  int ToBytes(uint8_t *bytes, std::size_t bytes_n) {
    return ToBytes(bytes, bytes_n, *this);
  }

  int FromBytes(const std::vector<uint8_t> &bytes);
  int FromBytes(const boost::asio::mutable_buffer &bytes);
  int FromBytes(const uint8_t *bytes, std::size_t bytes_n) {
    return FromBytes(bytes, bytes_n, *this);
  }

  std::size_t GetByteSize() const;

 private:
  int ToBytes(uint8_t *bytes, std::size_t bytes_n, const Data &data);
  int FromBytes(const uint8_t *bytes, std::size_t bytes_n, Data &data);

  static const uint8_t ver_major = 1;
  static const uint8_t ver_minor = 0;
};

inline
std::vector<uint8_t> Data::ToBytes() {
  std::vector<uint8_t> bytes;
  ToBytes(bytes);
  return bytes;
}

inline
int Data::ToBytes(std::vector<uint8_t> &bytes) {
  auto n = GetByteSize();
  if (bytes.size() < n) {
    bytes.resize(n);
  }
  return ToBytes(bytes.data(), bytes.size(), *this);
}

inline
int Data::FromBytes(const std::vector<uint8_t> &bytes) {
  return FromBytes(bytes.data(), bytes.size(), *this);
}

inline
int Data::FromBytes(const boost::asio::mutable_buffer &buf) {
  return FromBytes(reinterpret_cast<uint8_t *>(buf.data()), buf.size(), *this);
}

/*
version 1.0

data
| version | type | pkg_size | pkg_data |
| 2       | 1    | 4        | -        |

pkg_data
| pts | dts | size | data | stream_index | flags | side_data_elems | side_data ... | duration | pos |
| 8   | 8   | 4    | -    | 4            | 4     | 4               | -             | 8        | 8   |

side_data
| type | size | data |
| 1    | 4    | -    |
*/

inline
std::size_t Data::GetByteSize() const {
  std::size_t n = (2+1+4) + (8*4+4*4) + packet->size;
  for (int i = 0, end = packet->side_data_elems; i < end; ++i) {
    n += 5 + (packet->side_data + i)->size;
  }
  return n;
}

template <typename T>
std::size_t to_bytes(uint8_t *bytes, const T &v) {
  *bytes = static_cast<uint8_t>(v);
  return sizeof(v);
}

inline
int Data::ToBytes(uint8_t *bytes, std::size_t bytes_n, const Data &data) {
  auto pkg_size = GetByteSize();
  if (bytes_n < pkg_size) {
    return ERROR_NOT_ENOUGH;
  }
  std::size_t pos = 0;
  pos += bytes::toc<uint8_t>(bytes+pos, ver_major);
  pos += bytes::toc<uint8_t>(bytes+pos, ver_minor);
  pos += bytes::toc<uint8_t>(bytes+pos, data.type);
  pos += bytes::toc<uint32_t>(bytes+pos, pkg_size);
  // pkg_data
  pos += bytes::toc<int64_t>(bytes+pos, data.packet->pts);
  pos += bytes::toc<int64_t>(bytes+pos, data.packet->dts);
  pos += bytes::toc<int>(bytes+pos, data.packet->size);
  pos += bytes::to(bytes+pos, data.packet->data, data.packet->size);
  pos += bytes::toc<int>(bytes+pos, data.packet->stream_index);
  pos += bytes::toc<int>(bytes+pos, data.packet->flags);
  pos += bytes::toc<int>(bytes+pos, data.packet->side_data_elems);
  for (int i = 0, end = packet->side_data_elems; i < end; ++i) {
    auto side_data = packet->side_data + i;
    pos += bytes::toc<uint8_t>(bytes+pos, side_data->type);
    pos += bytes::toc<int>(bytes+pos, side_data->size);
    pos += bytes::to(bytes+pos, side_data->data, side_data->size);
  }
  pos += bytes::toc<int64_t>(bytes+pos, data.packet->duration);
  pos += bytes::toc<int64_t>(bytes+pos, data.packet->pos);
  LOG_IF(FATAL, pkg_size != pos) << "Packet to bytes fail, pkg_size="
      << pkg_size << ", pos=" << pos;
  return OK;
}

inline
int Data::FromBytes(const uint8_t *bytes, std::size_t bytes_n, Data &data) {
  if (bytes_n < 7) {
    return ERROR_NOT_ENOUGH;
  }
  std::size_t pos = 0;
  auto ver_major = bytes::from<uint8_t>(bytes+pos);
  auto ver_minor = bytes::from<uint8_t>(bytes+pos+1);
  (void)ver_major;
  (void)ver_minor;

  data.type = bytes::fromc<decltype(data.type), uint8_t>(bytes+pos+2);
  std::size_t pkg_size = bytes::from<uint32_t>(bytes+pos+3);
  pos += 7;
  if (bytes_n < pkg_size) {
    return ERROR_NOT_ENOUGH;
  }

  // pkg_data
  data.packet->pts = bytes::from<int64_t>(bytes+pos);
  data.packet->dts = bytes::from<int64_t>(bytes+pos+8);
  auto data_size = bytes::from<int>(bytes+pos+16);
  auto data_buf = static_cast<uint8_t *>(av_malloc(data_size));
  pos += 20;
  if (data_buf) {
    pos += bytes::from(bytes+pos, data_buf, data_size);
    av_packet_from_data(data.packet, data_buf, data_size);
  } else {
    return ERROR_ALLOC_FAIL;
  }
  data.packet->stream_index = bytes::from<int>(bytes+pos);
  data.packet->flags = bytes::from<int>(bytes+pos+4);
  auto side_data_elems = bytes::from<int>(bytes+pos+8);
  pos += 12;
  for (int i = 0, end = side_data_elems; i < end; ++i) {
    auto type = bytes::fromc<AVPacketSideDataType, uint8_t>(bytes+pos);
    auto size = bytes::from<int>(bytes+pos+1);
    auto data_buf = static_cast<uint8_t *>(av_malloc(size));
    if (data_buf) {
      bytes::from(bytes+pos+5, data_buf, size);
      av_packet_add_side_data(data.packet, type, data_buf, size);
    } else {
      return ERROR_ALLOC_FAIL;
    }
    pos += 5 + size;
  }
  data.packet->duration = bytes::from<int64_t>(bytes+pos);
  data.packet->pos = bytes::from<int64_t>(bytes+pos);
  pos += 16;

  LOG_IF(FATAL, pkg_size != pos) << "Packet from bytes fail, pkg_size="
      << pkg_size << ", pos=" << pos;
  return OK;
}

}  // namespace net
