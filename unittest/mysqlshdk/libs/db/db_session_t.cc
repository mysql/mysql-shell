/*
 * Copyright (c) 2017, 2026, Oracle and/or its affiliates.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.0,
 * as published by the Free Software Foundation.
 *
 * This program is designed to work with certain software (including
 * but not limited to OpenSSL) that is licensed under separate terms,
 * as designated in a particular file or component or in included license
 * documentation.  The authors of MySQL hereby grant you an additional
 * permission to link the program and your derivative works with the
 * separately licensed software that they have either included with
 * the program or referenced in the documentation.
 *
 * This program is distributed in the hope that it will be useful,  but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 * the GNU General Public License, version 2.0, for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "mysqlshdk/libs/db/mysql/session.h"
#include "mysqlshdk/libs/db/mysqlx/session.h"
#include "mysqlshdk/libs/db/utils/utils.h"
#include "mysqlshdk/libs/utils/process_launcher.h"
#include "mysqlshdk/libs/utils/utils_file.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_path.h"
#include "unittest/mysqlshdk/libs/db/db_common.h"
#include "unittest/test_utils.h"

extern "C" const char *g_test_home;

namespace mysqlshdk {
namespace db {

#ifdef HAVE_PYTHON
class Router_version_test : public tests::Shell_test_env {
 protected:
  void SetUp() override {
    Shell_test_env::SetUp();
    m_session = mysql::Session::create();
  }

  void TearDown() override {
    if (m_session) m_session->close();

    if (m_proxy) m_proxy->kill();

    Shell_test_env::TearDown();
  }

  void start_proxy(const char *greeting = "26.10.0-router", int error = 0,
                   bool invalid_version = false) {
    const auto executable =
        shcore::path::join_path(shcore::get_binary_folder(), "mysqlsh");
    const auto script =
        shcore::path::join_path(g_test_home, "data", "router_version_proxy.py");
    const auto error_code = std::to_string(error);
    const char *args[] = {executable.c_str(),
                          "--py",
                          "--file",
                          script.c_str(),
                          _mysql_port.c_str(),
                          greeting,
                          error_code.c_str(),
                          invalid_version ? "1" : "0",
                          nullptr};
    m_proxy = std::make_unique<shcore::Process_launcher>(args);
    m_proxy->enable_reader_thread();
    m_proxy->start();

    std::string output;

    for (int i = 0; i < 1000; ++i) {
      output += m_proxy->read_all();

      if (output.find('\n') != std::string::npos || m_proxy->check()) break;

      shcore::sleep_ms(10);
    }

    ASSERT_EQ(0, output.find("PORT ")) << output;
    m_options = Connection_options(_mysql_uri);
    m_options.set_host("127.0.0.1");
    m_options.set_port(std::stoi(output.substr(5)));
    m_options.get_ssl_options().set_mode(Ssl_mode::Disabled);
    m_options.set_compression("DISABLED");
  }

  void expect_probe_count(int expected) {
    EXPECT_EQ(expected, m_session->query("SELECT 'router_version_probe_count'")
                            ->fetch_one_or_throw()
                            ->get_int(0));
  }

  void expect_getters_preserve_result(const utils::Version &version,
                                      const char *version_sql) {
    auto result = m_session->query("SELECT 1 UNION ALL SELECT 2");
    EXPECT_EQ(1, result->fetch_one_or_throw()->get_int(0));
    EXPECT_EQ(version, m_session->get_server_version());
    EXPECT_STREQ(version_sql, get_server_version_sql(m_session));
    EXPECT_EQ(2, result->fetch_one_or_throw()->get_int(0));
    EXPECT_EQ(nullptr, result->fetch_one());
  }

  std::shared_ptr<mysql::Session> m_session;
  Connection_options m_options;
  std::unique_ptr<shcore::Process_launcher> m_proxy;
};

TEST_F(Router_version_test, caches_backend_version_and_refreshes_on_reconnect) {
  SKIP_UNLESS_DIRECT_MODE();
  EXPECT_FALSE(m_session->is_server_version_known());
  auto direct = mysql::open_session(Connection_options(_mysql_uri));
  const utils::Version backend(
      direct->query("SELECT @@version")->fetch_one_or_throw()->get_string(0));
  const auto version_sql =
      direct->query("SHOW VARIABLES LIKE 'version_with_update'")->fetch_one()
          ? "@@version_with_update"
          : "@@version";
  ASSERT_NO_FATAL_FAILURE(start_proxy("99.1.0-router"));
  m_session->connect(m_options);
  EXPECT_TRUE(m_session->is_server_version_known());
  EXPECT_EQ(backend, m_session->get_server_version());
  expect_probe_count(1);
  expect_getters_preserve_result(backend, version_sql);
  expect_probe_count(1);

  m_session->close();
  EXPECT_FALSE(m_session->is_server_version_known());
  EXPECT_EQ(utils::Version(), m_session->get_server_version());
  m_session->connect(m_options);
  EXPECT_TRUE(m_session->is_server_version_known());
  EXPECT_EQ(backend, m_session->get_server_version());
  expect_probe_count(1);

  m_session->close();
  m_session->connect(Connection_options(_mysql_uri));
  EXPECT_EQ(direct->get_server_version(), m_session->get_server_version());

  m_session->close();
  m_session->connect(m_options);
  auto *handle = m_session->release_connection();
  shcore::Scoped_callback cleanup([handle]() { mysql_close(handle); });
  EXPECT_FALSE(m_session->is_open());
  EXPECT_FALSE(m_session->is_server_version_known());
  EXPECT_EQ(utils::Version(), m_session->get_server_version());
  m_session->connect(Connection_options(_mysql_uri));
  EXPECT_EQ(direct->get_server_version(), m_session->get_server_version());
}

TEST_F(Router_version_test, direct_greeting_does_not_query_backend_version) {
  SKIP_UNLESS_DIRECT_MODE();
  for (const auto *greeting : {"26.10.0-commercial", "26.10.0-router-extra"}) {
    SCOPED_TRACE(greeting);
    ASSERT_NO_FATAL_FAILURE(start_proxy(greeting, ER_PARSE_ERROR));
    m_session->connect(m_options);
    expect_getters_preserve_result(utils::Version(26, 10, 0),
                                   "@@version_with_update");
    expect_probe_count(0);
    m_session->close();
  }
}

TEST_F(Router_version_test,
       expired_password_keeps_fallback_after_password_reset) {
  SKIP_UNLESS_DIRECT_MODE();
  auto admin = mysql::open_session(Connection_options(_mysql_uri));
  admin->execute("DROP USER IF EXISTS 'router_version_expired'@'%'");
  admin->execute(
      "CREATE USER 'router_version_expired'@'%' "
      "IDENTIFIED BY 'expired-password' PASSWORD EXPIRE");
  shcore::Scoped_callback cleanup(
      [&]() { admin->execute("DROP USER 'router_version_expired'@'%'"); });
  ASSERT_NO_FATAL_FAILURE(start_proxy());
  m_options.set_user("router_version_expired");
  m_options.set_password("expired-password");
  m_options.set(kCanHandleExpiredPasswords, "true");
  m_options.set(kGetServerPublicKey, "true");
  ASSERT_NO_THROW(m_session->connect(m_options));
  EXPECT_EQ(utils::Version(26, 10, 0), m_session->get_server_version());
  EXPECT_STREQ("@@version", get_server_version_sql(m_session));

  m_session->execute("SET PASSWORD = 'updated-password'");
  expect_getters_preserve_result(utils::Version(26, 10, 0), "@@version");
  expect_probe_count(1);
  EXPECT_NO_THROW(m_session->query(
      shcore::str_format("SELECT CONCAT(%s, ' ', @@version_comment)",
                         get_server_version_sql(m_session))));

  m_session->close();
  m_options.set_password("updated-password");
  m_session->connect(m_options);
  const utils::Version backend(
      admin->query("SELECT @@version")->fetch_one_or_throw()->get_string(0));
  EXPECT_EQ(backend, m_session->get_server_version());
  expect_probe_count(1);
}

TEST_F(Router_version_test, version_lookup_failures_use_fallback) {
  SKIP_UNLESS_DIRECT_MODE();
  for (const bool invalid_version : {false, true}) {
    SCOPED_TRACE(invalid_version ? "invalid version" : "query error");
    ASSERT_NO_FATAL_FAILURE(start_proxy("26.10.0-router",
                                        invalid_version ? 0 : ER_PARSE_ERROR,
                                        invalid_version));
    ASSERT_NO_THROW(m_session->connect(m_options));
    EXPECT_TRUE(m_session->is_open());
    EXPECT_FALSE(m_session->is_server_version_known());
    expect_getters_preserve_result(utils::Version(26, 10, 0), "@@version");
    expect_probe_count(1);
    m_session->close();
  }
}

TEST_F(Router_version_test, record_replay_preserves_version_fallback) {
  SKIP_UNLESS_DIRECT_MODE();
  const auto folder = shcore::create_temporary_folder();
  const std::string previous_prefix = replay::g_recording_path_prefix;
  const auto previous_context =
      replay::current_recording_dir().substr(previous_prefix.size());
  shcore::Scoped_callback cleanup([&]() {
    replay::set_recording_path_prefix(previous_prefix);
    replay::begin_recording_context(previous_context);
    shcore::remove_directory(folder, true);
  });
  replay::set_recording_path_prefix(folder + "/");

  auto direct = mysql::open_session(Connection_options(_mysql_uri));
  const auto backend_version_sql =
      direct->query("SHOW VARIABLES LIKE 'version_with_update'")->fetch_one()
          ? "@@version_with_update"
          : "@@version";

  auto replayed = std::make_shared<replay::Replayer_mysql>();
  EXPECT_FALSE(replayed->is_server_version_known());
  replay::begin_recording_context("router-version");

  for (const auto error : {ER_MUST_CHANGE_PASSWORD, 0}) {
    SCOPED_TRACE(error);
    ASSERT_NO_FATAL_FAILURE(start_proxy("26.10.0-router", error));
    auto recorded = std::make_shared<replay::Recorder_mysql>();
    recorded->connect(m_options);
    const auto version = recorded->get_server_version();
    const auto version_sql = error ? "@@version" : backend_version_sql;
    EXPECT_EQ(!error, recorded->is_server_version_known());
    EXPECT_STREQ(version_sql, get_server_version_sql(recorded));
    const auto sql = shcore::str_format("SELECT CONCAT(%s, '')", version_sql);
    const auto version_text =
        recorded->query(sql)->fetch_one_or_throw()->get_string(0);
    recorded->close();
    EXPECT_FALSE(recorded->is_server_version_known());

    replayed->connect(m_options);
    EXPECT_EQ(version, replayed->get_server_version());
    EXPECT_EQ(!error, replayed->is_server_version_known());
    EXPECT_STREQ(version_sql, get_server_version_sql(replayed));
    EXPECT_EQ(version_text,
              replayed->query(sql)->fetch_one_or_throw()->get_string(0));
    replayed->close();
    EXPECT_FALSE(replayed->is_server_version_known());
  }
}
#endif  // HAVE_PYTHON

std::shared_ptr<mysqlshdk::db::ISession> Db_tests::make_session() {
  if (is_classic)
    return mysqlshdk::db::mysql::Session::create();
  else
    return mysqlshdk::db::mysqlx::Session::create();
}

void Db_tests::TearDownTestCase() { run_script_classic({"drop schema xtest"}); }

void Db_tests::SetUp() {
  is_classic = true;
  Shell_test_env::SetUp();
  std::cout << "Testing classic\n";
  session = mysqlshdk::db::mysql::Session::create();
}

bool Db_tests::switch_proto() {
  if (is_classic) {
    is_classic = false;
    std::cout << "Testing X proto\n";
    session = mysqlshdk::db::mysqlx::Session::create();
    return true;
  }
  return false;
}

TEST_F(Db_tests, connect_uri) {
  do {
    SCOPED_TRACE(is_classic ? "mysql" : "mysqlx");
    EXPECT_FALSE(session->is_server_version_known());

    // Connection failure
    {
      auto connection_options = mysqlshdk::db::Connection_options(uri_nopass());
      connection_options.set_password("fake_pwd");

      EXPECT_THROW(session->connect(connection_options), std::exception);
      EXPECT_FALSE(session->is_server_version_known());
    }

    // Success connection, no schema selected
    {
      auto connection_options = mysqlshdk::db::Connection_options(uri());

      ASSERT_NO_THROW(session->connect(connection_options));
      EXPECT_TRUE(session->is_server_version_known());
      auto result = std::shared_ptr<mysqlshdk::db::IResult>(
          session->query("select database()"));
      auto row = result->fetch_one();
      EXPECT_TRUE(row->is_null(0));
      session->close();
      EXPECT_FALSE(session->is_server_version_known());
    }

    // Success connection, default schema
    {
      auto connection_options =
          mysqlshdk::db::Connection_options(uri() + "/mysql");
      ASSERT_NO_THROW(session->connect(connection_options));
      EXPECT_TRUE(session->is_server_version_known());
      auto result = std::shared_ptr<mysqlshdk::db::IResult>(
          session->query("select database()"));
      auto row = result->fetch_one();
      EXPECT_FALSE(row->is_null(0));
      EXPECT_EQ("mysql", row->get_string(0));
      session->close();
      EXPECT_FALSE(session->is_server_version_known());
    }
  } while (switch_proto());
}

TEST_F(Db_tests, execute) {
  do {
    SCOPED_TRACE(is_classic ? "mysql" : "mysqlx");
    auto connection_options = mysqlshdk::db::Connection_options(uri());
    EXPECT_NO_THROW(session->connect(connection_options));

    // Execute error, trying to use an unexisting schema
    EXPECT_THROW(session->execute("use some_weird_schema"), std::exception);

    // Execute success, use a valid schema
    EXPECT_NO_THROW(session->execute("use mysql"));

    session->close();
  } while (switch_proto());
}

TEST_F(Db_tests, query) {
  do {
    SCOPED_TRACE(is_classic ? "mysql" : "mysqlx");
    auto connection_options = mysqlshdk::db::Connection_options(uri());

    EXPECT_NO_THROW(session->connect(connection_options));

    // Query error, trying to retrieve an unexisting system variable
    EXPECT_THROW(session->query("select @@some_weird_variable"),
                 std::exception);

    // Query success, retrieving a valid system variable
    {
      auto result = std::shared_ptr<mysqlshdk::db::IResult>(
          session->query("select @@server_id"));
    }

    // Query success, retrieving a valid system variable, again
    {
      auto result = std::shared_ptr<mysqlshdk::db::IResult>(
          session->query("select @@server_id"));
    }

    // Query success, using buffered result
    {
      auto result = std::shared_ptr<mysqlshdk::db::IResult>(
          session->query(std::string("select @@server_id"), true));
    }

    // Required test database for the rest of this test
    EXPECT_NO_THROW(session->execute("drop schema if exists Db_sessions"));
    EXPECT_NO_THROW(session->execute("create schema Db_sessions"));
    EXPECT_NO_THROW(session->execute("use Db_sessions"));

    // Uses a stored procedure to test multi result behavior
    {
      std::string procedure =
          "create procedure test_query() begin select @@server_id; select "
          "@@server_uuid; end";
      EXPECT_NO_THROW(session->execute(procedure));
      auto result = session->query("call test_query()");
      EXPECT_TRUE(result->next_resultset());
      EXPECT_TRUE(result->next_resultset());
      EXPECT_FALSE(result->next_resultset());
    }

    // Pulls multiple results and ensures they are discarded
    // if a new execution occurs
    if (is_classic) {
      // TODO(alfredo) ensure common behaviour
      // at this layer, the pending resultset should not be discarded (or
      // cached) but rather throw an exception
      auto result = std::shared_ptr<mysqlshdk::db::IResult>(
          session->query("call test_query()"));
      result = std::shared_ptr<mysqlshdk::db::IResult>(
          session->query("select current_user()"));
      auto row = result->fetch_one();
      std::string user = _user + "@";
      EXPECT_EQ(row->get_string(0).find(user), 0);
    } else {
      PENDING_BUG_TEST(
          "Need to ensure proper behavior for non-consumed results");
    }

    EXPECT_NO_THROW(session->execute("drop schema Db_sessions"));

    session->close();
  } while (switch_proto());
}

TEST_F(Db_tests, auto_close) {
  do {
    SCOPED_TRACE(is_classic ? "mysql" : "mysqlx");
    ASSERT_NO_THROW(session->connect(Connection_options(uri())));

    auto result = session->query(
        "select count(*) from performance_schema.threads where "
        "PROCESSLIST_USER IS NOT NULL");
    int ocount = result->fetch_one()->get_int(0);

    {
      auto connection_options = mysqlshdk::db::Connection_options(uri());
      auto mysql_session = make_session();
      EXPECT_NO_THROW(mysql_session->connect(connection_options));

      EXPECT_BECOMES_TRUE(
          3,
          (ocount + 1) ==
              session
                  ->query(
                      "select count(*) from performance_schema.threads where "
                      "PROCESSLIST_USER IS NOT NULL")
                  ->fetch_one_or_throw()
                  ->get_int(0));
    }

    bool ok = false;
    int count = 0;
    for (int i = 0; i < 10; i++) {
      count = session
                  ->query(
                      "select count(*) from performance_schema.threads where "
                      "PROCESSLIST_USER IS NOT NULL")
                  ->fetch_one()
                  ->get_int(0);
      if (count == ocount) {
        ok = true;
        break;
      }
      shcore::sleep_ms(1000);
    }
    if (!ok) {
      EXPECT_EQ(ocount, count);
    }
  } while (switch_proto());
}

TEST_F(Db_tests, connect_read_timeout) {
  auto connection_options = mysqlshdk::db::Connection_options(uri());

  connection_options.set_unchecked("net-read-timeout", "1000");

  EXPECT_NO_THROW(session->connect(connection_options));

  EXPECT_NO_THROW(session->execute("SELECT SLEEP(0)"));

  EXPECT_THROW_LIKE(session->execute("SELECT SLEEP(2)"), mysqlshdk::db::Error,
                    "Lost connection to MySQL server during query");
}

TEST_F(Db_tests, connect_write_timeout) {
  auto connection_options = mysqlshdk::db::Connection_options(uri());

  connection_options.set_unchecked("net-write-timeout", "500");

  EXPECT_NO_THROW(session->connect(connection_options));

  // no way to easily test this
}

TEST_F(Db_tests, connect_local_infile_reject_policy_is_per_connection) {
  const auto mysql_session =
      std::dynamic_pointer_cast<mysqlshdk::db::mysql::Session>(session);
  ASSERT_NE(nullptr, mysql_session);

  auto reject_options = mysqlshdk::db::Connection_options(uri());
  reject_options.set(mysqlshdk::db::kLocalInfile, "true");
  reject_options.set_reject_local_infile_requests(true);

  ASSERT_NO_THROW(mysql_session->connect(reject_options));
  auto *handle = mysql_session->get_handle();
  ASSERT_NE(nullptr, handle);
  EXPECT_NE(nullptr, handle->options.local_infile_init);
  EXPECT_NE(nullptr, handle->options.local_infile_read);
  EXPECT_NE(nullptr, handle->options.local_infile_end);
  EXPECT_NE(nullptr, handle->options.local_infile_error);
  EXPECT_EQ(nullptr, handle->options.local_infile_userdata);

  mysql_session->close();

  auto regular_options = mysqlshdk::db::Connection_options(uri());
  regular_options.set(mysqlshdk::db::kLocalInfile, "true");

  ASSERT_NO_THROW(mysql_session->connect(regular_options));
  handle = mysql_session->get_handle();
  ASSERT_NE(nullptr, handle);
  EXPECT_EQ(nullptr, handle->options.local_infile_init);
  EXPECT_EQ(nullptr, handle->options.local_infile_read);
  EXPECT_EQ(nullptr, handle->options.local_infile_end);
  EXPECT_EQ(nullptr, handle->options.local_infile_error);
  EXPECT_EQ(nullptr, handle->options.local_infile_userdata);
}

TEST_F(Db_tests, connect_max_allowed_packet) {
  auto connection_options = mysqlshdk::db::Connection_options(uri());

  connection_options.set_unchecked("max-allowed-packet", "4096");
  connection_options.set_unchecked("net-buffer-length", "2048");

  EXPECT_NO_THROW(session->connect(connection_options));

  session->execute("use mysql");
  session->execute(
      "create temporary table big (a int primary key, b longtext)");
  session->execute("insert into big values (1, repeat('x', 10000))");

  auto result = session->query("select * from big");
  EXPECT_THROW_LIKE(result->fetch_one(), mysqlshdk::db::Error,
                    "Got packet bigger than 'max_allowed_packet' bytes");
}

}  // namespace db
}  // namespace mysqlshdk
