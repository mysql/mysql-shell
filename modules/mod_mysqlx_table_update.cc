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
#include "mod_mysqlx_table_update.h"
#include "mod_mysqlx_table.h"
#include "mod_mysqlx_resultset.h"
#include "shellcore/common.h"
#include "mod_mysqlx_expression.h"
#include "utils/utils_time.h"
#include <sstream>

using namespace mysh::mysqlx;
using namespace shcore;

TableUpdate::TableUpdate(boost::shared_ptr<Table> owner)
:Table_crud_definition(boost::static_pointer_cast<DatabaseObject>(owner))
{
  // Exposes the methods available for chaining
  add_method("update", boost::bind(&TableUpdate::update, this, _1), "data");
  add_method("set", boost::bind(&TableUpdate::set, this, _1), "data");
  add_method("where", boost::bind(&TableUpdate::where, this, _1), "data");
  add_method("orderBy", boost::bind(&TableUpdate::order_by, this, _1), "data");
  add_method("limit", boost::bind(&TableUpdate::limit, this, _1), "data");
  add_method("bind", boost::bind(&TableUpdate::bind, this, _1), "data");

  // Registers the dynamic function behavior
  register_dynamic_function("update", "");
  register_dynamic_function("set", "update, set");
  register_dynamic_function("where", "set");
  register_dynamic_function("orderBy", "set, where");
  register_dynamic_function("limit", "set, where, orderBy");
  register_dynamic_function("bind", "set, where, orderBy, limit, bind");
  register_dynamic_function("execute", "set, where, orderBy, limit, bind");
  register_dynamic_function("__shell_hook__", "set, where, orderBy, limit, bind");

  // Initial function update
  update_functions("");
}

#ifdef DOXYGEN
/**
* Initializes this record update handler.
* \return This TableUpdate object.
*
* This function is called automatically when Table.update() is called.
*
* The actual update of the records will occur only when the execute method is called.
*
* After this function invocation, the following functions can be invoked:
*
* - set(String attribute, Value value)
* - where(String searchCriteria)
* - orderBy(List sortExprStr)
* - limit(Integer numberOfRows)
* - bind(String name, Value value)
* - execute(ExecuteOptions options)
*
* \sa Usage examples at execute(ExecuteOptions options).
*/
TableUpdate TableUpdate::update(){}
#endif
shcore::Value TableUpdate::update(const shcore::Argument_list &args)
{
  // Each method validates the received parameters
  args.ensure_count(0, "TableUpdate.update");

  boost::shared_ptr<Table> table(boost::static_pointer_cast<Table>(_owner.lock()));

  if (table)
  {
    try
    {
      _update_statement.reset(new ::mysqlx::UpdateStatement(table->_table_impl->update()));

      // Updates the exposed functions
      update_functions("update");
    }
    CATCH_AND_TRANSLATE_CRUD_EXCEPTION("TableUpdate.update");
  }

  return Value(boost::static_pointer_cast<Object_bridge>(shared_from_this()));
}

#ifdef DOXYGEN
/**
* Updates the column value on records in a table.
* \param attribute A string with the column name to be updated.
* \param value The value to be set on the specified column.
* \return This TableUpdate object.
*
* Adds an opertion into the update handler to update a column value in on the records that were included on the selection filter and limit.
*
* The update will be done on the table's records once the execute method is called.
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
* - execute(ExecuteOptions options)
*
* \sa Usage examples at execute(ExecuteOptions options).
*/
TableUpdate TableUpdate::set(String attribute, Value value){}
#endif
shcore::Value TableUpdate::set(const shcore::Argument_list &args)
{
  // Each method validates the received parameters
  args.ensure_count(2, "TableUpdate.set");

  try
  {
    std::string field = args.string_at(0);

    // Only expression objects are allowed as values
    std::string expr_data;
    if (args[1].type == shcore::Object)
    {
      shcore::Object_bridge_ref object = args.object_at(1);

      boost::shared_ptr<Expression> expression = boost::dynamic_pointer_cast<Expression>(object);

      if (expression)
        expr_data = expression->get_data();
      else
      {
        std::stringstream str;
        str << "TableUpdate.set: Unsupported value received for table update operation on field \"" << field << "\", received: " << args[1].descr();
        throw shcore::Exception::argument_error(str.str());
      }
    }

    // Calls set for each of the values
    if (!expr_data.empty())
      _update_statement->set(field, expr_data);
    else
      _update_statement->set(field, map_table_value(args[1]));

    update_functions("set");
  }
  CATCH_AND_TRANSLATE_CRUD_EXCEPTION("TableUpdate.set");

  return Value(boost::static_pointer_cast<Object_bridge>(shared_from_this()));
}

#ifdef DOXYGEN
/**
* Sets the search condition to filter the records to be updated on the owner Table.
* \param searchCondition: An optional expression to filter the records to be updated;
* if not specified all the records will be updated from the table unless a limit is set.
* \return This TableUpdate object.
*
* The searchCondition supports using placeholders instead of raw values, example:
*
* \code{.js}
* // Setting adult flag on records
* table.update().set('adult', 'yes').where("age > 21").execute()
*
* // Equivalent code using bound values
* table.update().set('adult', 'yes').where("age > :adultAge").bind('adultAge', 21).execute()
* \endcode
*
* On the previous example, adultAge is a placeholder for a value that will be set by calling the bind() function
* right before calling execute().
*
* Note that if placeholders are used, a value must be bounded on each of them or the operation will fail.
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
* - execute(ExecuteOptions options)
*
* \sa Usage examples at execute(ExecuteOptions options).
*/
TableUpdate TableUpdate::where(String searchCondition){}
#endif
shcore::Value TableUpdate::where(const shcore::Argument_list &args)
{
  // Each method validates the received parameters
  args.ensure_count(1, "TableUpdate.where");

  try
  {
    _update_statement->where(args.string_at(0));

    // Updates the exposed functions
    update_functions("where");
  }
  CATCH_AND_TRANSLATE_CRUD_EXCEPTION("TableUpdate.where");

  return Value(boost::static_pointer_cast<Object_bridge>(shared_from_this()));
}

