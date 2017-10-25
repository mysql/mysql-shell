/*
 * Copyright (c) 2017, Oracle and/or its affiliates. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2 of the
 * License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301  USA
 */

#include "mysqlshdk/libs/db/replay/replayer.h"

#include <fstream>
#include <memory>
#include <string>

#include "mysqlshdk/libs/db/mysql/session.h"
#include "mysqlshdk/libs/db/mysqlx/session.h"
#include "mysqlshdk/libs/db/replay/setup.h"
#include "mysqlshdk/libs/db/session.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_string.h"

namespace mysqlshdk {
namespace db {
namespace replay {

extern Query_hook g_replay_query_hook;
extern Result_row_hook g_replay_row_hook;

Replayer_mysql::Replayer_mysql() {
  _trace.reset(new Trace(next_replay_path()));
}

void Replayer_mysql::connect(const mysqlshdk::db::Connection_options& data) {
  _connection_options = data;
  mysqlshdk::db::Connection_options expected = _trace->expected_connect();

  if (data.has_port() && expected.has_port() &&
      data.get_port() != expected.get_port()) {
    std::cerr << "Connect to: " << data.as_uri() << "\n";
    std::cerr << "trace is for: " << expected.as_uri() << "\n";
    throw sequence_error(
        ("Connection happening at unexpected order expecting " +
         expected.as_uri() + " but got " + data.as_uri())
            .c_str());
  }

  _trace->expected_connect_status(&_ssl_cipher);
  _open = true;
}

std::string filter_query(const std::string& sql) {
  // This will filter out parts of a query marked with a /*(*/ and /*)*/
  // Example:
  // CREATE USER /*(*/ user5432543232@localhost /*)*/
  //    IDENTIFIED BY /*(*/ 'randompassword423424' /*)*/;
  // Becomes:
  // CREATE USER
  //    IDENTIFIED BY ;
  static const char k_begin_ignore[] = "/*(*/";
  static const char k_end_ignore[] = "/*)*/";
  std::string s = sql;
  auto begin = s.find(k_begin_ignore);
  while (begin != std::string::npos) {
    auto end = s.find(k_end_ignore, begin);
    if (end != std::string::npos)
      s = s.substr(0, begin) + s.substr(end + sizeof(k_end_ignore) - 1);
    else
      s = s.substr(0, begin);
    begin = s.find(k_begin_ignore);
  }
  return s;
}

std::shared_ptr<IResult> Replayer_mysql::query(const std::string& sql_, bool) {
  std::string sql = sql_;
  if (g_replay_query_hook)
    sql = g_replay_query_hook(sql_);

  std::string expected = _trace->expected_query();
  if (shcore::str_ibeginswith(sql, "grant ") ||
      shcore::str_ibeginswith(sql, "create user ") ||
      shcore::str_ibeginswith(sql, "drop user ") ||
      shcore::str_ibeginswith(sql, "set password ") ||
      shcore::str_ibeginswith(sql, "alter user ") ||
      shcore::str_ibeginswith(sql, "show grants ")) {
    // skip checking if statement is the same
  } else if (filter_query(expected) != filter_query(sql)) {
    std::cerr
        << shcore::str_format(
               "Executing query:\n\t%s\nbut replayed session %s has:\n\t%s",
               sql.c_str(), _trace->trace_path().c_str(), expected.c_str())
        << "\n";
    throw sequence_error(
        shcore::str_format(
            "Executing query: %s\nbut replayed session %s has: %s", sql.c_str(),
            _trace->trace_path().c_str(), expected.c_str())
            .c_str());
  }

  if (getenv("TRACE_REPLAY")) {
    static std::ofstream ofs;
    if (!ofs.good())
      ofs.open(getenv("TRACE_REPLAY"));

    ofs << _trace->trace_path() << ": " << _trace->trace_index() << ": " << sql
        << "\n"
        << std::flush;
  }

  // if (const char* stop_on_query = getenv("PAUSE_ON_TRACE")) {
  //   // path_suffix:query_index
  //   std::string path_suffix, query_index;
  //   std::tie(path_suffix, query_index) =
  //       shcore::str_partition(stop_on_query, ":");
  //   if (shcore::str_endswith(_trace->trace_path(), path_suffix) &&
  //    _trace->trace_index() == static_cast<size_t>(std::stoul(query_index))) {
  //     std::cerr << "PAUSE on query for 30s: " << sql << "\n";
  //     shcore::sleep_ms(30 * 1000);
  //   }
  // }

  if (g_replay_row_hook)
    return _trace->expected_result(std::bind(
        g_replay_row_hook, _connection_options, sql, std::placeholders::_1));
  else
    return _trace->expected_result({});
}

void Replayer_mysql::execute(const std::string& sql) {
  query(sql, true);
}

void Replayer_mysql::close() {
  _trace->expected_close();
  _trace->expected_status();
  _open = false;
}

Result_mysql::Result_mysql(uint64_t affected_rows, unsigned int warning_count,
                           uint64_t last_insert_id, const char* info)
    : mysql::Result({}, affected_rows, warning_count, last_insert_id, info) {
}

}  // namespace replay
}  // namespace db
}  // namespace mysqlshdk
