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
#include "modules/devapi/mod_mysqlx_table_insert.h"
#include <memory>
#include <string>
#include <vector>
#include "modules/devapi/mod_mysqlx_resultset.h"
#include "modules/devapi/mod_mysqlx_table.h"
#include "scripting/common.h"
#include "utils/utils_time.h"

using namespace std::placeholders;
using namespace mysqlsh::mysqlx;
using namespace shcore;

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
  register_dynamic_function(F::execute, F::insertFieldsAndValues | F::values | F::bind);
  register_dynamic_function(F::__shell_hook__, F::insertFieldsAndValues | F::values | F::bind);

  // Initial function update
  update_functions(F::_empty);
}

#if DOXYGEN_CPP
/**
 * Initializes the record insertion handler.
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

            Mysqlx::Crud::Insert_TypedRow* row = message_.mutable_row()->Add();
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

//! Sets the values for a row to be inserted.
#if DOXYGEN_CPP
//! \param args should contain a list of values to be used to insert a record.
#else
//! \param value1 The value for the first column.
//! \param value2 The value for the second column.
//! And so on.
#endif
/**
* \return This TableInsert object.
*
* Each column value comes as a parameter on this function call.
*
* The number of parameters must match the length of the column list defined on
* the called insert function.
* If no column list was defined the number of parameters must match the number
* of columns defined on the Table where the records will be inserted.
*
* The values must be positioned on the list in a way they match the column list
* defined on the called insert function.
* If no column list was defined the fields must match the column definition of
* the Table where the records will be inserted.
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
    Mysqlx::Crud::Insert_TypedRow* row = message_.mutable_row()->Add();
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

/**
* Executes the record insertion.
* \return Result A result object that can be used to retrieve the results of the
* insertion operation.
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
    MySQL_timer timer;
    insert_bound_values(message_.mutable_args());
    timer.start();
    if (message_.mutable_row()->size()) {
      result.reset(new mysqlsh::mysqlx::Result(
          safe_exec([this]() { return session()->session()->execute_crud(message_); })));
    } else {
      result.reset(new mysqlsh::mysqlx::Result({}));
    }
    timer.end();
    result->set_execution_time(timer.raw_duration());
  }
  CATCH_AND_TRANSLATE_CRUD_EXCEPTION(get_function_name("execute"));

  return result ? shcore::Value::wrap(result.release()) : shcore::Value::Null();
}
