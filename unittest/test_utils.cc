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
#include <memory>
#include <random>
#include <string>
#include "db/uri_encoder.h"
#include "db/replay/setup.h"
#include "shellcore/base_session.h"
#include "shellcore/shell_resultset_dumper.h"
#include "utils/utils_file.h"
#include "utils/utils_general.h"
#include "utils/utils_string.h"

using namespace shcore;

static bool g_test_sessions = getenv("TEST_SESSIONS") != nullptr;
static bool g_test_debug = getenv("TEST_DEBUG") != nullptr;

std::vector<std::string> Shell_test_output_handler::log;
ngcommon::Logger *Shell_test_output_handler::_logger;

extern std::string g_mysql_version;
extern mysqlshdk::db::replay::Mode g_test_recording_mode;

Shell_test_output_handler::Shell_test_output_handler() {
  deleg.user_data = this;
  deleg.print = &Shell_test_output_handler::deleg_print;
  deleg.print_error = &Shell_test_output_handler::deleg_print_error;
  deleg.prompt = &Shell_test_output_handler::deleg_prompt;
  deleg.password = &Shell_test_output_handler::deleg_password;

  full_output.clear();
  debug = false;

  // Initialize the logger and attach the hook for error verification
  std::string log_path = shcore::get_binary_folder();
  log_path += "/mysqlsh.log";
  ngcommon::Logger::setup_instance(log_path.c_str(), g_test_debug);
  _logger = ngcommon::Logger::singleton();
  _logger->attach_log_hook(log_hook);
}

Shell_test_output_handler::~Shell_test_output_handler() {
  _logger->detach_log_hook(log_hook);
}

void Shell_test_output_handler::log_hook(const char *message,
                                         ngcommon::Logger::LOG_LEVEL level,
                                         const char *domain) {
  ngcommon::Logger::LOG_LEVEL current_level = _logger->get_log_level();

  // If the level of the log is different than
  // the one set, we don't want to store the message
  if (current_level == level) {
    std::string message_s(message);
    log.push_back(message_s);
  }
}

void Shell_test_output_handler::deleg_print(void *user_data, const char *text) {
  Shell_test_output_handler *target = (Shell_test_output_handler *)(user_data);

  target->full_output << text << std::endl;

  if (target->debug || g_test_debug || shcore::str_beginswith(text, "**"))
    std::cout << text << std::flush;

  std::lock_guard<std::mutex> lock(target->stdout_mutex);
  target->std_out.append(text);
}

void Shell_test_output_handler::deleg_print_error(void *user_data,
                                                  const char *text) {
  Shell_test_output_handler *target = (Shell_test_output_handler *)(user_data);

  target->full_output << makered(text) << std::endl;

  if (target->debug || g_test_debug)
    std::cerr << makered(text) << std::endl;

  target->std_err.append(text);
}

shcore::Prompt_result Shell_test_output_handler::deleg_prompt(
    void *user_data, const char *prompt, std::string *ret) {
  Shell_test_output_handler *target = (Shell_test_output_handler *)(user_data);
  std::string answer;
  std::string expected_prompt;

  target->full_output << prompt;
  {
    std::lock_guard<std::mutex> lock(target->stdout_mutex);
    target->std_out.append(prompt);
  }

  shcore::Prompt_result ret_val = shcore::Prompt_result::Cancel;
  if (!target->prompts.empty()) {
    std::tie(expected_prompt, answer) = target->prompts.front();
    target->prompts.pop_front();

    if (expected_prompt == "*" || expected_prompt.compare(prompt) == 0) {
      target->debug_print(makegreen(
          shcore::str_format("\n--> prompt %s %s", prompt, answer.c_str())));
      target->full_output << answer << std::endl;
    } else {
      ADD_FAILURE() << "Mismatched prompts. Expected: " << expected_prompt
                    << "\n"
                    << "actual: " << prompt;
      target->debug_print(
          makered(shcore::str_format("\n--> mismatched prompt '%s'", prompt)));
    }
    if (answer != "<<<CANCEL>>>")
      ret_val = shcore::Prompt_result::Ok;
  } else {
    ADD_FAILURE() << "Unexpected prompt for '" << prompt << "'";
    target->debug_print(
        makered(shcore::str_format("\n--> unexpected prompt '%s'", prompt)));
  }

  *ret = answer;
  return ret_val;
}

