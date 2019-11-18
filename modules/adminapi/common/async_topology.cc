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

#include "modules/adminapi/common/async_topology.h"

#include "modules/adminapi/common/async_replication_options.h"
#include "modules/adminapi/common/common.h"
#include "modules/adminapi/common/dba_errors.h"
#include "modules/adminapi/common/instance_pool.h"
#include "mysqlshdk/include/shellcore/console.h"
#include "mysqlshdk/libs/mysql/async_replication.h"
#include "mysqlshdk/libs/utils/logger.h"

namespace mysqlsh {
namespace dba {

// static constexpr const int k_replication_start_timeout = 60;
static constexpr const int k_replication_stop_timeout = 60;

namespace {

void start_channel(mysqlshdk::mysql::IInstance *instance,
                   const std::string &channel_name, bool dry_run) {
  auto console = mysqlsh::current_console();

  log_info("Starting channel '%s' at %s", channel_name.c_str(),
           instance->descr().c_str());
  if (!dry_run) {
    try {
      mysqlshdk::mysql::start_replication(instance, channel_name);
    } catch (const mysqlshdk::db::Error &e) {
      throw shcore::Exception::mysql_error_with_code_and_state(
          e.what(), e.code(), e.sqlstate());
    }
  }
}

void reset_channel(mysqlshdk::mysql::IInstance *instance,
                   const std::string &channel_name, bool reset_credentials,
                   bool dry_run) {
  log_info("Resetting slave for channel '%s' at %s", channel_name.c_str(),
           instance->descr().c_str());
  if (!dry_run) {
    try {
      mysqlshdk::mysql::reset_slave(instance, channel_name, reset_credentials);
    } catch (const mysqlshdk::db::Error &e) {
      throw shcore::Exception::mysql_error_with_code_and_state(
          e.what(), e.code(), e.sqlstate());
    }
  }
}

void setup_slave(mysqlshdk::mysql::IInstance *master,
                 mysqlshdk::mysql::IInstance *instance,
                 const std::string &channel_name,
                 const Async_replication_options &repl_options, bool dry_run,
                 bool stop_slave = false) {
  auto console = mysqlsh::current_console();

  console->print_info(shcore::str_format(
      "** Configuring %s to replicate from %s", instance->descr().c_str(),
      master->get_canonical_address().c_str()));

  if (stop_slave) {
    log_info("Stopping replication at %s ...", instance->descr().c_str());
    stop_channel(instance, channel_name, dry_run);
  }

  try {
    try {
      if (!dry_run)
        mysqlshdk::mysql::change_master(
            instance, master->get_canonical_hostname(),
            master->get_canonical_port(), channel_name,
            repl_options.repl_credentials.get_safe(),
            repl_options.master_connect_retry, repl_options.master_retry_count,
            repl_options.master_delay);
    } catch (const mysqlshdk::db::Error &e) {
      console->print_error(
          shcore::str_format("Error setting up async replication channel: %s",
                             e.format().c_str()));
      throw shcore::Exception::mysql_error_with_code_and_state(
          e.what(), e.code(), e.sqlstate());
    }

    log_info("Starting replication at %s ...", instance->descr().c_str());
    try {
      if (!dry_run) mysqlshdk::mysql::start_replication(instance, channel_name);
    } catch (const mysqlshdk::db::Error &e) {
      console->print_error(shcore::str_format(
          "Error starting async replication channel: %s", e.format().c_str()));
      throw shcore::Exception::mysql_error_with_code_and_state(
          e.what(), e.code(), e.sqlstate());
    }
  } catch (const std::exception &e) {
    log_error("%s", e.what());

    throw shcore::Exception::runtime_error(
        "Error setting up async replication: " + std::string(e.what()));
  }
}

void change_master_instance(mysqlshdk::mysql::IInstance *slave,
                            mysqlshdk::mysql::IInstance *new_master,
                            const std::string &channel_name, bool dry_run) {
  auto console = current_console();

  console->print_info(shcore::str_format(
      "** Changing replication source of %s to %s", slave->descr().c_str(),
      new_master->get_canonical_address().c_str()));

  log_info("Stopping replication at %s ...", slave->descr().c_str());
  stop_channel(slave, channel_name, dry_run);

  try {
    if (!dry_run)
      mysqlshdk::mysql::change_master_host_port(
          slave, new_master->get_canonical_hostname(),
          new_master->get_canonical_port(), channel_name);
  } catch (const mysqlshdk::db::Error &e) {
    console->print_error(
        shcore::str_format("%s: Error setting up async replication channel: %s",
                           slave->descr().c_str(), e.format().c_str()));
    throw shcore::Exception::mysql_error_with_code_and_state(e.what(), e.code(),
                                                             e.sqlstate());
  }

  log_info("Starting replication at %s ...", slave->descr().c_str());
  try {
    if (!dry_run) mysqlshdk::mysql::start_replication(slave, channel_name);
  } catch (const mysqlshdk::db::Error &e) {
    console->print_error(
        shcore::str_format("%s: Error starting async replication channel: %s",
                           slave->descr().c_str(), e.format().c_str()));
    throw shcore::Exception::mysql_error_with_code_and_state(e.what(), e.code(),
                                                             e.sqlstate());
  }
}
}  // namespace

static const char *k_channel_name = "";

void fence_instance(mysqlshdk::mysql::IInstance *instance) {
  try {
    instance->set_sysvar("SUPER_READ_ONLY", true,
                         mysqlshdk::mysql::Var_qualifier::PERSIST);
  } catch (const mysqlshdk::db::Error &e) {
    throw shcore::Exception::mysql_error_with_code_and_state(e.what(), e.code(),
                                                             e.sqlstate());
  }
}

void unfence_instance(mysqlshdk::mysql::IInstance *instance) {
  try {
    instance->set_sysvar("SUPER_READ_ONLY", false,
                         mysqlshdk::mysql::Var_qualifier::PERSIST);
    // Set SUPER_READ_ONLY=1 will also set READ_ONLY
    instance->set_sysvar("READ_ONLY", false,
                         mysqlshdk::mysql::Var_qualifier::PERSIST);
  } catch (const mysqlshdk::db::Error &e) {
    throw shcore::Exception::mysql_error_with_code_and_state(e.what(), e.code(),
                                                             e.sqlstate());
  }
}

void async_add_replica(mysqlshdk::mysql::IInstance *primary,
                       mysqlshdk::mysql::IInstance *target,
                       const Async_replication_options &repl_options,
                       bool dry_run) {
  setup_slave(primary, target, k_channel_name, repl_options, dry_run);

  // The fence added here is going to be cleared when a GR group
  // is created, but we set it up anyway in case something goes wrong
  // before that.
  log_info("Fencing new instance '%s' to prevent updates.",
           target->descr().c_str());
  if (!dry_run) fence_instance(target);
}

void async_rejoin_replica(mysqlshdk::mysql::IInstance *primary,
                          mysqlshdk::mysql::IInstance *target,
                          const Async_replication_options &rpl_options) {
  auto console = mysqlsh::current_console();

  // Setting primary for target instance and restarting replication.
  setup_slave(primary, target, k_channel_name, rpl_options, false, true);

  console->print_info("** Checking replication channel status...");
  // NOTE: We use a fixed timeout of 60 sec to wait for replication to start.
  //       Should we define a timout option for this?
  try {
    check_replication_startup(*target, k_channel_name);
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

void async_remove_replica(mysqlshdk::mysql::IInstance *target, bool dry_run) {
  // stop slave and clear credentials from slave_master_info
  stop_channel(target, k_channel_name, false);
  reset_channel(target, k_channel_name, true, false);

  log_info("Fencing removed instance %s", target->descr().c_str());
  if (!dry_run) fence_instance(target);
}

void async_set_primary(mysqlshdk::mysql::IInstance *current_primary,
                       mysqlshdk::mysql::IInstance *promoted,
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

  undo_list->push_front([=]() {
    if (!dry_run) unfence_instance(current_primary);
  });
  if (!dry_run) fence_instance(current_primary);

  undo_list->push_front([=]() {
    if (!dry_run) mysqlshdk::mysql::start_replication(promoted, k_channel_name);
  });

  // Stop slave at the instance being promoted
  stop_channel(promoted, k_channel_name, dry_run);

  undo_list->push_front(
      [=]() { reset_channel(current_primary, true, dry_run); });

  // Make old master a slave of new master
  setup_slave(promoted, current_primary, k_channel_name, repl_options, dry_run);

  undo_list->push_front([=]() {
    if (!dry_run) fence_instance(promoted);
  });

  // unfence the new master
  log_info("Clearing SUPER_READ_ONLY in new PRIMARY %s",
           promoted->descr().c_str());
  if (!dry_run) unfence_instance(promoted);
}

void async_force_primary(mysqlshdk::mysql::IInstance *promoted,
                         const Async_replication_options & /*repl_options*/,
                         bool dry_run) {
  // unfence the new master
  log_info("Clearing SUPER_READ_ONLY in new PRIMARY %s",
           promoted->descr().c_str());
  if (!dry_run) unfence_instance(promoted);

  // Stop slave at the instance being promoted
  stop_channel(promoted, k_channel_name, dry_run);
}

void undo_async_force_primary(mysqlshdk::mysql::IInstance *promoted,
                              bool dry_run) {
  // Start slave on candidate (revert stop slave).
  start_channel(promoted, k_channel_name, dry_run);

  // Fence the candidate (revert unfence).
  log_info("Re-enabling SUPER_READ_ONLY on %s", promoted->descr().c_str());
  if (!dry_run) fence_instance(promoted);
}

void async_change_primary(mysqlshdk::mysql::IInstance *primary,
                          const std::list<Scoped_instance> &secondaries,
                          const Async_replication_options & /*repl_options*/,
                          mysqlshdk::mysql::IInstance *old_primary,
                          shcore::Scoped_callback_list *undo_list,
                          bool dry_run) {
  for (const auto &slave : secondaries) {
    if (slave.get() != primary && slave.get() != old_primary) {
      // make sure it's fenced
      if (!dry_run) fence_instance(slave.get());

      // This will re-point the slave to the new master without changing any
      // other replication parameter.

      auto slave_ptr = slave.get();

      undo_list->push_front([=]() {
        change_master_instance(slave_ptr, old_primary, k_channel_name, dry_run);
      });

      change_master_instance(slave_ptr, primary, k_channel_name, dry_run);

      log_info("PRIMARY changed for instance %s", slave->descr().c_str());
    }
  }
}

void wait_apply_retrieved_trx(mysqlshdk::mysql::IInstance *instance,
                              int timeout_sec) {
  std::string gtid_set =
      mysqlshdk::mysql::get_received_gtid_set(*instance, k_channel_name);

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
  } catch (const mysqlshdk::db::Error &e) {
    throw shcore::Exception::mysql_error_with_code_and_state(e.what(), e.code(),
                                                             e.sqlstate());
  }
}

void wait_all_apply_retrieved_trx(
    std::list<Scoped_instance> *instances, int timeout_sec,
    bool invalidate_error_instances,
    std::vector<Instance_metadata> *out_instances_md,
    std::list<Instance_id> *out_invalidate_ids) {
  auto console = current_console();

  console->print_info("* Waiting for all received transactions to be applied");
  std::list<shcore::Dictionary_t> errors =
      execute_in_parallel(instances->begin(), instances->end(),
                          // Wait for retrieved GTIDs to be applied in parallel
                          // and return an error or null.
                          [timeout_sec](const Scoped_instance &inst) {
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
        instances->remove_if([&instance_uuid](const Scoped_instance &i) {
          return i->get_uuid() == instance_uuid;
        });
        out_instances_md->erase(md_it);
      }
    }

    if (!invalidate_error_instances) {
      console->print_error(
          "One or more SECONDARY instances failed to apply retrieved "
          "transactions. Use the 'invalidateErrorInstances' option to perform "
          "the failover anyway by skipping and invalidating instances with "
          "errors.");
      throw shcore::Exception(
          shcore::str_format(
              "%zi instance(s) failed to apply retrieved transactions",
              errors.size()),
          SHERR_DBA_GTID_SYNC_ERROR);
    } else {
      console->println();
    }
  }
}

void reset_channel(mysqlshdk::mysql::IInstance *instance,
                   bool reset_credentials, bool dry_run) {
  reset_channel(instance, k_channel_name, reset_credentials, dry_run);
}

void stop_channel(mysqlshdk::mysql::IInstance *instance,
                  const std::string &channel_name, bool dry_run) {
  auto console = mysqlsh::current_console();

  log_info("Stopping channel '%s' at %s", channel_name.c_str(),
           instance->descr().c_str());
  if (!dry_run) {
    try {
      if (!mysqlshdk::mysql::stop_replication_safe(
              instance, channel_name, k_replication_stop_timeout)) {
        throw shcore::Exception(
            "Could not stop replication channel at " + instance->descr() +
                " because there are unexpected open temporary tables.",
            SHERR_DBA_REPLICATION_INVALID);
      }
    } catch (const mysqlshdk::db::Error &e) {
      throw shcore::Exception::mysql_error_with_code_and_state(
          e.what(), e.code(), e.sqlstate());
    }
  }
}

}  // namespace dba
}  // namespace  mysqlsh
