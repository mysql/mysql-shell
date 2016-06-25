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
#include <boost/bind.hpp>
#include "mod_mysqlx_table_delete.h"
#include "mod_mysqlx_table.h"
#include "mod_mysqlx_resultset.h"
#include "shellcore/common.h"
#include "utils/utils_time.h"

using namespace mysh::mysqlx;
using namespace shcore;

TableDelete::TableDelete(boost::shared_ptr<Table> owner)
  :Table_crud_definition(boost::static_pointer_cast<DatabaseObject>(owner))
{
  // Exposes the methods available for chaining
  add_method("delete", boost::bind(&TableDelete::remove, this, _1), "data");
  add_method("where", boost::bind(&TableDelete::where, this, _1), "data");
  add_method("orderBy", boost::bind(&TableDelete::order_by, this, _1), "data");
  add_method("limit", boost::bind(&TableDelete::limit, this, _1), "data");
  add_method("bind", boost::bind(&TableDelete::bind, this, _1), "data");

  // Registers the dynamic function behavior
  register_dynamic_function("delete", "");
  register_dynamic_function("where", "delete");
  register_dynamic_function("orderBy", "delete, where");
  register_dynamic_function("limit", "delete, where, orderBy");
  register_dynamic_function("bind", "delete, where, orderBy, limit, bind");
  register_dynamic_function("execute", "delete, where, orderBy, limit, bind");
  register_dynamic_function("__shell_hook__", "delete, where, orderBy, limit, bind");

  // Initial function update
  update_functions("");
}

/**
* Initializes this record deletion handler.
* \return This TableDelete object.
*
* This function is called automatically when Table.delete() is called.
*
* The actual deletion of the records will occur only when the execute method is called.
*
* #### Method Chaining
*
* After this function invocation, the following functions can be invoked:
*
* - where(String searchCriteria)
* - orderBy(List sortExprStr)
* - limit(Integer numberOfRows)
* - bind(String name, Value value)
* - execute().
*
* \sa Usage examples at execute().
*/
#if DOXYGEN_JS
TableDelete TableDelete::delete(){}
#elif DOXYGEN_PY
TableDelete TableDelete::delete(){}
#endif
shcore::Value TableDelete::remove(const shcore::Argument_list &args)
{
  // Each method validates the received parameters
  args.ensure_count(0, get_function_name("delete").c_str());

  boost::shared_ptr<Table> table(boost::static_pointer_cast<Table>(_owner.lock()));

  if (table)
  {
    try
    {
      _delete_statement.reset(new ::mysqlx::DeleteStatement(table->_table_impl->remove()));

      // Updates the exposed functions
      update_functions("delete");
    }
    CATCH_AND_TRANSLATE_CRUD_EXCEPTION(get_function_name("delete"));
  }

  return Value(boost::static_pointer_cast<Object_bridge>(shared_from_this()));
}

//! Sets the search condition to filter the records to be deleted from the owner Table.
#if DOXYGEN_CPP
//! \param args should contain an optional string expression to filter the records to be deleted.
#else
//! \param searchCondition: An optional expression to filter the records to be deleted.
#endif
/**
* if not specified all the records will be deleted from the table unless a limit is set.
* \return This TableDelete object.
*
* The searchCondition supports \a [Parameter Binding](param_binding.html).
*
* #### Method Chaining
*
* This function can be invoked only once after:
*
* - delete()
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
TableDelete TableDelete::where(String searchCondition){}
#elif DOXYGEN_PY
TableDelete TableDelete::where(str searchCondition){}
#endif
shcore::Value TableDelete::where(const shcore::Argument_list &args)
{
  // Each method validates the received parameters
  args.ensure_count(1, get_function_name("where").c_str());

  boost::shared_ptr<Table> table(boost::static_pointer_cast<Table>(_owner.lock()));

  if (table)
  {
    try
    {
      _delete_statement->where(args.string_at(0));

      // Updates the exposed functions
      update_functions("where");
    }
    CATCH_AND_TRANSLATE_CRUD_EXCEPTION(get_function_name("where"));
  }

  return Value(boost::static_pointer_cast<Object_bridge>(shared_from_this()));
}

//! Sets the order in which the deletion should be done.
#if DOXYGEN_CPP
//! \param args should contain a list of expression strings defining a sort criteria, the deletion will be done following the order defined by this criteria.
#else
//! \param sortExprStr: A list of expression strings defining a sort criteria, the deletion will be done following the order defined by this criteria.
#endif
/**
* \return This TableDelete object.
*
* The elements of sortExprStr list are strings defining the column name on which the sorting will be based in the form of "columnIdentifier [ ASC | DESC ]".
* If no order criteria is specified, ascending will be used by default.
*
* This method is usually used in combination with limit to fix the amount of records to be deleted.
*
* #### Method Chaining
*
* This function can be invoked only once after:
*
* - delete()
* - where(String searchCondition)
*
* After this function invocation, the following functions can be invoked:
*
* - limit(Integer numberOfRows)
* - bind(String name, Value value)
* - execute()
*
*/
#if DOXYGEN_JS
TableDelete TableDelete::orderBy(List sortExprStr){}
#elif DOXYGEN_PY
TableDelete TableDelete::order_by(list sortExprStr){}
#endif
shcore::Value TableDelete::order_by(const shcore::Argument_list &args)
{
  args.ensure_count(1, get_function_name("orderBy").c_str());

  try
  {
    std::vector<std::string> fields;

    parse_string_list(args, fields);

    if (fields.size() == 0)
      throw shcore::Exception::argument_error("Order criteria can not be empty");

    _delete_statement->orderBy(fields);

    update_functions("orderBy");
  }
  CATCH_AND_TRANSLATE_CRUD_EXCEPTION(get_function_name("orderBy"));

  return Value(boost::static_pointer_cast<Object_bridge>(shared_from_this()));
}

