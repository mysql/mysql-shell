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
#include "../modules/base_session.h"
//#include "../modules/mod_session.h"
//#include "../modules/mod_schema.h"
#include "shellcore/common.h"
#include "test_utils.h"
#include <boost/algorithm/string.hpp>

namespace shcore {
  namespace sql_shell_tests {
    class Environment
    {
    public:
      Environment()
      {
        shell_core.reset(new Shell_core(&output_handler.deleg));

        shell_sql.reset(new Shell_sql(shell_core.get()));
      }

      ~Environment()
      {
      }

      Shell_test_output_handler output_handler;
      boost::shared_ptr<Shell_core> shell_core;
      boost::shared_ptr<Shell_sql> shell_sql;
    };

    class Shell_sql_test : public ::testing::Test
    {
    protected:
      Environment env;

      virtual void SetUp()
      {
        connect();
      }

      void connect()
      {
        const char *uri = getenv("MYSQL_URI");
        const char *pwd = getenv("MYSQL_PWD");
        const char *port = getenv("MYSQL_PORT");

        std::string x_uri(uri);
        if (port)
        {
          x_uri.append(":");
          x_uri.append(port);
        }

        Argument_list args;
        args.push_back(Value(x_uri));
        if (pwd)
          args.push_back(Value(pwd));

        boost::shared_ptr<mysh::BaseSession> session(mysh::connect_session(args));
        env.shell_core->set_global("session", Value(boost::static_pointer_cast<Object_bridge>(session)));
      }
    };

    TEST_F(Shell_sql_test, test_initial_state)
    {
      EXPECT_EQ("mysql-sql> ", env.shell_sql->prompt());
    }

    TEST_F(Shell_sql_test, full_statement)
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
      EXPECT_EQ("mysql-sql> ", env.shell_sql->prompt());
    }

    TEST_F(Shell_sql_test, sql_multi_line_statement)
    {
      Interactive_input_state state;
      std::string query = "show";
      env.shell_sql->handle_input(query, state);

      // Nothing is executed until the delimiter is reached and the prompt changes
      // Prompt changes to multiline mode
      EXPECT_EQ(Input_continued, state);
      //EXPECT_EQ("show", query);
      EXPECT_EQ("", env.shell_sql->get_handled_input());
      EXPECT_EQ("        -> ", env.shell_sql->prompt());

      // Caching the partial statements is now internal
      // we just send whatever is remaining for the query to execute
      query = "databases";
      env.shell_sql->handle_input(query, state);

      // Nothing is executed until the delimiter is reached and the prompt changes
      // Prompt changes to multiline
      EXPECT_EQ(Input_continued, state);
      //EXPECT_EQ("databases", query);
      EXPECT_EQ("", env.shell_sql->get_handled_input());
      EXPECT_EQ("        -> ", env.shell_sql->prompt());

      query = ";";
      env.shell_sql->handle_input(query, state);

      // Nothing is executed until the delimiter is reached and the prompt changes
      // Prompt changes to multiline
      EXPECT_EQ(Input_ok, state);
      EXPECT_EQ("", query);
      EXPECT_EQ("show\ndatabases", env.shell_sql->get_handled_input());
      EXPECT_EQ("mysql-sql> ", env.shell_sql->prompt());
    }

