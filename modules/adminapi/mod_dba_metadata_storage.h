/*
 * Copyright (c) 2016, 2018, Oracle and/or its affiliates. All rights reserved.
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

#ifndef MODULES_ADMINAPI_MOD_DBA_METADATA_STORAGE_H_
#define MODULES_ADMINAPI_MOD_DBA_METADATA_STORAGE_H_

/* TODO(all)
 * This class is to be phased out in favor of the more general
 * mysqlshdk::innodbcluster::Metadata_mysql class.
 *
 * Functionality in this class that actually belong to the metadata class
 * should be redesigned and moved thre.
 */

#include <memory>
#include <string>
#include <vector>
#include "modules/adminapi/mod_dba.h"
#include "modules/adminapi/mod_dba_cluster.h"
#include "modules/adminapi/mod_dba_replicaset.h"
#include "mysqlshdk/libs/db/session.h"

#define ER_NOT_VALID_PASSWORD 1819
#define ER_PLUGIN_IS_NOT_LOADED 1524

namespace mysqlsh {
namespace dba {
#if DOXYGEN_CPP
/**
* Represents a session to a Metadata Storage
*/
#endif
class MetadataStorage : public std::enable_shared_from_this<MetadataStorage> {
 protected:
  // Added for minimal gmock support
  MetadataStorage() : _tx_deep(0) {}

 public:
  MetadataStorage(const MetadataStorage &other) = delete;
  MetadataStorage(MetadataStorage &&other) = delete;
  MetadataStorage &operator=(const MetadataStorage &other) = delete;
  MetadataStorage &operator=(MetadataStorage &&other) = delete;
  explicit MetadataStorage(std::shared_ptr<mysqlshdk::db::ISession> session);
  virtual ~MetadataStorage();

  bool metadata_schema_exists();
  virtual void create_metadata_schema();
  void drop_metadata_schema();
  uint64_t get_cluster_id(const std::string &cluster_name);
  uint64_t get_cluster_id(uint64_t rs_id);
  virtual bool cluster_exists(const std::string &cluster_name);
  virtual void insert_cluster(const std::shared_ptr<Cluster> &cluster);
  void insert_replica_set(std::shared_ptr<ReplicaSet> replicaset,
                          bool is_default, bool is_adopted);
  uint32_t insert_host(const std::string &host, const std::string &ip_address,
                       const std::string &location);
  void insert_instance(const Instance_definition &options);
  void remove_instance(const std::string &instance_address);
  void drop_cluster(const std::string &cluster_name);
  bool cluster_has_default_replicaset_only(const std::string &cluster_name);
  virtual bool is_cluster_empty(uint64_t cluster_id);
  void drop_replicaset(uint64_t rs_id);
  void disable_replicaset(uint64_t rs_id);
  bool is_replicaset_active(uint64_t rs_id);
  void set_replicaset_group_name(std::shared_ptr<ReplicaSet> replicaset,
                                 const std::string &group_name);
  virtual void load_cluster(const std::string &cluster_name,
                           std::shared_ptr<Cluster> cluster);
  virtual void load_default_cluster(std::shared_ptr<Cluster> cluster);
  bool has_default_cluster();

  bool is_replicaset_empty(uint64_t rs_id);
  virtual bool is_instance_on_replicaset(uint64_t rs_id,
                                         const std::string &address);
  uint64_t get_replicaset_count(uint64_t rs_id) const;

  std::string get_seed_instance(uint64_t rs_id);
  std::vector<Instance_definition> get_replicaset_instances(
      uint64_t rs_id, bool with_state = false,
      const std::vector<std::string> &states = {},
      const std::shared_ptr<mysqlshdk::db::ISession> &alt_session = nullptr);
  std::vector<Instance_definition> get_replicaset_online_instances(
      uint64_t rs_id,
      const std::shared_ptr<mysqlshdk::db::ISession> &alt_session = nullptr);
  std::vector<Instance_definition> get_replicaset_active_instances(
      uint64_t rs_id,
      const std::shared_ptr<mysqlshdk::db::ISession> &alt_session = nullptr);

  Instance_definition get_instance(const std::string &instance_address);

  virtual void create_repl_account(std::string &username,
                                   std::string &password);

  std::shared_ptr<mysqlshdk::db::ISession> get_session() const {
    return _session;
  }

  std::shared_ptr<mysqlshdk::db::IResult> execute_sql(
      const std::string &sql, bool retry = false,
      const std::string &log_sql = "") const;

  void create_account(const std::string &username, const std::string &password,
                      const std::string &hostname);

  class Transaction {
   public:
    explicit Transaction(std::shared_ptr<MetadataStorage> md) : _md(md) {
      md->start_transaction();
    }

    ~Transaction() {
      try {
        if (_md)
          _md->rollback();
      } catch (std::exception &e) {
        log_error("Error implicitly rolling back transaction: %s", e.what());
      }
    }

    void commit() {
      if (_md) {
        _md->commit();
        _md.reset();
      }
    }

   private:
    std::shared_ptr<MetadataStorage> _md;
  };

 private:
  std::shared_ptr<mysqlshdk::db::ISession> _session;
  int _tx_deep;

  virtual void start_transaction();
  virtual void commit();
  virtual void rollback();

  bool get_cluster_from_query(const std::string &query,
                              std::shared_ptr<Cluster> cluster);
};
}  // namespace dba
}  // namespace mysqlsh

#endif  // MODULES_ADMINAPI_MOD_DBA_METADATA_STORAGE_H_