shcore::Prompt_result Shell_test_output_handler::deleg_password(
    void *user_data, const char *prompt, std::string *ret) {
  Shell_test_output_handler *target = (Shell_test_output_handler *)(user_data);
  std::string answer;
  std::string expected_prompt;

  target->full_output << prompt;
  {
    std::lock_guard<std::mutex> lock(target->stdout_mutex);
    target->std_out.append(prompt);
  }

  shcore::Prompt_result ret_val = shcore::Prompt_result::Cancel;
  if (!target->passwords.empty()) {
    std::tie(expected_prompt, answer) = target->passwords.front();
    target->passwords.pop_front();

    if (expected_prompt == "*" || expected_prompt.compare(prompt) == 0) {
      target->debug_print(makegreen(
          shcore::str_format("\n--> password %s %s", prompt, answer.c_str())));
      target->full_output << answer << std::endl;
    } else {
      ADD_FAILURE() << "Mismatched pwd prompts. Expected: " << expected_prompt
                    << "\n"
                    << "actual: " << prompt;
      target->debug_print(makered(
          shcore::str_format("\n--> mismatched pwd prompt '%s'", prompt)));
    }

    if (answer != "<<<CANCEL>>>")
      ret_val = shcore::Prompt_result::Ok;
  } else {
    ADD_FAILURE() << "Unexpected password prompt for '" << prompt << "'";
    target->debug_print(makered(
        shcore::str_format("\n--> unexpected pwd prompt '%s'", prompt)));
  }

  *ret = answer;
  return ret_val;
}

void Shell_test_output_handler::validate_stdout_content(
    const std::string &content, bool expected) {
  bool found = std_out.find(content) != std::string::npos;

  if (found != expected) {
    std::string error = expected ? "Missing" : "Unexpected";
    error += " Output: " + shcore::str_replace(content, "\n", "\n\t");
    ADD_FAILURE() << error << "\n"
                  << "STDOUT Actual: " +
                         shcore::str_replace(std_out, "\n", "\n\t")
                  << "\n"
                  << "STDERR Actual: " +
                         shcore::str_replace(std_err, "\n", "\n\t");
  }
}

void Shell_test_output_handler::validate_stderr_content(
    const std::string &content, bool expected) {
  if (content.empty()) {
    if (std_err.empty() != expected) {
      std::string error = std_err.empty() ? "Missing" : "Unexpected";
      error += " Error: " + shcore::str_replace(content, "\n", "\n\t");
      ADD_FAILURE() << error << "\n"
                    << "STDERR Actual: " +
                           shcore::str_replace(std_err, "\n", "\n\t")
                    << "\n"
                    << "STDOUT Actual: " +
                           shcore::str_replace(std_out, "\n", "\n\t");
    }
  } else {
    bool found = std_err.find(content) != std::string::npos;

    if (found != expected) {
      std::string error = expected ? "Missing" : "Unexpected";
      error += " Error: " + shcore::str_replace(content, "\n", "\n\t");
      ADD_FAILURE() << error << "\n"
                    << "STDERR Actual: " +
                           shcore::str_replace(std_err, "\n", "\n\t")
                    << "\n"
                    << "STDOUT Actual: " +
                           shcore::str_replace(std_out, "\n", "\n\t");
    }
  }
}

void Shell_test_output_handler::validate_log_content(
    const std::vector<std::string> &content, bool expected, bool clear) {
  for (auto &value : content) {
    bool found = false;

    if (std::find_if(log.begin(), log.end(), [&value](const std::string &str) {
          return str.find(value) != std::string::npos;
        }) != log.end()) {
      found = true;
    }

    if (found != expected) {
      std::string error = expected ? "Missing" : "Unexpected";
      error += " LOG: " + value;
      std::string s;
      for (const auto &piece : log)
        s += piece;

      ADD_FAILURE() << error << "\n"
                    << "LOG Actual: " + s;
    }
  }

  // Wipe the log here
  if (clear)
    wipe_log();
}

