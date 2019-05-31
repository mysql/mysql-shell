/*
 * Copyright (c) 2019, Oracle and/or its affiliates. All rights reserved.
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

#include "mysqlshdk/libs/mysql/clone.h"
#include "mysqlshdk/libs/mysql/instance.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "unittest/test_utils/mocks/mysqlshdk/libs/db/mock_result.h"
#include "unittest/test_utils/mocks/mysqlshdk/libs/db/mock_session.h"
#include "unittest/test_utils/shell_base_test.h"

namespace testing {
class Clone_test : public tests::Shell_base_test {
 protected:
  void SetUp() { tests::Shell_base_test::SetUp(); }

  void TearDown() { tests::Shell_base_test::TearDown(); }
};

TEST_F(Clone_test, plugin_installation) {
  using mysqlshdk::db::Type;

  std::shared_ptr<Mock_session> mock_session = std::make_shared<Mock_session>();
  mysqlshdk::mysql::Instance instance{mock_session};

  // TEST: Clone plugin already installed.
  {
    SCOPED_TRACE("Clone plugin already installed.");
    mock_session
        ->expect_query(
            "SELECT plugin_status FROM information_schema.plugins WHERE "
            "plugin_name = 'clone'")
        .then_return({{"", {"plugin_status"}, {Type::String}, {{"ACTIVE"}}}});
    mock_session
        ->expect_query(
            "show GLOBAL variables where `variable_name` in"
            " ('super_read_only')")
        .then_return({{"",
                       {"Variable_name", "Value"},
                       {Type::String, Type::String},
                       {{"super_read_only", "OFF"}}}});

    bool res = mysqlshdk::mysql::install_clone_plugin(instance, nullptr);
    EXPECT_FALSE(res);
  }

  // TEST: Error trying to install DISABLED clone plugin (already installed).
  {
    SCOPED_TRACE(
        "Error trying to install DISABLED clone plugin (already installed).");
    mock_session
        ->expect_query(
            "SELECT plugin_status FROM information_schema.plugins WHERE "
            "plugin_name = 'clone'")
        .then_return({{"", {"plugin_status"}, {Type::String}, {{"DISABLED"}}}});
    mock_session
        ->expect_query(
            "show GLOBAL variables where `variable_name` in"
            " ('super_read_only')")
        .then_return({{"",
                       {"Variable_name", "Value"},
                       {Type::String, Type::String},
                       {{"super_read_only", "OFF"}}}});

    EXPECT_THROW_LIKE(mysqlshdk::mysql::install_clone_plugin(instance, nullptr),
                      std::runtime_error,
                      "clone plugin is DISABLED and cannot be enabled on "
                      "runtime. Please enable the plugin and restart the "
                      "server.");
  }

  // NOTE: Currently not possible to test the instalation of the plugin when
  //       not already installed, since it will require the execution of the
  //       same query "SELECT plugin_status FROM information_schema.plugins "
  //       "WHERE plugin_name = 'clone'" with different results and this is not
  //       supported by mock_session/mock_result because the same result is
  //       always returned for both (the last one defined).

  // TEST: Uninstall clone plugin.
  {
    SCOPED_TRACE("Uninstall clone plugin (ACTIVE).");
    mock_session
        ->expect_query(
            "SELECT plugin_status FROM information_schema.plugins WHERE "
            "plugin_name = 'clone'")
        .then_return({{"", {"plugin_status"}, {Type::String}, {{"ACTIVE"}}}});
    mock_session
        ->expect_query(
            "show GLOBAL variables where `variable_name` in"
            " ('super_read_only')")
        .then_return({{"",
                       {"Variable_name", "Value"},
                       {Type::String, Type::String},
                       {{"super_read_only", "OFF"}}}});

    EXPECT_CALL(*mock_session, execute("UNINSTALL PLUGIN `clone`"));

    bool res = mysqlshdk::mysql::uninstall_clone_plugin(instance, nullptr);
    EXPECT_TRUE(res);
  }
  {
    SCOPED_TRACE("Uninstall clone plugin (DISABLED).");
    mock_session
        ->expect_query(
            "SELECT plugin_status FROM information_schema.plugins WHERE "
            "plugin_name = 'clone'")
        .then_return({{"", {"plugin_status"}, {Type::String}, {{"DISABLED"}}}});
    mock_session
        ->expect_query(
            "show GLOBAL variables where `variable_name` in"
            " ('super_read_only')")
        .then_return({{"",
                       {"Variable_name", "Value"},
                       {Type::String, Type::String},
                       {{"super_read_only", "OFF"}}}});

    EXPECT_CALL(*mock_session, execute("UNINSTALL PLUGIN `clone`"));

    bool res = mysqlshdk::mysql::uninstall_clone_plugin(instance, nullptr);
    EXPECT_TRUE(res);
  }

  // TEST: Try to uninstall clone plugin (already uninstalled).
  {
    SCOPED_TRACE("Try to uninstall clone plugin (already uninstalled).");
    mock_session
        ->expect_query(
            "SELECT plugin_status FROM information_schema.plugins WHERE "
            "plugin_name = 'clone'")
        .then_return({{"", {"plugin_status"}, {Type::String}, {}}});
    mock_session
        ->expect_query(
            "show GLOBAL variables where `variable_name` in"
            " ('super_read_only')")
        .then_return({{"",
                       {"Variable_name", "Value"},
                       {Type::String, Type::String},
                       {{"super_read_only", "OFF"}}}});

    bool res = mysqlshdk::mysql::uninstall_clone_plugin(instance, nullptr);
    EXPECT_FALSE(res);
  }
}

}  // namespace testing
