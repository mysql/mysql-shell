/*
 * Copyright (c) 2016, 2024, Oracle and/or its affiliates.
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

#include <list>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "modules/adminapi/common/cluster_types.h"
#include "modules/adminapi/common/common.h"
#include "modules/adminapi/common/instance_pool.h"
#include "modules/adminapi/common/metadata_management_mysql.h"
#include "mysqlshdk/libs/db/session.h"
#include "mysqlshdk/libs/mysql/group_replication.h"
#include "mysqlshdk/libs/mysql/undo.h"
#include "mysqlshdk/libs/mysql/utils.h"

namespace mysqlsh {
namespace dba {

inline constexpr const char *kNotifyClusterSetPrimaryChanged =
    "CLUSTER_SET_PRIMARY_CHANGED";

inline constexpr const char *kNotifyDataClusterSetId = "CLUSTER_SET_ID";

enum class Instance_type { GROUP_MEMBER, ASYNC_MEMBER, READ_REPLICA, NONE };

enum class Instance_column_md { ATTRIBUTES, ADDRESSES };

std::string to_string(const Instance_type instance_type);
Instance_type to_instance_type(const std::string &instance_type);

struct Instance_metadata {
  Cluster_id cluster_id;
  Instance_id id = 0;

  std::string address;
  std::string uuid;
  std::string label;
  std::string endpoint;
  std::string xendpoint;
  std::string grendpoint;
  uint32_t server_id = 0;
  bool hidden_from_router = false;
  std::string cert_subject;
  Instance_type instance_type = Instance_type::NONE;

  shcore::Dictionary_t tags = nullptr;

  // GR clusters only
  std::string group_name;
  Replica_type replica_type = Replica_type::GROUP_MEMBER;

  // Async clusters only
  Instance_id master_id = 0;
  std::string master_uuid;
  bool primary_master = false;
  bool invalidated = false;
};

struct Cluster_metadata {
  Cluster_type type = Cluster_type::GROUP_REPLICATION;

  Cluster_set_id cluster_set_id;

  Cluster_id cluster_id;
  std::string cluster_name;
  std::string description;
  bool enabled = true;

  shcore::Dictionary_t tags = nullptr;

  std::string full_cluster_name() const { return cluster_name; }

  // GR specific
  std::string group_name;
  std::string view_change_uuid;

  mysqlshdk::gr::Topology_mode cluster_topology_type =
      mysqlshdk::gr::Topology_mode::NONE;

  // AR specific
  Global_topology_type async_topology_type = Global_topology_type::NONE;
};

struct Cluster_set_member_metadata {
  Cluster_metadata cluster;

  Cluster_set_id cluster_set_id;
  Cluster_id master_cluster_id;

  bool primary_cluster = false;
  bool invalidated = false;
};

struct Cluster_set_metadata {
  Cluster_set_id id;

  std::string domain_name;
};

struct Router_metadata {
  std::string name;
  uint64_t id{0};
  std::string hostname;
  std::optional<std::string> rw_port;
  std::optional<std::string> ro_port;
  std::optional<std::string> rw_x_port;
  std::optional<std::string> ro_x_port;
  std::optional<std::string> rw_split_port;
  std::optional<std::string> bootstrap_target_type;
  std::optional<std::string> last_checkin;
  std::optional<std::string> version;
  std::optional<std::string> target_cluster;

  shcore::Dictionary_t tags = nullptr;
};

struct Router_options_metadata {
  std::map<std::string, shcore::Value> global;
  std::map<std::string, std::map<std::string, shcore::Value>> routers;
};

class Cluster_impl;
class Cluster_set_impl;
class Replica_set_impl;

using mysqlshdk::mysql::Transaction_undo;

inline uint32_t cluster_set_view_id_generation(uint64_t id) {
  return (id >> 32);
}

#if DOXYGEN_CPP
/**
 * Represents a session to a Metadata Storage
 */
