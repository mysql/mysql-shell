/* Copyright (c) 2014, 2016, Oracle and/or its affiliates. All rights reserved.

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

using namespace std::placeholders;

namespace shcore {
namespace sql_shell_tests {
class Environment {
public:
  Environment() {
    shell_core.reset(new Shell_core(&output_handler.deleg));

    shell_sql.reset(new Shell_sql(shell_core.get()));
  }

  ~Environment() {}

  Shell_test_output_handler output_handler;
  std::shared_ptr<Shell_core> shell_core;
  std::shared_ptr<Shell_sql> shell_sql;
};

class Shell_sql_test : public ::testing::Test {
protected:
  Environment env;
  shcore::Value _returned_value;

  virtual void SetUp() {
    connect();
  }

  virtual void TearDown() {
    shcore::Argument_list args;
    env.shell_core->get_dev_session()->close(args);
  }

  void process_result(shcore::Value result) {
    _returned_value = result;
  }

  shcore::Value handle_input(std::string& query, Input_state& state) {
    env.shell_sql->handle_input(query, state, std::bind(&Shell_sql_test::process_result, this, _1));

    return _returned_value;
  }

  void connect() {
    const char *uri = getenv("MYSQL_URI");
    const char *pwd = getenv("MYSQL_PWD");
    const char *port = getenv("MYSQL_PORT");

    std::string mysql_uri = "mysql://";
    mysql_uri.append(uri);
    if (port) {
      mysql_uri.append(":");
      mysql_uri.append(port);
    }

    Argument_list args;
    args.push_back(Value(mysql_uri));
    if (pwd)
      args.push_back(Value(pwd));

    env.shell_core->connect_dev_session(args, mysqlsh::SessionType::Classic);
  }
};

TEST_F(Shell_sql_test, test_initial_state) {
  EXPECT_EQ("mysql-sql> ", env.shell_sql->prompt());
}

TEST_F(Shell_sql_test, full_statement) {
  Input_state state;
  std::string query = "show databases;";
  handle_input(query, state);

  // Query totally executed, input should be OK
  // Handled contains the executed statement
  // Prompt is for full statement
  EXPECT_EQ(Input_state::Ok, state);
  EXPECT_EQ("", query);
  EXPECT_EQ("show databases;", env.shell_sql->get_handled_input());
  EXPECT_EQ("mysql-sql> ", env.shell_sql->prompt());
}

TEST_F(Shell_sql_test, sql_multi_line_statement) {
  Input_state state;
  std::string query = "show";
  handle_input(query, state);

  // Nothing is executed until the delimiter is reached and the prompt changes
  // Prompt changes to multiline mode
  EXPECT_EQ(Input_state::ContinuedSingle, state);
  //EXPECT_EQ("show", query);
  EXPECT_EQ("", env.shell_sql->get_handled_input());
  EXPECT_EQ("        -> ", env.shell_sql->prompt());

  // Caching the partial statements is now internal
  // we just send whatever is remaining for the query to execute
  query = "databases";
  handle_input(query, state);

  // Nothing is executed until the delimiter is reached and the prompt changes
  // Prompt changes to multiline
  EXPECT_EQ(Input_state::ContinuedSingle, state);
  //EXPECT_EQ("databases", query);
  EXPECT_EQ("", env.shell_sql->get_handled_input());
  EXPECT_EQ("        -> ", env.shell_sql->prompt());

  query = ";";
  handle_input(query, state);

  // Nothing is executed until the delimiter is reached and the prompt changes
  // Prompt changes to multiline
  EXPECT_EQ(Input_state::Ok, state);
  EXPECT_EQ("", query);
  EXPECT_EQ("show\ndatabases\n;", env.shell_sql->get_handled_input());
  EXPECT_EQ("mysql-sql> ", env.shell_sql->prompt());
}

TEST_F(Shell_sql_test, sql_multi_line_string_delimiter) {
  Input_state state;
  std::string query = "delimiter %%%";
  handle_input(query, state);

  // Nothing is executed until the delimiter is reached and the prompt changes
  // Prompt changes to multiline mode
  EXPECT_EQ(Input_state::Ok, state);

  query = "show";
  handle_input(query, state);

  // Nothing is executed until the delimiter is reached and the prompt changes
  // Prompt changes to multiline mode
  EXPECT_EQ(Input_state::ContinuedSingle, state);
  //EXPECT_EQ("show", query);
  EXPECT_EQ("", env.shell_sql->get_handled_input());
  EXPECT_EQ("        -> ", env.shell_sql->prompt());

  // Caching the partial statements is now internal
  // we just send whatever is remaining for the query to execute
  query = "databases";
  handle_input(query, state);

  // Nothing is executed until the delimiter is reached and the prompt changes
  // Prompt changes to multiline
  EXPECT_EQ(Input_state::ContinuedSingle, state);
  //EXPECT_EQ("databases", query);
  EXPECT_EQ("", env.shell_sql->get_handled_input());
  EXPECT_EQ("        -> ", env.shell_sql->prompt());

  query = "%%%";
  handle_input(query, state);

  // Nothing is executed until the delimiter is reached and the prompt changes
  // Prompt changes to multiline
  EXPECT_EQ(Input_state::Ok, state);
  EXPECT_EQ("", query);
  EXPECT_EQ("show\ndatabases\n%%%", env.shell_sql->get_handled_input());
  EXPECT_EQ("mysql-sql> ", env.shell_sql->prompt());

  query = "delimiter ;";
  handle_input(query, state);

  // Nothing is executed until the delimiter is reached and the prompt changes
  // Prompt changes to multiline mode
  EXPECT_EQ(Input_state::Ok, state);
}

TEST_F(Shell_sql_test, multiple_statements_continued) {
  Input_state state;
  std::string query = "show databases; show";
  handle_input(query, state);

  // The first statement will be handled but the second will be hold
  // until the delimiter is found.
  // Prompt changes to multiline
  // query will be updated to only keep what has not been executed
  EXPECT_EQ(Input_state::ContinuedSingle, state);
  //EXPECT_EQ("show", query);
  EXPECT_EQ("show databases;", env.shell_sql->get_handled_input());
  EXPECT_EQ("        -> ", env.shell_sql->prompt());

  query = "databases;";
  handle_input(query, state);
  EXPECT_EQ(Input_state::Ok, state);
  EXPECT_EQ("", query);
  EXPECT_EQ("show\ndatabases;", env.shell_sql->get_handled_input());
  EXPECT_EQ("mysql-sql> ", env.shell_sql->prompt());
}

TEST_F(Shell_sql_test, global_multi_line_statement_ignored) {
  Input_state state;
  std::string query = "show";
  handle_input(query, state);

  // Nothing is executed until the delimiter is reached and the prompt changes
  // Prompt changes to multiline mode
  EXPECT_EQ(Input_state::ContinuedSingle, state);
  EXPECT_EQ("", env.shell_sql->get_handled_input());
  EXPECT_EQ("        -> ", env.shell_sql->prompt());

  query = "databases;\nshow";
  handle_input(query, state);

  // Being global multiline make sthe statement to be executed right away
  EXPECT_EQ(Input_state::ContinuedSingle, state);
  EXPECT_EQ("show\ndatabases;", env.shell_sql->get_handled_input());
  EXPECT_EQ("        -> ", env.shell_sql->prompt());

  query = "databases;";
  handle_input(query, state);

  // Being global multiline make sthe statement to be executed right away
  EXPECT_EQ(Input_state::Ok, state);
  EXPECT_EQ("show\ndatabases;", env.shell_sql->get_handled_input());
  EXPECT_EQ("mysql-sql> ", env.shell_sql->prompt());
}

TEST_F(Shell_sql_test, multiple_statements_and_continued) {
  Input_state state;
  std::string query = "show databases; select 1;show";
  handle_input(query, state);

  // The first statement will be handled but the second will be hold
  // until the delimiter is found.
  // Prompt changes to multiline
  // query will be updated to only keep what has not been executed
  EXPECT_EQ(Input_state::ContinuedSingle, state);
  //EXPECT_EQ("show", query);
  EXPECT_EQ("show databases;select 1;", env.shell_sql->get_handled_input());
  EXPECT_EQ("        -> ", env.shell_sql->prompt());

  query = "databases;";
  handle_input(query, state);
  EXPECT_EQ(Input_state::Ok, state);
  EXPECT_EQ("", query);
  EXPECT_EQ("show\ndatabases;", env.shell_sql->get_handled_input());
  EXPECT_EQ("mysql-sql> ", env.shell_sql->prompt());
}

TEST_F(Shell_sql_test, multiline_comment) {
  Input_state state;
  std::string query = "/*";
  handle_input(query, state);

  EXPECT_EQ(Input_state::ContinuedSingle, state);
  EXPECT_EQ("", query);
  EXPECT_EQ("", env.shell_sql->get_handled_input());
  EXPECT_EQ("       /*> ", env.shell_sql->prompt());

  query = "this was a multiline comment";
  handle_input(query, state);
  EXPECT_EQ(Input_state::ContinuedSingle, state);
  EXPECT_EQ("", query);
  EXPECT_EQ("", env.shell_sql->get_handled_input());
  EXPECT_EQ("       /*> ", env.shell_sql->prompt());

  query = "*/";
  handle_input(query, state);
  EXPECT_EQ(Input_state::Ok, state);
  EXPECT_EQ("", query);
  EXPECT_EQ("", env.shell_sql->get_handled_input());
  EXPECT_EQ("mysql-sql> ", env.shell_sql->prompt());
}