    TEST_F(Shell_sql_test, sql_multi_line_string_delimiter)
    {
      Interactive_input_state state;
      std::string query = "delimiter %%%";
      env.shell_sql->handle_input(query, state);

      // Nothing is executed until the delimiter is reached and the prompt changes
      // Prompt changes to multiline mode
      EXPECT_EQ(Input_ok, state);

      query = "show";
      env.shell_sql->handle_input(query, state);

      // Nothing is executed until the delimiter is reached and the prompt changes
      // Prompt changes to multiline mode
      EXPECT_EQ(Input_continued, state);
      //EXPECT_EQ("show", query);
      EXPECT_EQ("", env.shell_sql->get_handled_input());
      EXPECT_EQ("        -> ", env.shell_sql->prompt());

      // Caching the partial statements is now internal
      // we just send whatever is remaining for the query to execute
      query = "databases";
      env.shell_sql->handle_input(query, state);

      // Nothing is executed until the delimiter is reached and the prompt changes
      // Prompt changes to multiline
      EXPECT_EQ(Input_continued, state);
      //EXPECT_EQ("databases", query);
      EXPECT_EQ("", env.shell_sql->get_handled_input());
      EXPECT_EQ("        -> ", env.shell_sql->prompt());

      query = "%%%";
      env.shell_sql->handle_input(query, state);

      // Nothing is executed until the delimiter is reached and the prompt changes
      // Prompt changes to multiline
      EXPECT_EQ(Input_ok, state);
      EXPECT_EQ("", query);
      EXPECT_EQ("show\ndatabases", env.shell_sql->get_handled_input());
      EXPECT_EQ("mysql-sql> ", env.shell_sql->prompt());

      query = "delimiter ;";
      env.shell_sql->handle_input(query, state);

      // Nothing is executed until the delimiter is reached and the prompt changes
      // Prompt changes to multiline mode
      EXPECT_EQ(Input_ok, state);
    }

    TEST_F(Shell_sql_test, global_multi_line_statement)
    {
      Interactive_input_state state;
      std::string query = "show\ndatabases";
      env.shell_sql->handle_input(query, state);

      // Being global multiline make sthe statement to be executed right away
      EXPECT_EQ(Input_ok, state);
      EXPECT_EQ("show\ndatabases", env.shell_sql->get_handled_input());
      EXPECT_EQ("mysql-sql> ", env.shell_sql->prompt());
    }

    TEST_F(Shell_sql_test, multiple_statements_continued)
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
      EXPECT_EQ("        -> ", env.shell_sql->prompt());