#endif
class MetadataStorage {
 protected:
  MetadataStorage() = default;

 public:
  MetadataStorage(MetadataStorage &&other) = delete;
  MetadataStorage &operator=(const MetadataStorage &other) = delete;
  MetadataStorage &operator=(MetadataStorage &&other) = delete;

  explicit MetadataStorage(const std::shared_ptr<Instance> &instance);
  explicit MetadataStorage(const Instance &instance);

  virtual ~MetadataStorage();

  bool is_valid() const;

  /**
   * Checks that the metadata schema exists in the target session.
   *
   * Works with any schema version.
   *
   * @param out_version - internal_version of the metadata schema, if not NULL
   * @returns true if MD schema exists, false if not.
   */
  bool check_version(mysqlshdk::utils::Version *out_version = nullptr) const;

  /**
   * Checks the type of cluster that the given server uuid belongs to. Returns
   * false if the uuid is not in the metadata.
   *
   * Works with any schema version.
   */
  bool check_instance_type(const std::string &uuid,
                           const mysqlshdk::utils::Version &md_version,
                           Cluster_type *out_type,
                           Replica_type *out_replica_type) const;

  bool check_all_members_online() const;

  virtual Cluster_id create_cluster_record(Cluster_impl *cluster, bool adopted,
                                           bool recreate);

  virtual Cluster_id create_async_cluster_record(Replica_set_impl *cluster,
                                                 bool adopted);

  /**
   * Deletes any unrelated records that a Cluster's Metadata may have
   *
   * This function ensures that any unrelated entry on the target Cluster's
   * Metadata is removed:
   *
   *  - All clusterset_members
   *  - All clusterset_views
   *  - Any instance that does not belong to the Cluster
   *  - Any other cluster entry in the clusters table
   *  - Any clusterset entry in the clustersets tables
   *
   * Useful in scenarios on which a Cluster is removed from a ClusterSet but, as
   * designed, the Cluster becomes an independent entity with its own metadata
   *
   * @param cluster_id The target Cluster ID
   */
  void cleanup_for_cluster(Cluster_id cluster_id);

  Cluster_set_id create_cluster_set_record(
      Cluster_set_impl *cs, Cluster_id seed_cluster_id,
      shcore::Dictionary_t seed_attributes);

  Instance_id insert_instance(const Instance_metadata &instance,
                              Transaction_undo *undo = nullptr);
  void update_instance(const Instance_metadata &instance);
  void remove_instance(std::string_view instance_address,
                       Transaction_undo *undo = nullptr);
  void drop_cluster(const std::string &cluster_name,
                    Transaction_undo *undo = nullptr);

  void update_cluster_name(const Cluster_id &cluster_id,
                           const std::string &new_cluster_name);

  bool query_cluster_attribute(const Cluster_id &cluster_id,
                               std::string_view attribute,
                               shcore::Value *out_value) const;

  void remove_cluster_attribute(const Cluster_id &cluster_id,
                                std::string_view attribute);

  void update_cluster_attribute(const Cluster_id &cluster_id,
                                std::string_view attribute,
                                const shcore::Value &value,
                                bool store_null = false);

  void update_clusters_attribute(std::string_view attribute,
                                 const shcore::Value &value);

  void update_cluster_capability(
      const Cluster_id &cluster_id, const std::string &capability,
      const std::string &value,
      const std::set<std::string> &allowed_operations);

  bool query_cluster_capability(const Cluster_id &cluster_id,
                                std::string_view capability,
                                shcore::Value *out_value) const;

  void update_cluster_set_attribute(const Cluster_set_id &clusterset_id,
                                    std::string_view attribute,
                                    const shcore::Value &value);

  bool query_cluster_set_attribute(const Cluster_set_id &clusterset_id,
                                   std::string_view attribute,
                                   shcore::Value *out_value) const;

  bool query_instance_attribute(std::string_view uuid,
                                std::string_view attribute,
                                shcore::Value *out_value) const;