TEST_F(Shell_sql_test, multiline_single_quote_continued_string) {
  Input_state state;
  std::string query = "select 'hello ";
  handle_input(query, state);

  EXPECT_EQ(Input_state::ContinuedSingle, state);
  //EXPECT_EQ("select 'hello ", query);
  EXPECT_EQ("", env.shell_sql->get_handled_input());
  EXPECT_EQ("        '> ", env.shell_sql->prompt());

  query = "world';";
  handle_input(query, state);
  EXPECT_EQ(Input_state::Ok, state);
  EXPECT_EQ("", query);
  EXPECT_EQ("select 'hello \nworld';", env.shell_sql->get_handled_input());
  EXPECT_EQ("mysql-sql> ", env.shell_sql->prompt());
}

TEST_F(Shell_sql_test, multiline_double_quote_continued_string) {
  Input_state state;
  std::string query = "select \"hello ";
  handle_input(query, state);

  EXPECT_EQ(Input_state::ContinuedSingle, state);
  //EXPECT_EQ("select \"hello ", query);
  EXPECT_EQ("", env.shell_sql->get_handled_input());
  EXPECT_EQ("        \"> ", env.shell_sql->prompt());

  query = "world\";";
  handle_input(query, state);
  EXPECT_EQ(Input_state::Ok, state);
  EXPECT_EQ("", query);
  EXPECT_EQ("select \"hello \nworld\";", env.shell_sql->get_handled_input());
  EXPECT_EQ("mysql-sql> ", env.shell_sql->prompt());
}

