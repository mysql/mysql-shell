/*
 * Copyright (c) 2015, 2017, Oracle and/or its affiliates. All rights reserved.
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
#include "modules/devapi/mod_mysqlx_table_update.h"
#include <memory>
#include <sstream>
#include <string>
#include <vector>
#include "db/mysqlx/mysqlx_parser.h"
#include "modules/devapi/mod_mysqlx_expression.h"
#include "modules/devapi/mod_mysqlx_resultset.h"
#include "modules/devapi/mod_mysqlx_table.h"
#include "scripting/common.h"
#include "utils/utils_time.h"

using namespace std::placeholders;
using namespace mysqlsh::mysqlx;
using namespace shcore;

TableUpdate::TableUpdate(std::shared_ptr<Table> owner)
    : Table_crud_definition(std::static_pointer_cast<DatabaseObject>(owner)) {
  message_.mutable_collection()->set_schema(owner->schema()->name());
  message_.mutable_collection()->set_name(owner->name());
  message_.set_data_model(Mysqlx::Crud::TABLE);

  // Exposes the methods available for chaining
  add_method("update", std::bind(&TableUpdate::update, this, _1), "data");
  add_method("set", std::bind(&TableUpdate::set, this, _1), "data");
  add_method("where", std::bind(&TableUpdate::where, this, _1), "data");
  add_method("orderBy", std::bind(&TableUpdate::order_by, this, _1), "data");
  add_method("limit", std::bind(&TableUpdate::limit, this, _1), "data");
  add_method("bind", std::bind(&TableUpdate::bind, this, _1), "data");

  // Registers the dynamic function behavior
  register_dynamic_function(F::update,F::_empty);
  register_dynamic_function(F::set, F::update | F::set);
  register_dynamic_function(F::where, F::set);
  register_dynamic_function(F::orderBy, F::set | F::where);
  register_dynamic_function(F::limit, F::set | F::where | F::orderBy);
  register_dynamic_function(F::bind, F::set | F::where | F::orderBy | F::limit | F::bind);
  register_dynamic_function(F::execute, F::set | F::where | F::orderBy | F::limit | F::bind);
  register_dynamic_function(F::__shell_hook__, F::set | F::where | F::orderBy | F::limit | F::bind);

  // Initial function update
  update_functions(F::_empty);
}

/**
* Initializes this record update handler.
* \return This TableUpdate object.
*
* This function is called automatically when Table.update() is called.
*
* The actual update of the records will occur only when the execute method is
* called.
*
* #### Method Chaining
*
* After this function invocation, the following functions can be invoked:
*
* - set(String attribute, Value value)
* - where(String searchCriteria)
* - orderBy(List sortExprStr)
* - limit(Integer numberOfRows)
* - bind(String name, Value value)
* - execute()
*
* \sa Usage examples at execute().
*/
#if DOXYGEN_JS
TableUpdate TableUpdate::update() {}
#elif DOXYGEN_PY
TableUpdate TableUpdate::update() {}
#endif
shcore::Value TableUpdate::update(const shcore::Argument_list &args) {
  // Each method validates the received parameters
  args.ensure_count(0, get_function_name("update").c_str());

  std::shared_ptr<Table> table(std::static_pointer_cast<Table>(_owner));

  if (table) {
    try {
      // Updates the exposed functions
      update_functions(F::update);
    }
    CATCH_AND_TRANSLATE_CRUD_EXCEPTION(get_function_name("update"));
  }

  return Value(std::static_pointer_cast<Object_bridge>(shared_from_this()));
}

