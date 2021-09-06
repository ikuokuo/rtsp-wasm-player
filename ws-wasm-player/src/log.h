#pragma once

#include "common/util/log.h"

struct Log {
  static void set_prefix(bool b) { UTIL_LOG_PREFIX = b; }
  static void set_minlevel(int n) { UTIL_LOG_MINLEVEL = n; }
  static void set_v(int n) { UTIL_LOG_V = n; }
};
