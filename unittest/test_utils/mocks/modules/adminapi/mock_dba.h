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

#ifndef UNITTEST_TEST_UTILS_MOCKS_MODULES_ADMINAPI_MOCK_DBA_H_
#define UNITTEST_TEST_UTILS_MOCKS_MODULES_ADMINAPI_MOCK_DBA_H_

#include <string>
#include <vector>
#include <utility>

#include "modules/adminapi/mod_dba.h"
#include "mysqlshdk/include/shellcore/base_session.h"
#include "mysqlshdk/include/shellcore/ishell_core.h"
#include "mysqlshdk/libs/utils/nullable.h"
#include "unittest/test_utils/mocks/gmock_clean.h"
#include "unittest/test_utils/mocks/modules/adminapi/mock_cluster.h"
#include "unittest/test_utils/mocks/modules/adminapi/mock_metadata_storage.h"
#include "unittest/test_utils/mocks/modules/adminapi/mock_provisioning_interface.h"

#define NO_MULTI_MASTER \
  {}
#define NO_FORCE \
  {}
#define NO_ADOPT_FROM_GR \
  {}
#define NO_CLEAN_READ_ONLY \
  {}
#define NO_REJOIN_LIST \
  {}
#define NO_REMOVE_LIST \
  {}
#define MULTI_MASTER_TRUE true
#define FORCE_TRUE true
#define ADOPT_FROM_GR_TRUE true
#define CLEAN_READ_ONLY_TRUE true
#define MULTI_MASTER_FALSE false
#define FORCE_FALSE false
#define ADOPT_FROM_GR_FALSE false
#define CLEAN_READ_ONLY_FALSE false

using mysqlshdk::utils::nullable;

namespace testing {
/**
 * Mock foe the Dba class.
 */
class Mock_dba : public mysqlsh::dba::Dba {
 public:
  Mock_dba() {
    ON_CALL(*this, _check_instance_configuration(_, _, _))
      .WillByDefault(Invoke(this, &Mock_dba::__check_instance_configuration));
  }
  void initialize(shcore::IShell_core *owner, bool chain_dba);

  MOCK_METHOD2(call, shcore::Value(const std::string &,
                                   const shcore::Argument_list &));

  MOCK_METHOD1(validate_instances_status_reboot_cluster,
               void(const shcore::Argument_list &args));

  MOCK_METHOD3(
      validate_instances_gtid_reboot_cluster,
      void(std::string *out_cluster_name,
           const shcore::Value::Map_type_ref &options,
           const std::shared_ptr<mysqlshdk::db::ISession> &instance_session));

  MOCK_METHOD2(get_replicaset_instances_status,
               std::vector<std::pair<std::string, std::string>>(
                   std::string *out_cluster_name,
                   const shcore::Value::Map_type_ref &options));

  MOCK_METHOD3(_check_instance_configuration,
      shcore::Value::Map_type_ref(
      const mysqlshdk::db::Connection_options &instance_def,
      const shcore::Value::Map_type_ref &options,
      bool allow_update));

  // Default action to propagate the call to the parent class
  shcore::Value::Map_type_ref __check_instance_configuration(
      const mysqlshdk::db::Connection_options &instance_def,
      const shcore::Value::Map_type_ref &options,
      bool allow_update) {
    return mysqlsh::dba::Dba::_check_instance_configuration(instance_def,
                                                            options,
                                                            allow_update);
  }

  shcore::Value create_mock_cluster(const std::string &name) {
    return shcore::Value::wrap<mysqlsh::dba::Cluster>(
        new Mock_cluster(name, _metadata_storage));
  }

  void expect_check_instance_configuration(const std::string &uri,
                                           const std::string &status);

  void expect_drop_metadata_schema(const nullable<bool> &force,
                                   const nullable<bool> &clean_read_only,
                                   bool succeed);

  void expect_configure_local_instance(
      const std::string &uri, const nullable<std::string> &cnf_path,
      const nullable<std::string> &admin_user,
      const nullable<std::string> &admin_password,
      const nullable<bool> &clean_read_only, bool succeed);

  void expect_create_cluster(const std::string &name,
                             const nullable<bool> &multi_master,
                             const nullable<bool> &force,
                             const nullable<bool> &adopt_from_gr,
                             const nullable<bool> &clean_read_only,
                             bool succeed);

  void expect_reboot_cluster(const nullable<std::string> &name,
                             const shcore::Value::Array_type_ref remove,
                             const shcore::Value::Array_type_ref rejoin,
                             const nullable<bool> &clean_read_only,
                             bool succeed);

  StrictMock<Mock_metadata_storage> &get_metadata() { return _mock_metadata; }
  StrictMock<Mock_provisioning_interface> &get_mpi() { return _mock_mpi; }

 private:
  StrictMock<Mock_metadata_storage> _mock_metadata;
  StrictMock<Mock_provisioning_interface> _mock_mpi;
};

}  // namespace testing

#endif  // UNITTEST_TEST_UTILS_MOCKS_MODULES_ADMINAPI_MOCK_DBA_H_
