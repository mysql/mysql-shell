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

#ifndef MODULES_ADMINAPI_COMMON_METADATA_STORAGE_H_
#define MODULES_ADMINAPI_COMMON_METADATA_STORAGE_H_

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "modules/adminapi/common/cluster_types.h"
#include "modules/adminapi/common/instance_pool.h"
#include "modules/adminapi/common/metadata_management_mysql.h"
#include "mysqlshdk/libs/db/session.h"
#include "mysqlshdk/libs/mysql/group_replication.h"

namespace mysqlsh {
namespace dba {

using Instance = mysqlsh::dba::Instance;

struct Instance_metadata {
  Cluster_id cluster_id = 0;
  Instance_id id = 0;

  std::string address;
  std::string uuid;
  std::string label;
  std::string endpoint;
  std::string xendpoint;
  std::string grendpoint;
  std::string role_type;

  // GR clusters only
  std::string group_name;
};

struct Cluster_metadata {
  Cluster_id cluster_id = 0;
  std::string cluster_name;
  std::string description;
  bool enabled = true;
  std::string group_name;
  std::string topology_type;

  std::string full_cluster_name() const { return cluster_name; }
};

struct Router_metadata {
  std::string name;
  uint64_t id;
  std::string hostname;
  mysqlshdk::utils::nullable<uint64_t> rw_port;
  mysqlshdk::utils::nullable<uint64_t> ro_port;
  mysqlshdk::utils::nullable<uint64_t> rw_x_port;
  mysqlshdk::utils::nullable<uint64_t> ro_x_port;
  mysqlshdk::utils::nullable<std::string> last_checkin;
  mysqlshdk::utils::nullable<std::string> version;
};

class Cluster_impl;

#if DOXYGEN_CPP
/**
 * Represents a session to a Metadata Storage
 */
#endif
class MetadataStorage : public std::enable_shared_from_this<MetadataStorage> {
 protected:
  MetadataStorage() {}

 public:
  MetadataStorage(MetadataStorage &&other) = delete;
  MetadataStorage &operator=(const MetadataStorage &other) = delete;
  MetadataStorage &operator=(MetadataStorage &&other) = delete;

  explicit MetadataStorage(const std::shared_ptr<Instance> &instance);

  virtual ~MetadataStorage();

  /**
   * Checks that the metadata schema exists in the target session.
   *
   * Works with any schema version.
   *
   * @param out_version - version of the metadata schema, if not NULL
   * @returns true if MD schema exists, false if not.
   */
  bool check_exists(mysqlshdk::utils::Version *out_version = nullptr) const;

  virtual Cluster_id create_cluster_record(Cluster_impl *cluster, bool adopted);

  Instance_id insert_instance(const Instance_metadata &instance);
  void remove_instance(const std::string &instance_address);
  void drop_cluster(const std::string &cluster_name);

  void update_cluster_name(Cluster_id cluster_id,
                           const std::string &new_cluster_name);

  bool query_cluster_attribute(Cluster_id cluster_id,
                               const std::string &attribute,
                               shcore::Value *out_value);

  void update_cluster_attribute(Cluster_id cluster_id,
                                const std::string &attribute,
                                const shcore::Value &value);

  bool query_instance_attribute(const std::string &uuid,
                                const std::string &attribute,
                                shcore::Value *out_value);

  void update_instance_attribute(const std::string &uuid,
                                 const std::string &attribute,
                                 const shcore::Value &value);
  /**
   * Update the information on the metadata about the recovery user being used
   * by the instance with the given uuid.
   * @param instance_uuid The uuid of the instance
   * @param recovery_account_user string with the username of the recovery user.
   * If the string is empty, any existing information about the recovery user
   * of the instance with the given uuid is dropped from the metadata.
   * @param recovery_account_host string with the host of the recovery user.
   */
  void update_instance_recovery_account(
      const std::string &instance_uuid,
      const std::string &recovery_account_user,
      const std::string &recovery_account_host);
  /**
   * Fetch from the metadata the recovery account being used by the instance
   * with the given uuid.
   * @param instance_uuid the uuid of the instance
   * @return a pair with the recovery account user name and recovery account
   * hostname
   */
  std::pair<std::string, std::string> get_instance_recovery_account(
      const std::string &instance_uuid);
  void remove_instance_recovery_account(
      const std::string &instance_uuid,
      const std::string &recovery_account_user);
  bool is_recovery_account_unique(const std::string &recovery_account_user);

