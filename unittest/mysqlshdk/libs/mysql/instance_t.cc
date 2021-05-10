/*
 * Copyright (c) 2017, 2021, Oracle and/or its affiliates.
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

#include "mysqlshdk/libs/mysql/instance.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "unittest/mysqlshdk/libs/mysql/user_privileges_t.h"
#include "unittest/test_utils/mocks/mysqlshdk/libs/db/mock_result.h"
#include "unittest/test_utils/mocks/mysqlshdk/libs/db/mock_session.h"
#include "unittest/test_utils/shell_base_test.h"

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
  session
      .expect_query(
          "show SESSION variables "
          "where `variable_name` in ('character_set_server')")
      .then_return({{"show SESSION variables "
                     "where `variable_name` in ('character_set_server')",
                     {"Variable_name", "Value"},
                     {Type::String, Type::String},
                     {{"character_set_server", "latin1"}}}});
  mysqlshdk::utils::nullable<std::string> char_set_server =
      instance.get_sysvar_string("character_set_server",
                                 mysqlshdk::mysql::Var_qualifier::SESSION);
  EXPECT_FALSE(char_set_server.is_null());
  EXPECT_STREQ("latin1", (*char_set_server).c_str());

  // Get string system variable with GLOBAL scope.
  session
      .expect_query(
          "show GLOBAL variables "
          "where `variable_name` in ('character_set_server')")
      .then_return({{"show GLOBAL variables "
                     "where `variable_name` in ('character_set_server')",
                     {"Variable_name", "Value"},
                     {Type::String, Type::String},
                     {{"character_set_server", "latin1"}}}});
  char_set_server = instance.get_sysvar_string(
      "character_set_server", mysqlshdk::mysql::Var_qualifier::GLOBAL);
  EXPECT_FALSE(char_set_server.is_null());
  EXPECT_STREQ("latin1", (*char_set_server).c_str());

  EXPECT_CALL(session, close());
  _session->close();
}

TEST_F(Instance_test, get_sysvar_string_unexisting_variable) {
  EXPECT_CALL(session, connect(_connection_options));
  _session->connect(_connection_options);

  session
      .expect_query(
          "show GLOBAL variables where `variable_name` in"
          " ('unexisting_variable')")
      .then_return({{
          "show GLOBAL variables where `variable_name` in"
          " ('unexisting_variable')",
          {"Variable_name", "Value"},
          {Type::String, Type::String},
          {}  // No Records...
      }});

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

  session
      .expect_query(
          "show GLOBAL variables where `variable_name` in"
          " ('sql_warnings')")
      .then_return({{"show GLOBAL variables where `variable_name` in"
                     " ('sql_warnings')",
                     {"Variable_name", "Value"},
                     {Type::String, Type::String},
                     {{"sql_warnings", "OFF"}}}});
  mysqlshdk::utils::nullable<bool> sql_warnings =
      instance.get_sysvar_bool("sql_warnings");
  // The value was not modified
  EXPECT_FALSE(sql_warnings.is_null());
  EXPECT_FALSE(*sql_warnings);

  // Get boolean system variable with SESSION scope.
  session
      .expect_query(
          "show SESSION variables where `variable_name` in ('sql_warnings')")
      .then_return({{"show SESSION variables "
                     "where `variable_name` in ('sql_warnings')",
                     {"Variable_name", "Value"},
                     {Type::String, Type::String},
                     {{"sql_warnings", "OFF"}}}});
  sql_warnings = instance.get_sysvar_bool(
      "sql_warnings", mysqlshdk::mysql::Var_qualifier::SESSION);
  EXPECT_FALSE(sql_warnings.is_null());
  EXPECT_FALSE(*sql_warnings);

  // Get boolean system variable with GLOBAL scope.
  session
      .expect_query(
          "show GLOBAL variables where `variable_name` in ('sql_warnings')")
      .then_return({{"show GLOBAL variables "
                     "where `variable_name` in ('sql_warnings')",
                     {"Variable_name", "Value"},
                     {Type::String, Type::String},
                     {{"sql_warnings", "OFF"}}}});
  sql_warnings = instance.get_sysvar_bool(
      "sql_warnings", mysqlshdk::mysql::Var_qualifier::GLOBAL);
  EXPECT_FALSE(sql_warnings.is_null());
  EXPECT_FALSE(*sql_warnings);

  EXPECT_CALL(session, close());
  _session->close();
}

TEST_F(Instance_test, get_sysvar_boolean_unexisting_variable) {
  EXPECT_CALL(session, connect(_connection_options));
  _session->connect(_connection_options);

  session
      .expect_query(
          "show GLOBAL variables where `variable_name` in"
          " ('unexisting_variable')")
      .then_return({{
          "show GLOBAL variables where `variable_name` in"
          " ('unexisting_variable')",
          {"Variable_name", "Value"},
          {Type::String, Type::String},
          {}  // No Records...
      }});

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

  session
      .expect_query(
          "show GLOBAL variables where `variable_name` in"
          " ('server_uuid')")
      .then_return(
          {{"show GLOBAL variables where `variable_name` in"
            " ('server_uuid')",
            {"Variable_name", "Value"},
            {Type::String, Type::String},
            {{"server_uuid", "891a2c04-1cc7-11e7-8323-00059a3c7a00"}}}});

  mysqlshdk::mysql::Instance instance(_session);
  EXPECT_ANY_THROW(instance.get_sysvar_bool("server_uuid"));

  EXPECT_CALL(session, close());
  _session->close();
}

TEST_F(Instance_test, get_sysvar_int_existing_variable) {
  EXPECT_CALL(session, connect(_connection_options));
  _session->connect(_connection_options);
  mysqlshdk::mysql::Instance instance(_session);

  session
      .expect_query(
          "show GLOBAL variables where `variable_name` in"
          " ('server_id')")
      .then_return({{"show SESSION variables where `variable_name` in"
                     " ('server_id')",
                     {"Variable_name", "Value"},
                     {Type::String, Type::String},
                     {{"server_id", "0"}}}});
  mysqlshdk::utils::nullable<int64_t> server_id =
      instance.get_sysvar_int("server_id");
  // The value was not modified
  EXPECT_FALSE(server_id.is_null());
  EXPECT_EQ(0, *server_id);

  // Get integer system variable with SESSION scope.
  session
      .expect_query(
          "show SESSION variables "
          "where `variable_name` in ('wait_timeout')")
      .then_return({{"show SESSION variables "
                     "where `variable_name` in ('wait_timeout')",
                     {"Variable_name", "Value"},
                     {Type::String, Type::String},
                     {{"wait_timeout", "28800"}}}});
  mysqlshdk::utils::nullable<int64_t> wait_timeout = instance.get_sysvar_int(
      "wait_timeout", mysqlshdk::mysql::Var_qualifier::SESSION);
  EXPECT_FALSE(wait_timeout.is_null());
  EXPECT_EQ(28800, *wait_timeout);

  // Get integer system variable with GLOBAL scope.
  session
      .expect_query(
          "show GLOBAL variables "
          "where `variable_name` in ('wait_timeout')")
      .then_return({{"show GLOBAL variables "
                     "where `variable_name` in ('wait_timeout')",
                     {"Variable_name", "Value"},
                     {Type::String, Type::String},
                     {{"wait_timeout", "28800"}}}});
  wait_timeout = instance.get_sysvar_int(
      "wait_timeout", mysqlshdk::mysql::Var_qualifier::GLOBAL);
  EXPECT_FALSE(wait_timeout.is_null());
  EXPECT_EQ(28800, *wait_timeout);

  EXPECT_CALL(session, close());
  _session->close();
}

TEST_F(Instance_test, get_sysvar_int_unexisting_variable) {
  EXPECT_CALL(session, connect(_connection_options));
  _session->connect(_connection_options);

  session
      .expect_query(
          "show GLOBAL variables where `variable_name` in"
          " ('unexisting_variable')")
      .then_return({{
          "show GLOBAL variables where `variable_name` in"
          " ('unexisting_variable')",
          {"Variable_name", "Value"},
          {Type::String, Type::String},
          {}  // No Records...
      }});

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

  session
      .expect_query(
          "show GLOBAL variables where `variable_name` in"
          " ('server_uuid')")
      .then_return(
          {{"show GLOBAL variables where `variable_name` in"
            " ('server_uuid')",
            {"Variable_name", "Value"},
            {Type::String, Type::String},
            {{"server_uuid", "891a2c04-1cc7-11e7-8323-00059a3c7a00"}}}});

  mysqlshdk::mysql::Instance instance(_session);
  EXPECT_ANY_THROW(instance.get_sysvar_int("server_uuid"));

  EXPECT_CALL(session, close());
  _session->close();
}

TEST_F(Instance_test, get_sysvar_invalid_qualifier) {
  EXPECT_CALL(session, connect(_connection_options));
  _session->connect(_connection_options);

  mysqlshdk::mysql::Instance instance(_session);
  EXPECT_THROW(
      instance.get_sysvar_string("character_set_server",
                                 mysqlshdk::mysql::Var_qualifier::PERSIST),
      std::runtime_error);
  EXPECT_THROW(
      instance.get_sysvar_string("character_set_server",
                                 mysqlshdk::mysql::Var_qualifier::PERSIST_ONLY),
      std::runtime_error);
  EXPECT_THROW(instance.get_sysvar_bool(
                   "sql_warnings", mysqlshdk::mysql::Var_qualifier::PERSIST),
               std::runtime_error);
  EXPECT_THROW(
      instance.get_sysvar_bool("sql_warnings",
                               mysqlshdk::mysql::Var_qualifier::PERSIST_ONLY),
      std::runtime_error);
  EXPECT_THROW(instance.get_sysvar_int(
                   "wait_timeout", mysqlshdk::mysql::Var_qualifier::PERSIST),
               std::runtime_error);
  EXPECT_THROW(
      instance.get_sysvar_int("wait_timeout",
                              mysqlshdk::mysql::Var_qualifier::PERSIST_ONLY),
      std::runtime_error);

  EXPECT_CALL(session, close());
  _session->close();
}

TEST_F(Instance_test, set_sysvar) {
  EXPECT_CALL(session, connect(_connection_options));
  _session->connect(_connection_options);
  mysqlshdk::mysql::Instance instance(_session);

  // Test set string system variable with different scopes (GLOBAL and SESSION).
  session.expect_query("SET GLOBAL `lc_messages` = 'fr_FR'").then({""});
  instance.set_sysvar("lc_messages", (std::string) "fr_FR");
  session
      .expect_query(
          "show GLOBAL variables where `variable_name` in ('lc_messages')")
      .then_return({{"show GLOBAL variables "
                     "where `variable_name` in ('lc_messages')",
                     {"Variable_name", "Value"},
                     {Type::String, Type::String},
                     {{"lc_messages", "fr_FR"}}}});
  mysqlshdk::utils::nullable<std::string> new_value =
      instance.get_sysvar_string("lc_messages");
  EXPECT_STREQ("fr_FR", (*new_value).c_str());
  session.expect_query("SET GLOBAL `lc_messages` = 'pt_PT'").then({""});
  instance.set_sysvar("lc_messages", (std::string) "pt_PT",
                      mysqlshdk::mysql::Var_qualifier::GLOBAL);
  session
      .expect_query(
          "show GLOBAL variables where `variable_name` in ('lc_messages')")
      .then_return({{"show GLOBAL variables "
                     "where `variable_name` in ('lc_messages')",
                     {"Variable_name", "Value"},
                     {Type::String, Type::String},
                     {{"lc_messages", "pt_PT"}}}});
  new_value = instance.get_sysvar_string(
      "lc_messages", mysqlshdk::mysql::Var_qualifier::GLOBAL);
  EXPECT_STREQ("pt_PT", (*new_value).c_str());
  session.expect_query("SET SESSION `lc_messages` = 'es_MX'").then({""});
  instance.set_sysvar("lc_messages", (std::string) "es_MX",
                      mysqlshdk::mysql::Var_qualifier::SESSION);
  session
      .expect_query(
          "show SESSION variables where `variable_name` in ('lc_messages')")
      .then_return({{"show SESSION variables "
                     "where `variable_name` in ('lc_messages')",
                     {"Variable_name", "Value"},
                     {Type::String, Type::String},
                     {{"lc_messages", "es_MX"}}}});
  new_value = instance.get_sysvar_string(
      "lc_messages", mysqlshdk::mysql::Var_qualifier::SESSION);
  EXPECT_STREQ("es_MX", (*new_value).c_str());

  // Test set int system variable with different scopes (GLOBAL and SESSION).
  session.expect_query("SET GLOBAL `lock_wait_timeout` = 86400").then({""});
  instance.set_sysvar("lock_wait_timeout", (int64_t)86400);
  session
      .expect_query(
          "show GLOBAL variables "
          "where `variable_name` in ('lock_wait_timeout')")
      .then_return({{"show GLOBAL variables "
                     "where `variable_name` in ('lock_wait_timeout')",
                     {"Variable_name", "Value"},
                     {Type::String, Type::String},
                     {{"lock_wait_timeout", "86400"}}}});
  mysqlshdk::utils::nullable<int64_t> new_i_value =
      instance.get_sysvar_int("lock_wait_timeout");
  EXPECT_EQ(86400, *new_i_value);
  session.expect_query("SET GLOBAL `lock_wait_timeout` = 172800").then({""});
  instance.set_sysvar("lock_wait_timeout", (int64_t)172800,
                      mysqlshdk::mysql::Var_qualifier::GLOBAL);
  session
      .expect_query(
          "show GLOBAL variables "
          "where `variable_name` in ('lock_wait_timeout')")
      .then_return({{"show GLOBAL variables "
                     "where `variable_name` in ('lock_wait_timeout')",
                     {"Variable_name", "Value"},
                     {Type::String, Type::String},
                     {{"lock_wait_timeout", "172800"}}}});
  new_i_value = instance.get_sysvar_int(
      "lock_wait_timeout", mysqlshdk::mysql::Var_qualifier::GLOBAL);
  EXPECT_EQ(172800, *new_i_value);

  session.expect_query("SET SESSION `lock_wait_timeout` = 43200").then({""});
  instance.set_sysvar("lock_wait_timeout", (int64_t)43200,
                      mysqlshdk::mysql::Var_qualifier::SESSION);
  session
      .expect_query(
          "show SESSION variables "
          "where `variable_name` in ('lock_wait_timeout')")
      .then_return({{"show SESSION variables "
                     "where `variable_name` in ('lock_wait_timeout')",
                     {"Variable_name", "Value"},
                     {Type::String, Type::String},
                     {{"lock_wait_timeout", "43200"}}}});
  new_i_value = instance.get_sysvar_int(
      "lock_wait_timeout", mysqlshdk::mysql::Var_qualifier::SESSION);
  EXPECT_EQ(43200, *new_i_value);

  // Test set bool system variable with different scopes (GLOBAL and SESSION).
  session.expect_query("SET GLOBAL `sql_log_off` = 'ON'").then({""});
  instance.set_sysvar("sql_log_off", true);
  session
      .expect_query(
          "show GLOBAL variables where `variable_name` in ('sql_log_off')")
      .then_return({{"show GLOBAL variables "
                     "where `variable_name` in ('sql_log_off')",
                     {"Variable_name", "Value"},
                     {Type::String, Type::String},
                     {{"sql_log_off", "ON"}}}});
  mysqlshdk::utils::nullable<bool> new_b_value =
      instance.get_sysvar_bool("sql_log_off");
  EXPECT_TRUE(*new_b_value);
  session.expect_query("SET GLOBAL `sql_log_off` = 'ON'").then({""});
  instance.set_sysvar("sql_log_off", true,
                      mysqlshdk::mysql::Var_qualifier::GLOBAL);
  session
      .expect_query(
          "show GLOBAL variables where `variable_name` in ('sql_log_off')")
      .then_return({{"show GLOBAL variables "
                     "where `variable_name` in ('sql_log_off')",
                     {"Variable_name", "Value"},
                     {Type::String, Type::String},
                     {{"sql_log_off", "ON"}}}});
  new_b_value = instance.get_sysvar_bool(
      "sql_log_off", mysqlshdk::mysql::Var_qualifier::GLOBAL);
  EXPECT_TRUE(*new_b_value);
  session.expect_query("SET SESSION `sql_log_off` = 'OFF'").then({""});
  instance.set_sysvar("sql_log_off", false,
                      mysqlshdk::mysql::Var_qualifier::SESSION);
  session
      .expect_query(
          "show SESSION variables where `variable_name` in ('sql_log_off')")
      .then_return({{"show SESSION variables "
                     "where `variable_name` in ('sql_log_off')",
                     {"Variable_name", "Value"},
                     {Type::String, Type::String},
                     {{"sql_log_off", "OFF"}}}});
  new_b_value = instance.get_sysvar_bool(
      "sql_log_off", mysqlshdk::mysql::Var_qualifier::SESSION);
  EXPECT_FALSE(*new_b_value);

  // NOTE: Tests using PERSIST and PERSIT_ONLY are only supported by server
  //       versions >= 8.0.2
  // Set string with PERSIST_ONLY.
  session.expect_query("SET PERSIST_ONLY `lc_messages` = 'en_US'").then({""});
  instance.set_sysvar("lc_messages", (std::string) "en_US",
                      mysqlshdk::mysql::Var_qualifier::PERSIST_ONLY);
  session
      .expect_query(
          "show GLOBAL variables where `variable_name` in ('lc_messages')")
      .then_return({{"show GLOBAL variables "
                     "where `variable_name` in ('lc_messages')",
                     {"Variable_name", "Value"},
                     {Type::String, Type::String},
                     {{"lc_messages", "pt_PT"}}}});
  new_value = instance.get_sysvar_string(
      "lc_messages", mysqlshdk::mysql::Var_qualifier::GLOBAL);
  EXPECT_STREQ("pt_PT", (*new_value).c_str());
  std::string persited_var_value_stmt =
      "SELECT VARIABLE_VALUE "
      "FROM performance_schema.persisted_variables "
      "WHERE VARIABLE_NAME = 'lc_messages'";
  session.expect_query(persited_var_value_stmt)
      .then_return({{persited_var_value_stmt,
                     {"VARIABLE_VALUE"},
                     {Type::String},
                     {{"en_US"}}}});
  auto resultset = session.query(persited_var_value_stmt, false);
  std::string persisted_value = resultset->fetch_one()->get_string(0);
  EXPECT_STREQ("en_US", persisted_value.c_str());
  EXPECT_CALL(session, execute("RESET PERSIST lc_messages"));
  session.execute("RESET PERSIST lc_messages");
  // Set string with PERSIST.
  session.expect_query("SET PERSIST `lc_messages` = 'en_US'").then({""});
  instance.set_sysvar("lc_messages", (std::string) "en_US",
                      mysqlshdk::mysql::Var_qualifier::PERSIST);
  session
      .expect_query(
          "show GLOBAL variables where `variable_name` in ('lc_messages')")
      .then_return({{"show GLOBAL variables "
                     "where `variable_name` in ('lc_messages')",
                     {"Variable_name", "Value"},
                     {Type::String, Type::String},
                     {{"lc_messages", "en_US"}}}});
  new_value = instance.get_sysvar_string(
      "lc_messages", mysqlshdk::mysql::Var_qualifier::GLOBAL);
  EXPECT_STREQ("en_US", (*new_value).c_str());
  session.expect_query(persited_var_value_stmt)
      .then_return({{persited_var_value_stmt,
                     {"VARIABLE_VALUE"},
                     {Type::String},
                     {{"en_US"}}}});
  resultset = session.query(persited_var_value_stmt, false);
  persisted_value = resultset->fetch_one()->get_string(0);
  EXPECT_STREQ("en_US", persisted_value.c_str());
  EXPECT_CALL(session, execute("RESET PERSIST lc_messages"));
  session.execute("RESET PERSIST lc_messages");
  // Set int with PERSIST_ONLY.
  session.expect_query("SET PERSIST_ONLY `lock_wait_timeout` = 172801")
      .then({""});
  instance.set_sysvar("lock_wait_timeout", (int64_t)172801,
                      mysqlshdk::mysql::Var_qualifier::PERSIST_ONLY);
  session
      .expect_query(
          "show GLOBAL variables "
          "where `variable_name` in ('lock_wait_timeout')")
      .then_return({{"show GLOBAL variables "
                     "where `variable_name` in ('lock_wait_timeout')",
                     {"Variable_name", "Value"},
                     {Type::String, Type::String},
                     {{"lock_wait_timeout", "172800"}}}});
  new_i_value = instance.get_sysvar_int(
      "lock_wait_timeout", mysqlshdk::mysql::Var_qualifier::GLOBAL);
  EXPECT_EQ(172800, *new_i_value);
  persited_var_value_stmt =
      "SELECT VARIABLE_VALUE "
      "FROM performance_schema.persisted_variables "
      "WHERE VARIABLE_NAME = 'lock_wait_timeout'";
  session.expect_query(persited_var_value_stmt)
      .then_return({{persited_var_value_stmt,
                     {"VARIABLE_VALUE"},
                     {Type::Integer},
                     {{"172801"}}}});
  resultset = session.query(persited_var_value_stmt, false);
  int64_t persisted_i_value = resultset->fetch_one()->get_int(0);
  EXPECT_EQ(172801, persisted_i_value);
  EXPECT_CALL(session, execute("RESET PERSIST lock_wait_timeout"));
  session.execute("RESET PERSIST lock_wait_timeout");
  // Set int with PERSIST.
  session.expect_query("SET PERSIST `lock_wait_timeout` = 172801").then({""});
  instance.set_sysvar("lock_wait_timeout", (int64_t)172801,
                      mysqlshdk::mysql::Var_qualifier::PERSIST);
  session
      .expect_query(
          "show GLOBAL variables "
          "where `variable_name` in ('lock_wait_timeout')")
      .then_return({{"show GLOBAL variables "
                     "where `variable_name` in ('lock_wait_timeout')",
                     {"Variable_name", "Value"},
                     {Type::String, Type::String},
                     {{"lock_wait_timeout", "172801"}}}});
  new_i_value = instance.get_sysvar_int(
      "lock_wait_timeout", mysqlshdk::mysql::Var_qualifier::GLOBAL);
  EXPECT_EQ(172801, *new_i_value);
  session.expect_query(persited_var_value_stmt)
      .then_return({{persited_var_value_stmt,
                     {"VARIABLE_VALUE"},
                     {Type::Integer},
                     {{"172801"}}}});
  resultset = session.query(persited_var_value_stmt, false);
  persisted_i_value = resultset->fetch_one()->get_int(0);
  EXPECT_EQ(172801, persisted_i_value);
  EXPECT_CALL(session, execute("RESET PERSIST lock_wait_timeout"));
  session.execute("RESET PERSIST lock_wait_timeout");
  // Set boolean with PERSIST_ONLY.
  session.expect_query("SET PERSIST_ONLY `sql_log_off` = 'OFF'").then({""});
  instance.set_sysvar("sql_log_off", false,
                      mysqlshdk::mysql::Var_qualifier::PERSIST_ONLY);
  session
      .expect_query(
          "show GLOBAL variables where `variable_name` in ('sql_log_off')")
      .then_return({{"show GLOBAL variables "
                     "where `variable_name` in ('sql_log_off')",
                     {"Variable_name", "Value"},
                     {Type::String, Type::String},
                     {{"sql_log_off", "ON"}}}});
  new_b_value = instance.get_sysvar_bool(
      "sql_log_off", mysqlshdk::mysql::Var_qualifier::GLOBAL);
  EXPECT_TRUE(*new_b_value);
  persited_var_value_stmt =
      "SELECT VARIABLE_VALUE "
      "FROM performance_schema.persisted_variables "
      "WHERE VARIABLE_NAME = 'sql_log_off'";
  session.expect_query(persited_var_value_stmt)
      .then_return({{persited_var_value_stmt,
                     {"VARIABLE_VALUE"},
                     {Type::Integer},
                     {{"OFF"}}}});
  resultset = session.query(persited_var_value_stmt, false);
  std::string persisted_b_value = resultset->fetch_one()->get_string(0);
  EXPECT_STREQ("OFF", persisted_b_value.c_str());
  EXPECT_CALL(session, execute("RESET PERSIST sql_log_off"));
  session.execute("RESET PERSIST sql_log_off");
  // Set boolean with PERSIST.
  session.expect_query("SET PERSIST `sql_log_off` = 'OFF'").then({""});
  instance.set_sysvar("sql_log_off", false,
                      mysqlshdk::mysql::Var_qualifier::PERSIST);
  session
      .expect_query(
          "show GLOBAL variables where `variable_name` in ('sql_log_off')")
      .then_return({{"show GLOBAL variables "
                     "where `variable_name` in ('sql_log_off')",
                     {"Variable_name", "Value"},
                     {Type::String, Type::String},
                     {{"sql_log_off", "OFF"}}}});
  new_b_value = instance.get_sysvar_bool(
      "sql_log_off", mysqlshdk::mysql::Var_qualifier::GLOBAL);
  EXPECT_FALSE(*new_b_value);
  session.expect_query(persited_var_value_stmt)
      .then_return({{persited_var_value_stmt,
                     {"VARIABLE_VALUE"},
                     {Type::Integer},
                     {{"OFF"}}}});
  resultset = session.query(persited_var_value_stmt, false);
  persisted_b_value = resultset->fetch_one()->get_string(0);
  EXPECT_STREQ("OFF", persisted_b_value.c_str());
  EXPECT_CALL(session, execute("RESET PERSIST sql_log_off"));
  session.execute("RESET PERSIST sql_log_off");

  EXPECT_CALL(session, close());
  _session->close();
}

TEST_F(Instance_test, set_sysvar_default) {
  EXPECT_CALL(session, connect(_connection_options));
  _session->connect(_connection_options);
  mysqlshdk::mysql::Instance instance(_session);
  // Test set_sysvar_default with different scopes (GLOBAL and SESSION).
  session.expect_query("SET SESSION `max_user_connections` = DEFAULT")
      .then({""});
  instance.set_sysvar_default("max_user_connections",
                              mysqlshdk::mysql::Var_qualifier::SESSION);
  session
      .expect_query(
          "show SESSION variables where `variable_name` in "
          "('max_user_connections')")
      .then_return({{"show SESSION variables "
                     "where `variable_name` in ('max_user_connections')",
                     {"Variable_name", "Value"},
                     {Type::String, Type::String},
                     {{"max_user_connections", "0"}}}});
  mysqlshdk::utils::nullable<std::string> new_value =
      instance.get_sysvar_string("max_user_connections",
                                 mysqlshdk::mysql::Var_qualifier::SESSION);
  EXPECT_STREQ("0", (*new_value).c_str());

  // Test set_sysvar_default with different scopes (GLOBAL and SESSION).
  session.expect_query("SET GLOBAL `max_user_connections` = DEFAULT")
      .then({""});
  instance.set_sysvar_default("max_user_connections",
                              mysqlshdk::mysql::Var_qualifier::GLOBAL);
  session
      .expect_query(
          "show GLOBAL variables where `variable_name` in "
          "('max_user_connections')")
      .then_return({{"show GLOBAL variables "
                     "where `variable_name` in ('max_user_connections')",
                     {"Variable_name", "Value"},
                     {Type::String, Type::String},
                     {{"max_user_connections", "0"}}}});
  new_value = instance.get_sysvar_string(
      "max_user_connections", mysqlshdk::mysql::Var_qualifier::GLOBAL);
  EXPECT_STREQ("0", (*new_value).c_str());

  // NOTE: Tests using PERSIST and PERSIST_ONLY are only supported by server
  //       versions >= 8.0.2
  // Set sysvar_default with PERSIST_ONLY.
  session.expect_query("SET PERSIST_ONLY `lc_messages` = DEFAULT").then({""});
  instance.set_sysvar_default("lc_messages",
                              mysqlshdk::mysql::Var_qualifier::PERSIST_ONLY);
  session
      .expect_query(
          "show GLOBAL variables where `variable_name` in ('lc_messages')")
      .then_return({{"show GLOBAL variables "
                     "where `variable_name` in ('lc_messages')",
                     {"Variable_name", "Value"},
                     {Type::String, Type::String},
                     {{"lc_messages", "en_US"}}}});
  new_value = instance.get_sysvar_string(
      "lc_messages", mysqlshdk::mysql::Var_qualifier::GLOBAL);
  EXPECT_STREQ("en_US", (*new_value).c_str());
  std::string persisted_var_value_stmt =
      "SELECT VARIABLE_VALUE "
      "FROM performance_schema.persisted_variables "
      "WHERE VARIABLE_NAME = 'lc_messages'";
  session.expect_query(persisted_var_value_stmt)
      .then_return({{persisted_var_value_stmt,
                     {"VARIABLE_VALUE"},
                     {Type::String},
                     {{"en_US"}}}});
  auto resultset = session.query(persisted_var_value_stmt, false);
  std::string persisted_value = resultset->fetch_one()->get_string(0);
  EXPECT_STREQ("en_US", persisted_value.c_str());
  EXPECT_CALL(session, execute("RESET PERSIST lc_messages"));
  session.execute("RESET PERSIST lc_messages");
  // Set string with PERSIST.
  session.expect_query("SET PERSIST `lc_messages` = DEFAULT").then({""});
  instance.set_sysvar_default("lc_messages",
                              mysqlshdk::mysql::Var_qualifier::PERSIST);
  session
      .expect_query(
          "show GLOBAL variables where `variable_name` in ('lc_messages')")
      .then_return({{"show GLOBAL variables "
                     "where `variable_name` in ('lc_messages')",
                     {"Variable_name", "Value"},
                     {Type::String, Type::String},
                     {{"lc_messages", "en_US"}}}});
  new_value = instance.get_sysvar_string(
      "lc_messages", mysqlshdk::mysql::Var_qualifier::GLOBAL);
  EXPECT_STREQ("en_US", (*new_value).c_str());
  session.expect_query(persisted_var_value_stmt)
      .then_return({{persisted_var_value_stmt,
                     {"VARIABLE_VALUE"},
                     {Type::String},
                     {{"en_US"}}}});
  resultset = session.query(persisted_var_value_stmt, false);
  persisted_value = resultset->fetch_one()->get_string(0);
  EXPECT_STREQ("en_US", persisted_value.c_str());
  EXPECT_CALL(session, execute("RESET PERSIST lc_messages"));
  session.execute("RESET PERSIST lc_messages");

  EXPECT_CALL(session, close());
  _session->close();
}

TEST_F(Instance_test, get_system_variables) {
  EXPECT_CALL(session, connect(_connection_options));
  _session->connect(_connection_options);

  session
      .expect_query(
          "show GLOBAL variables where `variable_name` in"
          " ('server_id', 'server_uuid', 'unexisting_variable')")
      .then_return(
          {{"show GLOBAL variables where `variable_name` in"
            " ('server_id', 'server_uuid', 'unexisting_variable')",
            {"Variable_name", "Value"},
            {Type::String, Type::String},
            {{"server_id", "0"},
             {"server_uuid", "891a2c04-1cc7-11e7-8323-00059a3c7a00"}}}});

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

TEST_F(Instance_test, install_plugin_win) {
  EXPECT_CALL(session, connect(_connection_options));
  _session->connect(_connection_options);
  mysqlshdk::mysql::Instance instance(_session);

  // Install plugin on Windows
  session
      .expect_query(
          "show GLOBAL variables where `variable_name` in "
          "('version_compile_os')")
      .then_return({{"",
                     {"Variable_name", "Value"},
                     {Type::String, Type::String},
                     {{"version_compile_os", "Win64"}}}});
  EXPECT_EXECUTE_CALL(
      session,
      "INSTALL PLUGIN `validate_password` SONAME 'validate_password.dll'");
  instance.install_plugin("validate_password");

  EXPECT_CALL(session, close());
  _session->close();
}

TEST_F(Instance_test, install_plugin_lin) {
  EXPECT_CALL(session, connect(_connection_options));
  _session->connect(_connection_options);
  mysqlshdk::mysql::Instance instance(_session);

  // Install plugin on Non-Windows
  session
      .expect_query(
          "show GLOBAL variables where `variable_name` in "
          "('version_compile_os')")
      .then_return({{"",
                     {"Variable_name", "Value"},
                     {Type::String, Type::String},
                     {{"version_compile_os", "linux-glibc2.5"}}}});
  EXPECT_EXECUTE_CALL(
      session,
      "INSTALL PLUGIN `validate_password` SONAME 'validate_password.so'");
  instance.install_plugin("validate_password");

  // Second install fails because plugin is already installed.
  session
      .expect_query(
          "show GLOBAL variables where `variable_name` in "
          "('version_compile_os')")
      .then_return({{"",
                     {"Variable_name", "Value"},
                     {Type::String, Type::String},
                     {{"version_compile_os", "linux-glibc2.5"}}}});
  EXPECT_EXECUTE_CALL(
      session,
      "INSTALL PLUGIN `validate_password` SONAME 'validate_password.so'")
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
  session
      .expect_query(
          "SELECT plugin_status FROM information_schema.plugins "
          "WHERE plugin_name = 'validate_password'")
      .then_return({{"", {"plugin_status"}, {Type::String}, {{"ACTIVE"}}}});
  mysqlshdk::utils::nullable<std::string> res =
      instance.get_plugin_status("validate_password");
  ASSERT_FALSE(res.is_null());
  EXPECT_STREQ("ACTIVE", (*res).c_str());

  // Test plugin DISABLED
  session
      .expect_query(
          "SELECT plugin_status FROM information_schema.plugins "
          "WHERE plugin_name = 'validate_password'")
      .then_return({{"", {"plugin_status"}, {Type::String}, {{"DISABLED"}}}});
  res = instance.get_plugin_status("validate_password");
  ASSERT_FALSE(res.is_null());
  EXPECT_STREQ("DISABLED", (*res).c_str());

  // Test plugin not available
  session
      .expect_query(
          "SELECT plugin_status FROM information_schema.plugins "
          "WHERE plugin_name = 'validate_password'")
      .then_return({{"", {"plugin_status"}, {Type::String}, {}}});
  res = instance.get_plugin_status("validate_password");
  ASSERT_TRUE(res.is_null());

  EXPECT_CALL(session, close());
  _session->close();
}

TEST_F(Instance_test, uninstall_plugin) {
  EXPECT_CALL(session, connect(_connection_options));
  _session->connect(_connection_options);

  mysqlshdk::mysql::Instance instance(_session);
  EXPECT_EXECUTE_CALL(session, "UNINSTALL PLUGIN `validate_password`");
  instance.uninstall_plugin("validate_password");

  // Second uninstall fails because plugin was already uninstalled.
  EXPECT_EXECUTE_CALL(session, "UNINSTALL PLUGIN `validate_password`")
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
  EXPECT_EXECUTE_CALL(session, "CREATE DATABASE test_db");
  _session->execute("CREATE DATABASE test_db");
  EXPECT_EXECUTE_CALL(session, "CREATE TABLE test_db.t1 (c1 INT)");
  _session->execute("CREATE TABLE test_db.t1 (c1 INT)");
  EXPECT_EXECUTE_CALL(session, "CREATE TABLE test_db.t2 (c1 INT)");
  _session->execute("CREATE TABLE test_db.t2 (c1 INT)");
  EXPECT_EXECUTE_CALL(session, "CREATE DATABASE test_db2");
  _session->execute("CREATE DATABASE test_db2");
  EXPECT_EXECUTE_CALL(session, "CREATE TABLE test_db2.t1 (c1 INT)");
  _session->execute("CREATE TABLE test_db2.t1 (c1 INT)");

  // Create user with SELECT, INSERT, UPDATE on test_db.* and DELETE on
  // test_db.t1.
  EXPECT_EXECUTE_CALL(session,
                      "CREATE USER IF NOT EXISTS 'test_user'@'test_host' "
                      "IDENTIFIED BY /*((*/ 'test_pwd' /*))*/");
  EXPECT_EXECUTE_CALL(
      session,
      "GRANT SELECT, INSERT, UPDATE ON test_db.* TO 'test_user'@'test_host'");
  EXPECT_EXECUTE_CALL(session,
                      "GRANT DELETE ON test_db.t1 TO 'test_user'@'test_host'");
  EXPECT_EXECUTE_CALL(session,
                      "GRANT ALTER,DROP ON test_db.t2 TO "
                      "'test_user'@'test_host' WITH GRANT OPTION");
  EXPECT_EXECUTE_CALL(session,
                      "GRANT SELECT ON test_db2.* TO 'test_user'@'test_host' "
                      "WITH GRANT OPTION");
  std::vector<std::tuple<std::string, std::string, bool>> test_priv = {
      std::make_tuple("SELECT, INSERT, UPDATE", "test_db.*", false),
      std::make_tuple("DELETE", "test_db.t1", false),
      std::make_tuple("ALTER,DROP", "test_db.t2", true),
      std::make_tuple("SELECT", "test_db2.*", true)};
  instance.create_user("test_user", "test_host", "test_pwd", test_priv);
  // Create user with ALL on *.* WITH GRANT OPTION.
  EXPECT_EXECUTE_CALL(session,
                      "CREATE USER IF NOT EXISTS 'dba_user'@'dba_host' "
                      "IDENTIFIED BY /*((*/ 'dba_pwd' /*))*/");
  EXPECT_EXECUTE_CALL(
      session, "GRANT ALL ON *.* TO 'dba_user'@'dba_host' WITH GRANT OPTION");
  std::vector<std::tuple<std::string, std::string, bool>> dba_priv = {
      std::make_tuple("ALL", "*.*", true)};
  instance.create_user("dba_user", "dba_host", "dba_pwd", dba_priv);

  // Second create users fail because they already exist.
  EXPECT_EXECUTE_CALL(session,
                      "CREATE USER IF NOT EXISTS 'test_user'@'test_host' "
                      "IDENTIFIED BY /*((*/ 'test_pwd' /*))*/")
      .Times(1)
      .WillRepeatedly(Throw(std::exception()));
  EXPECT_THROW(
      instance.create_user("test_user", "test_host", "test_pwd", test_priv),
      std::exception);
  EXPECT_EXECUTE_CALL(session,
                      "CREATE USER IF NOT EXISTS 'dba_user'@'dba_host' "
                      "IDENTIFIED BY /*((*/ 'dba_pwd' /*))*/")
      .Times(1)
      .WillRepeatedly(Throw(std::exception()));
  EXPECT_THROW(
      instance.create_user("dba_user", "dba_host", "dba_pwd", dba_priv),
      std::exception);
  EXPECT_CALL(session, close());
  _session->close();
}

