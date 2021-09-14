#pragma once

#include <chrono>
#include <cmath>
#include <cstdint>
#include <ctime>
#include <iomanip>
#include <ratio>
#include <sstream>
#include <string>

namespace times {

using clock = std::chrono::system_clock;

// template <class Rep, class Period = std::ratio<1>>
// using duration = std::chrono::duration<Rep, Period>;

using nanoseconds = std::chrono::nanoseconds;
using microseconds = std::chrono::microseconds;
using milliseconds = std::chrono::milliseconds;
using seconds = std::chrono::seconds;
using minutes = std::chrono::minutes;
using hours = std::chrono::hours;
using days = std::chrono::duration<std::int64_t, std::ratio<86400>>;

// to

template <typename Duration>
Duration to_duration(const std::int64_t &t) {
  return Duration{t};
}

template <typename Duration>
clock::time_point to_time_point(const Duration &d) {
  return clock::time_point(d);
}

template <typename Duration>
clock::time_point to_time_point(const std::int64_t &t) {
  return clock::time_point(Duration{t});
}

inline clock::time_point to_time_point(std::tm *tm) {
  return clock::from_time_t(std::mktime(tm));
}

inline struct std::tm *to_local_tm(const clock::time_point &t) {
  auto t_c = clock::to_time_t(t);
  return std::localtime(&t_c);
}

inline struct std::tm *to_utc_tm(const clock::time_point &t) {
  auto t_c = clock::to_time_t(t);
  return std::gmtime(&t_c);
}

// cast

template <typename FromDuration, typename ToDuration>
ToDuration cast(const FromDuration &d) {
  return std::chrono::duration_cast<ToDuration>(d);
}

template <typename Duration>
Duration cast(const clock::duration &d) {
  return cast<clock::duration, Duration>(d);
}

template <typename FromDuration, typename ToDuration>
std::int64_t cast(const std::int64_t &t) {
  return cast<FromDuration, ToDuration>(FromDuration{t}).count();
}

template <typename Duration>
clock::time_point cast(const clock::time_point &d) {
  // C++17, floor
  return std::chrono::time_point_cast<Duration>(d);
}

template <typename Duration>
clock::duration cast_mod(const clock::time_point &t) {
  return t - cast<Duration>(t);
}

// count

template <typename FromDuration, typename ToDuration>
std::int64_t count(const FromDuration &d) {
  return cast<FromDuration, ToDuration>(d).count();
}

template <typename Duration>
std::int64_t count(const clock::duration &d) {
  return cast<Duration>(d).count();
}

// day

inline std::tm *day_beg(std::tm *tm) {
  tm->tm_hour = 0;
  tm->tm_min = 0;
  tm->tm_sec = 0;
  return tm;
}

inline std::tm *day_end(std::tm *tm) {
  tm->tm_hour = 23;
  tm->tm_min = 59;
  tm->tm_sec = 59;
  return tm;
}

inline clock::time_point day_beg(const clock::time_point &t) {
  return cast<days>(t);
}

inline clock::time_point day_end(const clock::time_point &t) {
  return day_beg(t) + days(1) - clock::duration(1);
}

inline clock::duration day_time(const clock::time_point &t) {
  return cast_mod<days>(t);
}

// between

template <typename Duration>
std::int64_t between(
    const clock::time_point &t1, const clock::time_point &t2) {
  return count<Duration>(t2 - t1);
}

inline std::int64_t between_days(
    const clock::time_point &t1, const clock::time_point &t2) {
  return between<days>(day_beg(t1), day_beg(t2));
}

template <typename Duration>
std::int64_t between_days(const std::int64_t &t1, const std::int64_t &t2) {
  return between_days(to_time_point<Duration>(t1), to_time_point<Duration>(t2));
}

// epoch

inline clock::time_point epoch() {
  return clock::time_point(clock::duration{0});
}

template <typename Duration>
std::int64_t since_epoch(const clock::time_point &t) {
  return count<Duration>(t.time_since_epoch());
}

// now

inline clock::time_point now() {
  return clock::now();
}

template <typename Duration>
std::int64_t now() {
  return since_epoch<Duration>(now());
}

// string

inline std::string to_string(
    const clock::time_point &t, const std::tm *tm,
    const char *fmt = "%F %T", std::int32_t precision = 6,
    const char &sep = '.') {
  std::stringstream ss;
#if defined(__ANDROID__) || defined(__linux__)
  char foo[30];
  strftime(foo, sizeof(foo), fmt, tm);
  ss << foo;
#else
  ss << std::put_time(tm, fmt);
#endif
  if (precision > 0) {
    if (precision > 6)
      precision = 6;
    ss << sep << std::setfill('0') << std::setw(precision)
       << static_cast<std::int32_t>(
              count<microseconds>(cast_mod<seconds>(t)) /
              std::pow(10, 6 - precision));
  }
  return ss.str();
}

inline std::string to_local_string(
    const clock::time_point &t, const char *fmt = "%F %T",
    const std::int32_t &precision = 0, const char &sep = '.') {
  return to_string(t, to_local_tm(t), fmt, precision, sep);
}

inline std::string to_utc_string(
    const clock::time_point &t, const char *fmt = "%F %T",
    const std::int32_t &precision = 0, const char &sep = '.') {
  return to_string(t, to_utc_tm(t), fmt, precision, sep);
}

}  // namespace times
