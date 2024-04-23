/*
 * Copyright (c) 2017, 2024, Oracle and/or its affiliates.
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

#include "mysqlshdk/libs/db/replay/recorder.h"

#include <fstream>
#include <map>
#include <memory>
#include <string>
#include <string_view>

#include "mysqlshdk/libs/db/mysql/session.h"
#include "mysqlshdk/libs/db/mysqlx/session.h"
#include "mysqlshdk/libs/db/replay/mysqlx.h"
#include "mysqlshdk/libs/db/replay/setup.h"
#include "mysqlshdk/libs/db/session.h"
#include "mysqlshdk/libs/utils/utils_stacktrace.h"
#include "mysqlshdk/libs/utils/utils_string.h"

namespace mysqlshdk {
namespace db {
namespace replay {

std::function<void(std::shared_ptr<mysqlshdk::db::ISession>)>
    on_recorder_connect_hook;

std::function<void(std::shared_ptr<mysqlshdk::db::ISession>)>
    on_recorder_close_hook;

std::function<std::string(std::string_view sql)> on_recorder_query_replace_hook;

std::function<std::string(const std::string &value)>
    on_recorder_result_value_replace_hook;

Recorder_mysql::Recorder_mysql() = default;

void Recorder_mysql::do_connect(const mysqlshdk::db::Connection_options &data) {
  _trace = Trace_writer::create(new_recording_path("mysql_trace"));

  try {
    if (data.has_port()) _port = data.get_port();
    _trace->serialize_connect(data, "classic");
    super::do_connect(data);
    std::map<std::string, std::string> info;
    if (const char *s = get_ssl_cipher()) info["ssl_cipher"] = s;
    if (const char *s = get_connection_info()) info["connection_info"] = s;
    if (const char *s = get_server_info()) info["server_info"] = s;
    info["server_version"] = mysql::Session::get_server_version().get_full();
    info["connection_id"] = std::to_string(get_connection_id());
    info["protocol_info"] = std::to_string(get_protocol_info());
    _trace->serialize_connect_ok(info);

    if (on_recorder_connect_hook) on_recorder_connect_hook(shared_from_this());
  } catch (const db::Error &e) {
    _trace->serialize_error(e);
    throw;
  } catch (const std::runtime_error &e) {
    _trace->serialize_error(e);
    throw;
  }
}

std::shared_ptr<IResult> Recorder_mysql::querys(
    const char *sql, size_t length, bool,
    const std::vector<Query_attribute> &query_attributes) {
  // todo - add synchronization points for error.log on every query
  // assuming that error log contents change when a query is executed
  // if (set_log_save_point) set_log_save_point(_port);
  if (on_recorder_query_replace_hook) {
    _trace->serialize_query(on_recorder_query_replace_hook({sql, length}));
  } else {
    _trace->serialize_query(std::string(sql, length));
  }

  try {
    // Always buffer to make row serialization easier
    std::shared_ptr<IResult> result(
        super::querys(sql, length, true, query_attributes));
    _trace->serialize_result(result, on_recorder_result_value_replace_hook);
    std::dynamic_pointer_cast<mysql::Result>(result)->rewind();
    return result;
  } catch (const db::Error &e) {
    _trace->serialize_error(e);
    throw;
  }
}

std::shared_ptr<IResult> Recorder_mysql::query_udf(std::string_view sql, bool) {
  // todo - add synchronization points for error.log on every query
  // assuming that error log contents change when a query is executed
  // if (set_log_save_point) set_log_save_point(_port);
  if (on_recorder_query_replace_hook) {
    _trace->serialize_query(on_recorder_query_replace_hook(sql));
  } else {
    _trace->serialize_query(std::string(sql));
  }

  try {
    // Always buffer to make row serialization easier
    std::shared_ptr<IResult> result(super::query_udf(sql, true));
    _trace->serialize_result(result, on_recorder_result_value_replace_hook);
    std::dynamic_pointer_cast<mysql::Result>(result)->rewind();
    return result;
  } catch (const db::Error &e) {
    _trace->serialize_error(e);
    throw;
  }
}

void Recorder_mysql::executes(const char *sql, size_t length) {
  querys(sql, length, true);
}

void Recorder_mysql::do_close() {
  try {
    if (_trace && !_closed) {
      _closed = true;
      _trace->serialize_close();
      super::do_close();
      _trace->serialize_ok();
      _trace.reset();

      if (on_recorder_close_hook) on_recorder_close_hook(shared_from_this());
    } else {
      super::do_close();
    }
  } catch (db::Error &e) {
    if (_trace) {
      _trace->serialize_error(e);
      _trace.reset();
      if (on_recorder_close_hook) on_recorder_close_hook(shared_from_this());
    }
    throw;
  }
}

// ---

Recorder_mysqlx::Recorder_mysqlx() {}

void Recorder_mysqlx::do_connect(
    const mysqlshdk::db::Connection_options &data) {
  _trace = Trace_writer::create(new_recording_path("mysqlx_trace"));
  try {
    if (data.has_port()) _port = data.get_port();
    _trace->serialize_connect(data, "x");
    super::do_connect(data);
    std::map<std::string, std::string> info;
    if (const char *s = get_ssl_cipher()) info["ssl_cipher"] = s;
    info["connection_info"] = get_connection_info();
    info["connection_id"] = std::to_string(get_connection_id());
    info["server_version"] = get_server_version().get_full();
    _trace->serialize_connect_ok(info);

    if (on_recorder_connect_hook) on_recorder_connect_hook(shared_from_this());
  } catch (const db::Error &e) {
    _trace->serialize_error(e);
    throw;
  } catch (const std::runtime_error &e) {
    _trace->serialize_error(e);
    throw;
  }
}

std::shared_ptr<IResult> Recorder_mysqlx::querys(
    const char *sql, size_t length, bool,
    const std::vector<Query_attribute> &query_attributes) {
  try {
    // todo - add synchronization points for error.log on every query
    // assuming that error log contents change when a query is executed
    // if (set_log_save_point) set_log_save_point(_port);

    _trace->serialize_query(
        on_recorder_query_replace_hook
            ? on_recorder_query_replace_hook(std::string_view{sql, length})
            : std::string{sql, length});
    // Always buffer to make row serialization easier
    std::shared_ptr<IResult> result(
        super::querys(sql, length, true, query_attributes));
    _trace->serialize_result(result, nullptr);
    std::dynamic_pointer_cast<mysqlx::Result>(result)->rewind();
    return result;
  } catch (db::Error &e) {
    _trace->serialize_error(e);
    throw;
  }
}

std::shared_ptr<IResult> Recorder_mysqlx::execute_stmt(
    const std::string &ns, std::string_view stmt,
    const ::xcl::Argument_array &args) {
  if (ns != "sql")
    throw std::logic_error("recording for namespace " + ns +
                           " not implemented");
  if (args.empty()) {
    return querys(stmt.data(), stmt.length(), true);
  } else {
    const auto sql = replay::query(stmt, args);
    return querys(sql.data(), sql.length(), true);
  }
}

void Recorder_mysqlx::executes(const char *sql, size_t length) {
  querys(sql, length, true);
}

void Recorder_mysqlx::do_close() {
  try {
    if (_trace && !_closed) {
      _closed = true;
      _trace->serialize_close();
      super::do_close();
      _trace->serialize_ok();
      _trace.reset();

      if (on_recorder_close_hook) on_recorder_close_hook(shared_from_this());
    } else {
      super::do_close();
    }
  } catch (db::Error &e) {
    if (_trace) {
      _trace->serialize_error(e);
      _trace.reset();
      if (on_recorder_close_hook) on_recorder_close_hook(shared_from_this());
    }
    throw;
  }
}

}  // namespace replay
}  // namespace db
}  // namespace mysqlshdk
