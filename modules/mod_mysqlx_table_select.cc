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
#include <boost/format.hpp>
#include "mod_mysqlx_table_select.h"
#include "mod_mysqlx_table.h"
#include "mod_mysqlx_resultset.h"
#include "shellcore/common.h"
#include "utils/utils_time.h"

using namespace mysh::mysqlx;
using namespace shcore;

TableSelect::TableSelect(boost::shared_ptr<Table> owner)
  : Table_crud_definition(boost::static_pointer_cast<DatabaseObject>(owner))
{
  // Exposes the methods available for chaining
  add_method("select", boost::bind(&TableSelect::select, this, _1), "data");
  add_method("where", boost::bind(&TableSelect::where, this, _1), "data");
  add_method("groupBy", boost::bind(&TableSelect::group_by, this, _1), "data");
  add_method("having", boost::bind(&TableSelect::having, this, _1), "data");
  add_method("orderBy", boost::bind(&TableSelect::order_by, this, _1), "data");
  add_method("limit", boost::bind(&TableSelect::limit, this, _1), "data");
  add_method("offset", boost::bind(&TableSelect::offset, this, _1), "data");
  add_method("bind", boost::bind(&TableSelect::bind, this, _1), "data");

  // Registers the dynamic function behavior
  register_dynamic_function("select", "");
  register_dynamic_function("where", "select");
  register_dynamic_function("groupBy", "select, where");
  register_dynamic_function("having", "groupBy");
  register_dynamic_function("orderBy", "select, where, groupBy, having");
  register_dynamic_function("limit", "select, where, groupBy, having, orderBy");
  register_dynamic_function("offset", "limit");
  register_dynamic_function("bind", "select, where, groupBy, having, orderBy, offset, limit, bind");
  register_dynamic_function("execute", "select, where, groupBy, having, orderBy, offset, limit, bind");
  register_dynamic_function("__shell_hook__", "select, where, groupBy, having, orderBy, offset, limit, bind");

  // Initial function update
  update_functions("");
}

#ifdef DOXYGEN
/**
* Initializes this record selection handler.
* \param searchExprStr: An optional list of string expressions identifying the columns to be retrieved, alias support is enabled on these fields.
* \return This TableSelect object.
*
* The TableSelect handler will only retrieve the columns that were included on the filter, if no filter was set then all the columns will be included.
*
* Calling this function is allowed only for the first time and it is done automatically when Table.select() is called.
*
* #### Method Chaining
*
* After this function invocation, the following functions can be invoked:
*
* - where(String searchCriteria)
* - groupBy(List searchExprStr)
* - orderBy(List sortExprStr)
* - limit(Integer numberOfRows)
* - bind(String name, Value value)
* - execute()
*
* \sa Usage examples at execute().
*/
TableSelect TableSelect::select(List searchExprStr){}
#endif
shcore::Value TableSelect::select(const shcore::Argument_list &args)
{
  // Each method validates the received parameters
  args.ensure_count(0, 1, "TableSelect.select");

  boost::shared_ptr<Table> table(boost::static_pointer_cast<Table>(_owner.lock()));

  if (table)
  {
    try
    {
      std::vector<std::string> fields;

      if (args.size())
      {
        parse_string_list(args, fields);

        if (fields.size() == 0)
          throw shcore::Exception::argument_error("Field selection criteria can not be empty");
      }

      _select_statement.reset(new ::mysqlx::SelectStatement(table->_table_impl->select(fields)));

      // Updates the exposed functions
      update_functions("select");
    }
    CATCH_AND_TRANSLATE_CRUD_EXCEPTION("TableSelect.select");
  }

  return Value(boost::static_pointer_cast<Object_bridge>(shared_from_this()));
}

#ifdef DOXYGEN
/**
* Sets the search condition to filter the records to be retrieved from the owner Table.
* \param searchCondition: An optional expression to filter the records to be retrieved;
* if not specified all the records will be retrieved from the table unless a limit is set.
* \return This TableSelect object.
*
* The searchCondition supports \a [Parameter Binding](param_binding.html).
*
* #### Method Chaining
*
* This function can be invoked only once after:
*
* - select(List columns)
*
* After this function invocation, the following functions can be invoked:
*
* - groupBy(List searchExprStr)
* - orderBy(List sortExprStr)
* - limit(Integer numberOfRows)
* - bind(String name, Value value)
* - execute()
*
* \sa Usage examples at execute().
*/TableSelect TableSelect::where(String searchCondition){}
#endif
shcore::Value TableSelect::where(const shcore::Argument_list &args)
{
  args.ensure_count(1, "TableSelect.where");

  try
  {
    _select_statement->where(args.string_at(0));

    update_functions("where");
  }
  CATCH_AND_TRANSLATE_CRUD_EXCEPTION("TableSelect.where");

  return Value(boost::static_pointer_cast<Object_bridge>(shared_from_this()));
}