  void remove_instance_attribute(std::string_view uuid,
                                 std::string_view attribute);

  void update_instance_attribute(std::string_view uuid,
                                 std::string_view attribute,
                                 const shcore::Value &value,
                                 bool store_null = false,
                                 Transaction_undo *undo = nullptr);

  void update_instance_addresses(std::string_view uuid,
                                 std::string_view address,
                                 const shcore::Value &value,
                                 Transaction_undo *undo = nullptr);

  /**
   * Update the information on the metadata about a tag set on the instance with
   * the given uuid.
   * @param uuid The uuid of the instance
   * @param tagname string with the name of the tag
   * @param value: Value of the tag.
   * If the value is null, the provided tag will be dropped from the metadata of
   * the instance.
   */
  void set_instance_tag(const std::string &uuid, const std::string &tagname,
                        const shcore::Value &value);

  /**
   * Update the information on the metadata about a tag set on the cluster with
   * the given uuid.
   * @param uuid The uuid of the cluster
   * @param tagname string with the name of the tag
   * @param value: Value of the tag.
   * If the value is null, the provided tag will be dropped from the metadata of
   * the instance.
   */
  void set_cluster_tag(const std::string &uuid, const std::string &tagname,
                       const shcore::Value &value);

  void set_router_tag(Cluster_type type, const std::string &cluster_id,
                      const std::string &router, const std::string &tagname,
                      const shcore::Value &value);

  /**
   * Update the information on the metadata about the recovery user being used
   * by the instance with the given uuid.
   * @param instance_uuid The uuid of the instance
   * @param type whether account is for ReplicaSet or Cluster
   * @param recovery_account_user string with the username of the recovery user.
   * If the string is empty, any existing information about the recovery user
   * of the instance with the given uuid is dropped from the metadata.
   * @param recovery_account_host string with the host of the recovery user.
   */
  void update_instance_repl_account(std::string_view instance_uuid,
                                    Cluster_type type,
                                    Replica_type replica_type,
                                    std::string_view recovery_account_user,
                                    std::string_view recovery_account_host,
                                    Transaction_undo *undo = nullptr) const;

  void update_instance_uuid(Cluster_id cluster_id, Instance_id instance_id,
                            std::string_view instance_uuid);

  /**
   * Fetch from the metadata the recovery account being used by the instance
   * with the given uuid.
   * @param instance_uuid the uuid of the instance
   * @param type whether account is for ReplicaSet or Cluster
   * @para replica_type GROUP_MEMBER or READ_REPLICA
   * @return a pair with the recovery account user name and recovery account
   * hostname
   */
  std::pair<std::string, std::string> get_instance_repl_account(
      const std::string &instance_uuid, Cluster_type type,
      Replica_type replica_type);

  std::string get_instance_repl_account_user(std::string_view instance_uuid,
                                             Cluster_type type,
                                             Replica_type replica_type);

  void update_cluster_repl_account(const Cluster_id &cluster_id,
                                   const std::string &repl_account_user,
                                   const std::string &repl_account_host,
                                   Transaction_undo *undo = nullptr);

  std::pair<std::string, std::string> get_cluster_repl_account(
      const Cluster_id &cluster_id) const;

  std::string get_cluster_repl_account_user(const Cluster_id &cluster_id) const;

  std::pair<std::string, std::string> get_read_replica_repl_account(
      const std::string &instance_uuid) const;

  void update_read_replica_repl_account(const std::string &instance_uuid,
                                        const std::string &repl_account_user,
                                        const std::string &repl_account_host,
                                        Transaction_undo *undo = nullptr);

  /**
   * Fetch from the metadata list of server uuids
   * with recovery account information.
   *
   * @param cluster_id ID of a cluster
   * @return a map of std::string server UUIDS and recovery account
   */
  std::map<std::string, std::string> get_instances_with_recovery_accounts(
      const Cluster_id &cluster_id) const;

