/*
 * Copyright (c) 2016, 2021, Oracle and/or its affiliates.
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
#include <string>
#include <utility>
#include <vector>

#include "modules/adminapi/common/cluster_types.h"
#include "modules/adminapi/common/common.h"
#include "modules/adminapi/common/instance_pool.h"
#include "modules/adminapi/common/metadata_management_mysql.h"
#include "mysqlshdk/libs/db/session.h"
#include "mysqlshdk/libs/mysql/group_replication.h"
#include "mysqlshdk/libs/mysql/utils.h"

namespace mysqlsh {
namespace dba {

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

  // GR clusters only
  std::string group_name;

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

  std::string full_cluster_name() const { return cluster_name; }

  // GR specific
  std::string group_name;

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
class Cluster_set_impl;
class Replica_set_impl;

class Cached_metadata_base {
 public:
  Cached_metadata_base(const Cached_metadata_base &) = delete;

  void invalidate();

  bool is_valid() const;

  /*
   Associates the validity of this cache value to the given MetadataStorage
   object. The cache item will become invalid when that object is deleted or
   invalidated, as well as when invalidate() is called on the cached value
   itself.
   */
  void track(const std::shared_ptr<MetadataStorage> &owner);

 protected:
  Cached_metadata_base() {}

 private:
  friend class MetadataStorage;

  std::weak_ptr<MetadataStorage> m_owner;
};

template <typename C>
class Cached_metadata : public Cached_metadata_base {
 public:
  Cached_metadata() : Cached_metadata_base() {}
  Cached_metadata(const Cached_metadata &) = delete;

  void operator=(C &&value) { _value = std::move(value); }

  C &operator*() {
    if (!is_valid()) throw std::logic_error("refresh needed");
    return _value;
  }

  C *operator->() {
    if (!is_valid()) throw std::logic_error("refresh needed");
    return &_value;
  }

