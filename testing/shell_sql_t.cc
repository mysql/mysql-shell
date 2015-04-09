
/* Copyright (c) 2014, 2015, Oracle and/or its affiliates. All rights reserved.

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


#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <string>
#include <boost/shared_ptr.hpp>
#include <boost/pointer_cast.hpp>

#include "gtest/gtest.h"
#include "shellcore/types.h"
#include "shellcore/lang_base.h"
#include "shellcore/types_cpp.h"


#include "shellcore/shell_core.h"
#include "shellcore/shell_sql.h"
#include "../modules/mod_session.h"
#include "../modules/mod_db.h"


namespace shcore {


  namespace sql_shell_tests {
    class Environment
    {
    public:
      Environment()
      {
        deleg.print = print;
        deleg.password = password;
        shell_core.reset(new Shell_core(&deleg));

        session.reset(new mysh::Session(shell_core.get()));
        shell_core->set_global("_S", Value(boost::static_pointer_cast<Object_bridge>(session)));
        shell_sql.reset(new Shell_sql(shell_core.get()));
      }

      static void print(void *, const char *text)
      {
        std::cout << text << "\n";
      }

      static bool password(void *cdata, const char *prompt, std::string &ret)
      {
        return true;
      }

      ~Environment()
      {
      }


      Interpreter_delegate deleg;
      boost::shared_ptr<Shell_core> shell_core;
      boost::shared_ptr<Shell_sql> shell_sql;
      boost::shared_ptr<mysh::Session> session;
    };

    Environment env;

    TEST(Shell_sql_test, connection)
    {
      const char *uri = getenv("MYSQL_URI");
      const char *pwd = getenv("MYSQL_PWD");

      Argument_list args;
      args.push_back(Value(uri));
      if (pwd)
        args.push_back(Value(pwd));

      env.session->connect(args);
    }

    TEST(Shell_sql_test, test_initial_state)
    {
      EXPECT_EQ("mysql> ", env.shell_sql->prompt());
    }

    TEST(Shell_sql_test, full_statement)
    {
      Interactive_input_state state;
      std::string query = "show databases;";
      env.shell_sql->handle_input(query, state);

      // Query totally executed, input should be OK
      // Handled contains the executed statement
      // Prompt is for full statement
      EXPECT_EQ(Input_ok, state);
      EXPECT_EQ("", query);
      EXPECT_EQ("show databases", env.shell_sql->get_handled_input());
      EXPECT_EQ("mysql> ", env.shell_sql->prompt());
    }

    TEST(Shell_sql_test, multi_line_statement)
    {
      Interactive_input_state state;
      std::string query = "show";
      env.shell_sql->handle_input(query, state);

      // Nothing is executed until the delimiter is reached and the prompt changes
      // Prompt changes to multiline mode
      EXPECT_EQ(Input_continued, state);
      //EXPECT_EQ("show", query);
      EXPECT_EQ("", env.shell_sql->get_handled_input());
      EXPECT_EQ("    -> ", env.shell_sql->prompt());

      // Caching the partial statements is now internal
      // we just send whatever is remaining for the query to execute
      query = "databases";
      env.shell_sql->handle_input(query, state);

      // Nothing is executed until the delimiter is reached and the prompt changes
      // Prompt changes to multiline
      EXPECT_EQ(Input_continued, state);
      //EXPECT_EQ("databases", query);
      EXPECT_EQ("", env.shell_sql->get_handled_input());
      EXPECT_EQ("    -> ", env.shell_sql->prompt());

      
      query = ";";
      env.shell_sql->handle_input(query, state);

      // Nothing is executed until the delimiter is reached and the prompt changes
      // Prompt changes to multiline
      EXPECT_EQ(Input_ok, state);
      EXPECT_EQ("", query);
      EXPECT_EQ("show\ndatabases", env.shell_sql->get_handled_input());
      EXPECT_EQ("mysql> ", env.shell_sql->prompt());
    }


    TEST(Shell_sql_test, multiple_statements_continued)
    {
      Interactive_input_state state;
      std::string query = "show databases; show";
      env.shell_sql->handle_input(query, state);

      // The first statement will be handled but the second will be hold
      // until the delimiter is found.
      // Prompt changes to multiline
      // query will be updated to only keep what has not been executed
      EXPECT_EQ(Input_continued, state);
      //EXPECT_EQ("show", query);
      EXPECT_EQ("show databases", env.shell_sql->get_handled_input());
      EXPECT_EQ("    -> ", env.shell_sql->prompt());

      query = "databases;";
      env.shell_sql->handle_input(query, state);
      EXPECT_EQ(Input_ok, state);
      EXPECT_EQ("", query);
      EXPECT_EQ("show\ndatabases", env.shell_sql->get_handled_input());
      EXPECT_EQ("mysql> ", env.shell_sql->prompt());
    }

    TEST(Shell_sql_test, multiline_comment)
    {
      Interactive_input_state state;
      std::string query = "/*";
      env.shell_sql->handle_input(query, state);

      EXPECT_EQ(Input_continued, state);
      EXPECT_EQ("", query);
      EXPECT_EQ("", env.shell_sql->get_handled_input());
      EXPECT_EQ("   /*> ", env.shell_sql->prompt());

      query = "this was a multiline comment";
      env.shell_sql->handle_input(query, state);
      EXPECT_EQ(Input_continued, state);
      EXPECT_EQ("", query);
      EXPECT_EQ("", env.shell_sql->get_handled_input());
      EXPECT_EQ("   /*> ", env.shell_sql->prompt());

      query = "*/";
      env.shell_sql->handle_input(query, state);
      EXPECT_EQ(Input_ok, state);
      EXPECT_EQ("", query);
      EXPECT_EQ("", env.shell_sql->get_handled_input());
      EXPECT_EQ("mysql> ", env.shell_sql->prompt());
    }

    TEST(Shell_sql_test, multiline_single_quote_continued_string)
    {
      Interactive_input_state state;
      std::string query = "select 'hello ";
      env.shell_sql->handle_input(query, state);

      EXPECT_EQ(Input_continued, state);
      //EXPECT_EQ("select 'hello ", query);
      EXPECT_EQ("", env.shell_sql->get_handled_input());
      EXPECT_EQ("    '> ", env.shell_sql->prompt());

      query = "world';";
      env.shell_sql->handle_input(query, state);
      EXPECT_EQ(Input_ok, state);
      EXPECT_EQ("", query);
      EXPECT_EQ("select 'hello \nworld'", env.shell_sql->get_handled_input());
      EXPECT_EQ("mysql> ", env.shell_sql->prompt());
    }

    TEST(Shell_sql_test, multiline_double_quote_continued_string)
    {
      Interactive_input_state state;
      std::string query = "select \"hello ";
      env.shell_sql->handle_input(query, state);

      EXPECT_EQ(Input_continued, state);
      //EXPECT_EQ("select \"hello ", query);
      EXPECT_EQ("", env.shell_sql->get_handled_input());
      EXPECT_EQ("    \"> ", env.shell_sql->prompt());

      query = "world\";";
      env.shell_sql->handle_input(query, state);
      EXPECT_EQ(Input_ok, state);
      EXPECT_EQ("", query);
      EXPECT_EQ("select \"hello \nworld\"", env.shell_sql->get_handled_input());
      EXPECT_EQ("mysql> ", env.shell_sql->prompt());
    }

    TEST(Shell_sql_test, DISABLED_multiline_backtick_string)
    {
      Interactive_input_state state;
      std::string query = "select * from `t";
      env.shell_sql->handle_input(query, state);

      EXPECT_EQ(Input_continued, state);
      EXPECT_EQ("select * from `sakila`.`", query);
      EXPECT_EQ("", env.shell_sql->get_handled_input());
      EXPECT_EQ("    `> ", env.shell_sql->prompt());

      query = "film`;";
      env.shell_sql->handle_input(query, state);
      EXPECT_EQ(Input_ok, state);
      EXPECT_EQ("", query);
      EXPECT_EQ("", env.shell_sql->get_handled_input());
      EXPECT_EQ("mysql> ", env.shell_sql->prompt());
    }

    TEST(Shell_sql_test, print_help)
    {
      std::stringstream my_stdout;
      std::streambuf* stdout_backup = std::cout.rdbuf();
      std::string line;
      std::cout.rdbuf(my_stdout.rdbuf());

      // Generic topic prints the available commands
      EXPECT_TRUE(env.shell_sql->print_help(""));
      std::getline(my_stdout, line);
      EXPECT_EQ("===== SQL Mode Commands =====", line);
      std::getline(my_stdout, line);
      EXPECT_EQ("warnings   (\\W) Show warnings after every statement.", line);
      std::getline(my_stdout, line);
      EXPECT_EQ("nowarnings (\\w) Don't show warnings after every statement.", line);

      // Specific command help print
      EXPECT_TRUE(env.shell_sql->print_help("warnings"));
      std::getline(my_stdout, line);
      EXPECT_EQ("Show warnings after every statement.", line);
      std::getline(my_stdout, line);
      EXPECT_EQ("", line);
      std::getline(my_stdout, line);
      EXPECT_EQ("TRIGGERS: warnings or \\W", line);

      std::cout.rdbuf(stdout_backup);
    }
  }
}
