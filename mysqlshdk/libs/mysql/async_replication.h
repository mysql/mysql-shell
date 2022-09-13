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

#ifndef MYSQLSHDK_LIBS_MYSQL_ASYNC_REPLICATION_H_
#define MYSQLSHDK_LIBS_MYSQL_ASYNC_REPLICATION_H_

#include <map>
#include <string>
#include <vector>

#include "modules/adminapi/common/common.h"
#include "mysqlshdk/libs/mysql/instance.h"
#include "mysqlshdk/libs/mysql/replication.h"
#include "mysqlshdk/libs/utils/nullable.h"

namespace mysqlshdk {
namespace mysql {

void change_master(mysqlshdk::mysql::IInstance *instance,
                   const std::string &master_host, int master_port,
                   const std::string &channel_name,
                   const Auth_options &credentials,
                   const mysqlsh::dba::Cluster_ssl_mode &ssl_mode,
                   const mysqlshdk::utils::nullable<int> master_connect_retry,
                   const mysqlshdk::utils::nullable<int> master_retry_count,
                   const mysqlshdk::utils::nullable<bool> auto_failover,
                   const mysqlshdk::utils::nullable<int> master_delay);

void change_master_host_port(mysqlshdk::mysql::IInstance *instance,
                             const std::string &master_host, int master_port,
                             const std::string &channel_name);

void reset_slave(mysqlshdk::mysql::IInstance *instance,
                 const std::string &channel_name, bool reset_credentials);

void start_replication(mysqlshdk::mysql::IInstance *instance,
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
bool stop_replication_safe(mysqlshdk::mysql::IInstance *instance,
                           const std::string &channel_name, int timeout_sec);

void stop_replication(mysqlshdk::mysql::IInstance *instance,
                      const std::string &channel_name);

void start_replication_receiver(mysqlshdk::mysql::IInstance *instance,
                                const std::string &channel_name);

void stop_replication_receiver(mysqlshdk::mysql::IInstance *instance,
                               const std::string &channel_name);

void start_replication_applier(mysqlshdk::mysql::IInstance *instance,
                               const std::string &channel_name);

void stop_replication_applier(mysqlshdk::mysql::IInstance *instance,
                              const std::string &channel_name);

/**
 * Change the replication credentials in a specific replication channel
 *
 * @param instance Target instance to execute the changes (must be a source in a
 * replication topology)
 * @param rpl_user New username to be used in the channel
 * @param rpl_pwd New password to be used in the channel
 * @param repl_channel Name of the replication channel to be changed
 */
void change_replication_credentials(const mysqlshdk::mysql::IInstance &instance,
                                    const std::string &rpl_user,
                                    const std::string &rpl_pwd,
                                    const std::string &repl_channel);

}  // namespace mysql
}  // namespace mysqlshdk

#endif  // MYSQLSHDK_LIBS_MYSQL_ASYNC_REPLICATION_H_
