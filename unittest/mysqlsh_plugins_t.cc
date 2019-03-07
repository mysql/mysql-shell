/*
 * Copyright (c) 2018, 2019, Oracle and/or its affiliates. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.0,
 * as published by the Free Software Foundation.
 *
 * This program is also distributed with certain software (including
 * but not limited to OpenSSL) that is licensed under separate terms, as
 * designated in a particular file or component or in included license
 * documentation.  The authors of MySQL hereby grant you an additional
 * permission to link the program and your derivative works with the
 * separately licensed software that they have included with MySQL.
 * This program is distributed in the hope that it will be useful,  but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 * the GNU General Public License, version 2.0, for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include <thread>

#include "unittest/gtest_clean.h"
#include "unittest/test_utils.h"
#include "unittest/test_utils/command_line_test.h"
#include "unittest/test_utils/mocks/gmock_clean.h"

#include "mysqlshdk/libs/utils/utils_file.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_path.h"

namespace tests {

#ifdef _WIN32
#define unsetenv(var) _putenv(var "=")
#endif

class Mysqlsh_plugins_test : public Command_line_test {
 protected:
  void SetUp() override {
    Command_line_test::SetUp();

    cleanup();

    putenv(const_cast<char *>("MYSQLSH_PROMPT_THEME=invalid"));
    putenv(const_cast<char *>("MYSQLSH_TERM_COLOR_MODE=nocolor"));

    shcore::create_directory(get_plugin_folder(), true);
  }

  void TearDown() override {
    Command_line_test::TearDown();

    cleanup();
  }

  std::string get_plugin_folder() const {
    return shcore::path::join_path(shcore::get_user_config_path(),
                                   get_plugin_folder_name());
  }

  void write_plugin(const std::string &name, const std::string &contents) {
    shcore::create_file(shcore::path::join_path(get_plugin_folder(), name),
                        contents);
  }

  void delete_plugin(const std::string &name) {
    shcore::delete_file(shcore::path::join_path(get_plugin_folder(), name));
  }

  void run(const std::vector<std::string> &extra = {}) {
    shcore::create_file(k_file, shcore::str_join(m_test_input, "\n"));

    std::vector<const char *> args = {_mysqlsh, _uri.c_str(), "--interactive"};

    for (const auto &e : extra) {
      args.emplace_back(e.c_str());
    }

    args.emplace_back(nullptr);

    execute(args, nullptr, k_file);
  }

  std::string expected_output() const {
    return shcore::str_join(m_expected_output, "\n");
  }

  void add_test(const std::string &input, const std::string &output = "") {
    m_test_input.emplace_back(input);

    if (!output.empty()) {
      m_expected_output.emplace_back(output);
    }
  }

  void add_js_test(const std::string &input, const std::string &output = "") {
#ifdef HAVE_V8
    add_test(input, output);
#endif  // HAVE_V8
  }

  void add_py_test(const std::string &input, const std::string &output = "") {
#ifdef HAVE_PYTHON
    add_test(input, output);
#endif  // HAVE_PYTHON
  }

  void add_expected_log(const std::string &expected) {
    m_expected_log_output.emplace_back(expected);
  }

  void add_expected_js_log(const std::string &expected) {
#ifdef HAVE_V8
    add_expected_log(expected);
#endif  // HAVE_V8
  }

  void add_expected_py_log(const std::string &expected) {
#ifdef HAVE_PYTHON
    add_expected_log(expected);
#endif  // HAVE_PYTHON
  }

  void validate_log() const {
    const auto log = read_log_file();

    SCOPED_TRACE("Validate log file contests");

    for (const auto &entry : m_expected_log_output) {
      EXPECT_THAT(log, ::testing::HasSubstr(entry));
    }
  }

  static void wipe_log_file() {
    const auto log = shcore::Logger::singleton()->logfile_name();

    {
      std::ifstream f(log);
      if (!f.good()) {
        throw std::runtime_error("Failed to wipe log file '" + log +
                                 "': " + strerror(errno));
      }
      f.close();
    }
    {
      // truncate file
      std::ofstream out(log, std::ios::out | std::ios::trunc);
      out.close();
    }
  }

  static std::string read_log_file() {
    return shcore::get_text_file(shcore::Logger::singleton()->logfile_name());
  }

  virtual std::string get_plugin_folder_name() const = 0;

 private:
  void cleanup() {
    unsetenv("MYSQLSH_PROMPT_THEME");
    unsetenv("MYSQLSH_TERM_COLOR_MODE");
    wipe_out();

    const auto folder = get_plugin_folder();

    if (shcore::is_folder(folder)) {
      shcore::remove_directory(folder, true);
    }

    if (shcore::is_file(k_file)) {
      shcore::delete_file(k_file);
    }
  }

  std::vector<std::string> m_test_input;
  std::vector<std::string> m_expected_output;
  std::vector<std::string> m_expected_log_output;

  static const char k_file[];
};

const char Mysqlsh_plugins_test::k_file[] = "plugin_test";

class Mysqlsh_reports_test : public Mysqlsh_plugins_test {
 protected:
  std::string get_plugin_folder_name() const override { return "init.d"; }
};

TEST_F(Mysqlsh_reports_test, WL11263_TSF8_1) {
  // WL11263_TSF8_1 - Validate that the registered reports are loaded (are
  // available) when Shell starts.

  // create first JS plugin file
  write_plugin("first.js", R"(function report(s) {
  println('first JS report');
  return {'report' : []};
}

shell.registerReport('first_js', 'print', report);
)");

  // create second JS plugin file - upper-case extension
  write_plugin("second.JS", R"(function report(s) {
  println('second JS report');
  return {'report' : []};
}

shell.registerReport('second_js', 'print', report);
)");

  // create first PY plugin file
  write_plugin("third.py", R"(def report(s):
  print('third PY report');
  return {'report' : []};

shell.register_report('third_py', 'print', report);
)");

  // create second PY plugin file - upper-case extension
  write_plugin("fourth.PY", R"(def report(s):
  print('fourth PY report');
  return {'report' : []};

shell.register_report('fourth_py', 'print', report);
)");

  // check if first JS report is available
  add_js_test("\\show first_js", "first JS report");
  // check if second JS report is available
  add_js_test("\\show second_js", "second JS report");
  // check if first PY report is available
  add_py_test("\\show third_py", "third PY report");
  // check if second PY report is available
  add_py_test("\\show fourth_py", "fourth PY report");
  // run the test
  run();
  // check the output
  MY_EXPECT_CMD_OUTPUT_CONTAINS(expected_output());
  wipe_out();
}

TEST_F(Mysqlsh_reports_test, WL11263_TSF8_2) {
  // WL11263_TSF8_2 - Try to use registered plugin files that doesn't exist
  // nymore in the Shell config path, call the report and analyze the behaviour.

  // create JS plugin file
  write_plugin("plugin.js", R"(function js_report(s) {
  println('JS report');
  return {'report' : []};
}

shell.registerReport('js_report', 'print', js_report);
)");

  // create PY plugin file
  write_plugin("plugin.py", R"(def py_report(s):
  print('PY report');
  return {'report' : []};

shell.register_report('py_report', 'print', py_report);
)");

  add_js_test("\\js", "Switching to JavaScript mode...");
  add_js_test("os.sleep(2)");

  add_py_test("\\py", "Switching to Python mode...");
  add_py_test("import time");
  add_py_test("time.sleep(2)");

  // check if JS report is available
  add_js_test("\\show js_report", "JS report");
  // check if PY report is available
  add_py_test("\\show py_report", "PY report");

  std::thread thd([this]() {
    shcore::sleep_ms(1000);
    delete_plugin("plugin.js");
    delete_plugin("plugin.py");
  });

  // run the test - start in SQL mode so we can switch to another one
  run({"--sql"});
  // check the output
  MY_EXPECT_CMD_OUTPUT_CONTAINS(expected_output());
  wipe_out();

  thd.join();
}

TEST_F(Mysqlsh_reports_test, WL11263_TSF8_3) {
  // WL11263_TSF8_3 - Register plugin files that contains code in a language
  // different than the scripting languages supported, call the report and
  // analyze the behaviour.
  static constexpr auto cpp_code = R"(int main(int argc, char* argv[]) {
  return 0;
}
)";

  // create JS file with C++ code
  write_plugin("cpp_as.js", cpp_code);

  // create PY file with C++ code
  write_plugin("cpp_as.py", cpp_code);

  // check if only build-in reports are available
  add_test("\\show", "Available reports: query.");

  // log file should contain information about erroneous plugin files
  add_expected_js_log("Failed to compile JS plugin");
  add_expected_js_log("cpp_as.js");
  add_expected_py_log("Error while executing Python plugin");
  add_expected_py_log("cpp_as.py");

  // erase the log file
  wipe_log_file();

  // run the test
  run();
  // check the output
  MY_EXPECT_CMD_OUTPUT_CONTAINS(expected_output());
  wipe_out();

  // check log contents
  validate_log();
}

TEST_F(Mysqlsh_reports_test, WL11263_TSF8_4) {
  // WL11263_TSF8_4 - Validate that the plugin files doesn't influence the
  // global scripting context. Try to create global functions and variables,
  // and to set a new value to global objects (like session or dba), the context
  // must not change.

  // create JS plugin file
  write_plugin("plugin.js", R"(function js_report(s) {
  println('JS report');
  return {'report' : []};
}

shell.registerReport('js_report', 'print', js_report);

dba = 'DBA set from JS';
global_js_variable = 'JS variable';
)");

  // create PY plugin file
  write_plugin("plugin.py", R"(def py_report(s):
  print('PY report');
  return {'report' : []};

shell.register_report('py_report', 'print', py_report);

dba = 'DBA set from PY'
global_py_variable = 'PY variable'
)");

  // check if JS report is available
  add_js_test("\\show js_report", "JS report");
  // check if PY report is available
  add_py_test("\\show py_report", "PY report");

  add_js_test("\\js", "Switching to JavaScript mode...");
  add_js_test("js_report(null, null);",
              "ReferenceError: js_report is not defined");
  add_js_test("global_js_variable",
              "ReferenceError: global_js_variable is not defined");
  add_js_test("dba", "<Dba>");

  add_py_test("\\py", "Switching to Python mode...");
  add_py_test("js_report(Null, Null);", R"(Traceback (most recent call last):
  File "<string>", line 1, in <module>
NameError: name 'js_report' is not defined)");
  add_py_test("global_py_variable", R"(Traceback (most recent call last):
  File "<string>", line 1, in <module>
NameError: name 'global_py_variable' is not defined)");
  add_py_test("dba", "<Dba>");

  // run the test - start in SQL mode so we can switch to another one
  run({"--sql"});
  // check the output
  MY_EXPECT_CMD_OUTPUT_CONTAINS(expected_output());
  wipe_out();
}

TEST_F(Mysqlsh_reports_test, WL11263_TSF8_5) {
  // WL11263_TSF8_5 - Register a plugin file setting an undefined method, call
  // the report and analyze the behaviour.

  // create JS plugin file which tries to register undefined method
  write_plugin("plugin.js", R"(
shell.registerReport('undefined_js_report', 'print', undefined_js_report);
)");

  // create PY plugin file file which tries to register undefined method
  write_plugin("plugin.py", R"(
shell.register_report('undefined_py_report', 'print', undefined_py_report);
)");

  // check if JS report is not available
  add_js_test("\\show undefined_js_report",
              "Unknown report: undefined_js_report");
  // check if PY report is not available
  add_py_test("\\show undefined_py_report",
              "Unknown report: undefined_py_report");

  // log file should contain information about erroneous plugin files
  add_expected_js_log("Error while executing JS plugin");
  add_expected_js_log("plugin.js");
  add_expected_js_log("undefined_js_report is not defined");
  add_expected_py_log("Error while executing Python plugin");
  add_expected_py_log("plugin.py");
  add_expected_py_log("name 'undefined_py_report' is not defined");

  // erase the log file
  wipe_log_file();

  // run the test
  run();
  // check the output
  MY_EXPECT_CMD_OUTPUT_CONTAINS(expected_output());
  wipe_out();

  // check log contents
  validate_log();
}

}  // namespace tests
