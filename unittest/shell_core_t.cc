/* Copyright (c) 2014, 2015, Oracle and/or its affiliates. All rights reserved.

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
#include "../modules/mod_session.h"
#include "../modules/mod_db.h"
#include "test_utils.h"
#include "utils_file.h"

namespace shcore {
  namespace shell_core_tests {
    class Environment
    {
    public:
      Environment()
      {
        shell_core.reset(new Shell_core(&output_handler.deleg));
        session.reset(new mysh::Session(shell_core.get()));
        shell_core->set_global("session", Value(boost::static_pointer_cast<Object_bridge>(session)));
        bool initialized = false;
        shell_core->switch_mode(Shell_core::Mode_SQL, initialized);
      }

      ~Environment()
      {
      }

      boost::shared_ptr<Shell_core> shell_core;
      boost::shared_ptr<mysh::Session> session;
      Shell_test_output_handler output_handler;
    };

    class Shell_core_test : public ::testing::Test
    {
    protected:
      Environment _env;
      std::string _file_name;
      int _ret_val;

      void connect()
      {
        const char *uri = getenv("MYSQL_URI");
        const char *pwd = getenv("MYSQL_PWD");
        const char *port = getenv("MYSQL_PORT");

        std::string x_uri(uri);
        if (port)
        {
          x_uri.append(":");
          x_uri.append(port);
        }

        Argument_list args;
        args.push_back(Value(x_uri));
        if (pwd)
          args.push_back(Value(pwd));

        _env.session->connect(args);
      }

      void process(const std::string& path, bool continue_on_error)
      {
        _env.output_handler.wipe_out();
        _env.output_handler.wipe_err();

        _file_name = get_binary_folder() + "/" + path;

        std::ifstream stream(_file_name.c_str());
        if (stream.fail())
          FAIL();

        _ret_val = _env.shell_core->process_stream(stream, _file_name, continue_on_error);
      }
    };

    TEST_F(Shell_core_test, test_process_stream)
    {
      connect();

      // Successfully processed file
      process("sql/sql_ok.sql", false);
      EXPECT_EQ(0, _ret_val);
      EXPECT_NE(-1, _env.output_handler.std_out.find("first_result"));

      // Failed without the force option
      process("sql/sql_err.sql", false);
      EXPECT_EQ(1, _ret_val);
      EXPECT_NE(-1, _env.output_handler.std_out.find("first_result"));
      EXPECT_NE(-1, _env.output_handler.std_err.find("Table 'unexisting.whatever' doesn't exist"));
      EXPECT_EQ(-1, _env.output_handler.std_out.find("second_result"));

      // Failed without the force option
      process("sql/sql_err.sql", true);
      EXPECT_EQ(1, _ret_val);
      EXPECT_NE(-1, _env.output_handler.std_out.find("first_result"));
      EXPECT_NE(-1, _env.output_handler.std_err.find("Table 'unexisting.whatever' doesn't exist"));
      EXPECT_NE(-1, _env.output_handler.std_out.find("second_result"));

      // JS tests: outputs are not validated since in batch mode there's no autoprinting of resultsets
      // Error is also directed to the std::cerr directly
      bool initialized = false;
      _env.shell_core->switch_mode(Shell_core::Mode_JScript, initialized);
      process("js/js_ok.js", true);
      EXPECT_EQ(0, _ret_val);

      process("js/js_err.js", true);
      EXPECT_NE(-1, _env.output_handler.std_err.find("Table 'unexisting.whatever' doesn't exist"));
    }
  }
}