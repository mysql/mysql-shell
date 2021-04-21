/*
 * Copyright (c) 2019, 2021, Oracle and/or its affiliates.
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

#include "modules/adminapi/replica_set/replica_set_status.h"

#include <string>
#include <utility>
#include <vector>

#include "modules/adminapi/common/base_cluster_impl.h"
#include "modules/adminapi/common/common_status.h"
#include "modules/adminapi/common/dba_errors.h"
#include "modules/adminapi/common/parallel_applier_options.h"
#include "mysqlshdk/include/shellcore/console.h"
#include "mysqlshdk/libs/utils/strformat.h"
#include "mysqlshdk/libs/utils/utils_net.h"
#include "mysqlshdk/libs/utils/utils_string.h"

namespace mysqlsh {
namespace dba {

namespace {

void instance_diagnostics(shcore::Dictionary_t status,
                          const topology::Server &server,
                          const topology::Instance &instance) {
  shcore::Array_t issues = shcore::make_array();

  // invalidated member
  if (server.invalidated) {
    issues->push_back(
        shcore::Value("WARNING: Instance was INVALIDATED and must be removed "
                      "from the replicaset."));
  }

  // PRIMARY that's SRO
  if (server.primary_master && instance.is_fenced()) {
    std::vector<std::string> vars;

    if (instance.read_only.get_safe()) vars.push_back("read_only=ON");
    if (instance.super_read_only.get_safe())
      vars.push_back("super_read_only=ON");
    if (instance.offline_mode.get_safe()) vars.push_back("offline_mode=ON");

    issues->push_back(
        shcore::Value("ERROR: Instance is a PRIMARY but is READ-ONLY: " +
                      shcore::str_join(vars, ", ")));
  }

  // SECONDARY that's not SRO
  if (!server.primary_master && !instance.super_read_only.get_safe()) {
    issues->push_back(shcore::Value(
        "ERROR: Instance is NOT a PRIMARY but super_read_only "
        "option is OFF. Accidental updates to this instance are possible "
        "and will cause inconsistencies in the replicaset."));
  }

  // GTID consistency
  if (server.errant_transaction_count.get_safe() > 0) {
    issues->push_back(shcore::Value(shcore::str_format(
        "ERROR: %s errant transaction(s) detected. Topology changes will not "
        "be possible until the instance is removed from the replicaset to "
        "have the inconsistency repaired.",
        std::to_string(server.errant_transaction_count.get_safe()).c_str())));
  }

  if (server.primary_master) {
    const topology::Channel *channel = instance.master_channel.get();
    if (channel) {
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

    for (const auto &setting : required_values) {
      std::string current_value =
          current_values[std::get<0>(setting)].get_safe();

      if (!current_value.empty() && current_value != std::get<1>(setting)) {
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
                                   const Status_options &opts) {
  const topology::Instance *instance = server.get_primary_member();

  shcore::Dictionary_t status = shcore::make_dict();

  status->set("address", shcore::Value(instance->endpoint));
  // mode vs groupRole:
  // groupRole is the GR role in the group
  // mode is the aggregation of:
  // - group role
  // - read_only/super_read_only (which should be affected by whether the
  // cluster is the active)
  // - whether member is ONLINE
  // - whether member is in the majority group
  bool read_write = true;

  status->set("status", shcore::Value(to_string(server.status())));

  if (instance->connect_error.empty()) {
    if (instance->is_fenced()) read_write = false;
    bool is_primary = server.role() == topology::Node_role::PRIMARY;

    // only show fenced if details requested or if the value is unexpected
    if (opts.show_details ||
        (server.status() != topology::Node_status::ONLINE) ||
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

  if (!server.invalidated)
    status->set("instanceRole", shcore::Value(to_string(server.role())));
  else
    status->set("instanceRole", shcore::Value::Null());

  if (opts.show_details) {
    shcore::Array_t array = shcore::make_array();
    if (instance->read_only.get_safe())
      array->push_back(shcore::Value("read_only"));
    if (instance->super_read_only.get_safe())
      array->push_back(shcore::Value("super_read_only"));
    if (instance->offline_mode.get_safe())
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
                      expected_source, opts.show_details)));
    }

    if (!server.master_instance_uuid.empty() || server.invalidated) {
      shcore::Value txstatus;
      size_t count = 0;

      // only show consistency status info if there's something wrong
      if (!server.errant_transaction_count.is_null()) {
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
          opts.show_details) {
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
    const topology::Server_global_topology &topology,
    const Status_options &opts) {
  shcore::Dictionary_t topo_status = shcore::make_dict();

  for (const topology::Server &server : topology.servers()) {
    topo_status->set(server.label, shcore::Value(server_status(server, opts)));
  }

  Global_availability_status astatus;
  std::string astatus_text;

  std::tie(astatus, astatus_text) = global_availability_status(topology);

  const topology::Node *primary = topology.get_primary_master_node();

  return shcore::make_dict(
      "type", shcore::Value("ASYNC"),               //
      "topology", shcore::Value(topo_status),       //
      "status", shcore::Value(to_string(astatus)),  //
      "statusText", shcore::Value(astatus_text),    //
      "primary", primary ? shcore::Value(primary->label) : shcore::Value());
}

}  // namespace

shcore::Dictionary_t replica_set_status(
    const topology::Server_global_topology &topology,
    const Status_options &opts) {
  shcore::Dictionary_t status = shcore::make_dict();

  status->set("replicaSet", shcore::Value(cluster_status(topology, opts)));

  return status;
}

}  // namespace dba
}  // namespace mysqlsh
