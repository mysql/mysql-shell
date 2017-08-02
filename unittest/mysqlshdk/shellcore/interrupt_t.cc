/* Copyright (c) 2017, Oracle and/or its affiliates. All rights reserved.

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

#include <gtest_clean.h>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <string>
#include <thread>

#include "scripting/lang_base.h"
#include "scripting/types.h"
#include "scripting/types_cpp.h"
#include "shellcore/interrupt_handler.h"

#include "./test_utils.h"

#include "modules/devapi/mod_mysqlx_resultset.h"
#include "modules/devapi/mod_mysqlx_session.h"
#include "modules/mod_mysql_resultset.h"
#include "modules/mod_mysql_session.h"
#include "modules/mysql_connection.h"
#include "shellcore/base_session.h"
#include "shellcore/shell_core.h"
#include "shellcore/shell_jscript.h"
#include "shellcore/shell_python.h"
#include "shellcore/shell_sql.h"
#include "utils/utils_general.h"

namespace mysqlsh {

class Interrupt_tester : public shcore::Interrupt_helper {
 public:
  virtual void setup() {
    setup_ = true;
    unblocked_ = 1;
  }

  virtual void block() {
    // we're not supposed to get called while we're already blocked
    unblocked_++;
  }

  virtual void unblock(bool clear_pending) { unblocked_--; }

  int unblocked_ = 0;
  bool setup_ = false;
};

static Interrupt_tester interrupt_tester;

TEST(Interrupt, basics) {
  shcore::Interrupts::init(&interrupt_tester);
  EXPECT_EQ(0, interrupt_tester.unblocked_);

  shcore::Interrupts::setup();
  EXPECT_TRUE(interrupt_tester.setup_);
  EXPECT_EQ(1, interrupt_tester.unblocked_);

  shcore::Interrupts::unblock();
  EXPECT_EQ(0, interrupt_tester.unblocked_);
  shcore::Interrupts::block();
  EXPECT_EQ(1, interrupt_tester.unblocked_);
  shcore::Interrupts::unblock();
  EXPECT_EQ(0, interrupt_tester.unblocked_);

  bool flag = false;
  shcore::Interrupts::interrupt();  // nothing should happen

  EXPECT_EQ(0, interrupt_tester.unblocked_);

  shcore::Interrupts::push_handler([&flag]() {
    flag = true;
    return true;
  });
  EXPECT_EQ(0, interrupt_tester.unblocked_);
  shcore::Interrupts::pop_handler();
  EXPECT_EQ(0, interrupt_tester.unblocked_);
  ASSERT_FALSE(flag);
  shcore::Interrupts::interrupt();
  ASSERT_FALSE(flag);

  shcore::Interrupts::push_handler([&flag]() {
    flag = true;
    return true;
  });
  EXPECT_EQ(0, interrupt_tester.unblocked_);
  shcore::Interrupts::interrupt();
  EXPECT_EQ(0, interrupt_tester.unblocked_);
  ASSERT_TRUE(flag);
  flag = false;
  shcore::Interrupts::pop_handler();
  EXPECT_EQ(0, interrupt_tester.unblocked_);
  ASSERT_FALSE(flag);

  shcore::Interrupts::push_handler([&flag]() {
    flag = true;
    return true;
  });
  EXPECT_EQ(0, interrupt_tester.unblocked_);
  shcore::Interrupts::push_handler([&flag]() {
    flag = true;
    return true;
  });
  EXPECT_EQ(0, interrupt_tester.unblocked_);
  shcore::Interrupts::pop_handler();
  EXPECT_EQ(0, interrupt_tester.unblocked_);
  shcore::Interrupts::pop_handler();
  EXPECT_EQ(0, interrupt_tester.unblocked_);

  shcore::Interrupts::block();
  EXPECT_EQ(1, interrupt_tester.unblocked_);
  shcore::Interrupts::unblock();
  EXPECT_EQ(0, interrupt_tester.unblocked_);

  EXPECT_THROW(shcore::Interrupts::pop_handler(), std::logic_error);
  EXPECT_EQ(0, interrupt_tester.unblocked_);

  // check default propagation value
  EXPECT_FALSE(shcore::Interrupts::propagates_interrupt());
  shcore::Interrupts::set_propagate_interrupt(true);
  EXPECT_TRUE(shcore::Interrupts::propagates_interrupt());
}

//------------------------------------------------------------

struct Call_on_leave {
  std::function<void()> call;

  explicit Call_on_leave(std::function<void()> f) : call(f) {}

  ~Call_on_leave() { call(); }
};

class Interrupt_mysql : public Shell_core_test_wrapper {
 public:
  virtual void set_options() { _options->interactive = true; }

  void SetUp() {
    shcore::Interrupts::init(&interrupt_tester);
    Shell_core_test_wrapper::SetUp();
    execute("\\connect -c " + _mysql_uri);
    execute("\\py");
    execute("import time");
    wipe_all();
  }

  static void slow_deleg_print(void *user_data, const char *text) {
    shcore::sleep_ms(10);
    Shell_test_output_handler::deleg_print(user_data, text);
  }

  void make_output_slow() { output_handler.deleg.print = slow_deleg_print; }

  void unmake_output_slow() {
    output_handler.deleg.print = Shell_test_output_handler::deleg_print;
  }

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
         "   set nrows = 100;\n"
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
  }

  static void TearDownTestCase() { run_script_classic({"drop schema itst;"}); }

  std::shared_ptr<mysqlsh::ShellBaseSession> connect_classic(
      const std::string &uri, const std::string &password) {
    std::shared_ptr<mysqlsh::ShellBaseSession> session(
        new mysqlsh::mysql::ClassicSession());
    auto connection_options = shcore::get_connection_options(_mysql_uri);
    session->connect(connection_options);
    return session;
  }

  std::shared_ptr<mysqlsh::ShellBaseSession> connect_node(
      const std::string &uri, const std::string &password) {
    std::shared_ptr<mysqlsh::ShellBaseSession> session(
        new mysqlsh::mysqlx::Session());
    shcore::Argument_list args;
    auto connection_options = shcore::get_connection_options(_uri);
    session->connect(connection_options);
    return session;
  }

  void output_wait(const char *str, int timeout) {
    timeout *= 1000;
    while (timeout > 0) {
      if (output_handler.grep_stdout_thread_safe(str))
        return;

      shcore::sleep_ms(200);
      timeout -= 200;
    }
    FAIL() << "timeout waiting for " << str << "\n";
  }

  static const int k_processlist_info_column = 7;
  static const int k_processlist_state_column = 6;
  static const int k_processlist_command_column = 4;
  void session_wait(uint64_t sid, int timeout, const char *str,
                    int column = 7) {
    auto connection_options = shcore::get_connection_options(_mysql_uri);
    std::shared_ptr<mysqlsh::mysql::Connection> conn(
        new mysqlsh::mysql::Connection(connection_options));
    timeout *= 1000;
    while (timeout > 0) {
      std::unique_ptr<mysqlsh::mysql::Result> result(
          conn->run_sql("show full processlist"));
      for (;;) {
        auto row = result->fetch_one();
        if (!row)
          break;
        // for (int i = 0; i < 8; i++)
        //   printf("%s\t", row->get_value(i).descr().c_str());
        // printf("\n");
        if ((sid == 0 || row->get_value(0).as_uint() == sid) &&
            row->get_value_as_string(column).find(str) != std::string::npos)
          return;
      }
      shcore::sleep_ms(200);
      timeout -= 200;
    }
    FAIL() << "timeout waiting for " << str << "\n";
  }
};

class Interrupt_mysqlx : public Interrupt_mysql {
 public:
  void SetUp() override {
    shcore::Interrupts::init(&interrupt_tester);
    Shell_core_test_wrapper::SetUp();
    execute("\\py");
    execute("import time");
    execute("\\connect " + _uri);
    wipe_all();
  }
};

TEST_F(Interrupt_mysql, sql_classic) {
  // Test case for FR2
  std::shared_ptr<mysqlsh::ShellBaseSession> session;

  ASSERT_TRUE(_interactive_shell->shell_context()->get_dev_session().get());

  session = _interactive_shell->shell_context()->get_dev_session();
  ASSERT_TRUE(session.get());

  // Test that a query doing a sleep() on classic gets interrupted
  {
    bool kill_sent = false;
    std::thread thd([this, session, &kill_sent]() {
      // wait for the test session to popup up to 3s
      session_wait(session->get_connection_id(), 3, "test1");
      // then kill it
      kill_sent = true;
      shcore::Interrupts::interrupt();
    });
    try {
      auto result = std::static_pointer_cast<mysqlsh::mysql::ClassicResult>(
          session->raw_execute_sql("select sleep(5) as test1"));
      auto row = result->fetch_one();
      EXPECT_TRUE(kill_sent);
      EXPECT_EQ("1", row->get_value_as_string(0));
    } catch (std::exception &e) {
      FAIL() << e.what();
    }
    thd.join();
  }
}

TEST_F(Interrupt_mysql, sql_classic_javascript) {
  // Test case for FR5-a
  std::shared_ptr<mysqlsh::ShellBaseSession> session;

  ASSERT_TRUE(_interactive_shell->shell_context()->get_dev_session().get());

  session = _interactive_shell->shell_context()->get_dev_session();
  ASSERT_TRUE(session.get());

  execute("\\js");
  {
    std::thread thd([this, session]() {
      session_wait(session->get_connection_id(), 3, "sleep(42)");
      shcore::Interrupts::interrupt();
    });
    wipe_all();
    EXPECT_NO_THROW(
        execute("session.runSql('select * from mysql.user where "
                "sleep(42)'); print('FAILED');"));
    MY_EXPECT_STDOUT_NOT_CONTAINS("FAILED");
    thd.join();
    session_wait(session->get_connection_id(), 3, "Sleep",
                 k_processlist_command_column);
    // ensure next query runs ok
    wipe_all();
    execute("print(session.runSql('select * from itst.data').fetchAll());");
    MY_EXPECT_STDOUT_CONTAINS("last");
  }
}

TEST_F(Interrupt_mysql, sql_classic_py) {
  // Test case for FR7-a
  std::shared_ptr<mysqlsh::ShellBaseSession> session;

  ASSERT_TRUE(_interactive_shell->shell_context()->get_dev_session().get());

  session = _interactive_shell->shell_context()->get_dev_session();
  ASSERT_TRUE(session.get());

  execute("\\py");

  {
    std::thread thd([this, session]() {
      session_wait(session->get_connection_id(), 3, "sleep(42)");
      shcore::Interrupts::interrupt();
    });
    wipe_all();
    EXPECT_NO_THROW(
        execute("session.run_sql('select * from mysql.user where sleep(42)')\n"
                "print('FAILED')\n"));
    MY_EXPECT_STDOUT_NOT_CONTAINS("FAILED");
    MY_EXPECT_STDERR_CONTAINS("KeyboardInterrupt");
    thd.join();
    session_wait(session->get_connection_id(), 3, "Sleep",
                 k_processlist_command_column);
    // ensure next query runs ok
    wipe_all();
    execute("print(session.run_sql('select * from itst.data').fetch_all())\n");
    MY_EXPECT_STDOUT_CONTAINS("last");
  }
}

TEST_F(Interrupt_mysqlx, sql_x) {
  // Test case for FR2
  std::shared_ptr<mysqlsh::ShellBaseSession> session;

  session = _interactive_shell->shell_context()->get_dev_session();
  ASSERT_TRUE(session.get());
  ASSERT_EQ("Session", session->class_name());

  // Test that a query doing a sleep() on classic gets interrupted
  {
    bool kill_sent = false;
    std::thread thd([this, session, &kill_sent]() {
      // wait for the test session to popup up to 3s
      session_wait(session->get_connection_id(), 3, "test1");
      // then kill it
      kill_sent = true;
      shcore::Interrupts::interrupt();
    });
    try {
      auto result = std::static_pointer_cast<mysqlsh::mysqlx::SqlResult>(
          session->raw_execute_sql("select sleep(5) as test1"));
      shcore::Value row = result->fetch_one({});
      EXPECT_TRUE(kill_sent);
      EXPECT_EQ("1", row.as_object<mysqlsh::Row>()->get_member(0).repr());
    } catch (std::exception &e) {
      FAIL() << e.what();
    }
    thd.join();
  }
}

TEST_F(Interrupt_mysqlx, sql_x_err) {
  // Test case for FR2
  std::shared_ptr<mysqlsh::ShellBaseSession> session;

  ASSERT_TRUE(_interactive_shell->shell_context()->get_dev_session().get());

  session = _interactive_shell->shell_context()->get_dev_session();
  ASSERT_TRUE(session.get());

  {
    std::thread thd([this, session]() {
      session_wait(session->get_connection_id(), 3, "mysql");
      shcore::Interrupts::interrupt();
    });
    try {
      auto result = std::static_pointer_cast<mysqlsh::mysqlx::SqlResult>(
          session->raw_execute_sql("select * from mysql.user where sleep(1)"));
      FAIL() << "Did not get expected exception\n";
    } catch (std::exception &e) {
      EXPECT_STREQ("Query execution was interrupted", e.what());
    }
    thd.join();
  }
}

// Test that JS code running SQL gets interrupted
TEST_F(Interrupt_mysqlx, db_javascript_sql) {
  // Test case for FR5-b-1
  execute("\\js");
  std::shared_ptr<mysqlsh::ShellBaseSession> session;

  ASSERT_TRUE(_interactive_shell->shell_context()->get_dev_session().get());

  session = _interactive_shell->shell_context()->get_dev_session();
  ASSERT_TRUE(session.get());

  {
    std::thread thd([this, session]() {
      session_wait(session->get_connection_id(), 3, "sleep(42)");
      shcore::Interrupts::interrupt();
    });
    wipe_all();
    EXPECT_NO_THROW(
        execute("session.sql('select * from mysql.user where "
                "sleep(42)').execute(); print('FAILED');"));
    MY_EXPECT_STDOUT_NOT_CONTAINS("FAILED");
    thd.join();
    session_wait(session->get_connection_id(), 3, "Sleep",
                 k_processlist_command_column);
    // ensure next query runs ok
    wipe_all();
    execute(
        "print(session.sql('select * from itst.data').execute().fetchAll());");
    MY_EXPECT_STDOUT_CONTAINS("last");
  }
}

TEST_F(Interrupt_mysqlx, db_javascript_crud_table) {
  // Test case for FR5-b-2,3
  execute("\\js");
  execute("\\use itst");
  std::shared_ptr<mysqlsh::ShellBaseSession> session;

  ASSERT_TRUE(_interactive_shell->shell_context()->get_dev_session().get());

  session = _interactive_shell->shell_context()->get_dev_session();
  ASSERT_TRUE(session.get());

  // Test Table Find (sleep on projection)
  {
    std::thread thd([this, session]() {
      session_wait(session->get_connection_id(), 3, "sleep",
                   k_processlist_info_column);
      shcore::Interrupts::interrupt();
    });
    wipe_all();
    execute(
        "table = db.getTable('data'); print(table.select(['sleep(20)','b'])."
        "execute().fetchAll());");
    MY_EXPECT_STDOUT_NOT_CONTAINS("first");
    MY_EXPECT_STDERR_CONTAINS("interrupted");
    thd.join();
    session_wait(session->get_connection_id(), 3, "Sleep",
                 k_processlist_command_column);
    // ensure next query runs ok
    wipe_all();
    execute("print(db.getTable('data').select(['b']).execute().fetchAll());");
    MY_EXPECT_STDOUT_CONTAINS("first");
    MY_EXPECT_STDOUT_CONTAINS("last");
  }
  session_wait(session->get_connection_id(), 3, "Sleep",
               k_processlist_command_column);
}

TEST_F(Interrupt_mysqlx, db_javascript_crud_table2) {
  // Test case for FR5-b-2,3
  execute("\\js");
  execute("\\use itst");
  std::shared_ptr<mysqlsh::ShellBaseSession> session;

  ASSERT_TRUE(_interactive_shell->shell_context()->get_dev_session().get());

  session = _interactive_shell->shell_context()->get_dev_session();
  ASSERT_TRUE(session.get());

  wipe_all();
  {  // Again with sleep on filter
    std::thread thd([this, session]() {
      session_wait(session->get_connection_id(), 3, "sleep",
                   k_processlist_info_column);
      shcore::Interrupts::interrupt();
    });
    execute(
        "print(db.getTable('data').select(['b']).where('sleep(20)')"
        ".execute().fetchAll());");
    MY_EXPECT_STDOUT_NOT_CONTAINS("first");
    MY_EXPECT_STDERR_CONTAINS("interrupted");
    thd.join();
    session_wait(session->get_connection_id(), 3, "Sleep",
                 k_processlist_command_column);
    // ensure next query runs ok
    wipe_all();
    execute("print(db.getTable('data').select(['b']).execute().fetchAll());");
    MY_EXPECT_STDOUT_CONTAINS("first");
    MY_EXPECT_STDOUT_CONTAINS("last");
  }
}

TEST_F(Interrupt_mysqlx, db_javascript_crud_collection) {
  // Test case for FR5-b-2,3
  execute("\\js");
  execute("\\use itst");
  std::shared_ptr<mysqlsh::ShellBaseSession> session;

  ASSERT_TRUE(_interactive_shell->shell_context()->get_dev_session().get());

  session = _interactive_shell->shell_context()->get_dev_session();
  ASSERT_TRUE(session.get());

  // Test Collection Find
  {
    std::thread thd([this, session]() {
      session_wait(session->get_connection_id(), 3, "sleep",
                   k_processlist_info_column);
      shcore::Interrupts::interrupt();
    });
    execute(
        "res = db.getCollection('cdata').find().fields(['sleep(99)','b'])"
        ".execute()");
    execute("res.fetchAll()");
    MY_EXPECT_STDOUT_NOT_CONTAINS("first");
    MY_EXPECT_STDERR_CONTAINS("interrupted");
    thd.join();
    session_wait(session->get_connection_id(), 3, "Sleep",
                 k_processlist_command_column);
    // ensure next query runs ok
    wipe_all();
    execute(
        "print(db.getCollection('cdata').find().fields(['b'])."
        "execute().fetchAll());");
    MY_EXPECT_STDOUT_CONTAINS("first");
    MY_EXPECT_STDOUT_CONTAINS("last");
  }
}

TEST_F(Interrupt_mysqlx, db_javascript_crud_collection2) {
  // Test case for FR5-b-2,3
  execute("\\js");
  execute("\\use itst");
  std::shared_ptr<mysqlsh::ShellBaseSession> session;

  ASSERT_TRUE(_interactive_shell->shell_context()->get_dev_session().get());

  session = _interactive_shell->shell_context()->get_dev_session();
  ASSERT_TRUE(session.get());

  wipe_all();
  {
    std::thread thd([this, session]() {
      session_wait(session->get_connection_id(), 3, "sleep",
                   k_processlist_info_column);
      shcore::Interrupts::interrupt();
    });
    execute(
        "db.getCollection('cdata').find('sleep(10)').fields(['b'])."
        "execute();");
    MY_EXPECT_STDOUT_NOT_CONTAINS("first");
    MY_EXPECT_STDERR_CONTAINS("interrupted");
    thd.join();
    session_wait(session->get_connection_id(), 3, "Sleep",
                 k_processlist_command_column);
    // ensure next query runs ok
    wipe_all();
    execute(
        "print(db.getCollection('cdata').find().fields(['b'])."
        "execute().fetchAll());");
    MY_EXPECT_STDOUT_CONTAINS("first");
    MY_EXPECT_STDOUT_CONTAINS("last");
  }
}

TEST_F(Interrupt_mysqlx, db_javascript_crud_collection_changes) {
  // Test case for FR5-b-2,3
  execute("\\js");
  execute("\\use itst");
  std::shared_ptr<mysqlsh::ShellBaseSession> session;

  ASSERT_TRUE(_interactive_shell->shell_context()->get_dev_session().get());

  session = _interactive_shell->shell_context()->get_dev_session();
  ASSERT_TRUE(session.get());

  // FR5-b-3) JS Create, Read, Update and Delete from a database,
  // hit CTRL + C in the middle of the execution, then kill query is
  // sent to the server, check it the changes/transaction is rollback.

  wipe_all();
  {
    std::thread thd([this, session]() {
      session_wait(session->get_connection_id(), 3, "sleep",
                   k_processlist_info_column);
      shcore::Interrupts::interrupt();
    });
    execute(
        "db.getCollection('cdata').modify('sleep(10)').set('newfield', 42)."
        "execute();");
    MY_EXPECT_STDERR_CONTAINS("interrupted");
    thd.join();

    session_wait(session->get_connection_id(), 3, "Sleep",
                 k_processlist_command_column);
  }
  wipe_all();
  {
    std::thread thd([this, session]() {
      session_wait(session->get_connection_id(), 3, "sleep",
                   k_processlist_info_column);
      shcore::Interrupts::interrupt();
    });
    execute("db.getCollection('cdata').remove('sleep(10)').execute();");
    MY_EXPECT_STDERR_CONTAINS("interrupted");
    thd.join();

    session_wait(session->get_connection_id(), 3, "Sleep",
                 k_processlist_command_column);
  }
  wipe_all();

  // nothing changed
  execute(
      "println(db.getCollection('cdata').find('newfield=42').execute()."
      "fetchAll());");
  MY_EXPECT_STDOUT_CONTAINS("[]");

  // nothing deleted
  execute(
      "println(db.getCollection('cdata').find().execute().fetchAll().length, "
      "'rows')");
  MY_EXPECT_STDOUT_CONTAINS("102 rows");
}

// Test that Python code running SQL gets interrupted
TEST_F(Interrupt_mysqlx, db_python_sql) {
  // Test case for FR7-b-1
  execute("\\py");
  std::shared_ptr<mysqlsh::ShellBaseSession> session;

  ASSERT_TRUE(_interactive_shell->shell_context()->get_dev_session().get());

  session = _interactive_shell->shell_context()->get_dev_session();
  ASSERT_TRUE(session.get());

  {
    std::thread thd([this, session]() {
      session_wait(session->get_connection_id(), 3, "sleep",
                   k_processlist_info_column);
      shcore::Interrupts::interrupt();
    });
    wipe_all();
    EXPECT_NO_THROW(
        execute("session.sql('select * from itst.data where sleep(42)')."
                "execute()"));
    MY_EXPECT_STDOUT_NOT_CONTAINS("FAILED");
    MY_EXPECT_STDERR_CONTAINS("interrupted");
    thd.join();
    session_wait(session->get_connection_id(), 3, "Sleep",
                 k_processlist_command_column);
    // ensure next query runs ok
    wipe_all();
    execute("session.sql('select * from itst.data').execute()");
    MY_EXPECT_STDOUT_CONTAINS("last");
  }
}

TEST_F(Interrupt_mysqlx, db_python_crud_table) {
  // Test case for FR7-b-2,3
  execute("\\py");
  execute("\\use itst");
  std::shared_ptr<mysqlsh::ShellBaseSession> session;

  ASSERT_TRUE(_interactive_shell->shell_context()->get_dev_session().get());

  session = _interactive_shell->shell_context()->get_dev_session();
  ASSERT_TRUE(session.get());

  // Test Table Find (sleep on projection)
  {
    std::thread thd([this, session]() {
      session_wait(session->get_connection_id(), 3, "sleep",
                   k_processlist_info_column);
      shcore::Interrupts::interrupt();
    });
    wipe_all();
    execute("db.get_table('data').select(['sleep(20)','b']).execute()");
    MY_EXPECT_STDOUT_NOT_CONTAINS("first");
    // MY_EXPECT_STDERR_CONTAINS("Query execution was interrupted");
    // match either KeyboardInterrupt or Query execution was interrupted
    MY_EXPECT_STDERR_CONTAINS("interrupt");
    thd.join();
    session_wait(session->get_connection_id(), 3, "Sleep",
                 k_processlist_command_column);
    // ensure next query runs ok
    wipe_all();
    execute("db.get_table('data').select(['b']).execute()");
    MY_EXPECT_STDOUT_CONTAINS("first");
    MY_EXPECT_STDOUT_CONTAINS("last");
  }
}

TEST_F(Interrupt_mysqlx, db_python_crud_table2) {
  // Test case for FR7-b-2,3
  execute("\\py");
  execute("\\use itst");
  std::shared_ptr<mysqlsh::ShellBaseSession> session;

  ASSERT_TRUE(_interactive_shell->shell_context()->get_dev_session().get());

  session = _interactive_shell->shell_context()->get_dev_session();
  ASSERT_TRUE(session.get());

  {  // Again with sleep on filter
    std::thread thd([this, session]() {
      session_wait(session->get_connection_id(), 3, "sleep",
                   k_processlist_info_column);
      shcore::Interrupts::interrupt();
    });
    execute("db.get_table('data').select(['b']).where('sleep(20)').execute();");
    MY_EXPECT_STDOUT_NOT_CONTAINS("first");
    // MY_EXPECT_STDERR_CONTAINS("Query execution was interrupted");
    // match either KeyboardInterrupt or Query execution was interrupted
    MY_EXPECT_STDERR_CONTAINS("interrupt");
    thd.join();
    session_wait(session->get_connection_id(), 3, "Sleep",
                 k_processlist_command_column);
    // ensure next query runs ok
    wipe_all();
    execute("db.get_table('data').select(['b']).execute();");
    MY_EXPECT_STDOUT_CONTAINS("first");
    MY_EXPECT_STDOUT_CONTAINS("last");
  }
}

TEST_F(Interrupt_mysqlx, db_python_crud_collection) {
  // Test case for FR7-b-2,3
  execute("\\py");
  execute("\\use itst");
  std::shared_ptr<mysqlsh::ShellBaseSession> session;

  ASSERT_TRUE(_interactive_shell->shell_context()->get_dev_session().get());

  session = _interactive_shell->shell_context()->get_dev_session();
  ASSERT_TRUE(session.get());

  // Test Collection Find
  {
    std::thread thd([this, session]() {
      session_wait(session->get_connection_id(), 3, "sleep",
                   k_processlist_info_column);
      shcore::Interrupts::interrupt();
    });
    execute(
        "res = db.get_collection('cdata').find().fields(['sleep(99)','b'])"
        ".execute();");
    execute("res.fetch_all()");
    MY_EXPECT_STDOUT_NOT_CONTAINS("first");
    MY_EXPECT_STDERR_CONTAINS("interrupted");
    thd.join();
    session_wait(session->get_connection_id(), 3, "Sleep",
                 k_processlist_command_column);
    // ensure next query runs ok
    wipe_all();
    execute(
        "print(db.get_collection('cdata').find().fields(['b'])."
        "execute().fetch_all());");
    MY_EXPECT_STDOUT_CONTAINS("first");
    MY_EXPECT_STDOUT_CONTAINS("last");
  }
}

TEST_F(Interrupt_mysqlx, db_python_crud_collection2) {
  // Test case for FR7-b-2,3
  execute("\\py");
  execute("\\use itst");
  std::shared_ptr<mysqlsh::ShellBaseSession> session;

  ASSERT_TRUE(_interactive_shell->shell_context()->get_dev_session().get());

  session = _interactive_shell->shell_context()->get_dev_session();
  ASSERT_TRUE(session.get());

  wipe_all();
  {
    std::thread thd([this, session]() {
      session_wait(session->get_connection_id(), 3, "sleep",
                   k_processlist_info_column);
      shcore::Interrupts::interrupt();
    });
    execute(
        "db.get_collection('cdata').find('sleep(10)').fields(['b'])."
        "execute();");
    MY_EXPECT_STDOUT_NOT_CONTAINS("first");
    MY_EXPECT_STDERR_CONTAINS("interrupted");
    thd.join();
    session_wait(session->get_connection_id(), 3, "Sleep",
                 k_processlist_command_column);
    // ensure next query runs ok
    wipe_all();
    execute(
        "print(db.get_collection('cdata').find().fields(['b'])."
        "execute().fetch_all());");
    MY_EXPECT_STDOUT_CONTAINS("first");
    MY_EXPECT_STDOUT_CONTAINS("last");
  }

  // Ensure query is killed
  wipe_all();
  {
    std::thread thd([this, session]() {
      session_wait(session->get_connection_id(), 3, "sleep",
                   k_processlist_info_column);
      shcore::Interrupts::interrupt();
    });
    execute(
        "db.get_collection('cdata').modify('sleep(10)=0').set('x', 123)."
        "execute();");
    MY_EXPECT_STDOUT_NOT_CONTAINS("Query OK");
    MY_EXPECT_STDERR_CONTAINS("interrupted");
    thd.join();
    session_wait(session->get_connection_id(), 3, "Sleep",
                 k_processlist_command_column);
    wipe_all();
    execute("db.get_collection('cdata').find('x = 123').execute()");
    MY_EXPECT_STDOUT_CONTAINS("Empty set");
  }
}

TEST_F(Interrupt_mysqlx, db_javascript_drop) {
  // Test case for FR5 b
  execute("\\js");
  execute("\\use itst");
  std::shared_ptr<mysqlsh::ShellBaseSession> session;

  ASSERT_TRUE(_interactive_shell->shell_context()->get_dev_session().get());

  session = _interactive_shell->shell_context()->get_dev_session();
  ASSERT_TRUE(session.get());

  // open transactions on all tables from another session, to
  // acquire metadata locks and block the drop operations for long
  // enough that we can interrupt them
  std::shared_ptr<mysqlsh::ShellBaseSession> conn(
      connect_classic(_mysql_uri, _pwd));
  conn->raw_execute_sql("start transaction");
  conn->raw_execute_sql("insert into itst.data values (DEFAULT,1)");
  conn->raw_execute_sql(
      "insert into itst.cdata (doc) values ('{\"_id\":\"dummyyy\"}')");

  {
    std::thread thd([this, session]() {
      session_wait(session->get_connection_id(), 3, "Waiting",
                   k_processlist_state_column);
      shcore::Interrupts::interrupt();
    });
    execute("session.dropTable('itst', 'data')");
    MY_EXPECT_STDERR_CONTAINS("interrupted");
    thd.join();
    session_wait(session->get_connection_id(), 3, "Sleep",
                 k_processlist_command_column);
    // ensure next query runs ok
    wipe_all();
  }

  {
    std::thread thd([this, session]() {
      session_wait(session->get_connection_id(), 3, "Waiting",
                   k_processlist_state_column);
      shcore::Interrupts::interrupt();
    });
    execute("session.dropCollection('itst', 'cdata')");
    MY_EXPECT_STDERR_CONTAINS("interrupted");
    thd.join();
    session_wait(session->get_connection_id(), 3, "Sleep",
                 k_processlist_command_column);
    // ensure next query runs ok
    wipe_all();
  }

  {
    std::thread thd([this, session]() {
      session_wait(session->get_connection_id(), 3, "Waiting",
                   k_processlist_state_column);
      shcore::Interrupts::interrupt();
    });
    execute("session.dropSchema('itst')");
    MY_EXPECT_STDERR_CONTAINS("interrupted");
    thd.join();
    session_wait(session->get_connection_id(), 3, "Sleep",
                 k_processlist_command_column);
    // ensure next query runs ok
    wipe_all();
  }

  conn->raw_execute_sql("rollback");
  auto result = std::static_pointer_cast<mysqlsh::mysql::ClassicResult>(
      conn->raw_execute_sql(
          "select count(*) from information_schema.tables where "
          "table_schema='itst'"));
  auto row = result->fetch_one();
  EXPECT_EQ(2, row->get_value(0).as_int());
}

TEST_F(Interrupt_mysqlx, db_python_drop) {
  // Test case for FR7 b
  execute("\\py");
  execute("\\use itst");
  std::shared_ptr<mysqlsh::ShellBaseSession> session;

  ASSERT_TRUE(_interactive_shell->shell_context()->get_dev_session().get());

  session = _interactive_shell->shell_context()->get_dev_session();
  ASSERT_TRUE(session.get());

  // open transactions on all tables from another session, to
  // acquire metadata locks and block the drop operations for long
  // enough that we can interrupt them
  std::shared_ptr<mysqlsh::ShellBaseSession> conn(
      connect_classic(_mysql_uri, _pwd));
  conn->raw_execute_sql("start transaction");
  conn->raw_execute_sql("insert into itst.data values (DEFAULT,1)");
  conn->raw_execute_sql(
      "insert into itst.cdata (doc) values ('{\"_id\":\"dummyyy\"}')");

  {
    std::thread thd([this, session]() {
      session_wait(session->get_connection_id(), 3, "Waiting",
                   k_processlist_state_column);
      shcore::Interrupts::interrupt();
    });
    execute("session.drop_table('itst', 'data')");
    MY_EXPECT_STDERR_CONTAINS("interrupted");
    thd.join();
    session_wait(session->get_connection_id(), 3, "Sleep",
                 k_processlist_command_column);
    // ensure next query runs ok
    wipe_all();
  }

  {
    std::thread thd([this, session]() {
      session_wait(session->get_connection_id(), 3, "Waiting",
                   k_processlist_state_column);
      shcore::Interrupts::interrupt();
    });
    execute("session.drop_collection('itst', 'cdata')");
    MY_EXPECT_STDERR_CONTAINS("interrupted");
    thd.join();
    session_wait(session->get_connection_id(), 3, "Sleep",
                 k_processlist_command_column);
    // ensure next query runs ok
    execute("print 'flush'");  // this will flush any pending async callbacks
    wipe_all();
  }

  {
    std::thread thd([this, session]() {
      session_wait(session->get_connection_id(), 3, "Waiting",
                   k_processlist_state_column);
      shcore::Interrupts::interrupt();
    });
    execute("session.drop_schema('itst')");
    MY_EXPECT_STDERR_CONTAINS("interrupted");
    thd.join();
    session_wait(session->get_connection_id(), 3, "Sleep",
                 k_processlist_command_column);
    // ensure next query runs ok
    execute("print 'flush'");  // this will flush any pending async callbacks
    wipe_all();
  }

  conn->raw_execute_sql("rollback");
  auto result = std::static_pointer_cast<mysqlsh::mysql::ClassicResult>(
      conn->raw_execute_sql(
          "select count(*) from information_schema.tables where "
          "table_schema='itst'"));
  auto row = result->fetch_one();
  EXPECT_EQ(2, row->get_value(0).as_int());
}

// Test that native JavaScript code gets interrupted
TEST_F(Interrupt_mysql, javascript) {
  // Test case for FR4 JS
  execute("\\js");
  std::thread thd([this]() {
    output_wait("READY", 3);
    shcore::Interrupts::interrupt();
  });
  wipe_all();
  EXPECT_NO_THROW(
      execute("print('READY');\n"
              "for(i=0;i<1000000000;i++) {};\n"
              "print('FAILED');"));
  MY_EXPECT_STDOUT_NOT_CONTAINS("FAILED");
  thd.join();
}

// Test that native Python code gets interrupted
TEST_F(Interrupt_mysql, python) {
  // Test case for FR4 Py
  execute("\\py");
  std::thread thd([this]() {
    output_wait("READY", 3);
    shcore::Interrupts::interrupt();
  });
  wipe_all();
  EXPECT_NO_THROW(
      execute("print 'READY'\n"
              "import time\n"
              "for i in xrange(1000000000): time.sleep(0.1)\n"
              "print 'FAILED'\n\n"));
  MY_EXPECT_STDOUT_NOT_CONTAINS("FAILED");
  thd.join();
}

// The following tests check interruption of resultset printing

// execute a query which takes time to execute like a big select * , then hit
// ^C (Ctrl + C) and verify the query is killed (Check in server connection
// log to verify the KILL QUERY was sent) and error is displayed to the user.

TEST_F(Interrupt_mysql, sql_classic_resultset) {
  // Test case for FR3
  execute("\\sql");

  make_output_slow();
  Call_on_leave restore([this]() { unmake_output_slow(); });
  {
    std::thread thd([this]() {
      output_wait("first", 3);
      shcore::Interrupts::interrupt();
    });
    execute("select * from itst.data;");
    MY_EXPECT_STDOUT_NOT_CONTAINS("last");
    MY_EXPECT_STDOUT_CONTAINS(
        "Result printing interrupted, rows may be missing from the output.");
    thd.join();
  }
  wipe_all();
  {
    std::thread thd([this]() {
      output_wait("first", 3);
      shcore::Interrupts::interrupt();
    });
    execute("select * from itst.data\\G");
    MY_EXPECT_STDOUT_NOT_CONTAINS("last");
    MY_EXPECT_STDOUT_CONTAINS(
        "Result printing interrupted, rows may be missing from the output.");
    thd.join();
  }
}

TEST_F(Interrupt_mysqlx, sql_x_sql_resultset) {
  // Test case for FR3
  execute("\\sql");

  make_output_slow();
  Call_on_leave restore([this]() { unmake_output_slow(); });
  {
    std::thread thd([this]() {
      output_wait("first", 3);
      shcore::Interrupts::interrupt();
    });
    execute("select * from itst.data;");
    MY_EXPECT_STDOUT_NOT_CONTAINS("last");
    MY_EXPECT_STDOUT_CONTAINS(
        "Result printing interrupted, rows may be missing from the output.");
    thd.join();
  }
  wipe_all();
  {
    std::thread thd([this]() {
      output_wait("first", 3);
      shcore::Interrupts::interrupt();
    });
    execute("select * from itst.data\\G");
    MY_EXPECT_STDOUT_NOT_CONTAINS("last");
    MY_EXPECT_STDOUT_CONTAINS(
        "Result printing interrupted, rows may be missing from the output.");
    thd.join();
  }
}

#if 0  // not reliable until resultset printing rewrite
TEST_F(Interrupt_mysqlx, js_x_crud_resultset) {
  execute("\\js");
  execute("\\use itst");
  wipe_all();
  make_output_slow();
  {
    Call_on_leave restore([this]() { unmake_output_slow(); });
    std::thread thd([this]() {
      output_wait("first", 3);
      shcore::Interrupts::interrupt();
    });
    execute("db.getTable('data').select().execute();");
    MY_EXPECT_STDOUT_NOT_CONTAINS("last");
    MY_EXPECT_STDOUT_CONTAINS(
        "Result printing interrupted, rows may be missing from the output.");
    thd.join();
  }
  wipe_all();
  execute("db.getTable('data').select().execute();");
  MY_EXPECT_STDOUT_CONTAINS("last");
  MY_EXPECT_STDOUT_NOT_CONTAINS(
      "Result printing interrupted, rows may be missing from the output.");
}
#endif

}  // namespace mysqlsh
