/*
 * Copyright (c) 2015, 2018, Oracle and/or its affiliates. All rights reserved.
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
#include "modules/devapi/mod_mysqlx_table_insert.h"
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

REGISTER_HELP_CLASS(TableInsert, mysqlx);
REGISTER_HELP(TABLEINSERT_BRIEF, "Operation to insert data into a table.");
REGISTER_HELP(TABLEINSERT_DETAIL,
              "A TableInsert object is created through the <b>insert</b> "
              "function on the <b>Table</b> class.");
/*
 * Class constructor represents the call to the first method on the
 * call chain, on this case insert.
 * It will reveive not only the parameter documented on the insert function
 * but also other initialization data for the object:
 * - The connection represents the intermediate class in charge of actually
 *   creating the message.
 * - Message information that is not provided through the different functions
 */
TableInsert::TableInsert(std::shared_ptr<Table> owner)
    : Table_crud_definition(std::static_pointer_cast<DatabaseObject>(owner)) {
  message_.mutable_collection()->set_schema(owner->schema()->name());
  message_.mutable_collection()->set_name(owner->name());
  message_.set_data_model(Mysqlx::Crud::TABLE);
  // The values function should not be enabled if values were already given
  add_method("insert", std::bind(&TableInsert::insert, this, _1), "data");
  add_method("values", std::bind(&TableInsert::values, this, _1), "data");

  // Registers the dynamic function behavior
  register_dynamic_function(F::insert, F::_empty);
  register_dynamic_function(F::values, F::insert | F::insertFields | F::values);
  register_dynamic_function(F::execute,
                            F::insertFieldsAndValues | F::values | F::bind);
  register_dynamic_function(F::__shell_hook__,
                            F::insertFieldsAndValues | F::values | F::bind);

  // Initial function update
  update_functions(F::_empty);
}

REGISTER_HELP_FUNCTION(insert, TableInsert);
REGISTER_HELP(TABLEINSERT_INSERT_BRIEF, "Initializes the insertion operation.");
REGISTER_HELP(TABLEINSERT_INSERT_SIGNATURE, "()");
REGISTER_HELP(TABLEINSERT_INSERT_SIGNATURE1, "(columnList)");
REGISTER_HELP(TABLEINSERT_INSERT_SIGNATURE2, "(column[, column, ...])");
REGISTER_HELP(TABLEINSERT_INSERT_SIGNATURE3,
              "({column:value[, column:value, ...]})");
REGISTER_HELP(TABLEINSERT_INSERT_RETURNS, "@returns This TableInsert object.");
REGISTER_HELP(TABLEINSERT_INSERT_DETAIL,
              "An insert operation requires the values to be inserted, "
              "optionally the target colums can be defined.");
REGISTER_HELP(TABLEINSERT_INSERT_DETAIL1,
              "If this function is called without any parameter, no column "
              "names will be defined yet.");
REGISTER_HELP(TABLEINSERT_INSERT_DETAIL2,
              "The column definition can be done by three ways:");
REGISTER_HELP(TABLEINSERT_INSERT_DETAIL3,
              "@li Passing to the function call an array with the columns "
              "names that will be used in the insertion.");
REGISTER_HELP(
    TABLEINSERT_INSERT_DETAIL4,
    "@li Passing multiple parameters, each of them being a column name.");
REGISTER_HELP(TABLEINSERT_INSERT_DETAIL5,
              "@li Passing a JSON document, using the column names as the "
              "document keys.");
REGISTER_HELP(TABLEINSERT_INSERT_DETAIL6,
              "If the columns are defined using either an array or multiple "
              "parameters, the values must match the defined column names in "
              "order and data type.");
REGISTER_HELP(TABLEINSERT_INSERT_DETAIL7,
              "If a JSON document was used, the operation is ready to be "
              "completed and it will insert the associated value into the "
              "corresponding column.");
REGISTER_HELP(TABLEINSERT_INSERT_DETAIL8,
              "If no columns are defined, insertion will suceed if the "
              "provided values match the database columns in number and data "
              "types.");
#if DOXYGEN_CPP
/**
 * $(TABLEINSERT_INSERT_BRIEF)
 *
 * $(TABLEINSERT_INSERT_BRIEF)
 * \param args contains the initialization data, possible values include:
 * \li An array of strings identifying the columns to be inserted. Sinsequent
 * calls to values must contain a value for each column defined here.
 * \li If no column information is set at all, it is the database who will
 * validate if the provided values can be inserted or not.
 * \return This TableInsert object.
 *
 * This function is called automatically when Table.insert() is called.
 *
 * #### Method Chaining
 *
 * After this function invocation, the following functions can be invoked:
 *
 * - values(Value value1, Value value2, ...)
 * - execute().
 *
 * \sa Usage examples at execute().
 */
