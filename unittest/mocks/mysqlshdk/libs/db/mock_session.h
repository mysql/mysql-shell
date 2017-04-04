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

#ifndef _UNITTEST_MOCKS_CORELIBS_DB_MOCK_SESSION_H
#define _UNITTEST_MOCKS_CORELIBS_DB_MOCK_SESSION_H

#include "mysqlshdk/libs/db/session.h"

#include <gmock/gmock.h>
#include <memory>

namespace testing {
/**
 * Mock for a Session object
 *
 * Simple call expectations and return values can be defined with:
 *
 * auto sresult = result.add_result("@@server_id", {MYSQL_TYPE_INT24});
 * auto row = sresult->add_row("1");
 * EXPECT_CALL(session, query("SELECT @@server_id", false)).WillOnce(Return(&result));
 *
 * Where:
 *   - First parameter is this session object
 *   - Second parameter is the function and parameters that is expected to be called
 *   - After closing the EXPECT_CALL() some actions can be defined to return specific results
 *     Keep in mind that the returned data must match the return type of the function called
 *
 * Look at Mock_result to see how to to create a fake result to be returned.
 */
class Mock_session : public mysqlshdk::db::ISession {
public:
  MOCK_METHOD2(connect, void(const std::string& URI, const char *password));
  MOCK_METHOD7(connect, void(const std::string &host, int port, const std::string &socket,
                              const std::string &user, const std::string &password, const std::string &schema,
                              const mysqlshdk::utils::Ssl_info& ssl_info));

  // Execution
  virtual std::unique_ptr<mysqlshdk::db::IResult> query(const std::string& sql, bool buffered) { return std::move(_result); }
  MOCK_METHOD1(execute, void(const std::string& sql));
  MOCK_METHOD0(start_transaction, void());
  MOCK_METHOD0(commit, void());
  MOCK_METHOD0(rollback, void());
  MOCK_METHOD0(get_ssl_cipher, const char *());

  // Disconnection
  MOCK_METHOD0(close, void());

  void set_result(mysqlshdk::db::IResult *result);

  // Exception Simulation
  static mysqlshdk::db::IResult* throw_exception_on_sql() {
    throw std::runtime_error("Exception while executing SQL");
    return nullptr;
  }

private:
  std::unique_ptr<mysqlshdk::db::IResult> _result;
};
}

#endif // MOCK_SESSION_H
