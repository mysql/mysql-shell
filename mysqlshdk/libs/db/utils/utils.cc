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

#include "mysqlshdk/libs/db/utils/utils.h"

#include <vector>

#include "mysqlshdk/include/shellcore/shell_init.h"
#include "mysqlshdk/libs/db/mysql/session.h"
#include "mysqlshdk/libs/db/mysqlx/session.h"
#include "mysqlshdk/libs/db/row.h"

namespace mysqlshdk {
namespace db {

std::string to_string(const IRow &row) {
  std::string text;
  if (row.num_fields() > 0) {
    text.append(row.get_as_string(0));
    for (uint32_t i = 1; i < row.num_fields(); ++i) {
      text.append("\t");
      text.append(row.get_as_string(i));
    }
  }
  return text;
}

void print(std::ostream &s, const IRow &row) {
  if (row.num_fields() > 0) {
    s << row.get_as_string(0);
    for (uint32_t i = 1; i < row.num_fields(); ++i) {
      s << "\t";
      s << row.get_as_string(i);
    }
  }
}

void dump(std::ostream &s, IResult *result) {
  auto &columns = result->get_metadata();

  if (columns.size() > 0) {
    s << columns[0].get_column_label();
    for (size_t i = 1; i < columns.size(); ++i) {
      s << "\t";
      s << columns[i].get_column_label();
    }
    s << "\n";
  }
  while (const auto row = result->fetch_one()) {
    print(s, *row);
    s << "\n";
  }
}

void feed_field(shcore::sqlstring *sql, const IRow &row, uint32_t field) {
  if (row.is_null(field)) {
    *sql << nullptr;
  } else {
    switch (row.get_type(field)) {
      case Type::Null:
        *sql << nullptr;
        break;
      case Type::Decimal:
      case Type::Bytes:
      case Type::Geometry:
      case Type::Json:
      case Type::Date:
      case Type::Time:
      case Type::DateTime:
      case Type::Enum:
      case Type::Vector:
      case Type::Set:
      case Type::String:
        *sql << row.get_string(field);
        break;
      case Type::Integer:
        *sql << row.get_int(field);
        break;
      case Type::UInteger:
        *sql << row.get_uint(field);
        break;
      case Type::Float:
        *sql << row.get_float(field);
        break;
      case Type::Double:
        *sql << row.get_double(field);
        break;
      case Type::Bit:
        assert(0);
        throw std::logic_error("not implemented");
    }
  }
}

void kill_query(const std::weak_ptr<ISession> &weak_session) {
  const auto report_error = [](const char *msg) {
    log_warning("Error canceling SQL query: %s", msg);
  };

  if (const auto session = weak_session.lock()) {
    try {
      mysqlsh::Mysql_thread mysql_thread;
      const auto &co = session->get_connection_options();
      std::shared_ptr<ISession> kill_session;

      switch (co.get_session_type()) {
        case mysqlsh::SessionType::X:
          kill_session = mysqlx::Session::create();
          break;

        case mysqlsh::SessionType::Classic:
          kill_session = mysql::Session::create();
          break;

        default:
          report_error("Unsupported session type.");
          return;
      }

      kill_session->connect(co);
      kill_session->execute("KILL QUERY " +
                            std::to_string(session->get_connection_id()));
      kill_session->close();
    } catch (const std::exception &e) {
      report_error(e.what());
    }
  } else {
    report_error("Session does not exist any more.");
  }
}

}  // namespace db
}  // namespace mysqlshdk
