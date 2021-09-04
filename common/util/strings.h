#pragma once

#include <algorithm>
#include <regex>
#include <sstream>
#include <string>
#include <vector>

namespace strings {

template <typename T>
std::string to_string(T &&t) {
  std::ostringstream ss;
  ss << t;
  return ss.str();
}

// test
//  https://stackoverflow.com/a/42844629

inline bool ends_with(const std::string &s, const std::string &suffix) {
  return s.size() >= suffix.size() &&
      0 == s.compare(s.size()-suffix.size(), suffix.size(), suffix);
}

inline bool starts_with(const std::string &s, const std::string &prefix) {
  return s.size() >= prefix.size() &&
      0 == s.compare(0, prefix.size(), prefix);
}

// split
//  https://stackoverflow.com/a/9437426

inline std::vector<std::string> split(const std::string &input,
    const std::string &regex) {
  std::regex re(regex);
  std::sregex_token_iterator
      first{input.begin(), input.end(), re, -1},
      last;
  return {first, last};
}

// trim
//  https://stackoverflow.com/a/217605

// trim from start (in place)
inline void ltrim(std::string &s) {
  s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
    return !std::isspace(ch);
  }));
}

// trim from end (in place)
inline void rtrim(std::string &s) {
  s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
    return !std::isspace(ch);
  }).base(), s.end());
}

// trim from both ends (in place)
inline void trim(std::string &s) {
  ltrim(s);
  rtrim(s);
}

// trim from start (copying)
inline std::string ltrim_copy(std::string s) {
  ltrim(s);
  return s;
}

// trim from end (copying)
inline std::string rtrim_copy(std::string s) {
  rtrim(s);
  return s;
}

// trim from both ends (copying)
inline std::string trim_copy(std::string s) {
  trim(s);
  return s;
}

}  // namespace strings