TEST_F(Instance_test, get_user_privileges_user_does_not_exist) {
  EXPECT_CALL(session, connect(_connection_options));
  _session->connect(_connection_options);

  // Check non existing user.
  testing::user_privileges::Setup_options options;

  options.user = "notexist_user";
  options.host = "notexist_host";
  options.user_exists = false;
  // Simulate 5.7 version is used (not relevant here, 5.7 used to avoid
  // executing additional queries to update the ALL privileges list).
  options.is_8_0 = false;

  testing::user_privileges::setup(options, &session);

  mysqlshdk::mysql::Instance instance(_session);

  auto up = instance.get_user_privileges(options.user, options.host);

  ASSERT_TRUE(nullptr != up);

  EXPECT_FALSE(up->user_exists());

  EXPECT_CALL(session, close());
  _session->close();
}

TEST_F(Instance_test, get_user_privileges_user_exists) {
  EXPECT_CALL(session, connect(_connection_options));
  _session->connect(_connection_options);

  // user with some privileges
  testing::user_privileges::Setup_options options;

  options.user = "test_user";
  options.host = "test_host";
  // Simulate 8.0.0 version is always used.
  options.is_8_0 = true;

  options.grants = {
      "GRANT USAGE ON *.* TO u@h",
      "GRANT INSERT, SELECT, UPDATE ON `test_db`.* TO u@h",
      "GRANT SELECT ON `test_db2`.* TO u@h WITH GRANT OPTION",
      "GRANT DELETE ON `test_db`.`t1` TO u@h",
      "GRANT DROP, ALTER ON `test_db`.`t2` TO u@h WITH GRANT OPTION",
  };

  testing::user_privileges::setup(options, &session);

  mysqlshdk::mysql::Instance instance(_session);

  auto up = instance.get_user_privileges(options.user, options.host);

  ASSERT_TRUE(nullptr != up);

  EXPECT_TRUE(up->user_exists());

  EXPECT_CALL(session, close());
  _session->close();
}

