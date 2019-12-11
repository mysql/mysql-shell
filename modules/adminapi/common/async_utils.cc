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

#include <list>
#include <string>

#include "modules/adminapi/common/async_utils.h"
#include "modules/adminapi/common/dba_errors.h"
#include "modules/adminapi/common/instance_pool.h"
#include "mysqlshdk/include/shellcore/console.h"
#include "mysqlshdk/libs/mysql/replication.h"

namespace mysqlsh {
namespace dba {

namespace {

void wait_pending_master_transactions(const std::string &master_gtid_set,
                                      mysqlshdk::mysql::IInstance *slave,
                                      int gtid_sync_timeout) {
  // Note: This function must be thread-safe

  try {
    // synchronize and wait
    if (!mysqlshdk::mysql::wait_for_gtid_set(*slave, master_gtid_set,
                                             gtid_sync_timeout)) {
      throw shcore::Exception("Timeout waiting for replica to synchronize",
                              SHERR_DBA_GTID_SYNC_TIMEOUT);
    }
  } catch (const mysqlshdk::db::Error &e) {
    throw shcore::Exception::mysql_error_with_code(e.what(), e.code());
  }
}
}  // namespace

/*
 * Synchronize all slaves to the master in parallel and then FTWRL on
 * everyone.
 *
 * Throws an exception on any error (including on just one of many slaves),
 * and in that case any acquired locks will be released.
 *
 * NOTE: All sessions are expected to be configured with a client-side read
 * timeout. This is because FTWRL does not have a built-in timeout. The read
 * timeout value must be larger than the current sync timeout, otherwise
 * the sync operations may timeout too early.
 */
void Global_locks::sync_and_lock_all(
    const std::list<Scoped_instance> &instances, const std::string &master_uuid,
    uint32_t gtid_sync_timeout, bool dry_run) {
  auto console = current_console();

  // NOTE: Set the master and list of slaves, otherwise UNLOCK TABLES is never
  // used when destroying the Global_locks object.

  // Find connection to the master
  for (const auto &i : instances) {
    if (i->get_uuid() == master_uuid) {
      m_master = i;
      break;
    }
  }
  if (!m_master) {
    throw shcore::Exception::runtime_error("PRIMARY instance not available");
  }

  // Set list of slaves (removing the master to avoid unlocking the same
  // instance twice later).
  m_slaves = instances;
  m_slaves.remove_if([this](const Scoped_instance &i) {
    return i->get_uuid() == m_master->get_uuid();
  });

  // 1 - pre-synchronize all slaves (in parallel)
  // This is to reduce the size of any backlog before we lock the master.
  console->print_info("** Pre-synchronizing SECONDARIES");

  std::string master_gtid_set =
      mysqlshdk::mysql::get_executed_gtid_set(*m_master);

  std::list<shcore::Dictionary_t> errors = execute_in_parallel(
      instances.begin(), instances.end(),
      // Execute a gtid sync in parallel and return an error or null
      [master_uuid, master_gtid_set,
       gtid_sync_timeout](const Scoped_instance &inst) {
        if (master_uuid != inst->get_uuid())
          wait_pending_master_transactions(master_gtid_set, inst.get(),
                                           gtid_sync_timeout);
      });

  if (!errors.empty()) {
    for (const auto &err : errors) {
      console->print_error(shcore::str_format(
          "%s: GTID sync failed: %s", err->get_string("from").c_str(),
          err->get_string("errmsg").c_str()));
    }
    throw shcore::Exception(
        shcore::str_format("%zi SECONDARY instance(s) failed to synchronize",
                           errors.size()),
        SHERR_DBA_GTID_SYNC_ERROR);
  }

  // 2 - lock master
  // This is to ensure that:
  // - master doesn't execute any new transactions
  // - global lock can't be acquired by someone else in master

  // This can block indefinitely, but the session is expected to have been
  // opened with a read-timeout, which would throw an exception
  console->print_info("** Acquiring global lock at PRIMARY");
  try {
    m_master->execute("FLUSH TABLES WITH READ LOCK");
  } catch (const shcore::Exception &e) {
    console->print_error(m_master->descr() +
                         ": FLUSH TABLES WITH READ LOCK failed: " + e.format());
    throw;
  }

  // Set SRO so that any attempts to do new writes at the PRIMARY error out
  // instead of blocking.
  if (!dry_run) m_master->execute("SET global super_read_only=1");

  m_master->execute("FLUSH BINARY LOGS");

  // 3 - synchronize and lock all slaves (in parallel)
  // Sync slaves to master once again and acquire lock
  console->print_info("** Acquiring global lock at SECONDARIES");

  // BUG#30344595: switching primary fails under client load
  // After locking the master and setting it to read-only, we must get the
  // updated (final) GTID_EXECUTED set, since new transactions might have been
  // committed during the pre-sync step, otherwise some trx might fail to be
  // applied on the secondaries (lost).
  master_gtid_set = mysqlshdk::mysql::get_executed_gtid_set(*m_master);

  for (const auto &inst : instances) {
    if (m_master->get_uuid() != inst->get_uuid()) {
      try {
        wait_pending_master_transactions(master_gtid_set, inst.get(),
                                         gtid_sync_timeout);
      } catch (const shcore::Exception &e) {
        console->print_error(shcore::str_format("%s: GTID sync failed: %s",
                                                inst->descr().c_str(),
                                                e.format().c_str()));
        throw;
      }
      try {
        inst->execute("FLUSH TABLES WITH READ LOCK");
      } catch (const shcore::Exception &e) {
        console->print_error(
            inst->descr() +
            ": FLUSH TABLES WITH READ LOCK failed: " + e.format());
        throw;
      }
    }
  }
}

void Global_locks::acquire(const std::list<Scoped_instance> &instances,
                           const std::string &master_uuid,
                           uint32_t gtid_sync_timeout, bool dry_run) {
  current_console()->print_info("* Acquiring locks in replicaset instances");

  sync_and_lock_all(instances, master_uuid, gtid_sync_timeout, dry_run);

  current_console()->print_info();
}

Global_locks::~Global_locks() {
  log_info("Releasing global locks");

  if (m_master) {
    try {
      m_master->execute("UNLOCK TABLES");
    } catch (const shcore::Error &e) {
      log_warning("%s: UNLOCK TABLES failed: %s", m_master->descr().c_str(),
                  e.format().c_str());
    }
  }

  for (const auto &inst : m_slaves) {
    try {
      inst->execute("UNLOCK TABLES");
    } catch (const shcore::Error &e) {
      log_warning("%s: UNLOCK TABLES failed: %s", inst->descr().c_str(),
                  e.format().c_str());
    }
  }
}

}  // namespace dba
}  // namespace mysqlsh