  int count_recovery_account_uses(const std::string &recovery_account_user,
                                  bool clusterset_account = false) const;

  std::vector<std::string> get_recovery_account_users();

  size_t iterate_recovery_account(
      const std::function<bool(uint32_t, std::string)> &cb);

  virtual bool is_instance_on_cluster(const Cluster_id &cluster_id,
                                      const std::string &address);

  bool get_cluster(const Cluster_id &cluster_id, Cluster_metadata *out_cluster);

  virtual bool get_cluster_for_server_uuid(const std::string &server_uuid,
                                           Cluster_metadata *out_cluster) const;

  bool get_cluster_for_cluster_name(const std::string &name,
                                    Cluster_metadata *out_cluster,
                                    bool allow_invalidated = false) const;

  bool get_cluster_set_member_for_cluster_name(
      const std::string &name, Cluster_set_member_metadata *out_cluster,
      bool allow_invalidated = false) const;

  std::vector<Cluster_metadata> get_all_clusters(
      bool include_invalidated = false);

  bool get_cluster_set(const Cluster_set_id &cs_id, bool allow_invalidated,
                       Cluster_set_metadata *out_cs,
                       std::vector<Cluster_set_member_metadata> *out_cs_members,
                       uint64_t *out_view_id = nullptr) const;

  bool get_cluster_set_member(const Cluster_id &cluster_id,
                              Cluster_set_member_metadata *out_cs_member) const;

  /**
   * Checks if the given instance label is unique in the specified replica
   * set.
   *
   * @param cluster_id ID of a cluster
   * @param label Instance label to be checked.
   *
   * @return True if the given instance label is unique.
   */
  bool is_instance_label_unique(const Cluster_id &cluster_id,
                                const std::string &label) const;
  void set_instance_label(const Cluster_id &cluster_id,
                          const std::string &label,
                          const std::string &new_label);
  size_t get_cluster_size(const Cluster_id &cluster_id) const;

  std::vector<Instance_metadata> get_replica_set_members(
      const Cluster_id &cluster_id, uint64_t *out_view_id);

  void get_replica_set_primary_info(const Cluster_id &cluster_id,
                                    std::string *out_primary_id,
                                    uint64_t *out_view_id);

  std::vector<Instance_metadata> get_all_instances(
      Cluster_id cluster_id = "", bool include_read_replicas = false);

  Instance_metadata get_instance_by_uuid(
      const std::string &uuid, const Cluster_id &cluster_id = "") const;

  Instance_metadata get_instance_by_address(
      std::string_view instance_address) const;

  Instance_metadata get_instance_by_label(std::string_view label) const;

  std::vector<Instance_metadata> get_replica_set_instances(
      const Cluster_id &rs_id);

  std::vector<Router_metadata> get_routers(const Cluster_id &cluster_id);
  std::vector<Router_metadata> get_clusterset_routers(const Cluster_set_id &cs);

  Router_options_metadata get_routing_options(Cluster_type type,
                                              const std::string &id);

  std::string get_cluster_name(const std::string &group_replication_group_name);
  std::string get_cluster_group_name(const std::string &cluster_name);

  void set_global_routing_option(Cluster_type type, const std::string &id,
                                 const std::string &option,
                                 const shcore::Value &value);

  void set_routing_option(Cluster_type type, const std::string &router,
                          const std::string &cluster_id,
                          const std::string &option,
                          const shcore::Value &value);

  shcore::Value get_global_routing_option(Cluster_type type,
                                          const std::string &cluster_id,
                                          const std::string &option);

  shcore::Value get_routing_option(Cluster_type type,
                                   const std::string &cluster_id,
                                   const std::string &router,
                                   const std::string &option);

  /**
   * Get the topology mode of the cluster from the metadata.
   *
   * @param cluster_id integer with the ID of the target cluster.
   *
   * @return mysqlshdk::gr::Topology_mode of the cluster registered in the
   *         metdata.
   */
  mysqlshdk::gr::Topology_mode get_cluster_topology_mode(
      const Cluster_id &cluster_id);

