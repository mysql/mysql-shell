/*
 * Copyright (c) 2018, 2024, Oracle and/or its affiliates.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.0,
 * as published by the Free Software Foundation.
 *
 * This program is designed to work with certain software (including
 * but not limited to OpenSSL) that is licensed under separate terms,
 * as designated in a particular file or component or in included license
 * documentation.  The authors of MySQL hereby grant you an additional
 * permission to link the program and your derivative works with the
 * separately licensed software that they have either included with
 * the program or referenced in the documentation.
 *
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
  EXPECT_EQ("1.00 query/s",
            format_throughput_items("query", "queries", 1, 0.25));
  EXPECT_EQ("1.00 query/s",
            format_throughput_items("query", "queries", 1, 0.5));
  EXPECT_EQ("2.00 queries/s",
            format_throughput_items("query", "queries", 2, 0.5));
  EXPECT_EQ("1.00 query/s", format_throughput_items("query", "queries", 1, 1));
  EXPECT_EQ("65.00 queries/s",
            format_throughput_items("query", "queries", 65, 1));
  EXPECT_EQ("10.00K queries/s",
            format_throughput_items("query", "queries", 10000, 1));
  EXPECT_EQ("11.00M queries/s",
            format_throughput_items("query", "queries", 11000000, 1));
  EXPECT_EQ("11.00G queries/s",
            format_throughput_items("query", "queries", 11000000000, 1));
  EXPECT_EQ("11.23T queries/s",
            format_throughput_items("query", "queries", 11230000000000, 1));

  EXPECT_EQ("0.00 documents/s",
            format_throughput_items("document", "documents", 0, 1));
  EXPECT_EQ("0.50 document/s",
            format_throughput_items("document", "documents", 1, 2));
  EXPECT_EQ("1.00 document/s",
            format_throughput_items("document", "documents", 1, 1));
  EXPECT_EQ("1.50 documents/s",
            format_throughput_items("document", "documents", 3, 2));
  EXPECT_EQ("400.00 documents/s",
            format_throughput_items("document", "documents", 400, 1));
  EXPECT_EQ("410.00 documents/s",
            format_throughput_items("document", "documents", 410 * 2, 2));
  EXPECT_EQ("12.00K documents/s",
            format_throughput_items("document", "documents", 12000, 1));
  EXPECT_EQ("403.81 documents/s",
            format_throughput_items("document", "documents", 25359, 62.8));

  EXPECT_EQ("1.00 byte/s", format_throughput_items("byte", "bytes", 1e6, 1e6));
  EXPECT_EQ("0.50 byte/s", format_throughput_items("byte", "bytes", 1e6, 2e6));
  EXPECT_EQ("2.00 bytes/s", format_throughput_items("byte", "bytes", 2e6, 1e6));
}

TEST(UtilsStrformat, format_throughput_bytes) {
  EXPECT_EQ("65.00 B/s", format_throughput_bytes(65, 1));
  EXPECT_EQ("10.00 KB/s", format_throughput_bytes(10000, 1));
  EXPECT_EQ("11.00 MB/s", format_throughput_bytes(11000000, 1));
  EXPECT_EQ("11.00 GB/s", format_throughput_bytes(11000000000, 1));
  EXPECT_EQ("11.23 TB/s", format_throughput_bytes(11230000000000, 1));
  EXPECT_EQ("860.18 KB/s",
            format_throughput_bytes(1091311621, 21 * 60 + 8 + 0.6972));
  EXPECT_EQ("1.37 GB/s", format_throughput_bytes(1372178973, 0.9119));
  EXPECT_EQ("1.37 GB/s", format_throughput_bytes(1372178973, 0.42));
}

}  // namespace utils
}  // namespace mysqlshdk
