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

#ifndef UNITTEST_TEST_UTILS_MOCKS_MODULES_ADMINAPI_MOCK_PROVISIONING_INTERFACE_H_
#define UNITTEST_TEST_UTILS_MOCKS_MODULES_ADMINAPI_MOCK_PROVISIONING_INTERFACE_H_

#include <string>
#include "modules/adminapi/mod_dba_provisioning_interface.h"
#include "unittest/test_utils/mocks/gmock_clean.h"

namespace testing {

class Mock_provisioning_interface : public mysqlsh::dba::ProvisioningInterface {
 public:
  MOCK_METHOD4(check,
               int(const mysqlshdk::db::Connection_options &connection_options,
                   const std::string &cnfpath, bool update,
                   shcore::Value::Array_type_ref *errors));

  MOCK_METHOD8(create_sandbox,
               int(int port, int portx, const std::string &sandbox_dir,
                   const std::string &password,
                   const shcore::Value &mycnf_options, bool start,
                   bool ignore_ssl_error,
                   shcore::Value::Array_type_ref *errors));
  MOCK_METHOD3(delete_sandbox, int(int port, const std::string &sandbox_dir,
                                   shcore::Value::Array_type_ref *errors));
  MOCK_METHOD3(kill_sandbox, int(int port, const std::string &sandbox_dir,
                                 shcore::Value::Array_type_ref *errors));
  MOCK_METHOD4(stop_sandbox, int(int port, const std::string &sandbox_dir,
                                 const std::string &password,
                                 shcore::Value::Array_type_ref *errors));
  MOCK_METHOD3(start_sandbox, int(int port, const std::string &sandbox_dir,
                                  shcore::Value::Array_type_ref *errors));
  MOCK_METHOD9(start_replicaset,
               int(const mysqlshdk::db::Connection_options &instance,
                   const std::string &repl_user,
                   const std::string &super_user_password,
                   const std::string &repl_user_password, bool multi_master,
                   const std::string &ssl_mode, const std::string &ip_whitelist,
                   const std::string &group_name,
                   shcore::Value::Array_type_ref *errors));
  MOCK_METHOD10(
      join_replicaset,
      int(const mysqlshdk::db::Connection_options &instance,
          const mysqlshdk::db::Connection_options &peer,
          const std::string &repl_user, const std::string &super_user_password,
          const std::string &repl_user_password, const std::string &ssl_mode,
          const std::string &ip_whitelist, const std::string &gr_group_seeds,
          bool skip_rpl_user, shcore::Value::Array_type_ref *errors));
  MOCK_METHOD2(leave_replicaset,
               int(const mysqlshdk::db::Connection_options &connection_options,
                   shcore::Value::Array_type_ref *errors));

  MOCK_METHOD1(set_verbose, void(int verbose));
  MOCK_METHOD0(get_verbose, int());
};

}  // namespace testing

#endif  // UNITTEST_TEST_UTILS_MOCKS_MODULES_ADMINAPI_MOCK_PROVISIONING_INTERFACE_H_
