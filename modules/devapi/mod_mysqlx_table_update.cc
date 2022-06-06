/*
 * Copyright (c) 2015, 2022, Oracle and/or its affiliates.
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
#include "modules/devapi/mod_mysqlx_expression.h"
#include "modules/devapi/mod_mysqlx_resultset.h"
#include "modules/devapi/mod_mysqlx_table.h"
#include "modules/devapi/protobuf_bridge.h"
#include "mysqlshdk/include/shellcore/utils_help.h"
#include "scripting/common.h"

using namespace std::placeholders;
using namespace mysqlsh::mysqlx;
using namespace shcore;

REGISTER_HELP_CLASS(TableUpdate, mysqlx);
REGISTER_HELP(TABLEUPDATE_BRIEF, "Operation to add update records in a Table.");
REGISTER_HELP(TABLEUPDATE_DETAIL,
              "A TableUpdate object is used to update rows in a Table, is "
              "created through the <b>update</b> function on the <b>Table</b> "
              "class.");
TableUpdate::TableUpdate(std::shared_ptr<Table> owner)
    : Table_crud_definition(std::static_pointer_cast<DatabaseObject>(owner)) {
  message_.mutable_collection()->set_schema(owner->schema()->name());
  message_.mutable_collection()->set_name(owner->name());
  message_.set_data_model(Mysqlx::Crud::TABLE);

  auto limit_id = F::limit;
  auto bind_id = F::bind;
  // Exposes the methods available for chaining
  add_method("update", std::bind(&TableUpdate::update, this, _1), "data");
  add_method("set", std::bind(&TableUpdate::set, this, _1), "data");
  add_method("where", std::bind(&TableUpdate::where, this, _1), "data");
  add_method("orderBy", std::bind(&TableUpdate::order_by, this, _1), "data");
  add_method("limit", std::bind(&TableUpdate::limit, this, _1, limit_id, false),
             "data");
  add_method("bind", std::bind(&TableUpdate::bind_, this, _1, bind_id), "data");

  // Registers the dynamic function behavior
  register_dynamic_function(F::update, F::set);
  register_dynamic_function(
      F::set, F::where | F::orderBy | F::limit | F::bind | F::execute,
      K_DISABLE_NONE, K_ALLOW_REUSE);
  register_dynamic_function(F::where, K_ENABLE_NONE, F::set);
  register_dynamic_function(F::orderBy, K_ENABLE_NONE, F::set | F::where);
  register_dynamic_function(F::limit, K_ENABLE_NONE,
                            F::set | F::where | F::orderBy);
  register_dynamic_function(F::bind, K_ENABLE_NONE,
                            F::set | F::where | F::orderBy | F::limit,
                            K_ALLOW_REUSE);
  register_dynamic_function(F::execute, F::limit, K_DISABLE_NONE,
                            K_ALLOW_REUSE);

  // Initial function update
  enable_function(F::update);
}

shcore::Value TableUpdate::this_object() {
  return Value(std::static_pointer_cast<Object_bridge>(shared_from_this()));
}

REGISTER_HELP_FUNCTION(update, TableUpdate);
REGISTER_HELP(TABLEUPDATE_UPDATE_BRIEF, "Initializes the update operation.");
REGISTER_HELP(TABLEUPDATE_UPDATE_RETURNS, "@returns This TableUpdate object.");
/**
 * $(TABLEUPDATE_UPDATE_BRIEF)
 *
 * $(TABLEUPDATE_UPDATE_RETURNS)
 *
 * #### Method Chaining
 *
 * After this function invocation, the following function can be invoked:
 */
#if DOXYGEN_JS
/**
 * - set(String attribute, Value value)
 */
#elif DOXYGEN_PY
/**
 * - set(str attribute, Value value)
 */
#endif
/**
 *
 * \sa Usage examples at execute().
 */
