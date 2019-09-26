/* Copyright (c) 2015, 2019, Oracle and/or its affiliates. All rights reserved.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License, version 2.0,
   as published by the Free Software Foundation.

   This program is also distributed with certain software (including
   but not limited to OpenSSL) that is licensed under separate terms, as
   designated in a particular file or component or in included license
   documentation.  The authors of MySQL hereby grant you an additional
   permission to link the program and your derivative works with the
   separately licensed software that they have included with MySQL.
   This program is distributed in the hope that it will be useful,  but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
   the GNU General Public License, version 2.0, for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation, Inc.,
   51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA */

#include <chrono>
#include <fstream>
#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <thread>
#include <tuple>
#include <typeinfo>
#include <utility>
#include <vector>
#include "gtest_clean.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "scripting/common.h"
#include "scripting/lang_base.h"
#include "shellcore/base_session.h"
#include "shellcore/shell_core.h"
#include "src/mysqlsh/cmdline_shell.h"
#include "unittest/test_utils/mod_testutils.h"
#include "unittest/test_utils/shell_base_test.h"

#ifndef UNITTEST_TEST_UTILS_H_
#define UNITTEST_TEST_UTILS_H_

#define EXPECT_BECOMES_TRUE(timeout, pred)           \
  do {                                               \
    auto t = time(nullptr);                          \
    bool ok = false;                                 \
    while (time(nullptr) - t < (timeout)) {          \
      if ((pred)) {                                  \
        ok = true;                                   \
        break;                                       \
      }                                              \
      shcore::sleep_ms(100);                         \
    }                                                \
    if (!ok) FAIL() << "Timeout waiting for " #pred; \
  } while (0)

#define EXPECT_THROW_NOTHING(expr)                                \
  try {                                                           \
    expr;                                                         \
  } catch (std::exception & e) {                                  \
    ADD_FAILURE() << "Expected no exception thrown by " #expr     \
                  << typeid(e).name() << ":" << e.what() << "\n"; \
  } catch (...) {                                                 \
    ADD_FAILURE() << "Expected no exception thrown by " #expr     \
                  << " but got something\n";                      \
  }

namespace mysqlsh {
class Test_debugger;
}  // namespace mysqlsh

namespace testing {
// Fake deleter for shared pointers to avoid they attempt deleting the passed
// object
struct SharedDoNotDelete {
  template <typename T>
  void operator()(T *) {}
};
}  // namespace testing

/**
 * \ingroup UTFramework
 * Interface to process shell output and feed prompt data.
 */
class Shell_test_output_handler {
 public:
  // You can define per-test set-up and tear-down logic as usual.
  Shell_test_output_handler();
  virtual ~Shell_test_output_handler();

  virtual void TearDown() {}

  static bool deleg_print(void *user_data, const char *text);
  static bool deleg_print_error(void *user_data, const char *text);
  static bool deleg_print_diag(void *user_data, const char *text);
  static shcore::Prompt_result deleg_prompt(void *user_data,
                                            const char *UNUSED(prompt),
                                            std::string *ret);
  static shcore::Prompt_result deleg_password(void *user_data,
                                              const char *UNUSED(prompt),
                                              std::string *ret);

  void wipe_out() {
    std::lock_guard<std::mutex> lock(stdout_mutex);
    std_out.clear();
  }

  void wipe_err() { std_err.clear(); }
  void wipe_log() { log.clear(); }
  void wipe_all() {
    wipe_out();
    std_err.clear();
  }

  bool grep_stdout_thread_safe(const std::string &text) {
    std::lock_guard<std::mutex> lock(stdout_mutex);
    return (std_out.find(text) != std::string::npos);
  }

  shcore::Interpreter_delegate deleg;
  std::string std_err;
  std::string std_out;
  std::string internal_std_err;
  std::string internal_std_out;
  std::stringstream full_output;
  std::mutex stdout_mutex;
  static std::vector<std::string> log;

  void set_log_level(shcore::Logger::LOG_LEVEL log_level) {
    _logger->set_log_level(log_level);
  }

  shcore::Logger::LOG_LEVEL get_log_level() { return _logger->get_log_level(); }

  void validate_stdout_content(const std::string &content, bool expected);
  void validate_stderr_content(const std::string &content, bool expected);
  void validate_log_content(const std::vector<std::string> &content,
                            bool expected, bool clear = true);
  void validate_log_content(const std::string &content, bool expected,
                            bool clear = true);

  void debug_print(const std::string &line);
  void debug_print_header(const std::string &line);
  void flush_debug_log();
  void wipe_debug_log() { full_output.clear(); }

  bool debug;

  void feed_to_prompt(const std::string &line) {
    prompts.push_back({std::string("*"), line});
  }

  std::list<std::pair<std::string, std::string>> prompts;
  std::list<std::pair<std::string, std::string>> passwords;

  void set_internal(bool value) { m_internal = value; }
  void set_answers_to_stdout(bool value) { m_answers_to_stdout = value; }
  void set_errors_to_stderr(bool value) { m_errors_on_stderr = value; }

 protected:
  static shcore::Logger *_logger;

  static void log_hook(const shcore::Logger::Log_entry &entry, void *);
  bool m_internal;
  bool m_answers_to_stdout;
  bool m_errors_on_stderr;
};

#define MY_EXPECT_STDOUT_CONTAINS(x)                 \
  do {                                               \
    SCOPED_TRACE("...in stdout check\n");            \
    output_handler.validate_stdout_content(x, true); \
  } while (0)
#define MY_EXPECT_STDERR_CONTAINS(x)                 \
  do {                                               \
    SCOPED_TRACE("...in stderr check\n");            \
    output_handler.validate_stderr_content(x, true); \
  } while (0)
