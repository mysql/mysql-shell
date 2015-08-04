/*
* Copyright (c) 2015, Oracle and/or its affiliates. All rights reserved.
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

#include "shellcore/common.h"
#include "gtest/gtest.h"
#include "test_utils.h"
#include "base_session.h"
#include "utils/utils_file.h"

namespace shcore {
  class Shell_application_log_tests : public Shell_core_test_wrapper
  {
  protected:
    static int i;
    ngcommon::Logger* _logger;

    static void my_hook(const char* message, ngcommon::Logger::LOG_LEVEL level, const char* domain)
    {
      std::string msg = "You have an error in your SQL syntax; check the manual that corresponds to your MySQL server version for the right syntax to use near '' at line 1";
      std::string message_s(message);
      EXPECT_TRUE(message_s.find(msg) != std::string::npos);
      i++;
    }

    // You can define per-test set-up and tear-down logic as usual.
    virtual void SetUp()
    {
      Shell_core_test_wrapper::SetUp();
      Shell_application_log_tests::i = 0;

      bool initilaized(false);

      std::string log_path = shcore::get_user_config_path();
      log_path += "mysqlx.log";
      ngcommon::Logger::create_instance(log_path.c_str(), false, ngcommon::Logger::LOG_ERROR);
      _logger = ngcommon::Logger::singleton();
      _logger->attach_log_hook(my_hook);

      _shell_core->switch_mode(Shell_core::Mode_JScript, initilaized);

      exec_and_out_equals("var mysql = require('mysql').mysql;");
      exec_and_out_equals("var session = mysql.getSession('" + _mysql_uri + "');");
    }

    virtual void TearDown()
    {
      _logger->detach_log_hook(my_hook);
    }
  };

  TEST_F(Shell_application_log_tests, test)
  {
    // issue an stmt with syntax error, then check the log.
    execute("print('x';");
    std::string std_err = "(shell):1:9: SyntaxError: Unexpected token ;\nin print('x';\n            ^\nSyntaxError: Unexpected token ;\n";
    EXPECT_TRUE(std_err == this->output_handler.std_err);
    execute("session.executeSql('select * from sakila.actor1 limit').all();");
    // The hook was invoked
    EXPECT_EQ(1, Shell_application_log_tests::i);
  }

  int Shell_application_log_tests::i;
}