  /**
   * Update the topology mode of the cluster in the metadata.
   *
   * @param cluster_id integer with the ID of the target cluster.
   * @param topology_mode mysqlshdk::gr::Topology_mode to set.
   */
  void update_cluster_topology_mode(
      const Cluster_id &cluster_id,
      const mysqlshdk::gr::Topology_mode &topology_mode);

  /**
   * Get the internal metadata session/instance object.
   */
  virtual std::shared_ptr<Instance> get_md_server() const {
    return m_md_server;
  }

 public:
  /**
   * Deletes metadata for the named router instance.
   *
   * @param router_def router identifier, as address[::name]
   * @return false if router_def doesn't match any router instances
   */
  bool remove_router(const std::string &router_def);

  /**
   * Sets a target-cluster for all Routers of a Cluster
   *
   * @param cluster_id The target Cluster ID
   * @param target_cluster The target-cluster value to be set
   */
  void set_target_cluster_for_all_routers(const Cluster_id &cluster_id,
                                          const std::string &target_cluster);

  /**
   * Migrates all Routers of a Cluster to a ClusterSet
   *
   * @param cluster_id The target Cluster ID
   * @param cluster_set_id The target clusterSet ID to be set
   */
  void migrate_routers_to_clusterset(const Cluster_id &cluster_id,
                                     const Cluster_set_id &cluster_set_id);

  /**
   * Migrates the Read Only Targets setting from a Cluster to a ClusterSet when
   * it becomes a ClusterSet
   *
   * @param cluster_id The target Cluster ID
   * @param cluster_set_id The target clusterSet ID to be set
   */
  void migrate_read_only_targets_to_clusterset(
      const Cluster_id &cluster_id, const Cluster_set_id &cluster_set_id);

  /**
   * Gets all routers using a Cluster as Target Cluster
   *
   * @param target_cluster_group_name The target Cluster
   * group_replication_group_name
   * @return a list of Router_metadata with the obtained Routers
   */
  std::vector<Router_metadata> get_routers_using_cluster_as_target(
      const Cluster_id &cluster_id);

  shcore::Value get_router_info(const std::string &router_name);

  // Async cluster view operations
  Instance_id record_async_member_added(const Instance_metadata &member);
  void record_async_member_rejoined(const Instance_metadata &member);
  void record_async_member_removed(const Cluster_id &cluster_id,
                                   Instance_id instance_id);
  void record_async_primary_switch(Instance_id new_primary_id);
  void record_async_primary_forced_switch(
      Instance_id new_primary_id, const std::list<Instance_id> &invalidated);

  // ClusterSet operations
  void record_cluster_set_member_added(
      const Cluster_set_member_metadata &cluster);
  void record_cluster_set_member_removed(const Cluster_set_id &cs_id,
                                         const Cluster_id &cluster_id);
  void record_cluster_set_member_rejoined(const Cluster_set_id &cs_id,
                                          const Cluster_id &cluster_id,
                                          const Cluster_id &master_cluster_id);
  void record_cluster_set_primary_switch(
      const Cluster_set_id &cs_id, const Cluster_id &new_primary_id,
      const std::list<Cluster_id> &invalidated);
  void record_cluster_set_primary_failover(
      const Cluster_set_id &cs_id, const Cluster_id &cluster_id,
      const std::list<Cluster_id> &invalidated);

 public:
  mysqlsh::dba::metadata::State get_state() { return m_md_state; }

  void invalidate_cached() {
    m_md_state = mysqlsh::dba::metadata::State::NONEXISTING;
  }

  /**
   * This function returns the current installed version of the MD schema
   */
  mysqlshdk::utils::Version installed_version() const {
    check_version();
    return m_md_version;
  }

  /**
   * This function returns the version of the MD schema that contains the
   * reliable information, could be MD schema or MD schema backup.
   */
  mysqlshdk::utils::Version real_version() const {
    check_version();
    return m_real_md_version;
  }