#ifdef DOXYGEN
/**
* Sets a grouping criteria for the resultset.
* \param searchExprStr: A list of string expressions identifying the grouping criteria.
* \return This TableSelect object.
*
* If used, the TableSelect handler will group the records using the stablished criteria.
*
* #### Method Chaining
*
* This function can be invoked only once after:
* - select(List projectedSearchExprStr)
* - where(String searchCondition)
*
* After this function invocation the following functions can be invoked:
*
* - having(String searchCondition)
* - orderBy(List sortExprStr)
* - limit(Integer numberOfRows)
* - bind(String name, Value value)
* - execute()
*
* \sa Usage examples at execute().
*/
TableSelect TableSelect::groupBy(List searchExprStr){}
#endif
shcore::Value TableSelect::group_by(const shcore::Argument_list &args)
{
  args.ensure_count(1, "TableSelect.groupBy");

  try
  {
    std::vector<std::string> fields;

    parse_string_list(args, fields);

    if (fields.size() == 0)
      throw shcore::Exception::argument_error("Grouping criteria can not be empty");

    _select_statement->groupBy(fields);

    update_functions("groupBy");
  }
  CATCH_AND_TRANSLATE_CRUD_EXCEPTION("TableSelect.groupBy");

  return Value(boost::static_pointer_cast<Object_bridge>(shared_from_this()));
}

#ifdef DOXYGEN
/**
* Sets a condition for records to be considered in agregate function operations.
* \param searchCondition: A condition on the agregate functions used on the grouping criteria.
* \return This TableSelect object.
*
* If used the TableSelect operation will only consider the records matching the stablished criteria.
*
* The searchCondition supports \a [Parameter Binding](param_binding.html).
*
* #### Method Chaining
*
* This function can be invoked only once after:
*
* - groupBy(List searchExprStr)
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
TableSelect TableSelect::having(String searchCondition){}
#endif
shcore::Value TableSelect::having(const shcore::Argument_list &args)
{
  args.ensure_count(1, "TableSelect.having");

  try
  {
    _select_statement->having(args.string_at(0));

    update_functions("having");
  }
  CATCH_AND_TRANSLATE_CRUD_EXCEPTION("TableSelect.having");

  return Value(boost::static_pointer_cast<Object_bridge>(shared_from_this()));
}

#ifdef DOXYGEN
/**
* Sets the sorting criteria to be used on the RowResult.
* \param sortExprStr: A list of expression strings defining the sort criteria for the returned records.
* \return This TableSelect object.
*
* If used the TableSelect handler will return the records sorted with the defined criteria.
*
* The elements of sortExprStr list are strings defining the column name on which the sorting will be based in the form of "columnIdentifier [ ASC | DESC ]".
* If no order criteria is specified, ascending will be used by default.
*
* #### Method Chaining
*
* This function can be invoked only once after:
*
* - select(List projectedSearchExprStr)
* - where(String searchCondition)
* - groupBy(List searchExprStr)
* - having(String searchCondition)
*
* After this function invocation, the following functions can be invoked:
*
* - limit(Integer numberOfRows)
* - bind(String name, Value value)
* - execute()
*
* \sa Usage examples at execute().
*/
TableSelect TableSelect::orderBy(List sortExprStr){}
#endif
shcore::Value TableSelect::order_by(const shcore::Argument_list &args)
{
  args.ensure_count(1, "TableSelect.orderBy");

  try
  {
    std::vector<std::string> fields;

    parse_string_list(args, fields);

    if (fields.size() == 0)
      throw shcore::Exception::argument_error("Order criteria can not be empty");

    _select_statement->orderBy(fields);

    update_functions("orderBy");
  }
  CATCH_AND_TRANSLATE_CRUD_EXCEPTION("TableSelect.orderBy");

  return Value(boost::static_pointer_cast<Object_bridge>(shared_from_this()));
}

