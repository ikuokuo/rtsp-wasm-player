#pragma once

#include <chrono>
#include <memory>
#include <string>
#include <vector>
#include <utility>

#include "log.h"

namespace logext {

using clock = std::chrono::system_clock;

/*
auto t = logext::TimeRecord::Create("task");
t->Beg("step a");
t->End();
VLOG(2) << t->Log();
*/
class TimeRecord {
 public:
  TimeRecord() = default;
  virtual ~TimeRecord() = default;

  static std::shared_ptr<TimeRecord> Create(std::string label);

  virtual void Reset(std::string) {}
  virtual void Beg(std::string) {}
  virtual void End() {}
  virtual std::string Log() { return ""; }
};

class TimeRecordImpl : public TimeRecord {
 public:
  struct record_t {
    std::string label;
    clock::time_point time_beg;
    clock::time_point time_end;
  };

  explicit TimeRecordImpl(std::string label) {
    Reset(std::move(label));
  }
  ~TimeRecordImpl() override {
  }

  void Reset(std::string label) override {
    is_beg_ = false;
    record_ = {
        std::move(label),
        clock::now(),
        clock::time_point{}};
    record_slices_.clear();
  }

  void Beg(std::string label) override {
    if (is_beg_) End();
    is_beg_ = true;
    record_slices_.push_back({
        std::move(label),
        clock::now(),
        clock::time_point{}});
  }

  void End() override {
    if (!is_beg_) return;
    is_beg_ = false;
    record_slices_.back().time_end = clock::now();
  }

  std::string Log() override {
    if (is_beg_) End();
    using namespace std::chrono;  // NOLINT
    record_.time_end = clock::now();
    auto &&d = duration_cast<milliseconds>(
        record_.time_end - record_.time_beg).count();
    std::stringstream ss;
    ss << record_.label << " cost " << d << " ms";
    if (!record_slices_.empty()) {
      ss << ", slices=[";
      for (std::size_t i = 0, n = record_slices_.size()-1; i <= n; i++) {
        auto &&r = record_slices_[i];
        auto &&d = duration_cast<milliseconds>(r.time_end - r.time_beg).count();
        ss << r.label << " " << d;
        if (i != n) ss << ", ";
      }
      ss << "] ms";
    }
    return ss.str();
  }

 private:
  bool is_beg_;
  record_t record_;
  std::vector<record_t> record_slices_;
};

inline
std::shared_ptr<TimeRecord> TimeRecord::Create(std::string label) {
  if (VLOG_IS_ON(2)) {
    return std::make_shared<TimeRecordImpl>(std::move(label));
  } else {
    static auto &&t = std::make_shared<TimeRecord>();
    return t;
  }
}

/*
class TimeGuard {
 public:
  explicit TimeGuard(std::string label)
    : label_{std::move(label)}, time_start_{clock::now()} {
  }
  ~TimeGuard() {
    using namespace std::chrono;  // NOLINT
    auto &&d = duration_cast<microseconds>(clock::now() - time_start_).count();
    VLOG(2) << label_ << " cost " << d * 0.001 << " ms";
  }

  static std::unique_ptr<TimeGuard> Create(std::string label);

 private:
  std::string label_;
  clock::time_point time_start_;
};
*/

}  // namespace logext
