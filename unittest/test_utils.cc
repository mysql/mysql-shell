/*
 * Copyright (c) 2015, 2025, Oracle and/or its affiliates.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.0,
 * as published by the Free Software Foundation.
 *
 * This program is designed to work with certain software (including
 * but not limited to OpenSSL) that is licensed under separate terms,
 * as designated in a particular file or component or in included license
 * documentation.  The authors of MySQL hereby grant you an additional
 * permission to link the program and your derivative works with the
 * separately licensed software that they have either included with
 * the program or referenced in the documentation.
 *
 * This program is distributed in the hope that it will be useful,  but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 * the GNU General Public License, version 2.0, for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "unittest/test_utils.h"

#include <cinttypes>
#include <memory>
#include <random>
#include <string>

#include <rapidjson/document.h>
#include <rapidjson/pointer.h>

#include "db/replay/setup.h"
#include "db/uri_encoder.h"
#include "mysqlshdk/libs/textui/textui.h"
#include "mysqlshdk/libs/utils/utils_path.h"
#include "shellcore/base_session.h"
#include "shellcore/shell_resultset_dumper.h"
#include "src/mysqlsh/prompt_handler.h"
#include "unittest/test_utils/sandboxes.h"
#include "utils/utils_file.h"
#include "utils/utils_general.h"
#include "utils/utils_string.h"

using namespace shcore;

std::vector<std::string> Shell_test_output_handler::log;

extern mysqlshdk::db::replay::Mode g_test_recording_mode;
extern int g_profile_test_scripts;

namespace testing {
namespace {
bool match_json_object(const rapidjson::Value *object,
                       const shcore::Dictionary_t &expected) {
  bool matched_all = false;

  if (object->IsObject()) {
    matched_all = true;
  } else {
    return false;
  }

  for (const auto &iter : *expected) {
    if (!object->HasMember(iter.first.c_str()) ||
        (*object)[iter.first.c_str()].GetString() != iter.second.as_string()) {
      matched_all = false;
      break;
    }
  }

  return matched_all;
}

bool check_json_array_contains(const rapidjson::Value *object,
                               const shcore::Value &expected) {
  if (object->IsArray()) {
    auto array = object->GetArray();
    for (const auto &element : array) {
      if (expected.get_type() == shcore::Map) {
        if (match_json_object(&element, expected.as_map())) {
          return true;
        }
      }
    }
  }

  return false;
}

bool check_json_contains(const rapidjson::Value *object,
                         const shcore::Value &expected) {
  if (object->IsArray()) {
    return check_json_array_contains(object, expected);
  } else if (object->IsObject()) {
    if (expected.get_type() == shcore::Value_type::Map) {
      return match_json_object(object, expected.as_map());
    }
  }

  return false;
}
}  // namespace

void expect_json_contains(const rapidjson::Value *object,
                          const std::string &expected_json, bool contains,
                          const char *file, int line) {
  std::string error_message =
      "Expected JSON element not found: " + expected_json + "\n\tAt " + file +
      ":" + std::to_string(line);
  SCOPED_TRACE(error_message);

  auto expected = shcore::Value::parse(expected_json);
  EXPECT_EQ(contains, testing::check_json_contains(object, expected));
}

}  // namespace testing

Shell_test_json_validator::Shell_test_json_validator(const std::string &json) {
  m_document.Parse(json.c_str());
}

/**
 * @brief Gets a JSON element using Pointer notation
 *
 * @param path The path to the element being searched
 * @return const rapidjson::Value* the found element or null
 */
const rapidjson::Value *Shell_test_json_validator::get_object(
    const std::string &path) const {
  return rapidjson::Pointer(path.c_str()).Get(m_document);
}

Shell_test_output_handler::Shell_test_output_handler()
    : deleg(this, &Shell_test_output_handler::deleg_print,
            &Shell_test_output_handler::deleg_prompt,
            &Shell_test_output_handler::deleg_print_error,
            &Shell_test_output_handler::deleg_print_diag),
      m_internal(false),
      m_answers_to_stdout(false),
      m_errors_on_stderr(false) {
  full_output.clear();
  debug = false;

  m_logger = shcore::current_logger();

  // Initialize the logger and attach the hook for error verification
  // Assumes logfile already initialized
  if (getenv("TEST_DEBUG") != nullptr) m_logger->log_to_stderr();

  // By default we need to setup the logger to LOG_INFO.
  m_logger->set_log_level(shcore::Logger::LOG_INFO);

  m_logger->attach_log_hook(log_hook, this);
}

Shell_test_output_handler::~Shell_test_output_handler() {
  m_logger->detach_log_hook(log_hook);
}