TEST_F(Instance_test, drop_user) {
  EXPECT_CALL(session, connect(_connection_options));
  _session->connect(_connection_options);
  mysqlshdk::mysql::Instance instance(_session);

  // DROP table and database previously created in test create_user.
  EXPECT_EXECUTE_CALL(session, "DROP TABLE test_db.t1");
  _session->execute("DROP TABLE test_db.t1");
  EXPECT_EXECUTE_CALL(session, "DROP TABLE test_db.t2");
  _session->execute("DROP TABLE test_db.t2");
  EXPECT_EXECUTE_CALL(session, "DROP DATABASE test_db");
  _session->execute("DROP DATABASE test_db");
  EXPECT_EXECUTE_CALL(session, "DROP TABLE test_db2.t1");
  _session->execute("DROP TABLE test_db2.t1");
  EXPECT_EXECUTE_CALL(session, "DROP DATABASE test_db2");
  _session->execute("DROP DATABASE test_db2");

  // Drop users previously created in test create_user.
  EXPECT_EXECUTE_CALL(session, "DROP USER 'test_user'@'test_host'");
  instance.drop_user("test_user", "test_host");
  EXPECT_EXECUTE_CALL(session, "DROP USER 'dba_user'@'dba_host'");
  instance.drop_user("dba_user", "dba_host");
  // Second drop user fails because user does not exist.
  EXPECT_EXECUTE_CALL(session, "DROP USER 'test_user'@'test_host'")
      .Times(1)
      .WillRepeatedly(Throw(std::exception()));
  EXPECT_THROW(instance.drop_user("test_user", "test_host"), std::exception);
  EXPECT_EXECUTE_CALL(session, "DROP USER 'dba_user'@'dba_host'")
      .Times(1)
      .WillRepeatedly(Throw(std::exception()));
  EXPECT_THROW(instance.drop_user("dba_user", "dba_host"), std::exception);

  EXPECT_CALL(session, close());
  _session->close();
}