#ifdef DOXYGEN
/**
* Sets the order in which the update should be done.
* \param sortExprStr: A list of expression strings defining a sort criteria, the update will be done following the order defined by this criteria.
* \return This TableUpdate object.
*
* The elements of sortExprStr list are strings defining the column name on which the sorting will be based in the form of "columnIdentifier [ ASC | DESC ]".
* If no order criteria is specified, ascending will be used by default.
*
* This method is usually used in combination with limit to fix the amount of records to be updated.
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
* - execute(ExecuteOptions options)
*/
TableUpdate TableUpdate::orderBy(List sortExprStr){}
#endif
shcore::Value TableUpdate::order_by(const shcore::Argument_list &args)
{
  args.ensure_count(1, "TableUpdate.orderBy");

  try
  {
    std::vector<std::string> fields;

    parse_string_list(args, fields);

    if (fields.size() == 0)
      throw shcore::Exception::argument_error("Order criteria can not be empty");

    _update_statement->orderBy(fields);

    update_functions("orderBy");
  }
  CATCH_AND_TRANSLATE_CRUD_EXCEPTION("TableUpdate.orderBy");

  return Value(boost::static_pointer_cast<Object_bridge>(shared_from_this()));
}

#ifdef DOXYGEN
/**
* Sets a limit for the records to be updated.
* \param numberOfRows the number of records to be updated.
* \return This TableUpdate object.
*
* This method is usually used in combination with sort to fix the amount of records to be updated.
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
* - execute(ExecuteOptions options)
*/
TableUpdate TableUpdate::limit(Integer numberOfRows){}
#endif
shcore::Value TableUpdate::limit(const shcore::Argument_list &args)
{
  args.ensure_count(1, "TableUpdate.limit");

  try
  {
    _update_statement->limit(args.uint_at(0));

    update_functions("limit");
  }
  CATCH_AND_TRANSLATE_CRUD_EXCEPTION("TableUpdate.limit");

  return Value(boost::static_pointer_cast<Object_bridge>(shared_from_this()));
}

#ifdef DOXYGEN
/**
* Binds a value to a specific placeholder used on this TableUpdate object.
* \param name: The name of the placeholder to which the value will be bound.
* \param value: The value to be bound on the placeholder.
* \return This TableUpdate object.
*
* This function can be invoked multiple times right before calling execute:
*
* After this function invocation, the following functions can be invoked:
*
* - bind(String name, Value value)
* - execute(ExecuteOptions options)
*
* An error will be raised if the placeholder indicated by name does not exist.
*
* This function must be called once for each used placeohlder or an error will be
* raised when the execute method is called.
*
* \sa Usage examples at execute(ExecuteOptions options).
*/
TableUpdate TableUpdate::bind(String name, Value value){}
#endif
shcore::Value TableUpdate::bind(const shcore::Argument_list &args)
{
  args.ensure_count(2, "TableUpdate.bind");

  try
  {
    _update_statement->bind(args.string_at(0), map_table_value(args[1]));

    update_functions("bind");
  }
  CATCH_AND_TRANSLATE_CRUD_EXCEPTION("TableUpdate.bind");

  return Value(boost::static_pointer_cast<Object_bridge>(shared_from_this()));
}

#ifdef DOXYGEN
/**
* Executes the record update with the configured filter and limit.
* \return Result A result object that can be used to retrieve the results of the update operation.
*
* This function can be invoked after any other function on this class except update().
*
* \code{.js}
* // open a connection
* var mysqlx = require('mysqlx').mysqlx;
* var mysession = mysqlx.getNodeSession("myuser@localhost", mypwd);
*
* // Creates a table named friends on the test schema
* mysession.sql('create table test.friends (name varchar(50), age integer, gender varchar(20));').execute();
*
* var table = mysession.test.friends;
*
* // create some initial data
* table.insert('name','last_name','age','gender')
*      .values('jack','black', 17, 'male')
*      .values('adam', 'sandler', 15, 'male')
*      .values('brian', 'adams', 14, 'male')
*      .values('alma', 'lopez', 13, 'female')
*      .values('carol', 'shiffield', 14, 'female')
*      .values('donna', 'summers', 16, 'female')
*      .values('angel', 'down', 14, 'male').execute();
*
* // Updates the age to the youngest
* var result = table.update().set('age', 14).orderBy(['age']).limit(1).execute();
*
* // Updates last name and age to angel
* var res_angel = table.update().set('last_name','downey).set('age',15).where('name="angel"').execute();
*
* // Previous example using a bound value
* var res_angel = table.update().set('last_name','downey).set('age',15).where('name=:name').bind('name', 'angel').execute();
* \endcode
*/
Result TableUpdate::execute(ExecuteOptions options){}
#endif
shcore::Value TableUpdate::execute(const shcore::Argument_list &args)
{
  mysqlx::Result *result = NULL;

  try
  {
    args.ensure_count(0, "TableUpdate.execute");
    MySQL_timer timer;
    timer.start();
    result = new mysqlx::Result(boost::shared_ptr< ::mysqlx::Result>(_update_statement->execute()));
    timer.end();
    result->set_execution_time(timer.raw_duration());
  }
  CATCH_AND_TRANSLATE_CRUD_EXCEPTION("TableUpdate.execute");

  return result ? shcore::Value::wrap(result) : shcore::Value::Null();
}
