/* Copyright (c) 2015, Oracle and/or its affiliates. All rights reserved.

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

#include "shellcore/lang_base.h"
#include "shellcore/shell_core.h"

class Shell_test_output_handler
{
public:
  // You can define per-test set-up and tear-down logic as usual.
  Shell_test_output_handler()
  {
    deleg.user_data = this;
    deleg.print = &Shell_test_output_handler::deleg_print;
    deleg.print_error = &Shell_test_output_handler::deleg_print_error;
  }

  virtual void TearDown()
  {
  }

  static void deleg_print(void *user_data, const char *text)
  {
    Shell_test_output_handler* target = (Shell_test_output_handler*)(user_data);
    target->std_out.append(text);
  }

  static void deleg_print_error(void *user_data, const char *text)
  {
    Shell_test_output_handler* target = (Shell_test_output_handler*)(user_data);
    target->std_err.append(text);
  }

  static bool password(void *user_data, const char *prompt, std::string &ret)
  {
    Shell_test_output_handler* target = (Shell_test_output_handler*)(user_data);
    ret = target->ret_pwd;
    return true;
  }

  void wipe_out() { std_out.clear(); }
  void wipe_err() { std_err.clear(); }
  void wipe_all() { std_out.clear(); std_err.clear(); }

  shcore::Interpreter_delegate deleg;
  std::string std_err;
  std::string std_out;
  std::string ret_pwd;
};

class Shell_core_test_wrapper : public ::testing::Test
{
protected:
  // You can define per-test set-up and tear-down logic as usual.
  virtual void SetUp()
  {
    _shell_core.reset(new shcore::Shell_core(&output_handler.deleg));

    const char *uri = getenv("MYSQL_URI");
    if (uri)
      _uri.assign(uri);

    const char *pwd = getenv("MYSQL_PWD");
    if (pwd)
      _pwd.assign(pwd);

    const char *port = getenv("MYSQL_PORT");
    if (port)
      _mysql_port.assign(port);
  }

  shcore::Value execute(const std::string& code)
  {
    std::string _code(code);
    shcore::Interactive_input_state state;
    return _shell_core->handle_input(_code, state, true);
  }

  shcore::Value exec_and_out_equals(const std::string& code, const std::string& out = "", const std::string& err = "")
  {
    shcore::Value ret_val = execute(code);
    EXPECT_EQ(out, output_handler.std_out);
    EXPECT_EQ(err, output_handler.std_err);

    output_handler.wipe_all();

    return ret_val;
  }

  shcore::Value exec_and_out_contains(const std::string& code, const std::string& out = "", const std::string& err = "")
  {
    shcore::Value ret_val = execute(code);

    if (out.length())
      EXPECT_NE(-1, int(output_handler.std_out.find(out)));

    if (err.length())
      EXPECT_NE(-1, int(output_handler.std_err.find(err)));

    output_handler.wipe_all();

    return ret_val;
  }

  Shell_test_output_handler output_handler;
  boost::shared_ptr<shcore::Shell_core> _shell_core;
  void wipe_out() { output_handler.wipe_out(); }
  void wipe_err() { output_handler.wipe_err(); }
  void wipe_all() { output_handler.wipe_all(); }

  std::string _uri;
  std::string _pwd;
  std::string _mysql_port;

  shcore::Interpreter_delegate deleg;
};
