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

#include "modules/mysql_connection.h"
#include "mysqlshdk/libs/utils/utils_file.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "unittest/gtest_clean.h"
#include "unittest/test_utils.h"
#include "unittest/test_utils/command_line_test.h"

namespace mysqlsh {

// The following tests check interruption of the shell in batch mode

class Interrupt_mysqlsh : public tests::Command_line_test {
 protected:
  static void SetUpTestCase() {
    run_script_classic(
        {"drop schema if exists itst;", "create schema if not exists itst;",
         "create table itst.data "
         "   (a int primary key auto_increment, b varchar(10))",
         "create table itst.cdata "
         "   (_id varchar(32) "
         "       generated always as (doc->>'$._id') stored primary key,"
         "    doc json"
         "   );",
         "create procedure itst.populate()\n"
         "begin\n"
         "   declare nrows int;\n"
         "   set nrows = 50;\n"
         "   insert into itst.data values (default, 'first');"
         "   insert into itst.cdata (doc) values ('{\"_id\":\"0\", "
         "                   \"b\":\"first\"}');"
         "   while nrows > 0 do "
         "       insert into itst.data values (default, '');"
         "       insert into itst.cdata (doc) values (json_object("
         "             \"_id\", concat(nrows, ''), \"b\", 'test'));"
         "       set nrows = nrows - 1;"
         "   end while;\n"
         "   insert into itst.data values (default, 'last');\n"
         "   insert into itst.cdata (doc) values ('{\"_id\":\"l1\", "
         "                   \"b\":\"last\"}');"
         "end",
         "call itst.populate()"});

    {
      std::ofstream f("test_while.py");
      f << "import time\n"
        << "import sys\n"
        << "x=0\n"
        << "try:\n"
        << "  while 1:\n"
        << "    if x == 0:\n"
        << "      x = 1\n"
        << "      print 'ready'\n"
        << "except BaseException, e:\n"
        << "  print repr(e)\n"
        << "  sys.exit(2)\n\n"
        << "print 'failed'\n";
      f.close();
    }
    {
      std::ofstream f("test_sleep.py");
      f << "import time\n"
        << "import sys\n"
        << "x=0\n"
        << "try:\n"
        << "  if x == 0:\n"
        << "    x = 1\n"
        << "    print 'ready'\n"
        << "  time.sleep(10)\n"
        << "except BaseException, e:\n"
        << "  print repr(e)\n"
        << "  sys.exit(2)\n\n"
        << "print 'failed'\n";
      f.close();
    }
    {
      std::ofstream f("test_queryx.py");
      f << "import sys\n"
        << "try:\n"
        << "  print 'ready'\n"
        << "  session.sql('select sleep(10)').execute()\n"
        << "except BaseException, e:\n"
        << "  print repr(e)\n"
        << "  sys.exit(2)\n\n"
        << "print 'failed'\n";
      f.close();
    }
    {
      std::ofstream f("test_queryc.py");
      f << "import sys\n"
        << "try:\n"
        << "  print 'ready'\n"
        << "  session.run_sql('select sleep(10)')\n"
        << "except BaseException, e:\n"
        << "  print repr(e)\n"
        << "  sys.exit(2)\n\n"
        << "print 'failed'\n";
      f.close();
    }

    {
      std::ofstream f("test_while.js");
      f << "x=0\n"
        << "while(true) {\n"
        << "  if (x == 0) {\n"
        << "    x = 1;\n"
        << "    println('ready');\n"
        << "  }\n"
        << "}\n"
        << "println('failed');\n";
      f.close();
    }
    {
      std::ofstream f("test_sleep.js");
      f << "println('ready');\n"
        << "os.sleep(10);\n"
        << "println('failed');\n";
      f.close();
    }
    {
      std::ofstream f("test_queryx.js");
      f << "println('ready');\n"
        << "session.sql('select sleep(10)').execute();\n"
        << "println('failed');\n";
      f.close();
    }
    {
      std::ofstream f("test_queryc.js");
      f << "println('ready');\n"
        << "session.runSql('select sleep(10)');\n"
        << "println('failed');\n";
      f.close();
    }
    {
      std::ofstream f("test_query.sql");
      f << "select 'ready';\n"
        << "select * from mysql.user where sleep(10);\n"
        << "select 'fail';\n";
      f.close();
    }
  }

