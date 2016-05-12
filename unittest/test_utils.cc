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

#include "test_utils.h"
#include <boost/algorithm/string.hpp>
#include <boost/bind.hpp>
#include "shellcore/shell_core_options.h"
#include "src/shell_resultset_dumper.h"
#include "src/interactive_shell.h"
#include "utils/utils_general.h"

using namespace shcore;

Shell_test_output_handler::Shell_test_output_handler()
{
  deleg.user_data = this;
  deleg.print = &Shell_test_output_handler::deleg_print;
  deleg.print_error = &Shell_test_output_handler::deleg_print_error;
  deleg.prompt = &Shell_test_output_handler::deleg_prompt;
  deleg.password = &Shell_test_output_handler::deleg_password;
}

void Shell_test_output_handler::deleg_print(void *user_data, const char *text)
{
  Shell_test_output_handler* target = (Shell_test_output_handler*)(user_data);
  target->std_out.append(text);
}

void Shell_test_output_handler::deleg_print_error(void *user_data, const char *text)
{
  Shell_test_output_handler* target = (Shell_test_output_handler*)(user_data);
  target->std_err.append(text);
}

bool Shell_test_output_handler::deleg_prompt(void *user_data, const char *UNUSED(prompt), std::string &ret)
{
  Shell_test_output_handler* target = (Shell_test_output_handler*)(user_data);
  std::string prompt;

  bool ret_val = false;
  if (!target->prompts.empty())
  {
    prompt = target->prompts.front();
    target->prompts.pop_front();

    ret_val = true;
  }

  ret = prompt;
  return ret_val;
}

bool Shell_test_output_handler::deleg_password(void *user_data, const char *UNUSED(prompt), std::string &ret)
{
  Shell_test_output_handler* target = (Shell_test_output_handler*)(user_data);
  ret = target->ret_pwd;
  return true;
}

void Shell_test_output_handler::validate_stdout_content(const std::string& content, bool expected)
{
  bool found = std_out.find(content) != std::string::npos;

  if (found != expected)
  {
    std::string error = expected ? "Missing" : "Unexpected";
    error += " Output: " + content;
    SCOPED_TRACE("STDOUT Actual: " + std_out);
    SCOPED_TRACE(error);
    ADD_FAILURE();
  }
}

void Shell_test_output_handler::validate_stderr_content(const std::string& content, bool expected)
{
  bool found = std_err.find(content) != std::string::npos;

  if (found != expected)
  {
    std::string error = expected ? "Missing" : "Unexpected";
    error += " Output: " + content;
    SCOPED_TRACE("STDOUT Actual: " + std_out);
    SCOPED_TRACE(error);
    ADD_FAILURE();
  }
}

void Shell_core_test_wrapper::SetUp()
{
  //_shell_core.reset(new shcore::Shell_core(&output_handler.deleg));
  char **argv = NULL;
  _options.reset(new Shell_command_line_options(0, argv));

  _interactive_shell.reset(new Interactive_shell(*_options.get(), &output_handler.deleg));

  const char *uri = getenv("MYSQL_URI");
  if (uri)
  {
    // Creates connection data and recreates URI, this will fix URI if no password is defined
    // So the UT don't prompt for password ever
    shcore::Value::Map_type_ref data = shcore::get_connection_data(uri);

    const char *pwd = getenv("MYSQL_PWD");
    if (pwd)
    {
      _pwd.assign(pwd);
      (*data)["dbPassword"] = shcore::Value(_pwd);
    }

    _uri = shcore::build_connection_string(data, true);
    _uri_nopasswd = shcore::strip_password(_uri);

    _mysql_uri = _uri;
  }

  const char *port = getenv("MYSQL_PORT");
  if (port)
  {
    _mysql_port.assign(port);
    _mysql_uri += ":" + _mysql_port;
  }

  _mysql_uri_nopasswd = shcore::strip_password(_mysql_uri);
}

void Shell_core_test_wrapper::TearDown()
{
  _interactive_shell.reset();
}

shcore::Value Shell_core_test_wrapper::execute(const std::string& code)
{
  std::string _code(code);

  _interactive_shell->process_line(_code);

  return _returned_value;
}

shcore::Value Shell_core_test_wrapper::exec_and_out_equals(const std::string& code, const std::string& out, const std::string& err)
{
  std::string expected_output(out);
  std::string expected_error(err);

  if (_interactive_shell->interactive_mode() == shcore::Shell_core::Mode_Python && out.length())
    expected_output += "\n";

  if (_interactive_shell->interactive_mode() == shcore::Shell_core::Mode_Python && err.length())
    expected_error += "\n";

  shcore::Value ret_val = execute(code);

  boost::trim(output_handler.std_out);
  boost::trim(output_handler.std_err);

  EXPECT_EQ(expected_output, output_handler.std_out);
  EXPECT_EQ(expected_error, output_handler.std_err);

  output_handler.wipe_all();

  return ret_val;
}

shcore::Value Shell_core_test_wrapper::exec_and_out_contains(const std::string& code, const std::string& out, const std::string& err)
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

void Crud_test_wrapper::set_functions(const std::string &functions)
{
  boost::algorithm::split(_functions, functions, boost::is_any_of(", "), boost::token_compress_on);
}

// Validates only the specified functions are available
// non listed functions are validated for unavailability
void Crud_test_wrapper::ensure_available_functions(const std::string& functions)
{
  bool is_js = _interactive_shell->interactive_mode() == shcore::Shell_core::Mode_JScript;
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