#pragma once

#include <algorithm>
#include <cstdint>
#include <string>

namespace bytes {

using byte_t = std::uint8_t;
using size_t = std::size_t;

// to

template <typename T>
size_t to(byte_t *bytes, T value) {
  size_t size = sizeof(T) / sizeof(byte_t);
  for (size_t i = 0; i < size; i++) {  // big endian
    bytes[i] = static_cast<byte_t>((value >> (8 * (size - i - 1))) & 0xff);
  }
  return size;
}

template <>
inline
size_t to(byte_t *bytes, byte_t value) {
  *bytes = value;
  return 1;
}

template <>
inline
size_t to(byte_t *bytes, double value) {
  byte_t *val = reinterpret_cast<byte_t *>(&value);
  std::copy(val, val + 8, bytes);
  return 8;
}

inline
size_t to(byte_t *bytes, byte_t *data, size_t size) {
  std::copy(data, data + size, bytes);
  return size;
}

inline
size_t to(byte_t *bytes, std::string value,
               size_t size = 0, char pad = ' ') {
  std::copy(value.begin(), value.end(), bytes);
  if (size > value.size()) {
    for (size_t i = value.size(); i < size; i++) {
      bytes[i] = pad;
    }
    return size;
  }
  return value.size();
}

template <typename C, typename T>
size_t toc(byte_t *bytes, T value) {
  return to(bytes, static_cast<C>(value));
}

// from

template <typename T>
T from(const byte_t *bytes) {
  size_t size = sizeof(T) / sizeof(byte_t);
  T value = 0;
  for (size_t i = 0; i < size; i++) {  // big endian
    value |= bytes[i] << (8 * (size - i - 1));
  }
  return value;
}

template <>
inline
byte_t from(const byte_t *bytes) {
  return *bytes;
}

template <>
inline
double from(const byte_t *bytes) {
  return *(reinterpret_cast<const double *>(bytes));
}

inline
size_t from(const byte_t *bytes, byte_t *data, size_t size) {
  std::copy(bytes, bytes + size, data);
  return size;
}

inline
std::string from(const byte_t *bytes, size_t size,
                 bool trim = false, const char* trim_set = " ") {
  std::string s(reinterpret_cast<const char *>(bytes), size);
  if (trim) {  // rtrim
    auto end = s.find_last_not_of(trim_set);
    if (end == std::string::npos) {
      s.clear();
    } else {
      s.erase(end + 1);
    }
  }
  return s;
}

template <typename C, typename T>
C fromc(const byte_t *bytes) {
  return static_cast<C>(from<T>(bytes));
}

}  // namespace bytes
