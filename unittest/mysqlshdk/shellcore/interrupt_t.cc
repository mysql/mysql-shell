/* Copyright (c) 2017, 2020, Oracle and/or its affiliates. All rights reserved.

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License, version 2.0,
 as published by the Free Software Foundation.

 This program is also distributed with certain software (including
 but not limited to OpenSSL) that is licensed under separate terms, as
 designated in a particular file or component or in included license
 documentation.  The authors of MySQL hereby grant you an additional
 permission to link the program and your derivative works with the
 separately licensed software that they have included with MySQL.
 This program is distributed in the hope that it will be useful,  but
 WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 the GNU General Public License, version 2.0, for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software Foundation, Inc.,
 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA */

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

  virtual void unblock(bool /* clear_pending */) { unblocked_--; }

  int unblocked_ = 0;
  bool setup_ = false;
};

static Interrupt_tester interrupt_tester;

TEST(Interrupt, basics) {
  mysqlsh::Scoped_interrupt interrupt_handler(
      shcore::Interrupts::create(&interrupt_tester));
  EXPECT_EQ(0, interrupt_tester.unblocked_);

  shcore::current_interrupt()->setup();
  EXPECT_TRUE(interrupt_tester.setup_);
  EXPECT_EQ(1, interrupt_tester.unblocked_);

  shcore::current_interrupt()->unblock();
  EXPECT_EQ(0, interrupt_tester.unblocked_);
  shcore::current_interrupt()->block();
  EXPECT_EQ(1, interrupt_tester.unblocked_);
  shcore::current_interrupt()->unblock();
  EXPECT_EQ(0, interrupt_tester.unblocked_);

  bool flag = false;
  shcore::current_interrupt()->interrupt();  // nothing should happen

  EXPECT_EQ(0, interrupt_tester.unblocked_);

  shcore::current_interrupt()->push_handler([&flag]() {
    flag = true;
    return true;
  });
  EXPECT_EQ(0, interrupt_tester.unblocked_);
  shcore::current_interrupt()->pop_handler();
  EXPECT_EQ(0, interrupt_tester.unblocked_);
  ASSERT_FALSE(flag);
  shcore::current_interrupt()->interrupt();
  ASSERT_FALSE(flag);

  shcore::current_interrupt()->push_handler([&flag]() {
    flag = true;
    return true;
  });
  EXPECT_EQ(0, interrupt_tester.unblocked_);
  shcore::current_interrupt()->interrupt();
  EXPECT_EQ(0, interrupt_tester.unblocked_);
  ASSERT_TRUE(flag);
  flag = false;
  shcore::current_interrupt()->pop_handler();
  EXPECT_EQ(0, interrupt_tester.unblocked_);
  ASSERT_FALSE(flag);

  shcore::current_interrupt()->push_handler([&flag]() {
    flag = true;
    return true;
  });
  EXPECT_EQ(0, interrupt_tester.unblocked_);
  shcore::current_interrupt()->push_handler([&flag]() {
    flag = true;
    return true;
  });
  EXPECT_EQ(0, interrupt_tester.unblocked_);
  shcore::current_interrupt()->pop_handler();
  EXPECT_EQ(0, interrupt_tester.unblocked_);
  shcore::current_interrupt()->pop_handler();
  EXPECT_EQ(0, interrupt_tester.unblocked_);

  shcore::current_interrupt()->block();
  EXPECT_EQ(1, interrupt_tester.unblocked_);
  shcore::current_interrupt()->unblock();
  EXPECT_EQ(0, interrupt_tester.unblocked_);

  EXPECT_THROW(shcore::current_interrupt()->pop_handler(), std::logic_error);
  EXPECT_EQ(0, interrupt_tester.unblocked_);

  // check default propagation value
  EXPECT_FALSE(shcore::current_interrupt()->propagates_interrupt());
  shcore::current_interrupt()->set_propagate_interrupt(true);
  EXPECT_TRUE(shcore::current_interrupt()->propagates_interrupt());
}

//------------------------------------------------------------

struct Call_on_leave {
  std::function<void()> call;

  explicit Call_on_leave(std::function<void()> f) : call(f) {}

  ~Call_on_leave() { call(); }
};

class Interrupt_mysql : public Shell_core_test_wrapper {
 public:
  Interrupt_mysql() : m_handler(nullptr, slow_deleg_print, nullptr, nullptr) {
    m_current_interrupt = shcore::Interrupts::create(&interrupt_tester);
  }
  void set_options() override { _options->interactive = true; }

  void SetUp() override {
    mysqlsh::Scoped_interrupt interrupt_handler(m_current_interrupt);
    m_current_interrupt->setup();
    Shell_core_test_wrapper::SetUp();
    execute("\\connect --mc " + _mysql_uri);
    execute("\\py");
    execute("import time");
    wipe_all();
  }

