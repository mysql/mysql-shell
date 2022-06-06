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
#include "modules/devapi/mod_mysqlx_table_delete.h"
#include <memory>
#include <string>
#include <vector>
#include "modules/devapi/mod_mysqlx_resultset.h"
#include "modules/devapi/mod_mysqlx_table.h"
#include "mysqlshdk/include/shellcore/utils_help.h"
#include "scripting/common.h"

using namespace std::placeholders;
using namespace mysqlsh::mysqlx;
using namespace shcore;

REGISTER_HELP_CLASS(TableDelete, mysqlx);
REGISTER_HELP(TABLEDELETE_BRIEF, "Operation to delete data from a table.");
REGISTER_HELP(TABLEDELETE_DETAIL,
              "A TableDelete represents an operation to remove records from a "
              "Table, it is created through the <b>delete</b> function on the "
              "<b>Table</b> class.");
TableDelete::TableDelete(std::shared_ptr<Table> owner)
    : Table_crud_definition(std::static_pointer_cast<DatabaseObject>(owner)) {
  message_.mutable_collection()->set_schema(owner->schema()->name());
  message_.mutable_collection()->set_name(owner->name());
  message_.set_data_model(Mysqlx::Crud::TABLE);
  auto limit_id = F::limit;
  auto bind_id = F::bind;
  // Exposes the methods available for chaining
  add_method("delete", std::bind(&TableDelete::remove, this, _1), "data");
  add_method("where", std::bind(&TableDelete::where, this, _1), "data");
  add_method("orderBy", std::bind(&TableDelete::order_by, this, _1), "data");
  add_method("limit", std::bind(&TableDelete::limit, this, _1, limit_id, false),
             "data");
  add_method("bind", std::bind(&TableDelete::bind_, this, _1, bind_id), "data");

  // Registers the dynamic function behavior
  register_dynamic_function(F::delete_,
                            F::where | F::orderBy | F::limit | F::execute);
  register_dynamic_function(F::where, F::bind);
  register_dynamic_function(F::orderBy, F::bind, F::where);
  register_dynamic_function(F::limit, F::bind, F::where | F::orderBy);
  register_dynamic_function(F::bind, K_ENABLE_NONE,
                            F::where | F::orderBy | F::limit, K_ALLOW_REUSE);
  register_dynamic_function(F::execute, F::limit, K_DISABLE_NONE,
                            K_ALLOW_REUSE);

  // Initial function update
  update_functions(F::delete_);
}

shcore::Value TableDelete::this_object() {
  return Value(std::static_pointer_cast<Object_bridge>(shared_from_this()));
}

REGISTER_HELP_FUNCTION(delete, TableDelete);
REGISTER_HELP(TABLEDELETE_DELETE_BRIEF, "Initializes the deletion operation.");
REGISTER_HELP(TABLEDELETE_DELETE_RETURNS, "@returns This TableDelete object.");
REGISTER_HELP(
    TABLEDELETE_DELETE_DETAIL,
    "This function is called automatically when Table.delete() is called.");
REGISTER_HELP(TABLEDELETE_DELETE_DETAIL1,
              "The actual deletion of the records will occur only when the "
              "execute() method is called.");
/**
 * $(TABLEDELETE_DELETE_BRIEF)
 *
 * $(TABLEDELETE_DELETE_RETURNS)
 *
 * $(TABLEDELETE_DELETE_DETAIL)
 *
 * $(TABLEDELETE_DELETE_DETAIL1)
 *
 * #### Method Chaining
 *
 * After this function invocation, the following functions can be invoked:
 */
#if DOXYGEN_JS
/**
 * - where(String expression)
 * - orderBy(List sortCriteria),
 *   <a class="el" href="#ab82faffbf73dff990eec0c00beebb337">
 *   orderBy(String sortCriterion[, String sortCriterion, ...])</a>
 * - limit(Integer numberOfRows)
 */
#elif DOXYGEN_PY
/**
 * - where(str expression)
 * - order_by(list sortCriteria),
 *   <a class="el" href="#afff2c95e892cbb197b350e94750803cf">
 *   order_by(str sortCriterion[, str sortCriterion, ...])</a>
 * - limit(int numberOfRows)
 */
