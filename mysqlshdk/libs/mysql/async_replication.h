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

#ifndef MYSQLSHDK_LIBS_MYSQL_ASYNC_REPLICATION_H_
#define MYSQLSHDK_LIBS_MYSQL_ASYNC_REPLICATION_H_

#include <map>
#include <optional>
#include <string>
#include <vector>

#include "modules/adminapi/common/common.h"
#include "mysqlshdk/libs/mysql/instance.h"
#include "mysqlshdk/libs/mysql/replication.h"

namespace mysqlshdk {
namespace mysql {

struct Replication_options {
  std::optional<uint32_t> connect_retry;
  std::optional<uint32_t> retry_count;
  std::optional<bool> auto_failover;
  std::optional<uint32_t> delay;
  std::optional<double> heartbeat_period;
  std::optional<std::string> compression_algos;
  std::optional<uint32_t> zstd_compression_level;
  std::optional<std::string> bind;
  std::optional<std::string> network_namespace;
  bool auto_position = true;
};

enum class Read_replica_status {
  ONLINE,      // Replication channel up and running.
  CONNECTING,  // Replication channel up and connecting.
  OFFLINE,     // Replication channel stopped gracefully.
  ERROR,       // Replication channel stopped due to a replication error
               // (e.g. conflicting GTID-set)
  UNREACHABLE  // Shell cannot connect to the Read-Replica
};

std::string to_string(Read_replica_status status);

void change_master(const mysqlshdk::mysql::IInstance &instance,
                   const std::string &master_host, int master_port,
                   const std::string &channel_name,
                   const Auth_options &credentials,
                   const Replication_options &repl_options);

void change_master_host_port(const mysqlshdk::mysql::IInstance &instance,
                             const std::string &master_host, int master_port,
                             const std::string &channel_name,
                             const Replication_options &repl_options);

void change_master_repl_options(const mysqlshdk::mysql::IInstance &instance,
                                const std::string &channel_name,
                                const Replication_options &repl_options);

void reset_slave(const mysqlshdk::mysql::IInstance &instance,
                 const std::string &channel_name, bool reset_credentials);

void start_replication(const mysqlshdk::mysql::IInstance &instance,
                       const std::string &channel_name);

/**
 * Stop named slave and make sure there are no open temp tables. Returns false
 * while leaving slave running if temp tables are still open after the given
 * timeout.
 *
 * Open temp tables may occur in statement based replication, which can be
 * "illegally" introduced into a RBR setup via set session. Stopping a slave
 * while a temp table is open would result in applier errors.
 */
bool stop_replication_safe(const mysqlshdk::mysql::IInstance &instance,
                           const std::string &channel_name, int timeout_sec);

void stop_replication(const mysqlshdk::mysql::IInstance &instance,
                      const std::string &channel_name);

void start_replication_receiver(const mysqlshdk::mysql::IInstance &instance,
                                const std::string &channel_name);

void stop_replication_receiver(const mysqlshdk::mysql::IInstance &instance,
                               const std::string &channel_name);

void start_replication_applier(const mysqlshdk::mysql::IInstance &instance,
                               const std::string &channel_name);

void stop_replication_applier(const mysqlshdk::mysql::IInstance &instance,
                              const std::string &channel_name);

struct Replication_credentials_options {
  std::string password;
  mysqlshdk::db::Ssl_options ssl_options;
};

void change_replication_credentials(
    const mysqlshdk::mysql::IInstance &instance, std::string_view channel,
    std::string_view user, const Replication_credentials_options &options);

Read_replica_status get_read_replica_status(
    const mysqlshdk::mysql::IInstance &read_replica_instance);

}  // namespace mysql
}  // namespace mysqlshdk

#endif  // MYSQLSHDK_LIBS_MYSQL_ASYNC_REPLICATION_H_
