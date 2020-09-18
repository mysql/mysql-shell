/*
 * Copyright (c) 2019, 2020, Oracle and/or its affiliates.
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

#include "modules/adminapi/common/global_topology.h"
#include <algorithm>
#include <list>
#include <vector>
#include "modules/adminapi/common/common.h"
#include "modules/adminapi/common/dba_errors.h"
#include "modules/adminapi/common/metadata_storage.h"
#include "modules/adminapi/common/parallel_applier_options.h"
#include "mysqlshdk/include/scripting/types.h"
#include "mysqlshdk/include/shellcore/console.h"
#include "mysqlshdk/libs/mysql/repl_config.h"
#include "mysqlshdk/libs/utils/utils_net.h"
#include "mysqlshdk/libs/utils/utils_string.h"

namespace mysqlsh {
namespace dba {

std::string to_string(topology::Node_status status) {
  switch (status) {
    case topology::Node_status::UNREACHABLE:
      return "UNREACHABLE";
    case topology::Node_status::INVALIDATED:
      return "INVALIDATED";
    case topology::Node_status::ERROR:
      return "ERROR";
    case topology::Node_status::OFFLINE:
      return "OFFLINE";
    case topology::Node_status::INCONSISTENT:
      return "INCONSISTENT";
    case topology::Node_status::ONLINE:
      return "ONLINE";
  }
  throw std::logic_error("internal error");
}

std::string to_string(topology::Node_role role) {
  switch (role) {
    case topology::Node_role::PRIMARY:
      return "PRIMARY";
    case topology::Node_role::PASSIVE_PRIMARY:
      return "PASSIVE_PRIMARY";
    case topology::Node_role::INTERMEDIATE_PRIMARY:
      return "INTERMEDIATE_PRIMARY";
    case topology::Node_role::SECONDARY:
      return "SECONDARY";
  }
  throw std::logic_error("internal error");
}

std::string to_string(Global_topology_type type) {
  switch (type) {
    case Global_topology_type::NONE:
      return "NONE";

    case Global_topology_type::SINGLE_PRIMARY_TREE:
      return "SINGLE-PRIMARY-TREE";
  }
  throw std::logic_error("internal error");
}

Global_topology_type to_topology_type(const std::string &s_) {
  std::string s = shcore::str_upper(s_);
  if (s == "SINGLE-PRIMARY-TREE" || s == "ACTIVE-PASSIVE")
    return Global_topology_type::SINGLE_PRIMARY_TREE;
  else
    throw std::invalid_argument("Invalid replication topology type '" + s_ +
                                "'");
}

namespace topology {

namespace {

void load_instance_channels(Instance *instance, mysqlsh::dba::Instance *conn,
                            const std::string &channel_name) {
  std::vector<mysqlshdk::mysql::Replication_channel> channels(
      mysqlshdk::mysql::get_incoming_channels(*conn));
  // XXX fix duplicate scans
  for (const auto &ch : channels) {
    if (ch.channel_name == channel_name) {
      log_info("channel '%s' at %s with source_uuid: %s, source %s:%i (%s)",
               channel_name.c_str(), instance->label.c_str(),
               ch.source_uuid.c_str(), ch.host.c_str(), ch.port,
               (ch.status() == mysqlshdk::mysql::Replication_channel::OFF
                    ? "stopped"
                    : "running"));

      instance->master_channel.reset(new Channel());
      instance->master_channel->info = ch;

      if (!mysqlshdk::mysql::get_channel_info(
              *conn, ch.channel_name, &instance->master_channel->master_info,
              &instance->master_channel->relay_log_info)) {
        throw shcore::Exception::runtime_error(
            "Could not query replication configuration from "
            "mysql.slave_master_info, relay_log_info (channel = '" +
            ch.channel_name + "')");
      }
    } else if (ch.channel_name != "group_replication_applier") {
      log_warning("%s has an unmanaged/unexpected replication channel '%s'",
                  instance->label.c_str(), ch.channel_name.c_str());

      instance->unmanaged_channels.push_back(ch);
    }
  }
}

}  // namespace

const Instance *Node::get_member(const std::string &uuid) const {
  for (const auto &i : members()) {
    if (i.uuid == uuid) return &i;
  }
  throw shcore::Exception("Metadata missing for instance with uuid " + uuid,
                          SHERR_DBA_MEMBER_METADATA_MISSING);
}

Instance *Node::member(const std::string &uuid) {
  for (auto &i : m_members) {
    if (i.uuid == uuid) return &i;
  }
  throw shcore::Exception("Metadata missing for instance with uuid " + uuid,
                          SHERR_DBA_MEMBER_METADATA_MISSING);
}

// -----------------------------------------------------------------------------

Instance::Instance(Node *node, const Instance_metadata &info) {
  label = info.label;
  uuid = info.uuid;
  endpoint = info.endpoint;
  managed = true;
  node_ptr = node;
}

Instance_status Instance::status() const {
  if (dynamic_cast<const Server *>(node_ptr)) {
    if (connect_errno)
      return Instance_status::UNREACHABLE;
    else
      return Instance_status::OK;
  } else {
    if (!node_ptr) {
      return Instance_status::INVALID;
    } else if (in_majority.is_null()) {
      if (connect_errno)
        return Instance_status::UNREACHABLE;
      else
        return Instance_status::MISSING;
    } else {
      if (*in_majority) {
        if (*state == mysqlshdk::gr::Member_state::RECOVERING) {
          return Instance_status::RECOVERING;
        } else {
          assert(*state == mysqlshdk::gr::Member_state::ONLINE);
          return Instance_status::OK;
        }
      } else {
        return Instance_status::OUT_OF_QUORUM;
      }
    }
  }
}

// -----------------------------------------------------------------------------

Server::Server(const Cluster_metadata &cluster,
               const Instance_metadata &instance) {
  assert(cluster.group_name.empty());

  cluster_id = cluster.cluster_id;

  label = instance.label;
  managed = cluster_id.empty() ? false : true;

  instance_id = instance.id;
  invalidated = instance.invalidated;
  primary_master = instance.primary_master;
  master_instance_uuid = instance.master_uuid;

  m_members.push_back(Instance(this, instance));
  // Hard code some values to make this more or less equivalent to a group
  // with a single member, which is the PRIMARY
  Instance &i = m_members.back();
  i.group_primary = true;
  i.in_majority = true;
}

Node_status Server::status() const {
  if (invalidated) {
    return Node_status::INVALIDATED;
  } else {
    const Instance &server = m_members.front();
    switch (server.status()) {
      case Instance_status::OK:
        if (!master_instance_uuid.empty()) {
          // SECONDARY checks
          if (!server.master_channel || !master_node_ptr) {
            return Node_status::ERROR;
          }
          // check that the actual source is the expected source
          if (server.master_channel->info.source_uuid !=
              master_node_ptr->get_primary_member()->uuid) {
            log_warning(
                "Instance %s is expected to have source %s (%s), but is %s:%i "
                "(%s)",
                label.c_str(), master_node_ptr->label.c_str(),
                master_node_ptr->get_primary_member()->uuid.c_str(),
                server.master_channel->info.host.c_str(),
                server.master_channel->info.port,
                server.master_channel->info.source_uuid.c_str());
            return Node_status::ERROR;
          }

          if (server.master_channel->info.status() ==
                  mysqlshdk::mysql::Replication_channel::OFF ||
              server.master_channel->info.status() ==
                  mysqlshdk::mysql::Replication_channel::APPLIER_OFF ||
              server.master_channel->info.status() ==
                  mysqlshdk::mysql::Replication_channel::RECEIVER_OFF)
            return Node_status::OFFLINE;
          else if (server.master_channel->info.status() !=
                       mysqlshdk::mysql::Replication_channel::CONNECTING &&
                   server.master_channel->info.status() !=
                       mysqlshdk::mysql::Replication_channel::ON)
            return Node_status::ERROR;

          // check for GTID set inconsistencies
          if (errant_transaction_count.get_safe() > 0) {
            return Node_status::INCONSISTENT;
          }

          if (!server.is_fenced()) return Node_status::ERROR;
        } else {
          // PRIMARY checks
          if (server.is_fenced()) return Node_status::ERROR;

          if (server.master_channel) return Node_status::ERROR;
        }

        return Node_status::ONLINE;

      default:
        return Node_status::UNREACHABLE;
    }
  }
}

Node_role Server::role() const {
  if (primary_master)
    return Node_role::PRIMARY;
  else
    return Node_role::SECONDARY;
}

void Server::scan(mysqlsh::dba::Instance *conn,
                  const std::string &channel_name) {
  Instance &instance = m_members.front();

  try {
    // instance state info
    instance.server_id = *conn->get_sysvar_int("server_id");

    instance.offline_mode = conn->get_sysvar_bool("offline_mode");
    instance.read_only = conn->get_sysvar_bool("read_only");
    instance.super_read_only = conn->get_sysvar_bool("super_read_only");

    instance.executed_gtid_set = mysqlshdk::mysql::get_executed_gtid_set(*conn);

    // find masters
    load_instance_channels(&instance, conn, channel_name);

    // find slaves
    // record all slaves as unmanaged for a start, since we don't know their
    // corresponding channel name before we scan them. The unmanaged list is
    // updated after determining that a slave is a managed replica.
    auto slaves = mysqlshdk::mysql::get_slaves(*conn);
    std::copy(slaves.begin(), slaves.end(),
              std::back_inserter(instance.unmanaged_replicas));
    if (!instance.unmanaged_replicas.empty()) {
      log_info("%s has %zi instances replicating from it",
               instance.label.c_str(), instance.unmanaged_replicas.size());
    }
  } catch (const shcore::Error &e) {
    current_console()->print_error(conn->descr() +
                                   ": could not query instance: " + e.format());
    instance.connect_errno = e.code();
    instance.connect_error = e.what();
  }
}

// -----------------------------------------------------------------------------

bool Global_topology::is_single_active() const {
  return m_topology_type == Global_topology_type::SINGLE_PRIMARY_TREE ||
         m_topology_type == Global_topology_type::NONE;
}

const Instance *Global_topology::try_find_member(
    const std::string &uuid) const {
  for (auto g : nodes()) {
    for (auto &i : g->members()) {
      if (i.uuid == uuid) return &i;
    }
  }
  return nullptr;
}

const Instance *Global_topology::find_member(const std::string &uuid) const {
  const Instance *i = try_find_member(uuid);
  if (!i)
    throw shcore::Exception("Metadata missing for member with uuid " + uuid,
                            SHERR_DBA_MEMBER_METADATA_MISSING);
  return i;
}

void Global_topology::load_instance_state(Instance *instance,
                                          mysqlsh::dba::Instance *conn) {
  Parallel_applier_options parallel_applier_options(*conn);

  instance->server_id = *conn->get_sysvar_int("server_id");

  instance->offline_mode = conn->get_sysvar_bool("offline_mode");
  instance->read_only = conn->get_sysvar_bool("read_only");
  instance->super_read_only = conn->get_sysvar_bool("super_read_only");

  instance->executed_gtid_set = mysqlshdk::mysql::get_executed_gtid_set(*conn);
  load_instance_channels(instance, conn, m_repl_channel_name);

  load_instance_slaves(instance, conn);

  // Set parallel-applier options into instance
  instance->parallel_appliers = parallel_applier_options.get_current_settings();
}

void Global_topology::load_instance_slaves(Instance *instance,
                                           mysqlsh::dba::Instance *conn) {
  std::vector<mysqlshdk::mysql::Slave_host> slaves(
      mysqlshdk::mysql::get_slaves(*conn));
  if (!slaves.empty()) {
    log_info("%s has %zi instances replicating from it",
             instance->label.c_str(), slaves.size());

    for (const auto &slave : slaves) {
      try {
        find_member(slave.uuid);
        // if member is already known, ignore it
      } catch (...) {
        // if member is not known, log and include it in the instance's info
        log_info("%s has an unmanaged replica instance %s:%i (%s)",
                 instance->label.c_str(), slave.host.c_str(), slave.port,
                 slave.uuid.c_str());

        instance->unmanaged_replicas.push_back(slave);
      }
    }
  }
}

Global_topology::Global_topology(const std::string &channel_name) {
  m_repl_channel_name = channel_name;
}

std::list<const Node *> Global_topology::get_available_nodes() const {
  std::list<const Node *> anodes;
  for (const Node *g : nodes()) {
    if (g->status() == Node_status::ONLINE && !g->invalidated)
      anodes.push_back(g);
  }
  return anodes;
}

const Node *Global_topology::try_get_node_for_uuid(
    const std::string &uuid) const {
  for (auto g : nodes()) {
    for (auto &i : g->members()) {
      if (i.uuid == uuid) return g;
    }
  }
  return nullptr;
}

const Node *Global_topology::get_primary_master_node() const {
  assert(is_single_active());

  for (const auto g : nodes()) {
    if (g->primary_master && !g->invalidated) return g;
  }

  throw shcore::Exception("The global topology has no valid PRIMARY defined",
                          SHERR_DBA_ACTIVE_CLUSTER_UNDEFINED);
}

void Global_topology::check_gtid_consistency(bool use_configured_primary) {
  auto ipool = current_ipool();

  if (nodes().size() <= 1) {
    return;
  }

  const Node *master_node = nullptr;
  // check that slaves have fewer transactions than the PRIMARY
  if (use_configured_primary) master_node = get_primary_master_node();

  for (Node *node : nodes()) {
    if (!use_configured_primary) master_node = node->master_node_ptr;

    if (master_node && master_node != node) {
      if (!master_node->get_primary_member()->executed_gtid_set.is_null() &&
          !node->get_primary_member()->executed_gtid_set.is_null()) {
        try {
          Scoped_instance master(ipool->connect_unchecked_endpoint(
              master_node->get_primary_member()->endpoint));

          std::string errant_gtids = master->queryf_one_string(
              0, "", "SELECT GTID_SUBTRACT(?, @@global.gtid_executed)",
              *node->get_primary_member()->executed_gtid_set);

          size_t gtid_diff_size =
              mysqlshdk::mysql::estimate_gtid_set_size(errant_gtids);

          log_debug("GTIDs that exist in %s but not its source %s: '%s' (%zi)",
                    node->label.c_str(), master_node->label.c_str(),
                    errant_gtids.c_str(), gtid_diff_size);

          node->errant_transactions = errant_gtids;
          node->errant_transaction_count = gtid_diff_size;
        } catch (const std::exception &e) {
          log_warning("Could not check GTID state of %s: %s",
                      node->label.c_str(), e.what());
        }
      }
    }
  }
}

// -----------------------------------------------------------------------------

const Server *Server_global_topology::get_server(Instance_id id) const {
  for (const auto &g : m_servers) {
    if (g.instance_id == id) return &g;
  }
  throw shcore::Exception(
      "Metadata missing for replicaset with id " + std::to_string(id),
      SHERR_DBA_CLUSTER_METADATA_MISSING);
}

const Server *Server_global_topology::get_server(
    const std::string &uuid) const {
  for (const auto &g : m_servers) {
    if (g.members().front().uuid == uuid) return &g;
  }
  throw shcore::Exception(
      "Metadata missing for instance with server_uuid " + uuid,
      SHERR_DBA_CLUSTER_METADATA_MISSING);
}

Server *Server_global_topology::server(Instance_id id) {
  for (auto &g : m_servers) {
    if (g.instance_id == id) return &g;
  }
  throw shcore::Exception(
      "Metadata missing for instance with id " + std::to_string(id),
      SHERR_DBA_CLUSTER_METADATA_MISSING);
}

std::list<const Node *> Server_global_topology::get_slave_nodes(
    const Node *master) const {
  std::list<const Node *> slaves;

  for (const auto &g : servers()) {
    if (g.master_node_ptr == master) slaves.push_back(&g);
  }

  return slaves;
}

const topology::Server *Server_global_topology::find_failover_candidate(
    const std::list<Scoped_instance> &candidates) const {
  size_t best_gtid_set_size = 0;
  const topology::Server *best_server = nullptr;
  auto console = current_console();

  // Find server with most up-to-date gtid set
  for (const auto &c : candidates) {
    auto g = get_server(c->get_uuid());
    // NOTE: Use the current (up-to-date) GTID_EXECUTED set and not the one
    //       "cached" by the Server_global_topology object which might not
    //       include the GTIDs applied from the relay log during the operation.
    std::string gtid_set = mysqlshdk::mysql::get_executed_gtid_set(c);

    if (!gtid_set.empty()) {
      console->print_info(g->label + " has GTID set " + gtid_set);

      size_t size = mysqlshdk::mysql::estimate_gtid_set_size(gtid_set);
      if (size > best_gtid_set_size || (size > 0 && best_server == nullptr)) {
        best_gtid_set_size = size;
        best_server = g;
      }
    } else {
      console->print_info("Could not determine GTID set for " + g->label);
    }
  }

  if (!best_server)
    log_warning("Could not find any suitable failover candidates");
  else
    log_debug("Server with the largest GTID_EXECUTED set is %s",
              best_server->label.c_str());

  return best_server;
}

void Server_global_topology::refresh(MetadataStorage *metadata, bool deep) {
  assert(!m_nodes.empty());

  std::string uuid = m_servers.front().members().front().uuid;
  m_servers.clear();
  m_nodes.clear();

  load(metadata, uuid);

  check_servers(deep);
}

void Server_global_topology::load(MetadataStorage *metadata,
                                  const std::string &uuid) {
  Cluster_metadata cluster;
  if (metadata->get_cluster_for_server_uuid(uuid, &cluster)) {
    load_cluster(metadata, cluster);
  } else {
    throw shcore::Exception(
        "Metadata missing for instance with server_uuid " + uuid,
        SHERR_DBA_CLUSTER_METADATA_MISSING);
  }
}

void Server_global_topology::load_cluster(MetadataStorage *metadata,
                                          const Cluster_metadata &cluster) {
  m_topology_type = cluster.async_topology_type;

  std::vector<Instance_metadata> instances(
      metadata->get_replica_set_instances(cluster.cluster_id));
  log_debug("%zi instances in replicaset %s", instances.size(),
            cluster.full_cluster_name().c_str());

  for (const auto &instance : instances) {
    add_server(cluster, instance);
  }
}

Server &Server_global_topology::add_server(const Cluster_metadata &cluster,
                                           const Instance_metadata &instance) {
  m_servers.emplace_back(cluster, instance);
  Server &server = m_servers.back();
  m_nodes.push_back(&server);
  return server;
}

void Server_global_topology::remove_server(const Server &server) {
  m_nodes.remove_if([&server](const Node *n) { return n == &server; });
  m_servers.remove_if([&server](const Server &s) { return &s == &server; });
}

/**
 * Checks state of the cluster that owns given group and its members.
 * Connects to all instances in the cluster until a quorum is found.
 *
 * If deep is true, then the state of each individual member will be checked,
 * in addition to the state of the group as a whole.
 */