#endif
/**
 * - execute()
 *
 * \sa Usage examples at execute().
 */
//@{
#if DOXYGEN_JS
TableDelete TableDelete::delete () {}
#elif DOXYGEN_PY
TableDelete TableDelete::delete () {}
#endif
//@}
shcore::Value TableDelete::remove(const shcore::Argument_list &args) {
  // Each method validates the received parameters
  args.ensure_count(0, get_function_name("delete").c_str());

  std::shared_ptr<Table> table(std::static_pointer_cast<Table>(_owner));

  if (table) {
    try {
      // Updates the exposed functions
      update_functions(F::delete_);
      reset_prepared_statement();
    }
    CATCH_AND_TRANSLATE_CRUD_EXCEPTION(get_function_name("delete"));
  }

  return Value(std::static_pointer_cast<Object_bridge>(shared_from_this()));
}

REGISTER_HELP_FUNCTION(where, TableDelete);
REGISTER_HELP(TABLEDELETE_WHERE_BRIEF,
              "Sets the search condition to filter the records to be deleted "
              "from the Table.");
REGISTER_HELP(
    TABLEDELETE_WHERE_PARAM,
    "@param expression A condition to filter the records to be deleted.");
REGISTER_HELP(TABLEDELETE_WHERE_RETURNS, "@returns This TableDelete object.");
REGISTER_HELP(TABLEDELETE_WHERE_DETAIL,
              "If used, only those rows satisfying the <b>expression</b> will "
              "be deleted");
REGISTER_HELP(TABLEDELETE_WHERE_DETAIL1,
              "The <b>expression</b> supports parameter binding.");

/**
 * $(TABLEDELETE_WHERE_BRIEF)
 *
 * $(TABLEDELETE_WHERE_PARAM)
 *
 * $(TABLEDELETE_WHERE_RETURNS)
 *
 * $(TABLEDELETE_WHERE_DETAIL)
 *
 * The <b>expression</b> supports \a [Parameter Binding](param_binding.html).
 *
 * #### Method Chaining
 *
 * This function can be invoked only once after:
 *
 * - delete()
 *
 * After this function invocation, the following functions can be invoked:
 */
#if DOXYGEN_JS
/**
 * - orderBy(List sortCriteria),
 *   <a class="el" href="#ab82faffbf73dff990eec0c00beebb337">
 *   orderBy(String sortCriterion[, String sortCriterion, ...])</a>
 * - limit(Integer numberOfRows)
 * - bind(String name, Value value)
 */
#elif DOXYGEN_PY
/**
 * - order_by(list sortCriteria),
 *   <a class="el" href="#afff2c95e892cbb197b350e94750803cf">
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
TableDelete TableDelete::where(String expression) {}
#elif DOXYGEN_PY
TableDelete TableDelete::where(str expression) {}
#endif
//@}
shcore::Value TableDelete::where(const shcore::Argument_list &args) {
  // Each method validates the received parameters
  args.ensure_count(1, get_function_name("where").c_str());

  std::shared_ptr<Table> table(std::static_pointer_cast<Table>(_owner));

  if (table) {
    try {
      message_.set_allocated_criteria(::mysqlx::parser::parse_table_filter(
          args.string_at(0), &_placeholders));

      // Updates the exposed functions
      update_functions(F::where);
      reset_prepared_statement();
    }
    CATCH_AND_TRANSLATE_CRUD_EXCEPTION(get_function_name("where"));
  }

  return Value(std::static_pointer_cast<Object_bridge>(shared_from_this()));
}

REGISTER_HELP_FUNCTION(orderBy, TableDelete);
REGISTER_HELP(TABLEDELETE_ORDERBY_BRIEF,
              "Sets the order in which the records will be deleted.");
REGISTER_HELP(TABLEDELETE_ORDERBY_SIGNATURE, "(sortCriteriaList)");
REGISTER_HELP(TABLEDELETE_ORDERBY_SIGNATURE1,
              "(sortCriterion[, sortCriterion, ...])");
REGISTER_HELP(TABLEDELETE_ORDERBY_RETURNS, "@returns This TableDelete object.");
REGISTER_HELP(TABLEDELETE_ORDERBY_DETAIL,
              "If used, the TableDelete operation will delete the records "
              "in the order established by the sort criteria.");
REGISTER_HELP(TABLEDELETE_ORDERBY_DETAIL1,
              "Every defined sort criterion follows the format:");
REGISTER_HELP(TABLEDELETE_ORDERBY_DETAIL2, "name [ ASC | DESC ]");
REGISTER_HELP(TABLEDELETE_ORDERBY_DETAIL3,
              "ASC is used by default if the sort order is not specified.");
/**
 * $(TABLEDELETE_ORDERBY_BRIEF)
 *
 * $(TABLEDELETE_ORDERBY_RETURNS)
 *
 * $(TABLEDELETE_ORDERBY_DETAIL)
 *
 * $(TABLEDELETE_ORDERBY_DETAIL1)
 *
 * $(TABLEDELETE_ORDERBY_DETAIL2)
 *
 * $(TABLEDELETE_ORDERBY_DETAIL3)
 *
 * #### Method Chaining
 *
 * This function can be invoked only once after:
 *
 * - delete()
 */
