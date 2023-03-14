/*
 * Copyright (c) 2019, 2023, Oracle and/or its affiliates.
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

#include "modules/adminapi/common/async_topology.h"

#include <mysqld_error.h>
#include <vector>

#include "modules/adminapi/common/async_replication_options.h"
#include "modules/adminapi/common/common.h"
#include "modules/adminapi/common/dba_errors.h"
#include "modules/adminapi/common/instance_pool.h"
#include "mysql/clone.h"
#include "mysql/utils.h"
#include "mysqlshdk/include/shellcore/console.h"
#include "mysqlshdk/libs/mysql/async_replication.h"
#include "mysqlshdk/libs/mysql/replication.h"
#include "mysqlshdk/libs/utils/logger.h"
#include "mysqlshdk/libs/utils/utils_net.h"

namespace mysqlsh {
namespace dba {

namespace {
// static constexpr const int k_replication_start_timeout = 60;
constexpr const int k_replication_stop_timeout = 60;

mysqlshdk::mysql::Replication_options prepare_replica_options(
    const mysqlsh::dba::Async_replication_options &repl_options) {
  mysqlshdk::mysql::Replication_options options;
  options.connect_retry = repl_options.connect_retry;
  options.retry_count = repl_options.retry_count;
  options.auto_failover = repl_options.auto_failover;
  options.delay = repl_options.delay;
  options.heartbeat_period = repl_options.heartbeat_period;
  options.compression_algos = repl_options.compression_algos;
  options.zstd_compression_level = repl_options.zstd_compression_level;
  options.bind = repl_options.bind;
  options.network_namespace = repl_options.network_namespace;
  options.auto_position = true;
  return options;
}

void change_master(const mysqlshdk::mysql::IInstance &slave,
                   const mysqlshdk::mysql::IInstance &new_master,
                   const std::string &channel_name,
                   const Async_replication_options &repl_options,
                   bool dry_run) {
  auto console = current_console();
  console->print_info(shcore::str_format(
      "** Changing replication source of %s to %s", slave.descr().c_str(),
      new_master.get_canonical_address().c_str()));

  if (dry_run) return;

  try {
    // if credentials given, change everything, otherwise just change the
    // host:port
    if (repl_options.repl_credentials) {
      try {
        auto creds = *repl_options.repl_credentials;

        creds.ssl_options = prepare_replica_ssl_options(
            slave, repl_options.ssl_mode, repl_options.auth_type);

        mysqlshdk::mysql::change_master(
            slave, new_master.get_canonical_hostname(),
            new_master.get_canonical_port(), channel_name, creds,
            prepare_replica_options(repl_options));

      } catch (const shcore::Error &e) {
        console->print_error(
            shcore::str_format("Error setting up async replication channel: %s",
                               e.format().c_str()));
        throw shcore::Exception::mysql_error_with_code(e.what(), e.code());
      }

    } else {
      mysqlshdk::mysql::change_master_host_port(
          slave, new_master.get_canonical_hostname(),
          new_master.get_canonical_port(), channel_name,
          prepare_replica_options(repl_options));
    }
  } catch (const shcore::Error &e) {
    console->print_error(
        shcore::str_format("%s: Error setting up async replication channel: %s",
                           slave.descr().c_str(), e.format().c_str()));
    throw shcore::Exception::mysql_error_with_code(e.what(), e.code());
  }
}
}  // namespace

void fence_instance(mysqlshdk::mysql::IInstance *instance) {
  try {
    // fence with SET PERSIST but unfence with SET GLOBAL, so that instances are
    // SRO if they restart
    instance->set_sysvar("SUPER_READ_ONLY", true,
                         mysqlshdk::mysql::Var_qualifier::PERSIST);
  } catch (const shcore::Error &e) {
    throw shcore::Exception::mysql_error_with_code(e.what(), e.code());
  }
}

void unfence_instance(mysqlshdk::mysql::IInstance *instance, bool persist) {
  try {
    instance->set_sysvar("SUPER_READ_ONLY", false,
                         persist ? mysqlshdk::mysql::Var_qualifier::PERSIST
                                 : mysqlshdk::mysql::Var_qualifier::GLOBAL);
    // Set SUPER_READ_ONLY=1 will also set READ_ONLY
    instance->set_sysvar("READ_ONLY", false,
                         persist ? mysqlshdk::mysql::Var_qualifier::PERSIST
                                 : mysqlshdk::mysql::Var_qualifier::GLOBAL);
  } catch (const shcore::Error &e) {
    throw shcore::Exception::mysql_error_with_code(e.what(), e.code());
  }
}

void async_add_replica(mysqlshdk::mysql::IInstance *primary,
                       mysqlshdk::mysql::IInstance *target,
                       const std::string &channel_name,
                       const Async_replication_options &repl_options,
                       bool fence_slave, bool dry_run, bool start_replica) {
  stop_channel(*target, channel_name, true, dry_run);

  change_master(*target, *primary, channel_name, repl_options, dry_run);

  if (start_replica) start_channel(*target, channel_name, dry_run);

  if (fence_slave) {
    // The fence added here is going to be cleared when a GR group
    // is created, but we set it up anyway in case something goes wrong
    // before that.
    log_info("Fencing new instance '%s' to prevent updates.",
             target->descr().c_str());
    if (!dry_run) fence_instance(target);
  }
}

void async_rejoin_replica(mysqlshdk::mysql::IInstance *primary,
                          mysqlshdk::mysql::IInstance *target,
                          const std::string &channel_name,
                          const Async_replication_options &rpl_options,
                          bool clear_channel) {
  auto console = mysqlsh::current_console();

  stop_channel(*target, channel_name, true, false);

  if (clear_channel) reset_channel(*target, channel_name, true);

  // Setting primary for target instance and restarting replication.
  change_master(*target, *primary, channel_name, rpl_options, false);

  start_channel(*target, channel_name, false);

  console->print_info("** Checking replication channel status...");
  // NOTE: We use a fixed timeout of 60 sec to wait for replication to start.
  //       Should we define a timeout option for this?
  try {
    check_replication_startup(*target, channel_name);
  } catch (const shcore::Exception &) {
    console->print_error(
        "Issue found in the replication channel. Please fix the error and "
        "retry to join the instance.");
    throw;
  }

  log_info("Fencing new instance '%s' to prevent updates.",
           target->descr().c_str());
  fence_instance(target);
}

void async_remove_replica(mysqlshdk::mysql::IInstance *target,
                          const std::string &channel_name, bool dry_run) {
  // stop slave and clear credentials from slave_master_info
  remove_channel(*target, channel_name, dry_run);

  log_info("Fencing removed instance %s", target->descr().c_str());
  if (!dry_run) fence_instance(target);
}

void async_update_replica_credentials(
    mysqlshdk::mysql::IInstance *target, const std::string &channel_name,
    const Async_replication_options &rpl_options, bool dry_run) {
  using mysqlshdk::mysql::Replication_channel;

  Stop_channel_result channel_stopped =
      stop_channel(*target, channel_name, false, dry_run);

  if (!dry_run) {
    mysqlshdk::mysql::Replication_credentials_options options;
    options.password = rpl_options.repl_credentials->password.value_or("");

    change_replication_credentials(*target, channel_name,
                                   rpl_options.repl_credentials->user, options);
  }

  if (channel_stopped == Stop_channel_result::STOPPED)
    start_channel(*target, channel_name, dry_run);
}

void async_swap_primary(mysqlshdk::mysql::IInstance *current_primary,
                        mysqlshdk::mysql::IInstance *promoted,
                        const std::string &channel_name,
                        const Async_replication_options &repl_options,
                        shcore::Scoped_callback_list *undo_list, bool dry_run) {
  assert(current_primary != promoted);

  // Assumptions:
  // - all slaves are synced
  // - all instances are FTWRLed
  // - repl_options contains valid username/password

  // Fence the old master
  log_info("Enabling SUPER_READ_ONLY in old PRIMARY %s",
           current_primary->descr().c_str());

  if (undo_list)
    undo_list->push_front([=]() {
      if (!dry_run) unfence_instance(current_primary, true);
    });
  if (!dry_run) fence_instance(current_primary);

  if (undo_list)
    undo_list->push_front(
        [=]() { start_channel(*promoted, channel_name, dry_run); });

  // Stop slave at the instance being promoted
  stop_channel(*promoted, channel_name, true, dry_run);

  if (undo_list)
    undo_list->push_front([=]() {
      reset_channel(*current_primary, channel_name, true, dry_run);
    });

  // Make old master a slave of new master
  change_master(*current_primary, *promoted, channel_name, repl_options,
                dry_run);

  if (undo_list)
    undo_list->push_front(
        [=]() { stop_channel(*current_primary, channel_name, true, dry_run); });
  start_channel(*current_primary, channel_name, dry_run);

  if (undo_list) {
    undo_list->push_front([=]() {
      if (!dry_run) fence_instance(promoted);
    });
  }

  // unfence the new master
  log_info("Clearing SUPER_READ_ONLY in new PRIMARY %s",
           promoted->descr().c_str());
  if (!dry_run) unfence_instance(promoted, true);
}

void async_force_primary(mysqlshdk::mysql::IInstance *promoted,
                         const std::string &channel_name, bool dry_run) {
  // unfence the new master
  log_info("Clearing SUPER_READ_ONLY in new PRIMARY %s",
           promoted->descr().c_str());
  if (!dry_run) unfence_instance(promoted, true);

  // Stop slave at the instance being promoted
  stop_channel(*promoted, channel_name, true, dry_run);
}

void undo_async_force_primary(mysqlshdk::mysql::IInstance *promoted,
                              const std::string &channel_name, bool dry_run) {
  // Start slave on candidate (revert stop slave).
  start_channel(*promoted, channel_name, dry_run);

  // Fence the candidate (revert unfence).
  log_info("Re-enabling SUPER_READ_ONLY on %s", promoted->descr().c_str());
  if (!dry_run) fence_instance(promoted);
}

void async_create_channel(mysqlshdk::mysql::IInstance *target,
                          mysqlshdk::mysql::IInstance *primary,
                          const std::string &channel_name,
                          const Async_replication_options &repl_options,
                          bool dry_run) {
  change_master(*target, *primary, channel_name, repl_options, dry_run);
}

void async_change_primary(mysqlshdk::mysql::IInstance *replica,
                          mysqlshdk::mysql::IInstance *primary,
                          const std::string &channel_name,
                          const Async_replication_options &repl_options,
                          bool start_replica, bool dry_run,
                          bool reset_async_channel) {
  // make sure it's fenced
  if (!dry_run) fence_instance(replica);

  stop_channel(*replica, channel_name, true, dry_run);

  if (reset_async_channel) reset_channel(*replica, channel_name, true, dry_run);

  // This will re-point the slave to the new master without changing any
  // other replication parameter if repl_options has no credentials.
  async_create_channel(replica, primary, channel_name, repl_options, dry_run);

  if (start_replica) start_channel(*replica, channel_name, dry_run);

  log_info("PRIMARY changed for instance %s", replica->descr().c_str());
}

void async_change_channel_repl_options(
    const mysqlshdk::mysql::IInstance &instance,
    const std::string &channel_name,
    const Async_replication_options &repl_options, bool reset_async_channel) {
  stop_channel(instance, channel_name, true, false);

  if (reset_async_channel) reset_channel(instance, channel_name, true);

  mysqlshdk::mysql::change_master_repl_options(
      instance, channel_name, prepare_replica_options(repl_options));

  start_channel(instance, channel_name, false);
}

std::optional<Async_replication_options> async_merge_repl_options(
    const Async_replication_options &ar_options,
    const mysqlshdk::mysql::Replication_channel_master_info &channel_info) {
  std::optional<Async_replication_options> merged;

  if (ar_options.connect_retry.has_value()) {
    if (ar_options.connect_retry.value() !=
        static_cast<int>(channel_info.connect_retry)) {
      if (!merged) merged = Async_replication_options{};
      merged->connect_retry = ar_options.connect_retry;
    }
  }

  if (ar_options.retry_count.has_value()) {
    if (ar_options.retry_count.value() !=
        static_cast<int>(channel_info.retry_count)) {
      if (!merged) merged = Async_replication_options{};
      merged->retry_count = ar_options.retry_count;
    }
  }

  if (ar_options.heartbeat_period.has_value()) {
    // the server assigns a precision of 0.001 to this var, so we should also
    if (!shcore::compare_floating_point(ar_options.heartbeat_period.value(),
                                        channel_info.heartbeat_period, 0.001)) {
      if (!merged) merged = Async_replication_options{};
      merged->heartbeat_period = ar_options.heartbeat_period;
    }
  }

  if (ar_options.compression_algos.has_value()) {
    if (!channel_info.compression_algorithm.has_value() ||
        ar_options.compression_algos.value() !=
            channel_info.compression_algorithm.value()) {
      if (!merged) merged = Async_replication_options{};
      merged->compression_algos = ar_options.compression_algos;
    }
  }

  if (ar_options.zstd_compression_level.has_value()) {
    if (!channel_info.zstd_compression_level.has_value() ||
        ar_options.zstd_compression_level.value() !=
            static_cast<int>(channel_info.zstd_compression_level.value())) {
      if (!merged) merged = Async_replication_options{};
      merged->zstd_compression_level = ar_options.zstd_compression_level;
    }
  }

  if (ar_options.bind.has_value()) {
    if (ar_options.bind.value() != channel_info.bind) {
      if (!merged) merged = Async_replication_options{};
      merged->bind = ar_options.bind;
    }
  }

  if (ar_options.network_namespace.has_value()) {
    if (!channel_info.network_namespace.has_value() ||
        ar_options.network_namespace.value() !=
            channel_info.network_namespace.value()) {
      if (!merged) merged = Async_replication_options{};
      merged->network_namespace = ar_options.network_namespace;
    }
  }

  return merged;
}

void async_change_primary(
    mysqlshdk::mysql::IInstance *primary,
    mysqlshdk::mysql::IInstance *old_primary,
    const std::function<bool(mysqlshdk::mysql::IInstance **,
                             Async_replication_options *)>
        &cb_consume_secondaries,
    const std::string &channel_name, shcore::Scoped_callback_list *undo_list,
    bool dry_run) {
  mysqlshdk::mysql::IInstance *slave;
  Async_replication_options slave_repl_options;
  while (cb_consume_secondaries(&slave, &slave_repl_options)) {
    if (slave->get_uuid() == primary->get_uuid()) continue;
    if (old_primary && (slave->get_uuid() == old_primary->get_uuid())) continue;

    undo_list->push_front([=]() {
      if (old_primary)
        async_change_primary(slave, old_primary, channel_name, {}, true,
                             dry_run);
    });

    async_change_primary(slave, primary, channel_name, slave_repl_options, true,
                         dry_run);

    log_info("PRIMARY changed for instance %s", slave->descr().c_str());
  }
}

void wait_apply_retrieved_trx(mysqlshdk::mysql::IInstance *instance,
                              std::chrono::seconds timeout) {
  wait_for_apply_retrieved_trx(*instance, k_replicaset_channel_name, timeout,
                               false);
}

void wait_all_apply_retrieved_trx(
    std::list<std::shared_ptr<Instance>> *instances,
    std::chrono::seconds timeout, bool invalidate_error_instances,
    std::vector<Instance_metadata> *out_instances_md,
    std::list<Instance_id> *out_invalidate_ids) {
  auto console = current_console();

  console->print_info("* Waiting for all received transactions to be applied");
  std::list<shcore::Dictionary_t> errors =
      execute_in_parallel(instances->begin(), instances->end(),
                          // Wait for retrieved GTIDs to be applied in parallel
                          // and return an error or null.
                          [timeout](const std::shared_ptr<Instance> &inst) {
                            wait_apply_retrieved_trx(inst.get(), timeout);
                          });

  if (errors.empty()) return;

  for (const auto &err : errors) {
    auto instance_uuid = err->get_string("uuid");
    auto instance_desc = err->get_string("from");
    auto err_msg = shcore::str_format("%s: Failed to apply retrieved GTIDs: %s",
                                      instance_desc.c_str(),
                                      err->get_string("errmsg").c_str());

    if (!invalidate_error_instances) {
      console->print_error(err_msg);
    } else {
      log_warning("%s", err_msg.c_str());

      // Get instance metadata info and add it to invalidate list.
      auto md_it =
          std::find_if(out_instances_md->begin(), out_instances_md->end(),
                       [&instance_uuid](const Instance_metadata &i_md) {
                         return i_md.uuid == instance_uuid;
                       });
      if (md_it != out_instances_md->end()) {
        out_invalidate_ids->push_back(md_it->id);
      }

      console->print_note(
          md_it->label +
          " will be invalidated (fail to apply received GTIDs) and must be "
          "fixed or removed from the replicaset.");

      // Remove instance to invalidate from lists.
      instances->remove_if(
          [&instance_uuid](const std::shared_ptr<Instance> &i) {
            return i->get_uuid() == instance_uuid;
          });
      out_instances_md->erase(md_it);
    }
  }

  if (!invalidate_error_instances) {
    console->print_error(
        "One or more SECONDARY instances failed to apply retrieved "
        "transactions. Use the 'invalidateErrorInstances' option to "
        "perform the failover anyway by skipping and invalidating "
        "instances with errors.");
    throw shcore::Exception(
        shcore::str_format(
            "%zi instance(s) failed to apply retrieved transactions",
            errors.size()),
        SHERR_DBA_GTID_SYNC_ERROR);
  } else {
    console->print_info();
  }
}

Stop_channel_result stop_channel(const mysqlshdk::mysql::IInstance &instance,
                                 const std::string &channel_name, bool safe,
                                 bool dry_run) {
  log_info("Stopping channel '%s' at %s", channel_name.c_str(),
           instance.descr().c_str());

  if (dry_run) return Stop_channel_result::NOT_EXIST;

  try {
    mysqlshdk::mysql::Replication_channel channel;

    if (!mysqlshdk::mysql::get_channel_status(instance, channel_name,
                                              &channel)) {
      log_info("%s: Replication channel '%s' does not exist. Skipping stop.",
               instance.descr().c_str(), channel_name.c_str());
      return Stop_channel_result::NOT_EXIST;
    }

    if (channel.status() ==
        mysqlshdk::mysql::Replication_channel::Status::OFF) {
      log_info("%s: Replication channel '%s' is already stopped.",
               instance.descr().c_str(), channel_name.c_str());
      return Stop_channel_result::NOT_RUNNING;
    }

    // safe means we wait all transactions to be completely applied first
    if (safe) {
      if (!mysqlshdk::mysql::stop_replication_safe(
              instance, channel_name, k_replication_stop_timeout)) {
        throw shcore::Exception(
            "Could not stop replication channel at " + instance.descr() +
                " because there are unexpected open temporary tables.",
            SHERR_DBA_REPLICATION_INVALID);
      }
    } else {
      mysqlshdk::mysql::stop_replication(instance, channel_name);
    }

    // If the replication channel was stopped due to an error it's not
    // possible to configure it with a CHANGE REPLICA due to a protection
    // to avoid possible gaps in the replication stream. For that reason,
    // we must always reset the channel first to avoid being blocked with
    // that protection.
    if (channel.status() != mysqlshdk::mysql::Replication_channel::Status::ON) {
      log_info(
          "%s: Resetting channel because channel status of '%s' was not "
          "ON: %s",
          instance.descr().c_str(), channel_name.c_str(),
          mysqlshdk::mysql::format_status(channel).c_str());

      reset_channel(instance, channel_name, false, dry_run);
    }

    return Stop_channel_result::STOPPED;
  } catch (const shcore::Error &e) {
    throw shcore::Exception::mysql_error_with_code(e.what(), e.code());
  }

  return Stop_channel_result::NOT_EXIST;
}

void start_channel(const mysqlshdk::mysql::IInstance &instance,
                   const std::string &channel_name, bool dry_run) {
  log_info("Starting channel '%s' at %s", channel_name.c_str(),
           instance.descr().c_str());
  if (dry_run) return;

  try {
    mysqlshdk::mysql::start_replication(instance, channel_name);
  } catch (const shcore::Error &e) {
    mysqlsh::current_console()->print_error(
        shcore::str_format("%s: Error starting async replication channel: %s",
                           instance.descr().c_str(), e.format().c_str()));

    throw shcore::Exception::mysql_error_with_code(e.what(), e.code());
  }
}

void reset_channel(const mysqlshdk::mysql::IInstance &instance,
                   const std::string &channel_name, bool reset_credentials,
                   bool dry_run) {
  log_info("Resetting replica for channel '%s' at %s", channel_name.c_str(),
           instance.descr().c_str());
  if (dry_run) return;

  try {
    mysqlshdk::mysql::reset_slave(instance, channel_name, reset_credentials);
  } catch (const shcore::Error &e) {
    throw shcore::Exception::mysql_error_with_code(e.what(), e.code());
  }
}

void remove_channel(const mysqlshdk::mysql::IInstance &instance,
                    const std::string &channel_name, bool dry_run) {
  if (stop_channel(instance, channel_name, false, dry_run) !=
      Stop_channel_result::NOT_EXIST)
    reset_channel(instance, channel_name, true, dry_run);
}

void add_managed_connection_failover(
    const mysqlshdk::mysql::IInstance &target_instance,
    const mysqlshdk::mysql::IInstance &source, const std::string &channel_name,
    const std::string &network_namespace, int64_t primary_weight,
    int64_t secondary_weight, bool dry_run) {
  shcore::sqlstring query(
      "SELECT asynchronous_connection_failover_add_managed(?, ?, ?, ?, ?, ?, "
      "?, ?)",
      0);
  query << channel_name;
  query << "GroupReplication";
  query << source.get_group_name();
  query << source.get_canonical_hostname();
  query << source.get_canonical_port();
  query << network_namespace;
  query << primary_weight;
  query << secondary_weight;
  query.done();

  try {
    log_debug("Executing UDF: %s",
              query.str().c_str() + sizeof("SELECT"));  // hide "SELECT "
    if (!dry_run) {
      auto res = target_instance.query_udf(query);
      auto row = res->fetch_one();
      log_debug("UDF returned '%s'", row->get_string(0, "NULL").c_str());
    }
  } catch (const mysqlshdk::db::Error &error) {
    throw shcore::Exception::mysql_error_with_code_and_state(
        error.what(), error.code(), error.sqlstate());
  }
}

namespace {
void execute_add_source_connection_failover_udf(
    const mysqlshdk::mysql::IInstance &target_instance, const std::string &host,
    int port, const std::string &channel_name,
    const std::string &network_namespace, int64_t weight, bool dry_run) {
  shcore::sqlstring query(
      "SELECT asynchronous_connection_failover_add_source(?, ?, ?, ?, ?)", 0);
  query << channel_name;
  query << host;
  query << port;
  query << network_namespace;
  query << weight;
  query.done();

  try {
    log_debug("Executing UDF: %s",
              query.str().c_str() + sizeof("SELECT"));  // hide "SELECT "
    if (!dry_run) {
      auto res = target_instance.query_udf(query);
      auto row = res->fetch_one();
      log_debug("UDF returned '%s'", row->get_string(0, "NULL").c_str());
    }
  } catch (const mysqlshdk::db::Error &error) {
    throw shcore::Exception::mysql_error_with_code_and_state(
        error.what(), error.code(), error.sqlstate());
  }
}
}  // namespace

void add_source_connection_failover(
    const mysqlshdk::mysql::IInstance &target_instance,
    const mysqlshdk::mysql::IInstance &source, const std::string &channel_name,
    const std::string &network_namespace, int64_t weight, bool dry_run) {
  execute_add_source_connection_failover_udf(
      target_instance, source.get_canonical_hostname(),
      source.get_canonical_port(), channel_name, network_namespace, weight,
      dry_run);
}

void add_source_connection_failover(
    const mysqlshdk::mysql::IInstance &target_instance, const std::string &host,
    int port, const std::string &channel_name,
    const std::string &network_namespace, int64_t weight, bool dry_run) {
  execute_add_source_connection_failover_udf(target_instance, host, port,
                                             channel_name, network_namespace,
                                             weight, dry_run);
}

void delete_source_connection_failover(
    const mysqlshdk::mysql::IInstance &target_instance, const std::string &host,
    int port, const std::string &channel_name,
    const std::string &network_namespace, bool dry_run) {
  shcore::sqlstring query(
      "SELECT asynchronous_connection_failover_delete_source(?, ?, ?, ?)", 0);
  query << channel_name;
  query << host;
  query << port;
  query << network_namespace;
  query.done();

  try {
    log_debug("Executing UDF: %s",
              query.str().c_str() + sizeof("SELECT"));  // hide "SELECT "
    if (!dry_run) {
      auto res = target_instance.query_udf(query);
      auto row = res->fetch_one();
      log_debug("UDF returned '%s'", row->get_string(0, "NULL").c_str());
    }
  } catch (const mysqlshdk::db::Error &error) {
    throw shcore::Exception::mysql_error_with_code_and_state(
        error.what(), error.code(), error.sqlstate());
  }
}

void delete_managed_connection_failover(
    const mysqlshdk::mysql::IInstance &target_instance,
    const std::string &channel_name, bool dry_run) {
  std::string source_name =
      target_instance.queryf_one_string(0, "", R"*(SELECT managed_name
  FROM performance_schema.replication_asynchronous_connection_failover_managed
  WHERE managed_type = 'GroupReplication' AND channel_name=?)*",
                                        channel_name);

  if (source_name.empty()) {
    log_debug(
        "No entry (GroupReplication, %s) in "
        "replication_asynchronous_connection_failover_managed at %s",
        channel_name.c_str(), target_instance.descr().c_str());
    return;
  }

  auto query = "SELECT asynchronous_connection_failover_delete_managed(?,?)"_sql
               << channel_name << source_name;
  query.done();

  try {
    log_debug("Executing UDF: %s",
              query.str().c_str() + sizeof("SELECT"));  // hide "SELECT "
    if (!dry_run) {
      auto res = target_instance.query_udf(query);
      auto row = res->fetch_one();
      log_debug("UDF returned '%s'", row->get_string(0, "NULL").c_str());
    }
  } catch (const mysqlshdk::db::Error &error) {
    throw shcore::Exception::mysql_error_with_code_and_state(
        error.what(), error.code(), error.sqlstate());
  }
}

void reset_managed_connection_failover(
    const mysqlshdk::mysql::IInstance &target_instance, bool dry_run) {
  try {
    const char *query = "SELECT asynchronous_connection_failover_reset()";

    log_debug("Executing UDF: %s",
              query + sizeof("SELECT"));  // hide "SELECT "
    if (!dry_run) {
      // The server must be writeable to be able to execute the UDF
      bool super_read_only =
          target_instance.get_sysvar_bool("super_read_only", false);
      bool read_only = target_instance.get_sysvar_bool("read_only", false);

      shcore::on_leave_scope restore_read_only([super_read_only, read_only,
                                                &target_instance]() {
        if (super_read_only) {
          target_instance.set_sysvar("super_read_only", true,
                                     mysqlshdk::mysql::Var_qualifier::GLOBAL);
        }
        if (read_only) {
          target_instance.set_sysvar("read_only", true,
                                     mysqlshdk::mysql::Var_qualifier::GLOBAL);
        }
      });

      auto res = target_instance.query_udf(query);
      auto row = res->fetch_one();
      log_debug("UDF returned '%s'", row->get_string(0, "NULL").c_str());
    }
  } catch (const mysqlshdk::db::Error &error) {
    throw shcore::Exception::mysql_error_with_code_and_state(
        error.what(), error.code(), error.sqlstate());
  }
}

bool get_managed_connection_failover_configuration(
    const mysqlshdk::mysql::IInstance &instance,
    Managed_async_channel *out_channel_info) {
  // Check if the channel sources were configured manually or automatically
  auto result = instance.queryf(
      "SELECT MANAGED_NAME, CAST(CONFIGURATION->>'$.Primary_weight' AS "
      "UNSIGNED) AS primary_weight, CAST(CONFIGURATION->>'$.Secondary_weight' "
      "AS UNSIGNED) AS secondary_weight FROM "
      "performance_schema.replication_asynchronous_connection_failover_managed "
      "WHERE channel_name = ? AND managed_type = 'GroupReplication'",
      k_read_replica_async_channel_name);

  auto row = result->fetch_one();

  if (row && !row->is_null(0)) {
    // The sources were automatically configured
    out_channel_info->automatic_sources = true;
    out_channel_info->managed_name = row->get_string(0);
    out_channel_info->primary_weight = row->get_int(1);
    out_channel_info->secondary_weight = row->get_int(2);
  }

  // Get the list of sources
  result = instance.queryf(
      "SELECT host, port, weight FROM "
      "performance_schema.replication_asynchronous_connection_failover where "
      "channel_name = ? ORDER BY weight DESC",
      k_read_replica_async_channel_name);

  if (auto r = result->fetch_one_named()) {
    Managed_async_channel_source source = {};
    source.host = r.get_string("host");
    ;
    source.port = r.get_int("port");
    ;
    source.weight = r.get_int("weight");
    ;

    out_channel_info->sources.emplace(std::move(source));

    while ((r = result->fetch_one_named())) {
      Managed_async_channel_source src = {};
      src.host = r.get_string("host");
      ;
      src.port = r.get_int("port");
      ;
      src.weight = r.get_int("weight");
      ;

      out_channel_info->sources.emplace(std::move(src));
    }
  } else {
    return false;
  }

  // Check if SOURCE_CONNECTION_AUTO_FAILOVER is enabled or not
  result = instance.queryf(
      "SELECT SOURCE_CONNECTION_AUTO_FAILOVER FROM "
      "performance_schema.replication_connection_configuration WHERE "
      "channel_name = ?",
      k_read_replica_async_channel_name);

  row = result->fetch_one();

  if (row) {
    // The sources were automatically configured
    out_channel_info->automatic_failover = row->get_as_string(0) == "1";
    ;
  }

  return true;
}

void drop_clone_recovery_user_nobinlog(
    mysqlshdk::mysql::IInstance *target_instance,
    const mysqlshdk::mysql::Auth_options &account,
    const std::string &account_host) {
  log_info("Dropping account %s@%s at %s", account.user.c_str(),
           account_host.c_str(), target_instance->descr().c_str());

  try {
    mysqlshdk::mysql::Suppress_binary_log nobinlog(target_instance);
    target_instance->drop_user(account.user, account_host, true);
  } catch (const shcore::Error &) {
    auto console = current_console();
    console->print_warning(shcore::str_format(
        "%s: Error dropping account %s@%s.", target_instance->descr().c_str(),
        account.user.c_str(), account_host.c_str()));
    // ignore the error and move on
  }
}

void cleanup_clone_recovery(mysqlshdk::mysql::IInstance *recipient,
                            const mysqlshdk::mysql::Auth_options &clone_user,
                            const std::string &account_host) {
  // NOTE: disable binlog to avoid messing up with the GTID
  drop_clone_recovery_user_nobinlog(recipient, clone_user, account_host);

  // Reset clone settings
  recipient->set_sysvar_default("clone_valid_donor_list");

  // Uninstall the clone plugin
  log_info("Uninstalling the clone plugin on recipient '%s'.",
           recipient->get_canonical_address().c_str());
  mysqlshdk::mysql::uninstall_clone_plugin(*recipient, nullptr);
}

}  // namespace dba
}  // namespace mysqlsh
