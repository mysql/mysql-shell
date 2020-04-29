/*
 * Copyright (c) 2018, 2020, Oracle and/or its affiliates. All rights reserved.
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

extern bool g_test_parallel_execution;

namespace tests {

using shcore::path::join_path;

class Mysqlsh_extension_test : public Command_line_test {
 protected:
  void SetUp() override {
    Command_line_test::SetUp();

    cleanup();

    shcore::setenv("MYSQLSH_PROMPT_THEME", "invalid");
    shcore::setenv("MYSQLSH_TERM_COLOR_MODE", "nocolor");

    shcore::create_directory(get_plugin_folder(), true);
  }

  void TearDown() override {
    Command_line_test::TearDown();

    cleanup();
  }

  std::string get_plugin_folder() const {
    return join_path(shcore::get_user_config_path(), get_plugin_folder_name());
  }

  void write_plugin(const std::string &name, const std::string &contents) {
    shcore::create_file(join_path(get_plugin_folder(), name), contents);
  }

  void delete_plugin(const std::string &name) {
    shcore::delete_file(join_path(get_plugin_folder(), name));
  }

  void run(const std::vector<std::string> &extra = {}) {
    shcore::create_file(k_file, shcore::str_join(m_test_input, "\n"));

    std::vector<const char *> args = {_mysqlsh, _uri.c_str(), "--interactive"};

    for (const auto &e : extra) {
      args.emplace_back(e.c_str());
    }

    args.emplace_back(nullptr);

    auto user_config = shcore::get_user_config_path();
    user_config = "MYSQLSH_USER_CONFIG_HOME=" + user_config;
    execute(args, nullptr, k_file, {user_config});
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

  void add_unexpected_log(const std::string &expected) {
    m_unexpected_log_output.emplace_back(expected);
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

  void add_unexpected_js_log(const std::string &unexpected) {
#ifdef HAVE_V8
    add_unexpected_log(unexpected);
#endif  // HAVE_V8
  }

  void add_unexpected_py_log(const std::string &unexpected) {
#ifdef HAVE_PYTHON
    add_unexpected_log(unexpected);
#endif  // HAVE_PYTHON
  }

  void validate_log() const {
    const auto log = read_log_file();

    SCOPED_TRACE("Validate log file contests");

    for (const auto &entry : m_expected_log_output) {
      EXPECT_THAT(log, ::testing::HasSubstr(entry));
    }
    for (const auto &entry : m_unexpected_log_output) {
      EXPECT_THAT(log, Not(::testing::HasSubstr(entry)));
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
    shcore::unsetenv("MYSQLSH_PROMPT_THEME");
    shcore::unsetenv("MYSQLSH_TERM_COLOR_MODE");
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
  std::vector<std::string> m_unexpected_log_output;

  static const char k_file[];
};

const char Mysqlsh_extension_test::k_file[] = "plugin_test";

class Mysqlsh_reports_test : public Mysqlsh_extension_test {
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
  // WL11263_TSF8_2 - Try to use registered plugin files that doesn't
  // anymore in the Shell config path, call the report and analyze the
  // behaviour.

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
  add_test("\\show", "Available reports: query, thread, threads.");

  // log file should contain information about erroneous plugin files
  add_expected_js_log("Error loading JavaScript file");
  add_expected_js_log("cpp_as.js");
  add_expected_py_log("Error loading Python file");
  add_expected_py_log("cpp_as.py");

  // run the test
  run();

  // check log contents
  validate_log();

  // check the output
  MY_EXPECT_CMD_OUTPUT_CONTAINS(
      "WARNING: Found errors loading startup files, for more details look at "
      "the log at:");

  MY_EXPECT_CMD_OUTPUT_CONTAINS(expected_output());
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
  add_expected_js_log("Error loading JavaScript file");
  add_expected_js_log("plugin.js");
  add_expected_js_log("undefined_js_report is not defined");
  add_expected_py_log("Error loading Python file");
  add_expected_py_log("plugin.py");
  add_expected_py_log("name 'undefined_py_report' is not defined");

  // run the test
  run();
  // check the output
  MY_EXPECT_CMD_OUTPUT_CONTAINS(
      "WARNING: Found errors loading startup files, for more details look at "
      "the log at:");

  MY_EXPECT_CMD_OUTPUT_CONTAINS(expected_output());

  // check log contents
  validate_log();
}

TEST_F(Mysqlsh_reports_test, relative_require) {
  // write a simple report
  write_plugin("report.js", R"(
println('Hello from report!');
const mod = require('./dir/one.js');
mod.fun();
)");

  // create modules in a path relative to plugin
  const auto folder = join_path(get_plugin_folder(), "dir");
  shcore::create_directory(folder);
  shcore::create_file(join_path(folder, "one.js"), R"(
println('Hello from module one.js!');
const two = require('./two');
exports.fun = function() {
  println('Hello from fun(), module one.js!');
  two.fun();
}
)");
  shcore::create_file(join_path(folder, "two.js"), R"(
println('Hello from module two.js!');
exports.fun = function() {
  println('Hello from fun(), module two.js!');
}
)");

  // run the test
  run();

  // validate output
  MY_EXPECT_CMD_OUTPUT_CONTAINS(R"(
No default schema selected; type \use <schema> to set one.
Hello from report!
Hello from module one.js!
Hello from module two.js!
Hello from fun(), module one.js!
Hello from fun(), module two.js!
NOTE: MYSQLSH_PROMPT_THEME prompt theme file 'invalid' does not exist.
Bye!
)");
}

class Mysqlsh_plugin_test : public Mysqlsh_extension_test {
 public:
  std::string get_plugin_folder() const {
    return join_path(shcore::get_share_folder(), get_plugin_folder_name());
  }

  std::string get_user_plugin_folder() const {
    return join_path(shcore::get_user_config_path(), get_plugin_folder_name());
  }

  void write_plugin(const std::string &name, const std::string &contents,
                    const std::string &extension) {
    shcore::create_directory(join_path(get_plugin_folder(), name));
    shcore::create_file(
        join_path(get_plugin_folder(), name, "init" + extension), contents);
  }

  void write_user_plugin(const std::string &name, const std::string &contents,
                         const std::string &extension) {
    shcore::create_directory(join_path(get_user_plugin_folder(), name));
    shcore::create_file(
        join_path(get_user_plugin_folder(), name, "init" + extension),
        contents);
  }

  void delete_plugin(const std::string &name) {
    auto path = join_path(get_plugin_folder(), name);
    if (shcore::is_folder(path)) {
      shcore::remove_directory(path);
    } else {
      FAIL() << "Error deleting unexisting plugin: " << path.c_str()
             << std::endl;
    }
  }

  void delete_user_plugin(const std::string &name) {
    auto path = join_path(get_user_plugin_folder(), name);
    if (shcore::is_folder(path)) {
      shcore::remove_directory(path);
    } else {
      FAIL() << "Error deleting unexisting plugin: " << path.c_str()
             << std::endl;
    }
  }

 protected:
  std::string get_plugin_folder_name() const override { return "plugins"; }
};

TEST_F(Mysqlsh_plugin_test, WL13051_OK_shell_plugins) {
  if (g_test_parallel_execution) {
    SKIP_TEST("Skipping tests for parallel execution.");
  }

  // create first-js plugin, which defines a custom report
  write_plugin("first-js", R"(function report(s) {
  println('first JS report');
  return {'report' : []};
}

shell.registerReport('first_js', 'print', report);
)",
               ".js");

  // create third-py plugin, which defines a custom report
  write_plugin("third-py", R"(def report(s):
  print('third PY report');
  return {'report' : []};

shell.register_report('third_py', 'print', report);
)",
               ".py");

  // check if first_js report is available
  add_js_test("\\show first_js", "first JS report");
  add_py_test("\\py", "Switching to Python mode...");
  add_py_test("\\show third_py", "third PY report");

  add_expected_js_log(
      "The 'first_js' report of type 'print' has been registered.");
  add_expected_js_log(
      "The 'third_py' report of type 'print' has been registered.");
  run({"--log-level=debug"});
  // check the output
  MY_EXPECT_CMD_OUTPUT_CONTAINS(expected_output());
  validate_log();
  wipe_out();

  delete_plugin("first-js");
  delete_plugin("third-py");
}

TEST_F(Mysqlsh_plugin_test, WL13051_OK_user_plugins) {
  // create second-js plugin - which defines a new global object
  write_user_plugin("second-js", R"(function sample() {
  println('first object defined in JS');
}
var obj = shell.createExtensionObject();
shell.addExtensionObjectMember(obj, "testFunction", sample);
shell.registerGlobal('jsObject', obj);
)",
                    ".js");

  // create fourth-py plugin - which defines a new global object
  write_user_plugin("fourth-py", R"(def describe():
  print('first object defined in PY')

obj = shell.create_extension_object()
shell.add_extension_object_member(obj, "selfDescribe", describe);
shell.register_global('pyObject', obj);
)",
                    ".py");

  // This plugin is correctly defined except that the folder starts with . so it
  // will be ignored by the loader
  write_user_plugin(".git", R"(def describe():
  print('first object defined in PY')

obj = shell.create_extension_object()
shell.add_extension_object_member(obj, "selfDescribe", describe);
shell.register_global('pyObject', obj);
)",
                    ".py");

  add_expected_js_log(
      join_path(get_user_plugin_folder(), "second-js", "init.js"));
  add_expected_py_log(
      join_path(get_user_plugin_folder(), "fourth-py", "init.py"));
  add_unexpected_py_log(join_path(get_user_plugin_folder(), ".git", "init.py"));

  // check if jsObject.testFunction was properly registered in JS
  add_js_test("jsObject.testFunction()", "first object defined in JS");
  // check if jsObject.test_function was properly registered in PY
  add_py_test("\\py", "Switching to Python mode...");
  add_py_test("jsObject.test_function()", "first object defined in JS");
  // check if pyObject.self_describe was properly registered in PY
  add_py_test("pyObject.self_describe()", "first object defined in PY");
  // check if pyObject.selfDescribe was properly registered in JS
  add_js_test("\\js", "Switching to JavaScript mode...");
  add_js_test("pyObject.selfDescribe()", "first object defined in PY");
  // run the test
  run({"--log-level=debug"});

  // check the output
  MY_EXPECT_CMD_OUTPUT_CONTAINS(expected_output());
  wipe_out();

  validate_log();

  delete_user_plugin("second-js");
  delete_user_plugin("fourth-py");
  delete_user_plugin(".git");
}

TEST_F(Mysqlsh_plugin_test, WL13051_multiple_init_files) {
  // create first JS plugin file
  write_user_plugin("error-one", R"(function report(s) {
  println('first JS report');
  return {'report' : []};
}

shell.registerReport('first_js', 'print', report);
)",
                    ".js");

  write_user_plugin("error-one", R"(function report(s) {
  println('first JS report');
  return {'report' : []};
}

shell.registerReport('first_js', 'print', report);
)",
                    ".py");

  // check if first JS report is available

  add_js_test("\\show first_js", "Unknown report: first_js");

  add_expected_js_log(
      "Warning: Found multiple plugin initialization files for plugin "
      "'error-one'");

  // run the test
  run();
  // check the output
  MY_EXPECT_CMD_OUTPUT_CONTAINS(expected_output());
  MY_EXPECT_CMD_OUTPUT_CONTAINS(
      "WARNING: Found multiple plugin initialization files for plugin "
      "'error-one'");
  wipe_out();

  validate_log();

  delete_user_plugin("error-one");
}

TEST_F(Mysqlsh_plugin_test, WL13051_no_init_files) {
  // Create folder in the plugins directory
  shcore::create_directory(
      join_path(get_user_plugin_folder(), "invalid-plugin"));
  shcore::create_directory(join_path(get_user_plugin_folder(), ".git"));

  add_expected_py_log(
      "Missing initialization file for plugin 'invalid-plugin'");
  add_unexpected_py_log("Missing initialization file for plugin '.git'");

  // run the test
  run({"--log-level=debug"});
  // check the output
  MY_EXPECT_CMD_OUTPUT_CONTAINS(expected_output());
  MY_EXPECT_CMD_OUTPUT_NOT_CONTAINS(
      "WARNING: Missing initialization file for plugin "
      "'invalid-plugin'");

  validate_log();

  delete_user_plugin("invalid-plugin");
  delete_user_plugin(".git");
}

TEST_F(Mysqlsh_plugin_test, WL13051_errors_in_js_plugin) {
  // create first JS plugin file
  write_user_plugin("error-two", R"(function report(s) {
  println('first JS report';  // <-- The error
  return {'report' : []};
}

shell.registerReport('first_js', 'print', report);
)",
                    ".js");

  add_expected_js_log("Error loading JavaScript file");

  // run the test
  run();
  // check the output
  MY_EXPECT_CMD_OUTPUT_CONTAINS(
      "WARNING: Found errors loading plugins, for more details look at "
      "the log at:");
  wipe_out();

  validate_log();

  delete_user_plugin("error-two");
}

TEST_F(Mysqlsh_plugin_test, WL13051_errors_in_py_plugin) {
  // create first JS plugin file
  write_user_plugin("error-two", R"(def report(s):
  print ("first PY report"  // <-- The error
  return {"report" : []};
}

shell.register_report('first_py', 'print', report);
)",
                    ".py");

  add_expected_py_log("Error loading Python file");

  // run the test
  run();
  // check the output
  MY_EXPECT_CMD_OUTPUT_CONTAINS(
      "WARNING: Found errors loading plugins, for more details look at "
      "the log at:");
  wipe_out();

  validate_log();

  delete_user_plugin("error-two");
}

// This test emulates a plugin that has the function definition
// in subfolders, while only the function registration resides
// in the init.py file
TEST_F(Mysqlsh_plugin_test, py_plugin_with_imports) {
  // Creates complex_py plugin initialization file
  write_user_plugin("complex_py", R"(
print("Plugin File Path: {0}".format(__file__))

# loads a function definition from src package
from complex_py.src import definition

# loads a sibling script
from complex_py import sibling


# Executes the plugin registration
obj = shell.create_extension_object()
shell.add_extension_object_member(obj, "test_package", definition.my_function);
shell.add_extension_object_member(obj, "test_sibling", sibling.my_function);
shell.register_global('pyObject', obj);
)",
                    ".py");

  auto user_plugins = get_user_plugin_folder();
  auto main_package = join_path(user_plugins, "complex_py");
  auto src_package = join_path(main_package, "src");

  // Creates __init__.py to make the main plugin a package
  shcore::create_file(join_path(main_package, "__init__.py"), "");

  // Creates a sibling script with a function definition
  shcore::create_file(join_path(main_package, "sibling.py"),
                      R"(def my_function():
  print("Function at complex_py/sibling.py"))");

  // Creates the src sub-package with a function definition
  shcore::create_directory(src_package);
  shcore::create_file(join_path(src_package, "__init__.py"), "");
  shcore::create_file(join_path(src_package, "definition.py"),
                      R"(def my_function():
  print("Function at complex_py/src/definition.py"))");

  add_py_test("pyObject.test_package()",
              "Function at complex_py/src/definition.py");

  add_py_test("pyObject.test_sibling()", "Function at complex_py/sibling.py");

  add_expected_py_log(
      "The 'test_package' function has been registered into an unregistered "
      "extension object.");

  add_expected_py_log(
      "The 'test_sibling' function has been registered into an unregistered "
      "extension object.");

  add_expected_py_log(
      "The 'pyObject' extension object has been registered as a global "
      "object.");

  // run the test
  run({"--log-level=debug"});
  // check the output
  MY_EXPECT_CMD_OUTPUT_NOT_CONTAINS("WARNING: Found errors loading plugins");
  std::string expected("Plugin File Path: ");
  expected += join_path(get_user_plugin_folder(), "complex_py", "init.py");
  MY_EXPECT_CMD_OUTPUT_CONTAINS(expected.c_str());
  MY_EXPECT_CMD_OUTPUT_CONTAINS(expected_output().c_str());

  validate_log();

  wipe_out();

  delete_user_plugin("complex_py");
}

// This test emulates a folder in plugins that contains subfolders
// with plugins
TEST_F(Mysqlsh_plugin_test, py_multi_plugins) {
  // Creates multi_plugins plugin initialization file
  auto user_plugins = get_user_plugin_folder();
  auto main_package = join_path(user_plugins, "multi_plugins");

  // Creates __init__.py to make the main plugin a package
  shcore::create_directory(main_package);
  shcore::create_file(join_path(main_package, "__init__.py"), "");

  shcore::create_file(join_path(main_package, "plugin_common.py"),
                      R"(import mysqlsh

def global_function(caller):
  print("Global function called from {0}".format(caller)))");

  auto create_plugin = [main_package](const std::string &name,
                                      const std::string &greeting) {
    auto plugin = join_path(main_package, name);
    shcore::create_directory(plugin);
    std::string code =
        R"(from multi_plugins import plugin_common

# The implementation of the function to be added to demo
def hello_function():
  print("Hello World From {0}".format("%s"))

def global_function():
  plugin_common.global_function("%s")


# Ensures the demo objects is created if not exists already
if 'demo' in globals():
    global_obj = demo
else:
    global_obj = shell.create_extension_object()
    shell.register_global("demo", global_obj)

# Registers the function
shell.add_extension_object_member(global_obj, "hello_%s", hello_function)
shell.add_extension_object_member(global_obj, "common_%s", global_function)
)";

    shcore::create_file(
        join_path(plugin, "init.py"),
        shcore::str_format(code.c_str(), greeting.c_str(), greeting.c_str(),
                           name.c_str(), name.c_str()));
  };

  // Creates the several plugins
  create_plugin("mx", "Mexico");
  create_plugin("pl", "Poland");
  create_plugin("pt", "Portugal");
  create_plugin("us", "United States");
  create_plugin("at", "Austria");

  add_py_test("demo.hello_at()", "Hello World From Austria");
  add_py_test("demo.hello_mx()", "Hello World From Mexico");
  add_py_test("demo.hello_pl()", "Hello World From Poland");
  add_py_test("demo.hello_pt()", "Hello World From Portugal");
  add_py_test("demo.hello_us()", "Hello World From United States");

  add_py_test("demo.common_at()", "Global function called from Austria");
  add_py_test("demo.common_mx()", "Global function called from Mexico");
  add_py_test("demo.common_pl()", "Global function called from Poland");
  add_py_test("demo.common_pt()", "Global function called from Portugal");
  add_py_test("demo.common_us()", "Global function called from United States");

  add_expected_py_log(
      "The 'demo' extension object has been registered as a global object.");
  add_expected_py_log(
      "The 'hello_mx' function has been registered into the 'demo' extension "
      "object.");
  add_expected_py_log(
      "The 'hello_pl' function has been registered into the 'demo' extension "
      "object.");
  add_expected_py_log(
      "The 'hello_pt' function has been registered into the 'demo' extension "
      "object.");
  add_expected_py_log(
      "The 'hello_us' function has been registered into the 'demo' extension "
      "object.");
  add_expected_py_log(
      "The 'hello_at' function has been registered into the 'demo' extension "
      "object.");

  add_expected_py_log(
      "The 'common_mx' function has been registered into the 'demo' extension "
      "object.");
  add_expected_py_log(
      "The 'common_pl' function has been registered into the 'demo' extension "
      "object.");
  add_expected_py_log(
      "The 'common_pt' function has been registered into the 'demo' extension "
      "object.");
  add_expected_py_log(
      "The 'common_us' function has been registered into the 'demo' extension "
      "object.");
  add_expected_py_log(
      "The 'common_at' function has been registered into the 'demo' extension "
      "object.");
  // run the test
  run({"--log-level=debug"});
  // check the output
  MY_EXPECT_CMD_OUTPUT_CONTAINS(expected_output().c_str());
  MY_EXPECT_CMD_OUTPUT_NOT_CONTAINS("WARNING: Found errors loading plugins");

  validate_log();

  wipe_out();

  delete_user_plugin("multi_plugins");
}

#ifdef WITH_OCI
// This test emulates a plugins that uses the oci
TEST_F(Mysqlsh_plugin_test, oci_and_paramiko_plugin) {
  // Creates oci-paramiko plugin initialization file
  write_user_plugin("oci-paramiko", R"(import oci
import paramiko

print("OCI Version: {0}".format(oci.__version__))
print("Paramiko Version: {0}".format(paramiko.__version__))
)",
                    ".py");

  // run the test
  run();
  // check the output
  MY_EXPECT_CMD_OUTPUT_NOT_CONTAINS("WARNING: Found errors loading plugins");
  // NOTE: it is not important to check the OCI version, but just make sure that
  // there were no failures
  MY_EXPECT_CMD_OUTPUT_NOT_CONTAINS("No module named 'oci'");
  MY_EXPECT_CMD_OUTPUT_NOT_CONTAINS("No module named 'paramiko'");
  MY_EXPECT_CMD_OUTPUT_CONTAINS("OCI Version: ");
  MY_EXPECT_CMD_OUTPUT_CONTAINS("Paramiko Version: ");

  wipe_out();

  delete_user_plugin("oci-paramiko");
}
#endif

TEST_F(Mysqlsh_plugin_test, relative_require) {
  // write a simple plugin
  write_user_plugin("plugin", R"(
println('Hello from plugin!');
const mod = require('./dir/one.js');
mod.fun();
)",
                    ".js");

  // create modules in a path relative to plugin
  const auto folder = join_path(get_user_plugin_folder(), "plugin", "dir");
  shcore::create_directory(folder);
  shcore::create_file(join_path(folder, "one.js"), R"(
println('Hello from module one.js!');
const two = require('./two');
exports.fun = function() {
  println('Hello from fun(), module one.js!');
  two.fun();
}
)");
  shcore::create_file(join_path(folder, "two.js"), R"(
println('Hello from module two.js!');
exports.fun = function() {
  println('Hello from fun(), module two.js!');
}
)");

  // run the test
  run();

  // validate output
  MY_EXPECT_CMD_OUTPUT_CONTAINS(R"(
No default schema selected; type \use <schema> to set one.
Hello from plugin!
Hello from module one.js!
Hello from module two.js!
Hello from fun(), module one.js!
Hello from fun(), module two.js!
NOTE: MYSQLSH_PROMPT_THEME prompt theme file 'invalid' does not exist.
Bye!
)");

  // cleanup
  delete_user_plugin("plugin");
}

}  // namespace tests