void Server_global_topology::check_server(Instance_id id, bool /*deep*/) {
  Server *server = this->server(id);

  log_debug("Scanning state of replicaset %s", server->label.c_str());

  // Note: ipool methods that require metadata should not be called here
  auto ipool = current_ipool();

  for (auto &member : server->m_members) {
    log_debug("Connecting to %s", member.label.c_str());
    Scoped_instance minstance;
    try {
      minstance =
          Scoped_instance(ipool->connect_unchecked_endpoint(member.endpoint));
    } catch (shcore::Exception &e) {
      log_warning("Could not connect to %s: %s", member.label.c_str(),
                  e.format().c_str());
      if (e.is_mysql()) {
        member.connect_errno = e.code();
        member.connect_error = e.what();
      } else {
        throw;
      }
      continue;
    }

    // Load state of the member
    try {
      load_instance_state(&member, minstance.get());
    } catch (shcore::Error &e) {
      log_error("Error querying from %s: %s", member.label.c_str(),
                e.format().c_str());
      member.connect_errno = e.code();
      member.connect_error = e.what();
    }
  }
}

void Server_global_topology::check_servers(bool deep) {
  for (Server &g : m_servers) {
    check_server(g.instance_id, deep);
  }

  // resolve cross-references across groups
  for (Server &s : m_servers) {
    for (Instance &i : s.m_members) {
      if (i.master_channel) {
        const auto &ch = i.master_channel->info;
        if (!ch.source_uuid.empty()) {
          // this one should point to the actual master
          i.master_channel->master_ptr = try_find_member(ch.source_uuid);
          if (!i.master_channel->master_ptr)
            log_error("%s: replication source %s:%i (%s) is not known",
                      i.label.c_str(), ch.host.c_str(), ch.port,
                      ch.source_uuid.c_str());
        }
      }

      if (!s.master_instance_uuid.empty()) {
        // this one should point to the expected master
        s.master_node_ptr = get_server(s.master_instance_uuid);
      }
    }
  }
}