//@{
#if DOXYGEN_JS
TableUpdate TableUpdate::update() {}
#elif DOXYGEN_PY
TableUpdate TableUpdate::update() {}
#endif
//@}
shcore::Value TableUpdate::update(const shcore::Argument_list &args) {
  // Each method validates the received parameters
  args.ensure_count(0, get_function_name("update").c_str());

  std::shared_ptr<Table> table(std::static_pointer_cast<Table>(_owner));

  if (table) {
    try {
      // Updates the exposed functions
      update_functions(F::update);
      reset_prepared_statement();
    }
    CATCH_AND_TRANSLATE_CRUD_EXCEPTION(get_function_name("update"));
  }

  return Value(std::static_pointer_cast<Object_bridge>(shared_from_this()));
}

REGISTER_HELP_FUNCTION(set, TableUpdate);
REGISTER_HELP_FUNCTION_TEXT(TABLEUPDATE_SET, R"*(
Adds an update operation.

@param attribute Identifies the column to be updated by this operation.
@param value Defines the value to be set on the indicated column.

@returns This TableUpdate object.

Adds an operation into the update handler to update a column value in the
records that were included on the selection filter and limit.

<b>Using Expressions As Values</b>

If a <b>mysqlx.expr(...)</b> object is defined as a value, it will be evaluated
in the server, the resulting value will be set at the indicated column.
)*");

/**
 * $(TABLEUPDATE_SET_BRIEF)
 *
 * $(TABLEUPDATE_SET)
 *
 * The expression also can be used for \a [Parameter
 * Binding](param_binding.html).
 *
 * #### Method Chaining
 *
 * This function can be invoked multiple times after:
 * - update()
 */
#if DOXYGEN_JS
/**
 * - set(String attribute, Value value)
 */
#elif DOXYGEN_PY
/**
 * - set(str attribute, Value value)
 */
#endif
/**
 *
 * After this function invocation, the following functions can be invoked:
 */
#if DOXYGEN_JS
/**
 * - set(String attribute, Value value)
 * - where(String expression)
 * - orderBy(List sortCriteria),
 *   <a class="el" href="#ae5e292f1f1bc3b75f2a03e81a4e238ea">
 *   orderBy(String sortCriterion[, String sortCriterion, ...])</a>
 * - limit(Integer numberOfRows)
 * - bind(String name, Value value)
 */
#elif DOXYGEN_PY
/**
 * - set(str attribute, Value value)
 * - where(str expression)
 * - order_by(list sortCriteria),
 *   <a class="el" href="#a37542347a7a0810fd201e261e70f99a8">
 *   order_by(str sortCriterion[, str sortCriterion, ...])</a>
 * - limit(int numberOfRows)
 * - bind(str name, Value value)
 */
#endif
/**
 * - execute()
 *
 * \sa Usage examples at execute().
 */
//@{
#if DOXYGEN_JS
TableUpdate TableUpdate::set(String attribute, Value value) {}
#elif DOXYGEN_PY
TableUpdate TableUpdate::set(str attribute, Value value) {}
#endif
//@}
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
        str << "TableUpdate.set: Unsupported value received for table update "
               "operation on field \""
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
    reset_prepared_statement();
  }
  CATCH_AND_TRANSLATE_CRUD_EXCEPTION(get_function_name("set"));

  return Value(std::static_pointer_cast<Object_bridge>(shared_from_this()));
}

REGISTER_HELP_FUNCTION(where, TableUpdate);
REGISTER_HELP(TABLEUPDATE_WHERE_BRIEF,
              "Sets the search condition to filter the records to be updated.");
REGISTER_HELP(
    TABLEUPDATE_WHERE_PARAM,
    "@param expression A condition to filter the records to be updated.");
REGISTER_HELP(TABLEUPDATE_WHERE_RETURNS, "@returns This TableUpdate object.");
REGISTER_HELP(TABLEUPDATE_WHERE_DETAIL,
              "If used, only those rows satisfying the <b>expression</b> will "
              "be updated");
REGISTER_HELP(TABLEUPDATE_WHERE_DETAIL1,
              "The <b>expression</b> supports parameter binding.");
/**
 * $(TABLEUPDATE_WHERE_BRIEF)
 *
 * $(TABLEUPDATE_WHERE_PARAM)
 *
 * $(TABLEUPDATE_WHERE_RETURNS)
 *
 * $(TABLEUPDATE_WHERE_DETAIL)
 *
 * The <b>expression</b> supports \a [Parameter Binding](param_binding.html).
 *
 * #### Method Chaining
 *
 * This function can be invoked only once after:
 */
