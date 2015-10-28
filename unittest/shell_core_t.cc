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
#include "../modules/base_resultset.h"
#include "../src/shell_resultset_dumper.h"
#include "test_utils.h"
#include "utils/utils_file.h"
#include <boost/bind.hpp>

namespace shcore {
  namespace shell_core_tests {
    class Shell_core_test : public Shell_core_test_wrapper
    {
    protected:
      std::string _file_name;
      int _ret_val;
      boost::shared_ptr<mysh::ShellBaseSession> _session;

      boost::function<void(shcore::Value)> _result_processor;

      virtual void SetUp()
      {
        Shell_core_test_wrapper::SetUp();

        bool initilaized(false);
        _shell_core->switch_mode(Shell_core::Mode_SQL, initilaized);

        _result_processor = boost::bind(&Shell_core_test::process_result, this, _1);
      }

      void connect()
      {
        //std::string uri = _uri;
        std::cout << _mysql_uri;
        Argument_list args;
        args.push_back(Value(_mysql_uri));
        if (!_pwd.empty())
          args.push_back(Value(_pwd));

        _session = mysh::connect_session(args, mysh::Classic);

        _shell_core->set_global("session", Value(boost::static_pointer_cast<Object_bridge>(_session)));
      }

      // NOTE: this method is pretty much the same used on the shell application
      //       but since testing is done using the shell core class we need to mimic
      //       here in order to validate the outputs properly
      void process_result(shcore::Value result)
      {
        if ((*Shell_core_options::get())[SHCORE_INTERACTIVE].as_bool())
        {
          if (result)
          {
            Value shell_hook;
            boost::shared_ptr<Object_bridge> object;
            if (result.type == shcore::Object)
            {
              object = result.as_object();
              if (object && object->has_member("__shell_hook__"))
              shell_hook = object->get_member("__shell_hook__");

              if (shell_hook)
              {
                Argument_list args;
                Value hook_result = object->call("__shell_hook__", args);

                // Recursive call to continue processing shell hooks if any
                process_result(hook_result);
              }
            }

            // If the function is not found the values still needs to be printed
            if (!shell_hook)
            {
              // Resultset objects get printed
              if (object && object->class_name().find("Result") != -1)
              {
                boost::shared_ptr<mysh::ShellBaseResult> resultset = boost::static_pointer_cast<mysh::ShellBaseResult> (object);
                ResultsetDumper dumper(resultset);
                dumper.dump();
              }
              else
              {
                if ((*Shell_core_options::get())[SHCORE_OUTPUT_FORMAT].as_string().find("json") == 0)
                {
                  shcore::JSON_dumper dumper((*Shell_core_options::get())[SHCORE_OUTPUT_FORMAT].as_string() == "json");
                  dumper.start_object();
                  dumper.append_value("result", result);
                  dumper.end_object();

                  print(dumper.str());
                }
                else
                print(result.descr(true).c_str());
              }
            }
          }
        }

        // Return value of undefined implies an error processing
        if (result.type == shcore::Undefined)
          _shell_core->set_error_processing();
      }

      void process(const std::string& path)
      {
        wipe_all();

        _file_name = get_binary_folder() + "/" + path;

        std::ifstream stream(_file_name.c_str());
        if (stream.fail())
          FAIL();

        _ret_val = _shell_core->process_stream(stream, _file_name, _result_processor);
      }
    };

    TEST_F(Shell_core_test, test_process_stream)
    {
      connect();

      // Successfully processed file
      (*Shell_core_options::get())[SHCORE_BATCH_CONTINUE_ON_ERROR] = Value::False();
      process("sql/sql_ok.sql");
      EXPECT_EQ(0, _ret_val);
      EXPECT_NE(-1, static_cast<int>(output_handler.std_out.find("first_result")));

      // Failed without the force option
      process("sql/sql_err.sql");
      EXPECT_EQ(1, _ret_val);
      EXPECT_NE(-1, static_cast<int>(output_handler.std_out.find("first_result")));
      EXPECT_NE(-1, static_cast<int>(output_handler.std_err.find("Table 'unexisting.whatever' doesn't exist")));
      EXPECT_EQ(-1, static_cast<int>(output_handler.std_out.find("second_result")));

      // Failed without the force option
      (*Shell_core_options::get())[SHCORE_BATCH_CONTINUE_ON_ERROR] = Value::True();
      process("sql/sql_err.sql");
      EXPECT_EQ(1, _ret_val);
      EXPECT_NE(-1, static_cast<int>(output_handler.std_out.find("first_result")));
      EXPECT_NE(-1, static_cast<int>(output_handler.std_err.find("Table 'unexisting.whatever' doesn't exist")));
      EXPECT_NE(-1, static_cast<int>(output_handler.std_out.find("second_result")));

      // JS tests: outputs are not validated since in batch mode there's no autoprinting of resultsets
      // Error is also directed to the std::cerr directly
      bool initialized = false;
      _shell_core->switch_mode(Shell_core::Mode_JScript, initialized);
      process("js/js_ok.js");
      EXPECT_EQ(0, _ret_val);

      process("js/js_err.js");
      EXPECT_NE(-1, static_cast<int>(output_handler.std_err.find("Table 'unexisting.whatever' doesn't exist")));

      // Closes the connection
      _session->call("close", shcore::Argument_list());
    }
  }
}