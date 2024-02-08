/*
 * Copyright (c) 2015, 2024, Oracle and/or its affiliates.
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
#include <fstream>
#include <string>

#include "mysqlshdk/include/scripting/common.h"
#include "mysqlshdk/include/shellcore/base_session.h"
#include "mysqlshdk/libs/utils/utils_file.h"
#include "mysqlshdk/libs/utils/utils_path.h"
#include "unittest/gtest_clean.h"
#include "unittest/test_utils.h"
#include "unittest/test_utils/mocks/gmock_clean.h"

namespace shcore {
class Shell_application_log_tests : public Shell_core_test_wrapper {
 protected:
  static int i;
  std::shared_ptr<shcore::Logger> m_logger;

  static std::string error;

  static void my_hook(const shcore::Logger::Log_entry &entry, void *) {
    EXPECT_THAT(std::string{entry.message}, ::testing::HasSubstr(error));
    i++;
  }

  // You can define per-test set-up and tear-down logic as usual.
  void SetUp() override {
    Shell_core_test_wrapper::SetUp();
    Shell_application_log_tests::i = 0;

    const auto log_path =
        shcore::path::join_path(shcore::get_user_config_path(), "mysqlsh.log");
    m_logger = shcore::Logger::create_instance(log_path.c_str(), false,
                                               shcore::Logger::LOG_DEBUG);
    m_logger->attach_log_hook(my_hook);

    _interactive_shell->process_line("\\js");

    exec_and_out_equals("var mysql = require('mysql');");
    exec_and_out_equals("var session = mysql.getClassicSession('" + _mysql_uri +
                        "');");
  }

  void set_options() override { _opts->set_interactive(false); }

  void TearDown() override { m_logger->detach_log_hook(my_hook); }
};

std::string Shell_application_log_tests::error = "";

#ifdef HAVE_V8
TEST_F(Shell_application_log_tests, test) {
  mysqlsh::Scoped_logger log(m_logger);
  // issue a stmt with syntax error, then check the log.
  error =
      R"(SyntaxError: missing ) after argument list at (shell):1:7
in print('x';
         ^^^)";
  execute("print('x';");

  error =
      "You have an error in your SQL syntax; check the manual that corresponds "
      "to your MySQL server version for the right syntax to use near '' at "
      "line 1";
  execute("session.runSql('select * from sakila.actor1 limit');");

  // The hook was invoked, 3 times:
  //   1) execute print('x';
  //   2) and 3) execute session.runSql(stmt) + stmt is erroneous,
  //      so global sql log logs error message
  EXPECT_EQ(3, Shell_application_log_tests::i);

  execute("session.close();");
}
#endif

int Shell_application_log_tests::i;
}  // namespace shcore
