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

#include "mysqlshdk/libs/db/mysqlx/row.h"
#include <cerrno>
#include "mysqlshdk/libs/db/mysqlx/result.h"
#include "mysqlshdk/libs/utils/utils_string.h"

namespace mysqlshdk {
namespace db {
namespace mysqlx {

#define FIELD_ERROR(index, msg) \
  std::invalid_argument(        \
      shcore::str_format("%s(%u): " msg, __FUNCTION__, index).c_str())

#define FIELD_ERROR1(index, msg, arg) \
  std::invalid_argument(              \
      shcore::str_format("%s(%u): " msg, __FUNCTION__, index, arg).c_str())

#define VALIDATE_INDEX(index)                          \
  do {                                                 \
    if (index >= num_fields())                         \
      throw FIELD_ERROR(index, "index out of bounds"); \
  } while (0)

#define FAILED_GET_TYPE(index, TYPE_CHECK)                                     \
  do {                                                                         \
    Type ftype = get_type(index);                                              \
    if (!(TYPE_CHECK))                                                         \
      throw FIELD_ERROR1(index, "field type is %s", to_string(ftype).c_str()); \
    if (_row->is_null(index))                                                  \
      throw FIELD_ERROR(index, "field is null");                               \
    throw FIELD_ERROR(index, "could not get field value");                     \
  } while (0)

Row::Row(const Result *owner) : _owner(owner), _row(nullptr) {
}

void Row::reset(const xcl::XRow *row) {
  _row = row;
}

bool Row::is_null(uint32_t index) const {
  assert(_row);
  VALIDATE_INDEX(index);
  return _row->is_null(index);
}

uint32_t Row::num_fields() const {
  assert(_row);
  return static_cast<uint32_t>(_row->get_number_of_fields());
}

Type Row::get_type(uint32_t index) const {
  VALIDATE_INDEX(index);
  return _owner->get_metadata().at(index).get_type();
}

std::string Row::get_as_string(uint32_t index) const {
  assert(_row);
  VALIDATE_INDEX(index);
  std::string data;
  const Column &column = _owner->get_metadata().at(index);
  if (column.get_type() == Type::Date || column.get_type() == Type::DateTime) {
    // TODO workaround for libmysqlxclient bug
    return get_string(index);
  } else if (column.get_type() == Type::Bit) {
    return shcore::bits_to_string(get_bit(index), column.get_length());
  }
  if (!_row->get_field_as_string(index, &data)) {
    throw FIELD_ERROR(index, "failed converting field to string");
  }
  return data;
}

int64_t Row::get_int(uint32_t index) const {
  assert(_row);
  int64_t value;
  VALIDATE_INDEX(index);
  if (!_row->get_int64(index, &value)) {
    uint64_t uvalue;
    if (!_row->get_uint64(index, &uvalue)) {
      FAILED_GET_TYPE(index, (false));
    } else {
      if (uvalue > LLONG_MAX) {
        throw FIELD_ERROR(index, "field value out of the allowed range");
      }
      value = static_cast<int64_t>(uvalue);
    }
  }
  return value;
}

uint64_t Row::get_uint(uint32_t index) const {
  assert(_row);
  uint64_t value;
  VALIDATE_INDEX(index);
  if (!_row->get_uint64(index, &value)) {
    int64_t svalue;
    if (!_row->get_int64(index, &svalue)) {
      FAILED_GET_TYPE(index, (false));
    } else {
      if (svalue < 0) {
        throw FIELD_ERROR(index, "field value out of the allowed range");
      }
      value = static_cast<uint64_t>(svalue);
    }
  }
  return value;
}

std::string Row::get_string(uint32_t index) const {
  assert(_row);
  VALIDATE_INDEX(index);
  Type type = _owner->get_metadata().at(index).get_type();
  switch (type) {
    case Type::Date: {
      ::xcl::DateTime date;
      if (!_row->get_datetime(index, &date))
        FAILED_GET_TYPE(index, (true));
      // FIXME workaround for libmysqlxclient bug that always returns a
      // DateTime object with time
      date = xcl::DateTime(date.year(), date.month(), date.day());

      // FIXME remove the replace once libmysqlx is fixed
      return shcore::str_replace(date.to_string(), "/", "-");
    }
    case Type::DateTime: {
      ::xcl::DateTime date;
      if (!_row->get_datetime(index, &date))
        FAILED_GET_TYPE(index, (true));
      // FIXME remove the replace once libmysqlx is fixed
      return shcore::str_replace(date.to_string(), "/", "-");
    }
    case Type::Time: {
      ::xcl::Time time;
      if (!_row->get_time(index, &time))
        FAILED_GET_TYPE(index, (true));
      return time.to_string();
    }
    case Type::Enum: {
      std::string enu;
      if (!_row->get_enum(index, &enu))
        FAILED_GET_TYPE(index, (true));
      return enu;
    }
    case Type::Set: {
      std::set<std::string> set;
      if (!_row->get_set(index, &set))
        FAILED_GET_TYPE(index, (true));
      return shcore::str_join(set.begin(), set.end(), ",");
    }
    case Type::Geometry:
    case Type::Json:
    case Type::Bytes:
    case Type::String: {
      std::string value;
      if (!_row->get_string(index, &value))
        FAILED_GET_TYPE(index, (true));
      return value;
    }
    case Type::Null:
    case Type::Bit:
    case Type::Decimal:
    case Type::Integer:
    case Type::UInteger:
    case Type::Double:
    case Type::Float:
      FAILED_GET_TYPE(index, (false));
  }
  throw FIELD_ERROR(index, "invalid type");
}

std::pair<const char *, size_t> Row::get_string_data(uint32_t index) const {
  assert(_row);
  const char *data;
  size_t length;
  VALIDATE_INDEX(index);
  if (!_row->get_string(index, &data, &length))
    FAILED_GET_TYPE(index, (ftype == Type::String || ftype == Type::Bytes));
  return {data, length};
}

float Row::get_float(uint32_t index) const {
  assert(_row);
  float value;
  VALIDATE_INDEX(index);
  switch (_owner->get_metadata().at(index).get_type()) {
    case Type::Float:
      if (!_row->get_float(index, &value)) {
        throw FIELD_ERROR(index, "could not get float field value");
      }
      break;
    case Type::Double: {
      double fvalue;
      if (!_row->get_double(index, &fvalue)) {
        throw FIELD_ERROR(index, "could not get double field value");
      }
      value = static_cast<float>(fvalue);
      break;
    }
    case Type::Decimal: {
      xcl::Decimal decimal;
      if (_row->get_decimal(index, &decimal)) {
        value = std::stof(decimal.to_string());
      } else {
        throw FIELD_ERROR(index, "could not get decimal field value");
      }
      break;
    }
    default:
      FAILED_GET_TYPE(index, (false));
      break;
  }
  return value;
}

double Row::get_double(uint32_t index) const {
  assert(_row);
  double value;
  VALIDATE_INDEX(index);
  switch (_owner->get_metadata().at(index).get_type()) {
    case Type::Double:
      if (!_row->get_double(index, &value)) {
        throw FIELD_ERROR(index, "could not get double field value");
      }
      break;
    case Type::Float: {
      float fvalue;
      if (!_row->get_float(index, &fvalue)) {
        throw FIELD_ERROR(index, "could not get float field value");
      }
      value = static_cast<double>(fvalue);
      break;
    }
    case Type::Decimal: {
      xcl::Decimal decimal;
      if (_row->get_decimal(index, &decimal)) {
        value = std::stod(decimal.to_string());
      } else {
        throw FIELD_ERROR(index, "could not get decimal field value");
      }
      break;
    }
    default:
      FAILED_GET_TYPE(index, (false));
      break;
  }
  return value;
}

uint64_t Row::get_bit(uint32_t index) const {
  assert(_row);
  uint64_t value;
  VALIDATE_INDEX(index);
  if (!_row->get_bit(index, &value))
    FAILED_GET_TYPE(index, (ftype == Type::Bit));
  return value;
}

}  // namespace mysqlx
}  // namespace db
}  // namespace mysqlshdk
