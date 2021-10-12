/*
 * Copyright (c) 2017, 2021, Oracle and/or its affiliates.
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

#include "unittest/gprod_clean.h"
#include "unittest/gtest_clean.h"
#include "unittest/test_utils/mocks/gmock_clean.h"

#ifndef _WIN32
#include <sys/stat.h>
#endif
#include "ext/linenoise-ng/include/linenoise.h"
#include "modules/mod_shell.h"
#include "modules/mod_shell_options.h"
#include "mysqlshdk/libs/utils/utils_file.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_string.h"
#include "src/mysqlsh/cmdline_shell.h"
#include "unittest/test_utils.h"

namespace mysqlsh {

class Shell_history : public ::testing::Test {
 protected:
#ifdef HAVE_V8
  const std::string to_scripting = "\\js";
#else
  const std::string to_scripting = "\\py";
#endif

 public:
  Shell_history()
      : _options_file(
            Shell_core_test_wrapper::get_options_file_name("history_test")),
        m_handler(&m_capture, print_capture, print_capture, print_capture) {}

  void SetUp() override {
    remove(_options_file.c_str());
    linenoiseHistoryFree();
  }

 protected:
  void enable_capture() { current_console()->add_print_handler(&m_handler); }

  void disable_capture() {
    current_console()->remove_print_handler(&m_handler);
  }

  std::string _options_file;

  std::string m_capture;

  shcore::Interpreter_print_handler m_handler;

 private:
  static bool print_capture(void *cdata, const char *text) {
    std::string *m_capture = static_cast<std::string *>(cdata);
    m_capture->append(text).append("\n");
    return true;
  }
};

TEST_F(Shell_history, check_history_sql_not_connected) {
  // Start in SQL mode, do not connect to MySQL. This will ensure no statement
  // gets executed. However, we promise that all user input will be recorded
  // in the history - no matther what all user input shall be recorded.
  char *args[] = {const_cast<char *>("ut"), const_cast<char *>("--sql"),
                  nullptr};
  mysqlsh::Command_line_shell shell(
      std::make_shared<Shell_options>(2, args, _options_file));

  EXPECT_EQ(0, linenoiseHistorySize());

  shell.process_line("select 1;");

  EXPECT_EQ(1, linenoiseHistorySize());
  EXPECT_STREQ("select 1;", linenoiseHistoryLine(0));

  SKIP_TEST("Bug in history handling of empty commands");
  // Bug
  shell.process_line("select 2;;");
  EXPECT_EQ(2, linenoiseHistorySize());
  EXPECT_STREQ("select 1;", linenoiseHistoryLine(0));
  EXPECT_STREQ("select 2;", linenoiseHistoryLine(1));
}

TEST_F(Shell_history, check_password_history_linenoise) {
  // TS_HM#6 in SQL mode, commands that match the glob patterns IDENTIFIED,
  // PASSWORD or any pattern specified in the
  // shell.options["history.sql.ignorePattern"] option will be skipped from
  // being added to the history.
  // TS_CV#5 shell.options["history.sql.ignorePattern"]=string skip string from
  // history.

  const auto &server_uri = shell_test_server_uri();
  char *args[] = {const_cast<char *>("ut"),
                  const_cast<char *>(server_uri.c_str()), nullptr};
  mysqlsh::Command_line_shell shell(
      std::make_shared<Shell_options>(2, args, _options_file));
  shell._history.set_limit(100);

  auto coptions = shcore::get_connection_options("root@localhost");
  coptions.set_scheme("mysql");
  coptions.set_password("");
  coptions.set_port(atoi(getenv("MYSQL_PORT")));
  shell.connect(coptions, false);

  EXPECT_EQ("*IDENTIFIED*:*PASSWORD*",
            shell.get_options()->get("history.sql.ignorePattern").descr());

  EXPECT_EQ(0, linenoiseHistorySize());

  // \sql command should be filtered according to SQL mode rules
  EXPECT_EQ(0, linenoiseHistorySize());

  shell.process_line("\\sql select 1;");

  EXPECT_EQ(1, linenoiseHistorySize());
  EXPECT_STREQ("\\sql select 1;", linenoiseHistoryLine(0));

  // ensure certain words don't appear in history for more than 1 iteration
  shell.process_line("\\sql set password = 'secret' then fail;");
  EXPECT_EQ(2, linenoiseHistorySize());
  EXPECT_STREQ("\\sql select 1;", linenoiseHistoryLine(0));
  // 1st time added it should be there, but not after another one is added
  EXPECT_STREQ("\\sql set password = 'secret' then fail;",
               linenoiseHistoryLine(1));

  shell.process_line("\\sql create user foo@bar identified by 'secret';");
  EXPECT_EQ(2, linenoiseHistorySize());
  EXPECT_STREQ("\\sql select 1;", linenoiseHistoryLine(0));
  EXPECT_STREQ("\\sql create user foo@bar identified by 'secret';",
               linenoiseHistoryLine(1));

  shell.process_line("\\sql alter user foo@bar set password='secret';");
  EXPECT_EQ(2, linenoiseHistorySize());
  EXPECT_STREQ("\\sql select 1;", linenoiseHistoryLine(0));
  EXPECT_STREQ("\\sql alter user foo@bar set password='secret';",
               linenoiseHistoryLine(1));

  shell.process_line("\\sql secret;");
  EXPECT_EQ(2, linenoiseHistorySize());
  EXPECT_STREQ("\\sql select 1;", linenoiseHistoryLine(0));
  EXPECT_STREQ("\\sql secret;", linenoiseHistoryLine(1));
  shell.process_line("\\sql drop user foo@bar;");

  // TS_CV#9
  shell.process_line("\\sql");
  EXPECT_EQ(0, linenoiseHistorySize());

  shell.process_line("select 1;");

  EXPECT_EQ(1, linenoiseHistorySize());
  EXPECT_STREQ("select 1;", linenoiseHistoryLine(0));

  // ensure certain words don't appear in history for more than 1 iteration
  shell.process_line("set password = 'secret' then fail;");
  EXPECT_EQ(2, linenoiseHistorySize());
  EXPECT_STREQ("select 1;", linenoiseHistoryLine(0));
  // 1st time added it should be there, but not after another one is added
  EXPECT_STREQ("set password = 'secret' then fail;", linenoiseHistoryLine(1));

  shell.process_line("create user foo@bar identified by 'secret';");
  EXPECT_EQ(2, linenoiseHistorySize());
  EXPECT_STREQ("select 1;", linenoiseHistoryLine(0));
  EXPECT_STREQ("create user foo@bar identified by 'secret';",
               linenoiseHistoryLine(1));

  shell.process_line("alter user foo@bar set password='secret';");
  EXPECT_EQ(2, linenoiseHistorySize());
  EXPECT_STREQ("select 1;", linenoiseHistoryLine(0));
  EXPECT_STREQ("alter user foo@bar set password='secret';",
               linenoiseHistoryLine(1));

  shell.process_line("secret;");
  EXPECT_EQ(2, linenoiseHistorySize());
  EXPECT_STREQ("select 1;", linenoiseHistoryLine(0));
  EXPECT_STREQ("secret;", linenoiseHistoryLine(1));

  // set filter directly
  shell.set_sql_safe_for_logging("*SECret*");

  shell.process_line("top secret;");
  EXPECT_EQ(3, linenoiseHistorySize());
  EXPECT_STREQ("select 1;", linenoiseHistoryLine(0));
  EXPECT_STREQ("secret;", linenoiseHistoryLine(1));
  EXPECT_STREQ("top secret;", linenoiseHistoryLine(2));
  shell.process_line("top;");
  EXPECT_EQ(3, linenoiseHistorySize());
  EXPECT_STREQ("top;", linenoiseHistoryLine(2));

#ifdef HAVE_V8
  std::string print_stmt = "println('secret');";
#else
  std::string print_stmt = "print('secret')";
#endif

  // SQL filter only applies to SQL mode
  shell.process_line(to_scripting);
  shell.process_line(print_stmt);
  EXPECT_EQ(1, linenoiseHistorySize());
  EXPECT_STREQ(print_stmt.c_str(), linenoiseHistoryLine(0));

#ifdef HAVE_V8
  shell.process_line("\\py");
  shell.process_line("print('secret')");
  EXPECT_EQ(1, linenoiseHistorySize());
  EXPECT_STREQ("print('secret')", linenoiseHistoryLine(0));
#endif

  // unset filter via shell options
  mysqlsh::Options opts(shell.get_options());
  opts.set_member("history.sql.ignorePattern", shcore::Value(""));

  shell.process_line("top secret;");
  EXPECT_EQ(2, linenoiseHistorySize());
  EXPECT_STREQ("top secret;", linenoiseHistoryLine(1));

  // TS_CV#6
  // shell.options["history.sql.ignorePattern"]=string:string:string:string with
  // multiple strings separated by a colon (:) works correctly
  // TS_CV#7
  // TS_CV#8
  shell.process_line(to_scripting);
  shell.process_line(
      "shell.options['history.sql.ignorePattern'] = '*bla*:*ble*';");
  EXPECT_STREQ("shell.options['history.sql.ignorePattern'] = '*bla*:*ble*';",
               linenoiseHistoryLine(0));
  shell.process_line("\\sql");
  shell.process_line("select 'bga';");
  shell.process_line("select 'bge';");
  shell.process_line("select 'aablaaa';");
  shell.process_line("select 'aaBLEaa';");
  shell.process_line("select 'bla';");
  shell.process_line("select '*bla*';");
  shell.process_line("select 'bgi';");
  EXPECT_STREQ("select 'bga';", linenoiseHistoryLine(0));
  EXPECT_STREQ("select 'bge';", linenoiseHistoryLine(1));
  EXPECT_STREQ("select 'bgi';", linenoiseHistoryLine(2));

  shell.process_line(to_scripting);
  shell.process_line("shell.options['history.sql.ignorePattern'] = 'set;';");
  shell.process_line("\\sql");
  shell.process_line("\\history clear");
  EXPECT_EQ(1, linenoiseHistorySize());
  shell.process_line("bar;");
  EXPECT_EQ(2, linenoiseHistorySize());
  EXPECT_STREQ("bar;", linenoiseHistoryLine(1));
  shell.process_line("set;");
  EXPECT_EQ(3, linenoiseHistorySize());
  EXPECT_STREQ("bar;", linenoiseHistoryLine(1));
  EXPECT_STREQ("set;", linenoiseHistoryLine(2));
  shell.process_line("xset;");
  EXPECT_EQ(3, linenoiseHistorySize());
  EXPECT_STREQ("xset;", linenoiseHistoryLine(2));

  shell.process_line(to_scripting);
  shell.process_line("shell.options['history.sql.ignorePattern'] = '*';");
  shell.process_line("\\sql");
  shell.process_line("\\history clear");
  EXPECT_EQ(1, linenoiseHistorySize());
  shell.process_line("a;");
  EXPECT_EQ(1, linenoiseHistorySize());
  shell.process_line("hello world;");
  EXPECT_EQ(1, linenoiseHistorySize());
  shell.process_line("*;");
  EXPECT_EQ(1, linenoiseHistorySize());
  shell.process_line(";");
  EXPECT_EQ(1, linenoiseHistorySize());

  shell.process_line(to_scripting);
  shell.process_line("shell.options['history.sql.ignorePattern'] = '**';");
  shell.process_line("\\sql");
  shell.process_line("\\history clear");
  EXPECT_EQ(1, linenoiseHistorySize());
  shell.process_line("a;");
  EXPECT_EQ(1, linenoiseHistorySize());
  shell.process_line("hello world;");
  EXPECT_EQ(1, linenoiseHistorySize());
  shell.process_line("*;");
  EXPECT_EQ(1, linenoiseHistorySize());
  shell.process_line(";");
  EXPECT_EQ(1, linenoiseHistorySize());

  shell.process_line(to_scripting);
  shell.process_line("shell.options['history.sql.ignorePattern'] = '?';");
  shell.process_line("\\sql");
  shell.process_line("\\history clear");
  EXPECT_EQ(1, linenoiseHistorySize());
  shell.process_line(";");
  EXPECT_EQ(2, linenoiseHistorySize());
  shell.process_line("a;");
  EXPECT_EQ(2, linenoiseHistorySize());
  shell.process_line("aa;");
  EXPECT_EQ(3, linenoiseHistorySize());

  shell.process_line(to_scripting);
  shell.process_line("shell.options['history.sql.ignorePattern'] = '?*';");
  shell.process_line("\\sql");
  shell.process_line("\\history clear");
  EXPECT_EQ(1, linenoiseHistorySize());
  shell.process_line("?;");
  EXPECT_EQ(1, linenoiseHistorySize());
  shell.process_line("a;");
  EXPECT_EQ(1, linenoiseHistorySize());
  shell.process_line("aa;");
  EXPECT_EQ(1, linenoiseHistorySize());

  shell.process_line(to_scripting);
  shell.process_line("shell.options['history.sql.ignorePattern'] = 'a?b?c*';");
  shell.process_line("\\sql");
  shell.process_line("\\history clear");
  EXPECT_EQ(1, linenoiseHistorySize());
  shell.process_line("abc;");
  EXPECT_EQ(2, linenoiseHistorySize());
  shell.process_line("abxc;");
  EXPECT_EQ(3, linenoiseHistorySize());
  shell.process_line("xaxbxc;");
  EXPECT_EQ(4, linenoiseHistorySize());
  shell.process_line("axbxcx;");
  EXPECT_EQ(5, linenoiseHistorySize());

  shell.process_line("axbxc;");
  EXPECT_EQ(5, linenoiseHistorySize());
  shell.process_line("axbxcdddeefg;");
  EXPECT_EQ(5, linenoiseHistorySize());
  shell.process_line("a b c;");
  EXPECT_EQ(5, linenoiseHistorySize());
}

TEST_F(Shell_history, history_ignore_wildcard_questionmark) {
  // WL#10446 TS_CV#8 - check wildcards, * is covered elsewhere, ? is covered
  // here

  linenoiseHistoryFree();
  // NOTE: resizing it may cause troubles with \history dump indexing
  // because we do not call the approriate API
  linenoiseHistorySetMaxLen(100);

  char *args[] = {const_cast<char *>("ut"), const_cast<char *>("--sql"),
                  nullptr};
  mysqlsh::Command_line_shell shell(
      std::make_shared<Shell_options>(2, args, _options_file));

  // ? = match exactly one
  EXPECT_EQ(0, linenoiseHistorySize());
  shell.process_line(to_scripting);
  shell.process_line(
      "shell.options['history.sql.ignorePattern'] = '?ELECT 1;'");
  shell.process_line("\\sql");
  EXPECT_EQ(0, linenoiseHistorySize());

  shell.process_line("SELECT 1;");
  EXPECT_EQ(1, linenoiseHistorySize());

  shell.process_line("ELECT 1;");
  EXPECT_EQ(1, linenoiseHistorySize());

  shell.process_line(to_scripting);
  shell.process_line(
      "shell.options['history.sql.ignorePattern'] = '?? ??;:?\?'");
  shell.process_line("\\sql");
  EXPECT_EQ(0, linenoiseHistorySize());

  shell.process_line("AA BB;");
  EXPECT_EQ(1, linenoiseHistorySize());

  shell.process_line("A;");
  EXPECT_EQ(1, linenoiseHistorySize());

  shell.process_line(" A\n  ;");
  EXPECT_EQ(1, linenoiseHistorySize());
}

TEST_F(Shell_history, history_set_option) {
  // All user input shall be caputered in the history
  //
  // cmdline_shell.cc catches asssorted "events" that are generated upon user
  // input To test the requirement we need to try to trick base_shell.cc and
  // find events that are not properly handled or even missing ones. Most other
  // tests check the event SN_STATEMENT_EXECUTED.

  linenoiseHistoryFree();

  mysqlsh::Command_line_shell shell(
      std::make_shared<Shell_options>(0, nullptr, _options_file));

  // SN_SHELL_OPTION_CHANGED
  shell.process_line(
      "shell.options['history.sql.ignorePattern'] = '*SELECT 1*';");
  EXPECT_EQ(1, linenoiseHistorySize());
  EXPECT_STREQ("shell.options['history.sql.ignorePattern'] = '*SELECT 1*';",
               linenoiseHistoryLine(0));

  // And now another "plain assignment" from a user perspective
  shell.process_line("a = 1;");
  EXPECT_EQ(2, linenoiseHistorySize());
  EXPECT_STREQ("a = 1;", linenoiseHistoryLine(1));
}

TEST_F(Shell_history, history_ignore_pattern_js) {
  mysqlsh::Command_line_shell shell(
      std::make_shared<Shell_options>(0, nullptr, _options_file));

  shell.process_line("shell.options['history.sql.ignorePattern'] = '*SELECT*'");
  shell.process_line("// SELECT");

  EXPECT_STREQ("// SELECT", linenoiseHistoryLine(1));
}

TEST_F(Shell_history, history_ignore_pattern_py) {
  char *args[] = {const_cast<char *>("ut"), const_cast<char *>("--py"),
                  nullptr};
  mysqlsh::Command_line_shell shell(
      std::make_shared<Shell_options>(2, args, _options_file));

  shell.process_line(
      "setattr(shell.options, 'history.sql.ignorePattern', "
      "'WHAT A PROPERTY');");
  EXPECT_EQ(1, linenoiseHistorySize());
  EXPECT_STREQ(
      "setattr(shell.options, 'history.sql.ignorePattern', 'WHAT A PROPERTY');",
      linenoiseHistoryLine(0));

  // Second issue - comments not recorded in history
  shell.process_line("# WHAT A PROPERTY NAME");
  EXPECT_EQ(2, linenoiseHistorySize());
  EXPECT_STREQ("# WHAT A PROPERTY NAME", linenoiseHistoryLine(1));
}

TEST_F(Shell_history, history_linenoise) {
  // Test cases covered here:
  // TS_CLE#1 Commands executed by the user in the shell are saved to the
  // command history
  // TS_HM#1 only commands interactively typed by the user in the shell prompt
  // are saved to history file: ~/.mysqlsh/history file

  mysqlsh::Command_line_shell shell(
      std::make_shared<Shell_options>(0, nullptr, _options_file));
  const std::string hist_file = shell.history_file();
  shcore::delete_file(hist_file);

  {
    EXPECT_NO_THROW(shell.load_state());

    EXPECT_EQ(0, linenoiseHistorySize());
    shell.process_line("print(1);");
    EXPECT_EQ(1, linenoiseHistorySize());
    shell.process_line("print(2);");
    EXPECT_EQ(2, linenoiseHistorySize());
    shell.process_line("print(3);");
    EXPECT_EQ(3, linenoiseHistorySize());
    EXPECT_STREQ("print(1);", linenoiseHistoryLine(0));
    EXPECT_STREQ("print(2);", linenoiseHistoryLine(1));
    EXPECT_STREQ("print(3);", linenoiseHistoryLine(2));

    mysqlsh::Options opt(shell.get_options());

    // TS_CV#10
    // autosave off by default
    EXPECT_FALSE(opt.get_member("history.autoSave").as_bool());

    // check history autosave
    shell.save_state();
    EXPECT_FALSE(shcore::is_file(hist_file));

    opt.set_member("history.autoSave", shcore::Value::True());

    shell.save_state();
    EXPECT_TRUE(shcore::is_file(hist_file));

    // TS_CV#1 shell.options["history.maxSize"]=number sets the max number of
    // entries to store in the shell history file
    // TS_CV#2 (is not a requirement)
    opt.set_member("history.maxSize", shcore::Value(1));
    EXPECT_EQ(1, shell._history.size());
    EXPECT_STREQ("print(3);", linenoiseHistoryLine(0));

    // TS_CV#3 shell.options["history.maxSize"]=number, if the number of history
    // entries exceeds the configured maximum, the oldest entries will be
    // removed.
    shell.process_line("print(4);");
    EXPECT_EQ(1, shell._history.size());

    EXPECT_STREQ("print(4);", linenoiseHistoryLine(0));

    enable_capture();

    m_capture.clear();
    shell.process_line("\\help history");

    // TS_SC#1
    m_capture.clear();
    shell.process_line("\\history");
    EXPECT_EQ("    5  \\help history\n\n", m_capture);

    m_capture.clear();
    // TS_SC#3
    shell.process_line("\\history del 6");
    EXPECT_EQ("", m_capture);

    m_capture.clear();
    shell.process_line("\\history");
    EXPECT_EQ("    1  \\history del 6\n\n", m_capture);

    m_capture.clear();
    shell.process_line("print(5);");

    m_capture.clear();
    shell.process_line("\\history 1");
    EXPECT_EQ("Invalid options for \\history. See \\help history for syntax.\n",
              m_capture);

    m_capture.clear();
    shell.process_line("\\history 1 x");
    EXPECT_EQ("Invalid options for \\history. See \\help history for syntax.\n",
              m_capture);

    m_capture.clear();
    shell.process_line("\\history x 1");
    EXPECT_EQ("Invalid options for \\history. See \\help history for syntax.\n",
              m_capture);

    m_capture.clear();
    shell.process_line("\\history clear 1");
    EXPECT_EQ("\\history clear does not take any parameters\n", m_capture);

    m_capture.clear();
    shell.process_line("\\history del");
    EXPECT_EQ("\\history delete requires entry number to be deleted\n",
              m_capture);

    m_capture.clear();
    shell.process_line("\\history del 50");
    EXPECT_EQ("Invalid history entry: 50 - valid range is 8-8\n", m_capture);

    m_capture.clear();
    shell.process_line("\\history 0 -1 -1");
    EXPECT_EQ("Invalid options for \\history. See \\help history for syntax.\n",
              m_capture);

    // TS_SC#4 - cancelled

    m_capture.clear();
    // TS_SC#5
    shell.process_line("\\history clear");
    EXPECT_EQ("", m_capture);

    m_capture.clear();
    shell.process_line("\\history");
    EXPECT_EQ("    1  \\history clear\n\n", m_capture);
    EXPECT_EQ(1, shell._history.size());
    shell.load_state();
    EXPECT_EQ(1, shell._history.size());
    EXPECT_STREQ("print(3);", linenoiseHistoryLine(0));

    // check no history
    // TS_CV#4 shell.options["history.maxSize"]=0 , no history will be saved and
    // old history entries will also be deleted when the shell exits.

    // Note: linenoise history includes 1 history entry for the "current" line
    // being edited. The current line will be cleared from history as soon as
    // Enter is pressed. However we can't emulate keypress that from here, so
    // the "current" line will be stuck in the history until the next command.
    // This makes this test for something different from what we want, but
    // there's no better way
    m_capture.clear();
    shell.process_line("shell.options['history.maxSize']=0;\n");
    EXPECT_EQ(0, shell._history.size());

    m_capture.clear();
    shell.process_line("\\history\n");
    EXPECT_EQ("", m_capture);
    EXPECT_STREQ(NULL, linenoiseHistoryLine(1));

    m_capture.clear();
    // TS_SC#2
    shell.process_line("\\history save\n");
    EXPECT_EQ("Command history file saved with 0 entries.\n\n", m_capture);

    std::string hdata;
    shcore::load_text_file(hist_file, hdata);
    EXPECT_EQ("", hdata);
  }
  shcore::delete_file(hist_file);
}

TEST_F(Shell_history, check_help_shows_history) {
  // We should have one test that checks the output of \h for compleness
  // TODO(ulf) (Remove my [Ulf] note when checked): I don't mind if we have one
  // central UT for \h or every command we introduce has it's own tests.
  // My preference is a global one but I'm not the one to maintain the UTs so
  // I'll just do something and let the owners of the UTs decide.

  mysqlsh::Command_line_shell shell(
      std::make_shared<Shell_options>(0, nullptr, _options_file));

  {
    // Expecting the Shell mention \history in \help
    enable_capture();

    m_capture.clear();
    shell.process_line("\\help");
    EXPECT_TRUE(
        strstr(m_capture.c_str(),
               "\\history            View and edit command line history."));
  }
}

TEST_F(Shell_history, history_autosave_int) {
  mysqlsh::Command_line_shell shell(
      std::make_shared<Shell_options>(0, nullptr, _options_file));
  shcore::delete_file(shell.history_file());

  {
    mysqlsh::Options opt(shell.get_options());

    // Expecting the Shell to cast to boolean true if value is 1 or 0
    opt.set_member("history.autoSave", shcore::Value(1));
    EXPECT_TRUE(opt.get_member("history.autoSave").as_bool());

    // Expecting the Shell to print history.autoSave = true not 101
    enable_capture();

    m_capture.clear();
    shell.process_line("print(shell.options)");
    // we perform automatic type conversion on set
    EXPECT_TRUE(strstr(m_capture.c_str(), "\"history.autoSave\": true"));
  }
}

#ifdef HAVE_V8
TEST_F(Shell_history, check_history_source_js) {
  // WL#10446 says \source shall no add entries to the history
  // Only history entry shall the \source itself

  const auto &server_uri = shell_test_server_uri();
  char *args[] = {const_cast<char *>("ut"), const_cast<char *>("--js"),
                  const_cast<char *>(server_uri.c_str()), nullptr};
  mysqlsh::Command_line_shell shell(
      std::make_shared<Shell_options>(3, args, _options_file));
  shell._history.set_limit(10);

  std::ofstream of;
  of.open("test_source.js");
  of << "print(1);\n";
  of << "print(2);\n";
  of.close();

  EXPECT_NO_THROW(shell.load_state());
  EXPECT_EQ(0, linenoiseHistorySize());

  {
    enable_capture();

    shell.process_line("\\source test_source.js");
    EXPECT_EQ(1, linenoiseHistorySize());
    EXPECT_EQ("1\n2\n", m_capture);
  }

  shcore::delete_file("test_source.js");
}

TEST_F(Shell_history, check_history_source_js_nonl_interactive) {
  // WL#10446 says \source shall no add entries to the history
  // Only history entry shall the \source itself

  // BUG#30765725 SHELL PYTHON HISTORY INCLUDING LAST LINE OF SCRIPTS
  // If script doesn't have newline at the end of the file, then last statement
  // is not executed.

  const auto &server_uri = shell_test_server_uri();
  char *args[] = {const_cast<char *>("ut"), const_cast<char *>("--js"),
                  const_cast<char *>("--interactive=full"),
                  const_cast<char *>(server_uri.c_str()), nullptr};
  mysqlsh::Command_line_shell shell(
      std::make_shared<Shell_options>(4, args, _options_file));
  shell._history.set_limit(10);

  std::ofstream of;
  of.open("test_source_nonl.js");
  of << "print(`line1\nline2\nline3`)";
  of.close();

  EXPECT_NO_THROW(shell.load_state());
  EXPECT_EQ(0, linenoiseHistorySize());

  {
    enable_capture();

    shell.process_line("\\source test_source_nonl.js");
    EXPECT_EQ(1, linenoiseHistorySize());
    EXPECT_EQ(std::string{"\\source test_source_nonl.js"},
              std::string(linenoiseHistoryLine(0)));
    EXPECT_EQ(R"*(mysql-js> 
print(`line1

       -> 
line2

       -> 
line3`)

       -> 
line1
line2
line3
)*",
              m_capture);
  }

  shcore::delete_file("test_source_nonl.js");
}
#endif

TEST_F(Shell_history, check_history_source_py) {
  // WL#10446 says \source shall no add entries to the history
  // Only history entry shall the \source itself

  const auto &server_uri = shell_test_server_uri();
  char *args[] = {const_cast<char *>("ut"), const_cast<char *>("--py"),
                  const_cast<char *>(server_uri.c_str()), nullptr};
  mysqlsh::Command_line_shell shell(
      std::make_shared<Shell_options>(3, args, _options_file));
  shell._history.set_limit(10);

  std::ofstream of;
  of.open("test_source.py");
  of << "print(1)\n";
  of << "print(2)\n";
  of.close();

  EXPECT_NO_THROW(shell.load_state());
  EXPECT_EQ(0, linenoiseHistorySize());

  {
    enable_capture();

    shell.process_line("\\source test_source.py");
    EXPECT_EQ(1, linenoiseHistorySize());
    EXPECT_EQ("1\n\n\n2\n\n\n", m_capture);
  }

  shcore::delete_file("test_source.py");
}

TEST_F(Shell_history, check_history_source_py_nonl_interactive) {
  // WL#10446 says \source shall no add entries to the history
  // Only history entry shall the \source itself

  // BUG#30765725 SHELL PYTHON HISTORY INCLUDING LAST LINE OF SCRIPTS
  // If script doesn't have newline at the end of the file, then last statement
  // is not executed.

  const auto &server_uri = shell_test_server_uri();
  char *args[] = {const_cast<char *>("ut"), const_cast<char *>("--py"),
                  const_cast<char *>("--interactive=full"),
                  const_cast<char *>(server_uri.c_str()), nullptr};
  mysqlsh::Command_line_shell shell(
      std::make_shared<Shell_options>(4, args, _options_file));
  shell._history.set_limit(10);

  std::ofstream of;
  of.open("test_source_nonl.py");
  of << "print('''line1\nline2\nline3''')";
  of.close();

  EXPECT_NO_THROW(shell.load_state());
  EXPECT_EQ(0, linenoiseHistorySize());

  {
    enable_capture();

    shell.process_line("\\source test_source_nonl.py");
    EXPECT_EQ(1, linenoiseHistorySize());
    EXPECT_EQ(std::string{"\\source test_source_nonl.py"},
              std::string(linenoiseHistoryLine(0)));
    EXPECT_EQ(R"*(mysql-py> 
print('''line1

       -> 
line2

       -> 
line3''')

line1
line2
line3


mysql-py> 
)*",
              m_capture);
  }

  shcore::delete_file("test_source_nonl.py");
}

TEST_F(Shell_history, check_history_source_py_nonl_continuedstate_interactive) {
  // WL#10446 says \source shall no add entries to the history
  // Only history entry shall the \source itself

  // BUG#30765725 SHELL PYTHON HISTORY INCLUDING LAST LINE OF SCRIPTS
  // If script doesn't have newline at the end of the file, then last statement
  // is not executed.

  const auto &server_uri = shell_test_server_uri();
  char *args[] = {const_cast<char *>("ut"), const_cast<char *>("--py"),
                  const_cast<char *>("--interactive=full"),
                  const_cast<char *>(server_uri.c_str()), nullptr};
  mysqlsh::Command_line_shell shell(
      std::make_shared<Shell_options>(4, args, _options_file));
  shell._history.set_limit(10);

  std::ofstream of;
  of.open("test_source_nonl.py");
  // of << "print('''line1\nline2\nline3";
  of << "l = ['line1', 'line2', 'line3']\nfor s in l:\n  print(s";
  of.close();

  EXPECT_NO_THROW(shell.load_state());
  EXPECT_EQ(0, linenoiseHistorySize());

  {
    enable_capture();

    shell.process_line("\\source test_source_nonl.py");
    EXPECT_EQ(shell.input_state(), shcore::Input_state::Ok);
    EXPECT_EQ(1, linenoiseHistorySize());
    EXPECT_EQ(std::string{"\\source test_source_nonl.py"},
              std::string(linenoiseHistoryLine(0)));
    EXPECT_THAT(m_capture, ::testing::HasSubstr("SyntaxError"));
  }

  shcore::delete_file("test_source_nonl.py");
}

TEST_F(Shell_history, check_history_overflow_del) {
  // See if the history numbering still works for users when the history
  // overflows, entries are dropped and renumbering might take place.

  {
    mysqlsh::Command_line_shell shell(
        std::make_shared<Shell_options>(0, nullptr, _options_file));
    shell._history.set_limit(3);

    EXPECT_NO_THROW(shell.load_state());
    EXPECT_EQ(0, shell._history.size());

    shell.process_line("// 1");
    // Actual history is now:
    // 1   // 1
    shell.process_line("// 2");
    // Actual history is now:
    // 1   // 1
    // 2   // 2
    shell.process_line("// 3");
    // Actual history is now:
    // 1   // 1
    // 2   // 2
    // 3   // 3
    EXPECT_EQ(3, shell._history.size());

    enable_capture();

    // Note: history entries are added AFTER they are executed
    // that means if \history is called, it will print the stack up the
    // last command before \history itself

    // Now we print the entries and offsets to be used with history del
    // Did we just pop index 0 off the stack by pushing \\history?
    // A: Yes, but the pop only happens after the history is printed
    shell.process_line("\\history");
    EXPECT_EQ("    1  // 1\n\n    2  // 2\n\n    3  // 3\n\n", m_capture);
    EXPECT_EQ(3, shell._history.size());
    // Actual history is now:
    // 2   // 2
    // 3   // 3
    // 4   \history

    // Delete entry 2 // 2
    shell.process_line("\\history del 2");
    // Actual history is now:
    // 3   // 3
    // 4   \history
    // 5   \history del 2

    m_capture.clear();
    shell.process_line("\\history");
    // Actual history is now:
    // 4   \history
    // 5   \history del 2
    // 6   \history
    EXPECT_EQ("    3  // 3\n\n    4  \\history\n\n    5  \\history del 2\n\n",
              m_capture);
    EXPECT_EQ(3, shell._history.size());
  }
}

TEST_F(Shell_history, history_management) {
  mysqlsh::Command_line_shell shell(
      std::make_shared<Shell_options>(0, nullptr, _options_file));

  enable_capture();

  shell.process_line("\\js\n");
  shell.process_line("\\history clear\n");
  shell.process_line("shell.options['history.maxSize']=10;\n");

  shcore::create_file("test_file.js", "println('test1');\nprintln('test2');\n");

  std::string histfile = shell.history_file();
  shcore::delete_file(histfile);

  // TS_HM#2 History File does not add commands that are executed indirectly or
  // internally to the history (example: \source command is executed)
  shell.process_line("println('hello')\n");
  shell.process_line("\\source test_file.js\n");
  shell.process_line("\\history save\n");

  std::string hist;
  shcore::load_text_file(histfile, hist);
  EXPECT_TRUE(hist.find("'hello'") != std::string::npos);
  EXPECT_FALSE(hist.find("'test1'") != std::string::npos);
  EXPECT_FALSE(hist.find("'test2'") != std::string::npos);

// TS_HM#3 History File is protected from Other USERS
#ifndef _WIN32
  struct stat st;
  EXPECT_EQ(0, stat(histfile.c_str(), &st));
  EXPECT_EQ(S_IRUSR | S_IWUSR, st.st_mode & S_IRWXU);
  EXPECT_EQ(0, st.st_mode & S_IRWXG);
  EXPECT_EQ(0, st.st_mode & S_IRWXO);

  shcore::delete_file(histfile);
  mode_t omode = umask(0);
  shell.process_line("\\history save\n");
  EXPECT_EQ(0, stat(histfile.c_str(), &st));
  EXPECT_EQ(S_IRUSR | S_IWUSR, st.st_mode & S_IRWXU);
  EXPECT_EQ(0, st.st_mode & S_IRWXG);
  EXPECT_EQ(0, st.st_mode & S_IRWXO);

  shcore::delete_file(histfile);
  umask(0022);
  shell.process_line("\\history save\n");
  EXPECT_EQ(0, stat(histfile.c_str(), &st));
  EXPECT_EQ(S_IRUSR | S_IWUSR, st.st_mode & S_IRWXU);
  EXPECT_EQ(0, st.st_mode & S_IRWXG);
  EXPECT_EQ(0, st.st_mode & S_IRWXO);
  shcore::delete_file(histfile);

  umask(omode);
#endif

  // TS_HM#4 History File, If the command has multiple lines, the newlines are
  // stripped
  shell.process_line("if (123)\nbar=456;\n");
  shell.process_line("\\history save\n");
  shcore::load_text_file(histfile, hist);
  EXPECT_TRUE(hist.find("if (123) bar=456") != std::string::npos);

  shell.process_line("\\history clear\n");
  // TS_HM#5 History File If the command is the same as the previous one to the
  // one executed previously (case sensitive), it wont be duplicated.
  shell.process_line("println('bar');");
  shell.process_line("println('foo');");
  shell.process_line("println('foo');");
  m_capture.clear();
  shell.process_line("\\history");
  EXPECT_EQ(
      "    1  \\history clear\n\n"
      "    2  println('bar');\n\n"
      "    3  println('foo');\n\n",
      m_capture);
  m_capture.clear();
  // ensure ordering after an ignored duplicate item continues sequential
  shell.process_line("\\history");
  EXPECT_EQ(
      "    1  \\history clear\n\n"
      "    2  println('bar');\n\n"
      "    3  println('foo');\n\n"
      "    4  \\history\n\n",
      m_capture);

  // TS_HM#7 if the history file can't be read or written to, it will log an
  // error message and skip the read/write operation.
  shcore::delete_file(histfile);

  shcore::ensure_dir_exists(histfile.c_str());
  m_capture.clear();
  shell.process_line("\\history save");

  auto base = "Could not save command history to";
#ifdef _WIN32
  auto specific = histfile + ": Permission denied";
#else
  auto specific = histfile + ": Is a directory";
#endif
  auto base_found = m_capture.find(base) != std::string::npos;
  auto specific_found = m_capture.find(specific) != std::string::npos;

  if (!base_found || !specific_found) {
    SCOPED_TRACE(specific);
    SCOPED_TRACE(base);
    SCOPED_TRACE("Expected:");
    SCOPED_TRACE(m_capture);
    SCOPED_TRACE("Actual:");
    FAIL();
  }

#ifndef _WIN32
  EXPECT_EQ(0, chmod(histfile.c_str(), 0));

  m_capture.clear();
  EXPECT_NO_THROW(shell.load_state());

  base = "Could not load command history from";
  specific = histfile + ": Permission denied";
  base_found = m_capture.find(base) != std::string::npos;
  specific_found = m_capture.find(specific) != std::string::npos;

  if (!base_found || !specific_found) {
    SCOPED_TRACE(specific);
    SCOPED_TRACE(base);
    SCOPED_TRACE("Expected:");
    SCOPED_TRACE(m_capture);
    SCOPED_TRACE("Actual:");
    FAIL();
  }
#endif

  shcore::remove_directory(histfile, false);
}

TEST_F(Shell_history, history_sizes) {
  // We use a secondary history list to provide entry numbers that do not
  // change when the list contents changes so that \history del works.
  // This test shall cover internal list management (grow, shrink, overflow).
  // No crash is good enough.
  // See also src/mysqlsh/history.cc|h
  mysqlsh::Command_line_shell shell(
      std::make_shared<Shell_options>(0, nullptr, _options_file));

  enable_capture();

  shell.process_line("shell.options['history.maxSize'] = 4;");
  shell.process_line("print(1);");
  shell.process_line("print(2);");
  shell.process_line("print(3);");
  shell.process_line("print(4);");

  m_capture.clear();

  shell.process_line("\\history");

  EXPECT_EQ(4, shell._history.size());

  // NOTE: Always use shell._history.size() to check history size, otherwise
  // you will get different results when test cases are ran on UTs vs manually
  EXPECT_EQ(
      "    2  print(1);\n\n"
      "    3  print(2);\n\n"
      "    4  print(3);\n\n"
      "    5  print(4);\n\n",
      m_capture);

  shell.process_line("shell.options['history.maxSize'] = 3;");
  shell.process_line("\\history");
  shell.process_line("print(5);");
  shell.process_line("print(6);");
  EXPECT_EQ(3, shell._history.size());

  shell.process_line("shell.options['history.maxSize'] = 0;");
  shell.process_line("print(7);");
  shell.process_line("\\history");
  shell.process_line("print(77);");
  EXPECT_EQ(0, shell._history.size());

  shell.process_line("shell.options['history.maxSize'] = 1;");
  shell.process_line("print(8);");
  shell.process_line("\\history");
  shell.process_line("print(88);");
  EXPECT_EQ(1, shell._history.size());

  shell.process_line("shell.options['history.maxSize'] = 2;");
  shell.process_line("print(9);");
  shell.process_line("\\history");
  shell.process_line("print(99);");
  EXPECT_EQ(2, shell._history.size());

  shell.process_line("shell.options['history.maxSize'] = 0;");
  shell.process_line("\\history");
  EXPECT_EQ(0, shell._history.size());

  // no crash is good enough so far

  // attempt deleting entry that is not supposed to exist
  shell.process_line("shell.options['history.maxSize'] = 3;");
  shell.process_line("\\history clear");
  shell.process_line("print(42);");
  shell.process_line("\\history del 3");
  // ensure numbering continues normally
  shell.process_line("print(42);");
  m_capture.clear();
  shell.process_line("\\history");
  EXPECT_EQ(
      "    2  print(42);\n\n"
      "    3  \\history del 3\n\n"
      "    4  print(42);\n\n",
      m_capture);
}

TEST_F(Shell_history, history_del_invisible_entry) {
  // See also TEST_F(Shell_history, history_sizes)

  mysqlsh::Command_line_shell shell(
      std::make_shared<Shell_options>(0, nullptr, _options_file));

  enable_capture();

  // Trivial and should be covered elsewhere already
  shell.process_line("\\history del 10-1");
  EXPECT_TRUE(strstr(m_capture.c_str(), "Invalid"));

  // History has now one command and it will get the index 1
  // Can we access the invisible implementation dependent index 2?
  m_capture.clear();
  shell.process_line("\\history del 2");
  EXPECT_TRUE(strstr(m_capture.c_str(), "Invalid"));
}

TEST_F(Shell_history, history_source_history) {
  // Generate a history, save it, load and execute saved history
  // using \source. \source shall not add any executed commands to history
  // but preserve the history state from after save.

  mysqlsh::Command_line_shell shell(
      std::make_shared<Shell_options>(0, nullptr, _options_file));

  enable_capture();

  shell.process_line("session");
  shell.process_line("dba");
  shell.process_line("\\history save");

  std::string histfile = shell.history_file();
  std::string line = "\\source " + histfile;
  shell.process_line(line);
  shell.process_line("\\history");
  EXPECT_EQ(5, shell._history.size());

  EXPECT_EQ(
      "Command history file saved with 2 entries.\n\n"
      "    1  session\n\n"
      "    2  dba\n\n"
      "    3  \\history save\n\n"
      "    4  " +
          line + "\n\n",
      m_capture);
  shcore::delete_file(histfile);
}

TEST_F(Shell_history, history_del_range) {
  mysqlsh::Command_line_shell shell(
      std::make_shared<Shell_options>(0, nullptr, _options_file));

  enable_capture();

  shell.process_line("session");
  shell.process_line("dba");
  shell.process_line("mysql");
  shell.process_line("mysqlx");
  shell.process_line("shell");
  shell.process_line("util");
  EXPECT_EQ(6, shell._history.size());
  // valid range
  shell.process_line("\\history del 1-3");
  shell.process_line("\\history");

  EXPECT_EQ(5, shell._history.size());

  EXPECT_EQ(
      "    4  mysqlx\n\n"
      "    5  shell\n\n"
      "    6  util\n\n"
      "    7  \\history del 1-3\n\n",
      m_capture);

  // invalid range: using former entries
  m_capture.clear();
  shell.process_line("\\history del 1-3");
  EXPECT_EQ("Invalid history range: 1-3 - valid range is 4-8\n", m_capture);

  // lower bound bigger than upper bound
  m_capture.clear();
  shell.process_line("\\history del 7-4");
  EXPECT_EQ("Invalid history range 7-4. Last item must be greater than first\n",
            m_capture);

  shell.process_line("\\history clear");
  shell.process_line("session");
  shell.process_line("dba");
  m_capture.clear();
  shell.process_line("\\history del 1 - 3");
  // Not sure if we want to give an error here or be gentle and accept space
  EXPECT_EQ("\\history delete requires entry number to be deleted\n",
            m_capture);
}

TEST_F(Shell_history, history_entry_number_reset) {
  // Numbering shall only be reset when Shell is restarted
  // or when \\history clear is called

  mysqlsh::Command_line_shell shell(
      std::make_shared<Shell_options>(0, nullptr, _options_file));

  enable_capture();

  shell.process_line("session");
  shell.process_line("dba");
  shell.process_line("util");
  shell.process_line("\\history");
  EXPECT_EQ(4, shell._history.size());
  EXPECT_EQ(
      "    1  session\n\n"
      "    2  dba\n\n"
      "    3  util\n\n",
      m_capture);

  m_capture.clear();
  shell.process_line("\\history clear");
  shell.process_line("\\history");
  // This should get number 4, if numbering was not reset
  EXPECT_EQ("    1  \\history clear\n\n", m_capture);
  EXPECT_EQ(2, shell._history.size());
}

TEST_F(Shell_history, history_delete_range) {
#define LOAD_HISTORY(data)                                          \
  shcore::create_file("testhistory", shcore::str_join(data, "\n")); \
  shell._history.load("testhistory");

#define CHECK_DELRANGE(range, expected_init)                                   \
  {                                                                            \
    std::vector<std::string> expected(expected_init);                          \
    std::vector<std::string> dump;                                             \
    SCOPED_TRACE(range);                                                       \
    shell.process_line("\\history del " range);                                \
    shell._history.dump([&dump](const std::string &s) { dump.push_back(s); }); \
    for (std::vector<std::string>::size_type i = 0; i < expected.size();       \
         ++i) {                                                                \
      EXPECT_TRUE(i < dump.size());                                            \
      EXPECT_EQ(expected[i], shcore::str_strip(dump[i]));                      \
    }                                                                          \
    EXPECT_EQ(expected.size(), dump.size());                                   \
  }

  mysqlsh::Command_line_shell shell(
      std::make_shared<Shell_options>(0, nullptr, _options_file));
  enable_capture();
  using strv = std::vector<std::string>;
  shell._history.set_limit(10);

  // range of 1
  m_capture.clear();
  LOAD_HISTORY(strv({"one", "two", "three", "four"}));
  CHECK_DELRANGE(
      "1-1", strv({"2  two", "3  three", "4  four", "5  \\history del 1-1"}));
  EXPECT_TRUE(m_capture.empty());

  m_capture.clear();
  LOAD_HISTORY(strv({"one", "two", "three", "four"}));
  CHECK_DELRANGE(
      "2-2", strv({"1  one", "3  three", "4  four", "5  \\history del 2-2"}));
  EXPECT_TRUE(m_capture.empty());

  LOAD_HISTORY(strv({"one", "two", "three", "four"}));
  CHECK_DELRANGE(
      "4-4", strv({"1  one", "2  two", "3  three", "4  \\history del 4-4"}));
  EXPECT_TRUE(m_capture.empty());

  LOAD_HISTORY(strv({"one", "two", "three", "four"}));
  CHECK_DELRANGE("5-5", strv({"1  one", "2  two", "3  three", "4  four",
                              "5  \\history del 5-5"}));
  EXPECT_EQ("Invalid history range: 5-5 - valid range is 1-4\n", m_capture);

  // range of 2
  m_capture.clear();
  LOAD_HISTORY(strv({"one", "two", "three", "four"}));
  CHECK_DELRANGE("1-2", strv({"3  three", "4  four", "5  \\history del 1-2"}));
  EXPECT_TRUE(m_capture.empty());
  // (continuing) start outside, end inside
  m_capture.clear();
  CHECK_DELRANGE("1-3", strv({"3  three", "4  four", "5  \\history del 1-2",
                              "6  \\history del 1-3"}));
  EXPECT_EQ("Invalid history range: 1-3 - valid range is 3-5\n", m_capture);

  m_capture.clear();
  LOAD_HISTORY(strv({"one", "two", "three", "four"}));
  CHECK_DELRANGE("2-3", strv({"1  one", "4  four", "5  \\history del 2-3"}));
  EXPECT_TRUE(m_capture.empty());
  //  shell._history.dump([](const std::string &s) { std::cout << s << "\n"; });

  // inverted range
  m_capture.clear();
  LOAD_HISTORY(strv({"one", "two", "three", "four"}));
  CHECK_DELRANGE("2-1", strv({"1  one", "2  two", "3  three", "4  four",
                              "5  \\history del 2-1"}));
  EXPECT_EQ("Invalid history range 2-1. Last item must be greater than first\n",
            m_capture);

  // start inside, end outside
  m_capture.clear();
  LOAD_HISTORY(strv({"one", "two", "three", "four"}));
  CHECK_DELRANGE("3-5", strv({"1  one", "2  two", "3  \\history del 3-5"}));

  // start outside, end outside
  m_capture.clear();
  LOAD_HISTORY(strv({"one", "two", "three", "four"}));
  CHECK_DELRANGE(
      "1-1", strv({"2  two", "3  three", "4  four", "5  \\history del 1-1"}));
  CHECK_DELRANGE("1-5", strv({"2  two", "3  three", "4  four",
                              "5  \\history del 1-1", "6  \\history del 1-5"}));
  EXPECT_EQ("Invalid history range: 1-5 - valid range is 2-5\n", m_capture);

  // outside to end
  m_capture.clear();
  LOAD_HISTORY(strv({"one", "two", "three", "four"}));
  CHECK_DELRANGE(
      "1-1", strv({"2  two", "3  three", "4  four", "5  \\history del 1-1"}));
  CHECK_DELRANGE("1-", strv({"2  two", "3  three", "4  four",
                             "5  \\history del 1-1", "6  \\history del 1-"}));
  EXPECT_EQ("Invalid history range: 1- - valid range is 2-5\n", m_capture);

  m_capture.clear();
  LOAD_HISTORY(strv({"one", "two", "three", "four"}));
  CHECK_DELRANGE("5-", strv({"1  one", "2  two", "3  three", "4  four",
                             "5  \\history del 5-"}));
  EXPECT_EQ("Invalid history range: 5- - valid range is 1-4\n", m_capture);

  // middle to end
  m_capture.clear();
  LOAD_HISTORY(strv({"one", "two", "three", "four"}));
  CHECK_DELRANGE("3-", strv({"1  one", "2  two", "3  \\history del 3-"}));
  EXPECT_TRUE(m_capture.empty());

  // last to end
  m_capture.clear();
  LOAD_HISTORY(strv({"one", "two", "three", "four"}));
  CHECK_DELRANGE("4-",
                 strv({"1  one", "2  two", "3  three", "4  \\history del 4-"}));
  EXPECT_TRUE(m_capture.empty());

  m_capture.clear();
  LOAD_HISTORY(strv({"one", "two", "three", "four"}));
  CHECK_DELRANGE("5-", strv({"1  one", "2  two", "3  three", "4  four",
                             "5  \\history del 5-"}));
  EXPECT_EQ("Invalid history range: 5- - valid range is 1-4\n", m_capture);

  m_capture.clear();
  LOAD_HISTORY(strv({"one", "two", "three"}));
  CHECK_DELRANGE("-2", strv({"1  one", "2  \\history del -2"}));
  EXPECT_TRUE(m_capture.empty());

  m_capture.clear();
  LOAD_HISTORY(strv({"one", "two", "three"}));
  CHECK_DELRANGE("-5", strv({"1  \\history del -5"}));
  EXPECT_TRUE(m_capture.empty());

  m_capture.clear();
  LOAD_HISTORY(strv({"one", "two", "three", "four"}));
  CHECK_DELRANGE("3",
                 strv({"1  one", "2  two", "4  four", "5  \\history del 3"}));
  CHECK_DELRANGE("-3", strv({"1  one", "2  two", "3  \\history del -3"}));
  EXPECT_TRUE(m_capture.empty());

  // invalid
  m_capture.clear();
  LOAD_HISTORY(strv({"one", "two", "three", "four"}));
  CHECK_DELRANGE("-1-2", strv({"1  one", "2  two", "3  three", "4  four",
                               "5  \\history del -1-2"}));
  EXPECT_EQ(
      "\\history delete range argument needs to be in format first-last\n",
      m_capture);

  // small history size
  m_capture.clear();
  LOAD_HISTORY(strv({"one", "two", "three"}));
  shell._history.set_limit(3);
  CHECK_DELRANGE("3", strv({"1  one", "2  two", "3  \\history del 3"}));
  EXPECT_TRUE(m_capture.empty());

  m_capture.clear();
  LOAD_HISTORY(strv({"one", "two", "three"}));
  shell._history.set_limit(3);
  CHECK_DELRANGE("1", strv({"2  two", "3  three", "4  \\history del 1"}));
  EXPECT_EQ("", m_capture);

  m_capture.clear();
  LOAD_HISTORY(strv({"one", "two"}));
  shell._history.set_limit(2);
  CHECK_DELRANGE("2", strv({"1  one", "2  \\history del 2"}));
  EXPECT_TRUE(m_capture.empty());

  m_capture.clear();
  LOAD_HISTORY(strv({"one", "two"}));
  shell._history.set_limit(2);
  CHECK_DELRANGE("1", strv({"2  two", "3  \\history del 1"}));
  EXPECT_EQ("", m_capture);

  m_capture.clear();
  LOAD_HISTORY(strv({"one"}));
  shell._history.set_limit(1);
  CHECK_DELRANGE("1", strv({"1  \\history del 1"}));
  EXPECT_TRUE(m_capture.empty());

  m_capture.clear();
  shell._history.clear();
  shell._history.set_limit(0);
  CHECK_DELRANGE("1", strv({}));
  EXPECT_EQ("The history is already empty\n", m_capture);
  m_capture.clear();
  CHECK_DELRANGE("0", strv({}));
  EXPECT_EQ("The history is already empty\n", m_capture);

  // load and shrink
  m_capture.clear();
  LOAD_HISTORY(strv({"one", "two", "three"}));
  shell._history.set_limit(1);
  CHECK_DELRANGE("0", strv({"2  \\history del 0"}));
  EXPECT_EQ("Invalid history entry: 0 - valid range is 1-1\n", m_capture);

#undef CHECK_DELRANGE
  shcore::delete_file("testhistory");
}

TEST_F(Shell_history, history_numbering) {
  mysqlsh::Command_line_shell shell(
      std::make_shared<Shell_options>(0, nullptr, _options_file));
  enable_capture();
  using strv = std::vector<std::string>;

  shell._history.set_limit(10);

#define CHECK_NUMBERING_ADD(line, expected)                                    \
  {                                                                            \
    strv dump;                                                                 \
    SCOPED_TRACE(line);                                                        \
    shell.process_line(line);                                                  \
    EXPECT_TRUE(m_capture.empty());                                            \
    shell._history.dump([&dump](const std::string &s) { dump.push_back(s); }); \
    for (strv::size_type i = 0; i < dump.size(); ++i) {                        \
      EXPECT_TRUE(i < expected.size());                                        \
      EXPECT_EQ(expected[i], shcore::str_strip(dump[i]));                      \
    }                                                                          \
    EXPECT_EQ(expected.size(), dump.size());                                   \
  }

  // Test sequential numbering of history
  CHECK_NUMBERING_ADD("session", (strv{"1  session"}));
  CHECK_NUMBERING_ADD("dba", (strv{"1  session", "2  dba"}));
  CHECK_NUMBERING_ADD("util", (strv{"1  session", "2  dba", "3  util"}));

  // Must reset to 0 on clear
  shell._history.clear();
  CHECK_NUMBERING_ADD("mysqlx", (strv{"1  mysqlx"}));

  // Must reset on load
  shcore::create_file("testhistory", "mysql\nmysqlx\n");
  shell._history.load("testhistory");
  CHECK_NUMBERING_ADD("shell", (strv{"1  mysql", "2  mysqlx", "3  shell"}));

  // Adding a duplicate item ignores the duplicate
  shell._history.clear();
  CHECK_NUMBERING_ADD("session", (strv{"1  session"}));
  CHECK_NUMBERING_ADD("dba", (strv{"1  session", "2  dba"}));
  CHECK_NUMBERING_ADD("dba", (strv{"1  session", "2  dba"}));
  CHECK_NUMBERING_ADD("util", (strv{"1  session", "2  dba", "3  util"}));
  CHECK_NUMBERING_ADD("dba",
                      (strv{"1  session", "2  dba", "3  util", "4  dba"}));

  // Continued sequential numbering after filling up history
  shell._history.clear();
  shell._history.set_limit(3);
  CHECK_NUMBERING_ADD("session", (strv{"1  session"}));
  CHECK_NUMBERING_ADD("dba", (strv{"1  session", "2  dba"}));
  CHECK_NUMBERING_ADD("util", (strv{"1  session", "2  dba", "3  util"}));
  CHECK_NUMBERING_ADD("mysql", (strv{"2  dba", "3  util", "4  mysql"}));
  CHECK_NUMBERING_ADD("mysqlx", (strv{"3  util", "4  mysql", "5  mysqlx"}));

  // Delete 1st id
  shell._history.clear();
  shell._history.set_limit(10);
  CHECK_NUMBERING_ADD("session", (strv{"1  session"}));
  CHECK_NUMBERING_ADD("dba", (strv{"1  session", "2  dba"}));
  CHECK_NUMBERING_ADD("\\history del 1",
                      (strv{"2  dba", "3  \\history del 1"}));

  // Deleting an id that was already deleted is a no-op
  shell._history.clear();
  shell._history.set_limit(10);
  CHECK_NUMBERING_ADD("session", (strv{"1  session"}));
  CHECK_NUMBERING_ADD("dba", (strv{"1  session", "2  dba"}));
  CHECK_NUMBERING_ADD("util", (strv{"1  session", "2  dba", "3  util"}));
  CHECK_NUMBERING_ADD("mysql",
                      (strv{"1  session", "2  dba", "3  util", "4  mysql"}));

  CHECK_NUMBERING_ADD(
      "\\history del 3",
      (strv{"1  session", "2  dba", "4  mysql", "5  \\history del 3"}));

  CHECK_NUMBERING_ADD("shell", (strv{"1  session", "2  dba", "4  mysql",
                                     "5  \\history del 3", "6  shell"}));

  shell.process_line("\\history del 3");
  CHECK_NUMBERING_ADD(
      "\\history del 3",
      (strv{"1  session", "2  dba", "4  mysql", "5  \\history del 3",
            "6  shell", "7  \\history del 3"}));

  // Deleting the item at the limit
  shell._history.clear();
  shell._history.set_limit(4);
  CHECK_NUMBERING_ADD("session", (strv{"1  session"}));
  CHECK_NUMBERING_ADD("dba", (strv{"1  session", "2  dba"}));
  CHECK_NUMBERING_ADD("util", (strv{"1  session", "2  dba", "3  util"}));
  CHECK_NUMBERING_ADD("mysql",
                      (strv{"1  session", "2  dba", "3  util", "4  mysql"}));
  CHECK_NUMBERING_ADD(
      "\\history del 4",
      (strv{"1  session", "2  dba", "3  util", "4  \\history del 4"}));
  shell._history.clear();
  shell._history.set_limit(4);
  CHECK_NUMBERING_ADD("session", (strv{"1  session"}));
  CHECK_NUMBERING_ADD("dba", (strv{"1  session", "2  dba"}));
  CHECK_NUMBERING_ADD("util", (strv{"1  session", "2  dba", "3  util"}));
  CHECK_NUMBERING_ADD("mysql",
                      (strv{"1  session", "2  dba", "3  util", "4  mysql"}));
  m_capture.clear();
  shell.process_line("\\history del 5");
  EXPECT_EQ("Invalid history entry: 5 - valid range is 1-4\n", m_capture);

#undef CHECK_NUMBERING
  shcore::delete_file("testhistory");
}

TEST_F(Shell_history, never_filter_latest) {
  mysqlsh::Command_line_shell shell(
      std::make_shared<Shell_options>(0, nullptr, _options_file));

  shell._history.set_limit(4);

  // Bug #28749037 COMMAND HISTORY SHOULD KEEP ALL HISTORY ENTRIES FOR AT
  // LEAST ONE ENTRY
  shell.process_line("\\sql");
  shell._history.clear();
  shell.process_line("select 1;");
  shell.process_line("set password='secret';");
  EXPECT_EQ(2, linenoiseHistorySize());
  EXPECT_STREQ("select 1;", linenoiseHistoryLine(0));
  EXPECT_STREQ("set password='secret';", linenoiseHistoryLine(1));

  // this should clear the set password line
  shell._history.save("testhistory");
  std::string data = shcore::get_text_file("testhistory");
  EXPECT_EQ("select 1;\n", data);
  shcore::delete_file("testhistory");

  EXPECT_EQ(1, linenoiseHistorySize());
  EXPECT_STREQ("select 1;", linenoiseHistoryLine(0));

  shell.process_line("set password='secret';");
  // this should clear the set password line again
  shell.process_line("select 2;");

  EXPECT_EQ(2, linenoiseHistorySize());
  EXPECT_STREQ("select 1;", linenoiseHistoryLine(0));
  EXPECT_STREQ("select 2;", linenoiseHistoryLine(1));
}

TEST_F(Shell_history, history_split_by_mode) {
  std::string sql_history_file;
  std::string scripting_history_file;

  {
    char *args[] = {const_cast<char *>("ut"), const_cast<char *>("--sql"),
                    const_cast<char *>("--interactive"), nullptr};
    mysqlsh::Command_line_shell shell(
        std::make_shared<Shell_options>(2, args, _options_file));

    sql_history_file = shell.history_file();
    EXPECT_EQ(0, linenoiseHistorySize());
    shell.get_options()->set("history.autoSave", shcore::Value::True());

    shell.process_line("select 1;\n");
    EXPECT_EQ(1, linenoiseHistorySize());

    shell.process_line(to_scripting);
    scripting_history_file = shell.history_file();
    EXPECT_EQ(0, linenoiseHistorySize());
    shell.process_line("\\status\n");
    EXPECT_EQ(1, linenoiseHistorySize());

    shell.process_line("\\sql");
    EXPECT_EQ(2, linenoiseHistorySize());
    EXPECT_STREQ(to_scripting.c_str(), linenoiseHistoryLine(1));

    shell.process_line(to_scripting);
    EXPECT_EQ(2, linenoiseHistorySize());
    EXPECT_STREQ("\\sql", linenoiseHistoryLine(1));
  }

  shcore::delete_file(sql_history_file);
  shcore::delete_file(scripting_history_file);
}

TEST_F(Shell_history, migrate_old_history) {
  int history_size = 0;
  char *args[] = {const_cast<char *>("ut"), const_cast<char *>("--sql"),
                  nullptr};
  {
    mysqlsh::Command_line_shell shell(
        std::make_shared<Shell_options>(2, args, _options_file));
    EXPECT_FALSE(shcore::is_file(shell.history_file()));
    shell.process_line("select 1;\n");
    history_size = linenoiseHistorySize();
    shell.process_line("\\history save\n");
    const auto hist_file = shell.history_file();
    ASSERT_TRUE(shcore::is_file(hist_file));
    shcore::rename_file(hist_file, hist_file.substr(0, hist_file.rfind('.')));
  }

  mysqlsh::Command_line_shell shell(
      std::make_shared<Shell_options>(2, args, _options_file));
  shell.load_state();
  EXPECT_EQ(history_size, linenoiseHistorySize());
  EXPECT_TRUE(
      shcore::is_file(shell.history_file(shcore::IShell_core::Mode::SQL)));
  shcore::delete_file(shell.history_file(shcore::IShell_core::Mode::SQL));
#ifdef HAVE_V8
  shell.process_line("\\js\n");
  EXPECT_EQ(history_size, linenoiseHistorySize());
  EXPECT_TRUE(shcore::is_file(
      shell.history_file(shcore::IShell_core::Mode::JavaScript)));
  shcore::delete_file(
      shell.history_file(shcore::IShell_core::Mode::JavaScript));
#endif
#ifdef HAVE_PYTHON
  shell.process_line("\\py\n");
  EXPECT_EQ(history_size, linenoiseHistorySize());
  EXPECT_TRUE(
      shcore::is_file(shell.history_file(shcore::IShell_core::Mode::Python)));
  shcore::delete_file(shell.history_file(shcore::IShell_core::Mode::Python));
#endif
}

TEST_F(Shell_history, get_entry) {
  mysqlsh::Command_line_shell shell(
      std::make_shared<Shell_options>(0, nullptr, _options_file));

  const auto &history = shell._history;

  EXPECT_EQ("", history.get_entry(5));

  shell.process_line("a = 1");
  EXPECT_EQ("a = 1", history.get_entry(history.first_entry()));
  EXPECT_EQ("a = 1", history.get_entry(history.last_entry()));

  shell.process_line("b = 2");
  EXPECT_EQ("a = 1", history.get_entry(history.first_entry()));
  EXPECT_EQ("b = 2", history.get_entry(history.last_entry()));

  shell.process_line("c = 3");
  EXPECT_EQ("a = 1", history.get_entry(history.first_entry()));
  EXPECT_EQ("c = 3", history.get_entry(history.last_entry()));
}

}  // namespace mysqlsh
