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

#include "mysqlshdk/libs/db/mysql/result.h"
#include "mysqlshdk/libs/db/mysql/session.h"
#include "mysqlshdk/libs/db/mysql/row.h"
#include "utils/utils_general.h"

#include <stdlib.h>

namespace mysqlshdk {
namespace db {
namespace mysql {
Result::Result(std::shared_ptr<mysqlshdk::db::mysql::Session_impl> owner,
               uint64_t affected_rows_, unsigned int warning_count_,
               uint64_t last_insert_id, const char *info_) :
               _session(owner), _affected_rows(affected_rows_),
               _last_insert_id(last_insert_id),
               _warning_count(warning_count_),
               _fetched_row_count(0), _execution_time(0),
               _has_resultset(false), _fetch_started(false), _warnings(nullptr) {
  if (info_)
    _info.assign(info_);
}

int Result::fetch_metadata() {
  int num_fields = 0;

  _metadata.clear();

  // res could be NULL on queries not returning data
  std::shared_ptr<MYSQL_RES> res = _result.lock();

  if (res) {
    num_fields = mysql_num_fields(res.get());
    MYSQL_FIELD *fields = mysql_fetch_fields(res.get());

    for (int index = 0; index < num_fields; index++) {
      _metadata.push_back(mysqlshdk::db::Column(fields[index].db,
        fields[index].org_table,
        fields[index].table,
        fields[index].org_name,
        fields[index].name,
        fields[index].length,
        map_data_type(fields[index].type),
        fields[index].decimals,
        fields[index].charsetnr,
        bool(fields[index].charsetnr & UNSIGNED_FLAG),
        bool(fields[index].charsetnr & ZEROFILL_FLAG),
        bool(fields[index].charsetnr & BINARY_FLAG),
        bool(fields[index].charsetnr & NUM_FLAG)));
    }
  }

  return num_fields;
}

Result::~Result() {}

std::unique_ptr<IRow> Result::fetch_one() {
  std::unique_ptr<IRow> ret_val;

  if (has_resultset()) {
    // Loads the first row
    std::shared_ptr<MYSQL_RES> res = _result.lock();

    if (res) {
      MYSQL_ROW  mysql_row = mysql_fetch_row(res.get());
      if (mysql_row) {
        unsigned long *lengths;
        lengths = mysql_fetch_lengths(res.get());

        ret_val.reset(new Row(shared_from_this(), mysql_row, lengths));

        // Each read row increases the count
        _fetched_row_count++;
      }
    }
  }

  return ret_val;
}

bool Result::next_data_set() {
  bool ret_val = false;

  ret_val = _session->next_data_set();

  if (ret_val)
    _fetch_started = false;

  return ret_val;
}

std::unique_ptr<IRow> Result::fetch_one_warning() {
  std::unique_ptr<IRow> ret_val;

  if (_warning_count && !_warnings)
    _warnings = _session->query("show warnings", true);

  if (_warnings)
    ret_val = _warnings->fetch_one();

  return ret_val;
}

void Result::reset(std::shared_ptr<MYSQL_RES> res) {
  _has_resultset = false;

  if (res)
    _has_resultset = true;

  _result = res;
}

Type Result::map_data_type(int raw_type) {
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
      return Type::String;
    case MYSQL_TYPE_TINY_BLOB:
    case MYSQL_TYPE_MEDIUM_BLOB:
    case MYSQL_TYPE_LONG_BLOB:
    case MYSQL_TYPE_BLOB:
      return Type::Blob;
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
      return Type::Integer;
    case MYSQL_TYPE_FLOAT:
    case MYSQL_TYPE_DOUBLE:
      return Type::Double;
    case MYSQL_TYPE_DATETIME:
    case MYSQL_TYPE_TIMESTAMP:
    case MYSQL_TYPE_DATETIME2:
    case MYSQL_TYPE_TIMESTAMP2:
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
}
}
}
