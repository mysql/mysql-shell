/*
 * Copyright (c) 2014, Oracle and/or its affiliates. All rights reserved.
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

#include "table_crud_definition.h"
#include <sstream>

using namespace mysh::mysqlx;

::mysqlx::TableValue Table_crud_definition::map_table_value(shcore::Value source)
{
  switch (source.type)
  {
    case shcore::Null:
      return ::mysqlx::TableValue();
      break;
    case shcore::Bool:
      return ::mysqlx::TableValue(source.as_bool());
      break;
    case shcore::String:
      return ::mysqlx::TableValue(source.as_string());
      break;
    case shcore::Integer:
      return ::mysqlx::TableValue(source.as_int());
      break;
    case shcore::UInteger:
      return ::mysqlx::TableValue(source.as_uint());
      break;
    case shcore::Float:
      return ::mysqlx::TableValue(source.as_double());
      break;
    case shcore::Object:
    case shcore::Array:
    case shcore::Map:
    case shcore::MapRef:
    case shcore::Function:
      std::stringstream str;
      str << "Unsupported value received for table insert operation: " << source.descr();
      throw shcore::Exception::argument_error(str.str());
      break;
  }
}