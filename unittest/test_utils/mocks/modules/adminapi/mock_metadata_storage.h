/*
 * Copyright (c) 2017, Oracle and/or its affiliates. All rights reserved.
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

#ifndef UNITTEST_TEST_UTILS_MOCKS_MODULES_ADMINAPI_MOCK_METADATA_STORAGE_H_
#define UNITTEST_TEST_UTILS_MOCKS_MODULES_ADMINAPI_MOCK_METADATA_STORAGE_H_

#include <string>
#include "modules/adminapi/mod_dba_cluster.h"
#include "modules/adminapi/mod_dba_metadata_storage.h"
#include "modules/adminapi/mod_dba_replicaset.h"
#include "modules/mod_mysql_session.h"
#include "unittest/test_utils/mocks/gmock_clean.h"

namespace testing {
/**
  * Mock for the MetadataStorage class.
  */
class Mock_metadata_storage : public mysqlsh::dba::MetadataStorage {
 public:
  MOCK_METHOD0(metadata_schema_exists, bool());
  MOCK_METHOD0(create_metadata_schema, void());
  MOCK_METHOD0(drop_metadata_schema, void());
  MOCK_METHOD1(get_cluster_id, uint64_t(const std::string &));
  MOCK_METHOD1(get_cluster_id, uint64_t(uint64_t));
  MOCK_METHOD1(cluster_exists, bool(const std::string &));
  MOCK_METHOD1(insert_cluster,
               void(const std::shared_ptr<mysqlsh::dba::Cluster> &));
  MOCK_METHOD3(insert_replica_set,
               void(std::shared_ptr<mysqlsh::dba::ReplicaSet>, bool, bool));
  MOCK_METHOD3(insert_host, uint32_t(const std::string &, const std::string &,
                                     const std::string &));
  MOCK_METHOD1(insert_instance,
               void(const mysqlsh::dba::Instance_definition &));
  MOCK_METHOD1(remove_instance, void(const std::string &instance_address));
  MOCK_METHOD1(drop_cluster, void(const std::string &));
  MOCK_METHOD1(cluster_has_default_replicaset_only, bool(const std::string &));
  MOCK_METHOD1(is_cluster_empty, bool(uint64_t));
  MOCK_METHOD1(drop_replicaset, void(uint64_t));
  MOCK_METHOD1(disable_replicaset, void(uint64_t rs_id));
  MOCK_METHOD1(is_replicaset_active, bool(uint64_t rs_id));
  MOCK_METHOD1(get_replicaset_group_name, std::string(uint64_t rs_id));
  MOCK_METHOD2(set_replicaset_group_name,
               void(std::shared_ptr<mysqlsh::dba::ReplicaSet> replicaset,
                    const std::string &group_name));

  MOCK_METHOD1(get_cluster, std::shared_ptr<mysqlsh::dba::Cluster>(
                                const std::string &cluster_name));
  MOCK_METHOD0(get_default_cluster, std::shared_ptr<mysqlsh::dba::Cluster>());
  MOCK_METHOD0(has_default_cluster, bool());

  MOCK_METHOD1(get_replicaset,
               std::shared_ptr<mysqlsh::dba::ReplicaSet>(uint64_t rs_id));
  MOCK_METHOD1(is_replicaset_empty, bool(uint64_t rs_id));
  MOCK_METHOD2(is_instance_on_replicaset,
               bool(uint64_t rs_id, const std::string &address));
  MOCK_CONST_METHOD1(get_replicaset_count, uint64_t(uint64_t rs_id));

  MOCK_METHOD1(get_seed_instance, std::string(uint64_t rs_id));
  MOCK_METHOD1(get_replicaset_instances,
               std::shared_ptr<shcore::Value::Array_type>(uint64_t rs_id));
  MOCK_METHOD1(get_replicaset_online_instances,
               std::shared_ptr<shcore::Value::Array_type>(uint64_t rs_id));

  MOCK_METHOD1(get_instance, mysqlsh::dba::Instance_definition(
                                 const std::string &instance_address));

  MOCK_METHOD2(create_repl_account,
               void(std::string &username, std::string &password));

  MOCK_CONST_METHOD0(get_session, std::shared_ptr<mysqlshdk::db::ISession>());

  MOCK_METHOD1(set_session,
               void(std::shared_ptr<mysqlshdk::db::ISession> session));

  MOCK_CONST_METHOD3(execute_sql,
                     std::shared_ptr<mysqlsh::mysql::ClassicResult>(
                         const std::string &sql, bool retry,
                         const std::string &log_sql));

  MOCK_METHOD4(create_account,
               void(const std::string &username, const std::string &password,
                    const std::string &hostname, bool password_hashed));

  MOCK_METHOD0(start_transaction, void());
  MOCK_METHOD0(commit, void());
  MOCK_METHOD0(rollback, void());
};

}  // namespace testing

#endif  // UNITTEST_TEST_UTILS_MOCKS_MODULES_ADMINAPI_MOCK_METADATA_STORAGE_H_