  static void TearDownTestCase() {
    run_script_classic({"drop schema itst;"});
    shcore::delete_file("test_while.py");
    shcore::delete_file("test_sleep.py");
    shcore::delete_file("test_queryx.py");
    shcore::delete_file("test_queryc.py");

    shcore::delete_file("test_while.js");
    shcore::delete_file("test_sleep.js");
    shcore::delete_file("test_queryx.js");
    shcore::delete_file("test_queryc.js");

    shcore::delete_file("test_query.sql");
  }

  void kill_on_ready() {
    kill_thread = std::thread([this]() {
      while (!_process) {
        shcore::sleep_ms(100);  // wait for process
      }

      while (!grep_stdout("ready")) {
        if (!_process)
          return;
        shcore::sleep_ms(200);
      }
      shcore::sleep_ms(200);

      send_ctrlc();
    });
  }

  void SetUp() override {
    tests::Command_line_test::SetUp();

    kill_thread = std::thread();

    session.reset(new mysqlsh::mysql::ClassicSession());

    auto connection_options = shcore::get_connection_options(_mysql_uri);
    session->connect(connection_options);
  }

  void TearDown() override {
    if (kill_thread.joinable())
      kill_thread.join();

    if (session)
      session->close();
    session.reset();

    tests::Command_line_test::TearDown();
  }

