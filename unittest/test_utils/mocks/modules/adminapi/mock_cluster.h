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

#ifndef UNITTEST_TEST_UTILS_MOCKS_MODULES_ADMINAPI_MOCK_CLUSTER_H_
#define UNITTEST_TEST_UTILS_MOCKS_MODULES_ADMINAPI_MOCK_CLUSTER_H_

#include <string>
#include <vector>

#include "modules/adminapi/mod_dba_cluster.h"
#include "modules/adminapi/mod_dba_metadata_storage.h"
#include "unittest/test_utils/mocks/gmock_clean.h"

namespace testing {

/**
 * Mock for the dba.Cluster class
 */
class Mock_cluster : public mysqlsh::dba::Cluster {
 public:
  Mock_cluster() {}
  Mock_cluster(const std::string &name,
               std::shared_ptr<mysqlsh::dba::MetadataStorage> metadata_storage)
      : Cluster(name, metadata_storage) {
    _default_replica_set.reset(new mysqlsh::dba::ReplicaSet(
        name, mysqlsh::dba::ReplicaSet::kTopologyPrimaryMaster,
        metadata_storage));
    _default_replica_set->set_id(1);
  }

  void initialize(const std::string& name,
    std::shared_ptr<mysqlsh::dba::MetadataStorage> metadata_storage) {
    _name = name;
    _metadata_storage = metadata_storage;
    _session = _metadata_storage->get_session();
  }

  MOCK_METHOD2(call, shcore::Value(const std::string &,
                                   const shcore::Argument_list &));
};

}  // namespace testing

#endif  // UNITTEST_TEST_UTILS_MOCKS_MODULES_ADMINAPI_MOCK_CLUSTER_H_
