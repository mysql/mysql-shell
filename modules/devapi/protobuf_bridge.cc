/*
 * Copyright (c) 2018, Oracle and/or its affiliates. All rights reserved.
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

// MySQL DB access module, for use by plugins and others
// For the module that implements interactive DB functionality see mod_db

#include "modules/devapi/protobuf_bridge.h"
#include "db/mysqlx/util/setter_any.h"
#include "modules/devapi/mod_mysqlx_expression.h"

namespace mysqlsh {
namespace mysqlx {

void insert_bound_values(
    shcore::Argument_list *parameters,
    ::google::protobuf::RepeatedPtrField<::Mysqlx::Datatypes::Any> *target) {
  for (const auto &param : *parameters) {
    auto val = target->Add();
    val->set_type(Mysqlx::Datatypes::Any_Type_SCALAR);
    val->set_allocated_scalar(convert_value(param).release());
  }

  parameters->clear();
}

std::unique_ptr<Mysqlx::Datatypes::Scalar> convert_value(
    const shcore::Value &value) {
  std::unique_ptr<Mysqlx::Datatypes::Scalar> my_scalar(
      new Mysqlx::Datatypes::Scalar);

  switch (value.type) {
    case shcore::Undefined:
      throw shcore::Exception::argument_error("Invalid value");

    case shcore::Bool:
      mysqlshdk::db::mysqlx::util::set_scalar(*my_scalar, value.as_bool());
      return my_scalar;

    case shcore::UInteger:
      mysqlshdk::db::mysqlx::util::set_scalar(*my_scalar, value.as_uint());
      return my_scalar;

    case shcore::Integer:
      mysqlshdk::db::mysqlx::util::set_scalar(*my_scalar, value.as_int());
      return my_scalar;

    case shcore::String:
      mysqlshdk::db::mysqlx::util::set_scalar(*my_scalar, value.get_string());
      return my_scalar;

    case shcore::Float:
      mysqlshdk::db::mysqlx::util::set_scalar(*my_scalar, value.as_double());
      return my_scalar;

    case shcore::Object: {
      shcore::Object_bridge_ref object = value.as_object();
      std::string object_class = object->class_name();

      if (object_class == "Expression") {
        throw std::logic_error(
            "Only scalar values supported on this conversion");
      } else if (object_class == "Date") {
        my_scalar->set_type(Mysqlx::Datatypes::Scalar::V_STRING);
        my_scalar->mutable_v_string()->set_value(value.descr());
        return my_scalar;
      } else {
        std::stringstream str;
        str << "Unsupported value received: " << value.descr() << ".";
        throw shcore::Exception::argument_error(str.str());
      }
    }
    case shcore::Array: {
      // FIXME this should recursively encode the array (same for Map),
      // FIXME in case there are expressions embedded
      my_scalar->set_type(Mysqlx::Datatypes::Scalar::V_OCTETS);
      my_scalar->mutable_v_octets()->set_content_type(CONTENT_TYPE_JSON);
      my_scalar->mutable_v_octets()->set_value(value.json());
      return my_scalar;
    }
    case shcore::Map: {
      my_scalar->set_type(Mysqlx::Datatypes::Scalar::V_OCTETS);
      my_scalar->mutable_v_octets()->set_content_type(CONTENT_TYPE_JSON);
      my_scalar->mutable_v_octets()->set_value(value.json());
      return my_scalar;
    }
    case shcore::Null:
    case shcore::MapRef:
    case shcore::Function:
      std::stringstream str;
      str << "Unsupported value received: " << value.descr();
      throw shcore::Exception::argument_error(str.str());
      break;
  }
  return my_scalar;
}

}  // namespace mysqlx
}  // namespace mysqlsh
