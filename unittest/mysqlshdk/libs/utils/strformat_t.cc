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

#include <cstdio>
#include <cstdlib>
#include <string>

#include "mysqlshdk/libs/utils/strformat.h"
#include "unittest/gtest_clean.h"

namespace mysqlshdk {
namespace utils {

TEST(UtilsStrformat, format_seconds) {
  EXPECT_EQ("0.0000 sec", format_seconds(0));
  EXPECT_EQ("0.0001 sec", format_seconds(0.0001));
  EXPECT_EQ("1.0000 sec", format_seconds(1));
  EXPECT_EQ("50.0000 sec", format_seconds(50));
  EXPECT_EQ("2 min 30.0000 sec", format_seconds(150));
  EXPECT_EQ("59 min 0.5000 sec", format_seconds(59 * 60 + 0.5));
  EXPECT_EQ("2 hours 0.5000 sec", format_seconds(2 * 60 * 60 + 0.5));
  EXPECT_EQ("1 day 6 hours 12 min 34.5678 sec",
            format_seconds(30 * 60 * 60 + 12 * 60 + 34.5678));
  EXPECT_EQ("5 days 34.5678 sec", format_seconds(5 * 24 * 60 * 60 + 34.5678));
}

TEST(UtilsStrformat, format_bytes) {
  EXPECT_EQ("0 bytes", format_bytes(0));
  EXPECT_EQ("1000 bytes", format_bytes(1000));
  EXPECT_EQ("2.00 KB", format_bytes(2000));
  EXPECT_EQ("2.00 MB", format_bytes(2000000));
  EXPECT_EQ("2.00 GB", format_bytes(2000000000));
  EXPECT_EQ("2.00 TB", format_bytes(2000000000000));
}

TEST(UtilsStrformat, format_throughput_items) {
  EXPECT_EQ("65.00 queries/s", format_throughput_items("queries", 65, 1));
  EXPECT_EQ("10.00K queries/s", format_throughput_items("queries", 10000, 1));
  EXPECT_EQ("11.00M queries/s",
            format_throughput_items("queries", 11000000, 1));
  EXPECT_EQ("11.00G queries/s",
            format_throughput_items("queries", 11000000000, 1));
  EXPECT_EQ("11.23T queries/s",
            format_throughput_items("queries", 11230000000000, 1));
}

TEST(UtilsStrformat, format_throughput_bytes) {
  EXPECT_EQ("65.00 B/s", format_throughput_bytes(65, 1));
  EXPECT_EQ("10.00 KB/s", format_throughput_bytes(10000, 1));
  EXPECT_EQ("11.00 MB/s", format_throughput_bytes(11000000, 1));
  EXPECT_EQ("11.00 GB/s", format_throughput_bytes(11000000000, 1));
  EXPECT_EQ("11.23 TB/s", format_throughput_bytes(11230000000000, 1));
}

}  // namespace utils
}  // namespace mysqlshdk