  std::string version_schema() const {
    check_version();
    return m_md_version_schema;
  }

  mysqlsh::dba::metadata::State state() const {
    check_version();
    return m_md_state;
  }

  bool check_metadata(mysqlshdk::utils::Version *out_version = nullptr,
                      Cluster_type *out_type = nullptr,
                      Replica_type *out_replica_type = nullptr) const;

  bool check_cluster_set(
      const mysqlshdk::mysql::IInstance *target_instance = nullptr,
      uint64_t *out_view_id = nullptr,
      std::string *out_cs_domain_name = nullptr,
      Cluster_set_id *out_cluster_set_id = nullptr) const;

  bool supports_cluster_set() const;

  bool supports_read_replicas() const;

  class Transaction {
   public:
    explicit Transaction(std::shared_ptr<MetadataStorage> md)
        : m_md{std::move(md)} {
      m_md->execute_sql("START TRANSACTION");
#ifndef NDEBUG
      m_check_for_auto_commits = getenv("MYSQLSH_TEST_ALL");
#endif
    }

    ~Transaction() noexcept {
      try {
        rollback();
      } catch (const std::exception &e) {
        log_error("Error implicitly rolling back transaction: %s", e.what());
      }
    }

    void rollback() {
      if (!m_md) return;
#ifndef NDEBUG
      if (m_check_for_auto_commits) {
        mysqlshdk::mysql::assert_transaction_is_open(
            m_md->get_md_server()->get_session());
      }
#endif
      std::exchange(m_md, nullptr)->execute_sql("ROLLBACK");
    }

    void commit() {
      if (!m_md) return;

#ifndef NDEBUG
      if (m_check_for_auto_commits) {
        mysqlshdk::mysql::assert_transaction_is_open(
            m_md->get_md_server()->get_session());
      }
#endif
      std::exchange(m_md, nullptr)->execute_sql("COMMIT");
    }

   private:
    std::shared_ptr<MetadataStorage> m_md;
#ifndef NDEBUG
    bool m_check_for_auto_commits = false;
#endif
  };

 private:
  void begin_acl_change_record(const Cluster_id &cluster_id,
                               const char *operation, uint32_t *out_aclvid,
                               uint32_t *last_aclvid);

  void set_table_tag(std::string_view tablename,
                     std::string_view uuid_column_name, std::string_view uuid,
                     std::string_view tagname, const shcore::Value &value);

  bool cluster_sets_supported() const;

  void update_instance_metadata(std::string_view uuid,
                                Instance_column_md column,
                                std::string_view field,
                                const shcore::Value &value,
                                Transaction_undo *undo);

  friend class Transaction;

  std::shared_ptr<Instance> m_md_server;
  bool m_owns_md_server = false;
  mutable mysqlshdk::utils::Version m_md_version;
  mutable mysqlshdk::utils::Version m_real_md_version;
  mutable std::string m_md_version_schema;
  mutable mysqlsh::dba::metadata::State m_md_state =
      mysqlsh::dba::metadata::State::NONEXISTING;

  std::shared_ptr<mysqlshdk::db::IResult> execute_sql(
      const std::string &sql) const;

  template <typename... Args>
  inline std::shared_ptr<mysqlshdk::db::IResult> execute_sqlf(
      const std::string &sql, const Args &... args) const {
    return execute_sql(shcore::sqlformat(sql, args...));
  }

  Cluster_metadata unserialize_cluster_metadata(
      const mysqlshdk::db::Row_ref_by_name &record,
      const mysqlshdk::utils::Version &version) const;

  Instance_metadata unserialize_instance(
      const mysqlshdk::db::Row_ref_by_name &row,
      mysqlshdk::utils::Version *mdver_in = nullptr) const;
};

}  // namespace dba
}  // namespace mysqlsh

#endif  // MODULES_ADMINAPI_COMMON_METADATA_STORAGE_H_
