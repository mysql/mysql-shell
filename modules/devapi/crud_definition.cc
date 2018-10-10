/*
 * Copyright (c) 2014, 2018, Oracle and/or its affiliates. All rights reserved.
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
#include <mysqld_error.h>
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
#include "modules/devapi/protobuf_bridge.h"
#include "modules/mysqlxtest_utils.h"
#include "scripting/shexcept.h"
#include "shellcore/interrupt_handler.h"
#include "shellcore/utils_help.h"
#include "utils/utils_general.h"
#include "utils/utils_string.h"

using std::placeholders::_1;

namespace mysqlsh {
namespace mysqlx {

Crud_definition::Crud_definition(std::shared_ptr<DatabaseObject> owner)
    : _owner(owner), m_execution_count(0) {
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
Crud_definition::~Crud_definition() { reset_prepared_statement(); }

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
        data.push_back(index->get_string());
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

  std::shared_ptr<mysqlshdk::db::IResult> result;

  // Prepared statements are used when the statement is executed a
  // more than once after the last statement update and if the operation
  // allows prepared statements
  if (use_prepared()) {
    try {
      // If the statement has not been prepared, then it gets prepared
      if (!m_prep_stmt.has_stmt_id()) {
        m_prep_stmt.set_stmt_id(
            this->session()->session()->next_prep_stmt_id());
        set_prepared_stmt();
        this->session()->session()->prepare_stmt(m_prep_stmt);
      } else {
        update_limits();
      }

      Mysqlx::Prepare::Execute execute;
      execute.set_stmt_id(m_prep_stmt.stmt_id());
      insert_bound_values(execute.mutable_args());

      result = this->session()->session()->execute_prep_stmt(execute);
    } catch (const mysqlshdk::db::Error &error) {
      m_prep_stmt.clear_stmt_id();

      if (ER_UNKNOWN_COM_ERROR == error.code()) {
        this->session()->disable_prepared_statements();
        result = func();
      } else {
        throw;
      }
    }
  } else {
    result = func();
  }

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

  m_execution_count++;

  return std::static_pointer_cast<mysqlshdk::db::mysqlx::Result>(result);
}

std::shared_ptr<Session> Crud_definition::session() {
  if (_owner) {
    return std::static_pointer_cast<Session>(_owner->session());
  }
  return {};
}

/** By default, CRUD operations allow prepared statements if
 * the session allows them.
 */
bool Crud_definition::allow_prepared_statements() {
  return this->session()->allow_prepared_statements();
}

void Crud_definition::validate_bind_placeholder(const std::string &name) {
  // Now sets the right value on the position of the indicated placeholder
  std::vector<std::string>::iterator index =
      std::find(_placeholders.begin(), _placeholders.end(), name);
  if (index == _placeholders.end())
    throw shcore::Exception::argument_error(
        "Unable to bind value for unexisting placeholder: " + name);
}

void Crud_definition::validate_placeholders() {
  // First validates that all the placeholders have a bound value
  std::string str_undefined;
  if (!_placeholders.empty() && _bound_values.empty()) {
    str_undefined = shcore::str_join(_placeholders, ", ");
  } else {
    std::vector<std::string> undefined;
    for (const auto &placeholder : _placeholders) {
      if (_bound_values.find(placeholder) == _bound_values.end())
        undefined.push_back(placeholder);
    }
    str_undefined = shcore::str_join(undefined, ", ");
  }

  // Throws the error if needed
  if (!str_undefined.empty())
    throw shcore::Exception::argument_error(
        "Missing value bindings for the following placeholders: " +
        str_undefined);
}

void Crud_definition::insert_bound_values(
    ::google::protobuf::RepeatedPtrField<::Mysqlx::Datatypes::Scalar> *target) {
  validate_placeholders();

  // No errors, proceeds to set the values if any
  target->Clear();
  for (const auto &placeholder : _placeholders) {
    target->AddAllocated(_bound_values[placeholder].release());
    _bound_values.erase(placeholder);
  }
}

void Crud_definition::insert_bound_values(
    ::google::protobuf::RepeatedPtrField<::Mysqlx::Datatypes::Any> *target) {
  validate_placeholders();

  // No errors, proceeds to set the values if any
  target->Clear();
  for (const auto &placeholder : _placeholders) {
    auto val = target->Add();
    val->set_type(Mysqlx::Datatypes::Any_Type_SCALAR);
    val->set_allocated_scalar(_bound_values[placeholder].release());
    _bound_values.erase(placeholder);
  }
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

void Crud_definition::bind_value(const std::string &name, shcore::Value value) {
  validate_bind_placeholder(name);

  _bound_values[name] = convert_value(value);
}

void Crud_definition::reset_prepared_statement() {
  m_execution_count = 0;

  // If the statement was prepared previously, it should be deallocated
  if (m_prep_stmt.has_stmt_id()) {
    auto s = session();

    if (s && s->is_open())
      s->session()->deallocate_prep_stmt(m_prep_stmt.stmt_id());

    m_prep_stmt.Clear();
  }
}

REGISTER_HELP(
    LIMIT_EXECUTION_MODE,
    "This function can be called every time the statement is executed.");
shcore::Value Crud_definition::limit(
    const shcore::Argument_list &args,
    Dynamic_object::Allowed_function_mask limit_func_id, bool reset_offset) {
  std::string full_name = get_function_name("limit");
  args.ensure_count(1, full_name.c_str());

  try {
    if (m_limit.is_null()) reset_prepared_statement();

    m_limit = args.uint_at(0);

    if (reset_offset) m_offset = static_cast<uint64_t>(0);

    update_functions(limit_func_id);
  }
  CATCH_AND_TRANSLATE_CRUD_EXCEPTION(full_name);

  return this_object();
}

shcore::Value Crud_definition::offset(
    const shcore::Argument_list &args,
    Dynamic_object::Allowed_function_mask limit_func_id,
    const std::string fname) {
  std::string full_name = get_function_name(fname);
  args.ensure_count(1, full_name.c_str());

  try {
    // On offset we simply store the value to be bound
    m_offset = args.uint_at(0);
    update_functions(limit_func_id);
  }
  CATCH_AND_TRANSLATE_CRUD_EXCEPTION(full_name);

  return this_object();
}

shcore::Value Crud_definition::bind_(
    const shcore::Argument_list &args,
    Dynamic_object::Allowed_function_mask limit_func_id) {
  std::string full_name = get_function_name("bind");
  args.ensure_count(2, full_name.c_str());

  try {
    bind_value(args.string_at(0), args[1]);

    update_functions(limit_func_id);
  }
  CATCH_AND_TRANSLATE_CRUD_EXCEPTION(full_name);

  return this_object();
}

Crud_definition &Crud_definition::bind(const std::string &name,
                                       shcore::Value value) {
  bind_value(name, value);
  return *this;
}

}  // namespace mysqlx
}  // namespace mysqlsh
