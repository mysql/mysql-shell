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

// TODO: Major items:
// Ensure recoverability if a reconfiguration fails in the middle
// Ensure minor local issues don't affect global topology changes
// Ensure minor local issues are detected
// Ensure issues that would mean the global topology is wrong blocks topology
// changes

#include "modules/adminapi/common/star_global_topology_manager.h"

#include <assert.h>
#include <list>
#include <numeric>
#include <string>
#include <thread>
#include <vector>
#include "modules/adminapi/common/common.h"
#include "modules/adminapi/common/dba_errors.h"
#include "modules/adminapi/common/global_topology_check.h"
#include "modules/adminapi/common/instance_pool.h"
#include "modules/adminapi/common/validations.h"
#include "mysqlshdk/include/scripting/types.h"
#include "mysqlshdk/include/shellcore/console.h"
#include "mysqlshdk/libs/mysql/async_replication.h"
#include "mysqlshdk/libs/mysql/replication.h"
#include "mysqlshdk/libs/utils/utils_string.h"

namespace mysqlsh {
namespace dba {

namespace {
bool check_replication_working(const mysqlshdk::mysql::IInstance &instance,
                               const std::string &channel_name) {
  mysqlshdk::mysql::Replication_channel channel;
  bool repl_ok = false;
  if (!mysqlshdk::mysql::get_channel_status(instance, channel_name, &channel)) {
    log_info("%s: Replication channel '%s' is not active",
             instance.descr().c_str(), channel_name.c_str());
  } else {
    log_info("%s: Replication channel '%s': %s", instance.descr().c_str(),
             channel_name.c_str(),
             mysqlshdk::mysql::format_status(channel, true).c_str());
    if (channel.status() == mysqlshdk::mysql::Replication_channel::Status::ON) {
      repl_ok = true;
    } else if (channel.status() ==
               mysqlshdk::mysql::Replication_channel::Status::APPLIER_ERROR) {
      for (const auto &applier : channel.appliers) {
        if (applier.last_error.code != 0)
          current_console()->print_error(
              "Replication applier error in " + instance.descr() + ": " +
              mysqlshdk::mysql::to_string(applier.last_error));
      }

      // connection errors can be fixed by resetting the channel, but not
      // applier errors
      throw shcore::Exception(
          "Replication applier error in " + instance.descr(),
          SHERR_DBA_REPLICATION_APPLIER_ERROR);
    }
  }
  return repl_ok;
}

void validate_unsupported_options(
    const std::string &target, const topology::Channel &channel,
    const mysqlshdk::utils::Version &server_version) {
  const auto &master_info = channel.master_info;

  auto check_blank = [&](const char *opt, const std::string &s) {
    if (!s.empty()) {
      throw shcore::Exception(
          shcore::str_format("Replication option '%s' at '%s' has a "
                             "non-default value '%s' but it is "
                             "currently not supported by the AdminAPI.",
                             opt, target.c_str(), s.c_str()),
          SHERR_DBA_UNSUPPORTED_ASYNC_CONFIGURATION);
    }
  };

  auto check_equal = [&](const char *opt, uint64_t actual, uint64_t expected) {
    if (actual != expected) {
      throw shcore::Exception(
          shcore::str_format("Replication option '%s' at '%s' has a "
                             "non-default value '%s' but it is "
                             "currently not supported by the AdminAPI.",
                             opt, target.c_str(),
                             std::to_string(actual).c_str()),
          SHERR_DBA_UNSUPPORTED_ASYNC_CONFIGURATION);
    }
  };

  auto check_strequal = [&](const char *opt, const std::string &actual,
                            const std::string &expected) {
    if (actual != expected) {
      throw shcore::Exception(
          shcore::str_format("Replication option '%s' at '%s' has a "
                             "non-default value '%s' but it is "
                             "currently not supported by the AdminAPI.",
                             opt, target.c_str(), actual.c_str()),
          SHERR_DBA_UNSUPPORTED_ASYNC_CONFIGURATION);
    }
  };

  if (master_info.enabled_auto_position == 0) {
    throw shcore::Exception(target +
                                " uses replication without auto-positioning, "
                                "which is not supported by the AdminAPI.",
                            SHERR_DBA_UNSUPPORTED_ASYNC_CONFIGURATION);
  }

  constexpr auto k_default_connect_retry = 60;
  constexpr auto k_default_retry_count = 86400;
  constexpr auto k_default_heartbeat_period = 30;
  constexpr auto k_default_compression_algorithm = "uncompressed";

  std::string source_term =
      mysqlshdk::mysql::get_replication_source_keyword(server_version);

  check_equal(std::string(source_term + "_CONNECT_RETRY").c_str(),
              master_info.connect_retry, k_default_connect_retry);
  check_equal(std::string(source_term + "_SSL").c_str(),
              master_info.enabled_ssl, 0);
  check_equal(std::string(source_term + "_SSL_VERIFY_SERVER_CERT").c_str(),
              master_info.ssl_verify_server_cert, 0);
  check_equal(std::string(source_term + "_HEARTBEAT_PERIOD").c_str(),
              master_info.heartbeat, k_default_heartbeat_period);
  check_blank(std::string(source_term + "_BIND").c_str(), master_info.bind);
  check_strequal("IGNORE_SERVER_IDS", master_info.ignored_server_ids, "0");
  check_equal(std::string(source_term + "_RETRY_COUNT").c_str(),
              master_info.retry_count, k_default_retry_count);
  check_blank(std::string(source_term + "_PUBLIC_KEY_PATH").c_str(),
              master_info.public_key_path.get_safe());
  check_blank("NETWORK_NAMESPACE", master_info.network_namespace.get_safe());
  check_strequal(std::string(source_term + "_COMPRESSION_ALGORITHMS").c_str(),
                 master_info.master_compression_algorithm.get_safe(
                     k_default_compression_algorithm),
                 k_default_compression_algorithm);
  check_equal(std::string(source_term + "_DELAY").c_str(),
              channel.relay_log_info.sql_delay, 0);
  check_blank("PRIVILEGE_CHECKS_USER",
              channel.relay_log_info.privilege_checks_username.get_safe());
  check_blank("PRIVILEGE_CHECKS_USER",
              channel.relay_log_info.privilege_checks_hostname.get_safe());

  // channel.relay_log_info.require_row_format is allowed. It's new in 8.0.19
  // and if it's set to 1, it requires the binlog_format to be ROW, which is
  // fine by us.
}

}  // namespace

void Star_global_topology_manager::validate_adopt_cluster(
    const topology::Node **out_primary) {
  auto console = mysqlsh::current_console();

  console->print_info("* Checking discovered replication topology...");

  int n_masters = 0;
  const topology::Node *primary = nullptr;
  // first find the master and make sure there's only one
  for (const topology::Node *server : topology()->nodes()) {
    if (primary != server->master_node_ptr && server->master_node_ptr) {
      n_masters++;
      primary = server->master_node_ptr;
    }

    // also do some checks that could affect the search for a primary

    // check for bogus/unsupported channels
    if (!server->get_primary_member()->unmanaged_channels.empty()) {
      std::string channels;
      for (const auto &ch : server->get_primary_member()->unmanaged_channels) {
        if (!channels.empty()) channels.append(", ");
        channels.append(ch.channel_name);
      }

      console->print_error(
          server->label +
          " has one or more unsupported replication channels: " + channels);
      console->print_info(
          "Adopted topologies must not have any replication channels other "
          "than the default one.");

      throw shcore::Exception("Unsupported replication topology",
                              SHERR_DBA_UNSUPPORTED_ASYNC_TOPOLOGY);
    }
    if (!server->get_primary_member()->unmanaged_replicas.empty()) {
      console->print_error(
          server->label +
          " has one or more replicas with unsupported channels");
      console->print_info(
          "Adopted topologies must not have any replication channels other "
          "than the default one.");

      throw shcore::Exception("Unsupported replication topology",
                              SHERR_DBA_UNSUPPORTED_ASYNC_TOPOLOGY);
    }

    // check for connect errors
    if (server->get_primary_member()->connect_errno != 0) {
      console->print_error(
          "Could not connect to one or more instances in the topology.");
      console->print_info(
          "The same MySQL account must exist in all instances and have the "
          "same password.");

      throw shcore::Exception("Unsupported replication topology",
                              SHERR_DBA_UNSUPPORTED_ASYNC_TOPOLOGY);
    }

    // check pre-existing repl error
    if (auto channel = server->get_primary_member()->master_channel.get()) {
      if (channel->info.status() != mysqlshdk::mysql::Replication_channel::ON) {
        console->print_error(server->label + " has a replication channel '" +
                             channel->info.channel_name +
                             "' in an unexpected state.");
        console->print_info(
            "Any existing replication settings must be working correctly "
            "before it can be adopted.");

        throw shcore::Exception("Invalid replication state",
                                SHERR_DBA_INVALID_SERVER_CONFIGURATION);
      } else {
        log_info("Replication status of %s is OK", server->label.c_str());
      }
    }
  }

  if (topology()->nodes().size() < 2) {
    // Unlike single member GR groups, there's no way to tell
    // apart standalone vs single member AR, so we require at least 2 instances,
    // since a single instance can't be part of a master-slave setup.
    throw shcore::Exception(
        "Target server is not part of an async replication topology",
        SHERR_DBA_UNSUPPORTED_ASYNC_TOPOLOGY);
  }

  if (n_masters < 1) {
    console->print_error(
        "Unable to determine a PRIMARY instance in the topology.");

    throw shcore::Exception("Unsupported replication topology",
                            SHERR_DBA_UNSUPPORTED_ASYNC_TOPOLOGY);
  } else if (n_masters > 1) {
    console->print_error(
        "Unable to determine the PRIMARY instance in the topology. "
        "Multi-master topologies are not supported.");

    throw shcore::Exception("Unsupported replication topology",
                            SHERR_DBA_UNSUPPORTED_ASYNC_TOPOLOGY);
  } else {
    console->print_info(primary->label + " detected as the PRIMARY.");
    *out_primary = primary;
  }

  // make sure all other instances replicate from it and GTID state is
  // consistent
  for (const topology::Node *server : topology()->nodes()) {
    if (server != primary) {
      auto channel = server->get_primary_member()->master_channel.get();
      if (!channel) continue;

      // check bogus master
      if (channel->master_ptr != primary->get_primary_member()) {
        console->print_error(
            server->label +
            " is replicating from an instance other than the PRIMARY.");

        throw shcore::Exception("Unsupported replication topology",
                                SHERR_DBA_UNSUPPORTED_ASYNC_TOPOLOGY);
      } else {
        log_info("%s is replicating from expected instance %s",
                 server->label.c_str(), channel->master_ptr->label.c_str());
      }

      // gtid check
      if (server->errant_transaction_count.is_null()) {
        console->print_error(server->label +
                             " could not be checked for errant transactions.");
        throw shcore::Exception(
            "Unable to check target for errant transactions.",
            SHERR_DBA_TARGET_QUERY_ERROR);
      } else if (*server->errant_transaction_count > 0) {
        console->print_error(shcore::str_format(
            "A GTID check indicates that the transaction set in %s has %zi "
            "transactions that do not exist in its source %s.",
            server->label.c_str(), *server->errant_transaction_count,
            channel->master_ptr->label.c_str()));
        console->print_info(
            "Errant transactions, or transactions that were executed from "
            "an instance other than the designated PRIMARY, can lead to "
            "replication errors and data inconsistencies.");
        console->print_info();
        throw shcore::Exception("Errant transactions detected.",
                                SHERR_DBA_DATA_ERRANT_TRANSACTIONS);
      } else {
        log_info("%s has no errant transactions", server->label.c_str());
      }

      // check for unsupported replication options
      Scoped_instance instance(current_ipool()->connect_unchecked_endpoint(
          server->get_primary_member()->endpoint));
      auto server_version = instance->get_version();
      validate_unsupported_options(server->label, *channel, server_version);

      console->print_info("Replication state of " + server->label + " is OK.");
    }
  }
}

void Star_global_topology_manager::validate_add_replica(
    const topology::Node * /*master_node*/,
    mysqlshdk::mysql::IInstance *instance,
    const Async_replication_options & /*repl_options*/) {
  auto console = mysqlsh::current_console();

  console->print_info("* Checking async replication topology...");
  // Check that the current state of the global topology can safely accept
  // a new async slave
  log_info("ACTIVE master cluster is %s",
           m_topology->get_primary_master_node()->label.c_str());

  validate_global_topology_consistent(*m_topology);

  // Check if there's already an active channel in the replica
  mysqlshdk::mysql::Replication_channel channel;
  if (mysqlshdk::mysql::get_channel_status(
          *instance, topology()->repl_channel_name(), &channel)) {
    if (channel.status() != mysqlshdk::mysql::Replication_channel::OFF) {
      console->print_error(instance->descr() +
                           " already has an active replication channel '" +
                           topology()->repl_channel_name() + "'");

      throw shcore::Exception("Unexpected replication channel",
                              SHERR_DBA_INVALID_SERVER_CONFIGURATION);
    }
  }
}

void Star_global_topology_manager::validate_rejoin_replica(
    mysqlshdk::mysql::IInstance *instance) {
  auto console = mysqlsh::current_console();

  // Validate that the topology is active (available healthy PRIMARY).
  validate_global_topology_active_cluster_available(*m_topology);

  // Validate the status of the target instance.
  auto topology_node = m_topology->try_get_node_for_uuid(instance->get_uuid());
  if (!topology_node)
    topology_node = m_topology->try_get_node_for_endpoint(
        instance->get_canonical_address());
  if (!topology_node)
    throw shcore::Exception(
        "Unable to find instance '" + instance->descr() + "' in the topology.",
        SHERR_DBA_ASYNC_MEMBER_TOPOLOGY_MISSING);

  topology::Node_status status = topology_node->status();
  if (status == topology::Node_status::ONLINE ||
      status == topology::Node_status::UNREACHABLE) {
    console->print_error("Unable to rejoin an " + to_string(status) +
                         " instance. This operation can only be used to rejoin "
                         "instances with an " +
                         to_string(topology::Node_status::INVALIDATED) + ", " +
                         to_string(topology::Node_status::OFFLINE) + ", " +
                         to_string(topology::Node_status::INCONSISTENT) +
                         " or " + to_string(topology::Node_status::ERROR) +
                         " status.");
    throw shcore::Exception("Invalid status to execute operation, " +
                                topology_node->label + " is " +
                                to_string(status),
                            SHERR_DBA_ASYNC_MEMBER_INVALID_STATUS);
  }
}

void Star_global_topology_manager::validate_remove_replica(
    mysqlshdk::mysql::IInstance *master, mysqlshdk::mysql::IInstance *instance,
    bool force, bool *out_repl_working) {
  auto console = current_console();

  // check if there's something that would prevent replication from catching up
  // Note that we don't care about errant transactions in the removed replica,
  // but purged missing transactions are a fatal problem (except on force)
  bool sync_ok =
      ensure_gtid_sync_possible(*master, *instance, force ? false : true);

  ensure_gtid_no_errants(*master, *instance, false);

  // ensure replication is working ok
  if (sync_ok &&
      !check_replication_working(*instance, m_topology->repl_channel_name())) {
    console->print_warning(
        shcore::str_format("Replication is not active in instance %s.",
                           instance->descr().c_str()));
    if (!force) {
      console->print_error(
          "Instance cannot be safely removed while in this state.");
      console->print_info(
          "Use the 'force' option to remove this instance anyway. The instance "
          "may be left in an inconsistent state after removed.");

      throw shcore::Exception("Replication is not active in target instance",
                              SHERR_DBA_REPLICATION_INVALID);
    }
    *out_repl_working = false;
  } else {
    if (sync_ok)
      *out_repl_working = true;
    else
      *out_repl_working = false;
  }
}

// -----------------------------------------------------------------------------

namespace {
void validate_transaction_sets_for_switch_primary(
    mysqlshdk::mysql::IInstance *master, mysqlshdk::mysql::IInstance *slave) {
  auto console = mysqlsh::current_console();

  // Transaction set validation:
  // - It's expected that the current master has transactions missing from the
  // promoted slave. But by the final sync right before the cutoff, it should
  // not.
  // - The promoted slave in theory should not contain any transaction that's
  // not yet in the master. This should never happen, unless a user has bypassed
  // the READ-ONLY flags in the slave, in that case we should abort the switch
  // over and notify the user.
  // But in reality, the binlog can have internal events like view changes,
  // which should not affect the integrity of the data. So we can inform about
  // these, but they're not necessarily errors.

  ensure_gtid_sync_possible(*master, *slave, true);

  ensure_gtid_no_errants(*master, *slave, true);
}
}  // namespace

void Star_global_topology_manager::validate_switch_primary(
    mysqlshdk::mysql::IInstance *master,
    mysqlshdk::mysql::IInstance * /*promoted*/,
    const std::list<std::shared_ptr<Instance>> &instances) {
  auto console = mysqlsh::current_console();

  console->print_info("** Checking async replication topology...");
  validate_global_topology_consistent(*m_topology);

  console->print_info("** Checking transaction state of the instance...");
  for (const auto &instance : instances) {
    if (instance.get() == master) continue;

    auto node = m_topology->try_get_node_for_uuid(instance->get_uuid());
    if (!node)
      node = m_topology->try_get_node_for_endpoint(
          instance->get_canonical_address());
    validate_node_status(node);

    validate_transaction_sets_for_switch_primary(master, instance.get());
  }
}

// -----------------------------------------------------------------------------

void Star_global_topology_manager::validate_force_primary(
    mysqlshdk::mysql::IInstance *master,
    const std::list<std::shared_ptr<Instance>> &instances) {
  auto console = mysqlsh::current_console();

  // active is not available
  console->print_info("* Checking status of last known PRIMARY");
  validate_active_unavailable();

  const topology::Node *promoted_node = nullptr;
  std::vector<Instance_gtid_info> gtid_info;

  // Only check valid instances (not skipped/invalidated).
  for (const auto &valid_inst : instances) {
    auto inst = topology()->try_get_node_for_uuid(valid_inst->get_uuid());

    // Sanity check (not expected to be null), skip it if that's the case.
    if (!inst) {
      log_warning(
          "Validation of instance %s was skipped, not found in topology.",
          valid_inst->descr().c_str());
      continue;
    }

    // NOTE: Use the latest GTID_EXECUTED set (after catching up with received
    // trx), not the initially "cached" set by Global_topology.
    std::string gtid_set = mysqlshdk::mysql::get_executed_gtid_set(*valid_inst);
    if (!gtid_set.empty()) {
      gtid_info.emplace_back(Instance_gtid_info{inst->label, gtid_set});
      if (inst->get_primary_member()->uuid == master->get_uuid()) {
        promoted_node = inst;
      }
    }
  }
  if (!promoted_node) throw std::logic_error("internal error");

  console->print_info("* Checking status of promoted instance");
  validate_force_primary_target(promoted_node);

  console->print_info("* Checking transaction set status");

  // this will return instances that have the most up-to-date GTID sets
  gtid_info = filter_primary_candidates(*master, gtid_info, {});

  // check if the selected master is among the candidates
  bool ok = false;
  for (const auto &i : gtid_info) {
    if (i.server == promoted_node->label) {
      ok = true;
      break;
    }
  }
  if (!ok) {
    std::string candidates;
    for (const auto &i : gtid_info) {
      if (!candidates.empty()) {
        candidates.append(", ").append(i.server);
      } else {
        candidates = i.server;
      }
    }

    if (gtid_info.size() == 1) {
      console->print_error(candidates +
                           " is more up-to-date than the selected instance "
                           "and should be used for promotion instead.");
    } else {
      console->print_error(candidates +
                           " are more up-to-date than the selected instance "
                           "and should be used for promotion instead.");
    }
    throw shcore::Exception("Target instance is behind others",
                            SHERR_DBA_BADARG_INSTANCE_OUTDATED);
  }
}

void Star_global_topology_manager::validate_active_unavailable() {
  const topology::Node *primary_master = nullptr;
  auto console = mysqlsh::current_console();

  // The last known ACTIVE master cluster must be unavailable and unreachable.
  // If it's unavailable but reachable, restoring the active cluster is a
  // better fix for the cluster.

  // Find the primary master cluster according to the MD
  primary_master = m_topology->get_primary_master_node();

  assert(primary_master);

  log_info("Status of PRIMARY node %s is %s", primary_master->label.c_str(),
           to_string(primary_master->status()).c_str());

  switch (auto s = primary_master->status()) {
    case topology::Node_status::ERROR:
    case topology::Node_status::INCONSISTENT:
      console->print_note(primary_master->label +
                          " is still reachable and has status " +
                          to_string(primary_master->status()) + ".");
      console->print_info(
          "This operation is only allowed if the async PRIMARY is "
          "unavailable and cannot be restored. Please restore it and use "
          "<<<setPrimaryInstance>>>() or shut it down if you really want to "
          "perform a failover.");

      throw shcore::Exception("PRIMARY still reachable",
                              SHERR_DBA_PRIMARY_CLUSTER_STILL_REACHABLE);

    case topology::Node_status::ONLINE:
    case topology::Node_status::OFFLINE:  // PRIMARY can't be OFFLINE
      console->print_info("PRIMARY " + primary_master->label +
                          " is still available.");
      console->print_info(
          "Operation not allowed while there is still an available PRIMARY. "
          "Use <<<setPrimaryInstance>>>() to safely switch the PRIMARY.");

      throw shcore::Exception("PRIMARY still available",
                              SHERR_DBA_PRIMARY_CLUSTER_STILL_AVAILABLE);

    case topology::Node_status::UNREACHABLE:
    case topology::Node_status::INVALIDATED:
      console->print_note(primary_master->label + " is " + to_string(s));
      break;
  }
}

void Star_global_topology_manager::validate_force_primary_target(
    const topology::Node *promoted_node) {
  auto console = mysqlsh::current_console();

  bool ok = true;

  topology::Node_status status = promoted_node->status();

  console->print_note(promoted_node->label + " has status " +
                      to_string(status));

  // Replication errors are OK, since we're re-configuring replication anyway
  if (status != topology::Node_status::ONLINE &&
      status != topology::Node_status::ERROR &&
      status != topology::Node_status::OFFLINE) {
    validate_node_status(promoted_node);
  }

  if (!ok) {
    throw shcore::Exception("Sanity checks failed for " + promoted_node->label,
                            SHERR_DBA_GROUP_TOPOLOGY_ERROR);
  }
}

}  // namespace dba
}  // namespace mysqlsh