//! Sets a limit for the records to be deleted.
#if DOXYGEN_CPP
//! \param args should contain the number of records to be deleted.
#else
//! \param numberOfRows the number of records to be deleted.
#endif
/**
* \return This TableDelete object.
*
* This method is usually used in combination with sort to fix the amount of records to be deleted.
*
* #### Method Chaining
*
* This function can be invoked only once after:
*
* - delete()
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
TableDelete TableDelete::limit(Integer numberOfRows){}
#elif DOXYGEN_PY
TableDelete TableDelete::limit(int numberOfRows){}
#endif
shcore::Value TableDelete::limit(const shcore::Argument_list &args)
{
  args.ensure_count(1, get_function_name("limit").c_str());

  try
  {
    _delete_statement->limit(args.uint_at(0));

    update_functions("limit");
  }
  CATCH_AND_TRANSLATE_CRUD_EXCEPTION(get_function_name("limit"));

  return Value(boost::static_pointer_cast<Object_bridge>(shared_from_this()));
}

//! Binds a value to a specific placeholder used on this TableDelete object.
#if DOXYGEN_CPP
//! \param args should contain the next elements:
//! \li The name of the placeholder to which the value will be bound.
//! \li The value to be bound on the placeholder.
#else
//! \param name: The name of the placeholder to which the value will be bound.
//! \param value: The value to be bound on the placeholder.
#endif
/**
* \return This TableDelete object.
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
* This function must be called once for each used placeohlder or an error will be
* raised when the execute method is called.
*
* \sa Usage examples at execute().
*/
#if DOXYGEN_JS
TableDelete TableDelete::bind(String name, Value value){}
#elif DOXYGEN_PY
TableDelete TableDelete::bind(str name, Value value){}
#endif
shcore::Value TableDelete::bind(const shcore::Argument_list &args)
{
  args.ensure_count(2, get_function_name("bind").c_str());

  try
  {
    _delete_statement->bind(args.string_at(0), map_table_value(args[1]));

    update_functions("bind");
  }
  CATCH_AND_TRANSLATE_CRUD_EXCEPTION(get_function_name("bind"));

  return Value(boost::static_pointer_cast<Object_bridge>(shared_from_this()));
}

/**
* Executes the record deletion with the configured filter and limit.
* \return Result A result object that can be used to retrieve the results of the deletion operation.
*
* #### Method Chaining
*
* This function can be invoked after any other function on this class.
*/
#if DOXYGEN_JS
/**
*
* #### Examples
* \dontinclude "js_devapi/scripts/mysqlx_table_delete.js"
* \skip //@ TableDelete: delete under condition
* \until //@ TableDelete: with limit 3
* \until print('Records Left:', records.length, '\n');
*/
Result TableDelete::execute(){}
#elif DOXYGEN_PY
/**
*
* #### Examples
* \dontinclude "py_devapi/scripts/mysqlx_table_delete.py"
* \skip #@ TableDelete: delete under condition
* \until #@ TableDelete: with limit 3
* \until print 'Records Left:', len(records), '\n'
*/
Result TableDelete::execute(){}
#endif
shcore::Value TableDelete::execute(const shcore::Argument_list &args)
{
  mysqlx::Result *result = NULL;

  try
  {
    args.ensure_count(0, get_function_name("execute").c_str());

    MySQL_timer timer;
    timer.start();
    result = new mysqlx::Result(boost::shared_ptr< ::mysqlx::Result>(_delete_statement->execute()));
    timer.end();
    result->set_execution_time(timer.raw_duration());
  }
  CATCH_AND_TRANSLATE_CRUD_EXCEPTION(get_function_name("execute"));

  return result ? shcore::Value::wrap(result) : shcore::Value::Null();
}