/*
 * Copyright (c) 2015, Oracle and/or its affiliates. All rights reserved.
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
#include "mod_mysqlx_expression.h"
#include "shellcore/obj_date.h"
#include <sstream>

using namespace mysh::mysqlx;

::mysqlx::TableValue Table_crud_definition::map_table_value(shcore::Value source) {
  switch (source.type) {
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
    {
      shcore::Object_bridge_ref object = source.as_object();

      std::string object_class = object->class_name();

      if (object_class == "Expression") {
        std::shared_ptr<Expression> expression = std::dynamic_pointer_cast<Expression>(object);

        if (expression) {
          std::string expr_data = expression->get_data();
          if (expr_data.empty())
            throw shcore::Exception::argument_error("Expressions can not be empty.");
          else
            return ::mysqlx::TableValue(expr_data, ::mysqlx::TableValue::TExpression);
        }
      }
      if (object_class == "Date") {
        std::string data = source.descr();
        return ::mysqlx::TableValue(data);
      } else {
        std::stringstream str;
        str << "Unsupported value received: " << source.descr() << ".";
        throw shcore::Exception::argument_error(str.str());
      }
    }
    break;
    case shcore::Array:
    case shcore::Map:
    case shcore::MapRef:
    case shcore::Function:
    case shcore::Undefined:
      std::stringstream str;
      str << "Unsupported value received: " << source.descr();
      throw shcore::Exception::argument_error(str.str());
      break;
  }

  return ::mysqlx::TableValue();
}