#ifdef DOXYGEN
/**
* Sets the maximum number of records to be returned on the select operation.
* \param numberOfRows: The maximum number of records to be retrieved.
* \return This TableSelect object.
*
* If used, the TableSelect operation will return at most numberOfRows records.
*
* #### Method Chaining
*
* This function can be invoked only once after:
*
* - select(List projectedSearchExprStr)
* - where(String searchCondition)
* - groupBy(List searchExprStr)
* - having(String searchCondition)
* - orderBy(List sortExprStr)
*
* After this function invocation, the following functions can be invoked:
*
* - offset(Integer limitOffset)
* - bind(String name, Value value)
* - execute()
*
* \sa Usage examples at execute().
*/
TableSelect TableSelect::limit(Integer numberOfRows){}
#endif
shcore::Value TableSelect::limit(const shcore::Argument_list &args)
{
  args.ensure_count(1, "TableSelect.limit");

  try
  {
    _select_statement->limit(args.uint_at(0));

    update_functions("limit");
  }
  CATCH_AND_TRANSLATE_CRUD_EXCEPTION("TableSelect.limit");

  return Value(boost::static_pointer_cast<Object_bridge>(shared_from_this()));
}

#ifdef DOXYGEN
/**
* Sets number of records to skip on the RowResult when a limit has been defined.
* \param limitOffset: The number of records to skip before start including them on the Resultset.
* \return This TableSelect object.
*
* #### Method Chaining
*
* This function can be invoked only once after:
*
* - limit(Integer numberOfRows)
*
* After this function invocation, the following functions can be invoked:
*
* - bind(String name, Value value)
* - execute()
*
* \sa Usage examples at execute().
*/
TableSelect TableSelect::offset(Integer limitOffset){}
#endif
shcore::Value TableSelect::offset(const shcore::Argument_list &args)
{
  args.ensure_count(1, "TableSelect.offset");

  try
  {
    _select_statement->offset(args.uint_at(0));

    update_functions("offset");
  }
  CATCH_AND_TRANSLATE_CRUD_EXCEPTION("TableSelect.offset");

  return Value(boost::static_pointer_cast<Object_bridge>(shared_from_this()));
}

#ifdef DOXYGEN
/**
* Binds a value to a specific placeholder used on this TableSelect object.
* \param name: The name of the placeholder to which the value will be bound.
* \param value: The value to be bound on the placeholder.
* \return This TableSelect object.
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
TableSelect TableSelect::bind(String name, Value value){}
#endif

shcore::Value TableSelect::bind(const shcore::Argument_list &args)
{
  args.ensure_count(2, "TableSelect.bind");

  try
  {
    _select_statement->bind(args.string_at(0), map_table_value(args[1]));

    update_functions("bind");
  }
  CATCH_AND_TRANSLATE_CRUD_EXCEPTION("TableSelect.bind");

  return Value(boost::static_pointer_cast<Object_bridge>(shared_from_this()));
}

#ifdef DOXYGEN
/**
* Executes the Find operation with all the configured options and returns.
* \return RowResult A Row result object that can be used to traverse the records returned by rge select operation.
*
* #### Method Chaining
*
* This function can be invoked after any other function on this class.
*
* #### JavaScript Examples
*
* \dontinclude "js_devapi/scripts/mysqlx_table_select.js"
* \skip //@ Table.Select All
* \until print('Select Binding Name:', records[0].name, '\n');
*
* #### Python Examples
*
* \dontinclude "py_devapi/scripts/mysqlx_table_select.py"
* \skip #@ Table.Select All
* \until print 'Select Binding Name:', records[0].name, '\n'
*/
RowResult TableSelect::execute(){}
#endif
shcore::Value TableSelect::execute(const shcore::Argument_list &args)
{
  mysqlx::RowResult *result = NULL;

  try
  {
    args.ensure_count(0, "TableSelect.execute");

    MySQL_timer timer;
    timer.start();

    result = new mysqlx::RowResult(boost::shared_ptr< ::mysqlx::Result>(_select_statement->execute()));

    timer.end();
    result->set_execution_time(timer.raw_duration());
  }
  CATCH_AND_TRANSLATE_CRUD_EXCEPTION("TableSelect.execute");

  return result ? shcore::Value::wrap(result) : shcore::Value::Null();
}