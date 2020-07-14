/*
 * Copyright (c) 2020, Oracle and/or its affiliates.
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

#ifndef UNITTEST_TEST_UTILS_MOCKS_MYSQLSHDK_LIBS_DB_MOCK_MYSQL_SESSION_H_
#define UNITTEST_TEST_UTILS_MOCKS_MYSQLSHDK_LIBS_DB_MOCK_MYSQL_SESSION_H_

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "mysqlshdk/libs/db/mysql/session.h"
#include "unittest/test_utils/mocks/gmock_clean.h"
#include "unittest/test_utils/mocks/mysqlshdk/libs/db/mock_result.h"
#include "unittest/test_utils/mocks/mysqlshdk/libs/db/mock_session.h"

namespace testing {

class Mock_mysql_session : public mysqlshdk::db::mysql::Session,
                           public Mock_session_common {
 public:
  MOCK_METHOD1(
      connect,
      void(const mysqlshdk::db::Connection_options &connection_options));

  // Execution
  std::shared_ptr<mysqlshdk::db::IResult> querys(const char *sql, size_t len,
                                                 bool buffered) override {
    return Mock_session_common::do_querys(sql, len, buffered);
  }

  MOCK_METHOD2(executes, void(const char *, size_t));
  MOCK_METHOD1(execute, void(const std::string &));
  MOCK_METHOD0(start_transaction, void());
  MOCK_METHOD0(commit, void());
  MOCK_METHOD0(rollback, void());
  MOCK_CONST_METHOD0(get_connection_id, uint64_t());
  MOCK_CONST_METHOD0(get_ssl_cipher, const char *());
  MOCK_CONST_METHOD0(get_connection_options,
                     const mysqlshdk::db::Connection_options &());
  MOCK_CONST_METHOD0(is_open, bool());
  MOCK_CONST_METHOD0(get_server_version, mysqlshdk::utils::Version());

  // Error handling
  MOCK_CONST_METHOD0(get_last_error, mysqlshdk::db::Error *());

  // Disconnection
  MOCK_METHOD0(close, void());

  // Exception Simulation
  Mock_mysql_session &expect_query(const std::string &query) {
    Mock_session_common::do_expect_query(query);
    return *this;
  }
};

}  // namespace testing

#endif  // UNITTEST_TEST_UTILS_MOCKS_MYSQLSHDK_LIBS_DB_MOCK_MYSQL_SESSION_H_