#if DOXYGEN_JS
/**
 * - set(String attribute, Value value)
 */
#elif DOXYGEN_PY
/**
 * - set(str attribute, Value value)
 */
#endif
/**
 *
 * After this function invocation, the following functions can be invoked:
 */
#if DOXYGEN_JS
/**
 * - orderBy(List sortCriteria),
 *   <a class="el" href="#ae5e292f1f1bc3b75f2a03e81a4e238ea">
 *   orderBy(String sortCriterion[, String sortCriterion, ...])</a>
 * - limit(Integer numberOfRows)
 * - bind(String name, Value value)
 */
#elif DOXYGEN_PY
/**
 * - order_by(list sortCriteria),
 *   <a class="el" href="#a37542347a7a0810fd201e261e70f99a8">
 *   order_by(str sortCriterion[, str sortCriterion, ...])</a>
 * - limit(int numberOfRows)
 * - bind(str name, Value value)
 */
#endif
/**
 * - execute()
 *
 * \sa Usage examples at execute().
 */
//@{
#if DOXYGEN_JS
TableUpdate TableUpdate::where(String expression) {}
#elif DOXYGEN_PY
TableUpdate TableUpdate::where(str expression) {}
#endif
//@}
shcore::Value TableUpdate::where(const shcore::Argument_list &args) {
  // Each method validates the received parameters
  args.ensure_count(1, get_function_name("where").c_str());

  try {
    message_.set_allocated_criteria(::mysqlx::parser::parse_table_filter(
        args.string_at(0), &_placeholders));

    // Updates the exposed functions
    update_functions(F::where);
    reset_prepared_statement();
  }
  CATCH_AND_TRANSLATE_CRUD_EXCEPTION(get_function_name("where"));

  return Value(std::static_pointer_cast<Object_bridge>(shared_from_this()));
}

REGISTER_HELP_FUNCTION(orderBy, TableUpdate);
REGISTER_HELP(TABLEUPDATE_ORDERBY_BRIEF,
              "Sets the order in which the records will be updated.");
REGISTER_HELP(TABLEUPDATE_ORDERBY_SIGNATURE, "(sortCriteriaList)");
REGISTER_HELP(TABLEUPDATE_ORDERBY_SIGNATURE1,
              "(sortCriterion[, sortCriterion, ...])");
REGISTER_HELP(TABLEUPDATE_ORDERBY_RETURNS, "@returns This TableUpdate object.");
REGISTER_HELP(TABLEUPDATE_ORDERBY_DETAIL,
              "If used, the TableUpdate operation will update the records "
              "in the order established by the sort criteria.");
REGISTER_HELP(TABLEUPDATE_ORDERBY_DETAIL1,
              "Every defined sort criterion follows the format:");
REGISTER_HELP(TABLEUPDATE_ORDERBY_DETAIL2, "name [ ASC | DESC ]");
REGISTER_HELP(TABLEUPDATE_ORDERBY_DETAIL3,
              "ASC is used by default if the sort order is not specified.");
/**
 * $(TABLEUPDATE_ORDERBY_BRIEF)
 *
 * $(TABLEUPDATE_ORDERBY_RETURNS)
 *
 * $(TABLEUPDATE_ORDERBY_DETAIL)
 *
 * $(TABLEUPDATE_ORDERBY_DETAIL1)
 *
 * $(TABLEUPDATE_ORDERBY_DETAIL2)
 *
 * $(TABLEUPDATE_ORDERBY_DETAIL3)
 *
 * #### Method Chaining
 *
 * This function can be invoked only once after:
 */
#if DOXYGEN_JS
/**
 * - set(String attribute, Value value)
 * - where(String expression)
 */
#elif DOXYGEN_PY
/**
 * - set(str attribute, Value value)
 * - where(str expression)
 */
#endif
/**
 *
 * After this function invocation, the following functions can be invoked:
 */
#if DOXYGEN_JS
/**
 * - limit(Integer numberOfRows)
 * - bind(String name, Value value)
 */