TEST_F(Instance_test, get_system_variables_like) {
  EXPECT_CALL(session, connect(_connection_options));
  _session->connect(_connection_options);
  mysqlshdk::mysql::Instance instance(_session);

  // Get system variable using a pattern, using default scope (i.e., SESSION).
  session.expect_query("SHOW GLOBAL VARIABLES LIKE '%error_count%'")
      .then_return({{"",
                     {"Variable_name", "Value"},
                     {Type::String, Type::String},
                     {{"error_count", "0"}, {"max_error_count", "64"}}}});

  std::map<std::string, mysqlshdk::utils::nullable<std::string>> res =
      instance.get_system_variables_like("%error_count%");

  // Verify result.
  std::vector<std::string> vars;
  std::vector<std::string> values;
  for (const auto &element : res) {
    vars.push_back(element.first);
    values.push_back(*element.second);
  }
  EXPECT_THAT(vars, UnorderedElementsAre("error_count", "max_error_count"));
  EXPECT_THAT(values, UnorderedElementsAre("0", "64"));

  // Get SESSION system variable using a pattern.
  session.expect_query("SHOW SESSION VARIABLES LIKE '%error_count%'")
      .then_return({{"",
                     {"Variable_name", "Value"},
                     {Type::String, Type::String},
                     {{"error_count", "0"}, {"max_error_count", "64"}}}});

  res = instance.get_system_variables_like(
      "%error_count%", mysqlshdk::mysql::Var_qualifier::SESSION);

  // Verify result.
  vars.clear();
  values.clear();
  for (const auto &element : res) {
    vars.push_back(element.first);
    values.push_back(*element.second);
  }
  EXPECT_THAT(vars, UnorderedElementsAre("error_count", "max_error_count"));
  EXPECT_THAT(values, UnorderedElementsAre("0", "64"));

  // Get GLOBAL system variable using a pattern.
  session.expect_query("SHOW GLOBAL VARIABLES LIKE 'flush%'")
      .then_return({{"",
                     {"Variable_name", "Value"},
                     {Type::String, Type::String},
                     {{"flush", "OFF"}, {"flush_time", "0"}}}});

  res = instance.get_system_variables_like(
      "flush%", mysqlshdk::mysql::Var_qualifier::GLOBAL);

  // Verify result.
  vars.clear();
  values.clear();
  for (const auto &element : res) {
    vars.push_back(element.first);
    values.push_back(*element.second);
  }
  EXPECT_THAT(vars, UnorderedElementsAre("flush", "flush_time"));
  EXPECT_THAT(values, UnorderedElementsAre("OFF", "0"));

  // Error getting system variables from an non valid scope.
  EXPECT_THROW_LIKE(
      instance.get_system_variables_like(
          "%error_count%", mysqlshdk::mysql::Var_qualifier::PERSIST),
      std::runtime_error,
      "Invalid variable scope to get variables value, only GLOBAL and SESSION "
      "is supported.");
  EXPECT_THROW_LIKE(
      instance.get_system_variables_like(
          "%error_count%", mysqlshdk::mysql::Var_qualifier::PERSIST_ONLY),
      std::runtime_error,
      "Invalid variable scope to get variables value, only GLOBAL and SESSION "
      "is supported.");

  EXPECT_CALL(session, close());
  _session->close();
}

