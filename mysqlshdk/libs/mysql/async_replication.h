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

#ifndef MYSQLSHDK_LIBS_MYSQL_ASYNC_REPLICATION_H_
#define MYSQLSHDK_LIBS_MYSQL_ASYNC_REPLICATION_H_

#include <map>
#include <optional>
#include <string>
#include <vector>

#include "modules/adminapi/common/common.h"
#include "mysqlshdk/libs/mysql/instance.h"
#include "mysqlshdk/libs/mysql/replication.h"
#include "mysqlshdk/libs/utils/nullable.h"

namespace mysqlshdk {
namespace mysql {

void change_master(
    const mysqlshdk::mysql::IInstance &instance, const std::string &master_host,
    int master_port, const std::string &channel_name,
    const Auth_options &credentials, std::optional<int> master_connect_retry,
    std::optional<int> master_retry_count, std::optional<bool> auto_failover,
    std::optional<int> master_delay, bool source_auto_position = true);

void change_master_host_port(const mysqlshdk::mysql::IInstance &instance,
                             const std::string &master_host, int master_port,
                             const std::string &channel_name);

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

}  // namespace mysql
}  // namespace mysqlshdk

#endif  // MYSQLSHDK_LIBS_MYSQL_ASYNC_REPLICATION_H_
