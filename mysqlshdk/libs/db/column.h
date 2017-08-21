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

// MySQL DB access module, for use by plugins and others
// For the module that implements interactive DB functionality see mod_db

#ifndef _CORELIBS_DB_COLUMN_H_
#define _CORELIBS_DB_COLUMN_H_
#include <cstdint>
#include <string>
#include "mysqlshdk_export.h"

namespace mysqlshdk {
namespace db {
/*
 * We require a common representation of the MySQL data MySQL data types
 * So it can be used no matter the source of the metadata information
 * (MySQL protocol or XProtocol)
 */
enum class SHCORE_PUBLIC Type {
  Null,
  String,
  Integer,
  UInteger,
  Float,
  Double,
  Decimal,
  Bytes,
  Geometry,
  Json,
  Date,
  Time,
  DateTime,
  Bit,
  Enum,
  Set
};

std::string to_string(Type type);

inline bool is_string_type(Type type) {
  return (type == Type::Bytes || type == Type::Geometry || type == Type::Json ||
          type == Type::Date || type == Type::Time || type == Type::DateTime ||
          type == Type::Enum || type == Type::Set || type == Type::String);
}

/**
 * These class represents a protocol independent Column
 * The Resultset implementation for each protocol should
 * make sure the metadata is represented on instances of this
 * class.
 *
 * This class aligns with the Column API as specified
 * on the DevAPI.
 */
class SHCORE_PUBLIC Column {
 public:
  Column(const std::string& schema, const std::string& table_name,
         const std::string& table_label, const std::string& column_name,
         const std::string& column_label, uint32_t length, int frac_digits,
         Type type, uint32_t collation_id, bool unsigned_, bool zerofill,
         bool binary);

  bool operator==(const Column& o) const {
    return _schema == o._schema && _table_name == o._table_name &&
           _table_label == o._table_label && _column_name == o._column_name &&
           _column_label == o._column_label &&
           _collation_id == o._collation_id && _length == o._length &&
           _fractional == o._fractional && _type == o._type &&
           _unsigned == o._unsigned && _zerofill == o._zerofill &&
           _binary == o._binary;
  }

  const std::string& get_schema() const {
    return _schema;
  }
  const std::string& get_table_name() const {
    return _table_name;
  }
  const std::string& get_table_label() const {
    return _table_label;
  }
  const std::string& get_column_name() const {
    return _column_name;
  }
  const std::string& get_column_label() const {
    return _column_label;
  }
  uint32_t get_length() const {
    return _length;
  }
  int get_fractional() const {
    return _fractional;
  }
  Type get_type() const {
    return _type;
  }
  std::string get_collation_name() const;
  std::string get_charset_name() const;
  uint32_t get_collation() const {
    return _collation_id;
  }

  bool is_unsigned() const {
    return _unsigned;
  }
  bool is_zerofill() const {
    return _zerofill;
  }
  bool is_binary() const {
    return _binary;
  }

 private:
  std::string _schema;
  std::string _table_name;
  std::string _table_label;
  std::string _column_name;
  std::string _column_label;
  uint32_t _collation_id;
  uint32_t _length;
  int _fractional;
  Type _type;

  // Flags
  bool _unsigned;
  bool _zerofill;
  bool _binary;
};
}  // namespace db
}  // namespace mysqlshdk
#endif