void Shell_test_output_handler::validate_log_content(const std::string &content,
                                                     bool expected,
                                                     bool clear) {
  bool found = false;

  if (std::find_if(log.begin(), log.end(), [&content](const std::string &str) {
        return str.find(content) != std::string::npos;
      }) != log.end()) {
    found = true;
  }

  if (found != expected) {
    std::string error = expected ? "Missing" : "Unexpected";
    error += " LOG: " + content;
    std::string s;
    for (const auto &piece : log)
      s += piece;

    ADD_FAILURE() << error << "\n"
                  << "LOG Actual: " + s;
  }

  // Wipe the log here
  if (clear)
    wipe_log();
}

void Shell_test_output_handler::debug_print(const std::string &line) {
  if (debug || g_test_debug)
    std::cout << line << std::endl;

  full_output << line.c_str() << std::endl;
}

void Shell_test_output_handler::debug_print_header(const std::string &line) {
  if (debug || g_test_debug)
    std::cerr << makebold(line) << std::endl;

  std::string splitter(line.length(), '-');

  full_output << splitter.c_str() << std::endl;
  full_output << line.c_str() << std::endl;
  full_output << splitter.c_str() << std::endl;
}

void Shell_test_output_handler::flush_debug_log() {
  full_output.flush();
  std::cerr << full_output.str();

  full_output.str(std::string());
  full_output.clear();
}

void Shell_core_test_wrapper::connect_classic() {
  execute("\\connect -mc " + _mysql_uri);
}

void Shell_core_test_wrapper::connect_x() {
  execute("\\connect -mx " + _uri);
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
  Shell_base_test::SetUp();

  output_handler.debug_print_header(context_identifier());

  // Initializes the options member
  reset_options();

  // Allows derived classes configuring specific options
  debug = false;
  set_options();
  output_handler.debug = debug;

  // Initializes the interactive shell
  reset_shell();

  observe_session_notifications();
}

void Shell_core_test_wrapper::TearDown() {
  if (testutil) {
    _interactive_shell->set_global_object("testutil", {});
    testutil.reset();
  }
  _interactive_shell.reset();

  ignore_session_notifications();

  try {
    if (!_open_sessions.empty()) {
      for (auto entry : _open_sessions) {
        // Prints the session warnings ONLY if TEST_SESSIONS is enabled
        if (g_test_sessions)
          std::cerr << "WARNING: Closing dangling session opened on "
                    << entry.second << std::endl;

        auto session =
            std::dynamic_pointer_cast<mysqlsh::ShellBaseSession>(entry.first);
        if (session) {
          session->close();
        }
      }
      _open_sessions.clear();
    }
  } catch (std::exception &e) {
    std::cerr << "Unhandled exception in TearDown().1: " << e.what() << "\n";
  }

  try {
    tests::Shell_base_test::TearDown();
  } catch (std::exception &e) {
    std::cerr << "Unhandled exception in TearDown().2: " << e.what() << "\n";
  }
}

void Shell_core_test_wrapper::enable_testutil() {
  bool dummy_sandboxes =
      g_test_recording_mode == mysqlshdk::db::replay::Mode::Replay;

  testutil.reset(new tests::Testutils(
      _sandbox_dir, _recording_enabled && dummy_sandboxes,
      {_mysql_sandbox_nport1, _mysql_sandbox_nport2, _mysql_sandbox_nport3},
      _interactive_shell));
  testutil->set_expected_boilerplate_version(g_mysql_version);
  testutil->set_user_input_feeder(
      [this](const std::string &prompt, const std::string &text) {
        output_handler.prompts.push_back({prompt, text});
      },
      [this](const std::string &prompt, const std::string &pass) {
        output_handler.passwords.push_back({prompt, pass});
      });

  if (g_test_recording_mode != mysqlshdk::db::replay::Mode::Direct)
    testutil->set_sandbox_snapshot_dir(
        mysqlshdk::db::replay::current_recording_dir());
  _interactive_shell->set_global_object("testutil", testutil);
}

void Shell_core_test_wrapper::enable_replay() {
  // Assumes reset_mysql() was already called
  setup_recorder();
  enable_testutil();
}

void Shell_core_test_wrapper::observe_session_notifications() {
  observe_notification("SN_SESSION_CONNECTED");
  observe_notification("SN_SESSION_CONNECTION_LOST");
  observe_notification("SN_SESSION_CLOSED");
  observe_notification("SN_DEBUGGER");
}

void Shell_core_test_wrapper::ignore_session_notifications() {
  ignore_notification("SN_SESSION_CONNECTED");
  ignore_notification("SN_SESSION_CONNECTION_LOST");
  ignore_notification("SN_SESSION_CLOSED");
  ignore_notification("SN_DEBUGGER");
}

