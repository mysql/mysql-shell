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

#include "mysqlshdk/libs/db/replay/recorder.h"

#include <fstream>
#include <memory>
#include <string>

#include "mysqlshdk/libs/db/mysql/session.h"
#include "mysqlshdk/libs/db/mysqlx/session.h"
#include "mysqlshdk/libs/db/replay/setup.h"
#include "mysqlshdk/libs/db/session.h"

namespace mysqlshdk {
namespace db {
namespace replay {

Recorder_mysql::Recorder_mysql() {
  _trace.reset(Trace_writer::create(new_recording_path()));
}

void Recorder_mysql::connect(const mysqlshdk::db::Connection_options& data) {
  try {
    _port = data.get_port();
    _trace->serialize_connect(data);
    super::connect(data);
    _trace->serialize_connect_ok(get_ssl_cipher());
  } catch (shcore::database_error& e) {
    _trace->serialize_error(e);
    throw;
  } catch (db::Error& e) {
    _trace->serialize_error(e);
    throw;
  }
}

std::shared_ptr<IResult> Recorder_mysql::query(const std::string& sql, bool) {
  try {
    if (getenv("TRACE_RECORD")) {
      static std::ofstream ofs;
      if (!ofs.good())
        ofs.open(getenv("TRACE_RECORD"));

      ofs << _trace->trace_path() << ": " << _trace->trace_index() << ": "
          << sql << "\n"
          << std::flush;
    }

    // todo - add synchronization points for error.log on every query
    // assuming that error log contents change when a query is executed
    // if (set_log_save_point) set_log_save_point(_port);

    _trace->serialize_query(sql);
    // Always buffer to make row serialization easier
    std::shared_ptr<IResult> result(super::query(sql, true));
    _trace->serialize_result(result);
    std::dynamic_pointer_cast<mysql::Result>(result)->rewind();
    return result;
  } catch (shcore::database_error& e) {
    _trace->serialize_error(e);
    throw;
  } catch (db::Error& e) {
    _trace->serialize_error(e);
    throw;
  }
}

void Recorder_mysql::execute(const std::string& sql) {
  query(sql, true);
}

void Recorder_mysql::close() {
  try {
    _trace->serialize_close();
    super::close();
    _trace->serialize_ok();
    _trace.reset();
  } catch (shcore::database_error& e) {
    _trace->serialize_error(e);
    throw;
  } catch (db::Error& e) {
    _trace->serialize_error(e);
    throw;
  }
}

}  // namespace replay
}  // namespace db
}  // namespace mysqlshdk