TEST_F(Instance_test, is_set_persist_supported) {
  EXPECT_CALL(session, connect(_connection_options));
  _session->connect(_connection_options);
  mysqlshdk::mysql::Instance instance(_session);

  // Simulate 8.0.11 version is used.
  EXPECT_CALL(session, get_server_version())
      .WillRepeatedly(Return(mysqlshdk::utils::Version(8, 0, 11)));

  // Set persisted_globals_load is OFF.
  session
      .expect_query(
          "show GLOBAL variables where `variable_name` in "
          "('persisted_globals_load')")
      .then_return({{"show GLOBAL variables "
                     "where `variable_name` in ('persisted_globals_load')",
                     {"Variable_name", "Value"},
                     {Type::String, Type::String},
                     {{"persisted_globals_load", "OFF"}}}});
  mysqlshdk::utils::nullable<bool> res = instance.is_set_persist_supported();
  EXPECT_FALSE(res.is_null());
  EXPECT_FALSE(*res);

  // Set persisted_globals_load is ON.
  session
      .expect_query(
          "show GLOBAL variables where `variable_name` in "
          "('persisted_globals_load')")
      .then_return({{"show GLOBAL variables "
                     "where `variable_name` in ('persisted_globals_load')",
                     {"Variable_name", "Value"},
                     {Type::String, Type::String},
                     {{"persisted_globals_load", "ON"}}}});
  res = instance.is_set_persist_supported();
  EXPECT_FALSE(res.is_null());
  EXPECT_TRUE(*res);

  mysqlshdk::mysql::Instance instance_57(_session);

  // Simulate 5.7 version is used.
  EXPECT_CALL(session, get_server_version())
      .WillRepeatedly(Return(mysqlshdk::utils::Version(5, 7, 24)));

  // Set is not supported.
  res = instance_57.is_set_persist_supported();
  EXPECT_TRUE(res.is_null());

  EXPECT_CALL(session, close());
  _session->close();
}