#elif DOXYGEN_PY
/**
 * - limit(int numberOfRows)
 * - bind(str name, Value value)
 */
#endif
/**
 * - execute()
 *
 * \sa Usage examples at execute().
 */
//@{
#if DOXYGEN_JS
TableUpdate TableUpdate::orderBy(List sortCriteria) {}
TableUpdate TableUpdate::orderBy(
    String sortCriterion[, String sortCriterion, ...]) {}
#elif DOXYGEN_PY
TableUpdate TableUpdate::order_by(list sortCriteria) {}
TableUpdate TableUpdate::order_by(str sortCriterion[, str sortCriterion, ...]) {
}
#endif
//@}
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
    reset_prepared_statement();
  }
  CATCH_AND_TRANSLATE_CRUD_EXCEPTION(get_function_name("orderBy"));

  return Value(std::static_pointer_cast<Object_bridge>(shared_from_this()));
}

REGISTER_HELP_FUNCTION(limit, TableUpdate);
REGISTER_HELP(
    TABLEUPDATE_LIMIT_BRIEF,
    "Sets the maximum number of rows to be updated by the operation.");
REGISTER_HELP(TABLEUPDATE_LIMIT_PARAM,
              "@param numberOfRows The maximum number of rows to be updated.");
REGISTER_HELP(TABLEUPDATE_LIMIT_RETURNS, "@returns This TableUpdate object.");
REGISTER_HELP(
    TABLEUPDATE_LIMIT_DETAIL,
    "If used, the operation will update only <b>numberOfRows</b> rows.");
REGISTER_HELP(TABLEUPDATE_LIMIT_DETAIL1, "${LIMIT_EXECUTION_MODE}");
/**
 * $(TABLEUPDATE_LIMIT_BRIEF)
 *
 * $(TABLEUPDATE_LIMIT_PARAM)
 *
 * $(TABLEUPDATE_LIMIT_RETURNS)
 *
 * $(TABLEUPDATE_LIMIT_DETAIL)
 *
 * #### Method Chaining
 *
 * This function can be invoked only once after:
 */
#if DOXYGEN_JS
/**
 * - set(String attribute, Value value)
 * - where(String expression)
 * - orderBy(List sortCriteria),
 *   <a class="el" href="#ae5e292f1f1bc3b75f2a03e81a4e238ea">
 *   orderBy(String sortCriterion[, String sortCriterion, ...])</a>
 */
#elif DOXYGEN_PY
/**
 * - set(str attribute, Value value)
 * - where(str expression)
 * - order_by(list sortCriteria),
 *   <a class="el" href="#a37542347a7a0810fd201e261e70f99a8">
 *   order_by(str sortCriterion[, str sortCriterion, ...])</a>
 */
#endif
/**
 *
 * $(LIMIT_EXECUTION_MODE)
 *
 * After this function invocation, the following functions can be invoked:
 */
#if DOXYGEN_JS
/**
 * - bind(String name, Value value)
 */
#elif DOXYGEN_PY
/**
 * - bind(str name, Value value)
 */
#endif
/**
 * - execute()
 *
 * \sa Usage examples at execute().
 */
//@{
#if DOXYGEN_JS
TableUpdate TableUpdate::limit(Integer numberOfRows) {}
#elif DOXYGEN_PY
TableUpdate TableUpdate::limit(int numberOfRows) {}
#endif
//@}

REGISTER_HELP_FUNCTION(bind, TableUpdate);
REGISTER_HELP(
    TABLEUPDATE_BIND_BRIEF,
    "Binds a value to a specific placeholder used on this operation.");
REGISTER_HELP(TABLEUPDATE_BIND_PARAM,
              "@param name The name of the placeholder to which the value will "
              "be bound.");
REGISTER_HELP(TABLEUPDATE_BIND_PARAM1,
              "@param value The value to be bound on the placeholder.");
REGISTER_HELP(TABLEUPDATE_BIND_RETURNS, "@returns This TableUpdate object.");
REGISTER_HELP(
    TABLEUPDATE_BIND_DETAIL,
    "Binds the given value to the placeholder with the specified name.");
