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

#include <string>
#include <utility>
#include "mysqlshdk/libs/db/mysqlx/result.h"

#include "mysqlshdk/libs/db/charset.h"
#include "mysqlshdk/libs/db/mysqlx/row.h"
#include "mysqlshdk/libs/db/session.h"
#include "utils/utils_general.h"

namespace mysqlshdk {
namespace db {
namespace mysqlx {
Result::Result(std::unique_ptr<xcl::XQuery_result> result)
    : _result(std::move(result)), _row(this), _fetched_row_count(0) {
}

void Result::fetch_metadata() {
  _metadata.clear();

  // res could be NULL on queries not returning data
  const ::xcl::XQuery_result::Metadata &meta = _result->get_metadata();

  for (const ::xcl::Column_metadata &column : meta) {
    Type type = Type::Null;
    bool is_unsigned = false;
    bool is_padded = false;
    bool is_zerofill = false;
    bool is_binary = false;
    bool is_numeric = false;
    switch (column.type) {
      case ::xcl::Column_type::UINT:
        is_zerofill = (column.flags & 0x001) != 0;
        is_unsigned = true;
        is_numeric = true;
        type = Type::UInteger;
        break;
      case ::xcl::Column_type::SINT:
        is_zerofill = (column.flags & 0x001) != 0;
        type = Type::Integer;
        is_numeric = true;
        break;
      case ::xcl::Column_type::BIT:
        type = Type::Bit;
        is_unsigned = true;
        is_numeric = true;
        break;
      case ::xcl::Column_type::DOUBLE:
        is_unsigned = (column.flags & 0x001) != 0;
        type = Type::Double;
        is_numeric = true;
        break;
      case ::xcl::Column_type::FLOAT:
        is_unsigned = (column.flags & 0x001) != 0;
        type = Type::Float;
        is_numeric = true;
        break;
      case ::xcl::Column_type::DECIMAL:
        is_unsigned = (column.flags & 0x001) != 0;
        type = Type::Decimal;
        is_numeric = true;
        break;
      case ::xcl::Column_type::BYTES:
        is_padded = column.flags & 0x001;

        switch (column.content_type & 0x0003) {
          case 1:
            type = Type::Geometry;
            break;
          case 2:
            type = Type::Json;
            break;
          case 3:
            type = Type::String;  // XML
            break;
          default:
            if (column.collation == 0) {
              type = Type::Bytes;
            } else {
              if (mysqlshdk::db::charset::charset_name_from_collation_id(
                      column.collation) == "binary") {
                is_binary = true;
                type = Type::Bytes;
              } else {
                type = Type::String;
              }
            }
            break;
        }
        break;
      case ::xcl::Column_type::TIME:
        is_binary = true;
        type = Type::Time;
        break;
      case ::xcl::Column_type::DATETIME:
        is_binary = true;
        if (column.flags & 0x001)
          type = Type::DateTime;  // TIMESTAMP
        else if (column.length == 10)
          type = Type::Date;
        else
          type = Type::DateTime;
        break;
      case ::xcl::Column_type::SET:
        type = Type::Set;
        break;
      case ::xcl::Column_type::ENUM:
        type = Type::Enum;
        break;
    }

    // Note: padded means RIGHTPAD with \0 for CHAR columns
    // It is internal to the client lib
    (void)is_padded;

    _metadata.push_back(mysqlshdk::db::Column(
        column.schema,
        column.original_table.empty() ? column.table : column.original_table,
        column.table, column.original_name, column.name, column.length,
        column.fractional_digits, type, column.collation,
        is_numeric ? is_unsigned : false, is_zerofill, is_binary));
  }
}

std::string Result::get_info() const {
  std::string info;
  if (_result)
    _result->try_get_info_message(&info);
  return info;
}

int64_t Result::get_auto_increment_value() const {
  uint64_t i = 0;
  if (_result) {
    _result->try_get_last_insert_id(&i);
  }
  return i;
}

uint64_t Result::get_affected_row_count() const {
  uint64_t i = 0;
  if (_result) {
    _result->try_get_affected_rows(&i);
  }
  return i;
}

uint64_t Result::get_warning_count() const {
  if (_result)
    return _result->get_warnings().size();
  return 0;
}

Result::~Result() {
  // flush all
  if (_result) {
    try {
      while (next_resultset()) {
      }
    } catch (...) {
    }
  }
  _row.reset(nullptr);
}

const IRow *Result::fetch_one() {
  if (_pre_fetched) {
    if (_persistent_pre_fetch && !_pre_fetched_rows.empty()) {
      if (_fetched_row_count > 0)  // free the previously fetched row
        _pre_fetched_rows.pop_front();
      if (!_pre_fetched_rows.empty()) {
        // return the next row, but don't pop it yet otherwise it'll be freed
        const IRow *row = &_pre_fetched_rows.front();
        _fetched_row_count++;
        return row;
      }
    } else {
      if (_fetched_row_count < _pre_fetched_rows.size()) {
        return &_pre_fetched_rows[_fetched_row_count++];
      }
    }
  } else {
    // Loads the first row
    if (_result) {
      xcl::XError error;
      const ::xcl::XRow *row = _result->get_next_row(&error);
      if (error)
        throw mysqlshdk::db::Error(error.what(), error.error());
      if (row) {
        _row.reset(row);
        _fetched_row_count++;
        return &_row;
      }
    }
  }
  return nullptr;
}

void Result::rewind() {
  _fetched_row_count = 0;
}

bool Result::pre_fetch_rows(bool persistent) {
  if (_result) {
    _persistent_pre_fetch = persistent;
    _stop_pre_fetch = false;
    if (!_result->has_resultset())
      return false;
    Row wrapper(this);
    xcl::XError error;
    while (const ::xcl::XRow *row = _result->get_next_row(&error)) {
      if (_stop_pre_fetch)
        return true;
      wrapper.reset(row);
      _pre_fetched_rows.push_back(Row_copy(wrapper));
    }
    if (error) {
      std::stringstream msg;
      msg << "Error " << error.error() << " (" << error.what() << ")";
      msg << " while fetching row " << _pre_fetched_rows.size()+1 << ".";
      throw mysqlshdk::db::Error(msg.str().c_str(), error.error());
    }
    _pre_fetched = true;
  }
  return true;
}

void Result::stop_pre_fetch() {
  _stop_pre_fetch = true;
}

bool Result::has_resultset() {
  return _result->has_resultset();
}

bool Result::next_resultset() {
  bool ret_val = false;

  _pre_fetched_rows.clear();
  _pre_fetched = false;

  xcl::XError error;
  ret_val = _result->next_resultset(&error);
  if (error)
    throw mysqlshdk::db::Error(error.what(), error.error());
  _fetched_row_count = 0;
  return ret_val;
}

std::unique_ptr<Warning> Result::fetch_one_warning() {
  const auto &warnings = _result->get_warnings();
  if (_fetched_warning_count < warnings.size()) {
    std::unique_ptr<Warning> w(new Warning());
    const Mysqlx::Notice::Warning &warning = warnings[_fetched_warning_count];
    switch (warning.level()) {
      case Mysqlx::Notice::Warning::NOTE:
        w->level = Warning::Level::Note;
        break;
      case Mysqlx::Notice::Warning::WARNING:
        w->level = Warning::Level::Warn;
        break;
      case Mysqlx::Notice::Warning::ERROR:
        w->level = Warning::Level::Error;
        break;
    }
    w->msg = warning.msg();
    w->code = warning.code();
    _fetched_warning_count++;
    return w;
  }
  return {};
}

}  // namespace mysqlx
}  // namespace db
}  // namespace mysqlshdk