  virtual bool is_instance_on_cluster(Cluster_id cluster_id,
                                      const std::string &address);

  bool get_cluster(Cluster_id cluster_id, Cluster_metadata *out_cluster);

  bool get_cluster_for_server_uuid(const std::string &server_uuid,
                                   Cluster_metadata *out_cluster);

  bool get_cluster_for_group_name(const std::string &group_name,
                                  Cluster_metadata *out_cluster);

  bool get_cluster_for_cluster_name(const std::string &name,
                                    Cluster_metadata *out_cluster);

  /**
   * Checks if the given instance label is unique in the specified replica
   * set.
   *
   * @param cluster_id ID of a cluster
   * @param label Instance label to be checked.
   *
   * @return True if the given instance label is unique.
   */
  bool is_instance_label_unique(Cluster_id cluster_id,
                                const std::string &label) const;
  void set_instance_label(Cluster_id cluster_id, const std::string &label,
                          const std::string &new_label);
  size_t get_cluster_size(Cluster_id cluster_id) const;

  std::vector<Instance_metadata> get_all_instances(Cluster_id cluster_id = 0);
  Instance_metadata get_instance_by_uuid(const std::string &uuid);

  Instance_metadata get_instance_by_endpoint(
      const std::string &instance_address);

  std::vector<Instance_metadata> get_replica_set_instances(
      const Cluster_id &rs_id);

  std::vector<Router_metadata> get_routers(Cluster_id cluster_id);

  /**
   * Get the topology mode of the cluster from the metadata.
   *
   * @param cluster_id integer with the ID of the target cluster.
   *
   * @return mysqlshdk::gr::Topology_mode of the cluster registered in the
   *         metdata.
   */
  mysqlshdk::gr::Topology_mode get_cluster_topology_mode(Cluster_id cluster_id);

  /**
   * Update the topology mode of the cluster in the metadata.
   *
   * @param cluster_id integer with the ID of the target cluster.
   * @param topology_mode mysqlshdk::gr::Topology_mode to set.
   */
  void update_cluster_topology_mode(
      Cluster_id cluster_id, const mysqlshdk::gr::Topology_mode &topology_mode);

  /**
   * Get the internal metadata session.
   *
   * @return Instance object of the internal metadata session
   */
  Instance *get_md_server() const { return m_md_server.get(); }

 public:
  /**
   * Deletes metadata for the named router instance.
   *
   * @param router_def router identifier, as address[::name]
   * @return false if router_def doesn't match any router instances
   */
  bool remove_router(const std::string &router_def);

 private:
  class Transaction;
  friend class Transaction;

  std::shared_ptr<Instance> m_md_server;
  mutable mysqlshdk::utils::Version m_md_version;

  std::string get_router_query();

  std::shared_ptr<mysqlshdk::db::IResult> execute_sql(
      const std::string &sql) const;

  template <typename... Args>
  inline std::shared_ptr<mysqlshdk::db::IResult> execute_sqlf(
      const std::string &sql, const Args &... args) const {
    return execute_sql(shcore::sqlformat(sql, args...));
  }

  Cluster_metadata unserialize_cluster_metadata(
      const mysqlshdk::db::Row_ref_by_name &record);
};
}  // namespace dba
}  // namespace mysqlsh

#endif  // MODULES_ADMINAPI_COMMON_METADATA_STORAGE_H_
