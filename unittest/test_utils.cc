/* Copyright (c) 2015, 2017, Oracle and/or its affiliates. All rights reserved.

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
#include "shellcore/shell_core_options.h"
#include "shell/shell_resultset_dumper.h"
#include "utils/utils_general.h"
#include "utils/utils_file.h"
#include "modules/base_session.h"

using namespace shcore;

static bool g_test_sessions = getenv("TEST_SESSIONS") != nullptr;
static bool g_test_debug = getenv("TEST_DEBUG") != nullptr;

Shell_test_output_handler::Shell_test_output_handler() {
  deleg.user_data = this;
  deleg.print = &Shell_test_output_handler::deleg_print;
  deleg.print_error = &Shell_test_output_handler::deleg_print_error;
  deleg.prompt = &Shell_test_output_handler::deleg_prompt;
  deleg.password = &Shell_test_output_handler::deleg_password;

  full_output.clear();
}

void Shell_test_output_handler::deleg_print(void *user_data, const char *text) {
  Shell_test_output_handler* target = (Shell_test_output_handler*)(user_data);

  target->full_output << text << std::endl;

  if (g_test_debug)
    std::cout << text << std::endl;

  target->std_out.append(text);
}

void Shell_test_output_handler::deleg_print_error(void *user_data, const char *text) {
  Shell_test_output_handler* target = (Shell_test_output_handler*)(user_data);

  target->full_output << text << std::endl;

  if (g_test_debug)
    std::cerr << text << std::endl;

  target->std_err.append(text);
}

bool Shell_test_output_handler::deleg_prompt(void *user_data, const char *prompt, std::string &ret) {
  Shell_test_output_handler* target = (Shell_test_output_handler*)(user_data);
  std::string answer;

  target->full_output << prompt;
  target->std_out.append(prompt);

  bool ret_val = false;
  if (!target->prompts.empty()) {
    answer = target->prompts.front();
    target->prompts.pop_front();

    ret_val = true;
  }

  ret = answer;
  return ret_val;
}

bool Shell_test_output_handler::deleg_password(void *user_data, const char *prompt, std::string &ret) {
  Shell_test_output_handler* target = (Shell_test_output_handler*)(user_data);
  std::string answer;

  target->full_output << prompt;
  target->std_out.append(prompt);

  bool ret_val = false;
  if (!target->passwords.empty()) {
    answer = target->passwords.front();
    target->passwords.pop_front();

    ret_val = true;
  }

  ret = answer;
  return ret_val;
}

void Shell_test_output_handler::validate_stdout_content(const std::string& content, bool expected) {
  bool found = std_out.find(content) != std::string::npos;

  if (found != expected) {
    std::string error = expected ? "Missing" : "Unexpected";
    error += " Output: " + content;
    SCOPED_TRACE("STDOUT Actual: " + std_out);
    SCOPED_TRACE(error);
    ADD_FAILURE();
  }
}

void Shell_test_output_handler::validate_stderr_content(const std::string& content, bool expected) {
  if (content.empty()) {
    if (std_err.empty() != expected) {
      std::string error = std_err.empty() ? "Missing" : "Unexpected";
      error += " Error: " + content;
      SCOPED_TRACE("STDERR Actual: " + std_err);
      SCOPED_TRACE(error);
      ADD_FAILURE();
    }
  } else {
    bool found = std_err.find(content) != std::string::npos;

    if (found != expected) {
      std::string error = expected ? "Missing" : "Unexpected";
      error += " Error: " + content;
      SCOPED_TRACE("STDERR Actual: " + std_err);
      SCOPED_TRACE(error);
      ADD_FAILURE();
    }
  }
}

void Shell_test_output_handler::debug_print(const std::string& line) {
  full_output << line.c_str() << std::endl;
}

void Shell_test_output_handler::debug_print_header(const std::string& line) {
  std::string splitter(line.length(), '-');

  full_output << splitter.c_str() << std::endl;
  full_output << line.c_str() << std::endl;
  full_output << splitter.c_str() << std::endl;
}

void Shell_test_output_handler::flush_debug_log() {
  full_output.flush();
  std::cerr << full_output.str();;
  full_output.str(std::string());
  full_output.clear();
}


std::string Shell_core_test_wrapper::context_identifier() {
  std::string ret_val;

  auto test_info = info();

  if (test_info) {
    ret_val.append(test_info->test_case_name());
    ret_val.append(".");
    ret_val.append(test_info->name());
  }

  if (!_custom_context.empty())
    ret_val.append(": " + _custom_context);

  return ret_val;
}

void Shell_core_test_wrapper::SetUp() {
  output_handler.debug_print_header(context_identifier());

  // Initializes the options member
  reset_options();

  // Allows derived classes configuring specific options
  set_options();

  const char *uri = getenv("MYSQL_URI");
  if (uri) {
    // Creates connection data and recreates URI, this will fix URI if no password is defined
    // So the UT don't prompt for password ever
    shcore::Value::Map_type_ref data = shcore::get_connection_data(uri);

    _host = data->get_string("host");

    const char *pwd = getenv("MYSQL_PWD");
    if (pwd) {
      _pwd.assign(pwd);
      (*data)["dbPassword"] = shcore::Value(_pwd);
    }

    _uri = shcore::build_connection_string(data, true);
    _mysql_uri = _uri;

    const char *xport = getenv("MYSQLX_PORT");
    if (xport) {
      _port.assign(xport);
      (*data)["port"] = shcore::Value(_pwd);
      _uri += ":" + _port;
    }
    _uri_nopasswd = shcore::strip_password(_uri);

    const char *port = getenv("MYSQL_PORT");
    if (port) {
      _mysql_port.assign(port);
      _mysql_uri += ":" + _mysql_port;
    }

    const char *sandbox_port1 = getenv("MYSQL_SANDBOX_PORT1");
    if (sandbox_port1) {
      _mysql_sandbox_port1.assign(sandbox_port1);
    } else {
      std::string sandbox_port1 = std::to_string(atoi(_mysql_port.c_str()) + 10);
      _mysql_sandbox_port1.assign(sandbox_port1);
    }

    const char *sandbox_port2 = getenv("MYSQL_SANDBOX_PORT2");
    if (sandbox_port2) {
      _mysql_sandbox_port2.assign(sandbox_port2);
    } else {
      std::string sandbox_port2 = std::to_string(atoi(_mysql_port.c_str()) + 20);
      _mysql_sandbox_port2.assign(sandbox_port2);
    }

    const char *sandbox_port3 = getenv("MYSQL_SANDBOX_PORT3");
    if (sandbox_port3) {
      _mysql_sandbox_port3.assign(sandbox_port3);
    } else {
      std::string sandbox_port3 = std::to_string(atoi(_mysql_port.c_str()) + 30);
      _mysql_sandbox_port3.assign(sandbox_port3);
    }

    _mysql_uri_nopasswd = shcore::strip_password(_mysql_uri);
  }

  const char *tmpdir = getenv("TMPDIR");
  if (tmpdir) {
    _sandbox_dir.assign(tmpdir);

#ifdef WIN32
    auto tokens = shcore::split_string(_sandbox_dir, "\\");
    _sandbox_dir = shcore::join_strings(tokens, "\\\\");
#endif
  } else {
    // If not specified, the tests will create the sandboxes on the binary folder
    _sandbox_dir = shcore::get_binary_folder();
  }


#ifdef _WIN32
  std::string _path_splitter = "\\";
#else
  std::string _path_splitter = "/";
#endif

  std::vector<std::string> path_components = {_sandbox_dir, _mysql_sandbox_port1, "my.cnf"};
  _sandbox_cnf_1 = shcore::join_strings(path_components, _path_splitter);
  path_components[1] = _mysql_sandbox_port2;
  _sandbox_cnf_2 = shcore::join_strings(path_components, _path_splitter);
  path_components[1] = _mysql_sandbox_port3;
  _sandbox_cnf_3 = shcore::join_strings(path_components, _path_splitter);

  // Initializes the interactive shell
  reset_shell();

  observe_notification("SN_SESSION_CONNECTED");
  observe_notification("SN_SESSION_CONNECTION_LOST");
  observe_notification("SN_SESSION_CLOSED");
}

void Shell_core_test_wrapper::TearDown() {
  ignore_notification("SN_SESSION_CONNECTED");
  ignore_notification("SN_SESSION_CONNECTION_LOST");
  ignore_notification("SN_SESSION_CLOSED");

  if (!_open_sessions.empty()) {
    for (auto entry : _open_sessions) {
      // Prints the session warnings ONLY if TEST_SESSIONS is enabled
      if (g_test_sessions)
        std::cerr << "WARNING: Closing dangling session opened on " << entry.second << std::endl;

      auto session = std::dynamic_pointer_cast<mysqlsh::ShellBaseSession>(entry.first);
      session->close(shcore::Argument_list());
    }
  }

  _interactive_shell.reset();
}

void Shell_core_test_wrapper::handle_notification(const std::string &name, const shcore::Object_bridge_ref& sender, shcore::Value::Map_type_ref data) {
  std::string identifier = context_identifier();

  if (name == "SN_SESSION_CONNECTED") {
    auto position = _open_sessions.find(sender);
    if (position == _open_sessions.end())
      _open_sessions[sender] = identifier;
    // Prints the session warnings ONLY if TEST_SESSIONS is enabled
    else if (g_test_sessions) {
      std::cerr << "WARNING: Reopening session from " << _open_sessions[sender] << " at " << identifier << std::endl;
    }
  } else if (name == "SN_SESSION_CONNECTION_LOST" || name == "SN_SESSION_CLOSED") {
    auto position = _open_sessions.find(sender);
    if (position != _open_sessions.end())
      _open_sessions.erase(position);
    // Prints the session warnings ONLY if TEST_SESSIONS is enabled
    else if (g_test_sessions) {
      std::cerr << "WARNING: Closing a session that was never opened at " << identifier << std::endl;
    }
  }
}

shcore::Value Shell_core_test_wrapper::execute(const std::string& code) {
  std::string _code(code);

  if (g_test_debug)
    std::cout << "---> " << code << std::endl;

  _interactive_shell->process_line(_code);

  return _returned_value;
}

shcore::Value Shell_core_test_wrapper::exec_and_out_equals(const std::string& code, const std::string& out, const std::string& err) {
  std::string expected_output(out);
  std::string expected_error(err);

  if (_interactive_shell->interactive_mode() == shcore::Shell_core::Mode::Python && out.length())
    expected_output += "\n";

  if (_interactive_shell->interactive_mode() == shcore::Shell_core::Mode::Python && err.length())
    expected_error += "\n";

  shcore::Value ret_val = execute(code);

  boost::trim(output_handler.std_out);
  boost::trim(output_handler.std_err);

  if (expected_output != "*")
    EXPECT_EQ(expected_output, output_handler.std_out);

  if (expected_error != "*")
    EXPECT_EQ(expected_error, output_handler.std_err);

  output_handler.wipe_all();

  return ret_val;
}

shcore::Value Shell_core_test_wrapper::exec_and_out_contains(const std::string& code, const std::string& out, const std::string& err) {
  shcore::Value ret_val = execute(code);

  if (out.length()) {
    SCOPED_TRACE("STDOUT missing: " + out);
    SCOPED_TRACE("STDOUT actual: " + output_handler.std_out);
    EXPECT_NE(-1, int(output_handler.std_out.find(out)));
  }

  if (err.length()) {
    SCOPED_TRACE("STDERR missing: " + err);
    SCOPED_TRACE("STDERR actual: " + output_handler.std_err);
    EXPECT_NE(-1, int(output_handler.std_err.find(err)));
  }

  output_handler.wipe_all();

  return ret_val;
}

void Crud_test_wrapper::set_functions(const std::string &functions) {
  boost::algorithm::split(_functions, functions, boost::is_any_of(", "), boost::token_compress_on);
}

// Validates only the specified functions are available
// non listed functions are validated for unavailability
void Crud_test_wrapper::ensure_available_functions(const std::string& functions) {
  bool is_js = _interactive_shell->interactive_mode() == shcore::Shell_core::Mode::JScript;
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
  for (index = _functions.begin(); index != end; index++) {
    // If the function is suppossed to be valid it needs to be available on the
    // crud dir
    if (valid_functions.find(*index) != valid_functions.end()) {
      SCOPED_TRACE("Function " + *index + " should be available and is not.");
      if (is_js)
        exec_and_out_equals("print(real_functions.indexOf('" + *index + "') != -1)", "true");
      else
        exec_and_out_equals("index=real_functions.index('" + *index + "')");
    }

    // If not, should not be on the crud dir and calling it should be illegal
    else {
      SCOPED_TRACE("Function " + *index + " should NOT be available.");
      if (is_js)
        exec_and_out_equals("print(real_functions.indexOf('" + *index + "') == -1)", "true");
      else
        exec_and_out_contains("print(real_functions.index('" + *index + "'))", "", "is not in list");

      exec_and_out_contains("crud." + *index + "('');", "", "Forbidden usage of " + *index);
    }
  }
}
