/*
 * Copyright (c) 2017, Oracle and/or its affiliates. All rights reserved.
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
#ifndef _WIN32
#include <sys/stat.h>
#endif
#include "ext/linenoise-ng/include/linenoise.h"
#include "mysqlshdk/libs/utils/utils_file.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_string.h"
#include "modules/mod_shell_options.h"
#include "src/mysqlsh/cmdline_shell.h"
#include "modules/mod_shell.h"
#include "unittest/test_utils.h"

namespace mysqlsh {

class Shell_history : public ::testing::Test {
 public:
  virtual void SetUp() {
    linenoiseHistoryFree();
  }
};

TEST_F(Shell_history, check_history_sql_not_connected) {
  // Start in SQL mode, do not connect to MySQL. This will ensure no statement
  // gets executed. However, we promise that all user input will be recorded
  // in the history - no matther what all user input shall be recorded.
  char *args[] = {const_cast<char *>("ut"), const_cast<char *>("--sql"),
                  nullptr};
  mysqlsh::Command_line_shell shell(std::make_shared<Shell_options>(2, args));

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

  char *args[] = {const_cast<char *>("ut"), const_cast<char *>("--sql"),
                  const_cast<char *>(shell_test_server_uri().c_str()), nullptr};
  mysqlsh::Command_line_shell shell(std::make_shared<Shell_options>(3, args));
  shell._history.set_limit(100);

  const char *pwd = getenv("MYSQL_PWD");
  auto coptions = shcore::get_connection_options(getenv("MYSQL_URI"));
  coptions.set_scheme("mysql");
  if (pwd)
    coptions.set_password(pwd);
  else
    coptions.set_password("");
  coptions.set_port(atoi(getenv("MYSQL_PORT")));
  shell.connect(coptions, false);

  // TS_CV#9
  EXPECT_EQ("*IDENTIFIED*:*PASSWORD*",
      shell.get_options()->get("history.sql.ignorePattern").descr());

  EXPECT_EQ(0, linenoiseHistorySize());

  shell.process_line("select 1;");

  EXPECT_EQ(1, linenoiseHistorySize());
  EXPECT_STREQ("select 1;", linenoiseHistoryLine(0));

  // ensure certain words don't appear in history
  shell.process_line("set password = 'secret' then fail;");
  EXPECT_EQ(1, linenoiseHistorySize());
  EXPECT_STREQ("select 1;", linenoiseHistoryLine(0));

  shell.process_line("create user foo@bar identified by 'secret';");
  EXPECT_EQ(1, linenoiseHistorySize());
  EXPECT_STREQ("select 1;", linenoiseHistoryLine(0));

  shell.process_line("alter user foo@bar set password='secret';");
  EXPECT_EQ(1, linenoiseHistorySize());
  EXPECT_STREQ("select 1;", linenoiseHistoryLine(0));

  shell.process_line("secret;");
  EXPECT_EQ(2, linenoiseHistorySize());
  EXPECT_STREQ("select 1;", linenoiseHistoryLine(0));
  EXPECT_STREQ("secret;", linenoiseHistoryLine(1));

  // set filter directly
  shell.set_sql_safe_for_logging("*SECret*");

  shell.process_line("top secret;");
  EXPECT_EQ(2, linenoiseHistorySize());
  EXPECT_STREQ("select 1;", linenoiseHistoryLine(0));
  EXPECT_STREQ("secret;", linenoiseHistoryLine(1));

  // SQL filter only applies to SQL mode
  shell.process_line("\\js");
  shell.process_line("println('secret');");
  EXPECT_EQ(4, linenoiseHistorySize());
  EXPECT_STREQ("select 1;", linenoiseHistoryLine(0));
  EXPECT_STREQ("secret;", linenoiseHistoryLine(1));
  EXPECT_STREQ("\\js", linenoiseHistoryLine(2));
  EXPECT_STREQ("println('secret');", linenoiseHistoryLine(3));

  shell.process_line("\\py");
  shell.process_line("print 'secret'");
  EXPECT_EQ(6, linenoiseHistorySize());
  EXPECT_STREQ("select 1;", linenoiseHistoryLine(0));
  EXPECT_STREQ("secret;", linenoiseHistoryLine(1));
  EXPECT_STREQ("\\js", linenoiseHistoryLine(2));
  EXPECT_STREQ("println('secret');", linenoiseHistoryLine(3));
  EXPECT_STREQ("\\py", linenoiseHistoryLine(4));
  EXPECT_STREQ("print 'secret'", linenoiseHistoryLine(5));

  // unset filter via shell options
  shcore::Mod_shell_options opts(shell.get_options());
  opts.set_member("history.sql.ignorePattern", shcore::Value(""));

  shell.process_line("top secret;");
  EXPECT_EQ(7, linenoiseHistorySize());
  EXPECT_STREQ("top secret;", linenoiseHistoryLine(6));

  // TS_CV#6
  // shell.options["history.sql.ignorePattern"]=string:string:string:string with
  // multiple strings separated by a colon (:) works correctly
  // TS_CV#7
  // TS_CV#8
  shell.process_line("\\js");
  shell.process_line(
      "shell.options['history.sql.ignorePattern'] = '*bla*:*ble*';");
  shell.process_line("\\sql");
  shell.process_line("select 'bga';");
  shell.process_line("select 'bge';");
  shell.process_line("select 'aablaaa';");
  shell.process_line("select 'aaBLEaa';");
  shell.process_line("select 'bla';");
  shell.process_line("select '*bla*';");
  shell.process_line("select 'bgi';");
  EXPECT_STREQ("\\js", linenoiseHistoryLine(7));
  EXPECT_STREQ("shell.options['history.sql.ignorePattern'] = '*bla*:*ble*';",
               linenoiseHistoryLine(8));
  EXPECT_STREQ("\\sql", linenoiseHistoryLine(9));
  EXPECT_STREQ("select 'bga';", linenoiseHistoryLine(10));
  EXPECT_STREQ("select 'bge';", linenoiseHistoryLine(11));
  EXPECT_STREQ("select 'bgi';", linenoiseHistoryLine(12));

  shell.process_line("\\js");
  shell.process_line("shell.options['history.sql.ignorePattern'] = 'set;';");
  shell.process_line("\\sql");
  shell.process_line("\\history clear");
  EXPECT_EQ(1, linenoiseHistorySize());
  shell.process_line("bar;");
  EXPECT_EQ(2, linenoiseHistorySize());
  EXPECT_STREQ("bar;", linenoiseHistoryLine(1));
  shell.process_line("set;");
  EXPECT_EQ(2, linenoiseHistorySize());
  EXPECT_STREQ("bar;", linenoiseHistoryLine(1));
  shell.process_line("xset;");
  EXPECT_EQ(3, linenoiseHistorySize());
  EXPECT_STREQ("xset;", linenoiseHistoryLine(2));

  shell.process_line("\\js");
  shell.process_line("shell.options['history.sql.ignorePattern'] = '*';");
  shell.process_line("\\sql");
  shell.process_line("\\history clear");
  EXPECT_EQ(0, linenoiseHistorySize());
  shell.process_line("a;");
  EXPECT_EQ(0, linenoiseHistorySize());
  shell.process_line("hello world;");
  EXPECT_EQ(0, linenoiseHistorySize());
  shell.process_line("*;");
  EXPECT_EQ(0, linenoiseHistorySize());
  shell.process_line(";");
  EXPECT_EQ(0, linenoiseHistorySize());

  shell.process_line("\\js");
  shell.process_line("shell.options['history.sql.ignorePattern'] = '**';");
  shell.process_line("\\sql");
  shell.process_line("\\history clear");
  EXPECT_EQ(0, linenoiseHistorySize());
  shell.process_line("a;");
  EXPECT_EQ(0, linenoiseHistorySize());
  shell.process_line("hello world;");
  EXPECT_EQ(0, linenoiseHistorySize());
  shell.process_line("*;");
  EXPECT_EQ(0, linenoiseHistorySize());
  shell.process_line(";");
  EXPECT_EQ(0, linenoiseHistorySize());

  shell.process_line("\\js");
  shell.process_line("shell.options['history.sql.ignorePattern'] = '?';");
  shell.process_line("\\sql");
  shell.process_line("\\history clear");
  EXPECT_EQ(1, linenoiseHistorySize());
  shell.process_line(";");
  EXPECT_EQ(1, linenoiseHistorySize());
  shell.process_line("a;");
  EXPECT_EQ(2, linenoiseHistorySize());
  shell.process_line("aa;");
  EXPECT_EQ(3, linenoiseHistorySize());

  shell.process_line("\\js");
  shell.process_line("shell.options['history.sql.ignorePattern'] = '?*';");
  shell.process_line("\\sql");
  shell.process_line("\\history clear");
  EXPECT_EQ(0, linenoiseHistorySize());
  shell.process_line("?;");
  EXPECT_EQ(0, linenoiseHistorySize());
  shell.process_line("a;");
  EXPECT_EQ(0, linenoiseHistorySize());
  shell.process_line("aa;");
  EXPECT_EQ(0, linenoiseHistorySize());

  shell.process_line("\\js");
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
  EXPECT_EQ(4, linenoiseHistorySize());

  shell.process_line("axbxc;");
  EXPECT_EQ(4, linenoiseHistorySize());
  shell.process_line("axbxcdddeefg;");
  EXPECT_EQ(4, linenoiseHistorySize());
  shell.process_line("a b c;");
  EXPECT_EQ(4, linenoiseHistorySize());
}

TEST_F(Shell_history, history_ignore_wildcard_questionmark) {
  // WL#10446 TS_CV#8 - check wildcards, * is covered elsewhere, ? is covered
  // here

  linenoiseHistoryFree();
  // NOTE: resizing it may cause troubles with \history dump indexing
  // because we do not call the approriate API
  linenoiseHistorySetMaxLen(100);

  char *args[] = {const_cast<char*>("ut"), const_cast<char*>("--sql"), nullptr};
  mysqlsh::Command_line_shell shell(std::make_shared<Shell_options>(2, args));

  // ? = match exactly one
  EXPECT_EQ(0, linenoiseHistorySize());
  shell.process_line("\\js");
  shell.process_line(
      "shell.options['history.sql.ignorePattern'] = '?ELECT 1;'");
  shell.process_line("\\sql");
  EXPECT_EQ(3, linenoiseHistorySize());

  shell.process_line("SELECT 1;");
  EXPECT_EQ(3, linenoiseHistorySize());

  shell.process_line("ELECT 1;");
  EXPECT_EQ(4, linenoiseHistorySize());

  shell.process_line("\\js");
  shell.process_line(
      "shell.options['history.sql.ignorePattern'] = '?? ??;:?\?'");
  shell.process_line("\\sql");
  EXPECT_EQ(7, linenoiseHistorySize());

  shell.process_line("AA BB;");
  EXPECT_EQ(7, linenoiseHistorySize());

  shell.process_line("A;");
  EXPECT_EQ(7, linenoiseHistorySize());

  shell.process_line(" A\n  ;");
  EXPECT_EQ(8, linenoiseHistorySize());
}

TEST_F(Shell_history, history_set_option) {
  // All user input shall be caputered in the history
  //
  // cmdline_shell.cc catches asssorted "events" that are generated upon user
  // input To test the requirement we need to try to trick base_shell.cc and
  // find events that are not properly handled or even missing ones. Most other
  // tests check the event SN_STATEMENT_EXECUTED.

  linenoiseHistoryFree();

  mysqlsh::Command_line_shell shell(std::make_shared<Shell_options>());

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
  mysqlsh::Command_line_shell shell(std::make_shared<Shell_options>());

  shell.process_line("shell.options['history.sql.ignorePattern'] = '*SELECT*'");
  shell.process_line("// SELECT");

  EXPECT_STREQ("// SELECT", linenoiseHistoryLine(1));
}

TEST_F(Shell_history, history_ignore_pattern_py) {
  char *args[] = {const_cast<char *>("ut"), const_cast<char *>("--py"),
                  nullptr};
  mysqlsh::Command_line_shell shell(std::make_shared<Shell_options>(2, args));

  shell.process_line(
      "setattr(shell.options, 'history.sql.ignorePattern', "
      "'WHAT A PROPERTY');");
  EXPECT_EQ(1, linenoiseHistorySize());
  EXPECT_STREQ(
      "setattr(shell.options, 'history.sql.ignorePattern', 'WHAT A PROPERTY');",
      linenoiseHistoryLine(0));

// FIXME uncomment when python comment handling is fixed
#if 0
  // Second issue - comments not recorded in history
  shell.process_line("# WHAT A PROPERTY NAME");
  EXPECT_EQ(2, linenoiseHistorySize());
  EXPECT_STREQ("# WHAT A PROPERTY NAME", linenoiseHistoryLine(1));
#endif
}

static void print_capture(void *cdata, const char *text) {
  std::string *capture = static_cast<std::string *>(cdata);
  capture->append(text).append("\n");
}

TEST_F(Shell_history, history_linenoise) {
  // Test cases covered here:
  // TS_CLE#1 Commands executed by the user in the shell are saved to the
  // command history
  // TS_HM#1 only commands interactively typed by the user in the shell prompt
  // are saved to history file: ~/.mysqlsh/history file

  shcore::delete_file(shcore::get_user_config_path() + "/history");

  {
    mysqlsh::Command_line_shell shell(std::make_shared<Shell_options>());

    EXPECT_NO_THROW(shell.load_state(shcore::get_user_config_path()));

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

    shcore::Mod_shell_options opt(shell.get_options());

    // TS_CV#10
    // autosave off by default
    EXPECT_FALSE(opt.get_member("history.autoSave").as_bool());

    // check history autosave
    shell.save_state(shcore::get_user_config_path());
    EXPECT_FALSE(
        shcore::file_exists(shcore::get_user_config_path() + "/history"));

    opt.set_member("history.autoSave", shcore::Value::True());

    shell.save_state(shcore::get_user_config_path());
    EXPECT_TRUE(
        shcore::file_exists(shcore::get_user_config_path() + "/history"));

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

    std::string capture;
    shell._delegate->print = print_capture;
    shell._delegate->print_error = print_capture;
    shell._delegate->user_data = &capture;

    capture.clear();
    shell.process_line("\\help history");
    EXPECT_EQ(
        "View and edit command line history.\n\n"
        "NAME: \\history\n\n"
        "SYNTAX:\n"
        "   \\history                     Display history entries.\n"
        "   \\history del <num>[-<num>]   Delete entry/entries from history.\n"
        "   \\history clear               Clear history.\n"
        "   \\history save                Save history to file.\n\n"
        "EXAMPLES:\n"
        "   \\history\n"
        "   \\history del 123       Delete entry number 123\n"
        "   \\history del 10-20     Delete entries 10 to 20\n"
        "   \\history del 10-       Delete entries from 10 to last\n"
        "\n"
        "NOTE: The history.autoSave shell option may be set to true to "
        "automatically save the contents of the command history when the shell "
        "exits.\n\n",
        capture);

    // TS_SC#1
    capture.clear();
    shell.process_line("\\history");
    EXPECT_EQ("    5  \\help history\n\n", capture);

    capture.clear();
    // TS_SC#3
    shell.process_line("\\history del 6");
    EXPECT_EQ("", capture);

    capture.clear();
    shell.process_line("\\history");
    EXPECT_EQ("    7  \\history del 6\n\n", capture);

    capture.clear();
    shell.process_line("print(5);");

    capture.clear();
    shell.process_line("\\history 1");
    EXPECT_EQ(
        "Invalid options for \\history. See \\help history for syntax.\n\n",
        capture);

    capture.clear();
    shell.process_line("\\history 1 x");
    EXPECT_EQ(
        "Invalid options for \\history. See \\help history for syntax.\n\n",
        capture);

    capture.clear();
    shell.process_line("\\history x 1");
    EXPECT_EQ(
        "Invalid options for \\history. See \\help history for syntax.\n\n",
        capture);

    capture.clear();
    shell.process_line("\\history clear 1");
    EXPECT_EQ("\\history clear does not take any parameters\n\n", capture);

    capture.clear();
    shell.process_line("\\history del");
    EXPECT_EQ("\\history delete requires entry number to be deleted\n\n",
              capture);

    capture.clear();
    shell.process_line("\\history del 50");
    EXPECT_EQ("Invalid history entry 50\n\n", capture);

    capture.clear();
    shell.process_line("\\history 0 -1 -1");
    EXPECT_EQ(
        "Invalid options for \\history. See \\help history for syntax.\n\n",
        capture);

    // TS_SC#4 - cancelled

    capture.clear();
    // TS_SC#5
    shell.process_line("\\history clear");
    EXPECT_EQ("", capture);

    capture.clear();
    shell.process_line("\\history");
    EXPECT_EQ("    1  \\history clear\n\n", capture);
    EXPECT_EQ(1, shell._history.size());
    shell.load_state(shcore::get_user_config_path());
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
    capture.clear();
    shell.process_line("shell.options['history.maxSize']=0;\n");
    EXPECT_EQ(0, shell._history.size());

    capture.clear();
    shell.process_line("\\history\n");
    EXPECT_EQ("", capture);
    EXPECT_STREQ(NULL, linenoiseHistoryLine(1));

    capture.clear();
    // TS_SC#2
    shell.process_line("\\history save\n");
    EXPECT_EQ("Command history file saved with 0 entries.\n\n", capture);

    std::string hdata;
    shcore::load_text_file(shcore::get_user_config_path() + "/history", hdata);
    EXPECT_EQ("", hdata);
  }
  shcore::delete_file(shcore::get_user_config_path() + "/history");
}

TEST_F(Shell_history, check_help_shows_history) {
  // We should have one test that checks the output of \h for compleness
  // TODO(ulf) (Remove my [Ulf] note when checked): I don't mind if we have one
  // central UT for \h or every command we introduce has it's own tests.
  // My preference is a global one but I'm not the one to maintain the UTs so
  // I'll just do something and let the owners of the UTs decide.

  mysqlsh::Command_line_shell shell(std::make_shared<Shell_options>());

  {
    // Expecting the Shell mention \history in \help
    std::string capture;
    shell._delegate->print = print_capture;
    shell._delegate->print_error = print_capture;
    shell._delegate->user_data = &capture;

    capture.clear();
    shell.process_line("\\help");
    EXPECT_TRUE(
        strstr(capture.c_str(),
               "\\history            View and edit command line history."));
  }
}

TEST_F(Shell_history, history_autosave_int) {
  shcore::delete_file(shcore::get_user_config_path() + "/history");

  mysqlsh::Command_line_shell shell(std::make_shared<Shell_options>());

  {
    shcore::Mod_shell_options opt(shell.get_options());

    // Expecting the Shell to cast to boolean true if value is 1 or 0
    opt.set_member("history.autoSave", shcore::Value(1));
    EXPECT_TRUE(opt.get_member("history.autoSave").as_bool());

    // Expecting the Shell to print history.autoSave = true not 101
    std::string capture;
    shell._delegate->print = print_capture;
    shell._delegate->print_error = print_capture;
    shell._delegate->user_data = &capture;

    capture.clear();
    shell.process_line("print(shell.options)");
    // we perform automatic type conversion on set
    EXPECT_TRUE(strstr(capture.c_str(), "\"history.autoSave\": true"));
  }
}

TEST_F(Shell_history, check_history_source) {
  // WL#10446 says \source shall no add entries to the history
  // Only history entry shall the \source itself

  char *args[] = {const_cast<char *>("ut"), const_cast<char *>("--js"),
                  const_cast<char *>(shell_test_server_uri().c_str()), nullptr};
  mysqlsh::Command_line_shell shell(std::make_shared<Shell_options>(3, args));
  shell._history.set_limit(10);

  std::ofstream of;
  of.open("test_source.js");
  of << "print(1);\n";
  of << "print(2);\n";
  of.close();

  EXPECT_NO_THROW(shell.load_state(shcore::get_user_config_path()));
  EXPECT_EQ(0, linenoiseHistorySize());

  {
    std::string capture;
    shell._delegate->print = print_capture;
    shell._delegate->print_error = print_capture;
    shell._delegate->user_data = &capture;

    shell.process_line("\\source test_source.js");
    EXPECT_EQ(1, linenoiseHistorySize());
    EXPECT_EQ("1\n2\n", capture);
  }

  shcore::delete_file("test_source.js");
}

TEST_F(Shell_history, check_history_overflow_del) {
  // See if the history numbering still works for users when the history
  // overflows, entries are dropped and renumbering might take place.

  {
    mysqlsh::Command_line_shell shell(std::make_shared<Shell_options>());
    shell._history.set_limit(3);

    EXPECT_NO_THROW(shell.load_state(shcore::get_user_config_path()));
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

    std::string capture;
    shell._delegate->print = print_capture;
    shell._delegate->print_error = print_capture;
    shell._delegate->user_data = &capture;

    // Note: history entries are added AFTER they are executed
    // that means if \history is called, it will print the stack up the
    // last command before \history itself

    // Now we print the entries and offsets to be used with history del
    // Did we just pop index 0 off the stack by pushing \\history?
    // A: Yes, but the pop only happens after the history is printed
    shell.process_line("\\history");
    EXPECT_EQ("    1  // 1\n\n    2  // 2\n\n    3  // 3\n\n", capture);
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

    capture.clear();
    shell.process_line("\\history");
    // Actual history is now:
    // 4   \history
    // 5   \history del 2
    // 6   \history
    EXPECT_EQ("    3  // 3\n\n    4  \\history\n\n    5  \\history del 2\n\n",
              capture);
    EXPECT_EQ(3, shell._history.size());
  }
}

TEST_F(Shell_history, history_management) {
  mysqlsh::Command_line_shell shell(std::make_shared<Shell_options>());

  std::string capture;
  shell._delegate->print = print_capture;
  shell._delegate->print_error = print_capture;
  shell._delegate->user_data = &capture;

  shell.process_line("\\js\n");
  shell.process_line("\\history clear\n");
  shell.process_line("shell.options['history.maxSize']=10;\n");

  shcore::create_file("test_file.js", "println('test1');\nprintln('test2');\n");

  std::string histfile = shcore::get_user_config_path() + "/history";
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
  capture.clear();
  shell.process_line("\\history");
  EXPECT_EQ(
      "    1  \\history clear\n\n"
      "    2  println('bar');\n\n"
      "    3  println('foo');\n\n",
      capture);
  capture.clear();
  // ensure ordering after an ignored duplicate item continues sequential
  shell.process_line("\\history");
  EXPECT_EQ(
      "    1  \\history clear\n\n"
      "    2  println('bar');\n\n"
      "    3  println('foo');\n\n"
      "    4  \\history\n\n",
      capture);

  // TS_HM#7 if the history file can't be read or written to, it will log an
  // error message and skip the read/write operation.
  shcore::delete_file(histfile);

  shcore::ensure_dir_exists(histfile.c_str());
  capture.clear();
  shell.process_line("\\history save");
#ifdef _WIN32
  EXPECT_EQ("Could not save command history to ./history: Permission denied\n",
            capture);
#else
  EXPECT_EQ("Could not save command history to ./history: Is a directory\n",
            capture);
#endif
#ifndef _WIN32
  EXPECT_EQ(0, chmod(histfile.c_str(), 0));

  capture.clear();
  EXPECT_NO_THROW(shell.load_state(shcore::get_user_config_path()));
  EXPECT_EQ(
      "Could not load command history from ./history: Permission denied\n\n",
      capture);
#endif

  shcore::remove_directory(histfile, false);
}

TEST_F(Shell_history, history_sizes) {
  // We use a secondary history list to provide entry numbers that do not
  // change when the list contents changes so that \history del works.
  // This test shall cover internal list management (grow, shrink, overflow).
  // No crash is good enough.
  // See also src/mysqlsh/history.cc|h
  mysqlsh::Command_line_shell shell(std::make_shared<Shell_options>());

  std::string capture;
  shell._delegate->print = print_capture;
  shell._delegate->print_error = print_capture;
  shell._delegate->user_data = &capture;

  shell.process_line("shell.options['history.maxSize'] = 4;");
  shell.process_line("print(1);");
  shell.process_line("print(2);");
  shell.process_line("print(3);");
  shell.process_line("print(4);");
  shell.process_line("\\history");

  EXPECT_EQ(4, shell._history.size());

  // NOTE: Always use shell._history.size() to check history size, otherwise
  // you will get different results when test cases are ran on UTs vs manually
  EXPECT_EQ(
      "1\n"
      "2\n"
      "3\n"
      "4\n"
      "    2  print(1);\n\n"
      "    3  print(2);\n\n"
      "    4  print(3);\n\n"
      "    5  print(4);\n\n",
      capture);

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
  capture.clear();
  shell.process_line("\\history");
  EXPECT_EQ(
      "    2  print(42);\n\n"
      "    3  \\history del 3\n\n"
      "    4  print(42);\n\n",
      capture);
}

TEST_F(Shell_history, history_del_invisible_entry) {
  // See also TEST_F(Shell_history, history_sizes)

  mysqlsh::Command_line_shell shell(std::make_shared<Shell_options>());

  std::string capture;
  shell._delegate->print = print_capture;
  shell._delegate->print_error = print_capture;
  shell._delegate->user_data = &capture;

  // Trivial and should be covered elsewhere already
  shell.process_line("\\history del -1");
  EXPECT_TRUE(strstr(capture.c_str(), "Invalid"));

  // History has now one command and it will get the index 1
  // Can we access the invisible implementation dependent index 2?
  capture.clear();
  shell.process_line("\\history del 2");
  EXPECT_TRUE(strstr(capture.c_str(), "Invalid"));
}

TEST_F(Shell_history, history_source_history) {
  // Generate a history, save it, load and execute saved history
  // using \source. \source shall not add any executed commands to history
  // but preserve the history state from after save.

  mysqlsh::Command_line_shell shell(std::make_shared<Shell_options>());

  std::string capture;
  shell._delegate->print = print_capture;
  shell._delegate->print_error = print_capture;
  shell._delegate->user_data = &capture;

  shell.process_line("// 1");
  shell.process_line("// 2");
  shell.process_line("\\history save");

  std::string histfile = shcore::get_user_config_path() + "/history";
  std::string line = "\\source " + histfile;
  shell.process_line(line);
  shell.process_line("\\history");
  EXPECT_EQ(5, shell._history.size());

  EXPECT_EQ(
      "Command history file saved with 2 entries.\n\n"
      "    1  // 1\n\n"
      "    2  // 2\n\n"
      "    3  \\history save\n\n"
      "    4  " +
          line + "\n\n",
      capture);
}

TEST_F(Shell_history, history_del_range) {
  mysqlsh::Command_line_shell shell(std::make_shared<Shell_options>());

  std::string capture;
  shell._delegate->print = print_capture;
  shell._delegate->print_error = print_capture;
  shell._delegate->user_data = &capture;

  shell.process_line("// 1");
  shell.process_line("// 2");
  shell.process_line("// 3");
  shell.process_line("// 4");
  shell.process_line("// 5");
  shell.process_line("// 6");
  EXPECT_EQ(6, shell._history.size());
  // valid range
  shell.process_line("\\history del 1-3");
  shell.process_line("\\history");

  EXPECT_EQ(5, shell._history.size());

  EXPECT_EQ(
      "    4  // 4\n\n"
      "    5  // 5\n\n"
      "    6  // 6\n\n"
      "    7  \\history del 1-3\n\n",
      capture);

  // invalid range: using former entries
  capture.clear();
  shell.process_line("\\history del 1-3");
  EXPECT_EQ("Invalid history entry 1-3\n\n", capture);

  // lower bound bigger than upper bound
  capture.clear();
  shell.process_line("\\history del 7-4");
  EXPECT_EQ(
      "Invalid history range 7-4. Last item must be greater than first\n\n",
      capture);

  shell.process_line("\\history clear");
  shell.process_line("// 1");
  shell.process_line("// 2");
  capture.clear();
  shell.process_line("\\history del 1 - 3");
  // Not sure if we want to give an error here or be gentle and accept space
  EXPECT_EQ("\\history delete requires entry number to be deleted\n\n",
            capture);
}

TEST_F(Shell_history, history_entry_number_reset) {
  // Numbering shall only be reset when Shell is restarted
  // or when \\history clear is called

  mysqlsh::Command_line_shell shell(std::make_shared<Shell_options>());

  std::string capture;
  shell._delegate->print = print_capture;
  shell._delegate->print_error = print_capture;
  shell._delegate->user_data = &capture;

  shell.process_line("// 1");
  shell.process_line("// 2");
  shell.process_line("// 3");
  shell.process_line("\\history");
  EXPECT_EQ(4, shell._history.size());
  EXPECT_EQ(
      "    1  // 1\n\n"
      "    2  // 2\n\n"
      "    3  // 3\n\n",
      capture);

  capture.clear();
  shell.process_line("\\history clear");
  shell.process_line("\\history");
  // This should get number 4, if numbering was not reset
  EXPECT_EQ("    1  \\history clear\n\n", capture);
  EXPECT_EQ(2, shell._history.size());
}

TEST_F(Shell_history, shell_options_help_history) {
  mysqlsh::Command_line_shell shell(std::make_shared<Shell_options>());

  std::string capture;
  shell._delegate->print = print_capture;
  shell._delegate->print_error = print_capture;
  shell._delegate->user_data = &capture;

  shell.process_line("print(shell.help('options'))");

  EXPECT_TRUE(strstr(
      capture.c_str(),
      "history.autoSave: true to save command history when exiting the shell"));
  EXPECT_TRUE(strstr(capture.c_str(),
                     "history.sql.ignorePattern: colon separated list of glob "
                     "patterns to filter"));
  EXPECT_TRUE(strstr(
      capture.c_str(),
      "history.autoSave: true to save command history when exiting the shell"));
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
    for (std::vector<std::string>::size_type i = 0; i < expected.size(); ++i){ \
      EXPECT_TRUE(i < dump.size());                                            \
      EXPECT_EQ(expected[i], shcore::str_strip(dump[i]));                      \
    }                                                                          \
    EXPECT_EQ(expected.size(), dump.size());                                   \
  }

  mysqlsh::Command_line_shell shell(std::make_shared<Shell_options>());
  std::string capture;
  shell._delegate->print = print_capture;
  shell._delegate->print_error = print_capture;
  shell._delegate->user_data = &capture;
  using strv = std::vector<std::string>;
  shell._history.set_limit(10);

  // range of 1
  capture.clear();
  LOAD_HISTORY(strv({"one", "two", "three", "four"}));
  CHECK_DELRANGE(
      "1-1", strv({"2  two", "3  three", "4  four", "5  \\history del 1-1"}));
  EXPECT_TRUE(capture.empty());

  capture.clear();
  LOAD_HISTORY(strv({"one", "two", "three", "four"}));
  CHECK_DELRANGE(
      "2-2", strv({"1  one", "3  three", "4  four", "5  \\history del 2-2"}));
  EXPECT_TRUE(capture.empty());

  LOAD_HISTORY(strv({"one", "two", "three", "four"}));
  CHECK_DELRANGE(
      "4-4", strv({"1  one", "2  two", "3  three", "5  \\history del 4-4"}));
  EXPECT_TRUE(capture.empty());

  LOAD_HISTORY(strv({"one", "two", "three", "four"}));
  CHECK_DELRANGE("5-5", strv({"1  one", "2  two", "3  three", "4  four",
                              "5  \\history del 5-5"}));
  EXPECT_EQ("Invalid history entry 5-5\n\n", capture);

  // range of 2
  capture.clear();
  LOAD_HISTORY(strv({"one", "two", "three", "four"}));
  CHECK_DELRANGE("1-2", strv({"3  three", "4  four", "5  \\history del 1-2"}));
  EXPECT_TRUE(capture.empty());
  // (continuing) start outside, end inside
  capture.clear();
  CHECK_DELRANGE("1-3", strv({"3  three", "4  four", "5  \\history del 1-2",
                              "6  \\history del 1-3"}));
  EXPECT_EQ("Invalid history entry 1-3\n\n", capture);

  capture.clear();
  LOAD_HISTORY(strv({"one", "two", "three", "four"}));
  CHECK_DELRANGE("2-3", strv({"1  one", "4  four", "5  \\history del 2-3"}));
  EXPECT_TRUE(capture.empty());
  //  shell._history.dump([](const std::string &s) { std::cout << s << "\n"; });

  // inverted range
  capture.clear();
  LOAD_HISTORY(strv({"one", "two", "three", "four"}));
  CHECK_DELRANGE("2-1", strv({"1  one", "2  two", "3  three", "4  four",
                              "5  \\history del 2-1"}));
  EXPECT_EQ(
      "Invalid history range 2-1. Last item must be greater than first\n\n",
      capture);

  // start inside, end outside
  capture.clear();
  LOAD_HISTORY(strv({"one", "two", "three", "four"}));
  CHECK_DELRANGE("3-5", strv({"1  one", "2  two", "5  \\history del 3-5"}));

  // start outside, end outside
  capture.clear();
  LOAD_HISTORY(strv({"one", "two", "three", "four"}));
  CHECK_DELRANGE(
      "1-1", strv({"2  two", "3  three", "4  four", "5  \\history del 1-1"}));
  CHECK_DELRANGE("1-5", strv({"2  two", "3  three", "4  four",
                              "5  \\history del 1-1", "6  \\history del 1-5"}));
  EXPECT_EQ("Invalid history entry 1-5\n\n", capture);

  // outside to end
  capture.clear();
  LOAD_HISTORY(strv({"one", "two", "three", "four"}));
  CHECK_DELRANGE(
      "1-1", strv({"2  two", "3  three", "4  four", "5  \\history del 1-1"}));
  CHECK_DELRANGE("1-", strv({"2  two", "3  three", "4  four",
                             "5  \\history del 1-1", "6  \\history del 1-"}));
  EXPECT_EQ("Invalid history entry 1-\n\n", capture);

  capture.clear();
  LOAD_HISTORY(strv({"one", "two", "three", "four"}));
  CHECK_DELRANGE("5-", strv({"1  one", "2  two", "3  three", "4  four",
                             "5  \\history del 5-"}));
  EXPECT_EQ("Invalid history entry 5-\n\n", capture);

  // middle to end
  capture.clear();
  LOAD_HISTORY(strv({"one", "two", "three", "four"}));
  CHECK_DELRANGE("3-", strv({"1  one", "2  two", "5  \\history del 3-"}));
  EXPECT_TRUE(capture.empty());

  // last to end
  capture.clear();
  LOAD_HISTORY(strv({"one", "two", "three", "four"}));
  CHECK_DELRANGE("4-",
                 strv({"1  one", "2  two", "3  three", "5  \\history del 4-"}));
  EXPECT_TRUE(capture.empty());

  capture.clear();
  LOAD_HISTORY(strv({"one", "two", "three", "four"}));
  CHECK_DELRANGE("5-", strv({"1  one", "2  two", "3  three", "4  four",
                             "5  \\history del 5-"}));
  EXPECT_EQ("Invalid history entry 5-\n\n", capture);

  // invalid
  capture.clear();
  LOAD_HISTORY(strv({"one", "two", "three", "four"}));
  CHECK_DELRANGE("-1-2", strv({"1  one", "2  two", "3  three", "4  four",
                               "5  \\history del -1-2"}));
  EXPECT_EQ("Invalid history entry -1-2\n\n", capture);

  // small history size
  capture.clear();
  LOAD_HISTORY(strv({"one", "two", "three"}));
  shell._history.set_limit(3);
  CHECK_DELRANGE("3", strv({"1  one", "2  two", "4  \\history del 3"}));
  EXPECT_TRUE(capture.empty());

  capture.clear();
  LOAD_HISTORY(strv({"one", "two", "three"}));
  shell._history.set_limit(3);
  CHECK_DELRANGE("1", strv({"2  two", "3  three", "4  \\history del 1"}));
  EXPECT_EQ("", capture);

  capture.clear();
  LOAD_HISTORY(strv({"one", "two"}));
  shell._history.set_limit(2);
  CHECK_DELRANGE("2", strv({"1  one", "3  \\history del 2"}));
  EXPECT_TRUE(capture.empty());

  capture.clear();
  LOAD_HISTORY(strv({"one", "two"}));
  shell._history.set_limit(2);
  CHECK_DELRANGE("1", strv({"2  two", "3  \\history del 1"}));
  EXPECT_EQ("", capture);

  capture.clear();
  LOAD_HISTORY(strv({"one"}));
  shell._history.set_limit(1);
  CHECK_DELRANGE("1", strv({"2  \\history del 1"}));
  EXPECT_TRUE(capture.empty());

  capture.clear();
  shell._history.clear();
  shell._history.set_limit(0);
  CHECK_DELRANGE("1", strv({}));
  EXPECT_EQ("Invalid history entry 1\n\n", capture);
  capture.clear();
  CHECK_DELRANGE("0", strv({}));
  EXPECT_EQ("Invalid history entry 0\n\n", capture);

  // load and shrink
  capture.clear();
  LOAD_HISTORY(strv({"one", "two", "three"}));
  shell._history.set_limit(1);
  CHECK_DELRANGE("0", strv({"2  \\history del 0"}));
  EXPECT_EQ("Invalid history entry 0\n\n", capture);

#undef CHECK_DELRANGE
  shcore::delete_file("testhistory");
}

TEST_F(Shell_history, history_numbering) {
  mysqlsh::Command_line_shell shell(std::make_shared<Shell_options>());
  std::string capture;
  shell._delegate->print = print_capture;
  shell._delegate->print_error = print_capture;
  shell._delegate->user_data = &capture;
  using strv = std::vector<std::string>;

  shell._history.set_limit(10);

#define CHECK_NUMBERING_ADD(line, expected)                                    \
  {                                                                            \
    strv dump;                                                                 \
    SCOPED_TRACE(line);                                                        \
    shell.process_line(line);                                                  \
    EXPECT_TRUE(capture.empty());                                              \
    shell._history.dump([&dump](const std::string &s) { dump.push_back(s); }); \
    for (strv::size_type i = 0; i < dump.size(); ++i) {                        \
      EXPECT_TRUE(i < expected.size());                                        \
      EXPECT_EQ(expected[i], shcore::str_strip(dump[i]));                      \
    }                                                                          \
    EXPECT_EQ(expected.size(), dump.size());                                   \
  }

  // Test sequential numbering of history
  CHECK_NUMBERING_ADD("//1", (strv{"1  //1"}));
  CHECK_NUMBERING_ADD("//2", (strv{"1  //1", "2  //2"}));
  CHECK_NUMBERING_ADD("//3", (strv{"1  //1", "2  //2", "3  //3"}));

  // Must reset to 0 on clear
  shell._history.clear();
  CHECK_NUMBERING_ADD("//A", (strv{"1  //A"}));

  // Must reset on load
  shcore::create_file("testhistory", "//X\n//Y\n");
  shell._history.load("testhistory");
  CHECK_NUMBERING_ADD("//after", (strv{"1  //X", "2  //Y", "3  //after"}));

  // Adding a duplicate item ignores the duplicate
  shell._history.clear();
  CHECK_NUMBERING_ADD("//1", (strv{"1  //1"}));
  CHECK_NUMBERING_ADD("//2", (strv{"1  //1", "2  //2"}));
  CHECK_NUMBERING_ADD("//2", (strv{"1  //1", "2  //2"}));
  CHECK_NUMBERING_ADD("//3", (strv{"1  //1", "2  //2", "3  //3"}));
  CHECK_NUMBERING_ADD("//2", (strv{"1  //1", "2  //2", "3  //3", "4  //2"}));

  // Continued sequential numbering after filling up history
  shell._history.clear();
  shell._history.set_limit(3);
  CHECK_NUMBERING_ADD("//1", (strv{"1  //1"}));
  CHECK_NUMBERING_ADD("//2", (strv{"1  //1", "2  //2"}));
  CHECK_NUMBERING_ADD("//3", (strv{"1  //1", "2  //2", "3  //3"}));
  CHECK_NUMBERING_ADD("//4", (strv{"2  //2", "3  //3", "4  //4"}));
  CHECK_NUMBERING_ADD("//5", (strv{"3  //3", "4  //4", "5  //5"}));

  // Delete 1st id
  shell._history.clear();
  shell._history.set_limit(10);
  CHECK_NUMBERING_ADD("//1", (strv{"1  //1"}));
  CHECK_NUMBERING_ADD("//2", (strv{"1  //1", "2  //2"}));
  CHECK_NUMBERING_ADD("\\history del 1",
                      (strv{"2  //2", "3  \\history del 1"}));

  // Deleting an id that was already deleted is a no-op
  shell._history.clear();
  shell._history.set_limit(10);
  CHECK_NUMBERING_ADD("//1", (strv{"1  //1"}));
  CHECK_NUMBERING_ADD("//2", (strv{"1  //1", "2  //2"}));
  CHECK_NUMBERING_ADD("//3", (strv{"1  //1", "2  //2", "3  //3"}));
  CHECK_NUMBERING_ADD("//4", (strv{"1  //1", "2  //2", "3  //3", "4  //4"}));

  CHECK_NUMBERING_ADD("\\history del 3", (strv{"1  //1", "2  //2", "4  //4",
                                               "5  \\history del 3"}));

  CHECK_NUMBERING_ADD("//aa", (strv{"1  //1", "2  //2", "4  //4",
                                    "5  \\history del 3", "6  //aa"}));

  shell.process_line("\\history del 3");
  CHECK_NUMBERING_ADD("\\history del 3",
                      (strv{"1  //1", "2  //2", "4  //4", "5  \\history del 3",
                            "6  //aa", "7  \\history del 3"}));

  // Deleting the item at the limit
  shell._history.clear();
  shell._history.set_limit(4);
  CHECK_NUMBERING_ADD("//1", (strv{"1  //1"}));
  CHECK_NUMBERING_ADD("//2", (strv{"1  //1", "2  //2"}));
  CHECK_NUMBERING_ADD("//3", (strv{"1  //1", "2  //2", "3  //3"}));
  CHECK_NUMBERING_ADD("//4", (strv{"1  //1", "2  //2", "3  //3", "4  //4"}));
  CHECK_NUMBERING_ADD("\\history del 4", (strv{"1  //1", "2  //2", "3  //3",
                                               "5  \\history del 4"}));
  shell._history.clear();
  shell._history.set_limit(4);
  CHECK_NUMBERING_ADD("//1", (strv{"1  //1"}));
  CHECK_NUMBERING_ADD("//2", (strv{"1  //1", "2  //2"}));
  CHECK_NUMBERING_ADD("//3", (strv{"1  //1", "2  //2", "3  //3"}));
  CHECK_NUMBERING_ADD("//4", (strv{"1  //1", "2  //2", "3  //3", "4  //4"}));
  capture.clear();
  shell.process_line("\\history del 5");
  EXPECT_EQ("Invalid history entry 5\n\n", capture);

#undef CHECK_NUMBERING
  shcore::delete_file("testhistory");
}

}  // namespace mysqlsh
