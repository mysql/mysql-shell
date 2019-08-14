/*
 * Copyright (c) 2019, Oracle and/or its affiliates. All rights reserved.
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

#ifndef UNITTEST_TEST_UTILS_MOCKS_MYSQLSHDK_LIBS_MYSQL_MOCK_INSTANCE_H_
#define UNITTEST_TEST_UTILS_MOCKS_MYSQLSHDK_LIBS_MYSQL_MOCK_INSTANCE_H_

#include <map>
#include <memory>
#include <string>
#include <tuple>
#include <vector>

#include "mysqlshdk/libs/mysql/instance.h"
#include "mysqlshdk/libs/utils/nullable.h"
#include "unittest/test_utils/mocks/gmock_clean.h"

namespace mysqlshdk {
namespace mysql {
class Mock_instance : public mysqlshdk::mysql::IInstance {
 public:
  MOCK_CONST_METHOD0(descr, std::string());
  MOCK_CONST_METHOD0(get_canonical_hostname, std::string());
  MOCK_CONST_METHOD0(get_canonical_port, int());
  MOCK_CONST_METHOD0(get_canonical_address, std::string());
  MOCK_CONST_METHOD0(get_uuid, const std::string &());
  MOCK_CONST_METHOD0(get_group_name, const std::string &());

  MOCK_METHOD0(refresh, void());

  MOCK_CONST_METHOD1(
      query, std::shared_ptr<mysqlshdk::db::IResult>(const std::string &));

  MOCK_METHOD1(cache_global_sysvars, void(bool));
  MOCK_CONST_METHOD1(
      get_cached_global_sysvar,
      mysqlshdk::utils::nullable<std::string>(const std::string &));
  MOCK_CONST_METHOD1(get_cached_global_sysvar_as_bool,
                     mysqlshdk::utils::nullable<bool>(const std::string &));

  MOCK_CONST_METHOD2(get_sysvar_bool,
                     mysqlshdk::utils::nullable<bool>(const std::string &,
                                                      const Var_qualifier));
  MOCK_CONST_METHOD2(get_sysvar_string,
                     mysqlshdk::utils::nullable<std::string>(
                         const std::string &, const Var_qualifier));
  MOCK_CONST_METHOD2(get_sysvar_int,
                     mysqlshdk::utils::nullable<int64_t>(const std::string &,
                                                         const Var_qualifier));
  MOCK_CONST_METHOD3(set_sysvar, void(const std::string &, const std::string &,
                                      const Var_qualifier));
  MOCK_CONST_METHOD3(set_sysvar, void(const std::string &, const int64_t,
                                      const Var_qualifier));
  MOCK_CONST_METHOD3(set_sysvar, void(const std::string &, const bool,
                                      const Var_qualifier));
  MOCK_CONST_METHOD2(set_sysvar_default,
                     void(const std::string &, const Var_qualifier));
  MOCK_CONST_METHOD1(has_variable_compiled_value, bool(const std::string &));
  MOCK_CONST_METHOD0(is_performance_schema_enabled, bool());

  MOCK_CONST_METHOD1(is_read_only, bool(bool super));
  MOCK_CONST_METHOD0(get_version, mysqlshdk::utils::Version());

  MOCK_CONST_METHOD2(
      get_system_variables_like,
      std::map<std::string, mysqlshdk::utils::nullable<std::string>>(
          const std::string &, const Var_qualifier));

  MOCK_CONST_METHOD0(get_session, std::shared_ptr<mysqlshdk::db::ISession>());
  MOCK_CONST_METHOD0(close_session, void());
  MOCK_CONST_METHOD1(install_plugin, void(const std::string &));
  MOCK_CONST_METHOD1(uninstall_plugin, void(const std::string &));
  MOCK_CONST_METHOD1(get_plugin_status, mysqlshdk::utils::nullable<std::string>(
                                            const std::string &));
  MOCK_CONST_METHOD5(
      create_user,
      void(const std::string &, const std::string &, const std::string &,
           const std::vector<std::tuple<std::string, std::string, bool>> &,
           bool));
  MOCK_CONST_METHOD3(drop_user,
                     void(const std::string &, const std::string &, bool));
  MOCK_CONST_METHOD0(get_connection_options,
                     mysqlshdk::db::Connection_options());
  MOCK_CONST_METHOD2(get_current_user, void(std::string *, std::string *));
  MOCK_CONST_METHOD2(user_exists,
                     bool(const std::string &, const std::string &));
  MOCK_CONST_METHOD3(set_user_password,
                     void(const std::string &, const std::string &,
                          const std::string &));
  //   MOCK_CONST_METHOD2(get_user_privileges,
  //                      std::unique_ptr<mysqlshdk::mysql::User_privileges>(
  //                          const std::string &, const std::string &));
  std::unique_ptr<mysqlshdk::mysql::User_privileges> get_user_privileges(
      const std::string &user, const std::string &host) const {
    // gmock can't handle noncopyable return args... otoh we don't need this
    // method for now
    return {};
  }
  MOCK_CONST_METHOD0(is_set_persist_supported,
                     mysqlshdk::utils::nullable<bool>());
  MOCK_CONST_METHOD1(
      get_persisted_value,
      mysqlshdk::utils::nullable<std::string>(const std::string &));

  MOCK_CONST_METHOD0(get_fence_sysvars, std::vector<std::string>());

  MOCK_METHOD1(suppress_binary_log, void(bool));

  MOCK_CONST_METHOD2(query, std::shared_ptr<mysqlshdk::db::IResult>(
                                const std::string &, bool));

  MOCK_CONST_METHOD1(execute, void(const std::string &));
};
}  // namespace mysql
}  // namespace mysqlshdk

#endif  // UNITTEST_TEST_UTILS_MOCKS_MYSQLSHDK_LIBS_MYSQL_MOCK_INSTANCE_H_