//! Updates the column value on records in a table.
#if DOXYGEN_CPP
//! \param args should contain the next elements:
//! \li A string with the column name to be updated.
//! \li The value to be set on the specified column.
#else
//! \param attribute A string with the column name to be updated.
//! \param value The value to be set on the specified column.
#endif
/**
* \return This TableUpdate object.
*
* Adds an opertion into the update handler to update a column value in on the
* records that were included on the selection filter and limit.
*
* The update will be done on the table's records once the execute method is
* called.
*
* #### Using Expressions for Values
*
* Tipically, the received values are inserted into the table in a literal way.
*
* An additional option is to pass an explicit expression which is evaluated on
* the server, the resulting value is inserted on the table.
*
* To define an expression use:
* \code{.py}
* mysqlx.expr(expression)
* \endcode
*
* The expression also can be used for \a [Parameter
* Binding](param_binding.html).
*
* #### Method Chaining
*
* This function can be invoked multiple times after:
* - update()
* - set(String attribute, Value value)
*
* After this function invocation, the following functions can be invoked:
* - set(String attribute, Value value)
* - where(String searchCriteria)
* - orderBy(List sortExprStr)
* - limit(Integer numberOfRows)
* - bind(String name, Value value)
* - execute()
*
* \sa Usage examples at execute().
*/
#if DOXYGEN_JS
TableUpdate TableUpdate::set(String attribute, Value value) {}
#elif DOXYGEN_PY
TableUpdate TableUpdate::set(str attribute, Value value) {}
#endif
shcore::Value TableUpdate::set(const shcore::Argument_list &args) {
  // Each method validates the received parameters
  args.ensure_count(2, get_function_name("set").c_str());

  try {
    std::string field = args.string_at(0);

    // Only expression objects are allowed as values
    std::string expr_data;
    if (args[1].type == shcore::Object) {
      shcore::Object_bridge_ref object = args.object_at(1);

      auto expression = std::dynamic_pointer_cast<Expression>(object);

      if (expression) {
        expr_data = expression->get_data();
      } else {
        std::stringstream str;
        str << "TableUpdate.set: Unsupported value received for table update operation on field \""
            << field << "\", received: " << args[1].descr();
        throw shcore::Exception::argument_error(str.str());
      }
    }

    Mysqlx::Crud::UpdateOperation *operation =
        message_.mutable_operation()->Add();
    operation->mutable_source()->set_name(field);
    operation->set_operation(Mysqlx::Crud::UpdateOperation::SET);

    // Calls set for each of the values
    if (!expr_data.empty()) {
      ::mysqlx::Expr_parser parser(expr_data, false, false, &_placeholders);
      operation->set_allocated_value(parser.expr().release());
    } else {
      operation->mutable_value()->set_type(Mysqlx::Expr::Expr::LITERAL);
      operation->mutable_value()->set_allocated_literal(
          convert_value(args[1]).release());
    }

    update_functions(F::set);
  }
  CATCH_AND_TRANSLATE_CRUD_EXCEPTION(get_function_name("set"));

  return Value(std::static_pointer_cast<Object_bridge>(shared_from_this()));
}

//! Sets the search condition to filter the records to be updated from the owner
//! Table.
#if DOXYGEN_CPP
//! \param args should contain an optional string expression to filter the
//! records to be updated.
#else
//! \param searchCondition: An optional expression to filter the records to be
//! updated.
#endif
/**
* Sets the search condition to filter the records to be updated on the owner
* Table.
* if not specified all the records will be updated from the table unless a limit
* is set.
* \return This TableUpdate object.
*
* The searchCondition supports \a [Parameter Binding](param_binding.html).
*
* #### Method Chaining
*
* This function can be invoked only once after:
*
* - set(String attribute, Value value)
*
* After this function invocation, the following functions can be invoked:
*
* - orderBy(List sortExprStr)
* - limit(Integer numberOfRows)
* - bind(String name, Value value)
* - execute()
*
* \sa Usage examples at execute().
*/
#if DOXYGEN_JS
TableUpdate TableUpdate::where(String searchCondition) {}
#elif DOXYGEN_PY
TableUpdate TableUpdate::where(str searchCondition) {}
#endif
shcore::Value TableUpdate::where(const shcore::Argument_list &args) {
  // Each method validates the received parameters
  args.ensure_count(1, get_function_name("where").c_str());

  try {
    message_.set_allocated_criteria(::mysqlx::parser::parse_table_filter(
        args.string_at(0), &_placeholders));

    // Updates the exposed functions
    update_functions(F::where);
  }
  CATCH_AND_TRANSLATE_CRUD_EXCEPTION(get_function_name("where"));

  return Value(std::static_pointer_cast<Object_bridge>(shared_from_this()));
}

//! Sets the order in which the update should be done.
#if DOXYGEN_CPP
//! \param args should contain a list of expression strings defining a sort
//! criteria, the update will be done following the order defined by this
//! criteria.
#else
//! \param sortExprStr: A list of expression strings defining a sort criteria,
//! the update will be done following the order defined by this criteria.
#endif
/**
* \return This TableUpdate object.
*
* The elements of sortExprStr list are strings defining the column name on which
* the sorting will be based in the form of "columnIdentifier [ ASC | DESC ]".
* If no order criteria is specified, ascending will be used by default.
*
* This method is usually used in combination with limit to fix the amount of
* records to be updated.
*
* #### Method Chaining
*
* This function can be invoked only once after:
*
* - set(String attribute, Value value)
* - where(String searchCondition)
*
* After this function invocation, the following functions can be invoked:
*
* - limit(Integer numberOfRows)
* - bind(String name, Value value)
* - execute()
*/
#if DOXYGEN_JS
TableUpdate TableUpdate::orderBy(List sortExprStr) {}
#elif DOXYGEN_PY
TableUpdate TableUpdate::order_by(list sortExprStr) {}
#endif
shcore::Value TableUpdate::order_by(const shcore::Argument_list &args) {
  args.ensure_at_least(1, get_function_name("orderBy").c_str());

  try {
    std::vector<std::string> fields;

    parse_string_list(args, fields);

    if (fields.size() == 0)
      throw shcore::Exception::argument_error(
          "Order criteria can not be empty");

    for (auto &f : fields)
      ::mysqlx::parser::parse_table_sort_column(*message_.mutable_order(), f);

    update_functions(F::orderBy);
  }
  CATCH_AND_TRANSLATE_CRUD_EXCEPTION(get_function_name("orderBy"));

  return Value(std::static_pointer_cast<Object_bridge>(shared_from_this()));
}

