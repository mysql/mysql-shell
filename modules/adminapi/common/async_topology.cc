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

namespace mysqlsh {
namespace dba {

// static constexpr const int k_replication_start_timeout = 60;
static constexpr const int k_replication_stop_timeout = 60;

void change_master(mysqlshdk::mysql::IInstance *slave,
                   mysqlshdk::mysql::IInstance *new_master,
                   const std::string &channel_name,
                   const Async_replication_options &repl_options,
                   bool dry_run) {
  auto console = current_console();
  mysqlshdk::mysql::Replication_channel channel;

  console->print_info(shcore::str_format(
      "** Changing replication source of %s to %s", slave->descr().c_str(),
      new_master->get_canonical_address().c_str()));

  try {
    // if credentials given, change everything, otherwise just change the
    // host:port
    if (repl_options.repl_credentials) {
      try {
        if (!dry_run) {
          mysqlshdk::mysql::change_master(
              slave, new_master->get_canonical_hostname(),
              new_master->get_canonical_port(), channel_name,
              repl_options.repl_credentials.get_safe(), repl_options.ssl_mode,
              repl_options.master_connect_retry,
              repl_options.master_retry_count, repl_options.auto_failover);
        }
      } catch (const shcore::Error &e) {
        console->print_error(
            shcore::str_format("Error setting up async replication channel: %s",
                               e.format().c_str()));
        throw shcore::Exception::mysql_error_with_code(e.what(), e.code());
      }

    } else {
      if (!dry_run) {
        mysqlshdk::mysql::change_master_host_port(
            slave, new_master->get_canonical_hostname(),
            new_master->get_canonical_port(), channel_name);
      }
    }
  } catch (const shcore::Error &e) {
    console->print_error(
        shcore::str_format("%s: Error setting up async replication channel: %s",
                           slave->descr().c_str(), e.format().c_str()));
    throw shcore::Exception::mysql_error_with_code(e.what(), e.code());
  }
}

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
  stop_channel(target, channel_name, true, dry_run);

  change_master(target, primary, channel_name, repl_options, dry_run);

