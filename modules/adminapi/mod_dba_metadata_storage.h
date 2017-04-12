/*
 * Copyright (c) 2016, Oracle and/or its affiliates. All rights reserved.
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

#ifndef _MOD_DBA_METADATA_STORAGE_H_
#define _MOD_DBA_METADATA_STORAGE_H_

#include "mod_dba.h"
#include "mod_dba_cluster.h"
#include "mod_dba_replicaset.h"
#include <string>

namespace mysqlsh {
namespace mysql {
class ClassicResult;
}
namespace dba {
#if DOXYGEN_CPP
/**
* Represents a Session to a Metadata Storage
*/
#endif
class MetadataStorage : public std::enable_shared_from_this<MetadataStorage> {
public:
  MetadataStorage(std::shared_ptr<mysqlsh::ShellDevelopmentSession> session);
  ~MetadataStorage();

  bool metadata_schema_exists();
  void create_metadata_schema();
  void drop_metadata_schema();
  uint64_t get_cluster_id(const std::string &cluster_name);
  uint64_t get_cluster_id(uint64_t rs_id);
  bool cluster_exists(const std::string &cluster_name);
  void insert_cluster(const std::shared_ptr<Cluster> &cluster);
  void insert_replica_set(std::shared_ptr<ReplicaSet> replicaset, bool is_default, bool is_adopted);
  uint32_t insert_host(const shcore::Value::Map_type_ref &options);
  void insert_instance(const shcore::Value::Map_type_ref& options, uint64_t host_id, uint64_t rs_id);
  void remove_instance(const std::string &instance_address);
  void drop_cluster(const std::string &cluster_name);
  bool cluster_has_default_replicaset_only(const std::string &cluster_name);
  bool is_cluster_empty(uint64_t cluster_id);
  void drop_replicaset(uint64_t rs_id);
  void disable_replicaset(uint64_t rs_id);
  bool is_replicaset_active(uint64_t rs_id);
  std::string get_replicaset_group_name();
  void set_replicaset_group_name(std::shared_ptr<ReplicaSet> replicaset, const std::string &group_name);

  std::shared_ptr<Cluster> get_cluster(const std::string &cluster_name);
  std::shared_ptr<Cluster> get_default_cluster();
  bool has_default_cluster();

  std::shared_ptr<ReplicaSet> get_replicaset(uint64_t rs_id);
  bool is_replicaset_empty(uint64_t rs_id);
  bool is_instance_on_replicaset(uint64_t rs_id, const std::string &address);

  std::string get_seed_instance(uint64_t rs_id);
  std::shared_ptr<shcore::Value::Array_type> get_replicaset_instances(uint64_t rs_id);
  std::shared_ptr<shcore::Value::Array_type> get_replicaset_online_instances(uint64_t rs_id);

  void create_repl_account(std::string &username, std::string &password);

  std::shared_ptr<mysqlsh::ShellDevelopmentSession> get_session() const {return _session; };
  void set_session(std::shared_ptr<mysqlsh::ShellDevelopmentSession> session);

  std::shared_ptr<mysql::ClassicResult> execute_sql(const std::string &sql, bool retry = false, const std::string &log_sql = "") const;

  class Transaction {
  public:
    explicit Transaction(std::shared_ptr<MetadataStorage> md) : _md(md) {
      md->start_transaction();
    }

    ~Transaction() {
      if (_md) _md->rollback();
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
  Dba* _dba;
  std::shared_ptr<mysqlsh::ShellDevelopmentSession> _session;

  void start_transaction();
  void commit();
  void rollback();

  std::shared_ptr<Cluster> get_cluster_from_query(const std::string &query);

  std::shared_ptr<Cluster> get_cluster_matching(const std::string &condition, const std::string &value);
  // Overload the function for different types of value
  std::shared_ptr<Cluster> get_cluster_matching(const std::string &condition, bool value);
};
}
}

#endif  // _MOD_DBA_METADATA_STORAGE_H_