/**
 * Attempt to discover the full topology of an async replication setup.
 *
 * This will recursively traverse for all masters (show slave status) and slaves
 * (show slave hosts) and try to come up with the full replication topology.
 * Only the default replication channel ("") will be recursively searched,
 * although other channels will be recorded too.
 *
 * Notes:
 * - The topology may be any arbitrary graph and not necessarily a flat tree
 * - The initial instance can be any arbitrary node in the graph
 * - SHOW SLAVE HOSTS is unreliable in that it can show an entry for instances
 * that are not longer replicating from it. These ghost slaves must be ignored.
 */
void Server_global_topology::discover_from_unmanaged(dba::Instance *instance) {
  scan_instance_recursive(instance);
}

namespace {
enum class Slave_check_status { Slave, Unsupported_slave, Not_slave };

Slave_check_status check_slave_channel(
    const mysqlshdk::mysql::IInstance &instance,
    const mysqlshdk::mysql::IInstance &master,
    const std::string &channel_name) {
  auto channels = mysqlshdk::mysql::get_incoming_channels(instance);
  bool found = false;
  for (const auto &ch : channels) {
    if (ch.source_uuid == master.get_uuid()) {
      found = true;
      if (ch.channel_name == channel_name) {
        return Slave_check_status::Slave;
      }
    }
  }
  if (found)
    return Slave_check_status::Unsupported_slave;
  else
    return Slave_check_status::Not_slave;
}
}  // namespace

