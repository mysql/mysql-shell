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
#include "mysqlshdk_export.h"
#include <string>
#include <cstdint>

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
  Double,
  Decimal,
  Blob,
  Geometry,
  Json,
  DateTime,
  Date,
  Time,
  Bit,
  Enum,
  Set
};

/**
 * These class represents a protocol independent Column
 * The Resultset implementation for each protocol should
 * make sure the metadata is represented on instances of this
 * class.
 *
 * This class aligns with the Column API as specified
 * on the DevAPI.
 *
 * TODO: Fix the handling of the collation and charset data.
 */
class SHCORE_PUBLIC Column {
public:
  Column(const std::string& schema, const std::string& table_name,
         const std::string& table_label, const std::string& column_name,
         const std::string& column_label, int length, Type type,
         int decimals, int charset, bool unsigned_, bool zerofill, bool binary,
         bool numeric);

  const std::string& get_schema() const { return _schema; }
  const std::string& get_table_name() const { return _table_name; }
  const std::string& get_table_label() const { return _table_label; }
  const std::string& get_column_name() const { return _column_name; }
  const std::string& get_column_label() const { return _column_label; }
  long get_length() const { return _length; }
  Type get_type() const { return _type; }
  std::string get_collation() const { return _collation; }
  int get_charset() const { return _charset; }

  bool is_unsigned() const { return _unsigned; }
  bool is_zerofill() const { return _zerofill; }
  bool is_binary() const { return _binary; }
  bool is_numeric() const { return _numeric; }
  bool is_blob() const { return _blob; }

private:
  std::string _schema;
  std::string _table_name;
  std::string _table_label;
  std::string _column_name;
  std::string _column_label;
  std::string _collation;
  int _charset;
  uint64_t _length;
  Type _type;

  // Flags
  bool _unsigned;
  bool _zerofill;
  bool _binary;
  bool _numeric;
  bool _blob;
};
}
}
#endif
