/*
* Copyright (c) 2014, 2017, Oracle and/or its affiliates. All rights reserved.
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

#include "utils_time.h"
#include "utils/utils_string.h"
#include <cmath>
#include <ctime>

#if defined(WIN32)
#include <time.h>
#else
#include <sys/times.h>
#ifdef _SC_CLK_TCK // For mit-pthreads
#undef CLOCKS_PER_SEC
#define CLOCKS_PER_SEC (sysconf(_SC_CLK_TCK))
#endif
#endif

using namespace shcore;

uint64_t MySQL_timer::get_time() {
#if defined(WIN32)
  return clock();
#else
  struct tms tms_tmp;
  return times(&tms_tmp);
#endif
}

uint64_t MySQL_timer::start() {
  return (_start = get_time());
}

uint64_t MySQL_timer::end() {
  return (_end = get_time());
}

/**
 Write as many as 52+1 bytes to buff, in the form of a legible duration of time.

 len("4294967296 days, 23 hours, 59 minutes, 60.00 seconds")  ->  52

 Originally being measured at the client, the raw time was given in CLOCKS so real time was calculated
 dividing raw_time/CLOCKS_PER_SEC.

 Later we needed to parse time data coming from the server in seconds, hence the in_seconds parameter is
 used for such cases where raw time is already time in seconds.
 */

std::string MySQL_timer::format_legacy(uint64_t raw_time, int part_seconds, bool in_seconds) {
  std::string str_duration;

  int days = 0;
  int hours = 0;
  int minutes = 0;
  float seconds = 0.0;

  MySQL_timer::parse_duration(raw_time, days, hours, minutes, seconds, in_seconds);

  if (days)
    str_duration.append(str_format("%d %s", days, (days == 1 ? "day" : "days")));

  if (days || hours)
    str_duration.append(str_format("%s%d %s", (str_duration.empty() ? "" : ", "), hours, (hours == 1 ? "hour" : "hours")));

  if (days || hours || minutes)
    str_duration.append(str_format("%s%d %s", (str_duration.empty() ? "" : ", "), minutes, (minutes == 1 ? "minute" : "minutes")));

  if (part_seconds)
    str_duration.append(str_format("%s%.2f sec", (str_duration.empty() ? "" : ", "), seconds));
  else
    str_duration.append(str_format("%s%d sec", (str_duration.empty() ? "" : ", "), (int)seconds));

  return str_duration;
}

uint64_t MySQL_timer::seconds_to_duration(float s) {
  return static_cast<uint64_t>(s * CLOCKS_PER_SEC);
}

void MySQL_timer::parse_duration(uint64_t raw_time, int &days, int &hours, int &minutes, float &seconds, bool in_seconds) {
  double duration;

  if (in_seconds)
    duration = raw_time;
  else {
    uint64_t closk_per_second = CLOCKS_PER_SEC;
    duration = (double)raw_time / closk_per_second;
  }

  std::string str_duration;

  double minute_seconds = 60.0;
  double hour_seconds = 3600.0;
  double day_seconds = hour_seconds * 24;

  days = 0;
  hours = 0;
  minutes = 0;
  seconds = 0;

  if (duration >= day_seconds) {
    days = (int)floor(duration / day_seconds);
    duration -= days * day_seconds;
  }

  if (duration >= hour_seconds) {
    hours = (int)floor(duration / hour_seconds);
    duration -= hours * hour_seconds;
  }

  if (duration >= minute_seconds) {
    minutes = (int)floor(duration / minute_seconds);
    duration -= minutes * minute_seconds;
  }

  seconds = duration;
}