#if DOXYGEN_JS
/**
 * - where(String expression)
 */
#elif DOXYGEN_PY
/**
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
TableDelete TableDelete::orderBy(List sortCriteria) {}
TableDelete TableDelete::orderBy(
    String sortCriterion[, String sortCriterion, ...]) {}
#elif DOXYGEN_PY
TableDelete TableDelete::order_by(list sortCriteria) {}
TableDelete TableDelete::order_by(str sortCriterion[, str sortCriterion, ...]) {
}
#endif
//@}
shcore::Value TableDelete::order_by(const shcore::Argument_list &args) {
  args.ensure_at_least(1, get_function_name("orderBy").c_str());

  try {
    std::vector<std::string> fields;

    parse_string_list(args, fields);

    if (fields.size() == 0)
      throw shcore::Exception::argument_error(
          "Order criteria can not be empty");

    for (auto &field : fields)
      ::mysqlx::parser::parse_table_sort_column(*message_.mutable_order(),
                                                field);

    update_functions(F::orderBy);
    reset_prepared_statement();
  }
  CATCH_AND_TRANSLATE_CRUD_EXCEPTION(get_function_name("orderBy"));

  return Value(std::static_pointer_cast<Object_bridge>(shared_from_this()));
}

REGISTER_HELP_FUNCTION(limit, TableDelete);
REGISTER_HELP(
    TABLEDELETE_LIMIT_BRIEF,
    "Sets the maximum number of rows to be deleted by the operation.");
REGISTER_HELP(TABLEDELETE_LIMIT_PARAM,
              "@param numberOfRows The maximum number of rows to be deleted.");
REGISTER_HELP(TABLEDELETE_LIMIT_RETURNS, "@returns This TableDelete object.");
REGISTER_HELP(
    TABLEDELETE_LIMIT_DETAIL,
    "If used, the operation will delete only <b>numberOfRows</b> rows.");
REGISTER_HELP(TABLEDELETE_LIMIT_DETAIL1, "${LIMIT_EXECUTION_MODE}");
/**
 * $(TABLEDELETE_LIMIT_BRIEF)
 *
 * $(TABLEDELETE_LIMIT_PARAM)
 *
 * $(TABLEDELETE_LIMIT_RETURNS)
 *
 * $(TABLEDELETE_LIMIT_DETAIL)
 *
 * #### Method Chaining
 *
 * This function can be invoked only once after:
 *
 * - delete()
 */
#if DOXYGEN_JS
/**
 * - where(String expression)
 * - orderBy(List sortCriteria),
 *   <a class="el" href="#ab82faffbf73dff990eec0c00beebb337">
 *   orderBy(String sortCriterion[, String sortCriterion, ...])</a>
 */