TEST_F(Instance_test, get_persisted_value) {
  EXPECT_CALL(session, connect(_connection_options));
  _session->connect(_connection_options);
  mysqlshdk::mysql::Instance instance(_session);

  // No value found for the specified variable.
  session
      .expect_query(
          "SELECT variable_value FROM performance_schema.persisted_variables "
          "WHERE variable_name = 'gtid_mode'")
      .then_return({{
          "", {"variable_value"}, {Type::String}, {}  // No records
      }});
  mysqlshdk::utils::nullable<std::string> res =
      instance.get_persisted_value("gtid_mode");
  EXPECT_TRUE(res.is_null());

  // Persisted value successfully returned.
  session
      .expect_query(
          "SELECT variable_value FROM performance_schema.persisted_variables "
          "WHERE variable_name = 'gtid_mode'")
      .then_return({{"", {"variable_value"}, {Type::String}, {{"ON"}}}});
  res = instance.get_persisted_value("gtid_mode");
  EXPECT_FALSE(res.is_null());
  EXPECT_STREQ("ON", (*res).c_str());

  EXPECT_CALL(session, close());
  _session->close();
}

TEST_F(Instance_test, suppress_binary_log) {
  EXPECT_CALL(session, connect(_connection_options));
  _session->connect(_connection_options);
  mysqlshdk::mysql::Instance instance(_session);

  EXPECT_EXECUTE_CALL(session, "SET SESSION sql_log_bin=0");
  instance.suppress_binary_log(true);
  instance.suppress_binary_log(true);

  instance.suppress_binary_log(false);
  EXPECT_EXECUTE_CALL(session, "SET SESSION sql_log_bin=1");
  instance.suppress_binary_log(false);
}

