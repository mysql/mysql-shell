/*
 * Copyright (c) 2017, Oracle and/or its affiliates. All rights reserved.
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

#include "mysqlshdk/libs/db/replay/replayer.h"

#include <deque>
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

class Replayer_impl {
 public:
  void connect(const mysqlshdk::db::Connection_options &data) {
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

    _trace->expected_connect_status(&_info);
    _open = true;
  }

  std::string filter_query(const std::string &sql) {
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

  std::string do_query(const std::string &sql_) {
    std::string sql = sql_;
    if (g_replay_query_hook) sql = g_replay_query_hook(sql_);

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
              "Executing query: %s\nbut replayed session %s has: %s",
              sql.c_str(), _trace->trace_path().c_str(), expected.c_str())
              .c_str());
    }

    return sql;
  }

  void close() {
    if (_open) {
      _trace->expected_close();
      _trace->expected_status();
    }
    _open = false;
  }

  bool is_open() const { return _open; }

  const mysqlshdk::db::Connection_options &get_connection_options() const {
    return _connection_options;
  }

  const char *get_info_p(const std::string &key) const {
    if (_info.find(key) == _info.end()) return nullptr;
    return _info.at(key).c_str();
  }

  const std::string &get_info(const std::string &key) const {
    return _info[key];
  }

  void set_trace(Trace *trace) { _trace.reset(trace); }

  Trace &trace() { return *_trace; }

 private:
  mysqlshdk::db::Connection_options _connection_options;
  std::unique_ptr<Trace> _trace;
  mutable std::map<std::string, std::string> _info;
  bool _open = false;
};

//-----

Replayer_mysql::Replayer_mysql(int print_traces) : _print_traces(print_traces) {
  _impl.reset(new Replayer_impl());
}

Replayer_mysql::~Replayer_mysql() {}

void Replayer_mysql::connect(const mysqlshdk::db::Connection_options &data_) {
  _impl->set_trace(new Trace(next_replay_path("mysql_trace"), _print_traces));

  mysqlshdk::db::Connection_options data(data_);

  if (!data.has_scheme()) data.set_scheme("mysql");

  _impl->connect(data);
}

std::shared_ptr<IResult> Replayer_mysql::query(const std::string &sql_, bool) {
  std::string sql = _impl->do_query(sql_);
  if (g_replay_row_hook)
    return _impl->trace().expected_result(
        std::bind(g_replay_row_hook, _impl->get_connection_options(), sql,
                  std::placeholders::_1));
  else
    return _impl->trace().expected_result({});
}

void Replayer_mysql::execute(const std::string &sql) { query(sql, true); }

void Replayer_mysql::close() { _impl->close(); }

bool Replayer_mysql::is_open() const { return _impl->is_open(); }

uint64_t Replayer_mysql::get_connection_id() const {
  std::string cid = _impl->get_info("connection_id");
  if (cid.empty()) return 0;
  return std::stoull(cid);
}

const char *Replayer_mysql::get_ssl_cipher() const {
  return _impl->get_info_p("ssl_cipher");
}

const char *Replayer_mysql::get_connection_info() {
  return _impl->get_info_p("connection_info");
}

const char *Replayer_mysql::get_server_info() {
  return _impl->get_info_p("server_info");
}

mysqlshdk::utils::Version Replayer_mysql::get_server_version() const {
  return mysqlshdk::utils::Version(_impl->get_info("server_version"));
}

const mysqlshdk::db::Connection_options &
Replayer_mysql::get_connection_options() const {
  return _impl->get_connection_options();
}

Result_mysql::Result_mysql(uint64_t affected_rows, unsigned int warning_count,
                           uint64_t last_insert_id, const char *info)
    : mysql::Result({}, affected_rows, warning_count, last_insert_id, info) {}

// ---

Replayer_mysqlx::Replayer_mysqlx(int print_traces)
    : _print_traces(print_traces) {
  _impl.reset(new Replayer_impl());
}

Replayer_mysqlx::~Replayer_mysqlx() {}

void Replayer_mysqlx::connect(const mysqlshdk::db::Connection_options &data_) {
  _impl->set_trace(new Trace(next_replay_path("mysqlx_trace"), _print_traces));

  mysqlshdk::db::Connection_options data(data_);
  // Normalization done in the real Session object must be done here too
  if (!data.has_scheme()) data.set_scheme("mysqlx");

  // All connections should use mode = VERIFY_CA if no ssl mode is specified
  // and either ssl-ca or ssl-capath are specified
  if (!data.has_value(mysqlshdk::db::kSslMode) &&
      (data.has_value(mysqlshdk::db::kSslCa) ||
       data.has_value(mysqlshdk::db::kSslCaPath))) {
    data.set(mysqlshdk::db::kSslMode, {mysqlshdk::db::kSslModeVerifyCA});
  }

  _impl->connect(data);
}

std::shared_ptr<IResult> Replayer_mysqlx::query(const std::string &sql_, bool) {
  std::string sql = _impl->do_query(sql_);
  if (g_replay_row_hook)
    return _impl->trace().expected_result_x(
        std::bind(g_replay_row_hook, _impl->get_connection_options(), sql,
                  std::placeholders::_1));
  else
    return _impl->trace().expected_result_x({});
}

std::shared_ptr<IResult> Replayer_mysqlx::execute_stmt(
    const std::string &ns, const std::string &stmt,
    const ::xcl::Arguments &args) {
  if (ns != "sql" || !args.empty()) throw std::logic_error("not implemented");
  return query(stmt, true);
}

void Replayer_mysqlx::execute(const std::string &sql) { query(sql, true); }

void Replayer_mysqlx::close() { _impl->close(); }

bool Replayer_mysqlx::is_open() const { return _impl->is_open(); }

uint64_t Replayer_mysqlx::get_connection_id() const {
  std::string cid = _impl->get_info("connection_id");
  if (cid.empty()) return 0;
  return std::stoull(cid);
}

const char *Replayer_mysqlx::get_ssl_cipher() const {
  return _impl->get_info_p("ssl_cipher");
}

const std::string &Replayer_mysqlx::get_connection_info() const {
  return _impl->get_info("connection_info");
}

mysqlshdk::utils::Version Replayer_mysqlx::get_server_version() const {
  return mysqlshdk::utils::Version(_impl->get_info("server_version"));
}

const mysqlshdk::db::Connection_options &
Replayer_mysqlx::get_connection_options() const {
  return _impl->get_connection_options();
}

Result_mysqlx::Result_mysqlx(uint64_t affected_rows, unsigned int warning_count,
                             uint64_t last_insert_id, const char *info)
    : mysqlx::Result({}) {
  _affected_rows = affected_rows;
  _last_insert_id = last_insert_id;
  if (info) _info = info;
  _fetched_warning_count = warning_count;
}

}  // namespace replay
}  // namespace db
}  // namespace mysqlshdk
