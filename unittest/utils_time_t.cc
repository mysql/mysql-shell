/*
* Copyright (c) 2015, 2016, Oracle and/or its affiliates. All rights reserved.
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

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <string>
#include <stack>

#include "gtest/gtest.h"
#include "utils/utils_time.h"

#if defined(WIN32)
#include <time.h>
#else
#include <sys/times.h>
#ifdef _SC_CLK_TCK				// For mit-pthreads
#undef CLOCKS_PER_SEC
#define CLOCKS_PER_SEC (sysconf(_SC_CLK_TCK))
#endif
#endif

namespace shcore {
namespace utils_time_tests {
TEST(MySQL_timer_tests, parse_duration) {
  unsigned long raw_time;
  int days;
  int hours;
  int minutes;
  float seconds;
  unsigned long clocks_per_second = CLOCKS_PER_SEC;

  unsigned long clocks_per_minute = 60 * clocks_per_second;
  unsigned long clocks_per_hour = 60 * clocks_per_minute;
  unsigned long clocks_per_day = 24 * clocks_per_hour;

  // Sets some seconds as value.
  raw_time = 56 * CLOCKS_PER_SEC;
  MySQL_timer::parse_duration(raw_time, days, hours, minutes, seconds);
  EXPECT_EQ(0, days);
  EXPECT_EQ(0, hours);
  EXPECT_EQ(0, minutes);
  EXPECT_EQ(56, seconds);

  // Sets some minutes and seconds as value.
  raw_time = 5 * clocks_per_minute + 56 * CLOCKS_PER_SEC;
  MySQL_timer::parse_duration(raw_time, days, hours, minutes, seconds);
  EXPECT_EQ(0, days);
  EXPECT_EQ(0, hours);
  EXPECT_EQ(5, minutes);
  EXPECT_EQ(56, seconds);

  // Sets 3 hours, 5 minutes and 56 seconds as value.
  raw_time = 3 * clocks_per_hour + 5 * clocks_per_minute + 56 * CLOCKS_PER_SEC;
  MySQL_timer::parse_duration(raw_time, days, hours, minutes, seconds);
  EXPECT_EQ(0, days);
  EXPECT_EQ(3, hours);
  EXPECT_EQ(5, minutes);
  EXPECT_EQ(56, seconds);

  // Sets 8 days, 3 hours, 5 minutes and 56 seconds as value.
  raw_time = 8 * clocks_per_day + 3 * clocks_per_hour + 5 * clocks_per_minute + 56 * CLOCKS_PER_SEC;
  MySQL_timer::parse_duration(raw_time, days, hours, minutes, seconds);
  EXPECT_EQ(8, days);
  EXPECT_EQ(3, hours);
  EXPECT_EQ(5, minutes);
  EXPECT_EQ(56, seconds);
}

TEST(MySQL_timer_tests, format_legacy) {
  unsigned long raw_time;
  std::string formatted;

  unsigned long clocks_per_minute = 60 * CLOCKS_PER_SEC;
  unsigned long  clocks_per_hour = 60 * clocks_per_minute;
  unsigned long  clocks_per_day = 24 * clocks_per_hour;

  // Sets some seconds as value.
  raw_time = 1.5 * CLOCKS_PER_SEC;
  formatted = MySQL_timer::format_legacy(raw_time, true);
  EXPECT_EQ("1.50 sec", formatted);

  formatted = MySQL_timer::format_legacy(raw_time, false);
  EXPECT_EQ("1 sec", formatted);

  // Sets a minute and seconds as value
  raw_time = clocks_per_minute + 1.5 * CLOCKS_PER_SEC;
  formatted = MySQL_timer::format_legacy(raw_time, true);
  EXPECT_EQ("1 minute, 1.50 sec", formatted);

  // Sets minutes and seconds as value
  raw_time = 5 * clocks_per_minute + 1.5 * CLOCKS_PER_SEC;
  formatted = MySQL_timer::format_legacy(raw_time, true);
  EXPECT_EQ("5 minutes, 1.50 sec", formatted);

  // Sets an hour, a minute and seconds as value
  raw_time = clocks_per_hour + clocks_per_minute + 1.5 * CLOCKS_PER_SEC;
  formatted = MySQL_timer::format_legacy(raw_time, true);
  EXPECT_EQ("1 hour, 1 minute, 1.50 sec", formatted);

  // Sets hours, minutes and seconds as value
  raw_time = 3 * clocks_per_hour + 5 * clocks_per_minute + 1.5 * CLOCKS_PER_SEC;
  formatted = MySQL_timer::format_legacy(raw_time, true);
  EXPECT_EQ("3 hours, 5 minutes, 1.50 sec", formatted);

  // Sets a day, an hour, a minute and seconds as value
  raw_time = clocks_per_day + clocks_per_hour + clocks_per_minute + 1.5 * CLOCKS_PER_SEC;
  formatted = MySQL_timer::format_legacy(raw_time, true);
  EXPECT_EQ("1 day, 1 hour, 1 minute, 1.50 sec", formatted);

  // Sets days, hours, minutes and seconds as value
  raw_time = 2 * clocks_per_day + 3 * clocks_per_hour + 5 * clocks_per_minute + 1.5 * CLOCKS_PER_SEC;
  formatted = MySQL_timer::format_legacy(raw_time, true);
  EXPECT_EQ("2 days, 3 hours, 5 minutes, 1.50 sec", formatted);
}
}
}