//! Sets a limit for the records to be updated.
#if DOXYGEN_CPP
//! \param args should contain the number of records to be updated.
#else
//! \param numberOfRows the number of records to be updated.
#endif
/**
* \return This TableUpdate object.
*
* This method is usually used in combination with sort to fix the amount of
* records to be updated.
*
* #### Method Chaining
*
* This function can be invoked only once after:
*
* - set(String attribute, Value value)
* - where(String searchCondition)
* - orderBy(List sortExprStr)
*
* After this function invocation, the following functions can be invoked:
*
* - bind(String name, Value value)
* - execute()
*
* \sa Usage examples at execute().
*/
#if DOXYGEN_JS
TableUpdate TableUpdate::limit(Integer numberOfRows) {}
#elif DOXYGEN_PY
TableUpdate TableUpdate::limit(int numberOfRows) {}
#endif
shcore::Value TableUpdate::limit(const shcore::Argument_list &args) {
  args.ensure_count(1, get_function_name("limit").c_str());

  try {
    message_.mutable_limit()->set_row_count(args.uint_at(0));

    update_functions(F::limit);
  }
  CATCH_AND_TRANSLATE_CRUD_EXCEPTION(get_function_name("limit"));

  return Value(std::static_pointer_cast<Object_bridge>(shared_from_this()));
}

//! Binds a value to a specific placeholder used on this TableSelect object.
#if DOXYGEN_CPP
//! \param args should contain the next elements:
//! \li The name of the placeholder to which the value will be bound.
//! \li The value to be bound on the placeholder.
#else
//! \param name: The name of the placeholder to which the value will be bound.
//! \param value: The value to be bound on the placeholder.
#endif
/**
* \return This TableUpdate object.
*
* #### Method Chaining
*
* This function can be invoked multiple times right before calling execute:
*
* After this function invocation, the following functions can be invoked:
*
* - bind(String name, Value value)
* - execute()
*
* An error will be raised if the placeholder indicated by name does not exist.
*
* This function must be called once for each used placeohlder or an error will
* be
* raised when the execute method is called.
*
* \sa Usage examples at execute().
*/
#if DOXYGEN_JS
TableUpdate TableUpdate::bind(String name, Value value) {}
#elif DOXYGEN_PY
TableUpdate TableUpdate::bind(str name, Value value) {}
#endif
shcore::Value TableUpdate::bind(const shcore::Argument_list &args) {
  args.ensure_count(2, get_function_name("bind").c_str());

  try {
    bind_value(args.string_at(0), args[1]);

    update_functions(F::bind);
  }
  CATCH_AND_TRANSLATE_CRUD_EXCEPTION(get_function_name("bind"));

  return Value(std::static_pointer_cast<Object_bridge>(shared_from_this()));
}

/**
* Executes the record update with the configured filter and limit.
* \return Result A result object that can be used to retrieve the results of the
* update operation.
*
* #### Method Chaining
*
* This function can be invoked after any other function on this class except
* update().
*/
#if DOXYGEN_JS
/**
*
* #### Examples
* \dontinclude "js_devapi/scripts/mysqlx_table_update.js"
* \skip //@# TableUpdate: simple test
* \until print('All Females:', records.length, '\n');
*/
Result TableUpdate::execute() {}
#elif DOXYGEN_PY
/**
*
* #### Examples
* \dontinclude "py_devapi/scripts/mysqlx_table_update.py"
* \skip #@# TableUpdate: simple test
* \until print 'All Females:', len(records), '\n'
*/
Result TableUpdate::execute() {}
#endif
shcore::Value TableUpdate::execute(const shcore::Argument_list &args) {
  std::unique_ptr<mysqlsh::mysqlx::Result> result;
  args.ensure_count(0, get_function_name("execute").c_str());

  try {
    MySQL_timer timer;
    insert_bound_values(message_.mutable_args());
    timer.start();
    result.reset(new mysqlx::Result(
        safe_exec([this]() { return session()->session()->execute_crud(message_); })));
    timer.end();
    result->set_execution_time(timer.raw_duration());
  }
  CATCH_AND_TRANSLATE_CRUD_EXCEPTION(get_function_name("execute"));

  return result ? shcore::Value::wrap(result.release()) : shcore::Value::Null();
}
