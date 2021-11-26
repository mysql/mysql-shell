/*
 * Copyright (c) 2020, 2021, Oracle and/or its affiliates.
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

#include "mysqlshdk/libs/utils/utils_time.h"

#include <cstdio>
#include <cstring>
#include <ctime>
#include <regex>
#include <stdexcept>

#include "mysqlshdk/libs/utils/strformat.h"

namespace shcore {

namespace {

// copied from the router
time_t time_from_struct_tm_utc(struct tm *t_m) {
#if defined(_WIN32)
  return _mkgmtime(t_m);
#elif defined(__sun)
  // solaris, linux have typeof('timezone') == time_t
  return mktime(t_m) - timezone;
#else
  // linux, freebsd and apple have timegm()
  return timegm(t_m);
#endif
}

void validate_rfc3339(const std::string &s) {
  static std::regex format{
      R"(\d{4}-\d{2}-\d{2}.\d{2}:\d{2}:\d{2}(?:\.\d+)?(?:[Zz]|[+-]\d{2}:\d{2}))"};

  if (!std::regex_match(s, format)) {
    throw std::runtime_error("Timestamp '" + s + "' is not in RFC3339 format.");
  }
}

}  // namespace

std::string current_time_rfc3339() {
  return future_time_rfc3339(std::chrono::hours(0));
}

std::string future_time_rfc3339(const std::chrono::hours &h) {
  return time_point_to_rfc3339(std::chrono::system_clock::now() + h);
}

std::string time_point_to_rfc3339(
    const std::chrono::system_clock::time_point &tp) {
  const auto time = std::chrono::system_clock::to_time_t(tp);

  return mysqlshdk::utils::fmttime("%FT%TZ", mysqlshdk::utils::Time_type::GMT,
                                   &time);
}

std::chrono::system_clock::time_point rfc3339_to_time_point(
    const std::string &s) {
  validate_rfc3339(s);

  // yyyy-mm-ddThh:mm:ss, length is 19, we use sscanf() to parse this part
  // and parse the rest by hand
  static constexpr std::size_t pos = 19;

  auto it = s.c_str() + pos;
  uint64_t ns = 0;

  if ('.' == *it) {
    // . DIGIT+, time has fractions of seconds, move past dot
    ++it;
    // read fractions of seconds
    char *end = nullptr;
    const auto fraction = std::strtoull(it, &end, 10);
    it = end;

    // count number of digits
    const auto digits = end - s.c_str() - pos - 1;

    // we support up to 9 digits (nanoseconds), and ignore more than that
    if (digits <= 9) {
      // number of nanoseconds in a second
      uint64_t scale = 1'000'000'000;

      for (auto i = decltype(digits){0}; i < digits; ++i) {
        scale /= 10;
      }

      ns = scale * fraction;
    }
  }

  // time zone offset in seconds
  int64_t tz_offset = 0;

  if ('Z' != *it && 'z' != *it) {
    // {+|-}HH:MM
    const bool negative = '-' == *it;
    // move past sign
    ++it;

    char *end = nullptr;
    tz_offset = 60 * 60 * std::strtoll(it, &end, 10);
    ++end;
    tz_offset += 60 * std::strtoll(end, &end, 10);

    if (negative) {
      tz_offset = -tz_offset;
    }
  }

  struct tm tm;
  memset(&tm, 0, sizeof(struct tm));
  // RFC specifies separator to be 'T', but notes this could be a lower case
  // 't', or any other character which improves readability
  char separator = 0;
  sscanf(s.c_str(), "%4d-%2d-%2d%c%2d:%2d:%2d", &tm.tm_year, &tm.tm_mon,
         &tm.tm_mday, &separator, &tm.tm_hour, &tm.tm_min, &tm.tm_sec);

  // years start in 1900
  tm.tm_year -= 1900;
  // month is zero-based
  --tm.tm_mon;

  // subtract time zone offset to convert the given time to UTC
  // note: time_t always represents a UTC time in seconds
  auto ret = std::chrono::system_clock::from_time_t(
      time_from_struct_tm_utc(&tm) - tz_offset);

  ret += std::chrono::duration_cast<std::chrono::system_clock::duration>(
      std::chrono::nanoseconds(ns));

  return ret;
}

}  // namespace shcore