  // This function is overriden in order to grab a pointer to the final
  // delegate so we can set the slow printer.
  void reset_shell() override {
    mysqlsh::Scoped_interrupt interrupt_handler(m_current_interrupt);
    m_current_interrupt->setup();
    // If the options have not been set, they must be set now, otherwise
    // they should be explicitly reset
    if (!_opts) reset_options();

    std::unique_ptr<shcore::Interpreter_delegate> delegate(
        new shcore::Interpreter_delegate(output_handler.deleg));

    replace_shell(_opts, std::move(delegate));

    _interactive_shell->finish_init();
    set_defaults();
    enable_testutil();
  }

  static bool slow_deleg_print(void *, const char *) {
    shcore::sleep_ms(10);
    return false;
  }

  void make_output_slow() { current_console()->add_print_handler(&m_handler); }

  void unmake_output_slow() {
    current_console()->remove_print_handler(&m_handler);
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
      const std::string & /* uri */, const std::string & /* password */) {
    std::shared_ptr<mysqlsh::ShellBaseSession> session(
        new mysqlsh::mysql::ClassicSession());
    auto connection_options = shcore::get_connection_options(_mysql_uri);
    session->connect(connection_options);
    return session;
  }

  std::shared_ptr<mysqlsh::ShellBaseSession> connect_node(
      const std::string & /* uri */, const std::string & /* password */) {
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
      if (output_handler.grep_stdout_thread_safe(str)) return;

      shcore::sleep_ms(200);
      timeout -= 200;
    }
    FAIL() << "timeout waiting for " << str << "\n";
  }

  static const int k_processlist_info_column = 7;
  static const int k_processlist_state_column = 6;
  static const int k_processlist_command_column = 4;
  static void session_wait(uint64_t sid, int timeout, const char *str,
                           int column = 7) {
    auto connection_options = shcore::get_connection_options(_mysql_uri);
    auto conn = mysqlshdk::db::mysql::Session::create();
    conn->connect(connection_options);
    timeout *= 1000;
    while (timeout > 0) {
      auto result = conn->query("show full processlist");
      for (;;) {
        auto row = result->fetch_one();
        if (!row) break;
        // for (int i = 0; i < 8; i++)
        //   printf("%s\t", row->get_value(i).descr().c_str());
        // printf("\n");
        if ((sid == 0 || row->get_uint(0) == sid) &&
            row->get_as_string(column).find(str) != std::string::npos)
          return;
      }
      shcore::sleep_ms(200);
      timeout -= 200;
    }
    conn->close();
    FAIL() << "timeout waiting for " << str << "\n";
  }

 protected:
  std::shared_ptr<shcore::Interrupts> m_current_interrupt;

 private:
  shcore::Interpreter_print_handler m_handler;
};

class Interrupt_mysqlx : public Interrupt_mysql {
 public:
  void SetUp() override {
    mysqlsh::Scoped_interrupt interrupt_handler(m_current_interrupt);
    m_current_interrupt->setup();
    Shell_core_test_wrapper::SetUp();
    execute("\\py");
    execute("import time");
    execute("\\connect " + _uri);
    wipe_all();
  }
};

TEST_F(Interrupt_mysql, sql_classic) {
  mysqlsh::Scoped_interrupt interrupt_handler(m_current_interrupt);

  // Test case for FR2
  std::shared_ptr<mysqlsh::ShellBaseSession> session;

  ASSERT_TRUE(_interactive_shell->shell_context()->get_dev_session().get());

  session = _interactive_shell->shell_context()->get_dev_session();
  ASSERT_TRUE(session.get());

  // Test that a query doing a sleep() on classic gets interrupted
  {
    bool kill_sent = false;
    // thread should not use `session`, get the connection ID before creating it
    const auto connection_id = session->get_connection_id();
    std::thread thd =
        mysqlsh::spawn_scoped_thread([connection_id, &kill_sent]() {
          Mysql_thread mysql;
          // wait for the test session to popup up to 3s
          session_wait(connection_id, 3, "test1");
          // then kill it
          kill_sent = true;
          shcore::current_interrupt()->interrupt();
        });
    try {
      auto result = session->execute_sql("select sleep(5) as test1");
      auto row = result->fetch_one();
      EXPECT_TRUE(kill_sent);
      EXPECT_EQ("1", row->get_as_string(0));
    } catch (const std::exception &e) {
      FAIL() << e.what();
    }
    thd.join();
  }
}
#ifdef HAVE_V8
TEST_F(Interrupt_mysql, sql_classic_javascript) {
  mysqlsh::Scoped_interrupt interrupt_handler(m_current_interrupt);
  // Test case for FR5-a
  std::shared_ptr<mysqlsh::ShellBaseSession> session;

  ASSERT_TRUE(_interactive_shell->shell_context()->get_dev_session().get());

  session = _interactive_shell->shell_context()->get_dev_session();
  ASSERT_TRUE(session.get());

  execute("\\js");
  {
    // thread should not use `session`, get the connection ID before creating it
    const auto connection_id = session->get_connection_id();
    std::thread thd = mysqlsh::spawn_scoped_thread([connection_id]() {
      Mysql_thread mysql;
      session_wait(connection_id, 3, "sleep(42)");
      shcore::current_interrupt()->interrupt();
    });
    wipe_all();
    EXPECT_NO_THROW(
        execute("session.runSql('select * from mysql.user where "
                "sleep(42)'); print('FAILED');"));
    MY_EXPECT_STDOUT_NOT_CONTAINS("FAILED");
    thd.join();
    session_wait(connection_id, 3, "Sleep", k_processlist_command_column);
    // ensure next query runs ok
    wipe_all();
    execute("print(session.runSql('select * from itst.data').fetchAll());");
    MY_EXPECT_STDOUT_CONTAINS("last");
  }
}
#endif

