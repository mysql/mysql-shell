/*
 * Copyright (c) 2017, 2018, Oracle and/or its affiliates. All rights reserved.
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

#include "mysqlshdk/libs/db/column.h"
#include <stdexcept>
#include "mysqlshdk/libs/db/charset.h"

namespace mysqlshdk {
namespace db {

std::string to_string(Type type) {
  switch (type) {
    case Type::Null:
      return "Null";
    case Type::String:
      return "String";
    case Type::Integer:
      return "Integer";
    case Type::UInteger:
      return "UInteger";
    case Type::Float:
      return "Float";
    case Type::Double:
      return "Double";
    case Type::Decimal:
      return "Decimal";
    case Type::Bytes:
      return "Bytes";
    case Type::Geometry:
      return "Geometry";
    case Type::Json:
      return "Json";
    case Type::DateTime:
      return "DateTime";
    case Type::Date:
      return "Date";
    case Type::Time:
      return "Time";
    case Type::Bit:
      return "Bit";
    case Type::Enum:
      return "Enum";
    case Type::Set:
      return "Set";
  }
  throw std::logic_error("Unknown type");
}

Type string_to_type(const std::string &type) {
  if (type == "Null")
    return Type::Null;
  else if (type == "String")
    return Type::String;
  else if (type == "Integer")
    return Type::Integer;
  else if (type == "UInteger")
    return Type::UInteger;
  else if (type == "Float")
    return Type::Float;
  else if (type == "Double")
    return Type::Double;
  else if (type == "Decimal")
    return Type::Decimal;
  else if (type == "Bytes")
    return Type::Bytes;
  else if (type == "Geometry")
    return Type::Geometry;
  else if (type == "Json")
    return Type::Json;
  else if (type == "DateTime")
    return Type::DateTime;
  else if (type == "Date")
    return Type::Date;
  else if (type == "Time")
    return Type::Time;
  else if (type == "Bit")
    return Type::Bit;
  else if (type == "Enum")
    return Type::Enum;
  else if (type == "Set")
    return Type::Set;
  else
    throw std::logic_error("Unknown type " + type);
}

Column::Column(const std::string &schema, const std::string &table_name,
               const std::string &table_label, const std::string &column_name,
               const std::string &column_label, uint32_t length, int fractional,
               Type type, uint32_t collation_id, bool unsigned_, bool zerofill,
               bool binary)
    : _schema(schema),
      _table_name(table_name),
      _table_label(table_label),
      _column_name(column_name),
      _column_label(column_label),
      _collation_id(collation_id),
      _length(length),
      _fractional(fractional),
      _type(type),
      _unsigned(unsigned_),
      _zerofill(zerofill),
      _binary(binary) {}

std::string Column::get_collation_name() const {
  return charset::collation_name_from_collation_id(_collation_id);
}

std::string Column::get_charset_name() const {
  return charset::charset_name_from_collation_id(_collation_id);
}

bool Column::is_numeric() const {
  return (_type == Type::Integer || _type == Type::UInteger ||
          _type == Type::Float || _type == Type::Decimal ||
          _type == Type::Double);
}

}  // namespace db
}  // namespace mysqlshdk
