/*
 * Copyright (c) 2018, 2020, Oracle and/or its affiliates. All rights reserved.
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
#include <string>
#include <utility>
#include <vector>

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
    high_resolution_clock::duration dur(
        high_resolution_clock::duration::zero());
    for (auto &tp : _trace_points) {
      if (tp.depth == 0) dur += tp.end - tp.start;
    }
    return std::chrono::duration_cast<std::chrono::nanoseconds>(dur).count();
  }

  double total_milliseconds_elapsed() const {
    return total_nanoseconds_elapsed() / 1000000.0;
  }

  double total_seconds_elapsed() const {
    return total_nanoseconds_elapsed() / 1000000000.0;
  }

  static Profile_timer *activate();
  static void deactivate();

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
  };

  const std::vector<Trace_point> &trace_points() const { return _trace_points; }

 private:
  std::vector<Trace_point> _trace_points;
  std::vector<size_t> _nesting_levels;
  int _depth = 0;
};

extern Profile_timer *g_active_timer;

}  // namespace utils

inline void stage_begin(const char *note) {
  if (utils::g_active_timer) utils::g_active_timer->stage_begin(note);
}

inline void stage_end() {
  if (utils::g_active_timer) utils::g_active_timer->stage_end();
}

}  // namespace mysqlshdk

#endif  // MYSQLSHDK_LIBS_UTILS_PROFILING_H_