void Shell_core_test_wrapper::handle_notification(
    const std::string &name, const shcore::Object_bridge_ref &sender,
    shcore::Value::Map_type_ref data) {
  std::string identifier = context_identifier();

  if (name == "SN_SESSION_CONNECTED") {
    auto position = _open_sessions.find(sender);
    if (position == _open_sessions.end())
      _open_sessions[sender] = identifier;
    // Prints the session warnings ONLY if TEST_SESSIONS is enabled
    else if (g_test_sessions) {
      std::cerr << "WARNING: Reopening session from " << _open_sessions[sender]
                << " at " << identifier << std::endl;
    }
  } else if (name == "SN_SESSION_CONNECTION_LOST" ||
             name == "SN_SESSION_CLOSED") {
    auto position = _open_sessions.find(sender);
    if (position != _open_sessions.end())
      _open_sessions.erase(position);
    // Prints the session warnings ONLY if TEST_SESSIONS is enabled
    else if (g_test_sessions) {
      std::cerr << "WARNING: Closing a session that was never opened at "
                << identifier << std::endl;
    }
  } else if (name == "SN_DEBUGGER") {
    std::cout << "DEBUG NOTIFICATION: " << data->get_string("value").c_str()
              << std::endl;
  }
}

void Shell_core_test_wrapper::execute(const std::string& code) {
  std::string _code(code);

  std::string executed_input = makeblue("mysql---> " + _code);
  output_handler.debug_print(executed_input);

  _interactive_shell->process_line(_code);
}

void Shell_core_test_wrapper::execute_noerr(const std::string& code) {
  ASSERT_EQ("", output_handler.std_err);
  execute(code);
  ASSERT_EQ("", output_handler.std_err);
}

void Shell_core_test_wrapper::exec_and_out_equals(const std::string &code,
                                                  const std::string &out,
                                                  const std::string &err) {
  std::string expected_output(out);
  std::string expected_error(err);

  if (_interactive_shell->interactive_mode() ==
          shcore::Shell_core::Mode::Python &&
      out.length())
    expected_output += "\n";

  if (_interactive_shell->interactive_mode() ==
          shcore::Shell_core::Mode::Python &&
      err.length())
    expected_error += "\n";

  execute(code);

  output_handler.std_out = str_strip(output_handler.std_out, " ");
  output_handler.std_err = str_strip(output_handler.std_err, " ");

  if (expected_output != "*") {
    EXPECT_EQ(expected_output, output_handler.std_out);
  }

  if (expected_error != "*") {
    EXPECT_EQ(expected_error, output_handler.std_err);
  }

  output_handler.wipe_all();
}

void Shell_core_test_wrapper::exec_and_out_contains(const std::string &code,
                                                    const std::string &out,
                                                    const std::string &err) {
  execute(code);

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
}

void Crud_test_wrapper::set_functions(const std::string &functions) {
  std::vector<std::string> str_spl = split_string_chars(functions, ", ", true);
  std::copy(str_spl.begin(), str_spl.end(),
            std::inserter(_functions, _functions.end()));
}

// Validates only the specified functions are available
// non listed functions are validated for unavailability
void Crud_test_wrapper::ensure_available_functions(
    const std::string &functions) {
  bool is_js = _interactive_shell->interactive_mode() ==
               shcore::Shell_core::Mode::JavaScript;
  std::vector<std::string> v = split_string_chars(functions, ", ", true);
  std::set<std::string> valid_functions(v.begin(), v.end());

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
        exec_and_out_equals(
            "print(real_functions.indexOf('" + *index + "') != -1)", "true");
      else
        exec_and_out_equals("index=real_functions.index('" + *index + "')");
    }

    // If not, should not be on the crud dir and calling it should be illegal
    else {
      SCOPED_TRACE("Function " + *index + " should NOT be available.");
      if (is_js)
        exec_and_out_equals(
            "print(real_functions.indexOf('" + *index + "') == -1)", "true");
      else
        exec_and_out_contains("print(real_functions.index('" + *index + "'))",
                              "", "is not in list");

      exec_and_out_contains("crud." + *index + "('');", "",
                            "Forbidden usage of " + *index);
    }
  }
}
