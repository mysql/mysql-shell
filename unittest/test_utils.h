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
#include "shellcore/common.h"
#include <boost/algorithm/string.hpp>
#include <boost/bind.hpp>

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

  static bool password(void *user_data, const char *UNUSED(prompt), std::string &ret)
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
    {
      _uri.assign(uri);
      _mysql_uri = "mysql://";
      _mysql_uri.append(uri);
    }

    const char *pwd = getenv("MYSQL_PWD");
    if (pwd)
      _pwd.assign(pwd);

    const char *port = getenv("MYSQL_PORT");
    if (port)
    {
      _mysql_port.assign(port);
      _mysql_uri += ":" + _mysql_port;
    }
  }

  virtual void TearDown()
  {
    _shell_core.reset();
  }

  void process_result(shcore::Value result)
  {
    _returned_value = result;
  }

  shcore::Value execute(const std::string& code)
  {
    std::string _code(code);
    shcore::Interactive_input_state state;

    _shell_core->handle_input(_code, state, boost::bind(&Shell_core_test_wrapper::process_result, this, _1));

    return _returned_value;
  }

  shcore::Value exec_and_out_equals(const std::string& code, const std::string& out = "", const std::string& err = "")
  {
    std::string expected_output(out);
    std::string expected_error(err);

    if (_shell_core->interactive_mode() == shcore::Shell_core::Mode_Python && out.length())
      expected_output += "\n";

    if (_shell_core->interactive_mode() == shcore::Shell_core::Mode_Python && err.length())
      expected_error += "\n";

    shcore::Value ret_val = execute(code);
    EXPECT_EQ(expected_output, output_handler.std_out);
    EXPECT_EQ(expected_error, output_handler.std_err);

    output_handler.wipe_all();

    return ret_val;
  }

  shcore::Value exec_and_out_contains(const std::string& code, const std::string& out = "", const std::string& err = "")
  {
    shcore::Value ret_val = execute(code);

    if (out.length())
    {
      SCOPED_TRACE("STDOUT missing: " + out);
      SCOPED_TRACE("STDOUT actual: " + output_handler.std_out);
      EXPECT_NE(-1, int(output_handler.std_out.find(out)));
    }

    if (err.length())
    {
      SCOPED_TRACE("STDERR missing: " + err);
      SCOPED_TRACE("STDERR actual: " + output_handler.std_err);
      EXPECT_NE(-1, int(output_handler.std_err.find(err)));
    }

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
  std::string _mysql_uri;

  shcore::Value _returned_value;

  shcore::Interpreter_delegate deleg;
};

// Helper class to ease the creation of tests on the CRUD operations
// Specially on the chained methods
class Crud_test_wrapper : public ::Shell_core_test_wrapper
{
protected:
  std::set<std::string> _functions;

  // Sets the functions that will be available for chaining
  // in a CRUD operation
  void set_functions(const std::string &functions)
  {
    boost::algorithm::split(_functions, functions, boost::is_any_of(", "), boost::token_compress_on);
  }

  // Validates only the specified functions are available
  // non listed functions are validated for unavailability
  void ensure_available_functions(const std::string& functions)
  {
    bool is_js = _shell_core->interactive_mode() == shcore::Shell_core::Mode_JScript;
    std::set<std::string> valid_functions;
    boost::algorithm::split(valid_functions, functions, boost::is_any_of(", "), boost::token_compress_on);

    // Retrieves the active functions on the crud operation
    if (is_js)
      exec_and_out_equals("var real_functions = dir(crud)");
    else
      exec_and_out_equals("real_functions = crud.__members__");

    // Ensures the number of available functions is the expected
    std::stringstream ss;
    ss << valid_functions.size();

    {
      SCOPED_TRACE("Unexpected number of available functions.");
      if (is_js)
        exec_and_out_equals("print(real_functions.length)", ss.str());
      else
        exec_and_out_equals("print(len(real_functions))", ss.str());
    }

    std::set<std::string>::iterator index, end = _functions.end();
    for (index = _functions.begin(); index != end; index++)
    {
      // If the function is suppossed to be valid it needs to be available on the
      // crud dir
      if (valid_functions.find(*index) != valid_functions.end())
      {
        SCOPED_TRACE("Function " + *index + " should be available and is not.");
        if (is_js)
          exec_and_out_equals("print(real_functions.indexOf('" + *index + "') != -1)", "true");
        else
          exec_and_out_equals("index=real_functions.index('" + *index + "')");
      }

      // If not, should not be on the crud dir and calling it should be illegal
      else
      {
        SCOPED_TRACE("Function " + *index + " should NOT be available.");
        if (is_js)
          exec_and_out_equals("print(real_functions.indexOf('" + *index + "') == -1)", "true");
        else
          exec_and_out_contains("print(real_functions.index('" + *index + "'))", "", "is not in list");

        exec_and_out_contains("crud." + *index + "('');", "", "Forbidden usage of " + *index);
      }
    }
  }
};
