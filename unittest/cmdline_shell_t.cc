/*
 * Copyright (c) 2017, Oracle and/or its affiliates. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2 of the
 * License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301  USA
 */

#include <gtest/gtest.h>
#include <gtest/gtest_prod.h>
#ifndef _WIN32
#include <sys/stat.h>
#endif
#include "ext/linenoise-ng/include/linenoise.h"
#include "mysqlshdk/libs/utils/utils_file.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_string.h"
#include "src/mysqlsh/cmdline_shell.h"
#include "unittest/test_utils.h"

namespace mysqlsh {

TEST(Cmdline_shell, query_variable_classic) {
  Command_line_shell shell(std::make_shared<Shell_options>());
  shell.finish_init();

  EXPECT_EQ("", shell.query_variable(
                    "version", mysqlsh::Prompt_manager::Mysql_system_variable));

  const char *pwd = getenv("MYSQL_PWD");
  auto coptions = shcore::get_connection_options("mysql://root@localhost");
  if (pwd)
    coptions.set_password(pwd);
  else
    coptions.set_password("");
  coptions.set_port(getenv("MYSQL_PORT") ? atoi(getenv("MYSQL_PORT")) : 3306);
  shell.connect(coptions, false);
  EXPECT_NE("", shell.query_variable(
                    "version", mysqlsh::Prompt_manager::Mysql_system_variable));
  EXPECT_NE("",
            shell.query_variable(
                "sql_mode", mysqlsh::Prompt_manager::Mysql_session_variable));
  EXPECT_NE("", shell.query_variable("Com_select",
                                     mysqlsh::Prompt_manager::Mysql_status));
  EXPECT_NE(
      "", shell.query_variable("Com_select",
                               mysqlsh::Prompt_manager::Mysql_session_status));

  EXPECT_EQ("", shell.query_variable(
                    "bogus", mysqlsh::Prompt_manager::Mysql_system_variable));
}

TEST(Cmdline_shell, query_variable_x) {
  Command_line_shell shell(std::make_shared<Shell_options>());
  shell.finish_init();

  const char *pwd = getenv("MYSQL_PWD");
  auto coptions = shcore::get_connection_options("mysqlx://root@localhost");
  if (pwd)
    coptions.set_password(pwd);
  else
    coptions.set_password("");
  coptions.set_port(getenv("MYSQLX_PORT") ? atoi(getenv("MYSQLX_PORT"))
                                          : 33060);
  shell.connect(coptions, false);
  EXPECT_NE("", shell.query_variable(
                    "version", mysqlsh::Prompt_manager::Mysql_system_variable));
  EXPECT_NE("",
            shell.query_variable(
                "sql_mode", mysqlsh::Prompt_manager::Mysql_session_variable));
  EXPECT_NE("", shell.query_variable("Com_select",
                                     mysqlsh::Prompt_manager::Mysql_status));
  EXPECT_NE(
      "", shell.query_variable("Com_select",
                               mysqlsh::Prompt_manager::Mysql_session_status));

  EXPECT_EQ("", shell.query_variable(
                    "bogus", mysqlsh::Prompt_manager::Mysql_system_variable));
}

TEST(Cmdline_shell, prompt) {
  char *args[] = {const_cast<char *>("ut"), const_cast<char *>("--js"),
                  const_cast<char *>("--interactive"), nullptr};
  mysqlsh::Command_line_shell shell(std::make_shared<Shell_options>(3, args));
  shell.finish_init();

  EXPECT_EQ("mysql-js> ", shell.prompt());

  EXPECT_NO_THROW(shell.load_prompt_theme("invalid"));

  std::ofstream of;
  of.open("test.theme");
  of << "{'segments':[{'text':'A'},{'text':'B'}]}\n";
  of.close();

  shell.load_prompt_theme("test.theme");
  EXPECT_EQ("A B> ", shell.prompt());

  // continuation
  shell.process_line("if (1) {");
  EXPECT_EQ("  -> ", shell.prompt());
  shell.process_line("}");
  shell.process_line("");

  EXPECT_EQ("A B> ", shell.prompt());

  // bad theme data
  of.open("test.theme");
  of << "{'segments':{'text':'A'}}\n";
  of.close();
  EXPECT_NO_THROW(shell.load_prompt_theme("test.theme"));

  shcore::delete_file("test.theme");
}

static void print_capture(void *cdata, const char *text) {
  std::string *capture = static_cast<std::string *>(cdata);
  capture->append(text).append("\n");
}


TEST(Cmdline_shell, help) {
  mysqlsh::Command_line_shell shell(std::make_shared<Shell_options>());

  std::string capture;
  shell._delegate->print = print_capture;
  shell._delegate->print_error = print_capture;
  shell._delegate->user_data = &capture;

  shell.print_cmd_line_helper();
  EXPECT_TRUE(shcore::str_beginswith(capture, "MySQL Shell "));
  EXPECT_TRUE(strstr(capture.c_str(), "Copyright (c)"));
  EXPECT_TRUE(strstr(capture.c_str(), "Oracle and/or its"));
  EXPECT_TRUE(strstr(capture.c_str(), "Usage examples:"));

  capture.clear();
  shell.print_banner();
  std::string year = shcore::fmttime("%Y");
  std::string expected =
      "MySQL Shell 8.0.4-rc\n\nCopyright (c) 2016, " + year +
      ", Oracle and/or its "
      "affiliates. All rights reserved.\n\nOracle is a registered trademark of "
      "Oracle Corporation and/or its\naffiliates. Other names may be "
      "trademarks of their respective\nowners.\n\n\n\nType '\\help' or '\\?' "
      "for help; '\\quit' to exit.\n\n\n\n";
  EXPECT_EQ(expected, capture);
}

}  // namespace mysqlsh
