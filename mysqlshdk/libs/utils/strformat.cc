/*
 * Copyright (c) 2018, Oracle and/or its affiliates. All rights reserved.
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

#include "mysqlshdk/libs/utils/strformat.h"
#include <time.h>
#include <cmath>
#include <cstdio>
#include <tuple>
#include <utility>
#include "mysqlshdk/libs/utils/utils_string.h"

namespace mysqlshdk {
namespace utils {

using shcore::str_format;

std::string format_seconds(double secs) {
  char buffer[256];
  std::string str;
  int d = secs / (3600 * 24);
  secs -= d * 3600 * 24;
  int h = secs / 3600;
  secs -= h * 3600;
  int m = secs / 60;
  secs -= m * 60;
  if (d > 0) {
    snprintf(buffer, sizeof(buffer), d > 1 ? "%i days" : "%i day", d);
    str.append(buffer);
  }
  if (h > 0) {
    snprintf(buffer, sizeof(buffer), h > 1 ? "%i hours" : "%i hour", h);
    if (!str.empty()) str.append(" ");
    str.append(buffer);
  }
  if (m > 0) {
    snprintf(buffer, sizeof(buffer), "%i min", m);
    if (!str.empty()) str.append(" ");
    str.append(buffer);
  }
  snprintf(buffer, sizeof(buffer), "%01.4f sec", secs);
  if (!str.empty()) str.append(" ");
  str.append(buffer);
  return str;
}

std::string format_microseconds(double secs) {
  char buffer[256];
  std::string str;
  int d = secs / (3600 * 24);
  secs -= d * 3600 * 24;
  int h = secs / 3600;
  secs -= h * 3600;
  int m = secs / 60;
  secs -= m * 60;
  if (d > 0) {
    snprintf(buffer, sizeof(buffer), d > 1 ? "%i days" : "%i day", d);
    str.append(buffer);
  }
  if (h > 0) {
    snprintf(buffer, sizeof(buffer), h > 1 ? "%i hours" : "%i hour", h);
    if (!str.empty()) str.append(" ");
    str.append(buffer);
  }
  if (m > 0) {
    snprintf(buffer, sizeof(buffer), "%i min", m);
    if (!str.empty()) str.append(" ");
    str.append(buffer);
  }
  if (round(secs) > 0) {
    snprintf(buffer, sizeof(buffer), "%i sec", static_cast<int>(secs));
    if (!str.empty()) str.append(" ");
    str.append(buffer);
  }
  snprintf(buffer, sizeof(buffer), "%i usec",
           static_cast<int>((secs - round(secs)) * 1000000));
  if (!str.empty()) str.append(" ");
  str.append(buffer);

  return str;
}

std::string format_bytes(uint64_t bytes) {
  std::string unit;
  double nitems;
  std::tie(unit, nitems) = scale_value(bytes);
  if (nitems == bytes) {
    return std::to_string(bytes) + " bytes";
  } else {
    char buffer[64];
    snprintf(buffer, sizeof(buffer), "%.2f ", nitems);
    return buffer + unit + "B";
  }
}

std::string format_throughput_items(const std::string &item_name_singular,
                                    const std::string &item_name_plural,
                                    const uint64_t items, double seconds) {
  char buffer[64] = {};
  double ratio = items / seconds;
  std::string unit;
  double scaled_items;
  std::tie(unit, scaled_items) = scale_value(ratio);
  snprintf(buffer, sizeof(buffer), "%.2f", scaled_items);

  // Singular/plural form for English language.
  if (ratio > 0 && ratio <= 1.0) {
    return buffer + unit + " " + item_name_singular + "/s";
  }
  return buffer + unit + " " + item_name_plural + "/s";
}

std::string format_throughput_bytes(uint64_t bytes, double seconds) {
  char buffer[64];
  std::string unit;
  double nitems;
  std::tie(unit, nitems) = scale_value(bytes);
  snprintf(buffer, sizeof(buffer), "%.2f ", nitems / seconds);
  return buffer + unit + "B/s";
}

std::string fmttime(const char *fmt, Time_type type, time_t *time_ptr) {
  time_t t;
  char buf[64];

  if (time_ptr)
    t = *time_ptr;
  else
    t = time(nullptr);

  struct tm lt;
#ifdef _WIN32
  if (type == Time_type::LOCAL)
    localtime_s(&lt, &t);
  else
    gmtime_s(&lt, &t);
#else
  if (type == Time_type::LOCAL)
    localtime_r(&t, &lt);
  else
    gmtime_r(&t, &lt);
#endif
  strftime(buf, sizeof(buf), fmt, &lt);

  return buf;
}

}  // namespace utils
}  // namespace mysqlshdk
