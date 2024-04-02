/*
 * Copyright (c) 2019, 2024, Oracle and/or its affiliates.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.0,
 * as published by the Free Software Foundation.
 *
 * This program is designed to work with certain software (including
 * but not limited to OpenSSL) that is licensed under separate terms,
 * as designated in a particular file or component or in included license
 * documentation.  The authors of MySQL hereby grant you an additional
 * permission to link the program and your derivative works with the
 * separately licensed software that they have either included with
 * the program or referenced in the documentation.
 *
 * This program is distributed in the hope that it will be useful,  but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 * the GNU General Public License, version 2.0, for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "modules/adminapi/replica_set/status.h"

#include <string>
#include <utility>
#include <vector>

#include "modules/adminapi/common/async_topology.h"
#include "modules/adminapi/common/common_status.h"
#include "modules/adminapi/common/global_topology.h"
#include "modules/adminapi/common/parallel_applier_options.h"
#include "modules/adminapi/common/replication_account.h"
#include "modules/adminapi/replica_set/replica_set_impl.h"
#include "mysqlshdk/libs/utils/utils_net.h"
#include "mysqlshdk/libs/utils/utils_string.h"

namespace mysqlsh::dba::replicaset {

namespace {

void instance_diagnostics(shcore::Dictionary_t status,
                          const topology::Server &server,
                          const topology::Instance &instance) {
  shcore::Array_t issues = shcore::make_array();

  // invalidated member
  if (server.invalidated) {
    issues->push_back(
        shcore::Value("WARNING: Instance was INVALIDATED and must be rejoined "
                      "or removed from the replicaset."));
  }

  // PRIMARY that's SRO
  if (server.primary_master && instance.is_fenced()) {
    std::vector<std::string> vars;

    if (instance.read_only.value_or(false)) vars.push_back("read_only=ON");
    if (instance.super_read_only.value_or(false))
      vars.push_back("super_read_only=ON");
    if (instance.offline_mode.value_or(false))
      vars.push_back("offline_mode=ON");

    issues->push_back(
        shcore::Value("ERROR: Instance is a PRIMARY but is READ-ONLY: " +
                      shcore::str_join(vars, ", ")));
  }

  // SECONDARY that's not SRO
  if (!server.primary_master && !instance.super_read_only.value_or(false)) {
    issues->push_back(shcore::Value(
        "ERROR: Instance is NOT a PRIMARY but super_read_only "
        "option is OFF. Accidental updates to this instance are possible "
        "and will cause inconsistencies in the replicaset."));
  }

  // GTID consistency
  if (server.errant_transaction_count.value_or(0) > 0) {
    issues->push_back(shcore::Value(shcore::str_format(
        "ERROR: %s errant transaction(s) detected. Topology changes will not "
        "be possible until the instance is removed from the replicaset to "
        "have the inconsistency repaired.",
        std::to_string(*server.errant_transaction_count).c_str())));
  }

  if (server.primary_master) {
    if (instance.master_channel.get()) {
      issues->push_back(
          shcore::Value("ERROR: Instance is a PRIMARY but is replicating from "
                        "another instance."));
    }
  } else {
    const topology::Channel *channel = instance.master_channel.get();

    if (channel) {
      switch (channel->info.status()) {
        case mysqlshdk::mysql::Replication_channel::Status::ON:
          break;

        case mysqlshdk::mysql::Replication_channel::Status::CONNECTING:
          issues->push_back(
              shcore::Value("NOTE: Replication I/O thread is reconnecting."));
          break;

        case mysqlshdk::mysql::Replication_channel::Status::OFF:
          issues->push_back(shcore::Value("ERROR: Replication is stopped."));
          break;

        case mysqlshdk::mysql::Replication_channel::Status::RECEIVER_OFF:
          issues->push_back(shcore::Value(
              "ERROR: Replication I/O thread (receiver) is stopped."));
          break;

        case mysqlshdk::mysql::Replication_channel::Status::APPLIER_OFF:
          issues->push_back(shcore::Value(
              "ERROR: Replication SQL thread (applier) is stopped."));
          break;

        case mysqlshdk::mysql::Replication_channel::Status::CONNECTION_ERROR:
          issues->push_back(
              shcore::Value("ERROR: Replication I/O thread (receiver) has "
                            "stopped with an error."));
          break;

        case mysqlshdk::mysql::Replication_channel::Status::APPLIER_ERROR:
          issues->push_back(
              shcore::Value("ERROR: Replication SQL thread (applier) has "
                            "stopped with an error."));
          break;
      }
    }

    // Wrong master
    if (!server.invalidated) {
      if (!channel) {
        if (!server.master_node_ptr)
          issues->push_back(shcore::Value(
              "ERROR: Replication source channel is not configured."));
        else
          issues->push_back(shcore::Value(
              shcore::str_format("ERROR: Replication source channel is not "
                                 "configured, should be %s.",
                                 server.master_node_ptr->label.c_str())));
      } else if (channel->info.source_uuid != server.master_instance_uuid) {
        issues->push_back(shcore::Value(shcore::str_format(
            "ERROR: Replication source misconfigured. "
            "Expected %s but is %s:%i.",
            !server.master_node_ptr ? "<metadata error>"
                                    : server.master_node_ptr->label.c_str(),
            channel->info.host.c_str(), channel->info.port)));
      }
    }

    // Bogus slave
    for (const auto &ch : instance.unmanaged_channels) {
      issues->push_back(
          shcore::Value("ERROR: Invalid/unexpected replication channel: " +
                        mysqlshdk::mysql::format_status(ch)));
    }
  }

  // Check if parallel-appliers are not configured. The requirement was
  // introduced in 8.0.23 so only check if the version is equal or higher to
  // that.
  if (instance.version >= mysqlshdk::utils::Version(8, 0, 23)) {
    Parallel_applier_options parallel_applier_aux;
    auto current_values = instance.parallel_appliers;
    auto required_values =
        parallel_applier_aux.get_required_values(instance.version);

    for (const auto &[name, value] : required_values) {
      auto current_value = current_values[name].value_or("");

      if (!current_value.empty() && current_value != value) {
        issues->push_back(shcore::Value(
            "NOTE: The required parallel-appliers settings are not enabled on "
            "the instance. Use dba.configureReplicaSetInstance() to fix it."));
        break;
      }
    }
  }

  if (!issues->empty()) {
    status->set("instanceErrors", shcore::Value(issues));
  }
}

shcore::Dictionary_t server_status(const topology::Server &server,
                                   int show_details) {
  const topology::Instance *instance = server.get_primary_member();

  shcore::Dictionary_t status = shcore::make_dict();

  status->set("address", shcore::Value(instance->endpoint));

  if (instance->hidden_from_router) {
    status->set("hiddenFromRouter", shcore::Value(true));
  }

  // mode vs groupRole:
  // groupRole is the GR role in the group
  // mode is the aggregation of:
  // - group role
  // - read_only/super_read_only (which should be affected by whether the
  // cluster is the active)
  // - whether member is ONLINE
  // - whether member is in the majority group
  {
    bool read_write = true;

    status->set("status", shcore::Value(to_string(server.status())));

    if (instance->connect_error.empty()) {
      if (instance->is_fenced()) read_write = false;
      bool is_primary = server.role() == topology::Node_role::PRIMARY;

      // only show fenced if details requested or if the value is unexpected
      if (show_details || (server.status() != topology::Node_status::ONLINE) ||
          (is_primary && !read_write) || (!is_primary && read_write))
        status->set("fenced", instance->is_fenced() ? shcore::Value::True()
                                                    : shcore::Value::False());
    } else {
      status->set("fenced", shcore::Value::Null());
      status->set("connectError", shcore::Value(instance->connect_error));
    }

    // This should match the view the router would have of the instance.
    // Since the router doesn't consider replication state, we can't consider
    // that here either.
    if (server.status() != topology::Node_status::INVALIDATED &&
        server.status() != topology::Node_status::UNREACHABLE)
      status->set("mode", shcore::Value(read_write ? "R/W" : "R/O"));
    else
      status->set("mode", shcore::Value::Null());
  }

  if (!server.invalidated)
    status->set("instanceRole", shcore::Value(to_string(server.role())));
  else
    status->set("instanceRole", shcore::Value::Null());

  if (show_details) {
    shcore::Array_t array = shcore::make_array();
    if (instance->read_only.value_or(false))
      array->push_back(shcore::Value("read_only"));
    if (instance->super_read_only.value_or(false))
      array->push_back(shcore::Value("super_read_only"));
    if (instance->offline_mode.value_or(false))
      array->push_back(shcore::Value("offline_mode"));

    status->set("fenceSysVars", shcore::Value(array));
    status->set("serverUuid", shcore::Value(instance->uuid));

    if (!instance->unmanaged_channels.empty()) {
      shcore::Array_t unknown_channels = shcore::make_array();
      for (const auto &channel : instance->unmanaged_channels) {
        unknown_channels->push_back(shcore::Value(shcore::make_dict(
            "channel", shcore::Value(channel.channel_name), "source",
            shcore::Value(mysqlshdk::utils::make_host_and_port(
                channel.host, channel.port)))));
      }
      status->set("unknownChannels", shcore::Value(unknown_channels));
    }

    if (!instance->unmanaged_replicas.empty()) {
      shcore::Array_t unknown_replicas = shcore::make_array();
      for (const auto &slave : instance->unmanaged_replicas) {
        unknown_replicas->push_back(shcore::Value(
            mysqlshdk::utils::make_host_and_port(slave.host, slave.port)));
      }
      status->set("unknownReplicas", shcore::Value(unknown_replicas));
    }
  }

  if (instance->connect_error.empty()) {
    const topology::Channel *channel = instance->master_channel.get();

    if ((!server.primary_master && !server.invalidated) || channel) {
      std::string expected_source;

      if (!server.primary_master) {
        expected_source = instance->node_ptr->master_node_ptr
                              ? instance->node_ptr->master_node_ptr->label
                              : "";
      }

      status->set("replication",
                  shcore::Value(channel_status(
                      channel ? &channel->info : nullptr,
                      channel ? &channel->master_info : nullptr,
                      channel ? &channel->relay_log_info : nullptr,
                      expected_source, show_details)));
    }

    if (!server.master_instance_uuid.empty() || server.invalidated) {
      shcore::Value txstatus;
      size_t count = 0;

      // only show consistency status info if there's something wrong
      if (server.errant_transaction_count.has_value()) {
        count = *server.errant_transaction_count;
        txstatus = shcore::Value(count == 0 ? "OK" : "INCONSISTENT");
        if (count > 0) {
          status->set(
              "transactionSetConsistencyStatusText",
              shcore::Value(shcore::str_format(
                  "There are %zi transactions that were executed in this "
                  "instance that did not originate from the PRIMARY.",
                  count)));

          status->set("transactionSetErrantGtidSet",
                      shcore::Value(server.errant_transactions));
        }
      } else {
        txstatus = shcore::Value::Null();
      }
      if (server.status() != topology::Node_status::ONLINE || count > 0 ||
          show_details) {
        status->set("transactionSetConsistencyStatus", txstatus);
      }
    }

    instance_diagnostics(status, server, *instance);
  }

  return status;
}

std::pair<Global_availability_status, std::string> global_availability_status(
    const topology::Global_topology &topology) {
  int num_ok_actives = 0;
  int num_ok_passives = 0;
  int num_reachable_instances = 0;

  for (const topology::Node *n : topology.nodes()) {
    if (n->primary_master) {
      if (n->status() == topology::Node_status::ONLINE) {
        num_ok_actives++;
        num_reachable_instances++;
      } else {
        for (const topology::Instance &i : n->members()) {
          if (i.status() == Instance_status::OK) num_reachable_instances++;
        }
      }
    } else {
      if (n->status() == topology::Node_status::ONLINE) {
        num_ok_passives++;
        num_reachable_instances++;
      } else {
        for (const topology::Instance &i : n->members()) {
          if (i.status() == Instance_status::OK) num_reachable_instances++;
        }
      }
    }
  }

  Global_availability_status status;
  if (num_ok_actives > 0) {
    if (num_ok_actives + num_ok_passives ==
        static_cast<int>(topology.nodes().size()))
      status = Global_availability_status::AVAILABLE;
    else
      status = Global_availability_status::AVAILABLE_PARTIAL;
  } else if (num_reachable_instances >= 1) {
    status = Global_availability_status::UNAVAILABLE;
  } else {
    status = Global_availability_status::CATASTROPHIC;
  }

  std::string explain;
  switch (status) {
    case Global_availability_status::CATASTROPHIC:
      explain = "No instances are available or reachable.";
      break;
    case Global_availability_status::UNAVAILABLE:
      explain =
          "PRIMARY instance is not available, but there is at least "
          "one SECONDARY that could be force-promoted.";
      break;
    case Global_availability_status::AVAILABLE_PARTIAL:
      explain =
          "The PRIMARY instance is available, but one or more SECONDARY "
          "instances are not.";
      break;
    case Global_availability_status::AVAILABLE:
      explain = "All instances available.";
      break;
  }
  return {status, explain};
}

shcore::Dictionary_t cluster_status(
    const topology::Server_global_topology &topology, int show_details) {
  shcore::Dictionary_t topo_status = shcore::make_dict();

  for (const topology::Server &server : topology.servers()) {
    topo_status->set(server.label,
                     shcore::Value(server_status(server, show_details)));
  }

  Global_availability_status astatus;
  std::string astatus_text;

  std::tie(astatus, astatus_text) = global_availability_status(topology);

  const topology::Node *primary = topology.get_primary_master_node();

  return shcore::make_dict(
      "type", shcore::Value("ASYNC"),                        //
      "topology", shcore::Value(std::move(topo_status)),     //
      "status", shcore::Value(to_string(astatus)),           //
      "statusText", shcore::Value(std::move(astatus_text)),  //
      "primary", primary ? shcore::Value(primary->label) : shcore::Value());
}

}  // namespace

shcore::Value Status::do_run(int extended) {
  auto status = shcore::make_dict();

  const auto &md_storage = m_rset.get_metadata_storage();

  // topology info
  {
    Cluster_metadata cmd = m_rset.get_metadata();

    auto topo = topology::scan_global_topology(md_storage.get(), cmd,
                                               k_replicaset_channel_name, true);

    status->set(
        "replicaSet",
        shcore::Value(cluster_status(
            topo->as<const topology::Server_global_topology>(), extended)));

    status->get_map("replicaSet")
        ->set("name", shcore::Value(m_rset.get_name()));

    auto get_instance_errors = [](const shcore::Value::Map_type &topo_status,
                                  const std::string &endpoint) {
      auto topo_errors = topo_status.get_map(endpoint);
      if (!topo_errors->has_key("instanceErrors"))
        topo_errors->set("instanceErrors", shcore::Value(shcore::make_array()));

      return topo_errors->get_array("instanceErrors", nullptr);
    };

    // check for replication options mismatch
    for (const topology::Server &server :
         topo->as<const topology::Server_global_topology>().servers()) {
      auto i = server.get_primary_member();
      if (!i->master_channel) continue;

      auto topo_status = status->get_map("replicaSet")->get_map("topology");
      if (!topo_status->has_key(i->endpoint)) continue;

      // SSL channel mode
      {
        std::string_view ssl_mode = kClusterSSLModeDisabled;
        if (const auto &info = i->master_channel->master_info;
            info.enabled_ssl != 0) {
          bool certs_enabled = !info.ssl_ca.empty() || !info.ssl_capath.empty();
          if (info.ssl_verify_server_cert) {
            assert(certs_enabled);
            ssl_mode = kClusterSSLModeVerifyIdentity;
          } else {
            ssl_mode = certs_enabled ? kClusterSSLModeVerifyCA
                                     : kClusterSSLModeRequired;
          }
        }

        auto target_map =
            topo_status->get_map(i->endpoint)->get_map("replication");

        target_map->set("replicationSslMode", shcore::Value(ssl_mode));
      }

      // check for repl options inconsistencies
      {
        bool has_null_options;
        Async_replication_options ar_options;
        m_rset.read_replication_options(i->uuid, &ar_options,
                                        &has_null_options);

        bool not_configured{true};
        if (!has_null_options) {
          auto options_to_update = async_merge_repl_options(
              ar_options, i->master_channel->master_info);

          if (!options_to_update.has_value()) {
            not_configured = false;
          }
        }

        if (not_configured)
          get_instance_errors(*topo_status, i->endpoint)
              ->push_back(shcore::Value(
                  "WARNING: The instance replication channel is not configured "
                  "as expected (to see the affected options, use options()). "
                  "Please call rejoinInstance() to reconfigure the channel "
                  "accordingly."));
      }

      // check replication account
      {
        auto [md_account_user, md_account_host] =
            md_storage->get_instance_repl_account(
                i->uuid, Cluster_type::ASYNC_REPLICATION, Replica_type::NONE);

        if (md_account_user.empty() || md_account_host.empty()) {
          get_instance_errors(*topo_status, i->endpoint)
              ->push_back(shcore::Value(
                  "ERROR: the instance is missing its replication account info "
                  "from the metadata. Please call rescan() to fix this."));
        }

        auto intended_account_user =
            Replication_account::make_replication_user_name(
                i->server_id.value_or(0),
                Replication_account::k_async_recovery_user_prefix);

        if (intended_account_user != i->master_channel->master_info.user_name)
          get_instance_errors(*topo_status, i->endpoint)
              ->push_back(shcore::Value(shcore::str_format(
                  "WARNING: Incorrect recovery account (%s) being used. Use "
                  "ReplicaSet.rescan() to repair.",
                  i->master_channel->master_info.user_name.c_str())));

        if (!md_account_user.empty() &&
            (md_account_user != i->master_channel->master_info.user_name))
          get_instance_errors(*topo_status, i->endpoint)
              ->push_back(shcore::Value(shcore::str_format(
                  "WARNING: Stored recovery account (%s) doesn't match the one "
                  "actually in use (%s). Use ReplicaSet.rescan() to repair.",
                  md_account_user.c_str(),
                  i->master_channel->master_info.user_name.c_str())));
      }
    }
  }

  // gets the async replication channels SSL info
  scan_async_channel_ssl_info(status);

  // Gets the metadata version
  if (extended >= 1) {
    auto version =
        mysqlsh::dba::metadata::installed_version(md_storage->get_md_server());
    status->set("metadataVersion", shcore::Value(version.get_base()));
  }

  return shcore::Value(status);
}

void Status::scan_async_channel_ssl_info(shcore::Dictionary_t status) {
  const auto &md_storage = m_rset.get_metadata_storage();
  auto instances = md_storage->get_replica_set_instances(m_rset.get_id());

  auto topo = status->get_map("replicaSet")->get_map("topology");

  for (const auto &i : instances) {
    if (!topo->has_key(i.endpoint)) continue;
    if (!topo->get_map(i.endpoint)->has_key("replication")) continue;

    auto repl_account = md_storage->get_instance_repl_account_user(
        i.uuid, Cluster_type::ASYNC_REPLICATION, Replica_type::NONE);

    std::string ssl_cipher, ssl_version;
    mysqlshdk::mysql::iterate_thread_variables(
        *(md_storage->get_md_server()), "Binlog Dump GTID", repl_account,
        "Ssl%",
        [&ssl_cipher, &ssl_version](const std::string &var_name,
                                    std::string var_value) {
          if (var_name == "Ssl_cipher")
            ssl_cipher = std::move(var_value);
          else if (var_name == "Ssl_version")
            ssl_version = std::move(var_value);

          return (ssl_cipher.empty() || ssl_version.empty());
        });

    if (ssl_cipher.empty() && ssl_version.empty()) continue;

    auto target_map = topo->get_map(i.endpoint)->get_map("replication");
    assert(target_map->has_key("replicationSsl"));

    if (ssl_cipher.empty())
      (*target_map)["replicationSsl"] = shcore::Value(std::move(ssl_version));
    else if (ssl_version.empty())
      (*target_map)["replicationSsl"] = shcore::Value(std::move(ssl_cipher));
    else
      (*target_map)["replicationSsl"] = shcore::Value(
          shcore::str_format("%s %s", ssl_cipher.c_str(), ssl_version.c_str()));
  }
}

}  // namespace mysqlsh::dba::replicaset
