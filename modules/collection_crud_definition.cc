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

#include "collection_crud_definition.h"
#include "mod_mysqlx_expression.h"
#include <sstream>

using namespace mysh::mysqlx;

::mysqlx::DocumentValue Collection_crud_definition::map_document_value(shcore::Value source)
{
  switch (source.type)
  {
    case shcore::Undefined:
      throw shcore::Exception::argument_error("Invalid value");
    case shcore::Bool:
    case shcore::UInteger:
    case shcore::Integer:
      return ::mysqlx::DocumentValue(source.as_int());
      break;
    case shcore::String:
      return ::mysqlx::DocumentValue(source.as_string());
      break;
    case shcore::Float:
      return ::mysqlx::DocumentValue(source.as_double());
      break;
    case shcore::Object:
    {
      shcore::Object_bridge_ref object = source.as_object();

      std::string object_class = object->class_name();

      if (object_class == "Expression")
      {
        std::shared_ptr<Expression> expression = std::dynamic_pointer_cast<Expression>(object);

        if (expression)
        {
          std::string expr_data = expression->get_data();
          if (expr_data.empty())
            throw shcore::Exception::argument_error("Expressions can not be empty.");
          else
            return ::mysqlx::DocumentValue(expr_data, ::mysqlx::DocumentValue::TExpression);
        }
      }
      if (object_class == "Date")
      {
        std::string data = source.descr();
        return ::mysqlx::DocumentValue(data);
      }
      else
      {
        std::stringstream str;
        str << "Unsupported value received: " << source.descr() << ".";
        throw shcore::Exception::argument_error(str.str());
      }
    }
    break;
    case shcore::Array:
      return ::mysqlx::DocumentValue(source.json(), ::mysqlx::DocumentValue::TArray);
      break;
    case shcore::Map:
      return ::mysqlx::DocumentValue(::mysqlx::Document(source.json()));
      break;
    case shcore::Null:
    case shcore::MapRef:
    case shcore::Function:
      std::stringstream str;
      str << "Unsupported value received: " << source.descr();
      throw shcore::Exception::argument_error(str.str());
      break;
  }

  return ::mysqlx::DocumentValue();
}