Server *Server_global_topology::scan_instance_recursive(
    dba::Instance *instance) {
  auto console = current_console();
  auto ipool = current_ipool();

  console->print_info("** Scanning state of instance " + instance->descr());

  auto add_instance = [&](dba::Instance *new_instance) {
    Instance_metadata md;
    md.label = new_instance->get_canonical_address();
    md.uuid = new_instance->get_uuid();
    md.endpoint = new_instance->get_canonical_address();

    Server &server = add_server(Cluster_metadata(), md);
    server.instance_id = m_servers.size() + 1;
    server.label = md.endpoint;

    return &server;
  };

  Server *server = add_instance(instance);

  // Query instance info and the list of its peers
  server->scan(instance, m_repl_channel_name);

  topology::Instance *member = server->primary_member();

  // Run discovery on master recursively
  if (member->master_channel) {
    // Check first if the master is already known
    if (!try_find_member(member->master_channel->info.source_uuid)) {
      std::string endpoint = mysqlshdk::utils::make_host_and_port(
          member->master_channel->info.host, member->master_channel->info.port);

      log_info("%s replicates from %s", instance->descr().c_str(),
               endpoint.c_str());

      Scoped_instance master_conn;
      try {
        master_conn =
            Scoped_instance(ipool->connect_unchecked_endpoint(endpoint));
      } catch (const shcore::Exception &e) {
        console->print_error(shcore::str_format(
            "Could not connect to %s (source of %s): %s", endpoint.c_str(),
            instance->descr().c_str(), e.format().c_str()));

        throw;
      }

      Server *master_server = scan_instance_recursive(master_conn.get());
      member->master_channel->master_ptr = master_server->get_primary_member();
      member->node_ptr->master_node_ptr = master_server;
    }
  }

  // Run discovery on slaves
  std::list<mysqlshdk::mysql::Slave_host>::iterator next_it =
      member->unmanaged_replicas.begin();
  while (next_it != member->unmanaged_replicas.end()) {
    auto slave_it = next_it++;
    const auto &slave = *slave_it;

    std::string endpoint =
        mysqlshdk::utils::make_host_and_port(slave.host, slave.port);

    // Connect to the slave and scan it
    Scoped_instance slave_conn;
    try {
      slave_conn = Scoped_instance(ipool->connect_unchecked_endpoint(endpoint));
    } catch (const shcore::Exception &e) {
      console->print_error(shcore::str_format(
          "Could not connect to %s (slave of %s): %s", endpoint.c_str(),
          instance->descr().c_str(), e.format().c_str()));

      throw;
    }

    // Check if the instance we got from show slave hosts is actually a slave
    bool skip = false;
    switch (check_slave_channel(*slave_conn, *instance, m_repl_channel_name)) {
      case Slave_check_status::Unsupported_slave:
        skip = true;
        break;
      case Slave_check_status::Slave:
        break;
      case Slave_check_status::Not_slave:
        // this is a ghost slave
        skip = true;
        member->unmanaged_replicas.erase(slave_it);
        break;
    }
    if (skip) continue;

    // Check first if the slave is already known and if so, don't scan it again
    if (auto slave_member = try_find_member(slave.uuid)) {
      if (slave_member->master_channel &&
          slave_member->master_channel->info.source_uuid == member->uuid) {
        slave_member->master_channel->master_ptr = member;
        slave_member->node_ptr->master_node_ptr = member->node_ptr;
      }
      member->unmanaged_replicas.erase(slave_it);
      continue;
    }

    // Scan the slave
    Server *slave_server = scan_instance_recursive(slave_conn.get());
    topology::Instance *slave_member = slave_server->primary_member();

    // The slave's default channel master is us, so this is a manageable
    // replica. Update master_ptr and remove it from the list of unmanaged
    // replicas
    slave_member->master_channel->master_ptr = member;
    slave_member->node_ptr->master_node_ptr = member->node_ptr;

    member->unmanaged_replicas.erase(slave_it);
  }

  return server;
}