  std::shared_ptr<mysqlsh::ShellBaseSession> session;
  std::thread kill_thread;
};

#ifndef _WIN32
// Sending CTRL_C via code in windows is not working
TEST_F(Interrupt_mysqlsh, crud_js_x_cli) {
  // FR8-b-8 FR9-b-9
  kill_on_ready();

  int rc =
      execute({_mysqlsh, "--js", (_uri + "/itst").c_str(), "-e",
               "print('ready'); db.data.select().where('sleep(5)=0').execute()",
               nullptr});
  EXPECT_NE(0, rc);

  MY_EXPECT_CMD_OUTPUT_CONTAINS("nterrupted");
}

TEST_F(Interrupt_mysqlsh, crud_py_x_cli) {
  // FR8-b-9 FR9-b-9
  kill_on_ready();

  int rc =
      execute({_mysqlsh, "--py", (_uri + "/itst").c_str(), "-e",
               "print('ready'); db.data.select().where('sleep(5)=0').execute()",
               nullptr});
  EXPECT_NE(0, rc);

  MY_EXPECT_CMD_OUTPUT_CONTAINS("nterrupted");
}

TEST_F(Interrupt_mysqlsh, py_cli) {
  // FR8-b-9 FR9-b-9
  kill_on_ready();

  int rc = execute({_mysqlsh, "--py", "-e",
                    "import time; print 'ready'; time.sleep(10)", nullptr});
  EXPECT_EQ(130, rc);
}

TEST_F(Interrupt_mysqlsh, js_cli) {
  // FR8-b-8 FR9-b-8
  kill_on_ready();

  int rc = execute(
      {_mysqlsh, "--js", "-e", "print('ready'); os.sleep(10)", nullptr});
  EXPECT_EQ(130, rc);
}

#define TEST_INTERRUPT_SCRIPT_I(uri, lang, file)                              \
  {                                                                           \
    SetUp();                                                                  \
    SCOPED_TRACE(file);                                                       \
    kill_on_ready();                                                          \
    int rc =                                                                  \
        execute({_mysqlsh, uri, "--interactive", lang, "-f", file, nullptr}); \
    EXPECT_EQ(exitcode, rc);                                                  \
    MY_EXPECT_CMD_OUTPUT_CONTAINS(expect);                                    \
    if (unexpect)                                                             \
      MY_EXPECT_CMD_OUTPUT_NOT_CONTAINS(unexpect);                            \
    TearDown();                                                               \
  }

#define TEST_INTERRUPT_SCRIPT_B(uri, lang, file)                  \
  {                                                               \
    SetUp();                                                      \
    SCOPED_TRACE(file);                                           \
    kill_on_ready();                                              \
    int rc = execute({_mysqlsh, uri, lang, "-f", file, nullptr}); \
    EXPECT_EQ(exitcode, rc);                                      \
    MY_EXPECT_CMD_OUTPUT_CONTAINS(expect);                        \
    MY_EXPECT_CMD_OUTPUT_NOT_CONTAINS("fail");                    \
    TearDown();                                                   \
  }

#define TEST_INTERRUPT_SOURCE_I(uri, lang, file)                  \
  {                                                               \
    SetUp();                                                      \
    SCOPED_TRACE(file);                                           \
    kill_on_ready();                                              \
    int rc = execute({_mysqlsh, uri, "--interactive", lang, "-e", \
                      "\\source " file, nullptr});                \
    EXPECT_EQ(exitcode, rc);                                      \
    MY_EXPECT_CMD_OUTPUT_CONTAINS(expect);                        \
    if (unexpect)                                                 \
      MY_EXPECT_CMD_OUTPUT_NOT_CONTAINS(unexpect);                \
    TearDown();                                                   \
  }

TEST_F(Interrupt_mysqlsh, py_file_interactive) {
  // Test cases for FR8-a-3 FR8-b-3 FR9-a-3 FR9-b-3
  const char *expect = "KeyboardInterrupt";
  const char *unexpect = "fail";
  const int exitcode = 2;
  TEST_INTERRUPT_SCRIPT_I("--py", "--py", "test_while.py");
  TEST_INTERRUPT_SCRIPT_I("--py", "--py", "test_sleep.py");
  TEST_INTERRUPT_SCRIPT_I(_mysql_uri.c_str(), "--py", "test_while.py");
  TEST_INTERRUPT_SCRIPT_I(_mysql_uri.c_str(), "--py", "test_sleep.py");
  TEST_INTERRUPT_SCRIPT_I(_uri.c_str(), "--py", "test_while.py");
  TEST_INTERRUPT_SCRIPT_I(_uri.c_str(), "--py", "test_sleep.py");

  TEST_INTERRUPT_SCRIPT_I(_uri.c_str(), "--py", "test_queryx.py");
  TEST_INTERRUPT_SCRIPT_I(_mysql_uri.c_str(), "--py", "test_queryc.py");
}

TEST_F(Interrupt_mysqlsh, py_source_interactive) {
  // Test cases for FR8-a-6 FR8-b-6 FR9-a-6 FR9-b-6
  const char *expect = "KeyboardInterrupt";
  const char *unexpect = "fail";
  const int exitcode = 2;
  TEST_INTERRUPT_SOURCE_I("--py", "--py", "test_while.py");
  TEST_INTERRUPT_SOURCE_I("--py", "--py", "test_sleep.py");
  TEST_INTERRUPT_SOURCE_I(_mysql_uri.c_str(), "--py", "test_while.py");
  TEST_INTERRUPT_SOURCE_I(_mysql_uri.c_str(), "--py", "test_sleep.py");
  TEST_INTERRUPT_SOURCE_I(_uri.c_str(), "--py", "test_while.py");
  TEST_INTERRUPT_SOURCE_I(_uri.c_str(), "--py", "test_sleep.py");

  TEST_INTERRUPT_SOURCE_I(_uri.c_str(), "--py", "test_queryx.py");
  TEST_INTERRUPT_SOURCE_I(_mysql_uri.c_str(), "--py", "test_queryc.py");
}

TEST_F(Interrupt_mysqlsh, py_file_batch) {
  // Test cases for FR8-a-3 FR8-b-3 FR9-a-3 FR9-b-3
  const char *expect = "KeyboardInterrupt";
  const int exitcode = 2;
  TEST_INTERRUPT_SCRIPT_B("--py", "--py", "test_while.py");
  TEST_INTERRUPT_SCRIPT_B("--py", "--py", "test_sleep.py");
  TEST_INTERRUPT_SCRIPT_B(_mysql_uri.c_str(), "--py", "test_while.py");
  TEST_INTERRUPT_SCRIPT_B(_mysql_uri.c_str(), "--py", "test_sleep.py");
  TEST_INTERRUPT_SCRIPT_B(_uri.c_str(), "--py", "test_while.py");
  TEST_INTERRUPT_SCRIPT_B(_uri.c_str(), "--py", "test_sleep.py");

  TEST_INTERRUPT_SCRIPT_B(_uri.c_str(), "--py", "test_queryx.py");
  TEST_INTERRUPT_SCRIPT_B(_mysql_uri.c_str(), "--py", "test_queryc.py");
}

TEST_F(Interrupt_mysqlsh, js_file_interactive) {
  // Test cases for FR8-a-2 FR8-b-2 FR9-a-2 FR9-b-2
  const char *expect = "Script execution interrupted by user";
  const char *unexpect = nullptr;
  const int exitcode = 0;  // can't catch ^C in JS
  TEST_INTERRUPT_SCRIPT_I("--js", "--js", "test_while.js");
  TEST_INTERRUPT_SCRIPT_I("--js", "--js", "test_sleep.js");
  TEST_INTERRUPT_SCRIPT_I(_mysql_uri.c_str(), "--js", "test_while.js");
  TEST_INTERRUPT_SCRIPT_I(_mysql_uri.c_str(), "--js", "test_sleep.js");
  TEST_INTERRUPT_SCRIPT_I(_uri.c_str(), "--js", "test_while.js");
  TEST_INTERRUPT_SCRIPT_I(_uri.c_str(), "--js", "test_sleep.js");

  TEST_INTERRUPT_SCRIPT_I(_uri.c_str(), "--js", "test_queryx.js");
  TEST_INTERRUPT_SCRIPT_I(_mysql_uri.c_str(), "--js", "test_queryc.js");
}

TEST_F(Interrupt_mysqlsh, js_source_interactive) {
  // Test cases for FR8-a-5 FR8-b-5 FR9-a-5 FR9-b-5
  const char *expect = "Script execution interrupted by user";
  const char *unexpect = nullptr;
  const int exitcode = 0;  // can't catch ^C in JS
  TEST_INTERRUPT_SOURCE_I("--js", "--js", "test_while.js");
  TEST_INTERRUPT_SOURCE_I("--js", "--js", "test_sleep.js");
  TEST_INTERRUPT_SOURCE_I(_mysql_uri.c_str(), "--js", "test_while.js");
  TEST_INTERRUPT_SOURCE_I(_mysql_uri.c_str(), "--js", "test_sleep.js");
  TEST_INTERRUPT_SOURCE_I(_uri.c_str(), "--js", "test_while.js");
  TEST_INTERRUPT_SOURCE_I(_uri.c_str(), "--js", "test_sleep.js");

  TEST_INTERRUPT_SOURCE_I(_uri.c_str(), "--js", "test_queryx.js");
  TEST_INTERRUPT_SOURCE_I(_mysql_uri.c_str(), "--js", "test_queryc.js");
}

TEST_F(Interrupt_mysqlsh, js_file_batch) {
  // Test cases for FR8-a-2 FR8-b-2 FR9-a-2 FR9-b-2
  const char *expect = "Script execution interrupted by user";
  const int exitcode = 130;

  // ^C on os.sleep() will not bubble up the termination somehow

  TEST_INTERRUPT_SCRIPT_B("--js", "--js", "test_while.js");
  // TEST_INTERRUPT_SCRIPT_B("--js", "--js", "test_sleep.js");
  TEST_INTERRUPT_SCRIPT_B(_mysql_uri.c_str(), "--js", "test_while.js");
  // TEST_INTERRUPT_SCRIPT_B(_mysql_uri.c_str(), "--js", "test_sleep.js");
  TEST_INTERRUPT_SCRIPT_B(_uri.c_str(), "--js", "test_while.js");
  // TEST_INTERRUPT_SCRIPT_B(_uri.c_str(), "--js", "test_sleep.js");

  expect = "Script execution interrupted by user";
  TEST_INTERRUPT_SCRIPT_B(_uri.c_str(), "--js", "test_queryx.js");
  TEST_INTERRUPT_SCRIPT_B(_mysql_uri.c_str(), "--js", "test_queryc.js");
}

//------------------------------------------------------------------------------

#if 0
// Not feasible because signal is sent to the shell, not mp
// In a real terminal, the signal would be sent to mp
TEST_F(Interrupt_mysqlsh, dba_js) {
  // FR10-a-3
  kill_on_ready();

  int rc =
      execute({_mysqlsh, "--js", _mysql_uri.c_str(), "-e",
               "println('ready');"
               "dba.deploySandboxInstance(5555, {'password':''});", nullptr});
  EXPECT_EQ(1, rc);

  // Check if the test table is intact
  auto result = std::static_pointer_cast<mysqlsh::mysql::ClassicResult>(
      session->raw_execute_sql("select count(*) from itst.data"));
  auto row = result->fetch_one();
  EXPECT_EQ("52", row->get_value_as_string(0));
}
#endif

//------------------------------------------------------------------------------

TEST_F(Interrupt_mysqlsh, sql_cli) {
  // FR8-a-7 FR9-b-7
  kill_on_ready();

  int rc =
      execute({_mysqlsh, "--sql", (_mysql_uri + "/itst").c_str(), "-e",
               "select 'ready';delete from data where sleep(20)=0", nullptr});
  EXPECT_EQ(130, rc);

  // Check if the test table is intact
  auto result = std::static_pointer_cast<mysqlsh::mysql::ClassicResult>(
      session->raw_execute_sql("select count(*) from itst.data"));
  auto row = result->fetch_one();
  EXPECT_EQ("52", row->get_value_as_string(0));
}

TEST_F(Interrupt_mysqlsh, sqlx_cli) {
  // FR8-b-7 FR9-b-7
  kill_on_ready();

  int rc =
      execute({_mysqlsh, "--sql", (_uri + "/itst").c_str(), "-e",
               "select 'ready';delete from data where sleep(20)=0", nullptr});
  EXPECT_EQ(130, rc);

  // Check if the test table is intact
  auto result = std::static_pointer_cast<mysqlsh::mysql::ClassicResult>(
      session->raw_execute_sql("select count(*) from itst.data"));
  auto row = result->fetch_one();
  EXPECT_EQ("52", row->get_value_as_string(0));
}

TEST_F(Interrupt_mysqlsh, sql_file_batch) {
  kill_on_ready();

  // FR8-a-1 FR9-a-1
  int rc = execute(
      {_mysqlsh, _mysql_uri.c_str(), "--sql", "-f", "test_query.sql", nullptr});
  EXPECT_EQ(130, rc);

  MY_EXPECT_CMD_OUTPUT_CONTAINS("Query execution was interrupted");
  MY_EXPECT_CMD_OUTPUT_NOT_CONTAINS("fail");
}

TEST_F(Interrupt_mysqlsh, sql_file_source) {
  kill_on_ready();

  // FR8-a-4
  execute({_mysqlsh, _mysql_uri.c_str(), "--sql", "--interactive", "-e",
           "\\source test_query.sql", nullptr});

  MY_EXPECT_CMD_OUTPUT_CONTAINS("Query execution was interrupted");
  // Uncomment when Bug #26417116 is fixed
  // MY_EXPECT_CMD_OUTPUT_NOT_CONTAINS("fail");
}

TEST_F(Interrupt_mysqlsh, sql_file_interactive) {
  kill_on_ready();

  // FR8-a-1 FR9-a-1
  int rc = execute({_mysqlsh, _mysql_uri.c_str(), "--interactive", "--sql",
                    "-f", "test_query.sql", nullptr});
  EXPECT_EQ(0, rc);  // ^C during interactive doesn't abort shell

  MY_EXPECT_CMD_OUTPUT_CONTAINS("Query execution was interrupted");
  // ^C in interactive doesn't stop executing either
  MY_EXPECT_CMD_OUTPUT_CONTAINS("fail");
}

TEST_F(Interrupt_mysqlsh, sqlx_file_batch) {
  kill_on_ready();

  // FR8-b-1 FR9-b-1
  int rc = execute(
      {_mysqlsh, _uri.c_str(), "--sql", "-f", "test_query.sql", nullptr});
  EXPECT_EQ(130, rc);

  MY_EXPECT_CMD_OUTPUT_CONTAINS("Query execution was interrupted");
  MY_EXPECT_CMD_OUTPUT_NOT_CONTAINS("fail");
}

TEST_F(Interrupt_mysqlsh, sqlx_file_interactive) {
  kill_on_ready();

  // FR8-b-1 FR9-b-1
  int rc = execute({_mysqlsh, _uri.c_str(), "--interactive", "--sql", "-f",
                    "test_query.sql", nullptr});
  EXPECT_EQ(0, rc);  // ^C during interactive doesn't abort shell

  MY_EXPECT_CMD_OUTPUT_CONTAINS("Query execution was interrupted");
  // ^C in interactive doesn't stop executing either
  MY_EXPECT_CMD_OUTPUT_CONTAINS("fail");
}

TEST_F(Interrupt_mysqlsh, sqlx_file_source) {
  kill_on_ready();

  // FR8-b-4 FR9-b-4
  execute({_mysqlsh, _uri.c_str(), "--sql", "--interactive", "-e",
           "\\source test_query.sql", nullptr});

  MY_EXPECT_CMD_OUTPUT_CONTAINS("Query execution was interrupted");
  // Uncomment when Bug #26417116 is fixed
  // MY_EXPECT_CMD_OUTPUT_NOT_CONTAINS("fail");
}
#endif

}  // namespace mysqlsh
