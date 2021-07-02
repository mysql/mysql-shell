/*
 * Copyright (c) 2018, 2021, Oracle and/or its affiliates.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.0,
 * as published by the Free Software Foundation.
 *
 * This program is also distributed with certain software (including
 * but not limited to OpenSSL) that is licensed under separate terms, as
 * designated in a particular file or component or in included license
 * documentation.  The authors of MySQL hereby grant you an additional
 * permission to link the program and your derivative works with the
 * separately licensed software that they have included with MySQL.
 * This program is distributed in the hope that it will be useful,  but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 * the GNU General Public License, version 2.0, for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef MYSQLSHDK_LIBS_UTILS_PROFILING_H_
#define MYSQLSHDK_LIBS_UTILS_PROFILING_H_

#include <chrono>
#include <map>
#include <mutex>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "mysqlshdk/libs/utils/utils_general.h"

namespace mysqlshdk {
namespace utils {

class Profile_timer {
  using high_resolution_clock = std::chrono::high_resolution_clock;

 public:
  Profile_timer() {
    _trace_points.reserve(32);
    _nesting_levels.reserve(32);
  }

  inline void reserve(size_t space) { _trace_points.reserve(space); }

  inline void stage_begin(const char *note) {
    _trace_points.emplace_back(note, high_resolution_clock::now(), _depth);
    _nesting_levels.emplace_back(_trace_points.size() - 1);
    ++_depth;
  }

  inline void stage_end() {
    size_t stage = _nesting_levels.back();
    _nesting_levels.pop_back();
    --_depth;
    _trace_points.at(stage).end = high_resolution_clock::now();
  }

  uint64_t total_nanoseconds_elapsed() const {
    auto dur = high_resolution_clock::duration::zero();
    for (const auto &tp : _trace_points) {
      if (tp.depth == 0) dur += tp.elapsed();
    }
    return std::chrono::duration_cast<std::chrono::nanoseconds>(dur).count();
  }

  double total_milliseconds_elapsed() const {
    return total_nanoseconds_elapsed() / 1000000.0;
  }

  double total_seconds_elapsed() const {
    return total_nanoseconds_elapsed() / 1000000000.0;
  }

 public:
  struct Trace_point {
    char note[33];
    high_resolution_clock::time_point start;
    high_resolution_clock::time_point end;
    int depth;

    Trace_point(const char *n, high_resolution_clock::time_point &&t, int d)
        : start(std::move(t)), depth(d) {
      snprintf(note, sizeof(note), "%s", n);
    }

    high_resolution_clock::duration elapsed() const { return end - start; }

    uint64_t nanoseconds_elapsed() const {
      return std::chrono::duration_cast<std::chrono::nanoseconds>(elapsed())
          .count();
    }

    double milliseconds_elapsed() const {
      return nanoseconds_elapsed() / 1000000.0;
    }

    double seconds_elapsed() const {
      return nanoseconds_elapsed() / 1000000000.0;
    }
  };

  const std::vector<Trace_point> &trace_points() const { return _trace_points; }

 private:
  std::vector<Trace_point> _trace_points;
  std::vector<size_t> _nesting_levels;
  int _depth = 0;
};

/**
 * This class keeps track execution time on specifics blocks of code
 * during a complete session. A session is defined by the time when the
 * global profiler is enabled and the time is finished.
 *
 * The execution time is recorded by thread/stage groups.
 *
 * This class is NOT intended to be used (instantiated) within the shell
 * functionality, but to be used as a global profiler which will record
 * execution time associated to specific IDs and then print accumulated
 * statistics.
 *
 * Use it throught the following functions in the profiling namespace:
 *
 * - activate
 * - deactivate
 * - stage_begin
 * - stage_end
 */
class Global_profiler {
 public:
  ~Global_profiler() { print_stats(); }
  void stage_begin(const std::string &id) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_time_profilers[std::this_thread::get_id()][id].stage_begin(id.c_str());
  }

  void stage_end(const std::string &id) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_time_profilers.at(std::this_thread::get_id()).at(id).stage_end();
  }

  void print_stats();

  using Time_profilers = std::map<std::string, Profile_timer>;

 private:
  std::mutex m_mutex;
  std::map<std::thread::id, Time_profilers> m_time_profilers;
};

}  // namespace utils

/**
 * This namespace provides the functions required to perform profiling
 * of specific sections of code.
 *
 * TO ACTIVATE THE PROFILER
 * ------------------------
 * Call the following functions to activate/deactivate it as needed:
 * - mysqlshdk::profiling::activate();
 * - mysqlshdk::profiling::deactivate();
 *
 * Or do as follows to ensure it is active within a specific context:
 *
 *  mysqlshdk::profiling::activate();
 *  auto finally =
 *      shcore::on_leave_scope([]() { mysqlshdk::profiling::deactivate(); });
 *
 * TO ADD A MEASURING POINT WITHIN A THREAD
 * ----------------------------------------
 *  Call the following functions to measure the time spent between them:
 *
 * - mysqlshdk::profiling::stage_begin("<MEASURE ID>");
 * - mysqlshdk::profiling::stage_end("<MEASURE ID>");
 *
 * <MEASURE ID>: Text to identify the context being measured.
 *
 * Make sure the measure ID passed to both functions is the same. It will
 * record a the number of milliseconds spend between both calls for each time
 * the code is hit while the profiling is active.
 *
 * To measure the time within a specific scope you can also use:
 *
 *  mysqlshdk::profiling::stage_begin("<MEASURE ID>");
 *  shcore::on_leave_scope end_stage(
 *      []() { mysqlshdk::profiling::stage_end("<MEASURE ID>"); });
 *
 * You can add as many measure points as needed.
 */
namespace profiling {
extern mysqlshdk::utils::Global_profiler *g_active_profiler;

void activate();
void deactivate();

inline void stage_begin(const std::string &id) {
  if (g_active_profiler) g_active_profiler->stage_begin(id);
}

inline void stage_end(const std::string &id) {
  if (g_active_profiler) g_active_profiler->stage_end(id);
}

}  // namespace profiling

}  // namespace mysqlshdk

#endif  // MYSQLSHDK_LIBS_UTILS_PROFILING_H_
