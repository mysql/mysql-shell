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

#include "mysqlshdk/libs/mysql/async_replication.h"

#include <mysqld_error.h>
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
                   const mysqlsh::dba::Cluster_ssl_mode &ssl_mode,
                   const mysqlshdk::utils::nullable<int> master_connect_retry,
                   const mysqlshdk::utils::nullable<int> master_retry_count,
                   const mysqlshdk::utils::nullable<bool> auto_failover) {
  log_info(
      "Setting up async source for channel '%s' of %s to %s:%i (user "
      "%s)",
      channel_name.c_str(), instance->descr().c_str(), master_host.c_str(),
      master_port,
      (credentials.user.empty() ? "unchanged" : "'" + credentials.user + "'")
          .c_str());

  try {
    std::string source_term_cmd =
        mysqlshdk::mysql::get_replication_source_keyword(
            instance->get_version(), true);
    std::string source_term = mysqlshdk::mysql::get_replication_source_keyword(
        instance->get_version());

    std::string options;
    if (ssl_mode != mysqlsh::dba::Cluster_ssl_mode::NONE) {
      bool enable_ssl = false;

      if (ssl_mode == mysqlsh::dba::Cluster_ssl_mode::REQUIRED) {
        enable_ssl = true;
      } else if (ssl_mode == mysqlsh::dba::Cluster_ssl_mode::VERIFY_CA ||
                 ssl_mode == mysqlsh::dba::Cluster_ssl_mode::VERIFY_IDENTITY) {
        throw shcore::Exception::argument_error("Unsupported Cluster SSL-mode");
      }

      options.append(", " + source_term +
                     "_SSL=" + std::to_string(enable_ssl ? 1 : 0));
    }

    if (!master_retry_count.is_null()) {
      options.append(", " + source_term +
                     "_RETRY_COUNT=" + std::to_string(*master_retry_count));
    }

    if (!master_connect_retry.is_null()) {
      options.append(", " + source_term +
                     "_CONNECT_RETRY=" + std::to_string(*master_connect_retry));
    }

    if (!auto_failover.is_null()) {
      options.append(", SOURCE_CONNECTION_AUTO_FAILOVER=" +
                     std::to_string(*auto_failover));
    }

    instance->executef("CHANGE " + source_term_cmd +
                           " TO /*!80011 get_master_public_key=1, */"
                           " " +
                           source_term + "_HOST=/*(*/ ? /*)*/, " + source_term +
                           "_PORT=/*(*/ ? /*)*/,"
                           " " +
                           source_term + "_USER=?, " + source_term +
                           "_PASSWORD=/*((*/ ? /*))*/" + options + ", " +
                           source_term +
                           "_AUTO_POSITION=1"
                           " FOR CHANNEL ?",
                       master_host, master_port, credentials.user,
                       *credentials.password, channel_name);
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

  std::string source_term_cmd =
      mysqlshdk::mysql::get_replication_source_keyword(instance->get_version(),
                                                       true);
  std::string source_term =
      mysqlshdk::mysql::get_replication_source_keyword(instance->get_version());

  try {
    instance->executef("CHANGE " + source_term_cmd + " TO " + source_term +
                           "_HOST=/*(*/ ? /*)*/, " + source_term +
                           "_PORT=/*(*/ ? /*)*/ "
                           "FOR CHANNEL ?",
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
  std::string replica_term =
      mysqlshdk::mysql::get_replica_keyword(instance->get_version());

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
  std::string replica_term =
      mysqlshdk::mysql::get_replica_keyword(instance->get_version());

  instance->executef("START " + replica_term + " FOR CHANNEL ?", channel_name);
}

void stop_replication(mysqlshdk::mysql::IInstance *instance,
                      const std::string &channel_name) {
  log_debug("Stopping replica channel %s for %s...", channel_name.c_str(),
            instance->descr().c_str());
  std::string replica_term =
      mysqlshdk::mysql::get_replica_keyword(instance->get_version());

  instance->executef("STOP " + replica_term + " FOR CHANNEL ?", channel_name);
}

bool stop_replication_safe(mysqlshdk::mysql::IInstance *instance,
                           const std::string &channel_name, int timeout_sec) {
  log_debug("Stopping replica channel %s for %s...", channel_name.c_str(),
            instance->descr().c_str());
  std::string replica_term =
      mysqlshdk::mysql::get_replica_keyword(instance->get_version());

  try {
    while (timeout_sec-- >= 0) {
      instance->executef("STOP " + replica_term + " FOR CHANNEL ?",
                         channel_name);

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
  } catch (const shcore::Exception &e) {
    if (e.code() == ER_SLAVE_CHANNEL_DOES_NOT_EXIST) {
      log_info("Trying to stop non-existing channel '%s', ignoring...",
               channel_name.c_str());
      return true;
    }
    throw;
  }

  return false;
}

void start_replication_receiver(mysqlshdk::mysql::IInstance *instance,
                                const std::string &channel_name) {
  log_debug("Starting replica io_thread %s for %s...", channel_name.c_str(),
            instance->descr().c_str());
  instance->executef(
      "START " +
          mysqlshdk::mysql::get_replica_keyword(instance->get_version()) +
          " IO_THREAD FOR CHANNEL ?",
      channel_name);
}

void stop_replication_receiver(mysqlshdk::mysql::IInstance *instance,
                               const std::string &channel_name) {
  log_debug("Stopping replica io_thread %s for %s...", channel_name.c_str(),
            instance->descr().c_str());
  instance->executef(
      "STOP " + mysqlshdk::mysql::get_replica_keyword(instance->get_version()) +
          " IO_THREAD FOR CHANNEL ?",
      channel_name);
}

void start_replication_applier(mysqlshdk::mysql::IInstance *instance,
                               const std::string &channel_name) {
  log_debug("Starting replica sql_thread %s for %s...", channel_name.c_str(),
            instance->descr().c_str());
  instance->executef(
      "START " +
          mysqlshdk::mysql::get_replica_keyword(instance->get_version()) +
          " SQL_THREAD FOR CHANNEL ?",
      channel_name);
}

void stop_replication_applier(mysqlshdk::mysql::IInstance *instance,
                              const std::string &channel_name) {
  log_debug("Stopping replica sql_thread %s for %s...", channel_name.c_str(),
            instance->descr().c_str());
  instance->executef(
      "STOP " + mysqlshdk::mysql::get_replica_keyword(instance->get_version()) +
          " SQL_THREAD FOR CHANNEL ?",
      channel_name);
}

void change_replication_credentials(const mysqlshdk::mysql::IInstance &instance,
                                    const std::string &rpl_user,
                                    const std::string &rpl_pwd,
                                    const std::string &repl_channel) {
  std::string source_term_cmd =
      mysqlshdk::mysql::get_replication_source_keyword(instance.get_version(),
                                                       true);

  std::string source_term =
      mysqlshdk::mysql::get_replication_source_keyword(instance.get_version());

  std::string change_master_stmt_fmt = "CHANGE " + source_term_cmd + " TO " +
                                       source_term + "_USER = /*(*/ ? /*)*/, " +
                                       source_term +
                                       "_PASSWORD = /*((*/ ? /*))*/ "
                                       "FOR CHANNEL ?";
  shcore::sqlstring change_master_stmt =
      shcore::sqlstring(change_master_stmt_fmt.c_str(), 0);
  change_master_stmt << rpl_user;
  change_master_stmt << rpl_pwd;
  change_master_stmt << repl_channel;
  change_master_stmt.done();

  try {
    instance.execute(change_master_stmt);
  } catch (const std::exception &err) {
    throw std::runtime_error{"Cannot set replication user to '" + rpl_user +
                             "'. Error executing CHANGE " + source_term +
                             " statement: " + err.what()};
  }
}

}  // namespace mysql
}  // namespace mysqlshdk
