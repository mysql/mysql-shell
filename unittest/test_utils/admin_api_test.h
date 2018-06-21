/* Copyright (c) 2017, 2018 Oracle and/or its affiliates. All rights reserved.

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

#ifndef UNITTEST_TEST_UTILS_ADMIN_API_TEST_H_
#define UNITTEST_TEST_UTILS_ADMIN_API_TEST_H_

#include <memory>
#include <string>
#include <vector>
#include "modules/adminapi/mod_dba_common.h"
#include "mysqlshdk/libs/db/mysql/session.h"
#include "mysqlshdk/libs/utils/nullable.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "src/interactive/interactive_global_dba.h"
#include "unittest/test_utils.h"
#include "unittest/test_utils/shell_test_wrapper.h"

using mysqlshdk::mysql::Instance;
using mysqlshdk::mysql::Var_qualifier;
using mysqlshdk::utils::nullable;

namespace tests {
/**
 * \ingroup UTFramework
 * Helper class for Admin API tests that use the Server Mock.
 *
 * This class contains helper functions that will add sql statements and data
 * that should be processed by the tests that use a Server Mock.
 * \todo The functions in this class must be documented.
 */
class Admin_api_test : public Shell_core_test_wrapper {
 public:
  virtual void SetUp();
  static std::shared_ptr<mysqlshdk::db::ISession> create_session(
      int port, std::string user = "root") {
    auto session = mysqlshdk::db::mysql::Session::create();

    auto connection_options = shcore::get_connection_options(
        user + ":root@localhost:" + std::to_string(port), false);
    session->connect(connection_options);

    return session;
  }

  static void SetUpSampleCluster(const char *context = nullptr);
  static void TearDownSampleCluster(const char *context = nullptr);

 public:
  std::shared_ptr<mysqlshdk::db::ISession> get_classic_session() {
    return _interactive_shell->shell_context()
        ->get_dev_session()
        ->get_core_session();
  }

  std::shared_ptr<mysqlshdk::db::ISession> create_local_session(int port) {
    mysqlshdk::db::Connection_options session_args;
    session_args.set_scheme("mysql");
    session_args.set_host("localhost");
    session_args.set_port(port);
    session_args.set_user("user");
    session_args.set_password("");

    auto session = mysqlshdk::db::mysql::Session::create();
    session->connect(session_args);
    return session;
  }
  static std::shared_ptr<mysqlsh::dba::ReplicaSet> _replicaset;
  static std::string group_name;
  static std::string uuid_1;
  static std::string uuid_2;
  static std::string uuid_3;
};
}  // namespace tests

#endif  // UNITTEST_TEST_UTILS_ADMIN_API_TEST_H_