  if (start_replica) start_channel(target, channel_name, dry_run);

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
                          const Async_replication_options &rpl_options) {
  auto console = mysqlsh::current_console();

  stop_channel(target, channel_name, true, false);

  // Setting primary for target instance and restarting replication.
  change_master(target, primary, channel_name, rpl_options, false);

  start_channel(target, channel_name, false);

  console->print_info("** Checking replication channel status...");
  // NOTE: We use a fixed timeout of 60 sec to wait for replication to start.
  //       Should we define a timout option for this?
  try {
    check_replication_startup(*target, channel_name);
  } catch (const shcore::Exception &e) {
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
  remove_channel(target, channel_name, dry_run);

  log_info("Fencing removed instance %s", target->descr().c_str());
  if (!dry_run) fence_instance(target);
}

void async_update_replica_credentials(
    mysqlshdk::mysql::IInstance *target, const std::string &channel_name,
    const Async_replication_options &rpl_options, bool dry_run) {
  using mysqlshdk::mysql::Replication_channel;

  Stop_channel_result channel_stopped =
      stop_channel(target, channel_name, false, dry_run);

  if (!dry_run) {
    change_replication_credentials(
        *target, rpl_options.repl_credentials->user,
        rpl_options.repl_credentials->password.get_safe(), channel_name);
  }

  if (channel_stopped == Stop_channel_result::STOPPED)
    start_channel(target, channel_name, dry_run);
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
        [=]() { start_channel(promoted, channel_name, dry_run); });

  // Stop slave at the instance being promoted
  stop_channel(promoted, channel_name, true, dry_run);

  if (undo_list)
    undo_list->push_front(
        [=]() { reset_channel(current_primary, channel_name, true, dry_run); });

  // Make old master a slave of new master
  change_master(current_primary, promoted, channel_name, repl_options, dry_run);

  if (undo_list)
    undo_list->push_front(
        [=]() { stop_channel(current_primary, channel_name, true, dry_run); });
  start_channel(current_primary, channel_name, dry_run);

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
                         const std::string &channel_name,
                         const Async_replication_options & /*repl_options*/,
                         bool dry_run) {
  // unfence the new master
  log_info("Clearing SUPER_READ_ONLY in new PRIMARY %s",
           promoted->descr().c_str());
  if (!dry_run) unfence_instance(promoted, true);

  // Stop slave at the instance being promoted
  stop_channel(promoted, channel_name, true, dry_run);
}

void undo_async_force_primary(mysqlshdk::mysql::IInstance *promoted,
                              const std::string &channel_name, bool dry_run) {
  // Start slave on candidate (revert stop slave).
  start_channel(promoted, channel_name, dry_run);

  // Fence the candidate (revert unfence).
  log_info("Re-enabling SUPER_READ_ONLY on %s", promoted->descr().c_str());
  if (!dry_run) fence_instance(promoted);
}

void async_create_channel(mysqlshdk::mysql::IInstance *target,
                          mysqlshdk::mysql::IInstance *primary,
                          const std::string &channel_name,
                          const Async_replication_options &repl_options,
                          bool dry_run) {
  change_master(target, primary, channel_name, repl_options, dry_run);
}

void async_change_primary(mysqlshdk::mysql::IInstance *replica,
                          mysqlshdk::mysql::IInstance *primary,
                          const std::string &channel_name,
                          const Async_replication_options &repl_options,
                          bool start_replica, bool dry_run) {
  // make sure it's fenced
  if (!dry_run) fence_instance(replica);

  stop_channel(replica, channel_name, true, dry_run);

  // This will re-point the slave to the new master without changing any
  // other replication parameter if repl_options has no credentials.
  async_create_channel(replica, primary, channel_name, repl_options, dry_run);

  if (start_replica) start_channel(replica, channel_name, dry_run);

  log_info("PRIMARY changed for instance %s", replica->descr().c_str());
}

void async_change_primary(
    mysqlshdk::mysql::IInstance *primary,
    const std::list<std::shared_ptr<Instance>> &secondaries,
    const std::string &channel_name,
    const Async_replication_options & /*repl_options*/,
    mysqlshdk::mysql::IInstance *old_primary,
    shcore::Scoped_callback_list *undo_list, bool dry_run) {
  for (const auto &slave : secondaries) {
    if (slave->get_uuid() != primary->get_uuid() &&
        (!old_primary || slave->get_uuid() != old_primary->get_uuid())) {
      undo_list->push_front([=]() {
        if (old_primary)
          async_change_primary(slave.get(), old_primary, channel_name, {}, true,
                               dry_run);
      });

      async_change_primary(slave.get(), primary, channel_name, {}, true,
                           dry_run);

      log_info("PRIMARY changed for instance %s", slave->descr().c_str());
    }
  }
}

void wait_apply_retrieved_trx(mysqlshdk::mysql::IInstance *instance,
                              int timeout_sec) {
  std::string gtid_set = mysqlshdk::mysql::get_received_gtid_set(
      *instance, k_replicaset_channel_name);

  auto console = mysqlsh::current_console();
  console->print_info("** Waiting for received transactions to be applied at " +
                      instance->descr());

  try {
    if (!mysqlshdk::mysql::wait_for_gtid_set(*instance, gtid_set,
                                             timeout_sec)) {
      throw shcore::Exception(
          "Timeout waiting for retrieved transactions to be applied on "
          "instance " +
              instance->descr(),
          SHERR_DBA_GTID_SYNC_TIMEOUT);
    }
  } catch (const shcore::Error &e) {
    throw shcore::Exception::mysql_error_with_code(e.what(), e.code());
  }
}

void wait_all_apply_retrieved_trx(
    std::list<std::shared_ptr<Instance>> *instances, int timeout_sec,
    bool invalidate_error_instances,
    std::vector<Instance_metadata> *out_instances_md,
    std::list<Instance_id> *out_invalidate_ids) {
  auto console = current_console();

  console->print_info("* Waiting for all received transactions to be applied");
  std::list<shcore::Dictionary_t> errors =
      execute_in_parallel(instances->begin(), instances->end(),
                          // Wait for retrieved GTIDs to be applied in parallel
                          // and return an error or null.
                          [timeout_sec](const std::shared_ptr<Instance> &inst) {
                            wait_apply_retrieved_trx(inst.get(), timeout_sec);
                          });

  if (!errors.empty()) {
    for (const auto &err : errors) {
      std::string instance_uuid = err->get_string("uuid");
      std::string instance_desc = err->get_string("from");
      std::string err_msg{shcore::str_format(
          "%s: Failed to apply retrieved GTIDs: %s", instance_desc.c_str(),
          err->get_string("errmsg").c_str())};

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
}

Stop_channel_result stop_channel(mysqlshdk::mysql::IInstance *instance,
                                 const std::string &channel_name, bool safe,
                                 bool dry_run) {
  auto console = mysqlsh::current_console();
  Stop_channel_result channel_stopped = Stop_channel_result::NOT_EXIST;

  log_info("Stopping channel '%s' at %s", channel_name.c_str(),
           instance->descr().c_str());
  if (!dry_run) {
    try {
      mysqlshdk::mysql::Replication_channel channel;

      if (!mysqlshdk::mysql::get_channel_status(*instance, channel_name,
                                                &channel)) {
        log_info("%s: Replication channel '%s' does not exist. Skipping stop.",
                 instance->descr().c_str(), channel_name.c_str());
        channel_stopped = Stop_channel_result::NOT_EXIST;
      } else if (channel.status() ==
                 mysqlshdk::mysql::Replication_channel::Status::OFF) {
        log_info("%s: Replication channel '%s' is already stopped.",
                 instance->descr().c_str(), channel_name.c_str());
        channel_stopped = Stop_channel_result::NOT_RUNNING;
      } else {
        // safe means we wait all transactions to be completely applied first
        if (safe) {
          if (!mysqlshdk::mysql::stop_replication_safe(
                  instance, channel_name, k_replication_stop_timeout)) {
            throw shcore::Exception(
                "Could not stop replication channel at " + instance->descr() +
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
        if (channel.status() !=
            mysqlshdk::mysql::Replication_channel::Status::ON) {
          log_info(
              "%s: Resetting channel because channel status of '%s' was not "
              "ON: %s",
              instance->descr().c_str(), channel_name.c_str(),
              mysqlshdk::mysql::format_status(channel).c_str());

          reset_channel(instance, channel_name, false, dry_run);
        }

        channel_stopped = Stop_channel_result::STOPPED;
      }
    } catch (const shcore::Error &e) {
      throw shcore::Exception::mysql_error_with_code(e.what(), e.code());
    }
  }

  return channel_stopped;
}

void start_channel(mysqlshdk::mysql::IInstance *instance,
                   const std::string &channel_name, bool dry_run) {
  auto console = mysqlsh::current_console();

  log_info("Starting channel '%s' at %s", channel_name.c_str(),
           instance->descr().c_str());
  if (!dry_run) {
    try {
      mysqlshdk::mysql::start_replication(instance, channel_name);
    } catch (const shcore::Error &e) {
      console->print_error(
          shcore::str_format("%s: Error starting async replication channel: %s",
                             instance->descr().c_str(), e.format().c_str()));

      throw shcore::Exception::mysql_error_with_code(e.what(), e.code());
    }
  }
}

void reset_channel(mysqlshdk::mysql::IInstance *instance,
                   const std::string &channel_name, bool reset_credentials,
                   bool dry_run) {
  log_info("Resetting replica for channel '%s' at %s", channel_name.c_str(),
           instance->descr().c_str());
  if (!dry_run) {
    try {
      mysqlshdk::mysql::reset_slave(instance, channel_name, reset_credentials);
    } catch (const shcore::Error &e) {
      throw shcore::Exception::mysql_error_with_code(e.what(), e.code());
    }
  }
}

void remove_channel(mysqlshdk::mysql::IInstance *instance,
                    const std::string &channel_name, bool dry_run) {
  if (stop_channel(instance, channel_name, false, dry_run) !=
      Stop_channel_result::NOT_EXIST)
    reset_channel(instance, channel_name, true, dry_run);
}

void add_managed_connection_failover(
    const mysqlshdk::mysql::IInstance &target_instance,
    const mysqlshdk::mysql::IInstance &source, const std::string &channel_name,
    const std::string &network_namespace, bool dry_run, int64_t primary_weight,
    int64_t secondary_weight) {
  std::string managed_type = "GroupReplication";

  shcore::sqlstring query(
      "SELECT asynchronous_connection_failover_add_managed(?, ?, ?, ?, ?, ?, "
      "?, ?)",
      0);
  query << channel_name;
  query << managed_type;
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
  } else {
    shcore::sqlstring query(
        "SELECT asynchronous_connection_failover_delete_managed(?,?)", 0);
    query << channel_name;
    query << source_name;
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
}

void create_clone_recovery_user_nobinlog(
    mysqlshdk::mysql::IInstance *target_instance,
    const mysqlshdk::mysql::Auth_options &donor_account,
    const std::string &account_host, bool dry_run) {
  log_info("Creating clone recovery user %s@%s at %s%s.",
           donor_account.user.c_str(), account_host.c_str(),
           target_instance->descr().c_str(), dry_run ? " (dryRun)" : "");

  if (!dry_run) {
    try {
      auto console = mysqlsh::current_console();

      mysqlshdk::mysql::Suppress_binary_log nobinlog(target_instance);
      // Create recovery user for clone equal to the donor user
      mysqlshdk::mysql::create_user_with_password(
          *target_instance, donor_account.user, {account_host},
          {std::make_tuple("CLONE_ADMIN, EXECUTE", "*.*", false),
           std::make_tuple("SELECT", "performance_schema.*", false)},
          *donor_account.password);
    } catch (const shcore::Error &e) {
      throw shcore::Exception::mysql_error_with_code(e.what(), e.code());
    }
  }
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
  } catch (const shcore::Error &e) {
    auto console = current_console();
    console->print_warning(shcore::str_format(
        "%s: Error dropping account %s@%s.", target_instance->descr().c_str(),
        account.user.c_str(), account_host.c_str()));
    // ignore the error and move on
  }
}

void refresh_target_connections(mysqlshdk::mysql::IInstance *target) {
  Connection_options opts = target->get_connection_options();

  try {
    target->query("SELECT 1");
  } catch (const shcore::Error &e) {
    if (mysqlshdk::db::is_mysql_client_error(e.code())) {
      log_debug(
          "Connection to instance '%s' is lost: %s. Re-establishing a "
          "connection.",
          target->get_canonical_address().c_str(), e.format().c_str());

      target->get_session()->connect(opts);
    } else {
      throw;
    }
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
