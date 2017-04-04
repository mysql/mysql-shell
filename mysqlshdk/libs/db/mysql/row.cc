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

#include "mysqlshdk/libs/db/mysql/row.h"
#include <climits> // C limit constants
#include <limits> // std::numeric_limits
#include <cmath> // HUGE_VAL
#include <cerrno>

namespace mysqlshdk {
namespace db {
namespace mysql {
std::map<std::string, std::set<Type> > Row::_type_mappings = {
  {"string", {Type::Decimal,
              Type::Date,
              Type::NewDate,
              Type::Time,
              Type::String,
              Type::VarChar,
              Type::VarString,
              Type::NewDecimal,
              Type::Json} },
  {"int",    {Type::Year,
              Type::Tiny,
              Type::Short,
              Type::Int24,
              Type::Long,
              Type::LongLong} },
  {"double", {Type::Float,
              Type::Double} },
  {"date",   {Type::DateTime,
              Type::Timestamp} }
};

Row::Row(MYSQL_ROW row, unsigned long *lengths,
         const std::vector<Column> &metadata) :
_row(row), _metadata(metadata) {

  for (int index = 0; index < metadata.size(); index++)
    _lengths.push_back(lengths[index]);
}

std::string Row::resolve_mysql_data_type(Type type)
{
  switch (type) {
    case Type::Null:
        return "null";
    case Type::Decimal:
        return "decimal";
    case Type::Date:
        return "date";
    case Type::NewDate:
        return "new date";
    case Type::Time:
        return "time";
    case Type::String:
        return "string";
    case Type::VarChar:
        return "varchar";
    case Type::VarString:
        return "var string";
    case Type::NewDecimal:
        return "new decimal";
    case Type::TinyBlob:
        return "tiny blob";
    case Type::MediumBlob:
        return "medium blob";
    case Type::LongBlob:
        return "long blob";
    case Type::Blob:
        return "blob";
    case Type::Geometry:
        return "geometry";
    case Type::Json:
        return "json";
    case Type::Year:
        return "year";
    case Type::Tiny:
        return "tiny";
    case Type::Short:
        return "short";
    case Type::Int24:
        return "int 24";
    case Type::Long:
        return "long";
    case Type::LongLong:
        return "long long";
    case Type::Float:
        return "float";
    case Type::Double:
        return "double";
    case Type::DateTime:
        return "dsatetime";
    case Type::Timestamp:
        return "timestamp";
    case Type::Bit:
        return "bit";
    case Type::Enum:
        return "enum";
    case Type::Set:
        return "set";
  }

  return "Unknown";
}

bool Row::is_null(int index) const {
  validate_index(index);

  return _row[index] == NULL;
}

bool Row::is_int(int index) const {
  return validate_type(index, "int") &&
    _metadata[index].is_signed();
}

bool Row::is_uint(int index) const {
  return validate_type(index, "int") &&
    !_metadata[index].is_signed();
}

bool Row::is_string(int index) const {
  return validate_type(index, "string");
}

bool Row::is_binary(int index) const {
  return _metadata[index].is_binary();
}

bool Row::is_double(int index) const {
  return validate_type(index, "double");
}

bool Row::is_date(int index) const {
  return validate_type(index, "date");
}

void Row::validate_index(int index) const {
  if (index < 0 || index >= _metadata.size())
    throw std::runtime_error("Error trying to fetch row data: index out of bounds");
}

bool Row::validate_type(int index, const std::string& type, bool throw_on_error) const {
  bool ret_val = false;

  validate_index(index);

  Type column_type;
  if (index < _metadata.size())
    column_type = _metadata[index].get_type();

  ret_val = _type_mappings[type].find(column_type) != _type_mappings[type].end();

  // TODO: Add both requested type translator and found type translator
  //       to be included on the error message
  if (!ret_val && throw_on_error) {
    std::string error("Error trying to fetch row data: invalid data type: "\
      "expected " + type + " "\
      "found " + resolve_mysql_data_type(_metadata[index].get_type()) + " "\
      "at position " + std::to_string(index));

      throw std::runtime_error(error);
  }

  return ret_val;
}

size_t Row::size() const {
  return _metadata.size();
}

int64_t Row::get_int(int index) const {
  uint64_t ret_val = 0;

  if (validate_type(index, "int", true)) {

    if (!_metadata[index].is_signed()) {
      uint64_t unsigned_val = strtoul(_row[index], nullptr, 0);

      if ((errno == ERANGE && unsigned_val == ULONG_MAX)  ||
          unsigned_val > (std::numeric_limits<int64_t>::max)())
        throw std::runtime_error("Error trying to fetch row data: unsigned "\
                                 "integer value exceeds allowed range");

      ret_val = static_cast<int64_t>(unsigned_val);
    } else {
      ret_val = strtol(_row[index], nullptr, 0);

      if (errno == ERANGE && (ret_val == LONG_MAX || ret_val == LONG_MIN))
        throw std::runtime_error("Error trying to fetch row data: integer "\
                                 "value exceeds allowed range");
    }
  }

  return ret_val;
}

uint64_t Row::get_uint(int index) const {
  uint64_t ret_val = 0;

  if (validate_type(index, "int", true)) {

    if (!_metadata[index].is_signed()) {
      ret_val = strtoul(_row[index], nullptr, 0);

      if (ret_val == ULONG_MAX && errno == ERANGE)
        throw std::runtime_error("Error trying to fetch row data: unsigned "\
                                 "integer value exceeds allowed range");
    }
    else {
      int64_t signed_val = strtol(_row[index], nullptr, 0);

      if (signed_val < 0 || (errno == ERANGE &&
         (signed_val == LONG_MAX || signed_val == LONG_MIN)))
        throw std::runtime_error("Error trying to fetch row data: integer value out of the allowed range");

      ret_val = static_cast<uint64_t>(signed_val);
    }
  }

  return ret_val;
}

std::string Row::get_string(int index) const {
  std::string ret_val;
  if (validate_type(index, "string", true)) {
    ret_val = std::string(_row[index], _lengths[index]);
  }

  return ret_val;
}

std::pair<const char*, size_t> Row::get_data(int index) const {
  return std::pair<const char*, size_t>(_row[index], _lengths[index]);
}

double Row::get_double(int index) const {
  double ret_val = 0;

  if (validate_type(index, "double", true))
  ret_val = strtod(_row[index], nullptr);

  if (errno == ERANGE && (ret_val == HUGE_VAL || ret_val == -HUGE_VAL))
    throw std::runtime_error("Error trying to fetch row data: double value "\
                             "out of the allowed range");

  return ret_val;
}


std::string Row::get_date(int index) const {
  std::string ret_val;

  if (validate_type(index, "date", true))
    ret_val = _row[index];

  return ret_val;
}

// TODO: Need to add support for these data types
//case MYSQL_TYPE_BIT:
//case MYSQL_TYPE_ENUM:
//case MYSQL_TYPE_SET:

}
}
}
