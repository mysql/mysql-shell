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

  session.expect_query("show variables where `variable_name` in"
                       " ('server_uuid')").
    then_return({
      {
        "show variables where `variable_name` in"
        " ('server_uuid')",
        { "Variable_name", "Value" },
        { Type::VarString, Type::VarString },
        {
          { "server_uuid", "891a2c04-1cc7-11e7-8323-00059a3c7a00" }
        }
      }
  });

  mysqlshdk::utils::nullable<std::string> server_uuid =
      instance.get_sysvar_string("server_uuid");

  // The value was not modified
  EXPECT_FALSE(server_uuid.is_null());

  EXPECT_EQ("891a2c04-1cc7-11e7-8323-00059a3c7a00", *server_uuid);

  EXPECT_CALL(session, close());
  _session->close();
}

TEST_F(Instance_test, get_sysvar_string_unexisting_variable) {
  EXPECT_CALL(session, connect(_mysql_uri, _pwd.c_str()));
  _session->connect(_mysql_uri, _pwd.c_str());

  session.expect_query("show variables where `variable_name` in"
                       " ('unexisting_variable')").
    then_return({
      {
        "show variables where `variable_name` in"
        " ('unexisting_variable')",
        { "Variable_name", "Value" },
        { Type::VarString, Type::VarString },
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
  EXPECT_CALL(session, connect(_mysql_uri, _pwd.c_str()));
  _session->connect(_mysql_uri, _pwd.c_str());

  session.expect_query("show variables where `variable_name` in"
                       " ('sql_warnings')").
    then_return({
      {
        "show variables where `variable_name` in"
        " ('sql_warnings')",
        { "Variable_name", "Value" },
        { Type::VarString, Type::VarString },
        {
          { "sql_warnings", "OFF" }
        }
      }
  });

  mysqlshdk::mysql::Instance instance(_session);
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

  session.expect_query("show variables where `variable_name` in"
                       " ('unexisting_variable')").
    then_return({
      {
        "show variables where `variable_name` in"
        " ('unexisting_variable')",
        { "Variable_name", "Value" },
        { Type::VarString, Type::VarString },
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
  EXPECT_CALL(session, connect(_mysql_uri, _pwd.c_str()));
  _session->connect(_mysql_uri, _pwd.c_str());

  session.expect_query("show variables where `variable_name` in"
                       " ('server_uuid')").
    then_return({
      {
        "show variables where `variable_name` in"
        " ('server_uuid')",
        { "Variable_name", "Value" },
        { Type::VarString, Type::VarString },
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
  EXPECT_CALL(session, connect(_mysql_uri, _pwd.c_str()));
  _session->connect(_mysql_uri, _pwd.c_str());

  session.expect_query("show variables where `variable_name` in"
                       " ('server_id')").
    then_return({
      {
        "show variables where `variable_name` in"
        " ('server_id')",
        { "Variable_name", "Value" },
        { Type::VarString, Type::VarString },
        {
          { "server_id", "0" }
        }
      }
  });

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

  session.expect_query("show variables where `variable_name` in"
                       " ('unexisting_variable')").
    then_return({
      {
        "show variables where `variable_name` in"
        " ('unexisting_variable')",
        { "Variable_name", "Value" },
        { Type::VarString, Type::VarString },
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
  EXPECT_CALL(session, connect(_mysql_uri, _pwd.c_str()));
  _session->connect(_mysql_uri, _pwd.c_str());

  session.expect_query("show variables where `variable_name` in"
                       " ('server_uuid')").
    then_return({
      {
        "show variables where `variable_name` in"
        " ('server_uuid')",
        { "Variable_name", "Value" },
        { Type::VarString, Type::VarString },
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

TEST_F(Instance_test, get_system_variables) {
  EXPECT_CALL(session, connect(_mysql_uri, _pwd.c_str()));
  _session->connect(_mysql_uri, _pwd.c_str());

  session.expect_query("show variables where `variable_name` in"
                       " ('server_id', 'server_uuid', 'unexisting_variable')").
    then_return({
      {
        "show variables where `variable_name` in"
        " ('server_id', 'server_uuid', 'unexisting_variable')",
        { "Variable_name", "Value" },
        { Type::VarString, Type::VarString },
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

}  // namespace testing
