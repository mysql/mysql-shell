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

#include "mysqlshdk/libs/mysql/instance.h"
#include "unittest/mocks/mysqlshdk/libs/db/mock_result.h"
#include "unittest/mocks/mysqlshdk/libs/db/mock_session.h"
#include "unittest/test_utils.h"

using mysqlshdk::db::Type;
namespace testing {

// Fake deleter for the _session to avoid it attempt deleting the passed
// Session object, since we are passing a pointer created from a reference
struct DoNotDelete {
  template <typename T>
  void operator()(T *) {}
};

class Instance_test : public Shell_base_test {
 protected:
  // An Instance requires an ISession shared pointer
  std::shared_ptr<mysqlshdk::db::ISession> _session;
  Mock_session session;

  virtual void SetUp() {
    _session.reset(&session, DoNotDelete());

    Shell_base_test::SetUp();
  }
};

TEST_F(Instance_test, get_session) {
  mysqlshdk::mysql::Instance instance(_session);

  EXPECT_EQ(_session, instance.get_session());
}

TEST_F(Instance_test, get_sysvar_string_existing_variable) {
  EXPECT_CALL(session, connect(_mysql_uri, _pwd.c_str()));
  _session->connect(_mysql_uri, _pwd.c_str());

  mysqlshdk::mysql::Instance instance(_session);

  Mock_result *result = new Mock_result();
  auto fake_result = result->add_result({"VARIABLE_NAME", "VARIABLE_VALUE"},
                                        {Type::VarString, Type::VarString});
  fake_result->add_row({"server_uuid", "7042e17f-1f14-11e7-bc28-28d24455c64b"});
  // Passes the result to the session, ownership included.
  session.set_result(result);

  mysqlshdk::utils::nullable<std::string> server_uuid =
      instance.get_sysvar_string("server_uuid");

  // The value was not modified
  EXPECT_FALSE(server_uuid.is_null());

  EXPECT_EQ("7042e17f-1f14-11e7-bc28-28d24455c64b", *server_uuid);

  EXPECT_CALL(session, close());
  _session->close();
}

TEST_F(Instance_test, get_sysvar_string_unexisting_variable) {
  EXPECT_CALL(session, connect(_mysql_uri, _pwd.c_str()));
  _session->connect(_mysql_uri, _pwd.c_str());

  mysqlshdk::mysql::Instance instance(_session);

  Mock_result *result = new Mock_result();
  auto fake_result = result->add_result({"VARIABLE_NAME", "VARIABLE_VALUE"},
                                        {Type::VarString, Type::VarString});
  // Passes the result to the session, ownership included.
  session.set_result(result);

  mysqlshdk::utils::nullable<std::string> server_uuid =
      instance.get_sysvar_string("unexisting_variable");

  // The value was not modified
  EXPECT_TRUE(server_uuid.is_null());

  EXPECT_CALL(session, close());
  _session->close();
}

TEST_F(Instance_test, get_sysvar_boolean_existing_variable) {
  EXPECT_CALL(session, connect(_mysql_uri, _pwd.c_str()));
  _session->connect(_mysql_uri, _pwd.c_str());

  mysqlshdk::mysql::Instance instance(_session);

  Mock_result *result = new Mock_result();
  auto fake_result = result->add_result({"VARIABLE_NAME", "VARIABLE_VALUE"},
                                        {Type::VarString, Type::VarString});
  fake_result->add_row({"sql_warnings", "OFF"});
  // Passes the result to the session, ownership included.
  session.set_result(result);

  mysqlshdk::utils::nullable<bool> sql_warnings =
      instance.get_sysvar_bool("sql_warnings");

  // The value was not modified
  EXPECT_FALSE(sql_warnings.is_null());

  EXPECT_FALSE(*sql_warnings);

  EXPECT_CALL(session, close());
  _session->close();
}

TEST_F(Instance_test, get_sysvar_boolean_unexisting_variable) {
  EXPECT_CALL(session, connect(_mysql_uri, _pwd.c_str()));
  _session->connect(_mysql_uri, _pwd.c_str());

  Mock_result *result = new Mock_result();
  auto fake_result = result->add_result({"VARIABLE_NAME", "VARIABLE_VALUE"},
                                        {Type::VarString, Type::VarString});
  // Passes the result to the session, ownership included.
  session.set_result(result);

  mysqlshdk::mysql::Instance instance(_session);
  mysqlshdk::utils::nullable<bool> sql_warnings =
      instance.get_sysvar_bool("unexisting_variable");

  // The value was not modified
  EXPECT_TRUE(sql_warnings.is_null());

  EXPECT_CALL(session, close());
  _session->close();
}

TEST_F(Instance_test, get_sysvar_boolean_invalid_variable) {
  EXPECT_CALL(session, connect(_mysql_uri, _pwd.c_str()));
  _session->connect(_mysql_uri, _pwd.c_str());

  Mock_result *result = new Mock_result();
  auto fake_result = result->add_result({"VARIABLE_NAME", "VARIABLE_VALUE"},
                                        {Type::VarString, Type::VarString});
  fake_result->add_row({"server_uuid", "7042e17f-1f14-11e7-bc28-28d24455c64b"});
  // Passes the result to the session, ownership included.
  session.set_result(result);

  mysqlshdk::mysql::Instance instance(_session);
  EXPECT_ANY_THROW(instance.get_sysvar_bool("server_uuid"));

  EXPECT_CALL(session, close());
  _session->close();
}

TEST_F(Instance_test, get_sysvar_int_existing_variable) {
  EXPECT_CALL(session, connect(_mysql_uri, _pwd.c_str()));
  _session->connect(_mysql_uri, _pwd.c_str());

  Mock_result *result = new Mock_result();
  auto fake_result = result->add_result({"VARIABLE_NAME", "VARIABLE_VALUE"},
                                        {Type::VarString, Type::VarString});
  fake_result->add_row({"server_id", "0"});
  // Passes the result to the session, ownership included.
  session.set_result(result);

  mysqlshdk::mysql::Instance instance(_session);
  mysqlshdk::utils::nullable<int64_t> server_id =
      instance.get_sysvar_int("server_id");

  // The value was not modified
  EXPECT_FALSE(server_id.is_null());

  EXPECT_EQ(0, *server_id);

  EXPECT_CALL(session, close());
  _session->close();
}

TEST_F(Instance_test, get_sysvar_int_unexisting_variable) {
  EXPECT_CALL(session, connect(_mysql_uri, _pwd.c_str()));
  _session->connect(_mysql_uri, _pwd.c_str());

  Mock_result *result = new Mock_result();
  auto fake_result = result->add_result({"VARIABLE_NAME", "VARIABLE_VALUE"},
                                        {Type::VarString, Type::VarString});
  // Passes the result to the session, ownership included.
  session.set_result(result);

  mysqlshdk::mysql::Instance instance(_session);
  mysqlshdk::utils::nullable<int64_t> server_id =
      instance.get_sysvar_int("unexisting_variable");

  // The value was not modified
  EXPECT_TRUE(server_id.is_null());

  EXPECT_CALL(session, close());
  _session->close();
}

TEST_F(Instance_test, get_sysvar_int_invalid_variable) {
  EXPECT_CALL(session, connect(_mysql_uri, _pwd.c_str()));
  _session->connect(_mysql_uri, _pwd.c_str());

  Mock_result *result = new Mock_result();
  auto fake_result = result->add_result({"VARIABLE_NAME", "VARIABLE_VALUE"},
                                        {Type::VarString, Type::VarString});
  fake_result->add_row({"server_uuid", "7042e17f-1f14-11e7-bc28-28d24455c64b"});
  // Passes the result to the session, ownership included.
  session.set_result(result);

  mysqlshdk::mysql::Instance instance(_session);
  EXPECT_ANY_THROW(instance.get_sysvar_int("server_uuid"));

  EXPECT_CALL(session, close());
  _session->close();
}

TEST_F(Instance_test, get_system_variables) {
  EXPECT_CALL(session, connect(_mysql_uri, _pwd.c_str()));
  _session->connect(_mysql_uri, _pwd.c_str());

  Mock_result *result = new Mock_result();
  auto fake_result = result->add_result({"VARIABLE_NAME", "VARIABLE_VALUE"},
                                        {Type::VarString, Type::VarString});
  fake_result->add_row({"server_id", "0"});
  fake_result->add_row({"server_uuid", "7042e17f-1f14-11e7-bc28-28d24455c64b"});
  // Passes the result to the session, ownership included.
  session.set_result(result);

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

}  // namespace testing
