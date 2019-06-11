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

#ifndef MODULES_ADMINAPI_REPLICASET_REPLICASET_H_
#define MODULES_ADMINAPI_REPLICASET_REPLICASET_H_

#define JSON_STANDARD_OUTPUT 0
#define JSON_STATUS_OUTPUT 1
#define JSON_TOPOLOGY_OUTPUT 2
#define JSON_RESCAN_OUTPUT 3

#include <set>
#include <string>
#include <vector>

#include "modules/adminapi/common/common.h"
#include "modules/adminapi/common/group_replication_options.h"
#include "modules/adminapi/common/provisioning_interface.h"
#include "mysqlshdk/include/scripting/types.h"
#include "mysqlshdk/libs/config/config.h"
#include "mysqlshdk/libs/db/connection_options.h"
#include "scripting/types.h"
#include "scripting/types_cpp.h"
#include "shellcore/shell_options.h"

namespace mysqlsh {
namespace dba {
class MetadataStorage;
class Cluster_impl;

#if DOXYGEN_CPP
/**
 * Represents a ReplicaSet
 */
#endif
class ReplicaSet {
 public:
  using Instance_info = mysqlshdk::innodbcluster::Instance_info;

  ReplicaSet(const std::string &name, const std::string &topology_type,
             const std::string &group_name,
             std::shared_ptr<MetadataStorage> metadata_storage);
  virtual ~ReplicaSet();

  void set_id(uint64_t id) { _id = id; }
  uint64_t get_id() const { return _id; }

  void set_name(const std::string &name) { _name = name; }
  const std::string &get_name() const { return _name; }

  void set_cluster(Cluster_impl *cluster) { m_cluster = cluster; }

  /**
   * Get the cluster parent object.
   *
   * @return shared_ptr to Cluster parent object.
   */
  Cluster_impl *get_cluster() const { return m_cluster; }

  std::string get_topology_type() const { return _topology_type; }

  void set_topology_type(const std::string &topology_type) {
    _topology_type = topology_type;
  }

  const std::string &get_group_name() const { return _group_name; }

  void sanity_check() const;

  void set_group_name(const std::string &group_name);

  void add_instance_metadata(
      const mysqlshdk::db::Connection_options &instance_definition,
      const std::string &label = "") const;

  void remove_instance_metadata(
      const mysqlshdk::db::Connection_options &instance_def);

  void adopt_from_gr();

  std::vector<std::string> get_online_instances() const;

  std::vector<Instance_definition> get_instances_from_metadata() const;

  std::vector<Instance_info> get_instances() const;

  std::unique_ptr<mysqlshdk::config::Config> create_config_object(
      std::vector<std::string> ignored_instances = {},
      bool skip_invalid_state = false) const;

  void execute_in_members(
      const std::vector<std::string> &states,
      const mysqlshdk::db::Connection_options &cnx_opt,
      const std::vector<std::string> &ignore_instances_vector,
      std::function<bool(std::shared_ptr<mysqlshdk::db::ISession> session)>
          functor,
      bool ignore_network_conn_errors = true) const;

  static char const *kTopologySinglePrimary;
  static char const *kTopologyMultiPrimary;
  static const char *kWarningDeprecateSslMode;

  void add_instance(const mysqlshdk::db::Connection_options &connection_options,
                    const shcore::Dictionary_t &options);
  shcore::Value check_instance_state(const Connection_options &instance_def);

  shcore::Value rejoin_instance(mysqlshdk::db::Connection_options *instance_def,
                                const shcore::Value::Map_type_ref &options);
  shcore::Value remove_instance(const shcore::Argument_list &args);
  void dissolve(const shcore::Dictionary_t &options);
  void rescan(const shcore::Dictionary_t &options);
  shcore::Value force_quorum_using_partition_of(
      const shcore::Argument_list &args);

  void remove_instances(const std::vector<std::string> &remove_instances);
  void rejoin_instances(const std::vector<std::string> &rejoin_instances,
                        const shcore::Value::Map_type_ref &options);
  void set_instance_option(const Connection_options &instance_def,
                           const std::string &option,
                           const shcore::Value &value);

  void switch_to_single_primary_mode(const Connection_options &instance_def);
  void switch_to_multi_primary_mode();
  void set_primary_instance(const Connection_options &instance_def);

  /**
   * Update the replicaset members according to the removed instance.
   *
   * More specifically, remove the specified local address from the
   * group_replication_group_seeds variable of all alive members of the
   * replicaset and then remove the recovery user used by the instance on the
   * other members through a primary instance.
   *
   * @param local_gr_address string with the local GR address (XCom) to remove.
   * @param instance target instance that was removed from the replicaset.
   */
  void update_group_members_for_removed_member(
      const std::string &local_gr_address,
      const mysqlshdk::mysql::Instance &instance);

  /**
   * Get the primary instance address.
   *
   * This funtion retrieves the address of a primary instance in the
   * replicaset. In single primary mode, only one instance is the primary and
   * its address is returned, otherwise it is assumed that multi primary mode
   * is used. In multi primary mode, the address of any available instance
   * is returned.
   */
  mysqlshdk::db::Connection_options pick_seed_instance() const;

  void validate_server_uuid(
      const std::shared_ptr<mysqlshdk::db::ISession> &instance_session) const;

  void validate_server_id(
      const mysqlshdk::mysql::IInstance &target_instance) const;

  std::string get_cluster_group_seeds(
      const std::shared_ptr<mysqlshdk::db::ISession> &instance_session =
          nullptr) const;

  void query_group_wide_option_values(
      mysqlshdk::mysql::IInstance *target_instance,
      mysqlshdk::utils::nullable<std::string> *out_gr_consistency,
      mysqlshdk::utils::nullable<int64_t> *out_gr_member_expel_timeout) const;

 private:
  friend Cluster_impl;

  void verify_topology_type_change() const;

 protected:
  uint64_t _id;
  std::string _name;
  std::string _topology_type;
  std::string _group_name;

 private:
  Cluster_impl *m_cluster;

  std::shared_ptr<MetadataStorage> _metadata_storage;
};
}  // namespace dba
}  // namespace mysqlsh

#endif  // MODULES_ADMINAPI_REPLICASET_REPLICASET_H_