TEST_F(Instance_test, get_fence_sysvars) {
  EXPECT_CALL(session, connect(_connection_options));
  _session->connect(_connection_options);
  mysqlshdk::mysql::Instance instance(_session);

  // No fence sysvars enabled.
  session.expect_query("SELECT @@read_only, @@super_read_only, @@offline_mode")
      .then_return({{"",
                     {"@@read_only", "@@super_read_only", "@@offline_mode"},
                     {Type::String, Type::String, Type::String},
                     {{"0", "0", "0"}}}});
  std::vector<std::string> res = instance.get_fence_sysvars();
  EXPECT_TRUE(res.empty());

  // All fence sysvars enabled.
  session.expect_query("SELECT @@read_only, @@super_read_only, @@offline_mode")
      .then_return({{"",
                     {"@@read_only", "@@super_read_only", "@@offline_mode"},
                     {Type::String, Type::String, Type::String},
                     {{"1", "1", "1"}}}});
  res = instance.get_fence_sysvars();
  EXPECT_FALSE(res.empty());
  EXPECT_THAT(res, ElementsAre("read_only", "super_read_only", "offline_mode"));

  // read_only enabled.
  session.expect_query("SELECT @@read_only, @@super_read_only, @@offline_mode")
      .then_return({{"",
                     {"@@read_only", "@@super_read_only", "@@offline_mode"},
                     {Type::String, Type::String, Type::String},
                     {{"1", "0", "0"}}}});
  res = instance.get_fence_sysvars();
  EXPECT_FALSE(res.empty());
  EXPECT_THAT(res, ElementsAre("read_only"));

  // super_read_only enabled.
  session.expect_query("SELECT @@read_only, @@super_read_only, @@offline_mode")
      .then_return({{"",
                     {"@@read_only", "@@super_read_only", "@@offline_mode"},
                     {Type::String, Type::String, Type::String},
                     {{"0", "1", "0"}}}});
  res = instance.get_fence_sysvars();
  EXPECT_FALSE(res.empty());
  EXPECT_THAT(res, ElementsAre("super_read_only"));

  // offline_mode enabled.
  session.expect_query("SELECT @@read_only, @@super_read_only, @@offline_mode")
      .then_return({{"",
                     {"@@read_only", "@@super_read_only", "@@offline_mode"},
                     {Type::String, Type::String, Type::String},
                     {{"0", "0", "1"}}}});
  res = instance.get_fence_sysvars();
  EXPECT_FALSE(res.empty());
  EXPECT_THAT(res, ElementsAre("offline_mode"));

  // Error if no rows are returned.
  session.expect_query("SELECT @@read_only, @@super_read_only, @@offline_mode")
      .then_return({{
          "",
          {"@@read_only", "@@super_read_only", "@@offline_mode"},
          {Type::String, Type::String, Type::String},
          {}  // No records.
      }});
  EXPECT_THROW(instance.get_fence_sysvars(), std::logic_error);
}

}  // namespace testing
