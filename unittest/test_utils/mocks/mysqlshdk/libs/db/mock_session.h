/*
 * Copyright (c) 2017, 2018, Oracle and/or its affiliates. All rights reserved.
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

#ifndef UNITTEST_MOCKS_MYSQLSHDK_LIBS_DB_MOCK_SESSION_H_
#define UNITTEST_MOCKS_MYSQLSHDK_LIBS_DB_MOCK_SESSION_H_

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "mysqlshdk/libs/db/session.h"
#include "unittest/test_utils/mocks/gmock_clean.h"
#include "unittest/test_utils/mocks/mysqlshdk/libs/db/mock_result.h"

namespace testing {
/**
 * Mock for a Session object
 *
 * Simple call expectations and return values can be defined with:
 *
 * auto sresult = result.add_result("@@server_id", {MYSQL_TYPE_INT24});
 * auto row = sresult->add_row("1");
 * EXPECT_CALL(session, query("SELECT @@server_id",
 * false)).WillOnce(Return(&result));
 *
 * Where:
 *   - First parameter is this session object
 *   - Second parameter is the function and parameters that is expected to be
 * called - After closing the EXPECT_CALL() some actions can be defined to
 * return specific results Keep in mind that the returned data must match the
 * return type of the function called
 *
 * Look at Mock_result to see how to to create a fake result to be returned.
 */
class Mock_session : public mysqlshdk::db::ISession {
 public:
  MOCK_METHOD1(
      connect,
      void(const mysqlshdk::db::Connection_options &connection_options));

  // Execution
  virtual std::shared_ptr<mysqlshdk::db::IResult> query(const std::string &sql,
                                                        bool buffered);
  MOCK_METHOD1(execute, void(const std::string &sql));
  MOCK_METHOD0(start_transaction, void());
  MOCK_METHOD0(commit, void());
  MOCK_METHOD0(rollback, void());
  MOCK_CONST_METHOD0(get_connection_id, uint64_t());
  MOCK_CONST_METHOD0(get_ssl_cipher, const char *());
  MOCK_CONST_METHOD0(get_connection_options,
                     const mysqlshdk::db::Connection_options &());
  MOCK_CONST_METHOD0(is_open, bool());
  MOCK_CONST_METHOD0(get_server_version, mysqlshdk::utils::Version());

  // Disconnection
  MOCK_METHOD0(close, void());

  // Exception Simulation
  Mock_session &expect_query(const std::string &query);
  void then_return(const std::vector<Fake_result_data> &data);
  void then_throw();

 private:
  size_t _last_query;
  std::vector<std::string> _queries;
  std::map<std::string, std::shared_ptr<mysqlshdk::db::IResult>> _results;
  std::vector<bool> _throws;
};
}  // namespace testing

#endif  // UNITTEST_MOCKS_MYSQLSHDK_LIBS_DB_MOCK_SESSION_H_