REGISTER_HELP(TABLEUPDATE_BIND_DETAIL1,
              "An error will be raised if the placeholder indicated by name "
              "does not exist.");
REGISTER_HELP(TABLEUPDATE_BIND_DETAIL2,
              "This function must be called once for each used placeholder or "
              "an error will be raised when the execute method is called.");

/**
 * $(TABLEUPDATE_BIND_BRIEF)
 *
 * $(TABLEUPDATE_BIND_PARAM)
 * $(TABLEUPDATE_BIND_PARAM1)
 *
 * $(TABLEUPDATE_BIND_RETURNS)
 *
 * $(TABLEUPDATE_BIND_DETAIL)
 *
 * $(TABLEUPDATE_BIND_DETAIL1)
 *
 * $(TABLEUPDATE_BIND_DETAIL2)
 *
 * #### Method Chaining
 *
 * This function can be invoked multiple times right before calling execute().
 *
 * After this function invocation, the following functions can be invoked:
 */
#if DOXYGEN_JS
/**
 * - bind(String name, Value value)
 */
#elif DOXYGEN_PY
/**
 * - bind(str name, Value value)
 */
#endif
/**
 * - execute()
 *
 * \sa Usage examples at execute().
 */
//@{
#if DOXYGEN_JS
TableUpdate TableUpdate::bind(String name, Value value) {}
#elif DOXYGEN_PY
TableUpdate TableUpdate::bind(str name, Value value) {}
#endif
//@}

REGISTER_HELP_FUNCTION(execute, TableUpdate);
REGISTER_HELP(TABLEUPDATE_EXECUTE_BRIEF,
              "Executes the update operation with all the configured options.");
REGISTER_HELP(TABLEUPDATE_EXECUTE_RETURNS, "@returns A Result object.");
/**
 * $(TABLEDELETE_EXECUTE_BRIEF)
 *
 * $(TABLEDELETE_EXECUTE_RETURNS)
 *
 * #### Method Chaining
 *
 * This function can be invoked after any other function on this class except
 * update().
 *
 * ### Examples
 */
//@{
#if DOXYGEN_JS
/**
 * #### Updating a single field in a record
 * \snippet mysqlx_table_update.js TableUpdate: simple test
 *
 * #### Updating a single field using expressions
 * \snippet mysqlx_table_update.js TableUpdate: expression
 *
 * #### Updating a single field using expressions and parameter binding
 * \snippet mysqlx_table_update.js TableUpdate: limits
 *
 * #### Updating a view
 * \snippet mysqlx_table_update.js TableUpdate: view
 */
Result TableUpdate::execute() {}
#elif DOXYGEN_PY
/**
 * #### Updating a single field in a record
 * \snippet mysqlx_table_update.py TableUpdate: simple test
 *
 * #### Updating a single field using expressions
 * \snippet mysqlx_table_update.py TableUpdate: expression
 *
 * #### Updating a single field using expressions and parameter binding
 * \snippet mysqlx_table_update.py TableUpdate: limits
 *
 * #### Updating a view
 * \snippet mysqlx_table_update.py TableUpdate: view
 */
Result TableUpdate::execute() {}
#endif
//@}
shcore::Value TableUpdate::execute(const shcore::Argument_list &args) {
  args.ensure_count(0, get_function_name("execute").c_str());

  std::shared_ptr<mysqlsh::mysqlx::Result> result;
  try {
    result = std::make_shared<mysqlx::Result>(safe_exec([this]() {
      update_limits();
      insert_bound_values(message_.mutable_args());
      return session()->session()->execute_crud(message_);
    }));

    update_functions(F::execute);
  }
  CATCH_AND_TRANSLATE_CRUD_EXCEPTION(get_function_name("execute"));

  return result ? shcore::Value::wrap(std::move(result))
                : shcore::Value::Null();
}

void TableUpdate::set_prepared_stmt() {
  m_prep_stmt.mutable_stmt()->set_type(
      Mysqlx::Prepare::Prepare_OneOfMessage_Type_UPDATE);
  message_.clear_args();
  update_limits();
  *m_prep_stmt.mutable_stmt()->mutable_update() = message_;
}
