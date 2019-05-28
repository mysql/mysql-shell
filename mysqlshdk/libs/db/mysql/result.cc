/*
 * Copyright (c) 2017, 2019, Oracle and/or its affiliates. All rights reserved.
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

#include "mysqlshdk/libs/db/mysql/result.h"

#include <cstdlib>
#include <string>
#include <utility>

#include "mysqlshdk/libs/db/mysql/row.h"
#include "mysqlshdk/libs/db/mysql/session.h"
#include "shellcore/interrupt_handler.h"
#include "utils/utils_general.h"
#include "utils/utils_string.h"

namespace mysqlshdk {
namespace db {
namespace mysql {
Result::Result(std::shared_ptr<mysqlshdk::db::mysql::Session_impl> owner,
               uint64_t affected_rows_, unsigned int warning_count_,
               uint64_t last_insert_id, const char *info_, bool buffered)
    : _session(owner),
      _affected_rows(affected_rows_),
      _last_insert_id(last_insert_id),
      _warning_count(warning_count_),
      _fetched_row_count(0),
      _has_resultset(false),
      m_buffered(buffered) {
  if (info_) _info.assign(info_);
  if (owner) {
    owner->prepare_fetch(this);
  }
}

// MYSQL-SERVER-CODE mysql.cc:3341
static const char *fieldtype2str(enum enum_field_types type) {
  switch (type) {
    case MYSQL_TYPE_BIT:
      return "BIT";
    case MYSQL_TYPE_BLOB:
      return "BLOB";
    case MYSQL_TYPE_DATE:
      return "DATE";
    case MYSQL_TYPE_DATETIME:
      return "DATETIME";
    case MYSQL_TYPE_NEWDECIMAL:
      return "NEWDECIMAL";
    case MYSQL_TYPE_DECIMAL:
      return "DECIMAL";
    case MYSQL_TYPE_DOUBLE:
      return "DOUBLE";
    case MYSQL_TYPE_ENUM:
      return "ENUM";
    case MYSQL_TYPE_FLOAT:
      return "FLOAT";
    case MYSQL_TYPE_GEOMETRY:
      return "GEOMETRY";
    case MYSQL_TYPE_INT24:
      return "INT24";
    case MYSQL_TYPE_JSON:
      return "JSON";
    case MYSQL_TYPE_LONG:
      return "LONG";
    case MYSQL_TYPE_LONGLONG:
      return "LONGLONG";
    case MYSQL_TYPE_LONG_BLOB:
      return "LONG_BLOB";
    case MYSQL_TYPE_MEDIUM_BLOB:
      return "MEDIUM_BLOB";
    case MYSQL_TYPE_NEWDATE:
      return "NEWDATE";
    case MYSQL_TYPE_NULL:
      return "NULL";
    case MYSQL_TYPE_SET:
      return "SET";
    case MYSQL_TYPE_SHORT:
      return "SHORT";
    case MYSQL_TYPE_STRING:
      return "STRING";
    case MYSQL_TYPE_TIME:
      return "TIME";
    case MYSQL_TYPE_TIMESTAMP:
      return "TIMESTAMP";
    case MYSQL_TYPE_TINY:
      return "TINY";
    case MYSQL_TYPE_TINY_BLOB:
      return "TINY_BLOB";
    case MYSQL_TYPE_VAR_STRING:
      return "VAR_STRING";
    case MYSQL_TYPE_YEAR:
      return "YEAR";
    default:
      return "?-unknown-?";
  }
}

// MYSQL-SERVER-CODE (slightly modified) mysql.c:3402
static std::string fieldflags2str(unsigned int f) {
  std::stringstream s;

#define ff2s_check_flag(X) \
  if (f & X##_FLAG) {      \
    s << #X " ";           \
    f &= ~X##_FLAG;        \
  }
  ff2s_check_flag(NOT_NULL);
  ff2s_check_flag(PRI_KEY);
  ff2s_check_flag(UNIQUE_KEY);
  ff2s_check_flag(MULTIPLE_KEY);
  ff2s_check_flag(BLOB);
  ff2s_check_flag(UNSIGNED);
  ff2s_check_flag(ZEROFILL);
  ff2s_check_flag(BINARY);
  ff2s_check_flag(ENUM);
  ff2s_check_flag(AUTO_INCREMENT);
  ff2s_check_flag(TIMESTAMP);
  ff2s_check_flag(SET);
  ff2s_check_flag(NO_DEFAULT_VALUE);
  ff2s_check_flag(NUM);
  ff2s_check_flag(PART_KEY);
  ff2s_check_flag(GROUP);
  ff2s_check_flag(UNIQUE);
  ff2s_check_flag(BINCMP);
  ff2s_check_flag(ON_UPDATE_NOW);
#undef ff2s_check_flag
  if (f) s << shcore::str_format(" unknowns=0x%04x", f);
  return s.str();
}

void Result::fetch_metadata() {
  int num_fields = 0;

  _metadata.clear();

  // res could be NULL on queries not returning data
  std::shared_ptr<MYSQL_RES> res = _result.lock();

  if (res) {
    num_fields = mysql_num_fields(res.get());
    MYSQL_FIELD *fields = mysql_fetch_fields(res.get());

    for (int index = 0; index < num_fields; index++) {
      _metadata.push_back(mysqlshdk::db::Column(
          fields[index].catalog, fields[index].db, fields[index].org_table,
          fields[index].table, fields[index].org_name, fields[index].name,
          fields[index].length, fields[index].decimals,
          map_data_type(fields[index].type, fields[index].flags),
          fields[index].charsetnr,
          static_cast<bool>(fields[index].flags & UNSIGNED_FLAG),
          static_cast<bool>(fields[index].flags & ZEROFILL_FLAG),
          static_cast<bool>(fields[index].flags & BINARY_FLAG),
          fieldflags2str(fields[index].flags),
          fieldtype2str(fields[index].type)));
    }
  }
}

Result::~Result() {}

const IRow *Result::fetch_one() {
  if (_pre_fetched) {
    if (!_persistent_pre_fetch) {
      if (!_pre_fetched_rows.empty()) {
        if (_fetched_row_count > 0)  // free the previously fetched row
          _pre_fetched_rows.pop_front();
        if (!_pre_fetched_rows.empty()) {
          // return the next row, but don't pop it yet otherwise it'll be freed
          const IRow *row = &_pre_fetched_rows.front();
          _fetched_row_count++;
          return row;
        }
      }
    } else {
      if (_fetched_row_count < _pre_fetched_rows.size()) {
        return &_pre_fetched_rows[_fetched_row_count++];
      }
    }
  } else {
    _row.reset();
    if (has_resultset()) {
      // Loads the first row
      std::shared_ptr<MYSQL_RES> res = _result.lock();

      if (res) {
        MYSQL_ROW mysql_row = mysql_fetch_row(res.get());
        if (mysql_row) {
          unsigned long *lengths;
          lengths = mysql_fetch_lengths(res.get());

          _row.reset(new Row(this, mysql_row, lengths));

          // Each read row increases the count
          _fetched_row_count++;
        } else {
          if (auto session = _session.lock()) {
            int code = 0;
            const char *state;
            const char *err = session->get_last_error(&code, &state);
            if (code != 0)
              throw shcore::Exception::mysql_error_with_code_and_state(
                  err, code, state);
          }
        }
      }
    }
    return _row.get();
  }

  return nullptr;
}

bool Result::next_resultset() {
  bool ret_val = false;

  if (auto s = _session.lock()) {
    ret_val = s->next_resultset();

    if (ret_val) s->prepare_fetch(this);
  }

  return ret_val;
}

void Result::rewind() {
  _fetched_row_count = 0;
  if (std::shared_ptr<MYSQL_RES> res = _result.lock())
    mysql_data_seek(res.get(), 0);
}

std::unique_ptr<Warning> Result::fetch_one_warning() {
  if (_warning_count && !_fetched_warnings) {
    _fetched_warnings = true;
    if (auto s = _session.lock()) {
      auto result =
          s->query("show warnings", sizeof("show warnings") - 1, true);
      while (auto row = result->fetch_one()) {
        std::unique_ptr<Warning> w(new Warning());
        std::string level = row->get_string(0);
        if (level == "Error") {
          w->level = Warning::Level::Error;
        } else if (level == "Warning") {
          w->level = Warning::Level::Warn;
        } else {
          assert(level == "Note");
          w->level = Warning::Level::Note;
        }
        w->code = row->get_int(1);
        w->msg = row->get_string(2);
        _warnings.push_back(std::move(w));
      }
    }
  }

  if (!_warnings.empty()) {
    auto tmp = std::move(_warnings.front());
    _warnings.pop_front();
    return tmp;
  }
  return {};
}

void Result::reset(std::shared_ptr<MYSQL_RES> res) {
  _field_names.reset();
  _has_resultset = false;
  _pre_fetched = false;
  _fetched_row_count = 0;
  _pre_fetched_rows.clear();
  _result = res;

  if (res) {
    _has_resultset = true;
    fetch_metadata();
  }

  if (auto s = _session.lock()) _gtids = s->get_last_gtids();
}

void Result::buffer() {
  shcore::Interrupt_handler intr([this]() {
    stop_pre_fetch();
    return false;
  });
  pre_fetch_rows(true);
}

bool Result::pre_fetch_rows(bool persistent) {
  auto result = _result.lock();
  if (result) {
    _persistent_pre_fetch = persistent;
    _stop_pre_fetch = false;
    if (!has_resultset()) return false;
    while (auto row = fetch_one()) {
      if (_stop_pre_fetch) return true;
      _pre_fetched_rows.push_back(mysqlshdk::db::Row_copy(*row));
    }
    _fetched_row_count = 0;

    _pre_fetched = true;
  }
  return true;
}

void Result::stop_pre_fetch() { _stop_pre_fetch = true; }

std::shared_ptr<Field_names> Result::field_names() const {
  if (!_field_names) {
    _field_names = std::make_shared<Field_names>();
    for (const auto &column : _metadata)
      _field_names->add(column.get_column_label());
  }
  return _field_names;
}

Type Result::map_data_type(int raw_type, int flags) {
  switch (raw_type) {
    case MYSQL_TYPE_NULL:
      return Type::Null;
    case MYSQL_TYPE_NEWDECIMAL:
    case MYSQL_TYPE_DECIMAL:
      return Type::Decimal;
    case MYSQL_TYPE_DATE:
    case MYSQL_TYPE_NEWDATE:
      return Type::Date;
    case MYSQL_TYPE_TIME2:
    case MYSQL_TYPE_TIME:
      return Type::Time;
    case MYSQL_TYPE_STRING:
    case MYSQL_TYPE_VARCHAR:
    case MYSQL_TYPE_VAR_STRING:
      if (flags & ENUM_FLAG)
        return Type::Enum;
      else if (flags & SET_FLAG)
        return Type::Set;
      else
        return Type::String;
    case MYSQL_TYPE_TINY_BLOB:
    case MYSQL_TYPE_MEDIUM_BLOB:
    case MYSQL_TYPE_LONG_BLOB:
    case MYSQL_TYPE_BLOB:
      return Type::Bytes;
    case MYSQL_TYPE_GEOMETRY:
      return Type::Geometry;
    case MYSQL_TYPE_JSON:
      return Type::Json;
    case MYSQL_TYPE_YEAR:
    case MYSQL_TYPE_TINY:
    case MYSQL_TYPE_SHORT:
    case MYSQL_TYPE_INT24:
    case MYSQL_TYPE_LONG:
    case MYSQL_TYPE_LONGLONG:
      return (flags & UNSIGNED_FLAG) ? Type::UInteger : Type::Integer;
    case MYSQL_TYPE_FLOAT:
      return Type::Float;
    case MYSQL_TYPE_DOUBLE:
      return Type::Double;
    case MYSQL_TYPE_DATETIME:
    case MYSQL_TYPE_TIMESTAMP:
    case MYSQL_TYPE_DATETIME2:
    case MYSQL_TYPE_TIMESTAMP2:
      // The difference between TIMESTAMP and DATETIME is entirely in terms
      // of internal representation at the server side. At the client side,
      // there is no difference.
      // TIMESTAMP is the number of seconds since epoch, so it cannot store
      // dates before 1970. DATETIME is an arbitrary date and time value,
      // so it does not have that limitation.
      return Type::DateTime;
    case MYSQL_TYPE_BIT:
      return Type::Bit;
    case MYSQL_TYPE_ENUM:
      return Type::Enum;
    case MYSQL_TYPE_SET:
      return Type::Set;
  }
  throw std::logic_error("Invalid type");
}
}  // namespace mysql
}  // namespace db
}  // namespace mysqlshdk