void Server_global_topology::show_raw() const {
  auto console = current_console();

  for (const auto &server : servers()) {
    const Instance *instance = server.get_primary_member();

    console->print_info(shcore::str_format(
        "- %s: uuid=%s read_only=%s", server.label.c_str(),
        instance->uuid.c_str(), instance->is_fenced() ? "yes" : "no"));
    if (instance->connect_errno != 0) {
      console->print_info("    - Unreachable: " + instance->connect_error);
      continue;
    }
    if (instance->master_channel) {
      console->print_info(shcore::str_format(
          "    - replicates from %s",
          instance->master_channel->master_ptr->label.c_str()));
      console->print_info(
          "\t" + shcore::str_join(shcore::str_split(
                                      mysqlshdk::mysql::format_status(
                                          instance->master_channel->info, true),
                                      "\n"),
                                  "\n\t"));
    }

    for (const auto &channel : instance->unmanaged_channels) {
      console->print_info(shcore::str_format(
          "    - replicates from %s:%i through unsupported channel '%s'",
          channel.host.c_str(), channel.port, channel.channel_name.c_str()));
    }
    for (const auto &slave : instance->unmanaged_replicas) {
      console->print_info(
          shcore::str_format("    - has a replica %s:%i through an unsupported "
                             "replication channel",
                             slave.host.c_str(), slave.port));
    }
  }
}

// -----------------------------------------------------------------------------

Global_topology *scan_global_topology(MetadataStorage *metadata,
                                      const Cluster_metadata &cmd,
                                      const std::string &channel_name,
                                      bool deep) {
  if (cmd.type == Cluster_type::GROUP_REPLICATION) {
    throw std::logic_error("internal error");
  } else {
    std::unique_ptr<Server_global_topology> topo(
        new Server_global_topology(channel_name));

    topo->load_cluster(metadata, cmd);
    topo->check_servers(deep);
    if (deep) topo->check_gtid_consistency();

    return topo.release();
  }
}

}  // namespace topology
}  // namespace dba
}  // namespace mysqlsh
