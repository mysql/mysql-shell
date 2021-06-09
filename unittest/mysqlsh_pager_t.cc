/*
 * Copyright (c) 2018, 2021, Oracle and/or its affiliates.
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

#include "unittest/gtest_clean.h"

#include "unittest/test_utils.h"
#include "unittest/test_utils/command_line_test.h"
#include "unittest/test_utils/mocks/gmock_clean.h"

#include "mysqlshdk/libs/utils/utils_file.h"
#include "mysqlshdk/libs/utils/utils_path.h"

namespace tests {

#ifdef HAVE_V8

#define SCRIPTING_LANGUAGE "JavaScript"
#define SCRIPTING_LANGUAGE_SHORT "js"
#define PRINT "println"
#define SET_PERSIST "setPersist"
#define UNSET_PERSIST "unsetPersist"
#define ENABLE_PAGER "enablePager"
#define DISABLE_PAGER "disablePager"

#elif defined HAVE_PYTHON

#define SCRIPTING_LANGUAGE "Python"
#define SCRIPTING_LANGUAGE_SHORT "py"
#define PRINT "print"
#define SET_PERSIST "set_persist"
#define UNSET_PERSIST "unset_persist"
#define ENABLE_PAGER "enable_pager"
#define DISABLE_PAGER "disable_pager"

#else

#error No scripting mode available

#endif

#define SCRIPTING_MODE "--" SCRIPTING_LANGUAGE_SHORT
#define CHANGE_MODE "\\" SCRIPTING_LANGUAGE_SHORT

class Mysqlsh_pager_test : public Command_line_test {
 protected:
  void SetUp() override {
    Command_line_test::SetUp();

    cleanup();
    shcore::setenv("MYSQLSH_PROMPT_THEME", "invalid");
    shcore::setenv("MYSQLSH_TERM_COLOR_MODE", "nocolor");
  }

  void TearDown() override {
    Command_line_test::TearDown();

    cleanup();
  }

 private:
  void cleanup() {
    shcore::unsetenv("PAGER");
    shcore::unsetenv("MYSQLSH_PROMPT_THEME");
    shcore::unsetenv("MYSQLSH_TERM_COLOR_MODE");
    execute({_mysqlsh, SCRIPTING_MODE, "-e",
             "shell.options." UNSET_PERSIST "('pager')", nullptr});
    wipe_out();
  }
};

class Pager_initialization_test : public Mysqlsh_pager_test {
 public:
  static void SetUpTestCase() {
    // test sequence used by all the tests
    shcore::create_file(k_file,
                        // display the initial value
                        PRINT
                        "(shell.options.pager);\n"
                        // change pager to 'one'
                        "\\pager one\n"
                        // restore the initial value
                        "\\pager\n"
                        // change pager to 'two'
                        "\\P two\n"
                        // set pager to the default value (empty string)
                        "\\nopager\n");
  }

  static void TearDownTestCase() { shcore::delete_file(k_file); }

 protected:
  void run(bool with_pager = false) {
    std::vector<const char *> args = {_mysqlsh, SCRIPTING_MODE,
                                      "--interactive"};

    if (with_pager) {
      args.emplace_back(k_cmdline_pager);
    }

    args.emplace_back(nullptr);

    execute(args, nullptr, k_file);
  }

  void set_environment_variable() { shcore::setenv("PAGER", "env"); }

  void persist_pager() {
    execute({_mysqlsh, SCRIPTING_MODE, "-e",
             "shell.options." SET_PERSIST "('pager', 'persist');", nullptr});
    wipe_out();
  }

 private:
  static const char k_file[];
  static constexpr const char *const k_cmdline_pager = "--pager=cmd";
};

const char Pager_initialization_test::k_file[] = "pager_initialization_test";
constexpr const char *const Pager_initialization_test::k_cmdline_pager;

TEST_F(Pager_initialization_test, default_pager) {
  static constexpr auto expected_output = R"(
Pager has been set to 'one'.
Pager has been disabled.
Pager has been set to 'two'.
Pager has been disabled.
Bye!)";

  run();
  MY_EXPECT_CMD_OUTPUT_CONTAINS(expected_output);
  wipe_out();
}

TEST_F(Pager_initialization_test, env) {
  static constexpr auto expected_output = R"(env
Pager has been set to 'one'.
Pager has been set to 'env'.
Pager has been set to 'two'.
Pager has been disabled.
Bye!)";

  // WL10755_TS6_1
  // Create and/or set the PAGER OS environment variable. Then starts MySQL
  // Shell, and verify that the shell.options["pager"] option has the same value
  // that the PAGER environment variable.

  // Set the PAGER environment variable.
  set_environment_variable();
  // Run MySQL Shell.
  run();
  // The initial value of shell.options["pager"] option must be equal to the
  // PAGER environment variable (first "env" in the expected_output).
  MY_EXPECT_CMD_OUTPUT_CONTAINS(expected_output);
  wipe_out();
}

TEST_F(Pager_initialization_test, persist) {
  static constexpr auto expected_output = R"(persist
Pager has been set to 'one'.
Pager has been set to 'persist'.
Pager has been set to 'two'.
Pager has been disabled.
Bye!)";

  // WL10755_TS7_3
  // Having an already value configured for the shell.options["pager"] option,
  // change its value with the \pager or \P command. Then call the command again
  // passing an empty string or with no arguments, validate that the
  // shell.options["pager"] option has the initial value before the changes.

  // Persist the shell.options["pager"] option.
  persist_pager();
  // Run MySQL Shell.
  run();
  // The initial value of shell.options["pager"] option must be equal to the
  // persisted value (first "persist" in the expected_output).

  // Change the initial value using the \pager command ("one" in the
  // expected_output).

  // Restore the initial value using \pager command with no arguments. The
  // shell.options["pager"] option must be equal to the initial value (second
  // "persist" in the expected_output).
  MY_EXPECT_CMD_OUTPUT_CONTAINS(expected_output);
  wipe_out();
}

TEST_F(Pager_initialization_test, cmdline) {
  static constexpr auto expected_output = R"(cmd
Pager has been set to 'one'.
Pager has been set to 'cmd'.
Pager has been set to 'two'.
Pager has been disabled.
Bye!)";

  // WL10755_TS8_2
  // Verify that the use of the \nopager command set the shell.options["pager"]
  // option to an empty string.

  // Run MySQL Shell with --pager option.
  run(true);
  // The initial value of shell.options["pager"] option must be equal to the
  // command line argument (first "cmd" in the expected_output).

  // Change the initial value using the \pager command ("one" in the
  // expected_output).

  // Restore the initial value using \pager command with no arguments. The
  // shell.options["pager"] option must be equal to the initial value (second
  // "cmd" in the expected_output).

  // Set pager to another value using \P command ("two" in the expected_value).

  // Set shell.options["pager"] option to the default value using \nopager
  // command. Default value must be equal to an empty string (empty line in the
  // expected_output).
  MY_EXPECT_CMD_OUTPUT_CONTAINS(expected_output);
  wipe_out();
}

TEST_F(Pager_initialization_test, env_persist) {
  static constexpr auto expected_output = R"(persist
Pager has been set to 'one'.
Pager has been set to 'persist'.
Pager has been set to 'two'.
Pager has been disabled.
Bye!)";

  // WL10755_TS6_2
  // Verify that setting a value to the shell.options["pager"] option using
  // shell.setPersist takes precedence over the PAGER OS environment variable.

  // Set the PAGER environment variable.
  set_environment_variable();
  // Persist the shell.options["pager"] option.
  persist_pager();
  // Run MySQL Shell.
  run();
  // The initial value of shell.options["pager"] option must be equal to the
  // persisted value (first "persist" in the expected_output).
  MY_EXPECT_CMD_OUTPUT_CONTAINS(expected_output);
  wipe_out();
}

TEST_F(Pager_initialization_test, env_cmdline) {
  static constexpr auto expected_output = R"(cmd
Pager has been set to 'one'.
Pager has been set to 'cmd'.
Pager has been set to 'two'.
Pager has been disabled.
Bye!)";

  set_environment_variable();
  run(true);
  MY_EXPECT_CMD_OUTPUT_CONTAINS(expected_output);
  wipe_out();
}

TEST_F(Pager_initialization_test, persist_cmdline) {
  static constexpr auto expected_output = R"(cmd
Pager has been set to 'one'.
Pager has been set to 'cmd'.
Pager has been set to 'two'.
Pager has been disabled.
Bye!)";

  persist_pager();
  run(true);
  MY_EXPECT_CMD_OUTPUT_CONTAINS(expected_output);
  wipe_out();
}

TEST_F(Pager_initialization_test, env_persist_cmdline) {
  static constexpr auto expected_output = R"(cmd
Pager has been set to 'one'.
Pager has been set to 'cmd'.
Pager has been set to 'two'.
Pager has been disabled.
Bye!)";

  // WL10755_TS9_2
  // Validate that the use of --pager option takes precedence over a persisted
  // value for the shell.options["pager"] option and a value set to the PAGER OS
  // environment variable.

  // Set the PAGER environment variable.
  set_environment_variable();
  // Persist the shell.options["pager"] option.
  persist_pager();
  // Run MySQL Shell with --pager option.
  run(true);
  // The initial value of shell.options["pager"] option must be equal to the
  // command line argument (first "cmd" in the expected_output).
  MY_EXPECT_CMD_OUTPUT_CONTAINS(expected_output);
  wipe_out();
}

class Pager_script_test : public Mysqlsh_pager_test {
 public:
  static void SetUpTestCase() {
    static const auto tmp_dir = getenv("TMPDIR");
    static constexpr auto pager_file = "pager.txt";
    s_output_file = shcore::path::join_path(tmp_dir, pager_file);

    static const auto binary_folder = shcore::get_binary_folder();
    static constexpr auto sample_pager = "sample_pager";
    s_pager_binary = shcore::path::join_path(binary_folder, sample_pager);

    s_pager_command = s_pager_binary + " --file=" + s_output_file;
  }

 protected:
  static std::string default_pager() { return s_pager_command; }

  void SetUp() override {
    Mysqlsh_pager_test::SetUp();

    set_pager(default_pager());
  }

  void TearDown() override {
    Mysqlsh_pager_test::TearDown();

    shcore::delete_file(s_output_file);
    shcore::delete_file(k_script_file);
  }

  void set_command(const std::string &s) { m_command = s; }

  void set_script(const std::string &s) {
    shcore::create_file(k_script_file, s);
    m_has_script = true;
  }

  void set_pager(const std::string &s) { m_current_pager = s; }

  void run(bool interactive, const std::vector<std::string> &extra = {}) {
    ASSERT_NE(m_has_script, !m_command.empty())
        << "Either script or command needs to be set.";

    std::vector<const char *> args = {_mysqlsh, "--pager",
                                      m_current_pager.c_str()};

    for (const auto &a : extra) {
      args.emplace_back(a.c_str());
    }

    if (interactive) {
      args.emplace_back("--interactive=full");
    }

    if (!m_command.empty()) {
      args.emplace_back("--execute");
      args.emplace_back(m_command.c_str());
    }

    if (m_has_script) {
      args.emplace_back("--file");
      args.emplace_back(k_script_file);
    }

    args.emplace_back(nullptr);

    execute(args);
  }

  std::string get_pager_output() {
    return shcore::get_text_file(s_output_file);
  }

 private:
  static constexpr const char *const k_script_file = "pager_script";
  static std::string s_output_file;
  static std::string s_pager_binary;
  static std::string s_pager_command;

  bool m_has_script = false;
  std::string m_command;
  std::string m_current_pager;
};

constexpr const char *const Pager_script_test::k_script_file;
std::string Pager_script_test::s_output_file;
std::string Pager_script_test::s_pager_binary;
std::string Pager_script_test::s_pager_command;

TEST_F(Pager_script_test, WL10755_TS11_1_1) {
  static constexpr auto expected_mysqlsh_output = R"(mysql-sql []> SELECT 1;
mysql-sql []> )";
  static constexpr auto expected_pager_output = R"(+---+
| 1 |
+---+
| 1 |
+---+
1 row in set ()";

  // WL10755_TS11_1
  // Execute commands and scripts using the options --file and --execute, and
  // using --interactive=full (-i=full). Pager must receive the output of
  // applicable commands.

  // Set command to be executed using --execute option.
  set_command("SELECT 1;");
  // Run MySQL Shell in full interactive mode.
  run(true, {"--sql", "--uri", _uri});
  // Verify that running the specified command did not produce output.
  MY_EXPECT_CMD_OUTPUT_CONTAINS(expected_mysqlsh_output);
  // Verify that pager received the output of executed command.
  EXPECT_THAT(get_pager_output(), ::testing::HasSubstr(expected_pager_output));
  wipe_out();
}

TEST_F(Pager_script_test, WL10755_TS11_1_2) {
  static constexpr auto expected_mysqlsh_output = R"(mysql-sql []> SELECT 1;
mysql-sql []> )";
  static constexpr auto expected_pager_output = R"(+---+
| 1 |
+---+
| 1 |
+---+
1 row in set ()";

  // WL10755_TS11_1
  // Execute commands and scripts using the options --file and --execute, and
  // using --interactive=full (-i=full). Pager must receive the output of
  // applicable commands.

  // Set script to be executed using --file option.
  set_script("SELECT 1;");
  // Run MySQL Shell in full interactive mode.
  run(true, {"--sql", "--uri", _uri});
  // Verify that running the specified script did not produce output.
  MY_EXPECT_CMD_OUTPUT_CONTAINS(expected_mysqlsh_output);
  // Verify that pager received the output of executed script.
  EXPECT_THAT(get_pager_output(), ::testing::HasSubstr(expected_pager_output));
  wipe_out();
}

TEST_F(Pager_script_test, WL10755_TS11_2_1) {
  static constexpr auto expected_mysqlsh_output =
      R"(*************************** 1. row ***************************
1: 1)";

  // WL10755_TS11_2
  // Execute commands and scripts using the options --file and --execute, and
  // not using --interactive (-i) or --interactive=full (-i=full). Pager must
  // not receive the output of applicable commands.

  // Set command to be executed using --execute option.
  set_command("SELECT 1;");
  // Run MySQL Shell in non-interactive mode.
  run(false, {"--sql", "--vertical", "--uri", _uri});
  // Verify that running the specified command produced output.
  MY_EXPECT_CMD_OUTPUT_CONTAINS(expected_mysqlsh_output);
  // Verify that pager did not receive the output of executed command.
  EXPECT_THROW(get_pager_output(), std::runtime_error);
  wipe_out();
}

TEST_F(Pager_script_test, WL10755_TS11_2_2) {
  static constexpr auto expected_mysqlsh_output =
      R"(*************************** 1. row ***************************
1: 1)";

  // WL10755_TS11_2
  // Execute commands and scripts using the options --file and --execute, and
  // not using --interactive (-i) or --interactive=full (-i=full). Pager must
  // not receive the output of applicable commands.

  // Set script to be executed using --file option.
  set_script("SELECT 1;");
  // Run MySQL Shell in non-interactive mode.
  run(false, {"--sql", "--vertical", "--uri", _uri});
  // Verify that running the specified script produced output.
  MY_EXPECT_CMD_OUTPUT_CONTAINS(expected_mysqlsh_output);
  // Verify that pager did not receive the output of executed script.
  EXPECT_THROW(get_pager_output(), std::runtime_error);
  wipe_out();
}

TEST_F(Pager_script_test, WL10755_TS11_3_1) {
  static constexpr auto expected_mysqlsh_output = R"(begin
Switching to SQL mode... Commands end with ;
Switching to %s mode...
end)";
  static constexpr auto expected_pager_output = R"(+---+
| 1 |
+---+
| 1 |
+---+
1 row in set ()";

  // WL10755_TS11_3
  // Execute commands and scripts using the options --file and --execute, and
  // using --interactive (-i). Pager must receive the output of applicable
  // commands.

  // Set command to be executed using --execute option.
  set_command(PRINT
              "('begin');\n"
              "\\sql\n"
              "SELECT 1;\n" CHANGE_MODE "\n" PRINT "('end');\n");
  // Run MySQL Shell in interactive mode.
  run(false, {"--interactive", SCRIPTING_MODE, "--uri", _uri});
  // Verify that running the specified command did not produce output.
  MY_EXPECT_CMD_OUTPUT_CONTAINS(
      shcore::str_format(expected_mysqlsh_output, SCRIPTING_LANGUAGE));
  // Verify that pager received the output of executed command.
  EXPECT_THAT(get_pager_output(), ::testing::HasSubstr(expected_pager_output));
  wipe_out();
}

TEST_F(Pager_script_test, WL10755_TS11_3_2) {
  static constexpr auto expected_mysqlsh_output = R"(begin
Switching to SQL mode... Commands end with ;
Switching to %s mode...
end)";
  static constexpr auto expected_pager_output = R"(+---+
| 1 |
+---+
| 1 |
+---+
1 row in set ()";

  // WL10755_TS11_3
  // Execute commands and scripts using the options --file and --execute, and
  // using --interactive (-i). Pager must receive the output of applicable
  // commands.

  // Set script to be executed using --file option.
  set_script(PRINT
             "('begin');\n"
             "\\sql\n"
             "SELECT 1;\n" CHANGE_MODE "\n" PRINT "('end');\n");
  // Run MySQL Shell in interactive mode.
  run(false, {"--interactive", SCRIPTING_MODE, "--uri", _uri});
  // Verify that running the specified script did not produce output.
  MY_EXPECT_CMD_OUTPUT_CONTAINS(
      shcore::str_format(expected_mysqlsh_output, SCRIPTING_LANGUAGE));
  // Verify that pager received the output of executed script.
  EXPECT_THAT(get_pager_output(), ::testing::HasSubstr(expected_pager_output));
  wipe_out();
}

TEST_F(Pager_script_test, WL10755_TS12_2) {
  static constexpr auto expected_mysqlsh_output =
      "sh: invalid_command: command not found";
  static constexpr auto fedora_specific_output =
      "sh: line 1: invalid_command: command not found";
  static constexpr auto platform_specific_output =
#if defined(_WIN32)
      R"('invalid_command' is not recognized as an internal or external command,
operable program or batch file.)";
#elif defined(__sun)
      "sh: invalid_command: not found";  // output on Solaris
#else
      "sh: 1: invalid_command: not found";  // output on Debian
#endif  // !_WIN32

  // WL10755_TS12_2
  // Set the shell.options["pager"] to an invalid external command and perform
  // an action which should use the Pager. MySQL Shell must display the external
  // error(s).

  // Set command to be executed using --execute option.
  set_command("SELECT 1;");
  // Set pager to an invalid command.
  set_pager("invalid_command");
  // Run MySQL Shell in full interactive mode.
  run(true, {"--sql", "--uri", _uri});
  // Verify that running the specified command reported an error.
  EXPECT_THAT(_output,
              ::testing::AnyOf(::testing::HasSubstr(expected_mysqlsh_output),
                               ::testing::HasSubstr(platform_specific_output),
                               ::testing::HasSubstr(fedora_specific_output)));
  wipe_out();
}

TEST_F(Pager_script_test, WL10755_TS13_5) {
  static constexpr auto expected_mysqlsh_output =
      R"(1111
2222
3333)";

  // WL10755_TS13_5
  // Run a script that contains the use of shell.enablePager() and
  // shell.disablePager(), not set -i or -i=full. The pager must not be used to
  // print the output.

  // Set script to be executed using --file option.
  set_script(PRINT
             "('1111');\n"
             "shell." ENABLE_PAGER "();\n" PRINT
             "('2222');\n"
             "shell." DISABLE_PAGER "();\n" PRINT "('3333');\n");
  // Run MySQL Shell in non-interactive mode.
  run(false, {SCRIPTING_MODE});
  // Verify that running the specified script produced output.
  MY_EXPECT_CMD_OUTPUT_CONTAINS(expected_mysqlsh_output);
  // Verify that pager did not receive the output of executed script.
  EXPECT_THROW(get_pager_output(), std::runtime_error);
  wipe_out();
}

TEST_F(Pager_script_test, WL10755_TS13_6) {
  static constexpr auto expected_mysqlsh_output =
      R"(1111
2222)";

  // WL10755_TS13_6
  // Run a script that contains the use of shell.disablePager(), not set -i or
  // -i=full. The method must have no effect in the execution.

  // Set script to be executed using --file option.
  set_script(PRINT
             "('1111');\n"
             "shell." DISABLE_PAGER "();\n" PRINT "('2222');\n");
  // Run MySQL Shell in non-interactive mode.
  run(false, {SCRIPTING_MODE});
  // Verify that running the specified script produced output.
  MY_EXPECT_CMD_OUTPUT_CONTAINS(expected_mysqlsh_output);
  // Verify that pager did not receive the output of executed script.
  EXPECT_THROW(get_pager_output(), std::runtime_error);
  wipe_out();
}

}  // namespace tests
