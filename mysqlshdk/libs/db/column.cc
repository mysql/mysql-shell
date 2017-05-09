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

#include "mysqlshdk/libs/db/column.h"

namespace mysqlshdk {
namespace db {

Column::Column(const std::string& schema, const std::string& table_name,
             const std::string& table_label, const std::string& column_name,
             const std::string& column_label, int length, Type type,
             int /* decimals*/, int charset, bool unsigned_, bool zerofill,
             bool binary, bool numeric) :
  _schema(schema),
  _table_name(table_name),
  _table_label(table_label),
  _column_name(column_name),
  _column_label(column_label),
  _charset(charset),
  _length(length),
  _type(type),
  _unsigned(unsigned_),
  _zerofill(zerofill),
  _binary(binary),
  _numeric(numeric)
{}

}
}