void Shell_test_output_handler::log_hook(const shcore::Logger::Log_entry &entry,
                                         void *self_ptr) {
  Shell_test_output_handler *self =
      reinterpret_cast<Shell_test_output_handler *>(self_ptr);
  shcore::Logger::LOG_LEVEL current_level = self->m_logger->get_log_level();

  // If the level of the log is different than
  // the one set, we don't want to store the message
  if (current_level == entry.level) self->log.emplace_back(entry.message);
}

bool Shell_test_output_handler::deleg_print(void *user_data, const char *text) {
  Shell_test_output_handler *target =
      reinterpret_cast<Shell_test_output_handler *>(user_data);

  std::lock_guard<std::mutex> lock(target->stdout_mutex);
  if (!target->m_internal) {
    target->full_output << text << std::flush;

    if (target->debug || g_test_trace_scripts ||
        shcore::str_beginswith(text, "**"))
      std::cout << text << std::flush;

    target->std_out.append(text);
  } else {
    target->internal_std_out.append(text);
  }

  return true;
}

bool Shell_test_output_handler::deleg_print_error(void *user_data,
                                                  const char *text) {
  Shell_test_output_handler *target =
      reinterpret_cast<Shell_test_output_handler *>(user_data);

  // Note: print_error() should send output to stderr instead of stdout and
  // thus, be removed in favour of print_diag(). However, we keep print_error()
  // and print_diag() separate, so that we can make print_error() send output
  // to stderr in the real shell app, while in tests it sends output to
  // stdout. This is for backwards compatibility in how tests are checked.
  //
  // Update: Now it is possible to make the print_error() send the text to
  // stderr as the shell by calling output_handler.set_errors_to_stderr(...)
  std::lock_guard<std::mutex> lock(target->stdout_mutex);
  target->full_output << text << std::flush;

  if (target->debug || g_test_trace_scripts)
    std::cout << (shcore::str_beginswith(text, "ERROR") ? makelred(text) : text)
              << std::flush;

  if (!target->m_internal) {
    if (target->m_errors_on_stderr) {
      target->std_err.append(text);
    } else {
      target->std_out.append(text);
    }
  } else {
    target->internal_std_err.append(text);
  }

  return true;
}

bool Shell_test_output_handler::deleg_print_diag(void *user_data,
                                                 const char *text) {
  Shell_test_output_handler *target =
      reinterpret_cast<Shell_test_output_handler *>(user_data);

  if (!target->m_internal) {
    target->full_output << makered(text) << std::endl;

    if (target->debug || g_test_trace_scripts)
      std::cerr << makered(text) << std::endl;

    target->std_err.append(text);
  } else {
    target->internal_std_err.append(text);
  }

  return true;
}

shcore::Prompt_result Shell_test_output_handler::do_prompt(bool is_password,
                                                           const char *prompt,
                                                           std::string *ret) {
  std::lock_guard<std::mutex> lock(stdout_mutex);
  shcore::prompt::Prompt_options expected_options;
  std::string answer, expected_prompt;
  full_output << prompt;
  std_out.append(prompt);

  auto &prompt_replies{is_password ? passwords : prompts};

  shcore::Prompt_result ret_val = shcore::Prompt_result::Cancel;
  if (!prompt_replies.empty()) {
    std::tie(expected_prompt, answer, expected_options) =
        prompt_replies.front();
    prompt_replies.pop_front();

    if (expected_prompt == "*" ||
        shcore::str_beginswith(prompt, expected_prompt)) {
      debug_print(makegreen(shcore::str_format(
          "\n--> %s %s %s", is_password ? "password" : "prompt", prompt,
          answer.c_str())));
      full_output << answer << std::endl;
    } else {
      ADD_FAILURE() << "Mismatched " << (is_password ? " pwd" : "")
                    << "prompts. Expected: '" << expected_prompt << "'\n"
                    << "actual: '" << prompt << "'";
      debug_print(
          makered(shcore::str_format("\n--> mismatched%s prompt '%s'",
                                     (is_password ? " pwd" : ""), prompt)));
    }
    if (answer != "<<<CANCEL>>>") ret_val = shcore::Prompt_result::Ok;
  } else {
    ADD_FAILURE() << "Unexpected " << (is_password ? "password " : "")
                  << "prompt for '" << prompt << "'";
    debug_print(
        makered(shcore::str_format("\n--> unexpected%s prompt '%s'",
                                   (is_password ? " pwd" : ""), prompt)));
  }

  if (m_answers_to_stdout)
    std_out.append(is_password ? std::string(answer.size(), '*') : answer)
        .append("\n");

  *ret = answer;
  return ret_val;
}

