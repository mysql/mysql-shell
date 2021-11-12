/*
 * Copyright (c) 2018, 2021, Oracle and/or its affiliates.
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

#ifndef MODULES_ADMINAPI_COMMON_PRECONDITIONS_H_
#define MODULES_ADMINAPI_COMMON_PRECONDITIONS_H_

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "modules/adminapi/common/cluster_types.h"
#include "modules/adminapi/common/common.h"
#include "mysqlshdk/libs/db/session.h"
#include "mysqlshdk/libs/mysql/instance.h"
#include "mysqlshdk/libs/utils/enumset.h"
#include "mysqlshdk/libs/utils/nullable.h"

namespace mysqlsh {
namespace dba {

class Instance;
class MetadataStorage;

// The AdminAPI maximum supported MySQL Server version
const mysqlshdk::utils::Version k_max_adminapi_server_version =
    mysqlshdk::utils::Version("8.1");

// The AdminAPI minimum supported MySQL Server version
const mysqlshdk::utils::Version k_min_adminapi_server_version =
    mysqlshdk::utils::Version("5.7");

// Specific minimum versions for InnoDB Cluster, ReplicaSet and ClusterSet
const mysqlshdk::utils::Version k_min_gr_version =
    mysqlshdk::utils::Version("5.7");
const mysqlshdk::utils::Version k_min_ar_version =
    mysqlshdk::utils::Version("8.0");
const mysqlshdk::utils::Version k_min_cs_version =
    mysqlshdk::utils::Version("8.0.27");

const Cluster_global_status_mask kClusterGlobalStateAnyOk =
    Cluster_global_status_mask(Cluster_global_status::OK)
        .set(Cluster_global_status::OK_NOT_REPLICATING)
        .set(Cluster_global_status::OK_NOT_CONSISTENT)
        .set(Cluster_global_status::OK_MISCONFIGURED);

const Cluster_global_status_mask kClusterGlobalStateAnyOkorNotOk =
    Cluster_global_status_mask(Cluster_global_status::OK)
        .set(Cluster_global_status::OK_NOT_REPLICATING)
        .set(Cluster_global_status::OK_NOT_CONSISTENT)
        .set(Cluster_global_status::OK_MISCONFIGURED)
        .set(Cluster_global_status::NOT_OK);

const Cluster_global_status_mask kClusterGlobalStateAny =
    Cluster_global_status_mask::any();

class Precondition_checker {
 public:
  Precondition_checker(const std::shared_ptr<MetadataStorage> &metadata,
                       const std::shared_ptr<Instance> &group_server);
  Cluster_check_info check_preconditions(
      const std::string &function_name,
      const Function_availability *custom_func_avail = nullptr);
  virtual ~Precondition_checker() {}

  static const std::map<std::string, Function_availability> s_preconditions;

 private:
#ifdef FRIEND_TEST
  FRIEND_TEST(Preconditions, check_session);
  FRIEND_TEST(Preconditions, check_instance_configuration_preconditions_errors);
  FRIEND_TEST(Preconditions, check_instance_configuration_preconditions);
  FRIEND_TEST(Preconditions,
              check_managed_instance_status_preconditions_errors);
  FRIEND_TEST(Preconditions, check_managed_instance_status_preconditions);
  FRIEND_TEST(Preconditions, check_quorum_state_preconditions_errors);
  FRIEND_TEST(Preconditions, check_quorum_state_preconditions);
  FRIEND_TEST(Preconditions, check_cluster_set_preconditions_errors);
  FRIEND_TEST(Preconditions, check_cluster_set_preconditions);
  FRIEND_TEST(Preconditions, check_cluster_fenced_preconditions);
#endif

  void check_session() const;
  metadata::State check_metadata_state_actions(
      const std::string &function_name);
  void check_instance_configuration_preconditions(
      mysqlsh::dba::TargetType::Type instance_type, int allowed_types) const;
  void check_managed_instance_status_preconditions(
      mysqlsh::dba::ManagedInstance::State instance_state,
      int allowed_states) const;
  void check_quorum_state_preconditions(
      mysqlsh::dba::ReplicationQuorum::State instance_quorum_state,
      mysqlsh::dba::ReplicationQuorum::State allowed_states) const;
  void check_cluster_set_preconditions(
      mysqlsh::dba::Cluster_global_status_mask allowed_states);
  void check_cluster_fenced_preconditions(bool allowed_on_fenced);

 protected:
  virtual Cluster_global_status get_cluster_global_state();
  virtual Cluster_status get_cluster_status();

  std::shared_ptr<MetadataStorage> m_metadata;
  std::shared_ptr<Instance> m_group_server;
};

// NOTE: BUG#30628479 is applicable to all the API functions and the root
// cause is that several instances of the Metadata class are created during a
// function execution. To solve this, all the API functions should create a
// single instance of the Metadata class and pass it down through the chain
// call.
Cluster_check_info check_function_preconditions(
    const std::string &function_name,
    const std::shared_ptr<MetadataStorage> &metadata,
    const std::shared_ptr<Instance> &group_server,
    const Function_availability *custom_func_avail = nullptr);

}  // namespace dba
}  // namespace mysqlsh

#endif  // MODULES_ADMINAPI_COMMON_PRECONDITIONS_H_
