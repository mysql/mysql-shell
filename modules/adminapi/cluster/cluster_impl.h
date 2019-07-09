/*
 * Copyright (c) 2016, 2019, Oracle and/or its affiliates. All rights reserved.
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

#ifndef MODULES_ADMINAPI_CLUSTER_CLUSTER_IMPL_H_
#define MODULES_ADMINAPI_CLUSTER_CLUSTER_IMPL_H_

#include <memory>
#include <string>

#include "scripting/types.h"
#include "scripting/types_cpp.h"
#include "shellcore/shell_options.h"

#include "modules/adminapi/cluster/replicaset/replicaset.h"
#include "modules/adminapi/common/cluster_types.h"
#include "modules/adminapi/common/common.h"
#include "modules/adminapi/common/group_replication_options.h"
#include "mysqlshdk/libs/db/connection_options.h"

namespace mysqlsh {
namespace dba {

// User provided option for telling us to assume that the cluster was created
// with a server where the full update history is reflected in its GTID set
constexpr const char k_cluster_attribute_assume_gtid_set_complete[] =
    "opt_gtidSetIsComplete";
// User provided option to disabling clone
constexpr const char k_cluster_attribute_disable_clone[] = "opt_disableClone";
// Flag to indicate the default cluster in the metadata
constexpr const char k_cluster_attribute_default[] = "default";

// Timestamp of when the instance was added to the group
constexpr const char k_instance_attribute_join_time[] = "joinTime";

class MetadataStorage;
struct Cluster_metadata;

class Cluster_impl {
 public:
  friend class Cluster;

  Cluster_impl(const Cluster_metadata &metadata,
               const std::shared_ptr<Instance> &group_server,
               const std::shared_ptr<MetadataStorage> &metadata_storage);

  Cluster_impl(const std::string &name, const std::string &group_name,
               const std::shared_ptr<Instance> &group_server,
               const std::shared_ptr<MetadataStorage> &metadata_storage);
  virtual ~Cluster_impl();

  uint64_t get_id() const { return _id; }
  void set_id(uint64_t id) { _id = id; }

  std::shared_ptr<ReplicaSet> get_default_replicaset() const {
    return _default_replica_set;
  }

  std::shared_ptr<ReplicaSet> create_default_replicaset(const std::string &name,
                                                        bool multi_primary);

  std::string get_name() const { return _name; }
  std::string get_description() const { return _description; }
  bool get_disable_clone_option() const;
  void set_disable_clone_option(const bool disable_clone);

  void set_description(const std::string &description) {
    _description = description;
  }

  std::string get_group_name() const { return m_group_name; }

  std::string get_topology_type() const {
    return get_default_replicaset()->get_topology_type();
  }

  void set_name(const std::string &name) { _name = name; }

  bool get_gtid_set_is_complete() const;

  std::shared_ptr<Instance> get_target_instance() const {
    return m_target_server;
  }

  std::shared_ptr<mysqlshdk::db::ISession> get_group_session() const {
    return get_target_instance()->get_session();
  }

  mysqlshdk::mysql::Auth_options create_replication_user(
      mysqlshdk::mysql::IInstance *target);

  void drop_replication_user(mysqlshdk::mysql::IInstance *target);

  void disconnect();

  bool contains_instance_with_address(const std::string &host_port);

 public:
  // TODO(alfredo) - whoever refactors these methods, should move them to
  // the API Cluster class. Equivalent methods should also exist here,
  // but for internal use (and thus, no options in Dictionary_t,
  // no shcore::Values etc). Parameter syntax validations should also be assumed
  // to have already been done.
  shcore::Value describe();
  shcore::Value options(const shcore::Dictionary_t &options);
  shcore::Value status(uint64_t extended);
  shcore::Value remove_instance(const shcore::Argument_list &args);
  void add_instance(const Connection_options &instance_def,
                    const shcore::Dictionary_t &options);
  shcore::Value rejoin_instance(const Connection_options &instance_def,
                                const shcore::Dictionary_t &options);

  void set_option(const std::string &option, const shcore::Value &value);

  Cluster_check_info check_preconditions(
      const std::string &function_name) const;

  std::shared_ptr<MetadataStorage> get_metadata_storage() const {
    return _metadata_storage;
  }

  /*
   * Synchronize transactions on target instance.
   *
   * Wait for all current cluster transactions to be applied on the specified
   * target instance.
   *
   * @param target_instance instance to wait for transaction to be applied.
   *
   * @throw RuntimeError if the timeout is reached when waiting for
   * transactions to be applied.
   */
  void sync_transactions(
      const mysqlshdk::mysql::IInstance &target_instance) const;

 protected:
  Cluster_id _id;
  std::string _name;
  std::string m_group_name;
  std::shared_ptr<ReplicaSet> _default_replica_set;
  std::string _description;
  // Session to a member of the group so we can query its status and other
  // stuff from pfs
  std::shared_ptr<Instance> m_target_server;
  std::shared_ptr<MetadataStorage> _metadata_storage;
  // Used shell options
  void init();

 public:
  shcore::Value force_quorum_using_partition_of(
      const shcore::Argument_list &args);

  void dissolve(const shcore::Dictionary_t &options);

  void rescan(const shcore::Dictionary_t &options);

  void switch_to_single_primary_mode(const Connection_options &instance_def);
  void switch_to_multi_primary_mode();
  void set_primary_instance(const Connection_options &instance_def);
  void set_instance_option(const Connection_options &instance_def,
                           const std::string &option,
                           const shcore::Value &value);
  shcore::Value check_instance_state(const Connection_options &instance_def);

  /**
   * Get the lowest server version of the cluster members.
   *
   * The version information is obtained from available (ONLINE and RECOVERING)
   * members, ignoring not available instances.
   *
   * @return Version object of the lowest instance version in the cluster.
   */
  mysqlshdk::utils::Version get_lowest_instance_version() const;
};

}  // namespace dba
}  // namespace mysqlsh

#endif  // MODULES_ADMINAPI_CLUSTER_CLUSTER_IMPL_H_
