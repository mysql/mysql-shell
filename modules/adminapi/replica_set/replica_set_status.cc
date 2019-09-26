/*
 * Copyright (c) 2019, Oracle and/or its affiliates. All rights reserved.
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

#include "modules/adminapi/common/dba_errors.h"
#include "mysqlshdk/include/shellcore/console.h"
#include "mysqlshdk/libs/utils/strformat.h"
#include "mysqlshdk/libs/utils/utils_string.h"

namespace mysqlsh {
namespace dba {

namespace {

void add_error(const mysqlshdk::mysql::Replication_channel::Error &error,
               const std::string &prefix, shcore::Dictionary_t status) {
  status->set(prefix + "Error", shcore::Value(error.message));
  status->set(prefix + "ErrorNumber", shcore::Value(error.code));
  status->set(prefix + "ErrorTimestamp", shcore::Value(error.timestamp));
}

shcore::Dictionary_t make_error(
    const mysqlshdk::mysql::Replication_channel::Error &error) {
  return shcore::make_dict("error", shcore::Value(error.message), "errorNumber",
                           shcore::Value(error.code), "errorTimestamp",
                           shcore::Value(error.timestamp));
}

void add_channel_errors(const mysqlshdk::mysql::Replication_channel &channel,
                        shcore::Dictionary_t status,
                        const Status_options &opts) {
  status->set(
      "receiverStatus",
      shcore::Value(mysqlshdk::mysql::to_string(channel.receiver.status)));
  if (!channel.receiver.thread_state.empty())
    status->set("receiverThreadState",
                shcore::Value(channel.receiver.thread_state));
  else
    status->set("receiverThreadState", shcore::Value(""));

  if (channel.receiver.last_error.code != 0)
    add_error(channel.receiver.last_error, "receiverLast", status);

  if (opts.show_details) {
    if (channel.coordinator.state !=
        mysqlshdk::mysql::Replication_channel::Coordinator::NONE) {
      status->set("coordinatorState", shcore::Value(mysqlshdk::mysql::to_string(
                                          channel.coordinator.state)));
      if (!channel.coordinator.thread_state.empty())
        status->set("coordinatorThreadState",
                    shcore::Value(channel.coordinator.thread_state));

      if (channel.coordinator.last_error.code != 0)
        add_error(channel.coordinator.last_error, "coordinatorLast", status);
    }
  }

  {
    const auto &applier = channel.appliers.front();
    if (!applier.thread_state.empty())
      status->set("applierThreadState", shcore::Value(applier.thread_state));
    else
      status->set("applierThreadState", shcore::Value(""));

    if (opts.show_details)
      status->set("applierState",
                  shcore::Value(mysqlshdk::mysql::to_string(applier.state)));

    mysqlshdk::mysql::Replication_channel::Applier::Status astatus =
        applier.status;

    shcore::Array_t applier_errors = shcore::make_array();
    for (const auto &channel_applier : channel.appliers) {
      if (channel_applier.last_error.code != 0) {
        applier_errors->push_back(
            shcore::Value(make_error(channel_applier.last_error)));
        astatus = mysqlshdk::mysql::Replication_channel::Applier::Status::ERROR;
      }
    }

    status->set("applierStatus",
                shcore::Value(mysqlshdk::mysql::to_string(astatus)));

    if (!applier_errors->empty()) {
      status->set("applierLastErrors", shcore::Value(applier_errors));
    }
  }
}

void channel_status(shcore::Dictionary_t status, const topology::Server &server,
                    const topology::Instance &instance,
                    const Status_options &opts) {
  const topology::Channel *channel = instance.master_channel.get();

  if ((server.primary_master || server.invalidated) && !channel) return;

  shcore::Dictionary_t rstatus = shcore::make_dict();

  bool show_expected_master = false;
  if (channel) {
    add_channel_errors(channel->info, rstatus, opts);

    if (channel->info.status() !=
        mysqlshdk::mysql::Replication_channel::Status::ON)
      show_expected_master = true;

    const topology::Instance *actual_master = channel->master_ptr;
    if (show_expected_master || opts.show_details) {
      if (actual_master) {
        rstatus->set("source", shcore::Value(actual_master->label));
      }
    }
    if (!actual_master) {
      rstatus->set("source", shcore::Value::Null());  // shouldn't happen
      show_expected_master = true;
    }

    if (opts.show_details > 1 || channel->info.appliers.size() > 1) {
      rstatus->set(
          "applierWorkerThreads",
          shcore::Value(static_cast<int>(channel->info.appliers.size())));
    }

    if (opts.show_details > 1 || channel->relay_log_info.sql_delay > 0) {
      shcore::Dictionary_t conf = shcore::make_dict();

      conf->set("delay", shcore::Value(channel->relay_log_info.sql_delay));

      if (opts.show_details > 1) {
        conf->set("heartbeatPeriod",
                  shcore::Value(channel->master_info.heartbeat));
        conf->set("connectRetry",
                  shcore::Value(channel->master_info.connect_retry));
        conf->set("retryCount",
                  shcore::Value(channel->master_info.retry_count));
      }

      rstatus->set("options", shcore::Value(conf));
    }

    if (opts.show_details) {
      rstatus->set("receiverTimeSinceLastMessage",
                   shcore::Value(channel->info.time_since_last_message));

      rstatus->set("applierQueuedTransactionSet",
                   shcore::Value(channel->info.queued_gtid_set_to_apply));
    }
    if (!channel->info.queued_gtid_set_to_apply.empty() || opts.show_details) {
      rstatus->set(
          "applierQueuedTransactionSetSize",
          shcore::Value(int64_t(mysqlshdk::mysql::estimate_gtid_set_size(
              channel->info.queued_gtid_set_to_apply))));
    }

    if (channel->info.status() ==
        mysqlshdk::mysql::Replication_channel::Status::ON) {
      if (channel->info.repl_lag_from_original ==
          channel->info.repl_lag_from_immediate) {
        rstatus->set("replicationLag",
                     channel->info.repl_lag_from_original.empty()
                         ? shcore::Value::Null()
                         : shcore::Value(channel->info.repl_lag_from_original));
      } else {
        rstatus->set("replicationLagFromOriginalSource",
                     shcore::Value(channel->info.repl_lag_from_original));

        rstatus->set("replicationLagFromImmediateMaster",
                     shcore::Value(channel->info.repl_lag_from_immediate));
      }
    } else {
      rstatus->set("replicationLag", shcore::Value::Null());
    }

    if (auto master =
            dynamic_cast<const topology::Server *>(server.master_node_ptr)) {
      if (master->get_primary_member() != actual_master)
        show_expected_master = true;
    }
  } else {
    rstatus->set("receiverStatus", shcore::Value::Null());
    rstatus->set("applierStatus", shcore::Value::Null());
    show_expected_master = true;
  }
  if (show_expected_master) {
    if (auto master =
            dynamic_cast<const topology::Server *>(server.master_node_ptr)) {
      rstatus->set("expectedSource", shcore::Value(master->label));
    } else {
      // shouldn't happen
      rstatus->set("expectedSource",
                   shcore::Value(server.master_instance_uuid));
    }
  }

  status->set("replication", shcore::Value(rstatus));
}

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
        std::to_string(server.errant_transaction_count).c_str())));
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
            shcore::Value(channel.host + ":" + std::to_string(channel.port)))));
      }
      status->set("unknownChannels", shcore::Value(unknown_channels));
    }

    if (!instance->unmanaged_replicas.empty()) {
      shcore::Array_t unknown_replicas = shcore::make_array();
      for (const auto &slave : instance->unmanaged_replicas) {
        unknown_replicas->push_back(
            shcore::Value(slave.host + ":" + std::to_string(slave.port)));
      }
      status->set("unknownReplicas", shcore::Value(unknown_replicas));
    }
  }

  if (instance->connect_error.empty()) {
    channel_status(status, server, *instance, opts);

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
