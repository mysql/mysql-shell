/* Copyright (c) 2014, 2016, Oracle and/or its affiliates. All rights reserved.

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; version 2 of the License.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA */

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <string>
#include <boost/shared_ptr.hpp>
#include <boost/pointer_cast.hpp>

#include "gtest/gtest.h"
#include "shellcore/types.h"
#include "shellcore/lang_base.h"
#include "shellcore/types_cpp.h"
#include "shellcore/common.h"

#include "shellcore/shell_core.h"
#include "shellcore/shell_sql.h"
#include "../modules/base_session.h"
#include "../modules/base_resultset.h"
#include "../src/shell_resultset_dumper.h"
#include "test_utils.h"
#include "utils/utils_file.h"
#include <boost/bind.hpp>

namespace shcore {
  namespace shell_core_tests {
    class Interactive_shell_test : public Shell_core_test_wrapper
    {
    protected:
      std::string _file_name;
      int _ret_val;

      boost::shared_ptr<Shell_command_line_options> _options;

      virtual void SetUp()
      {
        Shell_core_test_wrapper::SetUp();

        char **argv = NULL;
        Shell_command_line_options options(0, argv);
        _options.reset(new Shell_command_line_options(0, argv));
      }

      void init()
      {
        _interactive_shell.reset(new Interactive_shell(*_options.get(), &output_handler.deleg));
      }

      void connect()
      {
        std::cout << _mysql_uri;

        _interactive_shell->process_line("\\connect_classic " + _mysql_uri);
      }
    };

    TEST_F(Interactive_shell_test, warning_insecure_password)
    {
      // Test secure call passing uri with no password (will be prompted)
      _options->uri = "root@localhost";
      init();
      output_handler.ret_pwd = "whatever";

      _interactive_shell->connect(true);
      MY_EXPECT_STDOUT_NOT_CONTAINS("mysqlx: [Warning] Using a password on the command line interface can be insecure.");
      output_handler.wipe_all();

      // Test non secure call passing uri and password with cmd line params
      _options->uri = "root@localhost";
      _options->password = "whatever";
      init();

      _interactive_shell->connect(true);
      MY_EXPECT_STDOUT_CONTAINS("mysqlx: [Warning] Using a password on the command line interface can be insecure.");
      output_handler.wipe_all();
      _options->password = NULL; // Options cleanup

      // Test secure call passing uri with empty password
      _options->uri = "root:@localhost";
      init();

      _interactive_shell->connect(true);
      MY_EXPECT_STDOUT_NOT_CONTAINS("mysqlx: [Warning] Using a password on the command line interface can be insecure.");
      output_handler.wipe_all();

      // Test non secure call passing uri with password
      _options->uri = "root:whatever@localhost";
      init();

      _interactive_shell->connect(true);
      MY_EXPECT_STDOUT_CONTAINS("mysqlx: [Warning] Using a password on the command line interface can be insecure.");
      output_handler.wipe_all();
    }
  }
}