shcore::Prompt_result Shell_test_output_handler::deleg_prompt(
    void *user_data, const char *prompt,
    const shcore::prompt::Prompt_options &options, std::string *ret) {
  Shell_test_output_handler *target =
      reinterpret_cast<Shell_test_output_handler *>(user_data);

  auto ret_val =
      mysqlsh::Prompt_handler(
          std::bind(&Shell_test_output_handler::do_prompt, target, _1, _2, _3))
          .handle_prompt(prompt, options, ret);

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
      for (const auto &piece : log) s += piece;

      ADD_FAILURE() << error << "\n"
                    << "LOG Actual: " + s;
    }
  }

  // Wipe the log here
  if (clear) wipe_log();
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
    for (const auto &piece : log) s += piece;

    ADD_FAILURE() << error << "\n"
                  << "LOG Actual: " + s;
  }

  // Wipe the log here
  if (clear) wipe_log();
}

void Shell_test_output_handler::debug_print(const std::string &line) {
  if (debug || g_test_trace_scripts) std::cout << line << std::endl;

  full_output << line << '\n';
}

void Shell_test_output_handler::debug_print_header(const std::string &line) {
  if (debug || g_test_trace_scripts) std::cerr << makebold(line) << std::endl;

  std::string splitter(line.length(), '-');

  full_output << splitter << '\n';
  full_output << line << '\n';
  full_output << splitter << '\n';
}

void Shell_test_output_handler::flush_debug_log() {
  full_output.flush();
  std::cerr << full_output.str();

  full_output.str(std::string());
  full_output.clear();
}

Shell_test_json_validator Shell_test_output_handler::stdout_as_json() {
  std::lock_guard<std::mutex> lock(stdout_mutex);
  return Shell_test_json_validator(std_out);
}

void Shell_core_test_wrapper::connect_classic(const std::string &schema) {
  execute("\\connect --mc " + _mysql_uri +
          (schema.empty() ? "" : "/" + schema));
}

void Shell_core_test_wrapper::connect_x(const std::string &schema) {
  execute("\\connect --mx " + _uri + (schema.empty() ? "" : "/" + schema));
}

std::string Shell_core_test_wrapper::context_identifier() {
  std::string ret_val;

  if (auto test_info = info(); test_info) {
    ret_val.append(test_info->test_case_name());
    ret_val.append(".");
    ret_val.append(test_info->name());
  }

  if (!_custom_context.empty()) {
    if (ret_val.empty()) return _custom_context;

    ret_val.append(": ").append(_custom_context);
  }

  return ret_val;
}

std::string Shell_core_test_wrapper::get_options_file_name(const char *name) {
  return shcore::path::join_path(shcore::get_user_config_path(), name);
}

void Shell_core_test_wrapper::SetUp() {
  Shell_base_test::SetUp();

  m_start_time = std::chrono::steady_clock::now();

  output_handler.debug_print_header(context_identifier());

  debug = false;
  output_handler.debug = debug;

  // Initializes the interactive shell
  reset_shell();

  if (getenv("TEST_DEBUG")) {
    output_handler.set_log_level(shcore::Logger::LOG_DEBUG);
    enable_debug();
  }
}

void Shell_core_test_wrapper::TearDown() {
  if (!m_slowest_lines.empty()) {
    std::cout << makeyellow("Slowest lines:") << "\n";
    for (const auto &line : m_slowest_lines) {
      std::cout << "  " << line.first << "ms " << line.second << "\n";
    }
  }

  if (testutil) {
    _interactive_shell->set_global_object("testutil", {});
    testutil.reset();
  }
  _interactive_shell.reset();

  tests::Shell_base_test::TearDown();
}

void Shell_core_test_wrapper::enable_testutil() {
  bool dummy_sandboxes =
      g_test_recording_mode == mysqlshdk::db::replay::Mode::Replay;

  testutil.reset(
      new tests::Testutils(_sandbox_dir, _recording_enabled && dummy_sandboxes,
                           _interactive_shell, get_path_to_mysqlsh()));
  testutil->set_test_callbacks(
      [this](const std::string &prompt, const std::string &text,
             const shcore::prompt::Prompt_options &options) {
        output_handler.prompts.push_back({prompt, text, options});
      },
      [this](const std::string &prompt, const std::string &pass,
             const shcore::prompt::Prompt_options &options) {
        output_handler.passwords.push_back({prompt, pass, options});
      },
      [this](bool one) -> std::string {
        if (one) {
          return shcore::str_partition_after_inpl(&output_handler.std_out,
                                                  "\n");
        } else {
          return output_handler.std_out;
        }
      },
      [this](bool one) -> std::string {
        if (one) {
          return shcore::str_partition_after_inpl(&output_handler.std_err,
                                                  "\n");
        } else {
          return output_handler.std_err;
        }
      },
      [this]() -> void {
        {
          std::vector<std::string> prompts;
          for (const auto &prompt : output_handler.prompts) {
            prompts.push_back(shcore::str_format("PROMPT: '%s' [%s]",
                                                 std::get<0>(prompt).c_str(),
                                                 std::get<1>(prompt).c_str()));
          }
          SCOPED_TRACE(shcore::str_join(prompts, "\n").c_str());
          EXPECT_EQ(0, output_handler.prompts.size());
        }
        {
          std::vector<std::string> passwords;
          for (const auto &password : output_handler.passwords) {
            passwords.push_back(shcore::str_format(
                "PROMPT: '%s' [%s]", std::get<0>(password).c_str(),
                std::get<1>(password).c_str()));
          }
          SCOPED_TRACE(shcore::str_join(passwords, "\n").c_str());
          EXPECT_EQ(0, output_handler.passwords.size());
        }
      },
      [this]() -> void { output_handler.wipe_all(); });

  if (g_test_recording_mode != mysqlshdk::db::replay::Mode::Direct)
    testutil->set_sandbox_snapshot_dir(
        mysqlshdk::db::replay::current_recording_dir());

  _interactive_shell->set_global_object("testutil", testutil);
}