TEST_F(Interrupt_mysql, sql_classic_py) {
  mysqlsh::Scoped_interrupt interrupt_handler(m_current_interrupt);
  // Test case for FR7-a
  std::shared_ptr<mysqlsh::ShellBaseSession> session;

  ASSERT_TRUE(_interactive_shell->shell_context()->get_dev_session().get());

  session = _interactive_shell->shell_context()->get_dev_session();
  ASSERT_TRUE(session.get());

  execute("\\py");

  {
    // thread should not use `session`, get the connection ID before creating it
    const auto connection_id = session->get_connection_id();
    std::thread thd = mysqlsh::spawn_scoped_thread([connection_id]() {
      Mysql_thread mysql;
      session_wait(connection_id, 3, "sleep(42)");
      shcore::current_interrupt()->interrupt();
    });
    wipe_all();
    EXPECT_NO_THROW(
        execute("session.run_sql('select * from mysql.user where sleep(42)');"
                "print('FAILED')"));
    MY_EXPECT_STDOUT_NOT_CONTAINS("FAILED");
    MY_EXPECT_STDERR_CONTAINS("KeyboardInterrupt");
    thd.join();
    session_wait(connection_id, 3, "Sleep", k_processlist_command_column);
    // ensure next query runs ok
    wipe_all();
    execute("print(session.run_sql('select * from itst.data').fetch_all())\n");
    MY_EXPECT_STDOUT_CONTAINS("last");
  }
}

TEST_F(Interrupt_mysqlx, sql_x) {
  mysqlsh::Scoped_interrupt interrupt_handler(m_current_interrupt);
  // Test case for FR2
  std::shared_ptr<mysqlsh::ShellBaseSession> session;

  session = _interactive_shell->shell_context()->get_dev_session();
  ASSERT_TRUE(session.get());
  ASSERT_EQ("Session", session->class_name());

  // Test that a query doing a sleep() on classic gets interrupted
  {
    bool kill_sent = false;
    // thread should not use `session`, get the connection ID before creating it
    const auto connection_id = session->get_connection_id();
    std::thread thd =
        mysqlsh::spawn_scoped_thread([connection_id, &kill_sent]() {
          Mysql_thread mysql;
          // wait for the test session to popup up to 3s
          session_wait(connection_id, 3, "test1");
          // then kill it
          kill_sent = true;
          shcore::current_interrupt()->interrupt();
        });
    try {
      auto result = session->execute_sql("select sleep(5) as test1");
      auto row = result->fetch_one();
      EXPECT_TRUE(kill_sent);
      EXPECT_EQ("1", row->get_as_string(0));
    } catch (const std::exception &e) {
      FAIL() << e.what();
    }
    thd.join();
  }
}

TEST_F(Interrupt_mysqlx, sql_x_err) {
  mysqlsh::Scoped_interrupt interrupt_handler(m_current_interrupt);
  // Test case for FR2
  std::shared_ptr<mysqlsh::ShellBaseSession> session;

  ASSERT_TRUE(_interactive_shell->shell_context()->get_dev_session().get());

  session = _interactive_shell->shell_context()->get_dev_session();
  ASSERT_TRUE(session.get());

  {
    // thread should not use `session`, get the connection ID before creating it
    const auto connection_id = session->get_connection_id();
    std::thread thd = mysqlsh::spawn_scoped_thread([connection_id]() {
      Mysql_thread mysql;
      session_wait(connection_id, 3, "mysql");
      shcore::current_interrupt()->interrupt();
    });
    try {
      auto result =
          session->execute_sql("select * from mysql.user where sleep(1)");
      FAIL() << "Did not get expected exception\n";
    } catch (const std::exception &e) {
      EXPECT_STREQ("Query execution was interrupted", e.what());
    }
    thd.join();
  }
}

