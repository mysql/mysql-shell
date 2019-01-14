/*
 * Copyright (c) 2018, 2019, Oracle and/or its affiliates. All rights reserved.
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

#include <string>

#include "mysqlshdk/libs/mysql/replication.h"
#include "unittest/test_utils/mocks/mysqlshdk/libs/db/mock_session.h"
#include "unittest/test_utils/shell_base_test.h"

namespace testing {

class Replication_test : public tests::Shell_base_test {
 protected:
  void SetUp() { tests::Shell_base_test::SetUp(); }

  void TearDown() { tests::Shell_base_test::TearDown(); }
};

TEST_F(Replication_test, get_report_host) {
  using mysqlshdk::db::Type;

  std::shared_ptr<Mock_session> mock_session = std::make_shared<Mock_session>();
  mysqlshdk::mysql::Instance instance{mock_session};

  // Error if report_host is empty.
  mock_session->expect_query("SELECT @@REPORT_HOST")
      .then_return({{"", {"@@REPORT_HOST"}, {Type::String}, {{""}}}});
  EXPECT_THROW(mysqlshdk::mysql::get_report_host(instance), std::runtime_error);

  // Report host properly if report_host is valid (not empty).
  bool report_host_set;
  mock_session->expect_query("SELECT @@REPORT_HOST")
      .then_return(
          {{"", {"@@REPORT_HOST"}, {Type::String}, {{"my_report_host"}}}});
  EXPECT_EQ(mysqlshdk::mysql::get_report_host(instance, &report_host_set),
            "my_report_host");
  EXPECT_TRUE(report_host_set);

  // Report host properly using hostname if report_host is not defined (NULL).
  mock_session->expect_query("SELECT @@REPORT_HOST")
      .then_return({{"", {"@@REPORT_HOST"}, {Type::Null}, {{"___NULL___"}}}});
  mock_session->expect_query("SELECT @@HOSTNAME")
      .then_return({{"", {"@@HOSTNAME"}, {Type::String}, {{"my_hostname"}}}});
  EXPECT_EQ(mysqlshdk::mysql::get_report_host(instance, &report_host_set),
            "my_hostname");
  EXPECT_FALSE(report_host_set);
}

}  // namespace testing