#else
/**
 * Initializes the record insertion handler.
 * \return This TableInsert object.
 *
 * This function is called automatically when Table.insert() is called.
 *
 * #### Method Chaining
 *
 * After this function invocation, the following functions can be invoked:
 *
 * - values(Value value1, Value value2, ...)
 * - execute().
 *
 * \sa Usage examples at execute().
 */
#if DOXYGEN_JS
TableInsert TableInsert::insert() {}
#elif DOXYGEN_PY
TableInsert TableInsert::insert() {}
#endif

/**
 * Initializes the record insertion handler with the received column list.
 * \return This TableInsert object.
 *
 * This function is called automatically when Table.insert(List columns) is
 * called.
 *
 * #### Method Chaining
 *
 * After this function invocation, the following functions can be invoked:
 *
 * - values(Value value1, Value value2, ...)
 * - execute().
 *
 * \sa Usage examples at execute().
 */
#if DOXYGEN_JS
TableInsert TableInsert::insert(List columns) {}
#elif DOXYGEN_PY
TableInsert TableInsert::insert(list columns) {}
#endif

/**
 * Initializes the record insertion handler with the received column list.
 * \param col1 The first column name.
 * \param col2 The second column name.
 * \return This TableInsert object.
 *
 * This function is called automatically when Table.insert(String col1, String
 * col2, ...) is called.
 *
 * A string parameter should be specified for each column to be included on the
 * insertion process.
 *
 * #### Method Chaining
 *
 * After this function invocation, the following functions can be invoked:
 *
 * - values(Value value1, Value value2, ...)
 * - execute().
 *
 * \sa Usage examples at execute().
 */
#if DOXYGEN_JS
TableInsert TableInsert::insert(String col1, String col2, ...) {}
#elif DOXYGEN_PY
TableInsert TableInsert::insert(str col1, str col2, ...) {}
#endif
#endif
shcore::Value TableInsert::insert(const shcore::Argument_list &args) {
  std::shared_ptr<Table> table(std::static_pointer_cast<Table>(_owner));

  std::string path;

  if (table) {
    try {
      if (args.size()) {
        shcore::Value::Map_type_ref sh_columns_and_values;
        REGISTER_HELP(TABLEINSERT_VALUES_RETURNS,
                      "@returns This TableInsert object.");

        std::vector<std::string> columns;

        // An array with the fields was received as parameter
        if (args.size() == 1) {
          if (args[0].type == Array || args[0].type == String) {
            path = "Fields";

            parse_string_list(args, columns);

            for (auto &column : columns)
              message_.mutable_projection()->Add()->set_name(column);
          }
          // A map with fields and values was received as parameter
          else if (args[0].type == Map) {
            path = "FieldsAndValues";

            Mysqlx::Crud::Insert_TypedRow *row = message_.mutable_row()->Add();
            for (auto &iter : *args[0].as_map()) {
              message_.mutable_projection()->Add()->set_name(iter.first);
              encode_expression_value(row->mutable_field()->Add(), iter.second);
            }
          } else {
            throw shcore::Exception::type_error(
                "Argument #1 is expected to be either string, a list of "
                "strings or a map with fields and values");
          }
        } else {
          path = "Fields";

          parse_string_list(args, columns);
          for (auto &column : columns)
            message_.mutable_projection()->Add()->set_name(column);
        }
      }
    }
    CATCH_AND_TRANSLATE_CRUD_EXCEPTION("TableInsert.insert");
  }

  // Updates the exposed functions
  if (path == "") {
    update_functions(F::insert);
  } else if (path == "Fields") {
    update_functions(F::insertFields);
  } else if (path == "FieldsAndValues") {
    update_functions(F::insertFieldsAndValues);
  }

  return Value(std::static_pointer_cast<Object_bridge>(shared_from_this()));
}

REGISTER_HELP_FUNCTION(values, TableInsert);
REGISTER_HELP(TABLEINSERT_VALUES_BRIEF,
              "Adds a new row to the insert operation with the given values.");
REGISTER_HELP(TABLEINSERT_VALUES_SIGNATURE, "(value[, value, ...])");
REGISTER_HELP(TABLEINSERT_VALUES_RETURNS, "@returns This TableInsert object.");
REGISTER_HELP(
    TABLEINSERT_VALUES_DETAIL,
    "Each parameter represents the value for a column in the target table.");
REGISTER_HELP(TABLEINSERT_VALUES_DETAIL1,
              "If the columns were defined on the <b>insert</b> function, the "
              "number of values on this function must match the number of "
              "defined columns.");