#ifdef HAVE_V8
// Test that JS code running SQL gets interrupted
TEST_F(Interrupt_mysqlx, db_javascript_sql) {
  mysqlsh::Scoped_interrupt interrupt_handler(m_current_interrupt);
  // Test case for FR5-b-1
  execute("\\js");
  std::shared_ptr<mysqlsh::ShellBaseSession> session;

  ASSERT_TRUE(_interactive_shell->shell_context()->get_dev_session().get());

  session = _interactive_shell->shell_context()->get_dev_session();
  ASSERT_TRUE(session.get());

  {
    // thread should not use `session`, get the connection ID before creating it
    const auto connection_id = session->get_connection_id();
    std::thread thd = mysqlsh::spawn_scoped_thread([connection_id]() {
      Mysql_thread mysql;
      session_wait(connection_id, 3, "sleep(42)");
      shcore::current_interrupt()->interrupt();
    });
    wipe_all();
    EXPECT_NO_THROW(
        execute("session.sql('select * from mysql.user where "
                "sleep(42)').execute(); print('FAILED');"));
    MY_EXPECT_STDOUT_NOT_CONTAINS("FAILED");
    thd.join();
    session_wait(connection_id, 3, "Sleep", k_processlist_command_column);
    // ensure next query runs ok
    wipe_all();
    execute(
        "print(session.sql('select * from itst.data').execute().fetchAll());");
    MY_EXPECT_STDOUT_CONTAINS("last");
  }
}

