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

#include "mysqlshdk/libs/mysql/async_replication.h"

#include <tuple>
#include "mysqlshdk/libs/mysql/group_replication.h"
#include "mysqlshdk/libs/mysql/utils.h"
#include "mysqlshdk/libs/utils/logger.h"

namespace mysqlshdk {
namespace mysql {

void change_master(mysqlshdk::mysql::IInstance *instance,
                   const std::string &master_host, int master_port,
                   const std::string &channel_name,
                   const Auth_options &credentials,
                   const mysqlshdk::utils::nullable<int> master_connect_retry,
                   const mysqlshdk::utils::nullable<int> master_retry_count,
                   const mysqlshdk::utils::nullable<int> master_delay) {
  log_info(
      "Setting up async source for channel '%s' of %s to %s:%i (user "
      "%s)",
      channel_name.c_str(), instance->descr().c_str(), master_host.c_str(),
      master_port,
      (credentials.user.empty() ? "unchanged" : "'" + credentials.user + "'")
          .c_str());

  try {
    std::string options;
    if (!master_retry_count.is_null())
      options.append(", MASTER_RETRY_COUNT=" +
                     std::to_string(*master_retry_count));
    if (!master_connect_retry.is_null())
      options.append(", MASTER_CONNECT_RETRY=" +
                     std::to_string(*master_connect_retry));
    if (!master_delay.is_null())
      options.append(", MASTER_DELAY=" + std::to_string(*master_delay));

    instance->executef(
        "CHANGE MASTER TO /*!80011 get_master_public_key=1, */"
        " MASTER_HOST=/*(*/ ? /*)*/, MASTER_PORT=/*(*/ ? /*)*/,"
        " MASTER_USER=?, MASTER_PASSWORD=/*((*/ ? /*))*/" +
            options +
            ", MASTER_AUTO_POSITION=1"
            " FOR CHANNEL ?",
        master_host, master_port, credentials.user, *credentials.password,
        channel_name);
  } catch (const std::exception &e) {
    log_error("Error setting up async replication: %s", e.what());
    throw;
  }
}

void change_master_host_port(mysqlshdk::mysql::IInstance *instance,
                             const std::string &master_host, int master_port,
                             const std::string &channel_name) {
  log_info("Changing source address for channel '%s' of %s to %s:%i",
           channel_name.c_str(), instance->descr().c_str(), master_host.c_str(),
           master_port);

  try {
    instance->executef(
        "CHANGE MASTER TO "
        " MASTER_HOST=/*(*/ ? /*)*/, MASTER_PORT=/*(*/ ? /*)*/"
        " FOR CHANNEL ?",
        master_host, master_port, channel_name);
  } catch (const std::exception &e) {
    log_error("Error setting up async replication: %s", e.what());
    throw;
  }
}

void reset_slave(mysqlshdk::mysql::IInstance *instance,
                 const std::string &channel_name, bool reset_credentials) {
  log_debug("Resetting replica%s channel '%s' for %s...",
            reset_credentials ? " ALL" : "", channel_name.c_str(),
            instance->descr().c_str());
  std::string replica_term = mysqlshdk::mysql::get_replica_keyword(*instance);

  if (reset_credentials) {
    instance->executef("RESET " + replica_term + " ALL FOR CHANNEL ?",
                       channel_name);
  } else {
    instance->executef("RESET " + replica_term + " FOR CHANNEL ?",
                       channel_name);
  }
}

void start_replication(mysqlshdk::mysql::IInstance *instance,
                       const std::string &channel_name) {
  log_debug("Starting replica channel %s for %s...", channel_name.c_str(),
            instance->descr().c_str());
  std::string replica_term = mysqlshdk::mysql::get_replica_keyword(*instance);

  instance->executef("START " + replica_term + " FOR CHANNEL ?", channel_name);
}

void stop_replication(mysqlshdk::mysql::IInstance *instance,
                      const std::string &channel_name) {
  log_debug("Stopping replica channel %s for %s...", channel_name.c_str(),
            instance->descr().c_str());
  std::string replica_term = mysqlshdk::mysql::get_replica_keyword(*instance);

  instance->executef("STOP " + replica_term + " FOR CHANNEL ?", channel_name);
}

bool stop_replication_safe(mysqlshdk::mysql::IInstance *instance,
                           const std::string &channel_name, int timeout_sec) {
  log_debug("Stopping replica channel %s for %s...", channel_name.c_str(),
            instance->descr().c_str());
  std::string replica_term = mysqlshdk::mysql::get_replica_keyword(*instance);

  while (timeout_sec-- >= 0) {
    instance->executef("STOP " + replica_term + " FOR CHANNEL ?", channel_name);

    auto n = instance->queryf_one_string(
        1, "0", "SHOW STATUS LIKE 'Slave_open_temp_tables'");

    if (n == "0") return true;

    log_warning(
        "Slave_open_tables has unexpected value %s at %s (unsupported SBR in "
        "use?)",
        n.c_str(), instance->descr().c_str());

    instance->executef("START " + replica_term + " FOR CHANNEL ?",
                       channel_name);

    shcore::sleep_ms(1000);
  }

  return false;
}

void start_replication_receiver(mysqlshdk::mysql::IInstance *instance,
                                const std::string &channel_name) {
  log_debug("Starting replica io_thread %s for %s...", channel_name.c_str(),
            instance->descr().c_str());
  instance->executef("START " +
                         mysqlshdk::mysql::get_replica_keyword(*instance) +
                         " IO_THREAD FOR CHANNEL ?",
                     channel_name);
}

void stop_replication_receiver(mysqlshdk::mysql::IInstance *instance,
                               const std::string &channel_name) {
  log_debug("Stopping replica io_thread %s for %s...", channel_name.c_str(),
            instance->descr().c_str());
  instance->executef("STOP " +
                         mysqlshdk::mysql::get_replica_keyword(*instance) +
                         " IO_THREAD FOR CHANNEL ?",
                     channel_name);
}

void start_replication_applier(mysqlshdk::mysql::IInstance *instance,
                               const std::string &channel_name) {
  log_debug("Starting replica sql_thread %s for %s...", channel_name.c_str(),
            instance->descr().c_str());
  instance->executef("START " +
                         mysqlshdk::mysql::get_replica_keyword(*instance) +
                         " SQL_THREAD FOR CHANNEL ?",
                     channel_name);
}

void stop_replication_applier(mysqlshdk::mysql::IInstance *instance,
                              const std::string &channel_name) {
  log_debug("Stopping replica sql_thread %s for %s...", channel_name.c_str(),
            instance->descr().c_str());
  instance->executef("STOP " +
                         mysqlshdk::mysql::get_replica_keyword(*instance) +
                         " SQL_THREAD FOR CHANNEL ?",
                     channel_name);
}

}  // namespace mysql
}  // namespace mysqlshdk