 private:
  C _value;
};

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
  explicit MetadataStorage(const Instance &instance);

  virtual ~MetadataStorage();

  // invalidate the whole MetadataStorage object instance
  void invalidate();

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
                           Cluster_type *out_type) const;

  bool check_all_members_online() const;

  virtual Cluster_id create_cluster_record(Cluster_impl *cluster, bool adopted);

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

  Instance_id insert_instance(const Instance_metadata &instance);
  void update_instance(const Instance_metadata &instance);
  void remove_instance(const std::string &instance_address);
  void drop_cluster(const std::string &cluster_name);

  void update_cluster_name(const Cluster_id &cluster_id,
                           const std::string &new_cluster_name);

  bool query_cluster_attribute(const Cluster_id &cluster_id,
                               const std::string &attribute,
                               shcore::Value *out_value) const;

  void update_cluster_attribute(const Cluster_id &cluster_id,
                                const std::string &attribute,
                                const shcore::Value &value);

  void update_clusterset_attribute(const Cluster_set_id &clusterset_id,
                                   const std::string &attribute,
                                   const shcore::Value &value);

  bool query_clusterset_attribute(const Cluster_set_id &clusterset_id,
                                  const std::string &attribute,
                                  shcore::Value *out_value) const;

  bool query_instance_attribute(const std::string &uuid,
                                const std::string &attribute,
                                shcore::Value *out_value) const;

  void update_instance_attribute(const std::string &uuid,
                                 const std::string &attribute,
                                 const shcore::Value &value);

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

  /**
   * Returns the list of tags for a given instance stored on the metadata
   * @param uuid The uuid of the instance
   * @return string with the json object representation of the tags for the
   * given instance. Empty string means there are no tags.
   */
  std::string get_instance_tags(const std::string &uuid) const;

  /**
   * Returns the list of tags for a given cluster/replicaset stored on the
   * metadata
   * @param uuid The uuid of the cluster/replicaset
   * @return string with the json object representation of the tags for the
   * given cluster/replicaset. Empty string means there are no tags.
   */
  std::string get_cluster_tags(const std::string &uuid) const;

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

  void update_cluster_repl_account(const Cluster_id &cluster_id,
                                   const std::string &repl_account_user,
                                   const std::string &repl_account_host);

  std::pair<std::string, std::string> get_cluster_repl_account(
      const Cluster_id &cluster_id) const;

  /**
   * Fetch from the metadata list of server uuids
   * with recovery account information.
   *
   * @param cluster_id ID of a cluster
   * @return a map of std::string server UUIDS and recovery account
   */
  std::map<std::string, std::string> get_instances_with_recovery_accounts(
      const Cluster_id &cluster_id) const;

  bool is_recovery_account_unique(const std::string &recovery_account_user,
                                  bool clusterset_account = false);

  virtual bool is_instance_on_cluster(const Cluster_id &cluster_id,
                                      const std::string &address);

  bool get_cluster(const Cluster_id &cluster_id, Cluster_metadata *out_cluster);

  virtual bool get_cluster_for_server_uuid(const std::string &server_uuid,
                                           Cluster_metadata *out_cluster) const;

  bool get_cluster_for_cluster_name(const std::string &name,
                                    Cluster_metadata *out_cluster) const;

  bool get_cluster_set_member_for_cluster_name(
      const std::string &name, Cluster_set_member_metadata *out_cluster) const;

  std::vector<Cluster_metadata> get_all_clusters();

  bool get_cluster_set(
      const Cluster_set_id &cs_id, Cluster_set_metadata *out_cs,
      std::vector<Cluster_set_member_metadata> *out_cs_members) const;

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

  std::vector<Instance_metadata> get_all_instances(Cluster_id cluster_id = "");

  Instance_metadata get_instance_by_uuid(const std::string &uuid) const;

  Instance_metadata get_instance_by_address(
      const std::string &instance_address);

  std::vector<Instance_metadata> get_replica_set_instances(
      const Cluster_id &rs_id);

  std::vector<Router_metadata> get_routers(const Cluster_id &cluster_id);

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
   * Gets all routers using a Cluster as Target Cluster
   *
   * @param target_cluster_group_name The target Cluster
   * group_replication_group_name
   * @return a list of Router_metadata with the obtained Routers
   */
  std::vector<Router_metadata> get_routers_using_cluster_as_target(
      const Cluster_id &cluster_id);

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
                      Cluster_type *out_type = nullptr) const;

  bool check_cluster_set(Instance *target_instance = nullptr,
                         uint64_t *out_view_id = nullptr,
                         std::string *out_cs_domain_name = nullptr) const;

  bool supports_cluster_set() const;

  /**
   * Try to acquire an exclusive lock on the metadata.
   *
   * NOTE: Required lock service UDFs which is assumed to be already available
   *       if supported (automatically installed if needed by the instance
   *       level lock).
   *
   * @throw shcore::Exception if the lock cannot be acquired or any other
   * error occur when trying to obtain the lock.
   */
  void get_lock_exclusive() const;

  /**
   * Release all locks on the metadata.
   *
   * @param no_throw boolean indicating if exceptions are thrown in case a
   *                 failure occur releasing locks. By default, true meaning
   *                 that no exception is thrown.
   *
   */
  void release_lock(bool no_throw = true) const;

  class Transaction {
   public:
    explicit Transaction(const std::shared_ptr<MetadataStorage> &md,
                         bool check_for_auto_commits = false)
        : _md(md), m_check_for_auto_commits(check_for_auto_commits) {
      md->execute_sql("START TRANSACTION");
    }

    ~Transaction() {
      try {
        rollback();
      } catch (const std::exception &e) {
        log_error("Error implicitly rolling back transaction: %s", e.what());
      }
    }

    void rollback() {
      if (_md) {
#ifndef NDEBUG
        if (m_check_for_auto_commits) {
          mysqlshdk::mysql::assert_transaction_is_open(
              _md->get_md_server()->get_session());
        }
#endif
        auto tmp = _md;
        _md.reset();
        tmp->execute_sql("ROLLBACK");
      }
    }

    void commit() {
      if (_md) {
#ifndef NDEBUG
        if (m_check_for_auto_commits) {
          mysqlshdk::mysql::assert_transaction_is_open(
              _md->get_md_server()->get_session());
        }
#endif

        auto tmp = _md;
        _md.reset();
        tmp->execute_sql("COMMIT");
      }
    }

   private:
    std::shared_ptr<MetadataStorage> _md;
#ifdef __clang__
    [[maybe_unused]] bool m_check_for_auto_commits = false;
#else
    bool m_check_for_auto_commits = false;
#endif
  };

 private:
  void begin_acl_change_record(const Cluster_id &cluster_id,
                               const char *operation, uint32_t *out_aclvid,
                               uint32_t *last_aclvid);

  void set_table_tag(const std::string &tablename,
                     const std::string &uuid_column_name,
                     const std::string &uuid, const std::string &tagname,
                     const shcore::Value &value);

  std::string get_table_tags(const std::string &tablename,
                             const std::string &uuid_column_name,
                             const std::string &uuid) const;

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