#elif DOXYGEN_PY
/**
 * - where(str expression)
 * - order_by(list sortCriteria),
 *   <a class="el" href="#afff2c95e892cbb197b350e94750803cf">
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
TableDelete TableDelete::limit(Integer numberOfRows) {}
#elif DOXYGEN_PY
TableDelete TableDelete::limit(int numberOfRows) {}
#endif
//@}

REGISTER_HELP_FUNCTION(bind, TableDelete);
REGISTER_HELP(
    TABLEDELETE_BIND_BRIEF,
    "Binds a value to a specific placeholder used on this operation.");
REGISTER_HELP(TABLEDELETE_BIND_PARAM,
              "@param name The name of the placeholder to which the value will "
              "be bound.");
REGISTER_HELP(TABLEDELETE_BIND_PARAM1,
              "@param value The value to be bound on the placeholder.");
REGISTER_HELP(TABLEDELETE_BIND_RETURNS, "@returns This TableDelete object.");
REGISTER_HELP(
    TABLEDELETE_BIND_DETAIL,
    "Binds the given value to the placeholder with the specified name.");
REGISTER_HELP(TABLEDELETE_BIND_DETAIL1,
              "An error will be raised if the placeholder indicated by name "
              "does not exist.");
REGISTER_HELP(TABLEDELETE_BIND_DETAIL2,
              "This function must be called once for each used placeholder or "
              "an error will be raised when the execute method is called.");

/**
 * $(TABLEDELETE_BIND_BRIEF)
 *
 * $(TABLEDELETE_BIND_PARAM)
 * $(TABLEDELETE_BIND_PARAM1)
 *
 * $(TABLEDELETE_BIND_RETURNS)
 *
 * $(TABLEDELETE_BIND_DETAIL)
 *
 * $(TABLEDELETE_BIND_DETAIL1)
 *
 * $(TABLEDELETE_BIND_DETAIL2)
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
TableDelete TableDelete::bind(String name, Value value) {}
#elif DOXYGEN_PY
TableDelete TableDelete::bind(str name, Value value) {}
#endif
//@}

REGISTER_HELP_FUNCTION(execute, TableDelete);
REGISTER_HELP(TABLEDELETE_EXECUTE_BRIEF,
              "Executes the delete operation with all the configured options.");
REGISTER_HELP(TABLEDELETE_EXECUTE_RETURNS, "@returns A Result object.");
/**
 * $(TABLEDELETE_EXECUTE_BRIEF)
 *
 * $(TABLEDELETE_EXECUTE_RETURNS)
 *
 * #### Method Chaining
 *
 * This function can be invoked after any other function on this class.
 *
 * ### Examples
 */
//@{
#if DOXYGEN_JS
/**
 * #### Deleting records with a condition
 * \snippet mysqlx_table_delete.js TableDelete: delete under condition
 *
 * #### Deleting records with a condition and parameter binding
 * \snippet mysqlx_table_delete.js TableDelete: delete with binding
 *
 * #### Deleting all records using a view
 * \snippet mysqlx_table_delete.js TableDelete: full delete
 *
 * #### Deleting records with a limit
 * \snippet mysqlx_table_delete.js TableDelete: with limit
 */
Result TableDelete::execute() {}
#elif DOXYGEN_PY
/**
 * #### Deleting records with a condition
 * \snippet mysqlx_table_delete.py TableDelete: delete under condition
 *
 * #### Deleting records with a condition and parameter binding
 * \snippet mysqlx_table_delete.py TableDelete: delete with binding
 *
 * #### Deleting all records using a view
 * \snippet mysqlx_table_delete.py TableDelete: full delete
 *
 * #### Deleting records with a limit
 * \snippet mysqlx_table_delete.py TableDelete: with limit
 */
Result TableDelete::execute() {}
#endif
//@}
shcore::Value TableDelete::execute(const shcore::Argument_list &args) {
  args.ensure_count(0, get_function_name("execute").c_str());

  std::shared_ptr<mysqlsh::mysqlx::Result> result;
  try {
    result = std::make_shared<mysqlsh::mysqlx::Result>(safe_exec([this]() {
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

void TableDelete::set_prepared_stmt() {
  m_prep_stmt.mutable_stmt()->set_type(
      Mysqlx::Prepare::Prepare_OneOfMessage_Type_DELETE);
  message_.clear_args();
  update_limits();
  *m_prep_stmt.mutable_stmt()->mutable_delete_() = message_;
}
