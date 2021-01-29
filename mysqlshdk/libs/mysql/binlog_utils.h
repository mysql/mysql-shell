/*
 * Copyright (c) 2021, Oracle and/or its affiliates.
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

#ifndef MYSQLSHDK_LIBS_MYSQL_BINLOG_UTILS_H_
#define MYSQLSHDK_LIBS_MYSQL_BINLOG_UTILS_H_

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "mysqlshdk/libs/mysql/gtid_utils.h"
#include "mysqlshdk/libs/mysql/instance.h"

namespace mysqlshdk {
namespace mysql {

struct Binlog_event {
  uint32_t server_id;
  uint64_t pos;
  uint64_t end_log_pos;
  std::string event_type;
  std::string info;
};

/**
 * Inject an empty transaction with the given gtid at the server.
 */
void inject_gtid(const mysqlshdk::mysql::IInstance &server, const Gtid &gtid);

/**
 * Inject empty transactions with each of the gtids in the given set.
 */
size_t inject_gtid_set(const mysqlshdk::mysql::IInstance &server,
                       const Gtid_set &gtid_set);

/**
 * Returns list of binary logs at the server.
 */
std::vector<std::string> list_binlogs(
    const mysqlshdk::mysql::IInstance &server);

/**
 * Fetch events from binlog and call the given function for each until false is
 * returned.
 */
void list_binlog_events(
    const mysqlshdk::mysql::IInstance &server, const std::string &log_name,
    uint64_t start_position, uint64_t offset, uint64_t limit,
    const std::function<bool(const Gtid &gtid, const Binlog_event &)> &fn);

void list_binlog_transactions(
    const mysqlshdk::mysql::IInstance &server, const std::string &log_name,
    uint64_t start_position, uint64_t offset, uint64_t limit,
    const std::function<bool(const Gtid &gtid, const Binlog_event &)> &fn);

}  // namespace mysql
}  // namespace mysqlshdk

#endif  // MYSQLSHDK_LIBS_MYSQL_BINLOG_UTILS_H_
