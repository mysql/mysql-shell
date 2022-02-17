/*
 * Copyright (c) 2019, 2022, Oracle and/or its affiliates.
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

#ifndef MODULES_ADMINAPI_COMMON_GLOBAL_TOPOLOGY_H_
#define MODULES_ADMINAPI_COMMON_GLOBAL_TOPOLOGY_H_

#include <list>
#include <map>
#include <memory>
#include <string>
#include "modules/adminapi/common/cluster_types.h"
#include "modules/adminapi/common/health_enums.h"
#include "modules/adminapi/common/instance_pool.h"
#include "mysqlshdk/libs/mysql/group_replication.h"
#include "mysqlshdk/libs/mysql/replication.h"
#include "mysqlshdk/libs/utils/nullable.h"

namespace mysqlsh {
namespace dba {

struct Cluster_metadata;
struct Instance_metadata;

namespace topology {

class Channel;
class Node;

enum class Node_status {
  ONLINE,
  ERROR,         // Replication related error
  INCONSISTENT,  // GTID inconsistencies detected
  INVALIDATED,   // Instance was invalidated in a failover
  UNREACHABLE,   // Can't connect to instance
  OFFLINE        // Reachable but replication is stopped
};

enum class Node_role {
  PRIMARY,               // R/W master
  PASSIVE_PRIMARY,       // R/O master
  INTERMEDIATE_PRIMARY,  // R/O master that's also a slave
  SECONDARY              // slave
};

class Instance final {
 public:
  // static info
  std::string label;
  std::string uuid;
  std::string endpoint;
  bool managed = false;
  Node *node_ptr = nullptr;

 public:
  // queried static info
  mysqlshdk::utils::nullable<uint32_t> server_id;

  // state info
  int connect_errno = 0;
  std::string connect_error;

  mysqlshdk::null_bool gr_auto_rejoining;
  mysqlshdk::utils::nullable<mysqlshdk::gr::Member_state> state;
  mysqlshdk::null_bool in_majority;
  mysqlshdk::null_bool group_primary;

  mysqlshdk::null_bool super_read_only;
  mysqlshdk::null_bool read_only;
  mysqlshdk::null_bool offline_mode;
  mysqlshdk::utils::Version version;

  mysqlshdk::null_string executed_gtid_set;

  std::map<std::string, mysqlshdk::utils::nullable<std::string>>
      parallel_appliers;

  std::unique_ptr<Channel> master_channel;

  std::list<mysqlshdk::mysql::Replication_channel> unmanaged_channels;
  std::list<mysqlshdk::mysql::Slave_host> unmanaged_replicas;

 public:
  Instance_status status() const;

  bool is_fenced() const {
    return read_only.get_safe() || super_read_only.get_safe() ||
           offline_mode.get_safe();
  }

 private:
  friend class Server;

  Instance() {}
  Instance(Node *node, const Instance_metadata &info);
};

class Channel final {
 public:
  mysqlshdk::mysql::Replication_channel info;
  mysqlshdk::mysql::Replication_channel_master_info master_info;
  mysqlshdk::mysql::Replication_channel_relay_log_info relay_log_info;
  // the actual master (not necessarily the one in metadata)
  const Instance *master_ptr = nullptr;
};

class Node {
 public:
  // static info
  std::string label;
  bool managed = false;

  // from metadata
  Cluster_id cluster_id;
  bool invalidated = false;
  bool primary_master = false;
  const Node *master_node_ptr = nullptr;

  //
  mysqlshdk::utils::nullable<size_t> errant_transaction_count;
  std::string errant_transactions;

 public:
  virtual Node_status status() const = 0;
  virtual Node_role role() const = 0;

  virtual const std::list<Instance> &members() const { return m_members; }

  const Instance *get_member(const std::string &uuid) const;

  virtual std::list<const Instance *> online_members() const = 0;

  virtual const Instance *get_primary_member() const = 0;

  Instance *member(const std::string &uuid);

  virtual ~Node() {}

 protected:
  std::list<Instance> m_members;
};

class Server final : public Node {
 public:
  // static info
  Instance_id instance_id = 0;
  std::string master_instance_uuid;

 public:
  // state info

 public:
  Server(const Cluster_metadata &cluster, const Instance_metadata &instance);

  Node_status status() const override;
  Node_role role() const override;

  std::list<const Instance *> online_members() const override {
    return {&m_members.front()};
  }

  const Instance *get_primary_member() const override {
    return &m_members.front();
  }

  void scan(const mysqlshdk::mysql::IInstance *conn,
            const std::string &channel_name);

 private:
  Instance *primary_member() { return &m_members.front(); }

  friend class Server_global_topology;
};

class Global_topology {
 public:
  explicit Global_topology(const std::string &channel_name);
  virtual ~Global_topology() {}

  virtual void refresh(MetadataStorage *metadata, bool deep) = 0;

 public:
  Global_topology_type type() const { return m_topology_type; }

  virtual const std::list<Node *> &nodes() const = 0;

  bool is_single_active() const;

  std::list<const Node *> get_available_nodes() const;

  const Node *try_get_node_for_uuid(const std::string &uuid) const;

  const Node *get_primary_master_node() const;
  virtual std::list<const Node *> get_slave_nodes(const Node *master) const = 0;

  const std::string &repl_channel_name() const { return m_repl_channel_name; }

  void check_gtid_consistency(bool use_configured_primary = true);

  virtual void show_raw() const = 0;

  // the noun that describes this node (cluster, instance)
  std::string label_instance;

 protected:
  virtual void load_cluster(MetadataStorage *metadata,
                            const Cluster_metadata &cluster) = 0;

  const Instance *find_member(const std::string &uuid) const;
  const Instance *try_find_member(const std::string &uuid) const;

  void load_instance_state(Instance *instance, mysqlsh::dba::Instance *conn);
  void load_instance_slaves(Instance *instance, mysqlsh::dba::Instance *conn);

  uint32_t m_rclid = 0;
  Global_topology_type m_topology_type = Global_topology_type::NONE;
  std::string m_repl_channel_name;
};

class Server_global_topology : public Global_topology {
 public:
  explicit Server_global_topology(const std::string &channel_name)
      : Global_topology(channel_name) {
    label_instance = "instance";
  }

  void refresh(MetadataStorage *metadata, bool deep) override;

  void load(MetadataStorage *metadata, const std::string &server_uuid);

  void load_cluster(MetadataStorage *metadata,
                    const Cluster_metadata &cluster) override;

  void check_servers(bool deep);
  void check_server(Instance_id id, bool deep);

  void discover_from_unmanaged(const mysqlshdk::mysql::IInstance *instance);

 public:
  const std::list<Node *> &nodes() const override { return m_nodes; }
  const std::list<Server> &servers() const { return m_servers; }

  const Server *get_server(Instance_id id) const;
  const Server *get_server(const std::string &uuid) const;
  std::list<const Node *> get_slave_nodes(const Node *master) const override;

  void show_raw() const override;

  const topology::Server *find_failover_candidate(
      const std::list<std::shared_ptr<dba::Instance>> &candidates) const;

 private:
  void load_server_state(Server *server, mysqlsh::dba::Instance *conn,
                         bool deep);

  Server &add_server(const Cluster_metadata &cluster,
                     const Instance_metadata &instance);

  void remove_server(const Server &server);

  Server *scan_instance_recursive(const mysqlshdk::mysql::IInstance *instance);

  Server *server(Instance_id id);

  std::list<Node *> m_nodes;
  std::list<Server> m_servers;
};

Global_topology *scan_global_topology(MetadataStorage *metadata,
                                      const Cluster_metadata &cmd,
                                      const std::string &channel_name,
                                      bool deep);

}  // namespace topology

std::string to_string(topology::Node_status status);
std::string to_string(topology::Node_role role);

}  // namespace dba
}  // namespace mysqlsh

#endif  // MODULES_ADMINAPI_COMMON_GLOBAL_TOPOLOGY_H_