TEST_F(Interrupt_mysqlx, db_javascript_crud_table) {
  mysqlsh::Scoped_interrupt interrupt_handler(m_current_interrupt);
  // Test case for FR5-b-2,3
  execute("\\js");
  execute("\\use itst");
  std::shared_ptr<mysqlsh::ShellBaseSession> session;

  ASSERT_TRUE(_interactive_shell->shell_context()->get_dev_session().get());

  session = _interactive_shell->shell_context()->get_dev_session();
  ASSERT_TRUE(session.get());

  // Test Table Find (sleep on projection)
  {
    // thread should not use `session`, get the connection ID before creating it
    const auto connection_id = session->get_connection_id();
    std::thread thd = mysqlsh::spawn_scoped_thread([connection_id]() {
      Mysql_thread mysql;
      session_wait(connection_id, 3, "sleep", k_processlist_info_column);
      shcore::current_interrupt()->interrupt();
    });
    wipe_all();
    execute(
        "table = db.getTable('data'); print(table.select(['sleep(20)','b'])."
        "execute().fetchAll());");
    MY_EXPECT_STDOUT_NOT_CONTAINS("first");
    MY_EXPECT_STDERR_CONTAINS("interrupted");
    thd.join();
    session_wait(connection_id, 3, "Sleep", k_processlist_command_column);
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
  mysqlsh::Scoped_interrupt interrupt_handler(m_current_interrupt);
  // Test case for FR5-b-2,3
  execute("\\js");
  execute("\\use itst");
  std::shared_ptr<mysqlsh::ShellBaseSession> session;

  ASSERT_TRUE(_interactive_shell->shell_context()->get_dev_session().get());

  session = _interactive_shell->shell_context()->get_dev_session();
  ASSERT_TRUE(session.get());

  wipe_all();
  {  // Again with sleep on filter
    // thread should not use `session`, get the connection ID before creating it
    const auto connection_id = session->get_connection_id();
    std::thread thd = mysqlsh::spawn_scoped_thread([connection_id]() {
      Mysql_thread mysql;
      session_wait(connection_id, 3, "sleep", k_processlist_info_column);
      shcore::current_interrupt()->interrupt();
    });
    execute(
        "print(db.getTable('data').select(['b']).where('sleep(20)')"
        ".execute().fetchAll());");
    MY_EXPECT_STDOUT_NOT_CONTAINS("first");
    MY_EXPECT_STDERR_CONTAINS("interrupted");
    thd.join();
    session_wait(connection_id, 3, "Sleep", k_processlist_command_column);
    // ensure next query runs ok
    wipe_all();
    execute("print(db.getTable('data').select(['b']).execute().fetchAll());");
    MY_EXPECT_STDOUT_CONTAINS("first");
    MY_EXPECT_STDOUT_CONTAINS("last");
  }
}

TEST_F(Interrupt_mysqlx, db_javascript_crud_collection) {
  mysqlsh::Scoped_interrupt interrupt_handler(m_current_interrupt);
  // Test case for FR5-b-2,3
  execute("\\js");
  execute("\\use itst");
  std::shared_ptr<mysqlsh::ShellBaseSession> session;

  ASSERT_TRUE(_interactive_shell->shell_context()->get_dev_session().get());

  session = _interactive_shell->shell_context()->get_dev_session();
  ASSERT_TRUE(session.get());

  // Test Collection Find
  {
    // thread should not use `session`, get the connection ID before creating it
    const auto connection_id = session->get_connection_id();
    std::thread thd = mysqlsh::spawn_scoped_thread([connection_id]() {
      Mysql_thread mysql;
      session_wait(connection_id, 3, "sleep", k_processlist_info_column);
      shcore::current_interrupt()->interrupt();
    });
    execute(
        "res = db.getCollection('cdata').find().fields(['sleep(99)','b'])"
        ".execute()");
    execute("res.fetchAll()");
    MY_EXPECT_STDOUT_NOT_CONTAINS("first");
    MY_EXPECT_STDERR_CONTAINS("interrupted");
    thd.join();
    session_wait(connection_id, 3, "Sleep", k_processlist_command_column);
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
  mysqlsh::Scoped_interrupt interrupt_handler(m_current_interrupt);
  // Test case for FR5-b-2,3
  execute("\\js");
  execute("\\use itst");
  std::shared_ptr<mysqlsh::ShellBaseSession> session;

  ASSERT_TRUE(_interactive_shell->shell_context()->get_dev_session().get());

  session = _interactive_shell->shell_context()->get_dev_session();
  ASSERT_TRUE(session.get());

  wipe_all();
  {
    // thread should not use `session`, get the connection ID before creating it
    const auto connection_id = session->get_connection_id();
    std::thread thd = mysqlsh::spawn_scoped_thread([connection_id]() {
      Mysql_thread mysql;
      session_wait(connection_id, 3, "sleep", k_processlist_info_column);
      shcore::current_interrupt()->interrupt();
    });
    execute(
        "db.getCollection('cdata').find('sleep(10)').fields(['b'])."
        "execute();");
    MY_EXPECT_STDOUT_NOT_CONTAINS("first");
    MY_EXPECT_STDERR_CONTAINS("interrupted");
    thd.join();
    session_wait(connection_id, 3, "Sleep", k_processlist_command_column);
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
  mysqlsh::Scoped_interrupt interrupt_handler(m_current_interrupt);
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
    // thread should not use `session`, get the connection ID before creating it
    const auto connection_id = session->get_connection_id();
    std::thread thd = mysqlsh::spawn_scoped_thread([connection_id]() {
      Mysql_thread mysql;
      session_wait(connection_id, 3, "sleep", k_processlist_info_column);
      shcore::current_interrupt()->interrupt();
    });
    execute(
        "db.getCollection('cdata').modify('sleep(10)').set('newfield', 42)."
        "execute();");
    MY_EXPECT_STDERR_CONTAINS("interrupted");
    thd.join();

    session_wait(connection_id, 3, "Sleep", k_processlist_command_column);
  }
  wipe_all();
  {
    // thread should not use `session`, get the connection ID before creating it
    const auto connection_id = session->get_connection_id();
    std::thread thd = mysqlsh::spawn_scoped_thread([connection_id]() {
      Mysql_thread mysql;
      session_wait(connection_id, 3, "sleep", k_processlist_info_column);
      shcore::current_interrupt()->interrupt();
    });
    execute("db.getCollection('cdata').remove('sleep(10)').execute();");
    MY_EXPECT_STDERR_CONTAINS("interrupted");
    thd.join();

    session_wait(connection_id, 3, "Sleep", k_processlist_command_column);
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
#endif

// Test that Python code running SQL gets interrupted
TEST_F(Interrupt_mysqlx, db_python_sql) {
  mysqlsh::Scoped_interrupt interrupt_handler(m_current_interrupt);
  // Test case for FR7-b-1
  execute("\\py");
  std::shared_ptr<mysqlsh::ShellBaseSession> session;

  ASSERT_TRUE(_interactive_shell->shell_context()->get_dev_session().get());

  session = _interactive_shell->shell_context()->get_dev_session();
  ASSERT_TRUE(session.get());

  {
    // thread should not use `session`, get the connection ID before creating it
    const auto connection_id = session->get_connection_id();
    std::thread thd = mysqlsh::spawn_scoped_thread([connection_id]() {
      Mysql_thread mysql;
      session_wait(connection_id, 3, "sleep", k_processlist_info_column);
      shcore::current_interrupt()->interrupt();
    });
    wipe_all();
    EXPECT_NO_THROW(
        execute("session.sql('select * from itst.data where sleep(42)')."
                "execute()"));
    MY_EXPECT_STDOUT_NOT_CONTAINS("FAILED");
    MY_EXPECT_STDERR_CONTAINS("nterrupt");
    thd.join();
    session_wait(connection_id, 3, "Sleep", k_processlist_command_column);
    // ensure next query runs ok
    wipe_all();
    execute("session.sql('select * from itst.data').execute()");
    MY_EXPECT_STDOUT_CONTAINS("last");
  }
}

TEST_F(Interrupt_mysqlx, db_python_crud_table) {
  mysqlsh::Scoped_interrupt interrupt_handler(m_current_interrupt);
  // Test case for FR7-b-2,3
  execute("\\py");
  execute("\\use itst");
  std::shared_ptr<mysqlsh::ShellBaseSession> session;

  ASSERT_TRUE(_interactive_shell->shell_context()->get_dev_session().get());

  session = _interactive_shell->shell_context()->get_dev_session();
  ASSERT_TRUE(session.get());

  // Test Table Find (sleep on projection)
  {
    // thread should not use `session`, get the connection ID before creating it
    const auto connection_id = session->get_connection_id();
    std::thread thd = mysqlsh::spawn_scoped_thread([connection_id]() {
      Mysql_thread mysql;
      session_wait(connection_id, 3, "sleep", k_processlist_info_column);
      shcore::current_interrupt()->interrupt();
    });
    wipe_all();
    execute("db.get_table('data').select(['sleep(20)','b']).execute()");
    MY_EXPECT_STDOUT_NOT_CONTAINS("first");
    // MY_EXPECT_STDERR_CONTAINS("Query execution was interrupted");
    // match either KeyboardInterrupt or Query execution was interrupted
    MY_EXPECT_STDERR_CONTAINS("interrupt");
    thd.join();
    session_wait(connection_id, 3, "Sleep", k_processlist_command_column);
    // ensure next query runs ok
    wipe_all();
    execute("db.get_table('data').select(['b']).execute()");
    MY_EXPECT_STDOUT_CONTAINS("first");
    MY_EXPECT_STDOUT_CONTAINS("last");
  }
}

TEST_F(Interrupt_mysqlx, db_python_crud_table2) {
  mysqlsh::Scoped_interrupt interrupt_handler(m_current_interrupt);
  // Test case for FR7-b-2,3
  execute("\\py");
  execute("\\use itst");
  std::shared_ptr<mysqlsh::ShellBaseSession> session;

  ASSERT_TRUE(_interactive_shell->shell_context()->get_dev_session().get());

  session = _interactive_shell->shell_context()->get_dev_session();
  ASSERT_TRUE(session.get());

  {  // Again with sleep on filter
    // thread should not use `session`, get the connection ID before creating it
    const auto connection_id = session->get_connection_id();
    std::thread thd = mysqlsh::spawn_scoped_thread([connection_id]() {
      Mysql_thread mysql;
      session_wait(connection_id, 3, "sleep", k_processlist_info_column);
      shcore::current_interrupt()->interrupt();
    });
    execute("db.get_table('data').select(['b']).where('sleep(20)').execute();");
    MY_EXPECT_STDOUT_NOT_CONTAINS("first");
    // MY_EXPECT_STDERR_CONTAINS("Query execution was interrupted");
    // match either KeyboardInterrupt or Query execution was interrupted
    MY_EXPECT_STDERR_CONTAINS("interrupt");
    thd.join();
    session_wait(connection_id, 3, "Sleep", k_processlist_command_column);
    // ensure next query runs ok
    wipe_all();
    execute("db.get_table('data').select(['b']).execute();");
    MY_EXPECT_STDOUT_CONTAINS("first");
    MY_EXPECT_STDOUT_CONTAINS("last");
  }
}

TEST_F(Interrupt_mysqlx, db_python_crud_collection) {
  mysqlsh::Scoped_interrupt interrupt_handler(m_current_interrupt);
  // Test case for FR7-b-2,3
  execute("\\py");
  execute("\\use itst");
  std::shared_ptr<mysqlsh::ShellBaseSession> session;

  ASSERT_TRUE(_interactive_shell->shell_context()->get_dev_session().get());

  session = _interactive_shell->shell_context()->get_dev_session();
  ASSERT_TRUE(session.get());

  // Test Collection Find
  {
    // thread should not use `session`, get the connection ID before creating it
    const auto connection_id = session->get_connection_id();
    std::thread thd = mysqlsh::spawn_scoped_thread([connection_id]() {
      Mysql_thread mysql;
      session_wait(connection_id, 3, "sleep", k_processlist_info_column);
      shcore::current_interrupt()->interrupt();
    });
    execute(
        "res = db.get_collection('cdata').find().fields(['sleep(99)','b'])"
        ".execute();");
    execute("res.fetch_all()");
    MY_EXPECT_STDOUT_NOT_CONTAINS("first");
    MY_EXPECT_STDERR_CONTAINS("interrupted");
    thd.join();
    session_wait(connection_id, 3, "Sleep", k_processlist_command_column);
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
  mysqlsh::Scoped_interrupt interrupt_handler(m_current_interrupt);
  // Test case for FR7-b-2,3
  execute("\\py");
  execute("\\use itst");
  std::shared_ptr<mysqlsh::ShellBaseSession> session;

  ASSERT_TRUE(_interactive_shell->shell_context()->get_dev_session().get());

  session = _interactive_shell->shell_context()->get_dev_session();
  ASSERT_TRUE(session.get());

  wipe_all();
  {
    // thread should not use `session`, get the connection ID before creating it
    const auto connection_id = session->get_connection_id();
    std::thread thd = mysqlsh::spawn_scoped_thread([connection_id]() {
      Mysql_thread mysql;
      session_wait(connection_id, 3, "sleep", k_processlist_info_column);
      shcore::current_interrupt()->interrupt();
    });
    execute(
        "db.get_collection('cdata').find('sleep(10)').fields(['b'])."
        "execute();");
    MY_EXPECT_STDOUT_NOT_CONTAINS("first");
    MY_EXPECT_STDERR_CONTAINS("interrupted");
    thd.join();
    session_wait(connection_id, 3, "Sleep", k_processlist_command_column);
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
    // thread should not use `session`, get the connection ID before creating it
    const auto connection_id = session->get_connection_id();
    std::thread thd = mysqlsh::spawn_scoped_thread([connection_id]() {
      Mysql_thread mysql;
      session_wait(connection_id, 3, "sleep", k_processlist_info_column);
      shcore::current_interrupt()->interrupt();
    });
    execute(
        "db.get_collection('cdata').modify('sleep(10)=0').set('x', 123)."
        "execute();");
    MY_EXPECT_STDOUT_NOT_CONTAINS("Query OK");
    MY_EXPECT_STDERR_CONTAINS("interrupted");
    thd.join();
    session_wait(connection_id, 3, "Sleep", k_processlist_command_column);
    wipe_all();
    execute("db.get_collection('cdata').find('x = 123').execute()");
    MY_EXPECT_STDOUT_CONTAINS("Empty set");
  }
}

#ifdef HAVE_V8
TEST_F(Interrupt_mysqlx, db_javascript_drop) {
  mysqlsh::Scoped_interrupt interrupt_handler(m_current_interrupt);
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
  conn->execute_sql("start transaction");
  conn->execute_sql("insert into itst.data values (DEFAULT,1)");
  conn->execute_sql(
      "insert into itst.cdata (doc) values ('{\"_id\":\"dummyyy\"}')");

  {
    // thread should not use `session`, get the connection ID before creating it
    const auto connection_id = session->get_connection_id();
    std::thread thd = mysqlsh::spawn_scoped_thread([connection_id]() {
      Mysql_thread mysql;
      session_wait(connection_id, 3, "Waiting", k_processlist_state_column);
      shcore::current_interrupt()->interrupt();
    });
    execute("session.getSchema('itst').dropCollection('cdata')");
    MY_EXPECT_STDERR_CONTAINS("interrupted");
    thd.join();
    session_wait(connection_id, 3, "Sleep", k_processlist_command_column);
    // ensure next query runs ok
    wipe_all();
  }

  {
    // thread should not use `session`, get the connection ID before creating it
    const auto connection_id = session->get_connection_id();
    std::thread thd = mysqlsh::spawn_scoped_thread([connection_id]() {
      Mysql_thread mysql;
      session_wait(connection_id, 3, "Waiting", k_processlist_state_column);
      shcore::current_interrupt()->interrupt();
    });
    execute("session.dropSchema('itst')");
    MY_EXPECT_STDERR_CONTAINS("interrupted");
    thd.join();
    session_wait(connection_id, 3, "Sleep", k_processlist_command_column);
    // ensure next query runs ok
    wipe_all();
  }

  conn->execute_sql("rollback");
  auto result = conn->execute_sql(
      "select count(*) from information_schema.tables where "
      "table_schema='itst'");
  auto row = result->fetch_one();
  EXPECT_EQ(2, row->get_int(0));
}
#endif

TEST_F(Interrupt_mysqlx, db_python_drop) {
  mysqlsh::Scoped_interrupt interrupt_handler(m_current_interrupt);
  // Test case for FR7 b
  execute("\\py");
  execute("\\use itst");
  std::shared_ptr<mysqlsh::ShellBaseSession> session;
  wipe_all();

  ASSERT_TRUE(_interactive_shell->shell_context()->get_dev_session().get());

  session = _interactive_shell->shell_context()->get_dev_session();
  ASSERT_TRUE(session.get());

  // open transactions on all tables from another session, to
  // acquire metadata locks and block the drop operations for long
  // enough that we can interrupt them
  std::shared_ptr<mysqlsh::ShellBaseSession> conn(
      connect_classic(_mysql_uri, _pwd));
  conn->execute_sql("start transaction");
  conn->execute_sql("insert into itst.data values (DEFAULT,1)");
  conn->execute_sql(
      "insert into itst.cdata (doc) values ('{\"_id\":\"dummyyy\"}')");

  {
    // thread should not use `session`, get the connection ID before creating it
    const auto connection_id = session->get_connection_id();
    std::thread thd = mysqlsh::spawn_scoped_thread([connection_id]() {
      Mysql_thread mysql;
      session_wait(connection_id, 3, "Waiting", k_processlist_state_column);
      shcore::current_interrupt()->interrupt();
    });
    execute("session.get_schema('itst').drop_collection('cdata')");
    MY_EXPECT_STDERR_CONTAINS("nterrupt");
    thd.join();
    session_wait(connection_id, 3, "Sleep", k_processlist_command_column);
    // ensure next query runs ok
    execute("print('flush')");  // this will flush any pending async callbacks
    wipe_all();
  }

  {
    // thread should not use `session`, get the connection ID before creating it
    const auto connection_id = session->get_connection_id();
    std::thread thd = mysqlsh::spawn_scoped_thread([connection_id]() {
      Mysql_thread mysql;
      session_wait(connection_id, 3, "Waiting", k_processlist_state_column);
      shcore::current_interrupt()->interrupt();
    });
    execute("session.drop_schema('itst')");
    MY_EXPECT_STDERR_CONTAINS("nterrupt");
    thd.join();
    session_wait(connection_id, 3, "Sleep", k_processlist_command_column);
    // ensure next query runs ok
    execute("print('flush')");  // this will flush any pending async callbacks
    wipe_all();
  }

  conn->execute_sql("rollback");
  auto result = conn->execute_sql(
      "select count(*) from information_schema.tables where "
      "table_schema='itst'");
  auto row = result->fetch_one();
  EXPECT_EQ(2, row->get_int(0));
}

// Test that native JavaScript code gets interrupted
#ifdef HAVE_V8
TEST_F(Interrupt_mysql, javascript) {
  mysqlsh::Scoped_interrupt interrupt_handler(m_current_interrupt);
  // Test case for FR4 JS
  execute("\\js");
  std::thread thd = mysqlsh::spawn_scoped_thread([this]() {
    output_wait("READY", 3);
    shcore::current_interrupt()->interrupt();
  });
  wipe_all();
  EXPECT_NO_THROW(
      execute("print('READY');\n"
              "for(i=0;i<1000000000;i++) {};\n"
              "print('FAILED');"));
  MY_EXPECT_STDOUT_NOT_CONTAINS("FAILED");
  thd.join();
}
#endif

// Test that native Python code gets interrupted
TEST_F(Interrupt_mysql, python) {
  mysqlsh::Scoped_interrupt interrupt_handler(m_current_interrupt);
  // Test case for FR4 Py
  execute("\\py");
  execute("import time");
  execute(
      "def sleep():\n"
      "  for i in range(1000000000): time.sleep(0.1)");
  std::thread thd = mysqlsh::spawn_scoped_thread([this]() {
    output_wait("READY", 3);
    shcore::current_interrupt()->interrupt();
  });
  wipe_all();
  // execute() allows only for single_input as specified in Python's Grammar
  EXPECT_NO_THROW(
      execute("print('READY');"
              "sleep();"
              "print('FAILED')"));
  MY_EXPECT_STDOUT_NOT_CONTAINS("FAILED");
  thd.join();
}

// The following tests check interruption of resultset printing

// execute a query which takes time to execute like a big select * , then hit
// ^C (Ctrl + C) and verify the query is killed (Check in server connection
// log to verify the KILL QUERY was sent) and error is displayed to the user.

TEST_F(Interrupt_mysql, sql_classic_resultset) {
  mysqlsh::Scoped_interrupt interrupt_handler(m_current_interrupt);
  // Test case for FR3
  execute("\\sql");

  make_output_slow();
  Call_on_leave restore([this]() { unmake_output_slow(); });
  {
    std::thread thd = mysqlsh::spawn_scoped_thread([this]() {
      output_wait("first", 3);
      shcore::current_interrupt()->interrupt();
    });
    execute("select * from itst.data;");
    MY_EXPECT_STDOUT_NOT_CONTAINS("last");
    MY_EXPECT_STDOUT_CONTAINS(
        "Result printing interrupted, rows may be missing from the output.");
    thd.join();
  }
}

TEST_F(Interrupt_mysql, sql_classic_resultset_vertical) {
  mysqlsh::Scoped_interrupt interrupt_handler(m_current_interrupt);
  // Test case for FR3
  execute("\\sql");

  make_output_slow();
  Call_on_leave restore([this]() { unmake_output_slow(); });

  {
    std::thread thd = mysqlsh::spawn_scoped_thread([this]() {
      output_wait("first", 3);
      shcore::current_interrupt()->interrupt();
    });
    execute("select * from itst.data\\G");
    MY_EXPECT_STDOUT_NOT_CONTAINS("last");
    MY_EXPECT_STDOUT_CONTAINS(
        "Result printing interrupted, rows may be missing from the output.");
    thd.join();
  }
}

TEST_F(Interrupt_mysqlx, sql_x_sql_resultset) {
  mysqlsh::Scoped_interrupt interrupt_handler(m_current_interrupt);
  // Test case for FR3
  execute("\\sql");

  make_output_slow();
  Call_on_leave restore([this]() { unmake_output_slow(); });
  {
    std::thread thd = mysqlsh::spawn_scoped_thread([this]() {
      output_wait("first", 3);
      shcore::current_interrupt()->interrupt();
    });
    execute("select * from itst.data;");
    MY_EXPECT_STDOUT_NOT_CONTAINS("last");
    MY_EXPECT_STDOUT_CONTAINS(
        "Result printing interrupted, rows may be missing from the output.");
    thd.join();
  }
}

TEST_F(Interrupt_mysqlx, sql_x_sql_resultset_vertical) {
  mysqlsh::Scoped_interrupt interrupt_handler(m_current_interrupt);
  // Test case for FR3
  execute("\\sql");

  make_output_slow();
  Call_on_leave restore([this]() { unmake_output_slow(); });

  {
    std::thread thd = mysqlsh::spawn_scoped_thread([this]() {
      output_wait("first", 3);
      shcore::current_interrupt()->interrupt();
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
  mysqlsh::Scoped_interrupt interrupt_handler(m_current_interrupt);
  execute("\\js");
  execute("\\use itst");
  wipe_all();
  make_output_slow();
  {
    Call_on_leave restore([this]() { unmake_output_slow(); });
    std::thread thd = mysqlsh::spawn_scoped_thread([this]() {
      output_wait("first", 3);
      shcore::current_interrupt()->interrupt();
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
