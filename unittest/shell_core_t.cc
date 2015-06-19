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
#include "../modules/base_session.h"
#include "test_utils.h"
#include "utils_file.h"

namespace shcore {
  namespace shell_core_tests {
    class Shell_core_test : public Shell_core_test_wrapper
    {
    protected:
      std::string _file_name;
      int _ret_val;

      virtual void SetUp()
      {
        Shell_core_test_wrapper::SetUp();

        bool initilaized(false);
        _shell_core->switch_mode(Shell_core::Mode_SQL, initilaized);
      }

      void connect()
      {
        std::string uri = _uri;
        std::cout << uri;
        if (!_mysql_port.empty())
        {
          uri.append(":");
          uri.append(_mysql_port);
        }

        Argument_list args;
        args.push_back(Value(uri));
        if (!_pwd.empty())
          args.push_back(Value(_pwd));

        boost::shared_ptr<mysh::BaseSession> session(mysh::connect_session(args));

        _shell_core->set_global("session", Value(boost::static_pointer_cast<Object_bridge>(session)));
      }

      void process(const std::string& path, bool continue_on_error)
      {
        wipe_all();

        _file_name = get_binary_folder() + "/" + path;

        std::ifstream stream(_file_name.c_str());
        if (stream.fail())
          FAIL();

        _ret_val = _shell_core->process_stream(stream, _file_name, continue_on_error);
      }
    };

    TEST_F(Shell_core_test, test_process_stream)
    {
      connect();

      // Successfully processed file
      process("sql/sql_ok.sql", false);
      EXPECT_EQ(0, _ret_val);
      EXPECT_NE(-1, output_handler.std_out.find("first_result"));

      // Failed without the force option
      process("sql/sql_err.sql", false);
      EXPECT_EQ(1, _ret_val);
      EXPECT_NE(-1, output_handler.std_out.find("first_result"));
      EXPECT_NE(-1, output_handler.std_err.find("Table 'unexisting.whatever' doesn't exist"));
      EXPECT_EQ(-1, output_handler.std_out.find("second_result"));

      // Failed without the force option
      process("sql/sql_err.sql", true);
      EXPECT_EQ(1, _ret_val);
      EXPECT_NE(-1, output_handler.std_out.find("first_result"));
      EXPECT_NE(-1, output_handler.std_err.find("Table 'unexisting.whatever' doesn't exist"));
      EXPECT_NE(-1, output_handler.std_out.find("second_result"));

      // JS tests: outputs are not validated since in batch mode there's no autoprinting of resultsets
      // Error is also directed to the std::cerr directly
      bool initialized = false;
      _shell_core->switch_mode(Shell_core::Mode_JScript, initialized);
      process("js/js_ok.js", true);
      EXPECT_EQ(0, _ret_val);

      process("js/js_err.js", true);
      EXPECT_NE(-1, output_handler.std_err.find("Table 'unexisting.whatever' doesn't exist"));
    }
  }
}