#define MY_EXPECT_LOG_CONTAINS(x)                 \
  do {                                            \
    SCOPED_TRACE("...in log check\n");            \
    output_handler.validate_log_content(x, true); \
  } while (0)
#define MY_EXPECT_STDOUT_NOT_CONTAINS(x)              \
  do {                                                \
    SCOPED_TRACE("...in stdout check\n");             \
    output_handler.validate_stdout_content(x, false); \
  } while (0)
#define MY_EXPECT_STDERR_NOT_CONTAINS(x)              \
  do {                                                \
    SCOPED_TRACE("...in stderr check\n");             \
    output_handler.validate_stderr_content(x, false); \
  } while (0)
#define MY_EXPECT_LOG_NOT_CONTAINS(x)              \
  do {                                             \
    SCOPED_TRACE("...in log check\n");             \
    output_handler.validate_log_content(x, false); \
  } while (0)

/**
 * \ingroup UTFramework
 * Base class for tests that use an instance of the shell library.
 */
class Shell_core_test_wrapper : public tests::Shell_base_test {
 public:
  static std::string get_options_file_name(
      const char *name = "test_options.json");

 protected:
  // You can define per-test set-up and tear-down logic as usual.
  virtual void SetUp();
  virtual void TearDown();
  virtual void set_defaults() {}
  virtual ::testing::TestInfo *info() { return nullptr; }
  virtual std::string context_identifier();

  std::string _custom_context;

  // void process_result(shcore::Value result);
  void execute(int location, const std::string &code);
  void execute(const std::string &code);
  void execute_internal(const std::string &code);
  void execute_noerr(const std::string &code);
  void exec_and_out_equals(const std::string &code, const std::string &out = "",
                           const std::string &err = "");
  void exec_and_out_contains(const std::string &code,
                             const std::string &out = "",
                             const std::string &err = "");

  // This can be use to reinitialize the interactive shell with different
  // options First set the options on _options
  void reset_options(int argc = 0, const char **argv = nullptr,
                     bool remove_config = true) {
    extern char *g_mppath;
    extern int g_test_default_verbosity;
    std::string options_file = get_options_file_name();
    if (remove_config) std::remove(options_file.c_str());
    _opts.reset(new mysqlsh::Shell_options(argc, const_cast<char **>(argv),
                                           options_file));
    _options = const_cast<mysqlsh::Shell_options::Storage *>(&_opts->get());
    _options->gadgets_path = g_mppath;
    if (!argv) _options->verbose_level = g_test_default_verbosity;
    _options->db_name_cache = false;

    // Allows derived classes configuring specific options
    set_options();
  }

  bool debug;
  void enable_debug() {
    debug = true;
    output_handler.debug = true;
  }
  virtual void set_options() {}

  void enable_testutil();
  void enable_replay();
  virtual void execute_setup() {}

  void replace_shell(std::shared_ptr<mysqlsh::Shell_options> options,
                     std::unique_ptr<shcore::Interpreter_delegate> delegate) {
    // previous shell needs to be destroyed before a new one can be created
    _interactive_shell.reset();
    _interactive_shell.reset(
        new mysqlsh::Command_line_shell(options, std::move(delegate)));
  }

  virtual void reset_shell() {
    // If the options have not been set, they must be set now, otherwise
    // they should be explicitly reset
    if (!_opts) reset_options();

    replace_shell(_opts,
                  std::unique_ptr<shcore::Interpreter_delegate>{
                      new shcore::Interpreter_delegate(output_handler.deleg)});

    _interactive_shell->finish_init();
    set_defaults();
    enable_testutil();
  }
  void reset_replayable_shell(const char *sub_test_name = nullptr);

  void connect_classic();
  void connect_x();

  Shell_test_output_handler output_handler;
  std::shared_ptr<mysqlsh::Command_line_shell> _interactive_shell;
  std::shared_ptr<mysqlsh::Shell_options> _opts;
  mysqlsh::Shell_options::Storage *_options;

  // set to true in a subclass if reset_shell() should not be called
  // during SetUp()
  bool _delay_reset_shell = false;

  virtual void debug_print(const std::string &s) {
    output_handler.debug_print(s);
  }

  void wipe_out() { output_handler.wipe_out(); }
  void wipe_err() { output_handler.wipe_err(); }
  void wipe_log() { output_handler.wipe_log(); }
  void wipe_all() { output_handler.wipe_all(); }

  std::shared_ptr<tests::Testutils> testutil;

  static std::shared_ptr<tests::Testutils> make_testutils(
      std::shared_ptr<mysqlsh::Command_line_shell> shell = {}) {
    return std::make_shared<tests::Testutils>(_sandbox_dir, false, shell,
                                              get_path_to_mysqlsh());
  }

  std::shared_ptr<mysqlsh::Shell_options> get_options() {
    if (!_opts) reset_options();

    return _opts;
  }

  std::chrono::time_point<std::chrono::steady_clock> m_start_time =
      std::chrono::steady_clock::now();

  friend class mysqlsh::Test_debugger;
};

/**
 * \ingroup UTFramework
 * Helper class to ease the creation of tests on the CRUD operations specially
 * on the chained methods
 */
class Crud_test_wrapper : public ::Shell_core_test_wrapper {
 protected:
  std::set<std::string> _functions;

  // Sets the functions that will be available for chaining
  // in a CRUD operation
  void set_functions(const std::string &functions);

  // Validates only the specified functions are available
  // non listed functions are validated for unavailability
  void ensure_available_functions(const std::string &functions);
};

#endif  // UNITTEST_TEST_UTILS_H_
