/*
* Copyright (c) 2014, 2016, Oracle and/or its affiliates. All rights reserved.
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License as
* published by the Free Software Foundation; version 2 of the
* License.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
* 02110-1301  USA
*/

#include "utils_time.h"
#include "utils/utils_string.h"
#include <boost/format.hpp>
#include <cmath>

#if defined(WIN32)
#include <time.h>
#else
#include <sys/times.h>
#ifdef _SC_CLK_TCK				// For mit-pthreads
#undef CLOCKS_PER_SEC
#define CLOCKS_PER_SEC (sysconf(_SC_CLK_TCK))
#endif
#endif

using namespace shcore;

unsigned long MySQL_timer::get_time() {
#if defined(WIN32)
  return clock();
#else
  struct tms tms_tmp;
  return times(&tms_tmp);
#endif
}

unsigned long MySQL_timer::start() {
  return (_start = get_time());
}

unsigned long MySQL_timer::end() {
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

std::string MySQL_timer::format_legacy(unsigned long raw_time, int part_seconds, bool in_seconds) {
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

void MySQL_timer::parse_duration(unsigned long raw_time, int &days, int &hours, int &minutes, float &seconds, bool in_seconds) {
  double duration;

  if (in_seconds)
    duration = raw_time;
  else {
    unsigned long closk_per_second = CLOCKS_PER_SEC;
    duration = (double)(raw_time) / closk_per_second;
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
