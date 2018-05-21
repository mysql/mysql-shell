/*
 * Copyright (c) 2014, 2017, Oracle and/or its affiliates. All rights reserved.
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

#include "crud_definition.h"
#include <memory>
#include <string>
#include <vector>
#include "db/mysqlx/mysqlx_parser.h"
#include "db/mysqlx/util/setter_any.h"
#include "db/session.h"
#include "modules/devapi/base_database_object.h"
#include "modules/devapi/crud_definition.h"
#include "modules/devapi/mod_mysqlx_expression.h"
#include "modules/devapi/mod_mysqlx_session.h"
#include "scripting/shexcept.h"
#include "shellcore/interrupt_handler.h"
#include "utils/utils_general.h"
#include "utils/utils_string.h"

using std::placeholders::_1;

namespace mysqlsh {
namespace mysqlx {

Crud_definition::Crud_definition(std::shared_ptr<DatabaseObject> owner)
    : _owner(owner) {
  try {
    add_method("__shell_hook__", std::bind(&Crud_definition::execute, this, _1),
               "data");
    add_method("execute", std::bind(&Crud_definition::execute, this, _1),
               "data");
  } catch (shcore::Exception &e) {
    // Invalid typecast exception is the only option
    // The exception is recreated with a more explicit message
    throw shcore::Exception::argument_error(
        "Invalid connection used on CRUD operation.");
  }
}

void Crud_definition::parse_string_list(const shcore::Argument_list &args,
                                        std::vector<std::string> &data) {
  // When there is 1 argument, it must be either an array of strings or a string
  if (args.size() == 1 && args[0].type != shcore::Array &&
      args[0].type != shcore::String)
    throw shcore::Exception::argument_error(
        "Argument #1 is expected to be a string or an array of strings");

  if (args.size() == 1 && args[0].type == shcore::Array) {
    shcore::Value::Array_type_ref shell_fields = args.array_at(0);
    shcore::Value::Array_type::const_iterator index, end = shell_fields->end();

    int count = 0;
    for (index = shell_fields->begin(); index != end; index++) {
      count++;
      if (index->type != shcore::String)
        throw shcore::Exception::argument_error(shcore::str_format(
            "Element #%d is expected to be a string", count));
      else
        data.push_back(index->as_string());
    }
  } else {
    for (size_t index = 0; index < args.size(); index++)
      data.push_back(args.string_at(index));
  }
}

std::shared_ptr<mysqlshdk::db::mysqlx::Result> Crud_definition::safe_exec(
    std::function<std::shared_ptr<mysqlshdk::db::IResult>()> func) {
  bool interrupted = false;

  std::shared_ptr<ShellBaseSession> session(_owner->session());
  std::weak_ptr<ShellBaseSession> weak_session(session);

  shcore::Interrupt_handler intrl([weak_session, &interrupted]() {
    try {
      if (auto session = weak_session.lock()) {
        interrupted = true;
        session->kill_query();

        // don't propagate
        return false;
      }
    } catch (std::exception &e) {
      log_warning("Exception trying to kill query: %s", e.what());
    }
    return true;
  });
  std::shared_ptr<mysqlshdk::db::IResult> result = func();
  if (result && interrupted) {
    // If the query was interrupted but it didn't throw an exception
    // from "Error 1317 Query execution was interrupted", it means the
    // interruption happened at a time the query was not active. But we
    // still need to take action, because for the caller the query will look
    // like it was interrupted and no results will be expected. That will
    // leave the result data waiting on the wire, messing up the protocol
    // ordering.

    log_warning("Flushing resultset data from interrupted query...");
    try {
      while (result->next_resultset())
        ;
      result.reset();
    } catch (const mysqlshdk::db::Error &) {
      throw;
    }
    throw shcore::Exception::runtime_error(
        "Query interrupted. Results where flushed");
  }
  return std::static_pointer_cast<mysqlshdk::db::mysqlx::Result>(result);
}

std::shared_ptr<Session> Crud_definition::session() {
  if (_owner) {
    return std::static_pointer_cast<Session>(_owner->session());
  }
  return {};
}

void Crud_definition::init_bound_values() {
  // Initializes the bound values array on the first call to bind
  if (!_bound_values.size()) {
    for (size_t index = 0; index < _placeholders.size(); index++)
      _bound_values.push_back(NULL);
  }
}

void Crud_definition::validate_bind_placeholder(const std::string &name) {
  // Now sets the right value on the position of the indicated placeholder
  std::vector<std::string>::iterator index =
      std::find(_placeholders.begin(), _placeholders.end(), name);
  if (index == _placeholders.end())
    throw shcore::Exception::argument_error(
        "Unable to bind value for unexisting placeholder: " + name);
}

void Crud_definition::insert_bound_values(
    ::google::protobuf::RepeatedPtrField<::Mysqlx::Datatypes::Scalar> *target) {
  // First validates that all the placeholders have a bound value
  std::string str_undefined;
  if (!_placeholders.empty() && _bound_values.empty()) {
    str_undefined = shcore::str_join(_placeholders, ", ");
  } else {
    std::vector<std::string> undefined;
    for (size_t index = 0; index < _bound_values.size(); index++) {
      if (!_bound_values[index]) undefined.push_back(_placeholders[index]);
    }
    str_undefined = shcore::str_join(undefined, ", ");
  }

  // Throws the error if needed
  if (!str_undefined.empty())
    throw shcore::Exception::argument_error(
        "Missing value bindings for the following placeholders: " +
        str_undefined);

  // No errors, proceeds to set the values if any
  target->Clear();
  for (auto &value : _bound_values) target->AddAllocated(value.release());
}

void Crud_definition::encode_expression_object(Mysqlx::Expr::Expr *expr,
                                               const shcore::Value &value) {
  assert(value.type == shcore::Object);

  shcore::Object_bridge_ref object = value.as_object();
  std::string object_class = object->class_name();

  if (object_class == "Expression") {
    auto expression = std::dynamic_pointer_cast<Expression>(object);

    if (expression) {
      std::string expr_data = expression->get_data();
      if (expr_data.empty()) {
        throw shcore::Exception::argument_error(
            "Expressions can not be empty.");
      } else {
        parse_string_expression(expr, expr_data);
      }
    }
  } else if (object_class == "Date") {
    expr->set_type(Mysqlx::Expr::Expr::Expr::LITERAL);
    expr->mutable_literal()->set_type(Mysqlx::Datatypes::Scalar::V_STRING);
    expr->mutable_literal()->mutable_v_string()->set_value(value.descr());
  } else {
    std::stringstream str;
    str << "Unsupported value received: " << value.descr() << ".";
    throw shcore::Exception::argument_error(str.str());
  }
}

void Crud_definition::encode_expression_value(Mysqlx::Expr::Expr *expr,
                                              const shcore::Value &value) {
  switch (value.type) {
    case shcore::Undefined:
      throw shcore::Exception::argument_error("Invalid value");

    case shcore::Object: {
      encode_expression_object(expr, value);
      break;
    }

    case shcore::Bool:
    case shcore::UInteger:
    case shcore::Integer:
    case shcore::String:
    case shcore::Float: {
      std::unique_ptr<Mysqlx::Datatypes::Scalar> scalar(convert_value(value));
      expr->set_type(Mysqlx::Expr::Expr::Expr::LITERAL);
      expr->set_allocated_literal(scalar.release());
      break;
    }

    case shcore::Array: {
      shcore::Value::Array_type_ref array(value.as_array());
      expr->set_type(Mysqlx::Expr::Expr::Expr::ARRAY);
      for (shcore::Value &value : *array) {
        encode_expression_value(expr->mutable_array()->add_value(), value);
      }
      break;
    }

    case shcore::Map: {
      shcore::Value::Map_type_ref map(value.as_map());
      expr->set_type(Mysqlx::Expr::Expr::Expr::OBJECT);
      for (auto &iter : *map) {
        auto fld = expr->mutable_object()->add_fld();
        fld->set_key(iter.first);
        encode_expression_value(fld->mutable_value(), iter.second);
      }
      break;
    }

    case shcore::Null:
      expr->set_type(Mysqlx::Expr::Expr::Expr::LITERAL);
      expr->mutable_literal()->set_type(Mysqlx::Datatypes::Scalar::V_NULL);
      break;

    case shcore::MapRef:
    case shcore::Function:
      std::stringstream str;
      str << "Invalid value '" << value.descr() << "' in document";
      throw shcore::Exception::argument_error(str.str());
  }
}

std::unique_ptr<Mysqlx::Datatypes::Scalar> Crud_definition::convert_value(
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
      mysqlshdk::db::mysqlx::util::set_scalar(*my_scalar, value.as_string());
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

void Crud_definition::bind_value(const std::string &name, shcore::Value value) {
  init_bound_values();

  validate_bind_placeholder(name);

  // Now sets the right value on the position of the indicated placeholder
  std::vector<std::string>::iterator index =
      std::find(_placeholders.begin(), _placeholders.end(), name);

  _bound_values[index - _placeholders.begin()] = convert_value(value);
}

}  // namespace mysqlx
}  // namespace mysqlsh
