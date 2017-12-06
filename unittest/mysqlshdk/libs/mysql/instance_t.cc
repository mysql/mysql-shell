/* Copyright (c) 2017, Oracle and/or its affiliates. All rights reserved.

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

#include "mysqlshdk/libs/mysql/instance.h"
#include "unittest/test_utils/mocks/mysqlshdk/libs/db/mock_result.h"
#include "unittest/test_utils/mocks/mysqlshdk/libs/db/mock_session.h"
#include "unittest/test_utils/shell_base_test.h"
#include "mysqlshdk/libs/utils/utils_general.h"

using mysqlshdk::db::Type;
namespace testing {

// Fake deleter for the _session to avoid it attempt deleting the passed
// Session object, since we are passing a pointer created from a reference
struct DoNotDelete {
  template <typename T>
  void operator()(T *) {}
};

class Instance_test : public tests::Shell_base_test {
 protected:
  // An Instance requires an ISession shared pointer
  std::shared_ptr<mysqlshdk::db::ISession> _session;
  Mock_session session;
  mysqlshdk::db::Connection_options _connection_options;

  virtual void SetUp() {
    tests::Shell_base_test::SetUp();

    _session.reset(&session, DoNotDelete());

    _connection_options = shcore::get_connection_options(_mysql_uri);
  }
};

TEST_F(Instance_test, get_session) {
  mysqlshdk::mysql::Instance instance(_session);

  EXPECT_EQ(_session, instance.get_session());
}

TEST_F(Instance_test, get_sysvar_string_existing_variable) {
  EXPECT_CALL(session, connect(_connection_options));
  _session->connect(_connection_options);

  mysqlshdk::mysql::Instance instance(_session);

  // Get string system variable with SESSION scope.
  session.expect_query(
          "show SESSION variables "
          "where `variable_name` in ('character_set_server')")
      .then_return(
          {{"show SESSION variables "
            "where `variable_name` in ('character_set_server')",
            {"Variable_name", "Value"},
            {Type::String, Type::String},
            {{"character_set_server", "latin1"}}}});
  mysqlshdk::utils::nullable<std::string> char_set_server =
      instance.get_sysvar_string("character_set_server",
                                 mysqlshdk::mysql::VarScope::SESSION);
  EXPECT_FALSE(char_set_server.is_null());
  EXPECT_STREQ("latin1", (*char_set_server).c_str());

  // Get string system variable with GLOBAL scope.
  session.expect_query(
          "show GLOBAL variables "
          "where `variable_name` in ('character_set_server')")
      .then_return(
          {{"show GLOBAL variables "
            "where `variable_name` in ('character_set_server')",
            {"Variable_name", "Value"},
            {Type::String, Type::String},
            {{"character_set_server", "latin1"}}}});
  char_set_server = instance.get_sysvar_string(
      "character_set_server", mysqlshdk::mysql::VarScope::GLOBAL);
  EXPECT_FALSE(char_set_server.is_null());
  EXPECT_STREQ("latin1", (*char_set_server).c_str());

  EXPECT_CALL(session, close());
  _session->close();
}

TEST_F(Instance_test, get_sysvar_string_unexisting_variable) {
  EXPECT_CALL(session, connect(_connection_options));
  _session->connect(_connection_options);

  session.expect_query("show SESSION variables where `variable_name` in"
                       " ('unexisting_variable')").
    then_return({
      {
        "show SESSION variables where `variable_name` in"
        " ('unexisting_variable')",
        { "Variable_name", "Value" },
        { Type::String, Type::String },
        {}  // No Records...
      }
  });

  mysqlshdk::mysql::Instance instance(_session);
  mysqlshdk::utils::nullable<std::string> server_uuid =
      instance.get_sysvar_string("unexisting_variable");

  // The value was not modified
  EXPECT_TRUE(server_uuid.is_null());

  EXPECT_CALL(session, close());
  _session->close();
}

TEST_F(Instance_test, get_sysvar_boolean_existing_variable) {
  EXPECT_CALL(session, connect(_connection_options));
  _session->connect(_connection_options);
  mysqlshdk::mysql::Instance instance(_session);

  session.expect_query("show SESSION variables where `variable_name` in"
                       " ('sql_warnings')").
    then_return({
      {
        "show SESSION variables where `variable_name` in"
        " ('sql_warnings')",
        { "Variable_name", "Value" },
        { Type::String, Type::String },
        {
          { "sql_warnings", "OFF" }
        }
      }
  });
  mysqlshdk::utils::nullable<bool> sql_warnings =
      instance.get_sysvar_bool("sql_warnings");
  // The value was not modified
  EXPECT_FALSE(sql_warnings.is_null());
  EXPECT_FALSE(*sql_warnings);

  // Get boolean system variable with SESSION scope.
  session.expect_query(
          "show SESSION variables where `variable_name` in ('sql_warnings')")
      .then_return({{"show SESSION variables "
                     "where `variable_name` in ('sql_warnings')",
                     {"Variable_name", "Value"},
                     {Type::String, Type::String},
                     {{"sql_warnings", "OFF"}}}});
  sql_warnings = instance.get_sysvar_bool("sql_warnings",
                                          mysqlshdk::mysql::VarScope::SESSION);
  EXPECT_FALSE(sql_warnings.is_null());
  EXPECT_FALSE(*sql_warnings);

  // Get boolean system variable with GLOBAL scope.
  session.expect_query(
          "show GLOBAL variables where `variable_name` in ('sql_warnings')")
      .then_return({{"show GLOBAL variables "
                     "where `variable_name` in ('sql_warnings')",
                     {"Variable_name", "Value"},
                     {Type::String, Type::String},
                     {{"sql_warnings", "OFF"}}}});
  sql_warnings = instance.get_sysvar_bool("sql_warnings",
                                          mysqlshdk::mysql::VarScope::GLOBAL);
  EXPECT_FALSE(sql_warnings.is_null());
  EXPECT_FALSE(*sql_warnings);

  EXPECT_CALL(session, close());
  _session->close();
}

TEST_F(Instance_test, get_sysvar_boolean_unexisting_variable) {
  EXPECT_CALL(session, connect(_connection_options));
  _session->connect(_connection_options);

  session.expect_query("show SESSION variables where `variable_name` in"
                       " ('unexisting_variable')").
    then_return({
      {
        "show SESSION variables where `variable_name` in"
        " ('unexisting_variable')",
        { "Variable_name", "Value" },
        { Type::String, Type::String },
        {}  // No Records...
      }
  });

  mysqlshdk::mysql::Instance instance(_session);
  mysqlshdk::utils::nullable<bool> sql_warnings =
      instance.get_sysvar_bool("unexisting_variable");

  // The value was not modified
  EXPECT_TRUE(sql_warnings.is_null());

  EXPECT_CALL(session, close());
  _session->close();
}

TEST_F(Instance_test, get_sysvar_boolean_invalid_variable) {
  EXPECT_CALL(session, connect(_connection_options));
  _session->connect(_connection_options);

  session.expect_query("show SESSION variables where `variable_name` in"
                       " ('server_uuid')").
    then_return({
      {
        "show SESSION variables where `variable_name` in"
        " ('server_uuid')",
        { "Variable_name", "Value" },
        { Type::String, Type::String },
        {
          { "server_uuid", "891a2c04-1cc7-11e7-8323-00059a3c7a00" }
        }
      }
  });

  mysqlshdk::mysql::Instance instance(_session);
  EXPECT_ANY_THROW(instance.get_sysvar_bool("server_uuid"));

  EXPECT_CALL(session, close());
  _session->close();
}

TEST_F(Instance_test, get_sysvar_int_existing_variable) {
  EXPECT_CALL(session, connect(_connection_options));
  _session->connect(_connection_options);
  mysqlshdk::mysql::Instance instance(_session);

  session.expect_query("show SESSION variables where `variable_name` in"
                       " ('server_id')").
    then_return({
      {
        "show SESSION variables where `variable_name` in"
        " ('server_id')",
        { "Variable_name", "Value" },
        { Type::String, Type::String },
        {
          { "server_id", "0" }
        }
      }
  });
  mysqlshdk::utils::nullable<int64_t> server_id =
      instance.get_sysvar_int("server_id");
  // The value was not modified
  EXPECT_FALSE(server_id.is_null());
  EXPECT_EQ(0, *server_id);

  // Get integer system variable with SESSION scope.
  session.expect_query(
          "show SESSION variables "
          "where `variable_name` in ('wait_timeout')")
      .then_return({{"show SESSION variables "
                     "where `variable_name` in ('wait_timeout')",
                     {"Variable_name", "Value"},
                     {Type::String, Type::String},
                     {{"wait_timeout", "28800"}}}});
  mysqlshdk::utils::nullable<int64_t> wait_timeout = instance.get_sysvar_int(
      "wait_timeout", mysqlshdk::mysql::VarScope::SESSION);
  EXPECT_FALSE(wait_timeout.is_null());
  EXPECT_EQ(28800, *wait_timeout);

  // Get integer system variable with GLOBAL scope.
  session.expect_query(
          "show GLOBAL variables "
          "where `variable_name` in ('wait_timeout')")
      .then_return({{"show GLOBAL variables "
                     "where `variable_name` in ('wait_timeout')",
                     {"Variable_name", "Value"},
                     {Type::String, Type::String},
                     {{"wait_timeout", "28800"}}}});
  wait_timeout = instance.get_sysvar_int("wait_timeout",
                                         mysqlshdk::mysql::VarScope::GLOBAL);
  EXPECT_FALSE(wait_timeout.is_null());
  EXPECT_EQ(28800, *wait_timeout);

  EXPECT_CALL(session, close());
  _session->close();
}

TEST_F(Instance_test, get_sysvar_int_unexisting_variable) {
  EXPECT_CALL(session, connect(_connection_options));
  _session->connect(_connection_options);

  session.expect_query("show SESSION variables where `variable_name` in"
                       " ('unexisting_variable')").
    then_return({
      {
        "show SESSION variables where `variable_name` in"
        " ('unexisting_variable')",
        { "Variable_name", "Value" },
        { Type::String, Type::String },
        {}  // No Records...
      }
  });

  mysqlshdk::mysql::Instance instance(_session);
  mysqlshdk::utils::nullable<int64_t> server_id =
      instance.get_sysvar_int("unexisting_variable");

  // The value was not modified
  EXPECT_TRUE(server_id.is_null());

  EXPECT_CALL(session, close());
  _session->close();
}

TEST_F(Instance_test, get_sysvar_int_invalid_variable) {
  EXPECT_CALL(session, connect(_connection_options));
  _session->connect(_connection_options);

  session.expect_query("show SESSION variables where `variable_name` in"
                       " ('server_uuid')").
    then_return({
      {
        "show SESSION variables where `variable_name` in"
        " ('server_uuid')",
        { "Variable_name", "Value" },
        { Type::String, Type::String },
        {
          { "server_uuid", "891a2c04-1cc7-11e7-8323-00059a3c7a00" }
        }
      }
  });

  mysqlshdk::mysql::Instance instance(_session);
  EXPECT_ANY_THROW(instance.get_sysvar_int("server_uuid"));

  EXPECT_CALL(session, close());
  _session->close();
}

TEST_F(Instance_test, set_sysvar) {
  EXPECT_CALL(session, connect(_connection_options));
  _session->connect(_connection_options);
  mysqlshdk::mysql::Instance instance(_session);

  // Test set string system variable with different scopes (GLOBAL and SESSION).
  EXPECT_CALL(session, execute("SET SESSION `lc_messages` = 'fr_FR'"));
  instance.set_sysvar("lc_messages", (std::string) "fr_FR");
  session.expect_query(
          "show SESSION variables where `variable_name` in ('lc_messages')")
      .then_return({{"show SESSION variables "
                     "where `variable_name` in ('lc_messages')",
                     {"Variable_name", "Value"},
                     {Type::String, Type::String},
                     {{"lc_messages", "fr_FR"}}}});
  mysqlshdk::utils::nullable<std::string> new_value =
      instance.get_sysvar_string("lc_messages");
  EXPECT_STREQ("fr_FR", (*new_value).c_str());
  EXPECT_CALL(session, execute("SET GLOBAL `lc_messages` = 'pt_PT'"));
  instance.set_sysvar("lc_messages", (std::string) "pt_PT",
                      mysqlshdk::mysql::VarScope::GLOBAL);
  session.expect_query(
          "show GLOBAL variables where `variable_name` in ('lc_messages')")
      .then_return({{"show GLOBAL variables "
                     "where `variable_name` in ('lc_messages')",
                     {"Variable_name", "Value"},
                     {Type::String, Type::String},
                     {{"lc_messages", "pt_PT"}}}});
  new_value = instance.get_sysvar_string("lc_messages",
                                         mysqlshdk::mysql::VarScope::GLOBAL);
  EXPECT_STREQ("pt_PT", (*new_value).c_str());
  EXPECT_CALL(session, execute("SET SESSION `lc_messages` = 'es_MX'"));
  instance.set_sysvar("lc_messages", (std::string) "es_MX",
                      mysqlshdk::mysql::VarScope::SESSION);
  session.expect_query(
          "show SESSION variables where `variable_name` in ('lc_messages')")
      .then_return({{"show SESSION variables "
                     "where `variable_name` in ('lc_messages')",
                     {"Variable_name", "Value"},
                     {Type::String, Type::String},
                     {{"lc_messages", "es_MX"}}}});
  new_value = instance.get_sysvar_string("lc_messages",
                                         mysqlshdk::mysql::VarScope::SESSION);
  EXPECT_STREQ("es_MX", (*new_value).c_str());

  // Test set int system variable with different scopes (GLOBAL and SESSION).
  EXPECT_CALL(session, execute("SET SESSION `lock_wait_timeout` = 86400"));
  instance.set_sysvar("lock_wait_timeout", (int64_t) 86400);
  session.expect_query(
          "show SESSION variables "
          "where `variable_name` in ('lock_wait_timeout')")
      .then_return({{"show SESSION variables "
                     "where `variable_name` in ('lock_wait_timeout')",
                     {"Variable_name", "Value"},
                     {Type::String, Type::String},
                     {{"lock_wait_timeout", "86400"}}}});
  mysqlshdk::utils::nullable<int64_t> new_i_value = instance.get_sysvar_int(
      "lock_wait_timeout");
  EXPECT_EQ(86400, *new_i_value);
  EXPECT_CALL(session, execute("SET GLOBAL `lock_wait_timeout` = 172800"));
  instance.set_sysvar("lock_wait_timeout", (int64_t) 172800,
                      mysqlshdk::mysql::VarScope::GLOBAL);
  session.expect_query(
          "show GLOBAL variables "
          "where `variable_name` in ('lock_wait_timeout')")
      .then_return({{"show GLOBAL variables "
                     "where `variable_name` in ('lock_wait_timeout')",
                     {"Variable_name", "Value"},
                     {Type::String, Type::String},
                     {{"lock_wait_timeout", "172800"}}}});
  new_i_value = instance.get_sysvar_int("lock_wait_timeout",
                                        mysqlshdk::mysql::VarScope::GLOBAL);
  EXPECT_EQ(172800, *new_i_value);
  EXPECT_CALL(session, execute("SET SESSION `lock_wait_timeout` = 43200"));
  instance.set_sysvar("lock_wait_timeout", (int64_t) 43200,
                      mysqlshdk::mysql::VarScope::SESSION);
  session.expect_query(
          "show SESSION variables "
          "where `variable_name` in ('lock_wait_timeout')")
      .then_return({{"show SESSION variables "
                     "where `variable_name` in ('lock_wait_timeout')",
                     {"Variable_name", "Value"},
                     {Type::String, Type::String},
                     {{"lock_wait_timeout", "43200"}}}});
  new_i_value = instance.get_sysvar_int("lock_wait_timeout",
                                        mysqlshdk::mysql::VarScope::SESSION);
  EXPECT_EQ(43200, *new_i_value);

  // Test set bool system variable with different scopes (GLOBAL and SESSION).
  EXPECT_CALL(session, execute("SET SESSION `sql_log_off` = 'ON'"));
  instance.set_sysvar("sql_log_off", true);
  session.expect_query(
          "show SESSION variables where `variable_name` in ('sql_log_off')")
      .then_return({{"show SESSION variables "
                     "where `variable_name` in ('sql_log_off')",
                     {"Variable_name", "Value"},
                     {Type::String, Type::String},
                     {{"sql_log_off", "ON"}}}});
  mysqlshdk::utils::nullable<bool> new_b_value = instance.get_sysvar_bool(
      "sql_log_off");
  EXPECT_TRUE(*new_b_value);
  EXPECT_CALL(session, execute("SET GLOBAL `sql_log_off` = 'ON'"));
  instance.set_sysvar("sql_log_off", true, mysqlshdk::mysql::VarScope::GLOBAL);
  session.expect_query(
          "show GLOBAL variables where `variable_name` in ('sql_log_off')")
      .then_return({{"show GLOBAL variables "
                     "where `variable_name` in ('sql_log_off')",
                     {"Variable_name", "Value"},
                     {Type::String, Type::String},
                     {{"sql_log_off", "ON"}}}});
  new_b_value = instance.get_sysvar_bool("sql_log_off",
                                         mysqlshdk::mysql::VarScope::GLOBAL);
  EXPECT_TRUE(*new_b_value);
  EXPECT_CALL(session, execute("SET SESSION `sql_log_off` = 'OFF'"));
  instance.set_sysvar("sql_log_off", false,
                      mysqlshdk::mysql::VarScope::SESSION);
  session.expect_query(
          "show SESSION variables where `variable_name` in ('sql_log_off')")
      .then_return({{"show SESSION variables "
                     "where `variable_name` in ('sql_log_off')",
                     {"Variable_name", "Value"},
                     {Type::String, Type::String},
                     {{"sql_log_off", "OFF"}}}});
  new_b_value = instance.get_sysvar_bool("sql_log_off",
                                         mysqlshdk::mysql::VarScope::SESSION);
  EXPECT_FALSE(*new_b_value);

  EXPECT_CALL(session, close());
  _session->close();
}

TEST_F(Instance_test, get_system_variables) {
  EXPECT_CALL(session, connect(_connection_options));
  _session->connect(_connection_options);

  session.expect_query("show SESSION variables where `variable_name` in"
                       " ('server_id', 'server_uuid', 'unexisting_variable')").
    then_return({
      {
        "show SESSION variables where `variable_name` in"
        " ('server_id', 'server_uuid', 'unexisting_variable')",
        { "Variable_name", "Value" },
        { Type::String, Type::String },
        {
          { "server_id", "0" },
          { "server_uuid", "891a2c04-1cc7-11e7-8323-00059a3c7a00" }
        }
      }
  });

  mysqlshdk::mysql::Instance instance(_session);
  auto variables = instance.get_system_variables(
      {"server_id", "server_uuid", "unexisting_variable"});

  EXPECT_EQ(3, variables.size());

  EXPECT_FALSE(variables["server_id"].is_null());
  EXPECT_FALSE(variables["server_uuid"].is_null());
  EXPECT_TRUE(variables["unexisting_variable"].is_null());

  EXPECT_CALL(session, close());
  _session->close();
}

TEST_F(Instance_test, install_plugin) {
  EXPECT_CALL(session, connect(_connection_options));
  _session->connect(_connection_options);
  mysqlshdk::mysql::Instance instance(_session);

  // Install plugin on Windows
  session.expect_query("show SESSION variables where `variable_name` in "
                       "('version_compile_os')").
      then_return({
                      { "",
                        {"Variable_name", "Value"},
                        {Type::String, Type::String},
                        {
                            {"version_compile_os", "Win64"}
                        }
                      }
                  });
  EXPECT_CALL(session,
              execute("INSTALL PLUGIN `validate_password` SONAME "
                          "'validate_password.dll'"));
  instance.install_plugin("validate_password");

  // Install plugin on Non-Windows
  session.expect_query("show SESSION variables where `variable_name` in "
                       "('version_compile_os')").
    then_return({
      { "",
        {"Variable_name", "Value"},
        {Type::String, Type::String},
        {
            {"version_compile_os", "linux-glibc2.5"}
        }
      }
    });
  EXPECT_CALL(session,
              execute("INSTALL PLUGIN `validate_password` SONAME "
                      "'validate_password.so'"));
  instance.install_plugin("validate_password");

  // Second install fails because plugin is already installed.
  session.expect_query("show SESSION variables where `variable_name` in "
                           "('version_compile_os')").
      then_return({
                      {   "",
                          {"Variable_name", "Value"},
                          {Type::String, Type::String},
                          {
                              {"version_compile_os", "linux-glibc2.5"}
                          }
                      }
                  });
  EXPECT_CALL(session, execute("INSTALL PLUGIN `validate_password` SONAME "
                               "'validate_password.so'"))
      .Times(1)
      .WillRepeatedly(Throw(std::exception()));
  EXPECT_THROW(instance.install_plugin("validate_password"),
               std::runtime_error);
  EXPECT_CALL(session, close());
  _session->close();
}

TEST_F(Instance_test, get_plugin_status) {
  EXPECT_CALL(session, connect(_connection_options));
  _session->connect(_connection_options);
  mysqlshdk::mysql::Instance instance(_session);

  // Test plugin ACTIVE
  session.expect_query(
          "SELECT plugin_status FROM information_schema.plugins "
          "WHERE plugin_name = 'validate_password'")
      .then_return(
          {{"", {"plugin_status"}, {Type::String}, {{"ACTIVE"}}}});
  mysqlshdk::utils::nullable<std::string> res =
      instance.get_plugin_status("validate_password");
  ASSERT_FALSE(res.is_null());
  EXPECT_STREQ("ACTIVE", (*res).c_str());

  // Test plugin DISABLED
  session.expect_query(
          "SELECT plugin_status FROM information_schema.plugins "
          "WHERE plugin_name = 'validate_password'")
      .then_return(
          {{"", {"plugin_status"}, {Type::String}, {{"DISABLED"}}}});
  res = instance.get_plugin_status("validate_password");
  ASSERT_FALSE(res.is_null());
  EXPECT_STREQ("DISABLED", (*res).c_str());

  // Test plugin not available
  session.expect_query(
          "SELECT plugin_status FROM information_schema.plugins "
          "WHERE plugin_name = 'validate_password'")
      .then_return(
          {{"", {"plugin_status"}, {Type::String}, {}}});
  res = instance.get_plugin_status("validate_password");
  ASSERT_TRUE(res.is_null());

  EXPECT_CALL(session, close());
  _session->close();
}

TEST_F(Instance_test, uninstall_plugin) {
  EXPECT_CALL(session, connect(_connection_options));
  _session->connect(_connection_options);

  mysqlshdk::mysql::Instance instance(_session);
  EXPECT_CALL(session, execute("UNINSTALL PLUGIN `validate_password`"));
  instance.uninstall_plugin("validate_password");

  // Second uninstall fails because plugin was already uninstalled.
  EXPECT_CALL(session, execute("UNINSTALL PLUGIN `validate_password`"))
      .Times(1)
      .WillRepeatedly(Throw(std::exception()));
  EXPECT_THROW(instance.uninstall_plugin("validate_password"),
               std::runtime_error);
  EXPECT_CALL(session, close());
  _session->close();
}

TEST_F(Instance_test, create_user) {
  EXPECT_CALL(session, connect(_connection_options));
  _session->connect(_connection_options);
  mysqlshdk::mysql::Instance instance(_session);

  // Create database and tables for test (GRANT for non existing objects fail).
  EXPECT_CALL(session, execute("CREATE DATABASE test_db"));
  _session->execute("CREATE DATABASE test_db");
  EXPECT_CALL(session, execute("CREATE TABLE test_db.t1 (c1 INT)"));
  _session->execute("CREATE TABLE test_db.t1 (c1 INT)");
  EXPECT_CALL(session, execute("CREATE TABLE test_db.t2 (c1 INT)"));
  _session->execute("CREATE TABLE test_db.t2 (c1 INT)");
  EXPECT_CALL(session, execute("CREATE DATABASE test_db2"));
  _session->execute("CREATE DATABASE test_db2");
  EXPECT_CALL(session, execute("CREATE TABLE test_db2.t1 (c1 INT)"));
  _session->execute("CREATE TABLE test_db2.t1 (c1 INT)");

  // Create user with SELECT, INSERT, UPDATE on test_db.* and DELETE on
  // test_db.t1.
  EXPECT_CALL(session,
      execute("CREATE USER 'test_user'@'test_host' IDENTIFIED BY 'test_pwd'"));
  EXPECT_CALL(session,
              execute("GRANT SELECT, INSERT, UPDATE ON test_db.* "
                      "TO 'test_user'@'test_host'"));
  EXPECT_CALL(session,
              execute("GRANT DELETE ON test_db.t1 TO 'test_user'@'test_host'"));
  EXPECT_CALL(session,
              execute("GRANT ALTER,DROP ON test_db.t2 "
                      "TO 'test_user'@'test_host' WITH GRANT OPTION"));
  EXPECT_CALL(session,
              execute("GRANT SELECT ON test_db2.* "
                      "TO 'test_user'@'test_host' WITH GRANT OPTION"));
  EXPECT_CALL(session, execute("COMMIT"));
  std::vector<std::tuple<std::string, std::string, bool>> test_priv =
      {std::make_tuple("SELECT, INSERT, UPDATE", "test_db.*", false),
       std::make_tuple("DELETE", "test_db.t1", false),
       std::make_tuple("ALTER,DROP", "test_db.t2", true),
       std::make_tuple("SELECT", "test_db2.*", true)};
  instance.create_user("test_user", "test_host", "test_pwd", test_priv);
  // Create user with ALL on *.* WITH GRANT OPTION.
  EXPECT_CALL(
      session,
      execute("CREATE USER 'dba_user'@'dba_host' IDENTIFIED BY 'dba_pwd'"));
  EXPECT_CALL(
      session,
      execute("GRANT ALL ON *.* TO 'dba_user'@'dba_host' WITH GRANT OPTION"));
  EXPECT_CALL(session, execute("COMMIT"));
  std::vector<std::tuple<std::string, std::string, bool>> dba_priv =
      {std::make_tuple("ALL", "*.*", true)};
  instance.create_user("dba_user", "dba_host", "dba_pwd", dba_priv);

  // Second create users fail because they already exist.
  EXPECT_CALL(
      session,
      execute("CREATE USER 'test_user'@'test_host' IDENTIFIED BY 'test_pwd'"))
      .Times(1)
      .WillRepeatedly(Throw(std::exception()));
  EXPECT_CALL(session, execute("ROLLBACK"));
  EXPECT_THROW(instance.create_user("test_user", "test_host", "test_pwd",
                                    test_priv),
               std::exception);
  EXPECT_CALL(
      session,
      execute("CREATE USER 'dba_user'@'dba_host' IDENTIFIED BY 'dba_pwd'"))
      .Times(1)
      .WillRepeatedly(Throw(std::exception()));
  EXPECT_CALL(session, execute("ROLLBACK"));
  EXPECT_THROW(instance.create_user("dba_user", "dba_host", "dba_pwd",
                                    dba_priv),
               std::exception);
  EXPECT_CALL(session, close());
  _session->close();
}

TEST_F(Instance_test, check_user_not_exist) {
  EXPECT_CALL(session, connect(_connection_options));
  _session->connect(_connection_options);
  mysqlshdk::mysql::Instance instance(_session);

  // Check non existing user.
  session
      .expect_query("SELECT PRIVILEGE_TYPE, IS_GRANTABLE "
                    "FROM INFORMATION_SCHEMA.USER_PRIVILEGES "
                    "WHERE GRANTEE = \"'notexist_user'@'notexist_host'\"")
      .then_return({{
          "",
          {"PRIVILEGE_TYPE", "IS_GRANTABLE"},
          {Type::String, Type::String},
          {}  // No Records
      }});
  std::tuple<bool, std::string, bool> res = instance.check_user(
      "notexist_user", "notexist_host", std::vector<std::string>(), "*", "*");
  EXPECT_FALSE(std::get<0>(res));
  EXPECT_STREQ("", std::get<1>(res).c_str());
  EXPECT_FALSE(std::get<2>(res));

  EXPECT_CALL(session, close());
  _session->close();
}

TEST_F(Instance_test, check_user_invalid_privileges) {
  EXPECT_CALL(session, connect(_connection_options));
  _session->connect(_connection_options);
  mysqlshdk::mysql::Instance instance(_session);

  // Use of invalid privileges list.
  EXPECT_THROW(instance.check_user(
      "test_user", "test_host",
      std::vector<std::string>({"SELECT", "NO_PRIV", "INSERT"}),
      "*", "*"),
               std::runtime_error);
  EXPECT_THROW(instance.check_user(
      "test_user", "test_host",
      std::vector<std::string>({"SELECT", "ALL", "INSERT"}),
      "*", "*"),
               std::runtime_error);
  EXPECT_THROW(instance.check_user(
      "test_user", "test_host",
      std::vector<std::string>({"SELECT", "ALL PRIVILEGES", "INSERT"}),
      "*", "*"),
               std::runtime_error);
  EXPECT_THROW(instance.check_user(
      "test_user", "test_host",
      std::vector<std::string>({"SELECT", "All", "INSERT"}),
      "*", "*"),
               std::runtime_error);

  EXPECT_CALL(session, close());
  _session->close();
}

TEST_F(Instance_test, check_user_specific_privileges) {
  EXPECT_CALL(session, connect(_connection_options));
  _session->connect(_connection_options);
  mysqlshdk::mysql::Instance instance(_session);

  // Verify privileges for user with SELECT,INSERT,UPDATE on test_db.*,
  // DELETE on test_db.t1, ALTER,DROP on test_db.t2, and SELECT on test_db2.*.
  // Test subset of privileges for *.*, test_db.*, test_db2.*, mysql.*,
  // test_db.t1, test_db.t2, test_db.t3, test_db2.t1 and mysql.user
  session.expect_query("SELECT PRIVILEGE_TYPE, IS_GRANTABLE "
                       "FROM INFORMATION_SCHEMA.USER_PRIVILEGES "
                       "WHERE GRANTEE = \"'test_user'@'test_host'\"")
      .then_return({{"", {"PRIVILEGE_TYPE", "IS_GRANTABLE"},
                     {Type::String, Type::String},
                     {{"USAGE", "NO"}}}});
  session.expect_query("SELECT TABLE_SCHEMA, PRIVILEGE_TYPE, IS_GRANTABLE "
                       "FROM INFORMATION_SCHEMA.SCHEMA_PRIVILEGES "
                       "WHERE GRANTEE = \"'test_user'@'test_host'\" "
                       "ORDER BY TABLE_SCHEMA")
      .then_return({{"", {"TABLE_SCHEMA", "PRIVILEGE_TYPE", "IS_GRANTABLE"},
                     {Type::String, Type::String, Type::String},
                     {{"test_db", "INSERT", "NO"},
                      {"test_db", "SELECT", "NO"},
                      {"test_db", "UPDATE", "NO"},
                      {"test_db2", "SELECT", "YES"}}}});
  session.expect_query(
          "SELECT TABLE_SCHEMA, TABLE_NAME, PRIVILEGE_TYPE, IS_GRANTABLE "
          "FROM INFORMATION_SCHEMA.TABLE_PRIVILEGES "
          "WHERE GRANTEE = \"'test_user'@'test_host'\" "
          "ORDER BY TABLE_SCHEMA, TABLE_NAME")
      .then_return(
          {{"",
            {"TABLE_SCHEMA", "TABLE_NAME", "PRIVILEGE_TYPE", "IS_GRANTABLE"},
            {Type::String, Type::String, Type::String,
             Type::String},
            {{"test_db", "t1", "DELETE", "NO"},
             {"test_db", "t2", "DROP", "YES"},
             {"test_db", "t2", "ALTER", "YES"}}}});
  std::vector<std::string> test_priv = {"SELECT", "Insert", "UPDATE"};
  std::tuple<bool, std::string, bool> res = instance.check_user(
      "test_user", "test_host", test_priv, "*", "*");
  EXPECT_TRUE(std::get<0>(res));
  EXPECT_STREQ("SELECT,INSERT,UPDATE", std::get<1>(res).c_str());
  EXPECT_FALSE(std::get<2>(res));

  session.expect_query("SELECT PRIVILEGE_TYPE, IS_GRANTABLE "
                       "FROM INFORMATION_SCHEMA.USER_PRIVILEGES "
                       "WHERE GRANTEE = \"'test_user'@'test_host'\"")
      .then_return({{"", {"PRIVILEGE_TYPE", "IS_GRANTABLE"},
                     {Type::String, Type::String},
                     {{"USAGE", "NO"}}}});
  session.expect_query("SELECT TABLE_SCHEMA, PRIVILEGE_TYPE, IS_GRANTABLE "
                       "FROM INFORMATION_SCHEMA.SCHEMA_PRIVILEGES "
                       "WHERE GRANTEE = \"'test_user'@'test_host'\" "
                       "ORDER BY TABLE_SCHEMA")
      .then_return({{"", {"TABLE_SCHEMA", "PRIVILEGE_TYPE", "IS_GRANTABLE"},
                     {Type::String, Type::String, Type::String},
                     {{"test_db", "INSERT", "NO"},
                      {"test_db", "SELECT", "NO"},
                      {"test_db", "UPDATE", "NO"},
                      {"test_db2", "SELECT", "YES"}}}});
  session.expect_query(
          "SELECT TABLE_SCHEMA, TABLE_NAME, PRIVILEGE_TYPE, IS_GRANTABLE "
          "FROM INFORMATION_SCHEMA.TABLE_PRIVILEGES "
          "WHERE GRANTEE = \"'test_user'@'test_host'\" "
          "ORDER BY TABLE_SCHEMA, TABLE_NAME")
      .then_return(
          {{"",
            {"TABLE_SCHEMA", "TABLE_NAME", "PRIVILEGE_TYPE", "IS_GRANTABLE"},
            {Type::String, Type::String, Type::String,
             Type::String},
            {{"test_db", "t1", "DELETE", "NO"},
             {"test_db", "t2", "DROP", "YES"},
             {"test_db", "t2", "ALTER", "YES"}}}});
  res = instance.check_user("test_user", "test_host", test_priv, "test_db",
                            "*");
  EXPECT_TRUE(std::get<0>(res));
  EXPECT_STREQ("", std::get<1>(res).c_str());
  EXPECT_FALSE(std::get<2>(res));
  session.expect_query("SELECT PRIVILEGE_TYPE, IS_GRANTABLE "
                           "FROM INFORMATION_SCHEMA.USER_PRIVILEGES "
                           "WHERE GRANTEE = \"'test_user'@'test_host'\"")
      .then_return({{"", {"PRIVILEGE_TYPE", "IS_GRANTABLE"},
                     {Type::String, Type::String},
                     {{"USAGE", "NO"}}}});
  session.expect_query("SELECT TABLE_SCHEMA, PRIVILEGE_TYPE, IS_GRANTABLE "
                           "FROM INFORMATION_SCHEMA.SCHEMA_PRIVILEGES "
                           "WHERE GRANTEE = \"'test_user'@'test_host'\" "
                           "ORDER BY TABLE_SCHEMA")
      .then_return({{"", {"TABLE_SCHEMA", "PRIVILEGE_TYPE", "IS_GRANTABLE"},
                     {Type::String, Type::String, Type::String},
                     {{"test_db", "INSERT", "NO"},
                      {"test_db", "SELECT", "NO"},
                      {"test_db", "UPDATE", "NO"},
                      {"test_db2", "SELECT", "YES"}}}});
  session.expect_query(
          "SELECT TABLE_SCHEMA, TABLE_NAME, PRIVILEGE_TYPE, IS_GRANTABLE "
              "FROM INFORMATION_SCHEMA.TABLE_PRIVILEGES "
              "WHERE GRANTEE = \"'test_user'@'test_host'\" "
              "ORDER BY TABLE_SCHEMA, TABLE_NAME")
      .then_return(
          {{"",
            {"TABLE_SCHEMA", "TABLE_NAME", "PRIVILEGE_TYPE", "IS_GRANTABLE"},
            {Type::String, Type::String, Type::String,
             Type::String},
            {{"test_db", "t1", "DELETE", "NO"},
             {"test_db", "t2", "DROP", "YES"},
             {"test_db", "t2", "ALTER", "YES"}}}});
  res = instance.check_user("test_user", "test_host", test_priv, "test_db2",
                            "*");
  EXPECT_TRUE(std::get<0>(res));
  EXPECT_STREQ("INSERT,UPDATE", std::get<1>(res).c_str());
  EXPECT_TRUE(std::get<2>(res));
  session.expect_query("SELECT PRIVILEGE_TYPE, IS_GRANTABLE "
                       "FROM INFORMATION_SCHEMA.USER_PRIVILEGES "
                       "WHERE GRANTEE = \"'test_user'@'test_host'\"")
      .then_return({{"", {"PRIVILEGE_TYPE", "IS_GRANTABLE"},
                     {Type::String, Type::String},
                     {{"USAGE", "NO"}}}});
  session.expect_query("SELECT TABLE_SCHEMA, PRIVILEGE_TYPE, IS_GRANTABLE "
                       "FROM INFORMATION_SCHEMA.SCHEMA_PRIVILEGES "
                       "WHERE GRANTEE = \"'test_user'@'test_host'\" "
                       "ORDER BY TABLE_SCHEMA")
      .then_return({{"", {"TABLE_SCHEMA", "PRIVILEGE_TYPE", "IS_GRANTABLE"},
                     {Type::String, Type::String, Type::String},
                     {{"test_db", "INSERT", "NO"},
                      {"test_db", "SELECT", "NO"},
                      {"test_db", "UPDATE", "NO"},
                      {"test_db2", "SELECT", "YES"}}}});
  session.expect_query(
          "SELECT TABLE_SCHEMA, TABLE_NAME, PRIVILEGE_TYPE, IS_GRANTABLE "
          "FROM INFORMATION_SCHEMA.TABLE_PRIVILEGES "
          "WHERE GRANTEE = \"'test_user'@'test_host'\" "
          "ORDER BY TABLE_SCHEMA, TABLE_NAME")
      .then_return(
          {{"",
            {"TABLE_SCHEMA", "TABLE_NAME", "PRIVILEGE_TYPE", "IS_GRANTABLE"},
            {Type::String, Type::String, Type::String,
             Type::String},
            {{"test_db", "t1", "DELETE", "NO"},
             {"test_db", "t2", "DROP", "YES"},
             {"test_db", "t2", "ALTER", "YES"}}}});
  res = instance.check_user("test_user", "test_host", test_priv, "mysql", "*");
  EXPECT_TRUE(std::get<0>(res));
  EXPECT_STREQ("SELECT,INSERT,UPDATE", std::get<1>(res).c_str());
  EXPECT_FALSE(std::get<2>(res));
  session.expect_query("SELECT PRIVILEGE_TYPE, IS_GRANTABLE "
                       "FROM INFORMATION_SCHEMA.USER_PRIVILEGES "
                       "WHERE GRANTEE = \"'test_user'@'test_host'\"")
      .then_return({{"", {"PRIVILEGE_TYPE", "IS_GRANTABLE"},
                     {Type::String, Type::String},
                     {{"USAGE", "NO"}}}});
  session.expect_query("SELECT TABLE_SCHEMA, PRIVILEGE_TYPE, IS_GRANTABLE "
                       "FROM INFORMATION_SCHEMA.SCHEMA_PRIVILEGES "
                       "WHERE GRANTEE = \"'test_user'@'test_host'\" "
                       "ORDER BY TABLE_SCHEMA")
      .then_return({{"", {"TABLE_SCHEMA", "PRIVILEGE_TYPE", "IS_GRANTABLE"},
                     {Type::String, Type::String, Type::String},
                     {{"test_db", "INSERT", "NO"},
                      {"test_db", "SELECT", "NO"},
                      {"test_db", "UPDATE", "NO"},
                      {"test_db2", "SELECT", "YES"}}}});
  session.expect_query(
          "SELECT TABLE_SCHEMA, TABLE_NAME, PRIVILEGE_TYPE, IS_GRANTABLE "
          "FROM INFORMATION_SCHEMA.TABLE_PRIVILEGES "
          "WHERE GRANTEE = \"'test_user'@'test_host'\" "
          "ORDER BY TABLE_SCHEMA, TABLE_NAME")
      .then_return(
          {{"",
            {"TABLE_SCHEMA", "TABLE_NAME", "PRIVILEGE_TYPE", "IS_GRANTABLE"},
            {Type::String, Type::String, Type::String,
             Type::String},
            {{"test_db", "t1", "DELETE", "NO"},
             {"test_db", "t2", "DROP", "YES"},
             {"test_db", "t2", "ALTER", "YES"}}}});
  res = instance.check_user("test_user", "test_host", test_priv, "test_db",
                            "t1");
  EXPECT_TRUE(std::get<0>(res));
  EXPECT_STREQ("", std::get<1>(res).c_str());
  EXPECT_FALSE(std::get<2>(res));
  session.expect_query("SELECT PRIVILEGE_TYPE, IS_GRANTABLE "
                       "FROM INFORMATION_SCHEMA.USER_PRIVILEGES "
                       "WHERE GRANTEE = \"'test_user'@'test_host'\"")
      .then_return({{"", {"PRIVILEGE_TYPE", "IS_GRANTABLE"},
                     {Type::String, Type::String},
                     {{"USAGE", "NO"}}}});
  session.expect_query("SELECT TABLE_SCHEMA, PRIVILEGE_TYPE, IS_GRANTABLE "
                       "FROM INFORMATION_SCHEMA.SCHEMA_PRIVILEGES "
                       "WHERE GRANTEE = \"'test_user'@'test_host'\" "
                       "ORDER BY TABLE_SCHEMA")
      .then_return({{"", {"TABLE_SCHEMA", "PRIVILEGE_TYPE", "IS_GRANTABLE"},
                     {Type::String, Type::String, Type::String},
                     {{"test_db", "INSERT", "NO"},
                      {"test_db", "SELECT", "NO"},
                      {"test_db", "UPDATE", "NO"},
                      {"test_db2", "SELECT", "YES"}}}});
  session.expect_query(
          "SELECT TABLE_SCHEMA, TABLE_NAME, PRIVILEGE_TYPE, IS_GRANTABLE "
          "FROM INFORMATION_SCHEMA.TABLE_PRIVILEGES "
          "WHERE GRANTEE = \"'test_user'@'test_host'\" "
          "ORDER BY TABLE_SCHEMA, TABLE_NAME")
      .then_return(
          {{"",
            {"TABLE_SCHEMA", "TABLE_NAME", "PRIVILEGE_TYPE", "IS_GRANTABLE"},
            {Type::String, Type::String, Type::String,
             Type::String},
            {{"test_db", "t1", "DELETE", "NO"},
             {"test_db", "t2", "DROP", "YES"},
             {"test_db", "t2", "ALTER", "YES"}}}});
  res = instance.check_user("test_user", "test_host", test_priv, "test_db",
                            "t2");
  EXPECT_TRUE(std::get<0>(res));
  EXPECT_STREQ("", std::get<1>(res).c_str());
  EXPECT_FALSE(std::get<2>(res));
  session.expect_query("SELECT PRIVILEGE_TYPE, IS_GRANTABLE "
                       "FROM INFORMATION_SCHEMA.USER_PRIVILEGES "
                       "WHERE GRANTEE = \"'test_user'@'test_host'\"")
      .then_return({{"", {"PRIVILEGE_TYPE", "IS_GRANTABLE"},
                     {Type::String, Type::String},
                     {{"USAGE", "NO"}}}});
  session.expect_query("SELECT TABLE_SCHEMA, PRIVILEGE_TYPE, IS_GRANTABLE "
                       "FROM INFORMATION_SCHEMA.SCHEMA_PRIVILEGES "
                       "WHERE GRANTEE = \"'test_user'@'test_host'\" "
                       "ORDER BY TABLE_SCHEMA")
      .then_return({{"", {"TABLE_SCHEMA", "PRIVILEGE_TYPE", "IS_GRANTABLE"},
                     {Type::String, Type::String, Type::String},
                     {{"test_db", "INSERT", "NO"},
                      {"test_db", "SELECT", "NO"},
                      {"test_db", "UPDATE", "NO"},
                      {"test_db2", "SELECT", "YES"}}}});
  session.expect_query(
          "SELECT TABLE_SCHEMA, TABLE_NAME, PRIVILEGE_TYPE, IS_GRANTABLE "
          "FROM INFORMATION_SCHEMA.TABLE_PRIVILEGES "
          "WHERE GRANTEE = \"'test_user'@'test_host'\" "
          "ORDER BY TABLE_SCHEMA, TABLE_NAME")
      .then_return(
          {{"",
            {"TABLE_SCHEMA", "TABLE_NAME", "PRIVILEGE_TYPE", "IS_GRANTABLE"},
            {Type::String, Type::String, Type::String,
             Type::String},
            {{"test_db", "t1", "DELETE", "NO"},
             {"test_db", "t2", "DROP", "YES"},
             {"test_db", "t2", "ALTER", "YES"}}}});
  res = instance.check_user("test_user", "test_host", test_priv, "test_db",
                            "t3");
  EXPECT_TRUE(std::get<0>(res));
  EXPECT_STREQ("", std::get<1>(res).c_str());
  EXPECT_FALSE(std::get<2>(res));
  session.expect_query("SELECT PRIVILEGE_TYPE, IS_GRANTABLE "
                       "FROM INFORMATION_SCHEMA.USER_PRIVILEGES "
                       "WHERE GRANTEE = \"'test_user'@'test_host'\"")
      .then_return({{"", {"PRIVILEGE_TYPE", "IS_GRANTABLE"},
                     {Type::String, Type::String},
                     {{"USAGE", "NO"}}}});
  session.expect_query("SELECT TABLE_SCHEMA, PRIVILEGE_TYPE, IS_GRANTABLE "
                       "FROM INFORMATION_SCHEMA.SCHEMA_PRIVILEGES "
                       "WHERE GRANTEE = \"'test_user'@'test_host'\" "
                       "ORDER BY TABLE_SCHEMA")
      .then_return({{"", {"TABLE_SCHEMA", "PRIVILEGE_TYPE", "IS_GRANTABLE"},
                     {Type::String, Type::String, Type::String},
                     {{"test_db", "INSERT", "NO"},
                      {"test_db", "SELECT", "NO"},
                      {"test_db", "UPDATE", "NO"},
                      {"test_db2", "SELECT", "YES"}}}});
  session.expect_query(
          "SELECT TABLE_SCHEMA, TABLE_NAME, PRIVILEGE_TYPE, IS_GRANTABLE "
          "FROM INFORMATION_SCHEMA.TABLE_PRIVILEGES "
          "WHERE GRANTEE = \"'test_user'@'test_host'\" "
          "ORDER BY TABLE_SCHEMA, TABLE_NAME")
      .then_return(
          {{"",
            {"TABLE_SCHEMA", "TABLE_NAME", "PRIVILEGE_TYPE", "IS_GRANTABLE"},
            {Type::String, Type::String, Type::String,
             Type::String},
            {{"test_db", "t1", "DELETE", "NO"},
             {"test_db", "t2", "DROP", "YES"},
             {"test_db", "t2", "ALTER", "YES"}}}});
  res = instance.check_user("test_user", "test_host", test_priv, "test_db2",
                            "t1");
  EXPECT_TRUE(std::get<0>(res));
  EXPECT_STREQ("INSERT,UPDATE", std::get<1>(res).c_str());
  EXPECT_TRUE(std::get<2>(res));
  session.expect_query("SELECT PRIVILEGE_TYPE, IS_GRANTABLE "
                       "FROM INFORMATION_SCHEMA.USER_PRIVILEGES "
                       "WHERE GRANTEE = \"'test_user'@'test_host'\"")
      .then_return({{"", {"PRIVILEGE_TYPE", "IS_GRANTABLE"},
                     {Type::String, Type::String},
                     {{"USAGE", "NO"}}}});
  session.expect_query("SELECT TABLE_SCHEMA, PRIVILEGE_TYPE, IS_GRANTABLE "
                       "FROM INFORMATION_SCHEMA.SCHEMA_PRIVILEGES "
                       "WHERE GRANTEE = \"'test_user'@'test_host'\" "
                       "ORDER BY TABLE_SCHEMA")
      .then_return({{"", {"TABLE_SCHEMA", "PRIVILEGE_TYPE", "IS_GRANTABLE"},
                     {Type::String, Type::String, Type::String},
                     {{"test_db", "INSERT", "NO"},
                      {"test_db", "SELECT", "NO"},
                      {"test_db", "UPDATE", "NO"},
                      {"test_db2", "SELECT", "YES"}}}});
  session.expect_query(
          "SELECT TABLE_SCHEMA, TABLE_NAME, PRIVILEGE_TYPE, IS_GRANTABLE "
          "FROM INFORMATION_SCHEMA.TABLE_PRIVILEGES "
          "WHERE GRANTEE = \"'test_user'@'test_host'\" "
          "ORDER BY TABLE_SCHEMA, TABLE_NAME")
      .then_return(
          {{"",
            {"TABLE_SCHEMA", "TABLE_NAME", "PRIVILEGE_TYPE", "IS_GRANTABLE"},
            {Type::String, Type::String, Type::String,
             Type::String},
            {{"test_db", "t1", "DELETE", "NO"},
             {"test_db", "t2", "DROP", "YES"},
             {"test_db", "t2", "ALTER", "YES"}}}});
  res = instance.check_user("test_user", "test_host", test_priv, "mysql",
                            "user");
  EXPECT_TRUE(std::get<0>(res));
  EXPECT_STREQ("SELECT,INSERT,UPDATE", std::get<1>(res).c_str());
  EXPECT_FALSE(std::get<2>(res));

  // Test all given privileges for *.*, test_db.*, test_db2.*, mysql.*,
  // test_db.t1, test_db.t2, test_db.t3, test_db2.t1, and mysql.user
  session.expect_query("SELECT PRIVILEGE_TYPE, IS_GRANTABLE "
                       "FROM INFORMATION_SCHEMA.USER_PRIVILEGES "
                       "WHERE GRANTEE = \"'test_user'@'test_host'\"")
      .then_return({{"", {"PRIVILEGE_TYPE", "IS_GRANTABLE"},
                     {Type::String, Type::String},
                     {{"USAGE", "NO"}}}});
  session.expect_query("SELECT TABLE_SCHEMA, PRIVILEGE_TYPE, IS_GRANTABLE "
                       "FROM INFORMATION_SCHEMA.SCHEMA_PRIVILEGES "
                       "WHERE GRANTEE = \"'test_user'@'test_host'\" "
                       "ORDER BY TABLE_SCHEMA")
      .then_return({{"", {"TABLE_SCHEMA", "PRIVILEGE_TYPE", "IS_GRANTABLE"},
                     {Type::String, Type::String, Type::String},
                     {{"test_db", "INSERT", "NO"},
                      {"test_db", "SELECT", "NO"},
                      {"test_db", "UPDATE", "NO"},
                      {"test_db2", "SELECT", "YES"}}}});
  session.expect_query(
          "SELECT TABLE_SCHEMA, TABLE_NAME, PRIVILEGE_TYPE, IS_GRANTABLE "
          "FROM INFORMATION_SCHEMA.TABLE_PRIVILEGES "
          "WHERE GRANTEE = \"'test_user'@'test_host'\" "
          "ORDER BY TABLE_SCHEMA, TABLE_NAME")
      .then_return(
          {{"",
            {"TABLE_SCHEMA", "TABLE_NAME", "PRIVILEGE_TYPE", "IS_GRANTABLE"},
            {Type::String, Type::String, Type::String,
             Type::String},
            {{"test_db", "t1", "DELETE", "NO"},
             {"test_db", "t2", "DROP", "YES"},
             {"test_db", "t2", "ALTER", "YES"}}}});
  test_priv = {"SELECT", "INSERT", "UPDATE", "Delete"};
  res = instance.check_user("test_user", "test_host", test_priv, "*", "*");
  EXPECT_TRUE(std::get<0>(res));
  EXPECT_STREQ("SELECT,INSERT,UPDATE,DELETE", std::get<1>(res).c_str());
  EXPECT_FALSE(std::get<2>(res));
  session.expect_query("SELECT PRIVILEGE_TYPE, IS_GRANTABLE "
                       "FROM INFORMATION_SCHEMA.USER_PRIVILEGES "
                       "WHERE GRANTEE = \"'test_user'@'test_host'\"")
      .then_return({{"", {"PRIVILEGE_TYPE", "IS_GRANTABLE"},
                     {Type::String, Type::String},
                     {{"USAGE", "NO"}}}});
  session.expect_query("SELECT TABLE_SCHEMA, PRIVILEGE_TYPE, IS_GRANTABLE "
                       "FROM INFORMATION_SCHEMA.SCHEMA_PRIVILEGES "
                       "WHERE GRANTEE = \"'test_user'@'test_host'\" "
                       "ORDER BY TABLE_SCHEMA")
      .then_return({{"", {"TABLE_SCHEMA", "PRIVILEGE_TYPE", "IS_GRANTABLE"},
                     {Type::String, Type::String, Type::String},
                     {{"test_db", "INSERT", "NO"},
                      {"test_db", "SELECT", "NO"},
                      {"test_db", "UPDATE", "NO"},
                      {"test_db2", "SELECT", "YES"}}}});
  session.expect_query(
          "SELECT TABLE_SCHEMA, TABLE_NAME, PRIVILEGE_TYPE, IS_GRANTABLE "
          "FROM INFORMATION_SCHEMA.TABLE_PRIVILEGES "
          "WHERE GRANTEE = \"'test_user'@'test_host'\" "
          "ORDER BY TABLE_SCHEMA, TABLE_NAME")
      .then_return(
          {{"",
            {"TABLE_SCHEMA", "TABLE_NAME", "PRIVILEGE_TYPE", "IS_GRANTABLE"},
            {Type::String, Type::String, Type::String,
             Type::String},
            {{"test_db", "t1", "DELETE", "NO"},
             {"test_db", "t2", "DROP", "YES"},
             {"test_db", "t2", "ALTER", "YES"}}}});
  res = instance.check_user("test_user", "test_host", test_priv, "test_db",
                            "*");
  EXPECT_TRUE(std::get<0>(res));
  EXPECT_STREQ("DELETE", std::get<1>(res).c_str());
  EXPECT_FALSE(std::get<2>(res));
  session.expect_query("SELECT PRIVILEGE_TYPE, IS_GRANTABLE "
                       "FROM INFORMATION_SCHEMA.USER_PRIVILEGES "
                       "WHERE GRANTEE = \"'test_user'@'test_host'\"")
      .then_return({{"", {"PRIVILEGE_TYPE", "IS_GRANTABLE"},
                     {Type::String, Type::String},
                     {{"USAGE", "NO"}}}});
  session.expect_query("SELECT TABLE_SCHEMA, PRIVILEGE_TYPE, IS_GRANTABLE "
                       "FROM INFORMATION_SCHEMA.SCHEMA_PRIVILEGES "
                       "WHERE GRANTEE = \"'test_user'@'test_host'\" "
                       "ORDER BY TABLE_SCHEMA")
      .then_return({{"", {"TABLE_SCHEMA", "PRIVILEGE_TYPE", "IS_GRANTABLE"},
                     {Type::String, Type::String, Type::String},
                     {{"test_db", "INSERT", "NO"},
                      {"test_db", "SELECT", "NO"},
                      {"test_db", "UPDATE", "NO"},
                      {"test_db2", "SELECT", "YES"}}}});
  session.expect_query(
          "SELECT TABLE_SCHEMA, TABLE_NAME, PRIVILEGE_TYPE, IS_GRANTABLE "
          "FROM INFORMATION_SCHEMA.TABLE_PRIVILEGES "
          "WHERE GRANTEE = \"'test_user'@'test_host'\" "
          "ORDER BY TABLE_SCHEMA, TABLE_NAME")
      .then_return(
          {{"",
            {"TABLE_SCHEMA", "TABLE_NAME", "PRIVILEGE_TYPE", "IS_GRANTABLE"},
            {Type::String, Type::String, Type::String,
             Type::String},
            {{"test_db", "t1", "DELETE", "NO"},
             {"test_db", "t2", "DROP", "YES"},
             {"test_db", "t2", "ALTER", "YES"}}}});
  res = instance.check_user("test_user", "test_host", test_priv, "test_db2",
                            "*");
  EXPECT_TRUE(std::get<0>(res));
  EXPECT_STREQ("INSERT,UPDATE,DELETE", std::get<1>(res).c_str());
  EXPECT_TRUE(std::get<2>(res));
  session.expect_query("SELECT PRIVILEGE_TYPE, IS_GRANTABLE "
                       "FROM INFORMATION_SCHEMA.USER_PRIVILEGES "
                       "WHERE GRANTEE = \"'test_user'@'test_host'\"")
      .then_return({{"", {"PRIVILEGE_TYPE", "IS_GRANTABLE"},
                     {Type::String, Type::String},
                     {{"USAGE", "NO"}}}});
  session.expect_query("SELECT TABLE_SCHEMA, PRIVILEGE_TYPE, IS_GRANTABLE "
                       "FROM INFORMATION_SCHEMA.SCHEMA_PRIVILEGES "
                       "WHERE GRANTEE = \"'test_user'@'test_host'\" "
                       "ORDER BY TABLE_SCHEMA")
      .then_return({{"", {"TABLE_SCHEMA", "PRIVILEGE_TYPE", "IS_GRANTABLE"},
                     {Type::String, Type::String, Type::String},
                     {{"test_db", "INSERT", "NO"},
                      {"test_db", "SELECT", "NO"},
                      {"test_db", "UPDATE", "NO"},
                      {"test_db2", "SELECT", "YES"}}}});
  session.expect_query(
          "SELECT TABLE_SCHEMA, TABLE_NAME, PRIVILEGE_TYPE, IS_GRANTABLE "
          "FROM INFORMATION_SCHEMA.TABLE_PRIVILEGES "
          "WHERE GRANTEE = \"'test_user'@'test_host'\" "
          "ORDER BY TABLE_SCHEMA, TABLE_NAME")
      .then_return(
          {{"",
            {"TABLE_SCHEMA", "TABLE_NAME", "PRIVILEGE_TYPE", "IS_GRANTABLE"},
            {Type::String, Type::String, Type::String,
             Type::String},
            {{"test_db", "t1", "DELETE", "NO"},
             {"test_db", "t2", "DROP", "YES"},
             {"test_db", "t2", "ALTER", "YES"}}}});
  res = instance.check_user("test_user", "test_host", test_priv, "mysql", "*");
  EXPECT_TRUE(std::get<0>(res));
  EXPECT_STREQ("SELECT,INSERT,UPDATE,DELETE", std::get<1>(res).c_str());
  EXPECT_FALSE(std::get<2>(res));
  session.expect_query("SELECT PRIVILEGE_TYPE, IS_GRANTABLE "
                       "FROM INFORMATION_SCHEMA.USER_PRIVILEGES "
                       "WHERE GRANTEE = \"'test_user'@'test_host'\"")
      .then_return({{"", {"PRIVILEGE_TYPE", "IS_GRANTABLE"},
                     {Type::String, Type::String},
                     {{"USAGE", "NO"}}}});
  session.expect_query("SELECT TABLE_SCHEMA, PRIVILEGE_TYPE, IS_GRANTABLE "
                       "FROM INFORMATION_SCHEMA.SCHEMA_PRIVILEGES "
                       "WHERE GRANTEE = \"'test_user'@'test_host'\" "
                       "ORDER BY TABLE_SCHEMA")
      .then_return({{"", {"TABLE_SCHEMA", "PRIVILEGE_TYPE", "IS_GRANTABLE"},
                     {Type::String, Type::String, Type::String},
                     {{"test_db", "INSERT", "NO"},
                      {"test_db", "SELECT", "NO"},
                      {"test_db", "UPDATE", "NO"},
                      {"test_db2", "SELECT", "YES"}}}});
  session.expect_query(
          "SELECT TABLE_SCHEMA, TABLE_NAME, PRIVILEGE_TYPE, IS_GRANTABLE "
          "FROM INFORMATION_SCHEMA.TABLE_PRIVILEGES "
          "WHERE GRANTEE = \"'test_user'@'test_host'\" "
          "ORDER BY TABLE_SCHEMA, TABLE_NAME")
      .then_return(
          {{"",
            {"TABLE_SCHEMA", "TABLE_NAME", "PRIVILEGE_TYPE", "IS_GRANTABLE"},
            {Type::String, Type::String, Type::String,
             Type::String},
            {{"test_db", "t1", "DELETE", "NO"},
             {"test_db", "t2", "DROP", "YES"},
             {"test_db", "t2", "ALTER", "YES"}}}});
  res = instance.check_user("test_user", "test_host", test_priv, "test_db",
                            "t1");
  EXPECT_TRUE(std::get<0>(res));
  EXPECT_STREQ("", std::get<1>(res).c_str());
  EXPECT_FALSE(std::get<2>(res));
  session.expect_query("SELECT PRIVILEGE_TYPE, IS_GRANTABLE "
                       "FROM INFORMATION_SCHEMA.USER_PRIVILEGES "
                       "WHERE GRANTEE = \"'test_user'@'test_host'\"")
      .then_return({{"", {"PRIVILEGE_TYPE", "IS_GRANTABLE"},
                     {Type::String, Type::String},
                     {{"USAGE", "NO"}}}});
  session.expect_query("SELECT TABLE_SCHEMA, PRIVILEGE_TYPE, IS_GRANTABLE "
                       "FROM INFORMATION_SCHEMA.SCHEMA_PRIVILEGES "
                       "WHERE GRANTEE = \"'test_user'@'test_host'\" "
                       "ORDER BY TABLE_SCHEMA")
      .then_return({{"", {"TABLE_SCHEMA", "PRIVILEGE_TYPE", "IS_GRANTABLE"},
                     {Type::String, Type::String, Type::String},
                     {{"test_db", "INSERT", "NO"},
                      {"test_db", "SELECT", "NO"},
                      {"test_db", "UPDATE", "NO"},
                      {"test_db2", "SELECT", "YES"}}}});
  session.expect_query(
          "SELECT TABLE_SCHEMA, TABLE_NAME, PRIVILEGE_TYPE, IS_GRANTABLE "
          "FROM INFORMATION_SCHEMA.TABLE_PRIVILEGES "
          "WHERE GRANTEE = \"'test_user'@'test_host'\" "
          "ORDER BY TABLE_SCHEMA, TABLE_NAME")
      .then_return(
          {{"",
            {"TABLE_SCHEMA", "TABLE_NAME", "PRIVILEGE_TYPE", "IS_GRANTABLE"},
            {Type::String, Type::String, Type::String,
             Type::String},
            {{"test_db", "t1", "DELETE", "NO"},
             {"test_db", "t2", "DROP", "YES"},
             {"test_db", "t2", "ALTER", "YES"}}}});
  res = instance.check_user("test_user", "test_host", test_priv, "test_db",
                            "t2");
  EXPECT_TRUE(std::get<0>(res));
  EXPECT_STREQ("DELETE", std::get<1>(res).c_str());
  EXPECT_FALSE(std::get<2>(res));
  session.expect_query("SELECT PRIVILEGE_TYPE, IS_GRANTABLE "
                       "FROM INFORMATION_SCHEMA.USER_PRIVILEGES "
                       "WHERE GRANTEE = \"'test_user'@'test_host'\"")
      .then_return({{"", {"PRIVILEGE_TYPE", "IS_GRANTABLE"},
                     {Type::String, Type::String},
                     {{"USAGE", "NO"}}}});
  session.expect_query("SELECT TABLE_SCHEMA, PRIVILEGE_TYPE, IS_GRANTABLE "
                       "FROM INFORMATION_SCHEMA.SCHEMA_PRIVILEGES "
                       "WHERE GRANTEE = \"'test_user'@'test_host'\" "
                       "ORDER BY TABLE_SCHEMA")
      .then_return({{"", {"TABLE_SCHEMA", "PRIVILEGE_TYPE", "IS_GRANTABLE"},
                     {Type::String, Type::String, Type::String},
                     {{"test_db", "INSERT", "NO"},
                      {"test_db", "SELECT", "NO"},
                      {"test_db", "UPDATE", "NO"},
                      {"test_db2", "SELECT", "YES"}}}});
  session.expect_query(
          "SELECT TABLE_SCHEMA, TABLE_NAME, PRIVILEGE_TYPE, IS_GRANTABLE "
          "FROM INFORMATION_SCHEMA.TABLE_PRIVILEGES "
          "WHERE GRANTEE = \"'test_user'@'test_host'\" "
          "ORDER BY TABLE_SCHEMA, TABLE_NAME")
      .then_return(
          {{"",
            {"TABLE_SCHEMA", "TABLE_NAME", "PRIVILEGE_TYPE", "IS_GRANTABLE"},
            {Type::String, Type::String, Type::String,
             Type::String},
            {{"test_db", "t1", "DELETE", "NO"},
             {"test_db", "t2", "DROP", "YES"},
             {"test_db", "t2", "ALTER", "YES"}}}});
  res = instance.check_user("test_user", "test_host", test_priv, "test_db",
                            "t3");
  EXPECT_TRUE(std::get<0>(res));
  EXPECT_STREQ("DELETE", std::get<1>(res).c_str());
  EXPECT_FALSE(std::get<2>(res));
  session.expect_query("SELECT PRIVILEGE_TYPE, IS_GRANTABLE "
                       "FROM INFORMATION_SCHEMA.USER_PRIVILEGES "
                       "WHERE GRANTEE = \"'test_user'@'test_host'\"")
      .then_return({{"", {"PRIVILEGE_TYPE", "IS_GRANTABLE"},
                     {Type::String, Type::String},
                     {{"USAGE", "NO"}}}});
  session.expect_query("SELECT TABLE_SCHEMA, PRIVILEGE_TYPE, IS_GRANTABLE "
                       "FROM INFORMATION_SCHEMA.SCHEMA_PRIVILEGES "
                       "WHERE GRANTEE = \"'test_user'@'test_host'\" "
                       "ORDER BY TABLE_SCHEMA")
      .then_return({{"", {"TABLE_SCHEMA", "PRIVILEGE_TYPE", "IS_GRANTABLE"},
                     {Type::String, Type::String, Type::String},
                     {{"test_db", "INSERT", "NO"},
                      {"test_db", "SELECT", "NO"},
                      {"test_db", "UPDATE", "NO"},
                      {"test_db2", "SELECT", "YES"}}}});
  session.expect_query(
          "SELECT TABLE_SCHEMA, TABLE_NAME, PRIVILEGE_TYPE, IS_GRANTABLE "
          "FROM INFORMATION_SCHEMA.TABLE_PRIVILEGES "
          "WHERE GRANTEE = \"'test_user'@'test_host'\" "
          "ORDER BY TABLE_SCHEMA, TABLE_NAME")
      .then_return(
          {{"",
            {"TABLE_SCHEMA", "TABLE_NAME", "PRIVILEGE_TYPE", "IS_GRANTABLE"},
            {Type::String, Type::String, Type::String,
             Type::String},
            {{"test_db", "t1", "DELETE", "NO"},
             {"test_db", "t2", "DROP", "YES"},
             {"test_db", "t2", "ALTER", "YES"}}}});
  res = instance.check_user("test_user", "test_host", test_priv, "test_db2",
                            "t1");
  EXPECT_TRUE(std::get<0>(res));
  EXPECT_STREQ("INSERT,UPDATE,DELETE", std::get<1>(res).c_str());
  EXPECT_TRUE(std::get<2>(res));
  session.expect_query("SELECT PRIVILEGE_TYPE, IS_GRANTABLE "
                       "FROM INFORMATION_SCHEMA.USER_PRIVILEGES "
                       "WHERE GRANTEE = \"'test_user'@'test_host'\"")
      .then_return({{"", {"PRIVILEGE_TYPE", "IS_GRANTABLE"},
                     {Type::String, Type::String},
                     {{"USAGE", "NO"}}}});
  session.expect_query("SELECT TABLE_SCHEMA, PRIVILEGE_TYPE, IS_GRANTABLE "
                       "FROM INFORMATION_SCHEMA.SCHEMA_PRIVILEGES "
                       "WHERE GRANTEE = \"'test_user'@'test_host'\" "
                       "ORDER BY TABLE_SCHEMA")
      .then_return({{"", {"TABLE_SCHEMA", "PRIVILEGE_TYPE", "IS_GRANTABLE"},
                     {Type::String, Type::String, Type::String},
                     {{"test_db", "INSERT", "NO"},
                      {"test_db", "SELECT", "NO"},
                      {"test_db", "UPDATE", "NO"},
                      {"test_db2", "SELECT", "YES"}}}});
  session.expect_query(
          "SELECT TABLE_SCHEMA, TABLE_NAME, PRIVILEGE_TYPE, IS_GRANTABLE "
          "FROM INFORMATION_SCHEMA.TABLE_PRIVILEGES "
          "WHERE GRANTEE = \"'test_user'@'test_host'\" "
          "ORDER BY TABLE_SCHEMA, TABLE_NAME")
      .then_return(
          {{"",
            {"TABLE_SCHEMA", "TABLE_NAME", "PRIVILEGE_TYPE", "IS_GRANTABLE"},
            {Type::String, Type::String, Type::String,
             Type::String},
            {{"test_db", "t1", "DELETE", "NO"},
             {"test_db", "t2", "DROP", "YES"},
             {"test_db", "t2", "ALTER", "YES"}}}});
  res = instance.check_user("test_user", "test_host", test_priv, "mysql",
                            "user");
  EXPECT_TRUE(std::get<0>(res));
  EXPECT_STREQ("SELECT,INSERT,UPDATE,DELETE", std::get<1>(res).c_str());
  EXPECT_FALSE(std::get<2>(res));

  // Test ALL privileges for *.*, test_db.*, test_db2.*, mysql.*, test_db.t1,
  // test_db.t2, test_db.t3, test_db2.t1 and mysql.user
  session.expect_query("SELECT PRIVILEGE_TYPE, IS_GRANTABLE "
                       "FROM INFORMATION_SCHEMA.USER_PRIVILEGES "
                       "WHERE GRANTEE = \"'test_user'@'test_host'\"")
      .then_return({{"", {"PRIVILEGE_TYPE", "IS_GRANTABLE"},
                     {Type::String, Type::String},
                     {{"USAGE", "NO"}}}});
  session.expect_query("SELECT TABLE_SCHEMA, PRIVILEGE_TYPE, IS_GRANTABLE "
                       "FROM INFORMATION_SCHEMA.SCHEMA_PRIVILEGES "
                       "WHERE GRANTEE = \"'test_user'@'test_host'\" "
                       "ORDER BY TABLE_SCHEMA")
      .then_return({{"", {"TABLE_SCHEMA", "PRIVILEGE_TYPE", "IS_GRANTABLE"},
                     {Type::String, Type::String, Type::String},
                     {{"test_db", "INSERT", "NO"},
                      {"test_db", "SELECT", "NO"},
                      {"test_db", "UPDATE", "NO"},
                      {"test_db2", "SELECT", "YES"}}}});
  session.expect_query(
          "SELECT TABLE_SCHEMA, TABLE_NAME, PRIVILEGE_TYPE, IS_GRANTABLE "
          "FROM INFORMATION_SCHEMA.TABLE_PRIVILEGES "
          "WHERE GRANTEE = \"'test_user'@'test_host'\" "
          "ORDER BY TABLE_SCHEMA, TABLE_NAME")
      .then_return(
          {{"",
            {"TABLE_SCHEMA", "TABLE_NAME", "PRIVILEGE_TYPE", "IS_GRANTABLE"},
            {Type::String, Type::String, Type::String,
             Type::String},
            {{"test_db", "t1", "DELETE", "NO"},
             {"test_db", "t2", "DROP", "YES"},
             {"test_db", "t2", "ALTER", "YES"}}}});
  test_priv = {"ALL"};
  res = instance.check_user("test_user", "test_host", test_priv, "*", "*");
  EXPECT_TRUE(std::get<0>(res));
  EXPECT_STREQ("ALL", std::get<1>(res).c_str());
  EXPECT_FALSE(std::get<2>(res));
  session.expect_query("SELECT PRIVILEGE_TYPE, IS_GRANTABLE "
                       "FROM INFORMATION_SCHEMA.USER_PRIVILEGES "
                       "WHERE GRANTEE = \"'test_user'@'test_host'\"")
      .then_return({{"", {"PRIVILEGE_TYPE", "IS_GRANTABLE"},
                     {Type::String, Type::String},
                     {{"USAGE", "NO"}}}});
  session.expect_query("SELECT TABLE_SCHEMA, PRIVILEGE_TYPE, IS_GRANTABLE "
                       "FROM INFORMATION_SCHEMA.SCHEMA_PRIVILEGES "
                       "WHERE GRANTEE = \"'test_user'@'test_host'\" "
                       "ORDER BY TABLE_SCHEMA")
      .then_return({{"", {"TABLE_SCHEMA", "PRIVILEGE_TYPE", "IS_GRANTABLE"},
                     {Type::String, Type::String, Type::String},
                     {{"test_db", "INSERT", "NO"},
                      {"test_db", "SELECT", "NO"},
                      {"test_db", "UPDATE", "NO"},
                      {"test_db2", "SELECT", "YES"}}}});
  session.expect_query(
          "SELECT TABLE_SCHEMA, TABLE_NAME, PRIVILEGE_TYPE, IS_GRANTABLE "
          "FROM INFORMATION_SCHEMA.TABLE_PRIVILEGES "
          "WHERE GRANTEE = \"'test_user'@'test_host'\" "
          "ORDER BY TABLE_SCHEMA, TABLE_NAME")
      .then_return(
          {{"",
            {"TABLE_SCHEMA", "TABLE_NAME", "PRIVILEGE_TYPE", "IS_GRANTABLE"},
            {Type::String, Type::String, Type::String,
             Type::String},
            {{"test_db", "t1", "DELETE", "NO"},
             {"test_db", "t2", "DROP", "YES"},
             {"test_db", "t2", "ALTER", "YES"}}}});
  res = instance.check_user("test_user", "test_host", test_priv, "test_db",
                            "*");
  EXPECT_TRUE(std::get<0>(res));
  EXPECT_STREQ("ALTER,ALTER ROUTINE,CREATE,CREATE ROUTINE,CREATE TABLESPACE,"
               "CREATE TEMPORARY TABLES,CREATE USER,CREATE VIEW,DELETE,DROP,"
               "EVENT,EXECUTE,FILE,INDEX,LOCK TABLES,PROCESS,REFERENCES,RELOAD,"
               "REPLICATION CLIENT,REPLICATION SLAVE,SHOW DATABASES,SHOW VIEW,"
               "SHUTDOWN,SUPER,TRIGGER",
               std::get<1>(res).c_str());
  EXPECT_FALSE(std::get<2>(res));
  session.expect_query("SELECT PRIVILEGE_TYPE, IS_GRANTABLE "
                       "FROM INFORMATION_SCHEMA.USER_PRIVILEGES "
                       "WHERE GRANTEE = \"'test_user'@'test_host'\"")
      .then_return({{"", {"PRIVILEGE_TYPE", "IS_GRANTABLE"},
                     {Type::String, Type::String},
                     {{"USAGE", "NO"}}}});
  session.expect_query("SELECT TABLE_SCHEMA, PRIVILEGE_TYPE, IS_GRANTABLE "
                       "FROM INFORMATION_SCHEMA.SCHEMA_PRIVILEGES "
                       "WHERE GRANTEE = \"'test_user'@'test_host'\" "
                       "ORDER BY TABLE_SCHEMA")
      .then_return({{"", {"TABLE_SCHEMA", "PRIVILEGE_TYPE", "IS_GRANTABLE"},
                     {Type::String, Type::String, Type::String},
                     {{"test_db", "INSERT", "NO"},
                      {"test_db", "SELECT", "NO"},
                      {"test_db", "UPDATE", "NO"},
                      {"test_db2", "SELECT", "YES"}}}});
  session.expect_query(
          "SELECT TABLE_SCHEMA, TABLE_NAME, PRIVILEGE_TYPE, IS_GRANTABLE "
          "FROM INFORMATION_SCHEMA.TABLE_PRIVILEGES "
          "WHERE GRANTEE = \"'test_user'@'test_host'\" "
          "ORDER BY TABLE_SCHEMA, TABLE_NAME")
      .then_return(
          {{"",
            {"TABLE_SCHEMA", "TABLE_NAME", "PRIVILEGE_TYPE", "IS_GRANTABLE"},
            {Type::String, Type::String, Type::String,
             Type::String},
            {{"test_db", "t1", "DELETE", "NO"},
             {"test_db", "t2", "DROP", "YES"},
             {"test_db", "t2", "ALTER", "YES"}}}});
  res = instance.check_user("test_user", "test_host", test_priv, "test_db2",
                            "*");
  EXPECT_TRUE(std::get<0>(res));
  EXPECT_STREQ("ALTER,ALTER ROUTINE,CREATE,CREATE ROUTINE,CREATE TABLESPACE,"
               "CREATE TEMPORARY TABLES,CREATE USER,CREATE VIEW,DELETE,DROP,"
               "EVENT,EXECUTE,FILE,INDEX,INSERT,LOCK TABLES,PROCESS,REFERENCES,"
               "RELOAD,REPLICATION CLIENT,REPLICATION SLAVE,SHOW DATABASES,"
               "SHOW VIEW,SHUTDOWN,SUPER,TRIGGER,UPDATE",
               std::get<1>(res).c_str());
  EXPECT_TRUE(std::get<2>(res));
  session.expect_query("SELECT PRIVILEGE_TYPE, IS_GRANTABLE "
                       "FROM INFORMATION_SCHEMA.USER_PRIVILEGES "
                       "WHERE GRANTEE = \"'test_user'@'test_host'\"")
      .then_return({{"", {"PRIVILEGE_TYPE", "IS_GRANTABLE"},
                     {Type::String, Type::String},
                     {{"USAGE", "NO"}}}});
  session.expect_query("SELECT TABLE_SCHEMA, PRIVILEGE_TYPE, IS_GRANTABLE "
                       "FROM INFORMATION_SCHEMA.SCHEMA_PRIVILEGES "
                       "WHERE GRANTEE = \"'test_user'@'test_host'\" "
                       "ORDER BY TABLE_SCHEMA")
      .then_return({{"", {"TABLE_SCHEMA", "PRIVILEGE_TYPE", "IS_GRANTABLE"},
                     {Type::String, Type::String, Type::String},
                     {{"test_db", "INSERT", "NO"},
                      {"test_db", "SELECT", "NO"},
                      {"test_db", "UPDATE", "NO"},
                      {"test_db2", "SELECT", "YES"}}}});
  session.expect_query(
          "SELECT TABLE_SCHEMA, TABLE_NAME, PRIVILEGE_TYPE, IS_GRANTABLE "
          "FROM INFORMATION_SCHEMA.TABLE_PRIVILEGES "
          "WHERE GRANTEE = \"'test_user'@'test_host'\" "
          "ORDER BY TABLE_SCHEMA, TABLE_NAME")
      .then_return(
          {{"",
            {"TABLE_SCHEMA", "TABLE_NAME", "PRIVILEGE_TYPE", "IS_GRANTABLE"},
            {Type::String, Type::String, Type::String,
             Type::String},
            {{"test_db", "t1", "DELETE", "NO"},
             {"test_db", "t2", "DROP", "YES"},
             {"test_db", "t2", "ALTER", "YES"}}}});
  res = instance.check_user("test_user", "test_host", test_priv, "mysql", "*");
  EXPECT_TRUE(std::get<0>(res));
  EXPECT_STREQ("ALL", std::get<1>(res).c_str());
  EXPECT_FALSE(std::get<2>(res));
  session.expect_query("SELECT PRIVILEGE_TYPE, IS_GRANTABLE "
                       "FROM INFORMATION_SCHEMA.USER_PRIVILEGES "
                       "WHERE GRANTEE = \"'test_user'@'test_host'\"")
      .then_return({{"", {"PRIVILEGE_TYPE", "IS_GRANTABLE"},
                     {Type::String, Type::String},
                     {{"USAGE", "NO"}}}});
  session.expect_query("SELECT TABLE_SCHEMA, PRIVILEGE_TYPE, IS_GRANTABLE "
                       "FROM INFORMATION_SCHEMA.SCHEMA_PRIVILEGES "
                       "WHERE GRANTEE = \"'test_user'@'test_host'\" "
                       "ORDER BY TABLE_SCHEMA")
      .then_return({{"", {"TABLE_SCHEMA", "PRIVILEGE_TYPE", "IS_GRANTABLE"},
                     {Type::String, Type::String, Type::String},
                     {{"test_db", "INSERT", "NO"},
                      {"test_db", "SELECT", "NO"},
                      {"test_db", "UPDATE", "NO"},
                      {"test_db2", "SELECT", "YES"}}}});
  session.expect_query(
          "SELECT TABLE_SCHEMA, TABLE_NAME, PRIVILEGE_TYPE, IS_GRANTABLE "
          "FROM INFORMATION_SCHEMA.TABLE_PRIVILEGES "
          "WHERE GRANTEE = \"'test_user'@'test_host'\" "
          "ORDER BY TABLE_SCHEMA, TABLE_NAME")
      .then_return(
          {{"",
            {"TABLE_SCHEMA", "TABLE_NAME", "PRIVILEGE_TYPE", "IS_GRANTABLE"},
            {Type::String, Type::String, Type::String,
             Type::String},
            {{"test_db", "t1", "DELETE", "NO"},
             {"test_db", "t2", "DROP", "YES"},
             {"test_db", "t2", "ALTER", "YES"}}}});
  res = instance.check_user("test_user", "test_host", test_priv, "test_db",
                            "t1");
  EXPECT_TRUE(std::get<0>(res));
  EXPECT_STREQ("ALTER,ALTER ROUTINE,CREATE,CREATE ROUTINE,CREATE TABLESPACE,"
               "CREATE TEMPORARY TABLES,CREATE USER,CREATE VIEW,DROP,"
               "EVENT,EXECUTE,FILE,INDEX,LOCK TABLES,PROCESS,REFERENCES,RELOAD,"
               "REPLICATION CLIENT,REPLICATION SLAVE,SHOW DATABASES,SHOW VIEW,"
               "SHUTDOWN,SUPER,TRIGGER",
               std::get<1>(res).c_str());
  EXPECT_FALSE(std::get<2>(res));
  session.expect_query("SELECT PRIVILEGE_TYPE, IS_GRANTABLE "
                       "FROM INFORMATION_SCHEMA.USER_PRIVILEGES "
                       "WHERE GRANTEE = \"'test_user'@'test_host'\"")
      .then_return({{"", {"PRIVILEGE_TYPE", "IS_GRANTABLE"},
                     {Type::String, Type::String},
                     {{"USAGE", "NO"}}}});
  session.expect_query("SELECT TABLE_SCHEMA, PRIVILEGE_TYPE, IS_GRANTABLE "
                       "FROM INFORMATION_SCHEMA.SCHEMA_PRIVILEGES "
                       "WHERE GRANTEE = \"'test_user'@'test_host'\" "
                       "ORDER BY TABLE_SCHEMA")
      .then_return({{"", {"TABLE_SCHEMA", "PRIVILEGE_TYPE", "IS_GRANTABLE"},
                     {Type::String, Type::String, Type::String},
                     {{"test_db", "INSERT", "NO"},
                      {"test_db", "SELECT", "NO"},
                      {"test_db", "UPDATE", "NO"},
                      {"test_db2", "SELECT", "YES"}}}});
  session.expect_query(
          "SELECT TABLE_SCHEMA, TABLE_NAME, PRIVILEGE_TYPE, IS_GRANTABLE "
          "FROM INFORMATION_SCHEMA.TABLE_PRIVILEGES "
          "WHERE GRANTEE = \"'test_user'@'test_host'\" "
          "ORDER BY TABLE_SCHEMA, TABLE_NAME")
      .then_return(
          {{"",
            {"TABLE_SCHEMA", "TABLE_NAME", "PRIVILEGE_TYPE", "IS_GRANTABLE"},
            {Type::String, Type::String, Type::String,
             Type::String},
            {{"test_db", "t1", "DELETE", "NO"},
             {"test_db", "t2", "DROP", "YES"},
             {"test_db", "t2", "ALTER", "YES"}}}});
  res = instance.check_user("test_user", "test_host", test_priv, "test_db",
                            "t2");
  EXPECT_TRUE(std::get<0>(res));
  EXPECT_STREQ("ALTER ROUTINE,CREATE,CREATE ROUTINE,CREATE TABLESPACE,"
               "CREATE TEMPORARY TABLES,CREATE USER,CREATE VIEW,DELETE,"
               "EVENT,EXECUTE,FILE,INDEX,LOCK TABLES,PROCESS,REFERENCES,RELOAD,"
               "REPLICATION CLIENT,REPLICATION SLAVE,SHOW DATABASES,SHOW VIEW,"
               "SHUTDOWN,SUPER,TRIGGER", std::get<1>(res).c_str());
  EXPECT_TRUE(std::get<2>(res));
  session.expect_query("SELECT PRIVILEGE_TYPE, IS_GRANTABLE "
                       "FROM INFORMATION_SCHEMA.USER_PRIVILEGES "
                       "WHERE GRANTEE = \"'test_user'@'test_host'\"")
      .then_return({{"", {"PRIVILEGE_TYPE", "IS_GRANTABLE"},
                     {Type::String, Type::String},
                     {{"USAGE", "NO"}}}});
  session.expect_query("SELECT TABLE_SCHEMA, PRIVILEGE_TYPE, IS_GRANTABLE "
                       "FROM INFORMATION_SCHEMA.SCHEMA_PRIVILEGES "
                       "WHERE GRANTEE = \"'test_user'@'test_host'\" "
                       "ORDER BY TABLE_SCHEMA")
      .then_return({{"", {"TABLE_SCHEMA", "PRIVILEGE_TYPE", "IS_GRANTABLE"},
                     {Type::String, Type::String, Type::String},
                     {{"test_db", "INSERT", "NO"},
                      {"test_db", "SELECT", "NO"},
                      {"test_db", "UPDATE", "NO"},
                      {"test_db2", "SELECT", "YES"}}}});
  session.expect_query(
          "SELECT TABLE_SCHEMA, TABLE_NAME, PRIVILEGE_TYPE, IS_GRANTABLE "
          "FROM INFORMATION_SCHEMA.TABLE_PRIVILEGES "
          "WHERE GRANTEE = \"'test_user'@'test_host'\" "
          "ORDER BY TABLE_SCHEMA, TABLE_NAME")
      .then_return(
          {{"",
            {"TABLE_SCHEMA", "TABLE_NAME", "PRIVILEGE_TYPE", "IS_GRANTABLE"},
            {Type::String, Type::String, Type::String,
             Type::String},
            {{"test_db", "t1", "DELETE", "NO"},
             {"test_db", "t2", "DROP", "YES"},
             {"test_db", "t2", "ALTER", "YES"}}}});
  res = instance.check_user("test_user", "test_host", test_priv, "test_db",
                            "t3");
  EXPECT_TRUE(std::get<0>(res));
  EXPECT_STREQ("ALTER,ALTER ROUTINE,CREATE,CREATE ROUTINE,CREATE TABLESPACE,"
               "CREATE TEMPORARY TABLES,CREATE USER,CREATE VIEW,DELETE,DROP,"
               "EVENT,EXECUTE,FILE,INDEX,LOCK TABLES,PROCESS,REFERENCES,RELOAD,"
               "REPLICATION CLIENT,REPLICATION SLAVE,SHOW DATABASES,SHOW VIEW,"
               "SHUTDOWN,SUPER,TRIGGER", std::get<1>(res).c_str());
  EXPECT_FALSE(std::get<2>(res));
  session.expect_query("SELECT PRIVILEGE_TYPE, IS_GRANTABLE "
                       "FROM INFORMATION_SCHEMA.USER_PRIVILEGES "
                       "WHERE GRANTEE = \"'test_user'@'test_host'\"")
      .then_return({{"", {"PRIVILEGE_TYPE", "IS_GRANTABLE"},
                     {Type::String, Type::String},
                     {{"USAGE", "NO"}}}});
  session.expect_query("SELECT TABLE_SCHEMA, PRIVILEGE_TYPE, IS_GRANTABLE "
                       "FROM INFORMATION_SCHEMA.SCHEMA_PRIVILEGES "
                       "WHERE GRANTEE = \"'test_user'@'test_host'\" "
                       "ORDER BY TABLE_SCHEMA")
      .then_return({{"", {"TABLE_SCHEMA", "PRIVILEGE_TYPE", "IS_GRANTABLE"},
                     {Type::String, Type::String, Type::String},
                     {{"test_db", "INSERT", "NO"},
                      {"test_db", "SELECT", "NO"},
                      {"test_db", "UPDATE", "NO"},
                      {"test_db2", "SELECT", "YES"}}}});
  session.expect_query(
          "SELECT TABLE_SCHEMA, TABLE_NAME, PRIVILEGE_TYPE, IS_GRANTABLE "
          "FROM INFORMATION_SCHEMA.TABLE_PRIVILEGES "
          "WHERE GRANTEE = \"'test_user'@'test_host'\" "
          "ORDER BY TABLE_SCHEMA, TABLE_NAME")
      .then_return(
          {{"",
            {"TABLE_SCHEMA", "TABLE_NAME", "PRIVILEGE_TYPE", "IS_GRANTABLE"},
            {Type::String, Type::String, Type::String,
             Type::String},
            {{"test_db", "t1", "DELETE", "NO"},
             {"test_db", "t2", "DROP", "YES"},
             {"test_db", "t2", "ALTER", "YES"}}}});
  res = instance.check_user("test_user", "test_host", test_priv, "test_db2",
                            "t1");
  EXPECT_TRUE(std::get<0>(res));
  EXPECT_STREQ("ALTER,ALTER ROUTINE,CREATE,CREATE ROUTINE,CREATE TABLESPACE,"
               "CREATE TEMPORARY TABLES,CREATE USER,CREATE VIEW,DELETE,DROP,"
               "EVENT,EXECUTE,FILE,INDEX,INSERT,LOCK TABLES,PROCESS,REFERENCES,"
               "RELOAD,REPLICATION CLIENT,REPLICATION SLAVE,SHOW DATABASES,"
               "SHOW VIEW,SHUTDOWN,SUPER,TRIGGER,UPDATE",
               std::get<1>(res).c_str());
  EXPECT_TRUE(std::get<2>(res));
  session.expect_query("SELECT PRIVILEGE_TYPE, IS_GRANTABLE "
                       "FROM INFORMATION_SCHEMA.USER_PRIVILEGES "
                       "WHERE GRANTEE = \"'test_user'@'test_host'\"")
      .then_return({{"", {"PRIVILEGE_TYPE", "IS_GRANTABLE"},
                     {Type::String, Type::String},
                     {{"USAGE", "NO"}}}});
  session.expect_query("SELECT TABLE_SCHEMA, PRIVILEGE_TYPE, IS_GRANTABLE "
                       "FROM INFORMATION_SCHEMA.SCHEMA_PRIVILEGES "
                       "WHERE GRANTEE = \"'test_user'@'test_host'\" "
                       "ORDER BY TABLE_SCHEMA")
      .then_return({{"", {"TABLE_SCHEMA", "PRIVILEGE_TYPE", "IS_GRANTABLE"},
                     {Type::String, Type::String, Type::String},
                     {{"test_db", "INSERT", "NO"},
                      {"test_db", "SELECT", "NO"},
                      {"test_db", "UPDATE", "NO"},
                      {"test_db2", "SELECT", "YES"}}}});
  session.expect_query(
          "SELECT TABLE_SCHEMA, TABLE_NAME, PRIVILEGE_TYPE, IS_GRANTABLE "
          "FROM INFORMATION_SCHEMA.TABLE_PRIVILEGES "
          "WHERE GRANTEE = \"'test_user'@'test_host'\" "
          "ORDER BY TABLE_SCHEMA, TABLE_NAME")
      .then_return(
          {{"",
            {"TABLE_SCHEMA", "TABLE_NAME", "PRIVILEGE_TYPE", "IS_GRANTABLE"},
            {Type::String, Type::String, Type::String,
             Type::String},
            {{"test_db", "t1", "DELETE", "NO"},
             {"test_db", "t2", "DROP", "YES"},
             {"test_db", "t2", "ALTER", "YES"}}}});
  res = instance.check_user("test_user", "test_host", test_priv, "mysql",
                            "user");
  EXPECT_TRUE(std::get<0>(res));
  EXPECT_STREQ("ALL", std::get<1>(res).c_str());
  EXPECT_FALSE(std::get<2>(res));

  EXPECT_CALL(session, close());
  _session->close();
}  // NOLINT(readability/fn_size)

TEST_F(Instance_test, check_user_all_privileges) {
  EXPECT_CALL(session, connect(_connection_options));
  _session->connect(_connection_options);
  mysqlshdk::mysql::Instance instance(_session);

  // Verify privileges for dba user with ALL on *.* WITH GRANT OPTION.
  // Test ALL privileges for *.*, test_db.*, test_db2.*, mysql.*, test_db.t1,
  // test_db.t2, test_db.t3, test_db2.t1, and mysql.user
  session.expect_query("SELECT PRIVILEGE_TYPE, IS_GRANTABLE "
                       "FROM INFORMATION_SCHEMA.USER_PRIVILEGES "
                       "WHERE GRANTEE = \"'dba_user'@'dba_host'\"")
      .then_return({{"", {"PRIVILEGE_TYPE", "IS_GRANTABLE"},
                     {Type::String, Type::String},
                     {{"SELECT", "YES"}, {"INSERT", "YES"},
                      {"UPDATE", "YES"}, {"DELETE", "YES"},
                      {"CREATE", "YES"}, {"DROP", "YES"},
                      {"RELOAD", "YES"}, {"SHUTDOWN", "YES"},
                      {"PROCESS", "YES"}, {"FILE", "YES"},
                      {"REFERENCES", "YES"}, {"INDEX", "YES"},
                      {"ALTER", "YES"}, {"SHOW DATABASES", "YES"},
                      {"SUPER", "YES"}, {"CREATE TEMPORARY TABLES", "YES"},
                      {"LOCK TABLES", "YES"}, {"EXECUTE", "YES"},
                      {"REPLICATION SLAVE", "YES"},
                      {"REPLICATION CLIENT", "YES"}, {"CREATE VIEW", "YES"},
                      {"SHOW VIEW", "YES"}, {"CREATE ROUTINE", "YES"},
                      {"ALTER ROUTINE", "YES"}, {"CREATE USER", "YES"},
                      {"EVENT", "YES"}, {"TRIGGER", "YES"},
                      {"CREATE TABLESPACE", "YES"}}}});
  session.expect_query("SELECT TABLE_SCHEMA, PRIVILEGE_TYPE, IS_GRANTABLE "
                       "FROM INFORMATION_SCHEMA.SCHEMA_PRIVILEGES "
                       "WHERE GRANTEE = \"'dba_user'@'dba_host'\" "
                       "ORDER BY TABLE_SCHEMA")
      .then_return({{"",
          {"TABLE_SCHEMA", "PRIVILEGE_TYPE", "IS_GRANTABLE"},
          {Type::String, Type::String, Type::String},
          {}  // No Records.
      }});
  session.expect_query(
          "SELECT TABLE_SCHEMA, TABLE_NAME, PRIVILEGE_TYPE, IS_GRANTABLE "
          "FROM INFORMATION_SCHEMA.TABLE_PRIVILEGES "
          "WHERE GRANTEE = \"'dba_user'@'dba_host'\" "
          "ORDER BY TABLE_SCHEMA, TABLE_NAME")
      .then_return({{"",
          {"TABLE_SCHEMA", "TABLE_NAME", "PRIVILEGE_TYPE", "IS_GRANTABLE"},
          {Type::String, Type::String, Type::String, Type::String},
          {}  // No Records.
      }});
  std::vector<std::string> test_priv = {"All"};
  std::tuple<bool, std::string, bool> res = instance.check_user(
      "dba_user", "dba_host", test_priv, "*", "*");
  EXPECT_TRUE(std::get<0>(res));
  EXPECT_STREQ("", std::get<1>(res).c_str());
  EXPECT_TRUE(std::get<2>(res));
  session.expect_query("SELECT PRIVILEGE_TYPE, IS_GRANTABLE "
                       "FROM INFORMATION_SCHEMA.USER_PRIVILEGES "
                       "WHERE GRANTEE = \"'dba_user'@'dba_host'\"")
      .then_return({{"", {"PRIVILEGE_TYPE", "IS_GRANTABLE"},
                     {Type::String, Type::String},
                     {{"SELECT", "YES"}, {"INSERT", "YES"},
                      {"UPDATE", "YES"}, {"DELETE", "YES"},
                      {"CREATE", "YES"}, {"DROP", "YES"},
                      {"RELOAD", "YES"}, {"SHUTDOWN", "YES"},
                      {"PROCESS", "YES"}, {"FILE", "YES"},
                      {"REFERENCES", "YES"}, {"INDEX", "YES"},
                      {"ALTER", "YES"}, {"SHOW DATABASES", "YES"},
                      {"SUPER", "YES"}, {"CREATE TEMPORARY TABLES", "YES"},
                      {"LOCK TABLES", "YES"}, {"EXECUTE", "YES"},
                      {"REPLICATION SLAVE", "YES"},
                      {"REPLICATION CLIENT", "YES"}, {"CREATE VIEW", "YES"},
                      {"SHOW VIEW", "YES"}, {"CREATE ROUTINE", "YES"},
                      {"ALTER ROUTINE", "YES"}, {"CREATE USER", "YES"},
                      {"EVENT", "YES"}, {"TRIGGER", "YES"},
                      {"CREATE TABLESPACE", "YES"}}}});
  session.expect_query("SELECT TABLE_SCHEMA, PRIVILEGE_TYPE, IS_GRANTABLE "
                    "FROM INFORMATION_SCHEMA.SCHEMA_PRIVILEGES "
                    "WHERE GRANTEE = \"'dba_user'@'dba_host'\" "
                    "ORDER BY TABLE_SCHEMA")
      .then_return({{"",
          {"TABLE_SCHEMA", "PRIVILEGE_TYPE", "IS_GRANTABLE"},
          {Type::String, Type::String, Type::String},
          {}  // No Records.
      }});
  session.expect_query(
          "SELECT TABLE_SCHEMA, TABLE_NAME, PRIVILEGE_TYPE, IS_GRANTABLE "
          "FROM INFORMATION_SCHEMA.TABLE_PRIVILEGES "
          "WHERE GRANTEE = \"'dba_user'@'dba_host'\" "
          "ORDER BY TABLE_SCHEMA, TABLE_NAME")
      .then_return({{"",
          {"TABLE_SCHEMA", "TABLE_NAME", "PRIVILEGE_TYPE", "IS_GRANTABLE"},
          {Type::String, Type::String, Type::String, Type::String},
          {}  // No Records.
      }});
  res = instance.check_user("dba_user", "dba_host", test_priv, "*", "*");
  EXPECT_TRUE(std::get<0>(res));
  EXPECT_STREQ("", std::get<1>(res).c_str());
  EXPECT_TRUE(std::get<2>(res));
  session.expect_query("SELECT PRIVILEGE_TYPE, IS_GRANTABLE "
                       "FROM INFORMATION_SCHEMA.USER_PRIVILEGES "
                       "WHERE GRANTEE = \"'dba_user'@'dba_host'\"")
      .then_return({{"", {"PRIVILEGE_TYPE", "IS_GRANTABLE"},
                     {Type::String, Type::String},
                     {{"SELECT", "YES"}, {"INSERT", "YES"},
                      {"UPDATE", "YES"}, {"DELETE", "YES"},
                      {"CREATE", "YES"}, {"DROP", "YES"},
                      {"RELOAD", "YES"}, {"SHUTDOWN", "YES"},
                      {"PROCESS", "YES"}, {"FILE", "YES"},
                      {"REFERENCES", "YES"}, {"INDEX", "YES"},
                      {"ALTER", "YES"}, {"SHOW DATABASES", "YES"},
                      {"SUPER", "YES"}, {"CREATE TEMPORARY TABLES", "YES"},
                      {"LOCK TABLES", "YES"}, {"EXECUTE", "YES"},
                      {"REPLICATION SLAVE", "YES"},
                      {"REPLICATION CLIENT", "YES"}, {"CREATE VIEW", "YES"},
                      {"SHOW VIEW", "YES"}, {"CREATE ROUTINE", "YES"},
                      {"ALTER ROUTINE", "YES"}, {"CREATE USER", "YES"},
                      {"EVENT", "YES"}, {"TRIGGER", "YES"},
                      {"CREATE TABLESPACE", "YES"}}}});
  session.expect_query("SELECT TABLE_SCHEMA, PRIVILEGE_TYPE, IS_GRANTABLE "
                       "FROM INFORMATION_SCHEMA.SCHEMA_PRIVILEGES "
                       "WHERE GRANTEE = \"'dba_user'@'dba_host'\" "
                       "ORDER BY TABLE_SCHEMA")
      .then_return({{"",
          {"TABLE_SCHEMA", "PRIVILEGE_TYPE", "IS_GRANTABLE"},
          {Type::String, Type::String, Type::String},
          {}  // No Records.
      }});
  session.expect_query(
          "SELECT TABLE_SCHEMA, TABLE_NAME, PRIVILEGE_TYPE, IS_GRANTABLE "
          "FROM INFORMATION_SCHEMA.TABLE_PRIVILEGES "
          "WHERE GRANTEE = \"'dba_user'@'dba_host'\" "
          "ORDER BY TABLE_SCHEMA, TABLE_NAME")
      .then_return({{"",
          {"TABLE_SCHEMA", "TABLE_NAME", "PRIVILEGE_TYPE", "IS_GRANTABLE"},
          {Type::String, Type::String, Type::String, Type::String},
          {}  // No Records.
      }});
  res = instance.check_user("dba_user", "dba_host", test_priv, "test_db", "*");
  EXPECT_TRUE(std::get<0>(res));
  EXPECT_STREQ("", std::get<1>(res).c_str());
  EXPECT_TRUE(std::get<2>(res));
  session.expect_query("SELECT PRIVILEGE_TYPE, IS_GRANTABLE "
                       "FROM INFORMATION_SCHEMA.USER_PRIVILEGES "
                       "WHERE GRANTEE = \"'dba_user'@'dba_host'\"")
      .then_return({{"", {"PRIVILEGE_TYPE", "IS_GRANTABLE"},
                     {Type::String, Type::String},
                     {{"SELECT", "YES"}, {"INSERT", "YES"},
                      {"UPDATE", "YES"}, {"DELETE", "YES"},
                      {"CREATE", "YES"}, {"DROP", "YES"},
                      {"RELOAD", "YES"}, {"SHUTDOWN", "YES"},
                      {"PROCESS", "YES"}, {"FILE", "YES"},
                      {"REFERENCES", "YES"}, {"INDEX", "YES"},
                      {"ALTER", "YES"}, {"SHOW DATABASES", "YES"},
                      {"SUPER", "YES"}, {"CREATE TEMPORARY TABLES", "YES"},
                      {"LOCK TABLES", "YES"}, {"EXECUTE", "YES"},
                      {"REPLICATION SLAVE", "YES"},
                      {"REPLICATION CLIENT", "YES"}, {"CREATE VIEW", "YES"},
                      {"SHOW VIEW", "YES"}, {"CREATE ROUTINE", "YES"},
                      {"ALTER ROUTINE", "YES"}, {"CREATE USER", "YES"},
                      {"EVENT", "YES"}, {"TRIGGER", "YES"},
                      {"CREATE TABLESPACE", "YES"}}}});
  session.expect_query("SELECT TABLE_SCHEMA, PRIVILEGE_TYPE, IS_GRANTABLE "
                       "FROM INFORMATION_SCHEMA.SCHEMA_PRIVILEGES "
                       "WHERE GRANTEE = \"'dba_user'@'dba_host'\" "
                       "ORDER BY TABLE_SCHEMA")
      .then_return({{"",
          {"TABLE_SCHEMA", "PRIVILEGE_TYPE", "IS_GRANTABLE"},
          {Type::String, Type::String, Type::String},
          {}  // No Records.
      }});
  session.expect_query(
          "SELECT TABLE_SCHEMA, TABLE_NAME, PRIVILEGE_TYPE, IS_GRANTABLE "
          "FROM INFORMATION_SCHEMA.TABLE_PRIVILEGES "
          "WHERE GRANTEE = \"'dba_user'@'dba_host'\" "
          "ORDER BY TABLE_SCHEMA, TABLE_NAME")
      .then_return({{"",
          {"TABLE_SCHEMA", "TABLE_NAME", "PRIVILEGE_TYPE", "IS_GRANTABLE"},
          {Type::String, Type::String, Type::String, Type::String},
          {}  // No Records.
      }});
  res = instance.check_user("dba_user", "dba_host", test_priv, "test_db2", "*");
  EXPECT_TRUE(std::get<0>(res));
  EXPECT_STREQ("", std::get<1>(res).c_str());
  EXPECT_TRUE(std::get<2>(res));
  session.expect_query("SELECT PRIVILEGE_TYPE, IS_GRANTABLE "
                       "FROM INFORMATION_SCHEMA.USER_PRIVILEGES "
                       "WHERE GRANTEE = \"'dba_user'@'dba_host'\"")
      .then_return({{"", {"PRIVILEGE_TYPE", "IS_GRANTABLE"},
                     {Type::String, Type::String},
                     {{"SELECT", "YES"}, {"INSERT", "YES"},
                      {"UPDATE", "YES"}, {"DELETE", "YES"},
                      {"CREATE", "YES"}, {"DROP", "YES"},
                      {"RELOAD", "YES"}, {"SHUTDOWN", "YES"},
                      {"PROCESS", "YES"}, {"FILE", "YES"},
                      {"REFERENCES", "YES"}, {"INDEX", "YES"},
                      {"ALTER", "YES"}, {"SHOW DATABASES", "YES"},
                      {"SUPER", "YES"}, {"CREATE TEMPORARY TABLES", "YES"},
                      {"LOCK TABLES", "YES"}, {"EXECUTE", "YES"},
                      {"REPLICATION SLAVE", "YES"},
                      {"REPLICATION CLIENT", "YES"}, {"CREATE VIEW", "YES"},
                      {"SHOW VIEW", "YES"}, {"CREATE ROUTINE", "YES"},
                      {"ALTER ROUTINE", "YES"}, {"CREATE USER", "YES"},
                      {"EVENT", "YES"}, {"TRIGGER", "YES"},
                      {"CREATE TABLESPACE", "YES"}}}});
  session.expect_query("SELECT TABLE_SCHEMA, PRIVILEGE_TYPE, IS_GRANTABLE "
                       "FROM INFORMATION_SCHEMA.SCHEMA_PRIVILEGES "
                       "WHERE GRANTEE = \"'dba_user'@'dba_host'\" "
                       "ORDER BY TABLE_SCHEMA")
      .then_return({{"",
          {"TABLE_SCHEMA", "PRIVILEGE_TYPE", "IS_GRANTABLE"},
          {Type::String, Type::String, Type::String},
          {}  // No Records.
      }});
  session.expect_query(
          "SELECT TABLE_SCHEMA, TABLE_NAME, PRIVILEGE_TYPE, IS_GRANTABLE "
          "FROM INFORMATION_SCHEMA.TABLE_PRIVILEGES "
          "WHERE GRANTEE = \"'dba_user'@'dba_host'\" "
          "ORDER BY TABLE_SCHEMA, TABLE_NAME")
      .then_return({{"",
          {"TABLE_SCHEMA", "TABLE_NAME", "PRIVILEGE_TYPE", "IS_GRANTABLE"},
          {Type::String, Type::String, Type::String, Type::String},
          {}  // No Records.
      }});
  res = instance.check_user("dba_user", "dba_host", test_priv, "mysql", "*");
  EXPECT_TRUE(std::get<0>(res));
  EXPECT_STREQ("", std::get<1>(res).c_str());
  EXPECT_TRUE(std::get<2>(res));
  session.expect_query("SELECT PRIVILEGE_TYPE, IS_GRANTABLE "
                       "FROM INFORMATION_SCHEMA.USER_PRIVILEGES "
                       "WHERE GRANTEE = \"'dba_user'@'dba_host'\"")
      .then_return({{"", {"PRIVILEGE_TYPE", "IS_GRANTABLE"},
                     {Type::String, Type::String},
                     {{"SELECT", "YES"}, {"INSERT", "YES"},
                      {"UPDATE", "YES"}, {"DELETE", "YES"},
                      {"CREATE", "YES"}, {"DROP", "YES"},
                      {"RELOAD", "YES"}, {"SHUTDOWN", "YES"},
                      {"PROCESS", "YES"}, {"FILE", "YES"},
                      {"REFERENCES", "YES"}, {"INDEX", "YES"},
                      {"ALTER", "YES"}, {"SHOW DATABASES", "YES"},
                      {"SUPER", "YES"}, {"CREATE TEMPORARY TABLES", "YES"},
                      {"LOCK TABLES", "YES"}, {"EXECUTE", "YES"},
                      {"REPLICATION SLAVE", "YES"},
                      {"REPLICATION CLIENT", "YES"}, {"CREATE VIEW", "YES"},
                      {"SHOW VIEW", "YES"}, {"CREATE ROUTINE", "YES"},
                      {"ALTER ROUTINE", "YES"}, {"CREATE USER", "YES"},
                      {"EVENT", "YES"}, {"TRIGGER", "YES"},
                      {"CREATE TABLESPACE", "YES"}}}});
  session.expect_query("SELECT TABLE_SCHEMA, PRIVILEGE_TYPE, IS_GRANTABLE "
                       "FROM INFORMATION_SCHEMA.SCHEMA_PRIVILEGES "
                       "WHERE GRANTEE = \"'dba_user'@'dba_host'\" "
                       "ORDER BY TABLE_SCHEMA")
      .then_return({{"",
          {"TABLE_SCHEMA", "PRIVILEGE_TYPE", "IS_GRANTABLE"},
          {Type::String, Type::String, Type::String},
          {}  // No Records.
      }});
  session.expect_query(
          "SELECT TABLE_SCHEMA, TABLE_NAME, PRIVILEGE_TYPE, IS_GRANTABLE "
          "FROM INFORMATION_SCHEMA.TABLE_PRIVILEGES "
          "WHERE GRANTEE = \"'dba_user'@'dba_host'\" "
          "ORDER BY TABLE_SCHEMA, TABLE_NAME")
      .then_return({{"",
          {"TABLE_SCHEMA", "TABLE_NAME", "PRIVILEGE_TYPE", "IS_GRANTABLE"},
          {Type::String, Type::String, Type::String, Type::String},
          {}  // No Records.
      }});
  res = instance.check_user("dba_user", "dba_host", test_priv, "test_db", "t1");
  EXPECT_TRUE(std::get<0>(res));
  EXPECT_STREQ("", std::get<1>(res).c_str());
  EXPECT_TRUE(std::get<2>(res));
  session.expect_query("SELECT PRIVILEGE_TYPE, IS_GRANTABLE "
                       "FROM INFORMATION_SCHEMA.USER_PRIVILEGES "
                       "WHERE GRANTEE = \"'dba_user'@'dba_host'\"")
      .then_return({{"", {"PRIVILEGE_TYPE", "IS_GRANTABLE"},
                     {Type::String, Type::String},
                     {{"SELECT", "YES"}, {"INSERT", "YES"},
                      {"UPDATE", "YES"}, {"DELETE", "YES"},
                      {"CREATE", "YES"}, {"DROP", "YES"},
                      {"RELOAD", "YES"}, {"SHUTDOWN", "YES"},
                      {"PROCESS", "YES"}, {"FILE", "YES"},
                      {"REFERENCES", "YES"}, {"INDEX", "YES"},
                      {"ALTER", "YES"}, {"SHOW DATABASES", "YES"},
                      {"SUPER", "YES"}, {"CREATE TEMPORARY TABLES", "YES"},
                      {"LOCK TABLES", "YES"}, {"EXECUTE", "YES"},
                      {"REPLICATION SLAVE", "YES"},
                      {"REPLICATION CLIENT", "YES"}, {"CREATE VIEW", "YES"},
                      {"SHOW VIEW", "YES"}, {"CREATE ROUTINE", "YES"},
                      {"ALTER ROUTINE", "YES"}, {"CREATE USER", "YES"},
                      {"EVENT", "YES"}, {"TRIGGER", "YES"},
                      {"CREATE TABLESPACE", "YES"}}}});
  session.expect_query("SELECT TABLE_SCHEMA, PRIVILEGE_TYPE, IS_GRANTABLE "
                       "FROM INFORMATION_SCHEMA.SCHEMA_PRIVILEGES "
                       "WHERE GRANTEE = \"'dba_user'@'dba_host'\" "
                       "ORDER BY TABLE_SCHEMA")
      .then_return({{"",
          {"TABLE_SCHEMA", "PRIVILEGE_TYPE", "IS_GRANTABLE"},
          {Type::String, Type::String, Type::String},
          {}  // No Records.
      }});
  session.expect_query(
          "SELECT TABLE_SCHEMA, TABLE_NAME, PRIVILEGE_TYPE, IS_GRANTABLE "
          "FROM INFORMATION_SCHEMA.TABLE_PRIVILEGES "
          "WHERE GRANTEE = \"'dba_user'@'dba_host'\" "
          "ORDER BY TABLE_SCHEMA, TABLE_NAME")
      .then_return({{"",
          {"TABLE_SCHEMA", "TABLE_NAME", "PRIVILEGE_TYPE", "IS_GRANTABLE"},
          {Type::String, Type::String, Type::String, Type::String},
          {}  // No Records.
      }});
  res = instance.check_user("dba_user", "dba_host", test_priv, "test_db", "t2");
  EXPECT_TRUE(std::get<0>(res));
  EXPECT_STREQ("", std::get<1>(res).c_str());
  EXPECT_TRUE(std::get<2>(res));
  session.expect_query("SELECT PRIVILEGE_TYPE, IS_GRANTABLE "
                       "FROM INFORMATION_SCHEMA.USER_PRIVILEGES "
                       "WHERE GRANTEE = \"'dba_user'@'dba_host'\"")
      .then_return({{"", {"PRIVILEGE_TYPE", "IS_GRANTABLE"},
                     {Type::String, Type::String},
                     {{"SELECT", "YES"}, {"INSERT", "YES"},
                      {"UPDATE", "YES"}, {"DELETE", "YES"},
                      {"CREATE", "YES"}, {"DROP", "YES"},
                      {"RELOAD", "YES"}, {"SHUTDOWN", "YES"},
                      {"PROCESS", "YES"}, {"FILE", "YES"},
                      {"REFERENCES", "YES"}, {"INDEX", "YES"},
                      {"ALTER", "YES"}, {"SHOW DATABASES", "YES"},
                      {"SUPER", "YES"}, {"CREATE TEMPORARY TABLES", "YES"},
                      {"LOCK TABLES", "YES"}, {"EXECUTE", "YES"},
                      {"REPLICATION SLAVE", "YES"},
                      {"REPLICATION CLIENT", "YES"}, {"CREATE VIEW", "YES"},
                      {"SHOW VIEW", "YES"}, {"CREATE ROUTINE", "YES"},
                      {"ALTER ROUTINE", "YES"}, {"CREATE USER", "YES"},
                      {"EVENT", "YES"}, {"TRIGGER", "YES"},
                      {"CREATE TABLESPACE", "YES"}}}});
  session.expect_query("SELECT TABLE_SCHEMA, PRIVILEGE_TYPE, IS_GRANTABLE "
                       "FROM INFORMATION_SCHEMA.SCHEMA_PRIVILEGES "
                       "WHERE GRANTEE = \"'dba_user'@'dba_host'\" "
                       "ORDER BY TABLE_SCHEMA")
      .then_return({{"",
          {"TABLE_SCHEMA", "PRIVILEGE_TYPE", "IS_GRANTABLE"},
          {Type::String, Type::String, Type::String},
          {}  // No Records.
      }});
  session.expect_query(
          "SELECT TABLE_SCHEMA, TABLE_NAME, PRIVILEGE_TYPE, IS_GRANTABLE "
          "FROM INFORMATION_SCHEMA.TABLE_PRIVILEGES "
          "WHERE GRANTEE = \"'dba_user'@'dba_host'\" "
          "ORDER BY TABLE_SCHEMA, TABLE_NAME")
      .then_return({{"",
          {"TABLE_SCHEMA", "TABLE_NAME", "PRIVILEGE_TYPE", "IS_GRANTABLE"},
          {Type::String, Type::String, Type::String, Type::String},
          {}  // No Records.
      }});
  res = instance.check_user("dba_user", "dba_host", test_priv, "test_db", "t3");
  EXPECT_TRUE(std::get<0>(res));
  EXPECT_STREQ("", std::get<1>(res).c_str());
  EXPECT_TRUE(std::get<2>(res));
  session.expect_query("SELECT PRIVILEGE_TYPE, IS_GRANTABLE "
                       "FROM INFORMATION_SCHEMA.USER_PRIVILEGES "
                       "WHERE GRANTEE = \"'dba_user'@'dba_host'\"")
      .then_return({{"", {"PRIVILEGE_TYPE", "IS_GRANTABLE"},
                     {Type::String, Type::String},
                     {{"SELECT", "YES"}, {"INSERT", "YES"},
                      {"UPDATE", "YES"}, {"DELETE", "YES"},
                      {"CREATE", "YES"}, {"DROP", "YES"},
                      {"RELOAD", "YES"}, {"SHUTDOWN", "YES"},
                      {"PROCESS", "YES"}, {"FILE", "YES"},
                      {"REFERENCES", "YES"}, {"INDEX", "YES"},
                      {"ALTER", "YES"}, {"SHOW DATABASES", "YES"},
                      {"SUPER", "YES"}, {"CREATE TEMPORARY TABLES", "YES"},
                      {"LOCK TABLES", "YES"}, {"EXECUTE", "YES"},
                      {"REPLICATION SLAVE", "YES"},
                      {"REPLICATION CLIENT", "YES"}, {"CREATE VIEW", "YES"},
                      {"SHOW VIEW", "YES"}, {"CREATE ROUTINE", "YES"},
                      {"ALTER ROUTINE", "YES"}, {"CREATE USER", "YES"},
                      {"EVENT", "YES"}, {"TRIGGER", "YES"},
                      {"CREATE TABLESPACE", "YES"}}}});
  session.expect_query("SELECT TABLE_SCHEMA, PRIVILEGE_TYPE, IS_GRANTABLE "
                       "FROM INFORMATION_SCHEMA.SCHEMA_PRIVILEGES "
                       "WHERE GRANTEE = \"'dba_user'@'dba_host'\" "
                       "ORDER BY TABLE_SCHEMA")
      .then_return({{"",
          {"TABLE_SCHEMA", "PRIVILEGE_TYPE", "IS_GRANTABLE"},
          {Type::String, Type::String, Type::String},
          {}  // No Records.
      }});
  session.expect_query(
          "SELECT TABLE_SCHEMA, TABLE_NAME, PRIVILEGE_TYPE, IS_GRANTABLE "
          "FROM INFORMATION_SCHEMA.TABLE_PRIVILEGES "
          "WHERE GRANTEE = \"'dba_user'@'dba_host'\" "
          "ORDER BY TABLE_SCHEMA, TABLE_NAME")
      .then_return({{"",
          {"TABLE_SCHEMA", "TABLE_NAME", "PRIVILEGE_TYPE", "IS_GRANTABLE"},
          {Type::String, Type::String, Type::String, Type::String},
          {}  // No Records.
      }});
  res = instance.check_user("dba_user", "dba_host", test_priv, "test_db2",
                            "t1");
  EXPECT_TRUE(std::get<0>(res));
  EXPECT_STREQ("", std::get<1>(res).c_str());
  EXPECT_TRUE(std::get<2>(res));
  session.expect_query("SELECT PRIVILEGE_TYPE, IS_GRANTABLE "
                       "FROM INFORMATION_SCHEMA.USER_PRIVILEGES "
                       "WHERE GRANTEE = \"'dba_user'@'dba_host'\"")
      .then_return({{"", {"PRIVILEGE_TYPE", "IS_GRANTABLE"},
                     {Type::String, Type::String},
                     {{"SELECT", "YES"}, {"INSERT", "YES"},
                      {"UPDATE", "YES"}, {"DELETE", "YES"},
                      {"CREATE", "YES"}, {"DROP", "YES"},
                      {"RELOAD", "YES"}, {"SHUTDOWN", "YES"},
                      {"PROCESS", "YES"}, {"FILE", "YES"},
                      {"REFERENCES", "YES"}, {"INDEX", "YES"},
                      {"ALTER", "YES"}, {"SHOW DATABASES", "YES"},
                      {"SUPER", "YES"}, {"CREATE TEMPORARY TABLES", "YES"},
                      {"LOCK TABLES", "YES"}, {"EXECUTE", "YES"},
                      {"REPLICATION SLAVE", "YES"},
                      {"REPLICATION CLIENT", "YES"}, {"CREATE VIEW", "YES"},
                      {"SHOW VIEW", "YES"}, {"CREATE ROUTINE", "YES"},
                      {"ALTER ROUTINE", "YES"}, {"CREATE USER", "YES"},
                      {"EVENT", "YES"}, {"TRIGGER", "YES"},
                      {"CREATE TABLESPACE", "YES"}}}});
  session.expect_query("SELECT TABLE_SCHEMA, PRIVILEGE_TYPE, IS_GRANTABLE "
                       "FROM INFORMATION_SCHEMA.SCHEMA_PRIVILEGES "
                       "WHERE GRANTEE = \"'dba_user'@'dba_host'\" "
                       "ORDER BY TABLE_SCHEMA")
      .then_return({{"",
          {"TABLE_SCHEMA", "PRIVILEGE_TYPE", "IS_GRANTABLE"},
          {Type::String, Type::String, Type::String},
          {}  // No Records.
      }});
  session.expect_query(
          "SELECT TABLE_SCHEMA, TABLE_NAME, PRIVILEGE_TYPE, IS_GRANTABLE "
          "FROM INFORMATION_SCHEMA.TABLE_PRIVILEGES "
          "WHERE GRANTEE = \"'dba_user'@'dba_host'\" "
          "ORDER BY TABLE_SCHEMA, TABLE_NAME")
      .then_return({{"",
          {"TABLE_SCHEMA", "TABLE_NAME", "PRIVILEGE_TYPE", "IS_GRANTABLE"},
          {Type::String, Type::String, Type::String, Type::String},
          {}  // No Records.
      }});
  res = instance.check_user("dba_user", "dba_host", test_priv, "mysql", "user");
  EXPECT_TRUE(std::get<0>(res));
  EXPECT_STREQ("", std::get<1>(res).c_str());
  EXPECT_TRUE(std::get<2>(res));

  // Test a subset of privileges for *.*, test_db.*, test_db2.*, mysql.*,
  // test_db.t1, test_db.t2, test_db.t3, test_db2.t1 and mysql.user
  session.expect_query("SELECT PRIVILEGE_TYPE, IS_GRANTABLE "
                       "FROM INFORMATION_SCHEMA.USER_PRIVILEGES "
                       "WHERE GRANTEE = \"'dba_user'@'dba_host'\"")
      .then_return({{"", {"PRIVILEGE_TYPE", "IS_GRANTABLE"},
                     {Type::String, Type::String},
                     {{"SELECT", "YES"}, {"INSERT", "YES"},
                      {"UPDATE", "YES"}, {"DELETE", "YES"},
                      {"CREATE", "YES"}, {"DROP", "YES"},
                      {"RELOAD", "YES"}, {"SHUTDOWN", "YES"},
                      {"PROCESS", "YES"}, {"FILE", "YES"},
                      {"REFERENCES", "YES"}, {"INDEX", "YES"},
                      {"ALTER", "YES"}, {"SHOW DATABASES", "YES"},
                      {"SUPER", "YES"}, {"CREATE TEMPORARY TABLES", "YES"},
                      {"LOCK TABLES", "YES"}, {"EXECUTE", "YES"},
                      {"REPLICATION SLAVE", "YES"},
                      {"REPLICATION CLIENT", "YES"}, {"CREATE VIEW", "YES"},
                      {"SHOW VIEW", "YES"}, {"CREATE ROUTINE", "YES"},
                      {"ALTER ROUTINE", "YES"}, {"CREATE USER", "YES"},
                      {"EVENT", "YES"}, {"TRIGGER", "YES"},
                      {"CREATE TABLESPACE", "YES"}}}});
  session.expect_query("SELECT TABLE_SCHEMA, PRIVILEGE_TYPE, IS_GRANTABLE "
                       "FROM INFORMATION_SCHEMA.SCHEMA_PRIVILEGES "
                       "WHERE GRANTEE = \"'dba_user'@'dba_host'\" "
                       "ORDER BY TABLE_SCHEMA")
      .then_return({{"",
          {"TABLE_SCHEMA", "PRIVILEGE_TYPE", "IS_GRANTABLE"},
          {Type::String, Type::String, Type::String},
          {}  // No Records.
      }});
  session.expect_query(
          "SELECT TABLE_SCHEMA, TABLE_NAME, PRIVILEGE_TYPE, IS_GRANTABLE "
          "FROM INFORMATION_SCHEMA.TABLE_PRIVILEGES "
          "WHERE GRANTEE = \"'dba_user'@'dba_host'\" "
          "ORDER BY TABLE_SCHEMA, TABLE_NAME")
      .then_return({{"",
          {"TABLE_SCHEMA", "TABLE_NAME", "PRIVILEGE_TYPE", "IS_GRANTABLE"},
          {Type::String, Type::String, Type::String, Type::String},
          {}  // No Records.
      }});
  test_priv = {"Select", "INSERT", "UPDATE", "DELETE"};
  res = instance.check_user("dba_user", "dba_host", test_priv, "*", "*");
  EXPECT_TRUE(std::get<0>(res));
  EXPECT_STREQ("", std::get<1>(res).c_str());
  EXPECT_TRUE(std::get<2>(res));
  session.expect_query("SELECT PRIVILEGE_TYPE, IS_GRANTABLE "
                       "FROM INFORMATION_SCHEMA.USER_PRIVILEGES "
                       "WHERE GRANTEE = \"'dba_user'@'dba_host'\"")
      .then_return({{"", {"PRIVILEGE_TYPE", "IS_GRANTABLE"},
                     {Type::String, Type::String},
                     {{"SELECT", "YES"}, {"INSERT", "YES"},
                      {"UPDATE", "YES"}, {"DELETE", "YES"},
                      {"CREATE", "YES"}, {"DROP", "YES"},
                      {"RELOAD", "YES"}, {"SHUTDOWN", "YES"},
                      {"PROCESS", "YES"}, {"FILE", "YES"},
                      {"REFERENCES", "YES"}, {"INDEX", "YES"},
                      {"ALTER", "YES"}, {"SHOW DATABASES", "YES"},
                      {"SUPER", "YES"}, {"CREATE TEMPORARY TABLES", "YES"},
                      {"LOCK TABLES", "YES"}, {"EXECUTE", "YES"},
                      {"REPLICATION SLAVE", "YES"},
                      {"REPLICATION CLIENT", "YES"}, {"CREATE VIEW", "YES"},
                      {"SHOW VIEW", "YES"}, {"CREATE ROUTINE", "YES"},
                      {"ALTER ROUTINE", "YES"}, {"CREATE USER", "YES"},
                      {"EVENT", "YES"}, {"TRIGGER", "YES"},
                      {"CREATE TABLESPACE", "YES"}}}});
  session.expect_query("SELECT TABLE_SCHEMA, PRIVILEGE_TYPE, IS_GRANTABLE "
                       "FROM INFORMATION_SCHEMA.SCHEMA_PRIVILEGES "
                       "WHERE GRANTEE = \"'dba_user'@'dba_host'\" "
                       "ORDER BY TABLE_SCHEMA")
      .then_return({{"",
          {"TABLE_SCHEMA", "PRIVILEGE_TYPE", "IS_GRANTABLE"},
          {Type::String, Type::String, Type::String},
          {}  // No Records.
      }});
  session.expect_query(
          "SELECT TABLE_SCHEMA, TABLE_NAME, PRIVILEGE_TYPE, IS_GRANTABLE "
          "FROM INFORMATION_SCHEMA.TABLE_PRIVILEGES "
          "WHERE GRANTEE = \"'dba_user'@'dba_host'\" "
          "ORDER BY TABLE_SCHEMA, TABLE_NAME")
      .then_return({{"",
          {"TABLE_SCHEMA", "TABLE_NAME", "PRIVILEGE_TYPE", "IS_GRANTABLE"},
          {Type::String, Type::String, Type::String, Type::String},
          {}  // No Records.
      }});
  res = instance.check_user("dba_user", "dba_host", test_priv, "*", "*");
  EXPECT_TRUE(std::get<0>(res));
  EXPECT_STREQ("", std::get<1>(res).c_str());
  EXPECT_TRUE(std::get<2>(res));
  session.expect_query("SELECT PRIVILEGE_TYPE, IS_GRANTABLE "
                       "FROM INFORMATION_SCHEMA.USER_PRIVILEGES "
                       "WHERE GRANTEE = \"'dba_user'@'dba_host'\"")
      .then_return({{"", {"PRIVILEGE_TYPE", "IS_GRANTABLE"},
                     {Type::String, Type::String},
                     {{"SELECT", "YES"}, {"INSERT", "YES"},
                      {"UPDATE", "YES"}, {"DELETE", "YES"},
                      {"CREATE", "YES"}, {"DROP", "YES"},
                      {"RELOAD", "YES"}, {"SHUTDOWN", "YES"},
                      {"PROCESS", "YES"}, {"FILE", "YES"},
                      {"REFERENCES", "YES"}, {"INDEX", "YES"},
                      {"ALTER", "YES"}, {"SHOW DATABASES", "YES"},
                      {"SUPER", "YES"}, {"CREATE TEMPORARY TABLES", "YES"},
                      {"LOCK TABLES", "YES"}, {"EXECUTE", "YES"},
                      {"REPLICATION SLAVE", "YES"},
                      {"REPLICATION CLIENT", "YES"}, {"CREATE VIEW", "YES"},
                      {"SHOW VIEW", "YES"}, {"CREATE ROUTINE", "YES"},
                      {"ALTER ROUTINE", "YES"}, {"CREATE USER", "YES"},
                      {"EVENT", "YES"}, {"TRIGGER", "YES"},
                      {"CREATE TABLESPACE", "YES"}}}});
  session.expect_query("SELECT TABLE_SCHEMA, PRIVILEGE_TYPE, IS_GRANTABLE "
                       "FROM INFORMATION_SCHEMA.SCHEMA_PRIVILEGES "
                       "WHERE GRANTEE = \"'dba_user'@'dba_host'\" "
                       "ORDER BY TABLE_SCHEMA")
      .then_return({{"",
          {"TABLE_SCHEMA", "PRIVILEGE_TYPE", "IS_GRANTABLE"},
          {Type::String, Type::String, Type::String},
          {}  // No Records.
      }});
  session.expect_query(
          "SELECT TABLE_SCHEMA, TABLE_NAME, PRIVILEGE_TYPE, IS_GRANTABLE "
          "FROM INFORMATION_SCHEMA.TABLE_PRIVILEGES "
          "WHERE GRANTEE = \"'dba_user'@'dba_host'\" "
          "ORDER BY TABLE_SCHEMA, TABLE_NAME")
      .then_return({{"",
          {"TABLE_SCHEMA", "TABLE_NAME", "PRIVILEGE_TYPE", "IS_GRANTABLE"},
          {Type::String, Type::String, Type::String, Type::String},
          {}  // No Records.
      }});
  res = instance.check_user("dba_user", "dba_host", test_priv, "test_db", "*");
  EXPECT_TRUE(std::get<0>(res));
  EXPECT_STREQ("", std::get<1>(res).c_str());
  EXPECT_TRUE(std::get<2>(res));
  session.expect_query("SELECT PRIVILEGE_TYPE, IS_GRANTABLE "
                       "FROM INFORMATION_SCHEMA.USER_PRIVILEGES "
                       "WHERE GRANTEE = \"'dba_user'@'dba_host'\"")
      .then_return({{"", {"PRIVILEGE_TYPE", "IS_GRANTABLE"},
                     {Type::String, Type::String},
                     {{"SELECT", "YES"}, {"INSERT", "YES"},
                      {"UPDATE", "YES"}, {"DELETE", "YES"},
                      {"CREATE", "YES"}, {"DROP", "YES"},
                      {"RELOAD", "YES"}, {"SHUTDOWN", "YES"},
                      {"PROCESS", "YES"}, {"FILE", "YES"},
                      {"REFERENCES", "YES"}, {"INDEX", "YES"},
                      {"ALTER", "YES"}, {"SHOW DATABASES", "YES"},
                      {"SUPER", "YES"}, {"CREATE TEMPORARY TABLES", "YES"},
                      {"LOCK TABLES", "YES"}, {"EXECUTE", "YES"},
                      {"REPLICATION SLAVE", "YES"},
                      {"REPLICATION CLIENT", "YES"}, {"CREATE VIEW", "YES"},
                      {"SHOW VIEW", "YES"}, {"CREATE ROUTINE", "YES"},
                      {"ALTER ROUTINE", "YES"}, {"CREATE USER", "YES"},
                      {"EVENT", "YES"}, {"TRIGGER", "YES"},
                      {"CREATE TABLESPACE", "YES"}}}});
  session.expect_query("SELECT TABLE_SCHEMA, PRIVILEGE_TYPE, IS_GRANTABLE "
                       "FROM INFORMATION_SCHEMA.SCHEMA_PRIVILEGES "
                       "WHERE GRANTEE = \"'dba_user'@'dba_host'\" "
                       "ORDER BY TABLE_SCHEMA")
      .then_return({{"",
          {"TABLE_SCHEMA", "PRIVILEGE_TYPE", "IS_GRANTABLE"},
          {Type::String, Type::String, Type::String},
          {}  // No Records.
      }});
  session.expect_query(
          "SELECT TABLE_SCHEMA, TABLE_NAME, PRIVILEGE_TYPE, IS_GRANTABLE "
          "FROM INFORMATION_SCHEMA.TABLE_PRIVILEGES "
          "WHERE GRANTEE = \"'dba_user'@'dba_host'\" "
          "ORDER BY TABLE_SCHEMA, TABLE_NAME")
      .then_return({{"",
          {"TABLE_SCHEMA", "TABLE_NAME", "PRIVILEGE_TYPE", "IS_GRANTABLE"},
          {Type::String, Type::String, Type::String, Type::String},
          {}  // No Records.
      }});
  res = instance.check_user("dba_user", "dba_host", test_priv, "mysql", "*");
  EXPECT_TRUE(std::get<0>(res));
  EXPECT_STREQ("", std::get<1>(res).c_str());
  EXPECT_TRUE(std::get<2>(res));
  session.expect_query("SELECT PRIVILEGE_TYPE, IS_GRANTABLE "
                       "FROM INFORMATION_SCHEMA.USER_PRIVILEGES "
                       "WHERE GRANTEE = \"'dba_user'@'dba_host'\"")
      .then_return({{"", {"PRIVILEGE_TYPE", "IS_GRANTABLE"},
                     {Type::String, Type::String},
                     {{"SELECT", "YES"}, {"INSERT", "YES"},
                      {"UPDATE", "YES"}, {"DELETE", "YES"},
                      {"CREATE", "YES"}, {"DROP", "YES"},
                      {"RELOAD", "YES"}, {"SHUTDOWN", "YES"},
                      {"PROCESS", "YES"}, {"FILE", "YES"},
                      {"REFERENCES", "YES"}, {"INDEX", "YES"},
                      {"ALTER", "YES"}, {"SHOW DATABASES", "YES"},
                      {"SUPER", "YES"}, {"CREATE TEMPORARY TABLES", "YES"},
                      {"LOCK TABLES", "YES"}, {"EXECUTE", "YES"},
                      {"REPLICATION SLAVE", "YES"},
                      {"REPLICATION CLIENT", "YES"}, {"CREATE VIEW", "YES"},
                      {"SHOW VIEW", "YES"}, {"CREATE ROUTINE", "YES"},
                      {"ALTER ROUTINE", "YES"}, {"CREATE USER", "YES"},
                      {"EVENT", "YES"}, {"TRIGGER", "YES"},
                      {"CREATE TABLESPACE", "YES"}}}});
  session.expect_query("SELECT TABLE_SCHEMA, PRIVILEGE_TYPE, IS_GRANTABLE "
                       "FROM INFORMATION_SCHEMA.SCHEMA_PRIVILEGES "
                       "WHERE GRANTEE = \"'dba_user'@'dba_host'\" "
                       "ORDER BY TABLE_SCHEMA")
      .then_return({{"",
          {"TABLE_SCHEMA", "PRIVILEGE_TYPE", "IS_GRANTABLE"},
          {Type::String, Type::String, Type::String},
          {}  // No Records.
      }});
  session.expect_query(
          "SELECT TABLE_SCHEMA, TABLE_NAME, PRIVILEGE_TYPE, IS_GRANTABLE "
          "FROM INFORMATION_SCHEMA.TABLE_PRIVILEGES "
          "WHERE GRANTEE = \"'dba_user'@'dba_host'\" "
          "ORDER BY TABLE_SCHEMA, TABLE_NAME")
      .then_return({{"",
          {"TABLE_SCHEMA", "TABLE_NAME", "PRIVILEGE_TYPE", "IS_GRANTABLE"},
          {Type::String, Type::String, Type::String, Type::String},
          {}  // No Records.
      }});
  res = instance.check_user("dba_user", "dba_host", test_priv, "test_db", "t1");
  EXPECT_TRUE(std::get<0>(res));
  EXPECT_STREQ("", std::get<1>(res).c_str());
  EXPECT_TRUE(std::get<2>(res));
  session.expect_query("SELECT PRIVILEGE_TYPE, IS_GRANTABLE "
                       "FROM INFORMATION_SCHEMA.USER_PRIVILEGES "
                       "WHERE GRANTEE = \"'dba_user'@'dba_host'\"")
      .then_return({{"", {"PRIVILEGE_TYPE", "IS_GRANTABLE"},
                     {Type::String, Type::String},
                     {{"SELECT", "YES"}, {"INSERT", "YES"},
                      {"UPDATE", "YES"}, {"DELETE", "YES"},
                      {"CREATE", "YES"}, {"DROP", "YES"},
                      {"RELOAD", "YES"}, {"SHUTDOWN", "YES"},
                      {"PROCESS", "YES"}, {"FILE", "YES"},
                      {"REFERENCES", "YES"}, {"INDEX", "YES"},
                      {"ALTER", "YES"}, {"SHOW DATABASES", "YES"},
                      {"SUPER", "YES"}, {"CREATE TEMPORARY TABLES", "YES"},
                      {"LOCK TABLES", "YES"}, {"EXECUTE", "YES"},
                      {"REPLICATION SLAVE", "YES"},
                      {"REPLICATION CLIENT", "YES"}, {"CREATE VIEW", "YES"},
                      {"SHOW VIEW", "YES"}, {"CREATE ROUTINE", "YES"},
                      {"ALTER ROUTINE", "YES"}, {"CREATE USER", "YES"},
                      {"EVENT", "YES"}, {"TRIGGER", "YES"},
                      {"CREATE TABLESPACE", "YES"}}}});
  session.expect_query("SELECT TABLE_SCHEMA, PRIVILEGE_TYPE, IS_GRANTABLE "
                       "FROM INFORMATION_SCHEMA.SCHEMA_PRIVILEGES "
                       "WHERE GRANTEE = \"'dba_user'@'dba_host'\" "
                       "ORDER BY TABLE_SCHEMA")
      .then_return({{"",
          {"TABLE_SCHEMA", "PRIVILEGE_TYPE", "IS_GRANTABLE"},
          {Type::String, Type::String, Type::String},
          {}  // No Records.
      }});
  session.expect_query(
          "SELECT TABLE_SCHEMA, TABLE_NAME, PRIVILEGE_TYPE, IS_GRANTABLE "
          "FROM INFORMATION_SCHEMA.TABLE_PRIVILEGES "
          "WHERE GRANTEE = \"'dba_user'@'dba_host'\" "
          "ORDER BY TABLE_SCHEMA, TABLE_NAME")
      .then_return({{"",
          {"TABLE_SCHEMA", "TABLE_NAME", "PRIVILEGE_TYPE", "IS_GRANTABLE"},
          {Type::String, Type::String, Type::String, Type::String},
          {}  // No Records.
      }});
  res = instance.check_user("dba_user", "dba_host", test_priv, "test_db", "t2");
  EXPECT_TRUE(std::get<0>(res));
  EXPECT_STREQ("", std::get<1>(res).c_str());
  EXPECT_TRUE(std::get<2>(res));
  session.expect_query("SELECT PRIVILEGE_TYPE, IS_GRANTABLE "
                       "FROM INFORMATION_SCHEMA.USER_PRIVILEGES "
                       "WHERE GRANTEE = \"'dba_user'@'dba_host'\"")
      .then_return({{"", {"PRIVILEGE_TYPE", "IS_GRANTABLE"},
                     {Type::String, Type::String},
                     {{"SELECT", "YES"}, {"INSERT", "YES"},
                      {"UPDATE", "YES"}, {"DELETE", "YES"},
                      {"CREATE", "YES"}, {"DROP", "YES"},
                      {"RELOAD", "YES"}, {"SHUTDOWN", "YES"},
                      {"PROCESS", "YES"}, {"FILE", "YES"},
                      {"REFERENCES", "YES"}, {"INDEX", "YES"},
                      {"ALTER", "YES"}, {"SHOW DATABASES", "YES"},
                      {"SUPER", "YES"}, {"CREATE TEMPORARY TABLES", "YES"},
                      {"LOCK TABLES", "YES"}, {"EXECUTE", "YES"},
                      {"REPLICATION SLAVE", "YES"},
                      {"REPLICATION CLIENT", "YES"}, {"CREATE VIEW", "YES"},
                      {"SHOW VIEW", "YES"}, {"CREATE ROUTINE", "YES"},
                      {"ALTER ROUTINE", "YES"}, {"CREATE USER", "YES"},
                      {"EVENT", "YES"}, {"TRIGGER", "YES"},
                      {"CREATE TABLESPACE", "YES"}}}});
  session.expect_query("SELECT TABLE_SCHEMA, PRIVILEGE_TYPE, IS_GRANTABLE "
                       "FROM INFORMATION_SCHEMA.SCHEMA_PRIVILEGES "
                       "WHERE GRANTEE = \"'dba_user'@'dba_host'\" "
                       "ORDER BY TABLE_SCHEMA")
      .then_return({{"",
          {"TABLE_SCHEMA", "PRIVILEGE_TYPE", "IS_GRANTABLE"},
          {Type::String, Type::String, Type::String},
          {}  // No Records.
      }});
  session.expect_query(
          "SELECT TABLE_SCHEMA, TABLE_NAME, PRIVILEGE_TYPE, IS_GRANTABLE "
          "FROM INFORMATION_SCHEMA.TABLE_PRIVILEGES "
          "WHERE GRANTEE = \"'dba_user'@'dba_host'\" "
          "ORDER BY TABLE_SCHEMA, TABLE_NAME")
      .then_return({{"",
          {"TABLE_SCHEMA", "TABLE_NAME", "PRIVILEGE_TYPE", "IS_GRANTABLE"},
          {Type::String, Type::String, Type::String, Type::String},
          {}  // No Records.
      }});
  res = instance.check_user("dba_user", "dba_host", test_priv, "mysql", "user");
  EXPECT_TRUE(std::get<0>(res));
  EXPECT_STREQ("", std::get<1>(res).c_str());
  EXPECT_TRUE(std::get<2>(res));

  EXPECT_CALL(session, close());
  _session->close();
}

TEST_F(Instance_test, drop_user) {
  EXPECT_CALL(session, connect(_connection_options));
  _session->connect(_connection_options);
  mysqlshdk::mysql::Instance instance(_session);

  // DROP table and database previously created in test create_user.
  EXPECT_CALL(session, execute("DROP TABLE test_db.t1"));
  _session->execute("DROP TABLE test_db.t1");
  EXPECT_CALL(session, execute("DROP TABLE test_db.t2"));
  _session->execute("DROP TABLE test_db.t2");
  EXPECT_CALL(session, execute("DROP DATABASE test_db"));
  _session->execute("DROP DATABASE test_db");
  EXPECT_CALL(session, execute("DROP TABLE test_db2.t1"));
  _session->execute("DROP TABLE test_db2.t1");
  EXPECT_CALL(session, execute("DROP DATABASE test_db2"));
  _session->execute("DROP DATABASE test_db2");


  // Drop users previously created in test create_user.
  EXPECT_CALL(session, execute("DROP USER 'test_user'@'test_host'"));
  instance.drop_user("test_user", "test_host");
  EXPECT_CALL(session, execute("DROP USER 'dba_user'@'dba_host'"));
  instance.drop_user("dba_user", "dba_host");
  // Second drop user fails because user does not exist.
  EXPECT_CALL(session, execute("DROP USER 'test_user'@'test_host'"))
      .Times(1)
      .WillRepeatedly(Throw(std::exception()));
  EXPECT_THROW(instance.drop_user("test_user", "test_host"), std::exception);
  EXPECT_CALL(session, execute("DROP USER 'dba_user'@'dba_host'"))
      .Times(1)
      .WillRepeatedly(Throw(std::exception()));
  EXPECT_THROW(instance.drop_user("dba_user", "dba_host"), std::exception);

  EXPECT_CALL(session, close());
  _session->close();
}

}  // namespace testing