TEST_F(Shell_sql_test, DISABLED_multiline_backtick_string) {
  Input_state state;
  std::string query = "select * from `t";
  handle_input(query, state);

  EXPECT_EQ(Input_state::ContinuedSingle, state);
  EXPECT_EQ("select * from `sakila`.`", query);
  EXPECT_EQ("", env.shell_sql->get_handled_input());
  EXPECT_EQ("        `> ", env.shell_sql->prompt());

  query = "film`;";
  handle_input(query, state);
  EXPECT_EQ(Input_state::Ok, state);
  EXPECT_EQ("", query);
  EXPECT_EQ("", env.shell_sql->get_handled_input());
  EXPECT_EQ("mysql-sql> ", env.shell_sql->prompt());
}

TEST_F(Shell_sql_test, multiple_single_double_quotes) {
  Input_state state;
  std::string query = "SELECT '''' as a;";
  handle_input(query, state);
  EXPECT_EQ(Input_state::Ok, state);
  EXPECT_EQ("", query);
  EXPECT_EQ("SELECT '''' as a;", env.shell_sql->get_handled_input());
  EXPECT_EQ("mysql-sql> ", env.shell_sql->prompt());

  query = "SELECT \"\"\"\" as a;";
  handle_input(query, state);
  EXPECT_EQ(Input_state::Ok, state);
  EXPECT_EQ("", query);
  EXPECT_EQ("SELECT \"\"\"\" as a;", env.shell_sql->get_handled_input());
  EXPECT_EQ("mysql-sql> ", env.shell_sql->prompt());

  query = "SELECT \"\'\" as a;";
  handle_input(query, state);
  EXPECT_EQ(Input_state::Ok, state);
  EXPECT_EQ("", query);
  EXPECT_EQ("SELECT \"\'\" as a;", env.shell_sql->get_handled_input());
  EXPECT_EQ("mysql-sql> ", env.shell_sql->prompt());

  query = "SELECT '\\'' as a;";
  handle_input(query, state);
  EXPECT_EQ(Input_state::Ok, state);
  EXPECT_EQ("", query);
  EXPECT_EQ("SELECT '\\'' as a;", env.shell_sql->get_handled_input());
  EXPECT_EQ("mysql-sql> ", env.shell_sql->prompt());

}

