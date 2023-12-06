/*
 * Copyright (c) 2018, 2023, Oracle and/or its affiliates.
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

namespace mysqlsh {
namespace dba {

class Instance;
class MetadataStorage;

// Specific minimum versions for InnoDB Cluster, ReplicaSet and ClusterSet

class Precondition_checker {
 public:
  static const mysqlshdk::utils::Version k_max_adminapi_server_version;
  static const mysqlshdk::utils::Version k_min_adminapi_server_version;
  static const mysqlshdk::utils::Version k_deprecated_adminapi_server_version;
  static const mysqlshdk::utils::Version k_min_gr_version;
  static const mysqlshdk::utils::Version k_min_ar_version;
  static const mysqlshdk::utils::Version k_min_cs_version;
  static const mysqlshdk::utils::Version k_min_rr_version;

  static constexpr Cluster_global_status_mask kClusterGlobalStateAnyOk{
      Cluster_global_status::OK, Cluster_global_status::OK_NOT_REPLICATING,
      Cluster_global_status::OK_NOT_CONSISTENT,
      Cluster_global_status::OK_MISCONFIGURED};

  static constexpr Cluster_global_status_mask kClusterGlobalStateAnyOkorNotOk{
      Cluster_global_status::OK, Cluster_global_status::OK_NOT_REPLICATING,
      Cluster_global_status::OK_NOT_CONSISTENT,
      Cluster_global_status::OK_MISCONFIGURED, Cluster_global_status::NOT_OK};

  static constexpr Cluster_global_status_mask
      kClusterGlobalStateAnyOkorInvalidated{
          Cluster_global_status::OK, Cluster_global_status::OK_NOT_REPLICATING,
          Cluster_global_status::OK_NOT_CONSISTENT,
          Cluster_global_status::OK_MISCONFIGURED,
          Cluster_global_status::INVALIDATED};

  static constexpr Cluster_global_status_mask kClusterGlobalStateAny =
      Cluster_global_status_mask::any();

 public:
  Precondition_checker(const std::shared_ptr<MetadataStorage> &metadata,
                       const std::shared_ptr<Instance> &group_server,
                       bool primary_available);
  Cluster_check_info check_preconditions(
      const std::string &function_name,
      const Function_availability *custom_func_avail = nullptr);
  virtual ~Precondition_checker() = default;

  static const Function_availability &get_function_preconditions(
      const std::string &function_name);

  static const std::map<std::string, Function_availability> s_preconditions;

 private:
#ifdef FRIEND_TEST
  FRIEND_TEST(Admin_api_preconditions, check_session);
  FRIEND_TEST(Admin_api_preconditions,
              check_instance_configuration_preconditions_errors);
  FRIEND_TEST(Admin_api_preconditions,
              check_instance_configuration_preconditions);
  FRIEND_TEST(Admin_api_preconditions,
              check_managed_instance_status_preconditions_errors);
  FRIEND_TEST(Admin_api_preconditions,
              check_managed_instance_status_preconditions);
  FRIEND_TEST(Admin_api_preconditions, check_quorum_state_preconditions_errors);
  FRIEND_TEST(Admin_api_preconditions, check_quorum_state_preconditions);
  FRIEND_TEST(Admin_api_preconditions, check_cluster_set_preconditions_errors);
  FRIEND_TEST(Admin_api_preconditions, check_cluster_set_preconditions);
  FRIEND_TEST(Admin_api_preconditions, check_cluster_fenced_preconditions);
#endif

  void check_session() const;
  metadata::State check_metadata_state_actions(
      const std::string &function_name);
  void check_instance_configuration_preconditions(
      mysqlsh::dba::TargetType::Type instance_type, int allowed_types) const;
  void check_managed_instance_status_preconditions(
      mysqlsh::dba::TargetType::Type instance_type,
      mysqlsh::dba::ReplicationQuorum::State instance_quorum_state,
      bool primary_req) const;
  void check_quorum_state_preconditions(
      mysqlsh::dba::ReplicationQuorum::State instance_quorum_state,
      mysqlsh::dba::ReplicationQuorum::State allowed_states) const;
  void check_cluster_set_preconditions(
      mysqlsh::dba::Cluster_global_status_mask allowed_states);
  void check_cluster_fenced_preconditions(bool allowed_on_fenced);

 protected:
  virtual std::pair<Cluster_global_status, Cluster_availability>
  get_cluster_global_state() const;
  virtual Cluster_status get_cluster_status();

  std::shared_ptr<MetadataStorage> m_metadata;
  std::shared_ptr<Instance> m_group_server;
  bool m_primary_available;
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
    bool primary_available = false,
    const Function_availability *custom_func_avail = nullptr);

}  // namespace dba
}  // namespace mysqlsh

#endif  // MODULES_ADMINAPI_COMMON_PRECONDITIONS_H_