REGISTER_HELP(TABLEINSERT_VALUES_DETAIL2,
              "If no column was defined, the number of parameters must match "
              "the number of columns on the target Table.");
REGISTER_HELP(TABLEINSERT_VALUES_DETAIL3,
              "This function is not available when the <b>insert</b> is called "
              "passing a JSON object with columns and values.");
REGISTER_HELP(TABLEINSERT_VALUES_DETAIL4, "<b>Using Expressions As Values</b>");
REGISTER_HELP(TABLEINSERT_VALUES_DETAIL5,
              "If a <b>mysqlx.expr(...)</b> object is defined as a value, it "
              "will be evaluated in the server, the resulting value will be "
              "inserted into the record.");

/**
 * $(TABLEINSERT_VALUES_BRIEF)
 *
 * $(TABLEINSERT_VALUES_RETURNS)
 *
 * $(TABLEINSERT_VALUES_DETAIL)
 *
 * $(TABLEINSERT_VALUES_DETAIL1)
 *
 * $(TABLEINSERT_VALUES_DETAIL2)
 *
 * $(TABLEINSERT_VALUES_DETAIL3)
 *
 * #### Using Expressions for Values
 *
 * $(TABLEINSERT_VALUES_DETAIL5)
 *
 * #### Method Chaining
 *
 * This function can be invoked multiple times after:
 * - insert()
 * - insert(List columns)
 * - insert(String col1, String col2, ...)
 * - values(Value value1, Value value2, ...)
 *
 * After this function invocation, the following functions can be invoked:
 *
 * - execute().
 *
 * \sa Usage examples at execute().
 */
#if DOXYGEN_JS
TableInsert TableInsert::values(Value value1, Value value2, ...) {}
#elif DOXYGEN_PY
TableInsert TableInsert::values(Value value1, Value value2, ...) {}
#endif
shcore::Value TableInsert::values(const shcore::Argument_list &args) {
  // Each method validates the received parameters
  args.ensure_at_least(1, get_function_name("values").c_str());

  try {
    Mysqlx::Crud::Insert_TypedRow *row = message_.mutable_row()->Add();
    for (size_t index = 0; index < args.size(); index++) {
      encode_expression_value(row->mutable_field()->Add(), args[index]);
    }
    // Updates the exposed functions
    update_functions(F::values);
  }
  CATCH_AND_TRANSLATE_CRUD_EXCEPTION(get_function_name("values"));

  // Returns the same object
  return Value(std::static_pointer_cast<Object_bridge>(shared_from_this()));
}

REGISTER_HELP_FUNCTION(execute, TableInsert);
REGISTER_HELP(TABLEINSERT_EXECUTE_BRIEF, "Executes the insert operation.");
REGISTER_HELP(TABLEINSERT_EXECUTE_RETURNS,
              "@returns A <b>Result</b> object that can be used to retrieve "
              "the results ofoperation.");
/**
 * $(TABLEINSERT_EXECUTE_BRIEF)
 *
 * $(TABLEINSERT_EXECUTE_RETURNS)
 *
 * #### Method Chaining
 *
 * This function can be invoked after:
 * - values(Value value1, Value value2, ...)
 */
#if DOXYGEN_JS
/**
 *
 * #### Examples
 * \dontinclude "js_devapi/scripts/mysqlx_table_insert.js"
 * \skip //@ Table.insert execution
 * \until print("Affected Rows Document:", result.affectedItemCount, "\n");
 */
Result TableInsert::execute() {}
#elif DOXYGEN_PY
/**
 *
 * #### Examples
 * \dontinclude "py_devapi/scripts/mysqlx_table_insert.py"
 * \skip #@ Table.insert execution
 * \until print "Affected Rows Document:", result.affected_item_count, "\n"
 */
Result TableInsert::execute() {}
#endif
shcore::Value TableInsert::execute(const shcore::Argument_list &args) {
  std::unique_ptr<mysqlsh::mysqlx::Result> result;
  args.ensure_count(0, get_function_name("execute").c_str());
  try {
    insert_bound_values(message_.mutable_args());
    if (message_.mutable_row()->size()) {
      result.reset(new mysqlsh::mysqlx::Result(safe_exec(
          [this]() { return session()->session()->execute_crud(message_); })));
    } else {
      result.reset(new mysqlsh::mysqlx::Result({}));
    }
  }
  CATCH_AND_TRANSLATE_CRUD_EXCEPTION(get_function_name("execute"));

  return result ? shcore::Value::wrap(result.release()) : shcore::Value::Null();
}