TEST_F(Shell_sql_test, continued_stmt_multiline_comment) {
  // ATM A multiline comment in the middle of a statement is
  // not trimmed off
  Input_state state;
  std::string query = "SELECT 1 AS _one /*";

  handle_input(query, state);
  EXPECT_EQ(Input_state::ContinuedSingle, state);
  EXPECT_EQ("", query);

  // SQL has not been handled yet
  EXPECT_EQ("", env.shell_sql->get_handled_input());

  // Prompt for continued statement
  EXPECT_EQ("        -> ", env.shell_sql->prompt());


  query = "comment text */;";
  handle_input(query, state);
  EXPECT_EQ(Input_state::Ok, state);
  EXPECT_EQ("", query);
  EXPECT_EQ("SELECT 1 AS _one /*\ncomment text */;",
      env.shell_sql->get_handled_input());

  EXPECT_EQ("mysql-sql> ", env.shell_sql->prompt());
}


TEST_F(Shell_sql_test, continued_stmt_dash_dash_comment) {
  // ATM dash dash comments are trimmed from statements
  Input_state state;
  std::string query = "select 1 as one -- sample comment";

  handle_input(query, state);
  EXPECT_EQ(Input_state::ContinuedSingle, state);
  EXPECT_EQ("", query);

  // SQL has not been handled yet
  EXPECT_EQ("", env.shell_sql->get_handled_input());

  // Prompt for continued statement
  EXPECT_EQ("        -> ", env.shell_sql->prompt());


  query = ";select 2 as two;";
  handle_input(query, state);
  EXPECT_EQ(Input_state::Ok, state);
  EXPECT_EQ("", query);
  EXPECT_EQ("select 1 as one \n;select 2 as two;",
      env.shell_sql->get_handled_input());

  EXPECT_EQ("mysql-sql> ", env.shell_sql->prompt());
}

TEST_F(Shell_sql_test, continued_stmt_dash_dash_comment_batch) {
  // ATM dash dash comments are trimmed from statements
  Input_state state;
  std::string query = "select 1 as one -- sample comment\n;select 2 as two;";
  handle_input(query, state);
  EXPECT_EQ(Input_state::Ok, state);
  EXPECT_EQ("", query);
  EXPECT_EQ("select 1 as one \n;select 2 as two;",
      env.shell_sql->get_handled_input());

  EXPECT_EQ("mysql-sql> ", env.shell_sql->prompt());
}

TEST_F(Shell_sql_test, continued_stmt_hash_comment) {
  // ATM hash comments are trimmed from statements
  Input_state state;
  std::string query = "select 1 as one #sample comment";

  handle_input(query, state);
  EXPECT_EQ(Input_state::ContinuedSingle, state);
  EXPECT_EQ("", query);

  // SQL has not been handled yet
  EXPECT_EQ("", env.shell_sql->get_handled_input());

  // Prompt for continued statement
  EXPECT_EQ("        -> ", env.shell_sql->prompt());


  query = ";select 2 as two;";
  handle_input(query, state);
  EXPECT_EQ(Input_state::Ok, state);
  EXPECT_EQ("", query);
  EXPECT_EQ("select 1 as one \n;select 2 as two;",
      env.shell_sql->get_handled_input());

  EXPECT_EQ("mysql-sql> ", env.shell_sql->prompt());
}

TEST_F(Shell_sql_test, continued_stmt_hash_comment_batch) {
  // ATM hash comments are trimmed from statements
  Input_state state;
  std::string query = "select 1 as one #sample comment\n;select 2 as two;";
  handle_input(query, state);
  EXPECT_EQ(Input_state::Ok, state);
  EXPECT_EQ("", query);
  EXPECT_EQ("select 1 as one \n;select 2 as two;",
      env.shell_sql->get_handled_input());

  EXPECT_EQ("mysql-sql> ", env.shell_sql->prompt());
}

}
}