      query = "databases;";
      env.shell_sql->handle_input(query, state);
      EXPECT_EQ(Input_ok, state);
      EXPECT_EQ("", query);
      EXPECT_EQ("show\ndatabases", env.shell_sql->get_handled_input());
      EXPECT_EQ("mysql-sql> ", env.shell_sql->prompt());
    }

    TEST_F(Shell_sql_test, global_multi_line_statement_ignored)
    {
      Interactive_input_state state;
      std::string query = "show";
      env.shell_sql->handle_input(query, state);

      // Nothing is executed until the delimiter is reached and the prompt changes
      // Prompt changes to multiline mode
      EXPECT_EQ(Input_continued, state);
      EXPECT_EQ("", env.shell_sql->get_handled_input());
      EXPECT_EQ("        -> ", env.shell_sql->prompt());

      query = "databases;\nshow";
      env.shell_sql->handle_input(query, state);

      // Being global multiline make sthe statement to be executed right away
      EXPECT_EQ(Input_continued, state);
      EXPECT_EQ("show\ndatabases", env.shell_sql->get_handled_input());
      EXPECT_EQ("        -> ", env.shell_sql->prompt());

      query = "databases;";
      env.shell_sql->handle_input(query, state);

      // Being global multiline make sthe statement to be executed right away
      EXPECT_EQ(Input_ok, state);
      EXPECT_EQ("show\ndatabases", env.shell_sql->get_handled_input());
      EXPECT_EQ("mysql-sql> ", env.shell_sql->prompt());
    }

    TEST_F(Shell_sql_test, multiline_comment)
    {
      Interactive_input_state state;
      std::string query = "/*";
      env.shell_sql->handle_input(query, state);

      EXPECT_EQ(Input_continued, state);
      EXPECT_EQ("", query);
      EXPECT_EQ("", env.shell_sql->get_handled_input());
      EXPECT_EQ("       /*> ", env.shell_sql->prompt());

      query = "this was a multiline comment";
      env.shell_sql->handle_input(query, state);
      EXPECT_EQ(Input_continued, state);
      EXPECT_EQ("", query);
      EXPECT_EQ("", env.shell_sql->get_handled_input());
      EXPECT_EQ("       /*> ", env.shell_sql->prompt());

      query = "*/";
      env.shell_sql->handle_input(query, state);
      EXPECT_EQ(Input_ok, state);
      EXPECT_EQ("", query);
      EXPECT_EQ("", env.shell_sql->get_handled_input());
      EXPECT_EQ("mysql-sql> ", env.shell_sql->prompt());
    }

    TEST_F(Shell_sql_test, multiline_single_quote_continued_string)
    {
      Interactive_input_state state;
      std::string query = "select 'hello ";
      env.shell_sql->handle_input(query, state);

      EXPECT_EQ(Input_continued, state);
      //EXPECT_EQ("select 'hello ", query);
      EXPECT_EQ("", env.shell_sql->get_handled_input());
      EXPECT_EQ("        '> ", env.shell_sql->prompt());

      query = "world';";
      env.shell_sql->handle_input(query, state);
      EXPECT_EQ(Input_ok, state);
      EXPECT_EQ("", query);
      EXPECT_EQ("select 'hello \nworld'", env.shell_sql->get_handled_input());
      EXPECT_EQ("mysql-sql> ", env.shell_sql->prompt());
    }

    TEST_F(Shell_sql_test, multiline_double_quote_continued_string)
    {
      Interactive_input_state state;
      std::string query = "select \"hello ";
      env.shell_sql->handle_input(query, state);

      EXPECT_EQ(Input_continued, state);
      //EXPECT_EQ("select \"hello ", query);
      EXPECT_EQ("", env.shell_sql->get_handled_input());
      EXPECT_EQ("        \"> ", env.shell_sql->prompt());

      query = "world\";";
      env.shell_sql->handle_input(query, state);
      EXPECT_EQ(Input_ok, state);
      EXPECT_EQ("", query);
      EXPECT_EQ("select \"hello \nworld\"", env.shell_sql->get_handled_input());
      EXPECT_EQ("mysql-sql> ", env.shell_sql->prompt());
    }

    TEST_F(Shell_sql_test, DISABLED_multiline_backtick_string)
    {
      Interactive_input_state state;
      std::string query = "select * from `t";
      env.shell_sql->handle_input(query, state);

      EXPECT_EQ(Input_continued, state);
      EXPECT_EQ("select * from `sakila`.`", query);
      EXPECT_EQ("", env.shell_sql->get_handled_input());
      EXPECT_EQ("        `> ", env.shell_sql->prompt());

      query = "film`;";
      env.shell_sql->handle_input(query, state);
      EXPECT_EQ(Input_ok, state);
      EXPECT_EQ("", query);
      EXPECT_EQ("", env.shell_sql->get_handled_input());
      EXPECT_EQ("mysql-sql> ", env.shell_sql->prompt());
    }

    TEST_F(Shell_sql_test, print_help)
    {
      std::vector<std::string> lines;

      // Generic topic prints the available commands
      EXPECT_TRUE(env.shell_sql->print_help(""));
      boost::algorithm::split(lines, env.output_handler.std_out, boost::is_any_of("\n"), boost::token_compress_on);
      EXPECT_EQ("===== SQL Mode Commands =====", lines[0]);
      EXPECT_EQ("warnings   (\\W) Show warnings after every statement.", lines[1]);
      EXPECT_EQ("nowarnings (\\w) Don't show warnings after every statement.", lines[2]);
      env.output_handler.wipe_out();

      // Specific command help print
      EXPECT_TRUE(env.shell_sql->print_help("warnings"));
      boost::algorithm::split(lines, env.output_handler.std_out, boost::is_any_of("\n"), boost::token_compress_on);
      EXPECT_EQ("Show warnings after every statement.", lines[0]);
      EXPECT_EQ("TRIGGERS: warnings or \\W", lines[1]);
    }
  }
}