void Shell_core_test_wrapper::enable_replay() {
  // Assumes reset_mysql() was already called
  setup_recorder();
}

void Shell_core_test_wrapper::reset_replayable_shell(
    const char *sub_test_name) {
  setup_recorder(sub_test_name);  // must be called before set_defaults()
  reset_shell();
  execute_setup();

#ifdef _WIN32
  mysqlshdk::db::replay::set_replay_query_hook([](std::string_view sql) {
    return shcore::str_replace(sql, ".dll", ".so");
  });
#endif

  // Intercept queries and hack their results so that we can have
  // recorded local sessions that match the actual local environment
  mysqlshdk::db::replay::set_replay_row_hook(std::bind(
      &Shell_test_env::set_replay_row_hook, this, std::placeholders::_1,
      std::placeholders::_2, std::placeholders::_3));

  // Set up hook to replace (non-deterministic) queries.
  mysqlshdk::db::replay::set_replay_query_hook(
      [this](auto sql) { return Shell_test_env::query_replace_hook(sql); });
}

void Shell_core_test_wrapper::execute(int location, const std::string &code) {
  std::string executed_input;
  const auto now = std::chrono::steady_clock::now();

  if (g_profile_test_scripts) {
    const auto elapsed = now - m_start_time;
    const auto m = std::chrono::duration_cast<std::chrono::minutes>(elapsed);
    const auto s = std::chrono::duration_cast<std::chrono::seconds>(
        elapsed % std::chrono::minutes(1));
    const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        elapsed % std::chrono::seconds(1));

    executed_input = shcore::str_format(
        "[%2dm%02d.%02ds] %4d> ", static_cast<int>(m.count()),
        static_cast<int>(s.count()), static_cast<int>(ms.count()) / 10,
        location);

    executed_input += code;
    output_handler.debug_print(makeblue(executed_input));
  } else {
    executed_input = shcore::str_format("%4d> ", location);
    executed_input += code;
    output_handler.debug_print(makeblue(executed_input));
  }

  _interactive_shell->process_line(code);
  _interactive_shell->reconnect_if_needed();

  if (g_profile_test_scripts > 1) {
    const auto elapsed = std::chrono::steady_clock::now() - now;

    if (std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count() >
        500) {
      std::string time = shcore::str_format(
          "<<< %d.%02ds",
          static_cast<int>(
              std::chrono::duration_cast<std::chrono::seconds>(elapsed)
                  .count()),
          static_cast<int>(
              std::chrono::duration_cast<std::chrono::milliseconds>(
                  elapsed % std::chrono::seconds(1))
                  .count()) /
              10);

      output_handler.debug_print(makeblue(time));

      m_slowest_lines.push_back(
          {std::chrono::duration_cast<std::chrono::milliseconds>(elapsed)
               .count(),
           executed_input});
      m_slowest_lines.sort(
          [](const auto &a, const auto &b) { return a.first > b.first; });
      if (m_slowest_lines.size() > 10) m_slowest_lines.pop_back();
    }
  }
}

void Shell_core_test_wrapper::execute(const std::string &code) {
  std::string executed_input = makeblue("----> " + code);
  output_handler.debug_print(executed_input);

  _interactive_shell->process_line(code);
}

void Shell_core_test_wrapper::execute_internal(const std::string &code) {
  output_handler.internal_std_err.clear();
  output_handler.internal_std_out.clear();
  output_handler.set_internal(true);
  _interactive_shell->process_line(code);
  output_handler.set_internal(false);
}

void Shell_core_test_wrapper::execute_noerr(const std::string &code) {
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
    exec_and_out_equals("real_functions = dir(crud)");

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
    // If the function is suppossed to be valid it needs to be available on
    // the crud dir
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
