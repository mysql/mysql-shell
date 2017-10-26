/*
 * Copyright (c) 2017, Oracle and/or its affiliates. All rights reserved.
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

#include <gtest/gtest.h>
#include "utils/utils_string.h"

namespace shcore {

TEST(utils_string, split) {
  {
    const auto s = str_split("one/two/three/", "/");
    const std::vector<std::string> expect = {"one", "two", "three", ""};
    EXPECT_EQ(s, expect);
  }
  {
    const auto s = str_split("", "/");
    const std::vector<std::string> expect = {""};
    EXPECT_EQ(s, expect);
  }
  {
    const auto s = str_split("/", "/");
    const std::vector<std::string> expect = {"", ""};
    EXPECT_EQ(s, expect);
  }
  {
    const auto s = str_split("//", "/");
    const std::vector<std::string> expect = {"", "", ""};
    EXPECT_EQ(s, expect);
  }
  {
    const auto s = str_split("//a", "/");
    const std::vector<std::string> expect = {"", "", "a"};
    EXPECT_EQ(s, expect);
  }
}
}  // namespace shcore
