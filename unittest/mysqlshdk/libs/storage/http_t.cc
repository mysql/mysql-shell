/*
 * Copyright (c) 2021, Oracle and/or its affiliates. All rights reserved.
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
#include "unittest/test_utils/shell_test_env.h"

#include "mysqlshdk/libs/storage/backend/http.h"

namespace mysqlshdk {
namespace storage {
namespace backend {

TEST(Http_get_test, full_path_constructor) {
  const auto EXPECT_BASE_AND_PATH = [](const std::string &base,
                                       const std::string &path) {
    SCOPED_TRACE("base: '" + base + "', path: '" + path + "'");

    const auto expected_base = base + ('/' == base.back() ? "" : "/");
    const auto expected_url = expected_base + path;
    const auto get = Http_get(expected_url);

    EXPECT_EQ(expected_base, get.m_base.real());
    EXPECT_EQ(path, get.m_path);
    EXPECT_EQ(expected_url, get.full_path().real());
  };

  EXPECT_THROW_LIKE(Http_get(""), std::logic_error, "URI is empty");
  EXPECT_THROW_LIKE(Http_get("no scheme"), std::logic_error,
                    "URI does not have a scheme");

  EXPECT_BASE_AND_PATH("https://example.com", "");
  EXPECT_BASE_AND_PATH("https://example.com/", "");
  EXPECT_BASE_AND_PATH("https://example.com/", "exe.txt");
  EXPECT_BASE_AND_PATH("https://example.com/dir/", "exe.txt");
  EXPECT_BASE_AND_PATH("https://example.com/dir/dir2/", "exe.txt");
}

}  // namespace backend
}  // namespace storage
}  // namespace mysqlshdk
