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

#include "mysqlshdk/libs/mysql/binlog_utils.h"
#include <vector>
#include "mysqlshdk/libs/utils/utils_general.h"

namespace mysqlshdk {
namespace mysql {

void inject_gtid(const mysqlshdk::mysql::IInstance &server, const Gtid &gtid) {
  shcore::on_leave_scope guard(
      [&]() { server.executef("SET gtid_next = AUTOMATIC"); });

  server.executef("SET gtid_next = ?", gtid);
  server.execute("START TRANSACTION");
  server.execute("COMMIT");
}

size_t inject_gtid_set(const mysqlshdk::mysql::IInstance &server,
                       const Gtid_set &gtid_set) {
  shcore::on_leave_scope guard(
      [&]() { server.executef("SET gtid_next = AUTOMATIC"); });

  size_t count = 0;

  gtid_set.enumerate([&server, &count](const Gtid &gtid) {
    server.executef("SET gtid_next = ?", gtid);
    server.execute("START TRANSACTION");
    server.execute("COMMIT");
    ++count;
  });

  return count;
}

[[maybe_unused]] std::vector<std::string> list_binlogs(
    const mysqlshdk::mysql::IInstance &server) {
  std::vector<std::string> result;

  auto res = server.query("SHOW BINARY LOGS");
  while (auto row = res->fetch_one()) {
    result.push_back(row->get_string(0));
  }

  return result;
}

[[maybe_unused]] void list_binlog_events(
    const mysqlshdk::mysql::IInstance &server, const std::string &log_name,
    uint64_t start_position, uint64_t offset, uint64_t limit,
    const std::function<bool(const Gtid &, const Binlog_event &)> &fn) {
  Gtid last_gtid;
  const std::string prefix = "SET @@SESSION.GTID_NEXT=";

  auto res = server.queryf("SHOW BINLOG EVENTS IN ? FROM ? LIMIT ?, ?",
                           log_name, start_position, offset, limit);

  while (auto row = res->fetch_one_named()) {
    Binlog_event event;

    event.server_id = row.get_uint("Server_id");
    event.pos = row.get_uint("Pos");
    event.end_log_pos = row.get_uint("End_log_pos");
    event.event_type = row.get_string("Event_type");
    event.info = row.get_string("Info");

    if (event.event_type == "Gtid") {
      if (event.info.compare(0, prefix.size(), prefix) == 0) {
        auto p = event.info.find('\'');
        last_gtid = event.info.substr(p + 1, event.info.rfind('\'') - p - 1);
      }
    } else {
      fn(last_gtid, event);
    }
  }
}

}  // namespace mysql
}  // namespace mysqlshdk
