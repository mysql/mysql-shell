/*
 * Copyright (c) 2021, Oracle and/or its affiliates.
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

#include "unittest/gprod_clean.h"
#include "unittest/gtest_clean.h"

#include "mysqlshdk/libs/utils/utils_time.h"

#include <chrono>
#include <stdexcept>
#include <string>

namespace shcore {

TEST(utils_time, time_point_to_rfc3339) {
  const uint64_t epoch = 1637239475;
  const std::chrono::time_point<std::chrono::system_clock> tp{
      std::chrono::seconds{epoch}};
  EXPECT_EQ("2021-11-18T12:44:35Z", time_point_to_rfc3339(tp));
}

TEST(utils_time, rfc3339_to_time_point) {
  const auto EXPECT_THROWS = [](const std::string &rfc) {
    EXPECT_THROW_MSG(rfc3339_to_time_point(rfc), std::runtime_error,
                     "Timestamp '" + rfc + "' is not in RFC3339 format.");
  };

  EXPECT_THROWS("2021-11-18");
  EXPECT_THROWS("2021-11-18T12:44:35");
  EXPECT_THROWS("2021-11-18T12:44:35.");
  EXPECT_THROWS("2021-11-18T12:44:35.Z");
  EXPECT_THROWS("2021-11-18T12:44:35.+00:00");
  EXPECT_THROWS("2021-11-18T12:44:35x");
  EXPECT_THROWS("2021-11-18T12:44:35.1x");
  EXPECT_THROWS("2021-11-18T12:44:35.1x00");
  EXPECT_THROWS("2021-11-18T12:44:35.1x00:00");

  const auto EXPECT_TP = [](const std::string &rfc, const std::string &expected,
                            uint64_t expected_ns) {
    SCOPED_TRACE(rfc);

    const auto tp = rfc3339_to_time_point(rfc);
    EXPECT_EQ(expected, time_point_to_rfc3339(tp));
    EXPECT_EQ(
        std::chrono::duration_cast<std::chrono::system_clock::duration>(
            std::chrono::nanoseconds(expected_ns))
            .count(),
        tp.time_since_epoch().count() % std::chrono::system_clock::period::den);
  };

  EXPECT_TP("1996-12-19T16:39:57-08:00", "1996-12-20T00:39:57Z", 0);

  EXPECT_TP("2021-11-18T12:44:35Z", "2021-11-18T12:44:35Z", 0);
  EXPECT_TP("2021-11-18T12:44:35.5+00:00", "2021-11-18T12:44:35Z", 500'000'000);
  EXPECT_TP("2021-11-18T12:44:35.123Z", "2021-11-18T12:44:35Z", 123'000'000);
  EXPECT_TP("2021-11-18T12:44:35.123456-00:00", "2021-11-18T12:44:35Z",
            123'456'000);
  EXPECT_TP("2021-11-18T12:44:35.123456789Z", "2021-11-18T12:44:35Z",
            123'456'789);

  EXPECT_TP("2021-11-18T12:44:35+01:00", "2021-11-18T11:44:35Z", 0);
  EXPECT_TP("2021-11-18T12:44:35.5+10:30", "2021-11-18T02:14:35Z", 500'000'000);
  EXPECT_TP("2021-11-18T12:44:35.123+02:20", "2021-11-18T10:24:35Z",
            123'000'000);
  EXPECT_TP("2021-11-18T12:44:35.123456-01:00", "2021-11-18T13:44:35Z",
            123'456'000);
  EXPECT_TP("2021-11-18T12:44:35.123456789-10:30", "2021-11-18T23:14:35Z",
            123'456'789);
  EXPECT_TP("2021-11-18T12:44:35.123456789-02:20", "2021-11-18T15:04:35Z",
            123'456'789);
}

}  // namespace shcore
