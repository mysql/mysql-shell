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

#ifdef DOXYGEN
/**
* Initializes this record deletion handler.
* \return This TableDelete object.
*
* This function is called automatically when Table.delete() is called.
*
* The actual deletion of the records will occur only when the execute method is called.
*
* After this function invocation, the following functions can be invoked:
*
* - where(String searchCriteria)
* - orderBy(List sortExprStr)
* - limit(Integer numberOfRows)
* - bind(String name, Value value)
* - execute(ExecuteOptions options).
*
* \sa Usage examples at execute(ExecuteOptions options).
*/
TableDelete TableDelete::delete(){}
#endif
shcore::Value TableDelete::remove(const shcore::Argument_list &args)
{
  // Each method validates the received parameters
  args.ensure_count(0, "TableDelete.delete");

  boost::shared_ptr<Table> table(boost::static_pointer_cast<Table>(_owner.lock()));

  if (table)
  {
    try
    {
      _delete_statement.reset(new ::mysqlx::DeleteStatement(table->_table_impl->remove()));

      // Updates the exposed functions
      update_functions("delete");
    }
    CATCH_AND_TRANSLATE_CRUD_EXCEPTION("TableDelete.delete");
  }

  return Value(boost::static_pointer_cast<Object_bridge>(shared_from_this()));
}

#ifdef DOXYGEN
/**
* Sets the search condition to filter the records to be deleted from the owner Table.
* \param searchCondition: An optional expression to filter the records to be deleted;
* if not specified all the records will be deleted from the table unless a limit is set.
* \return This TableDelete object.
*
* The searchCondition supports using placeholders instead of raw values, example:
*
* \code{.js}
* // Deleting non adult records
* table.delete().where("age < 21").execute()
*
* // Equivalent code using bound values
* table.delete().where("age < :adultAge").bind('adultAge', 21).execute()
* \endcode
*
* On the previous example, adultAge is a placeholder for a value that will be set by calling the bind() function
* right before calling execute().
*
* Note that if placeholders are used, a value must be bounded on each of them or the operation will fail.
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
* - execute(ExecuteOptions options)
*
* \sa Usage examples at execute(ExecuteOptions options).
*/
TableDelete TableDelete::where(String searchCondition){}
#endif
shcore::Value TableDelete::where(const shcore::Argument_list &args)
{
  // Each method validates the received parameters
  args.ensure_count(1, "TableDelete.where");

  boost::shared_ptr<Table> table(boost::static_pointer_cast<Table>(_owner.lock()));

  if (table)
  {
    try
    {
      _delete_statement->where(args.string_at(0));

      // Updates the exposed functions
      update_functions("where");
    }
    CATCH_AND_TRANSLATE_CRUD_EXCEPTION("TableDelete.where");
  }

  return Value(boost::static_pointer_cast<Object_bridge>(shared_from_this()));
}

#ifdef DOXYGEN
/**
* Sets the order in which the deletion should be done.
* \param sortExprStr: A list of expression strings defining a sort criteria, the deletion will be done following the order defined by this criteria.
* \return This TableDelete object.
*
* The elements of sortExprStr list are strings defining the column name on which the sorting will be based in the form of "columnIdentifier [ ASC | DESC ]".
* If no order criteria is specified, ascending will be used by default.
*
* This method is usually used in combination with limit to fix the amount of records to be deleted.
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
* - execute(ExecuteOptions options)
*/
TableDelete TableDelete::orderBy(List sortExprStr){}
#endif
shcore::Value TableDelete::order_by(const shcore::Argument_list &args)
{
  args.ensure_count(1, "TableDelete.orderBy");

  try
  {
    std::vector<std::string> fields;

    parse_string_list(args, fields);

    if (fields.size() == 0)
      throw shcore::Exception::argument_error("Order criteria can not be empty");

    _delete_statement->orderBy(fields);

    update_functions("orderBy");
  }
  CATCH_AND_TRANSLATE_CRUD_EXCEPTION("TableDelete.orderBy");

  return Value(boost::static_pointer_cast<Object_bridge>(shared_from_this()));
}

#ifdef DOXYGEN
/**
* Sets a limit for the records to be deleted.
* \param numberOfRows the number of records to be deleted.
* \return This TableDelete object.
*
* This method is usually used in combination with sort to fix the amount of records to be deleted.
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
* - execute(ExecuteOptions options)
*/
TableDelete TableDelete::limit(Integer numberOfRows){}
#endif
shcore::Value TableDelete::limit(const shcore::Argument_list &args)
{
  args.ensure_count(1, "TableDelete.limit");

  try
  {
    _delete_statement->limit(args.uint_at(0));

    update_functions("limit");
  }
  CATCH_AND_TRANSLATE_CRUD_EXCEPTION("TableDelete.limit");

  return Value(boost::static_pointer_cast<Object_bridge>(shared_from_this()));
}

#ifdef DOXYGEN
/**
* Binds a value to a specific placeholder used on this TableDelete object.
* \param name: The name of the placeholder to which the value will be bound.
* \param value: The value to be bound on the placeholder.
* \return This TableDelete object.
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
TableDelete TableDelete::bind(String name, Value value){}
#endif
shcore::Value TableDelete::bind(const shcore::Argument_list &args)
{
  args.ensure_count(2, "TableDelete.bind");

  try
  {
    _delete_statement->bind(args.string_at(0), map_table_value(args[1]));

    update_functions("bind");
  }
  CATCH_AND_TRANSLATE_CRUD_EXCEPTION("TableDelete.bind");

  return Value(boost::static_pointer_cast<Object_bridge>(shared_from_this()));
}

#ifdef DOXYGEN
/**
* Executes the record deletion with the configured filter and limit.
* \return Resultset A resultset object that can be used to retrieve the results of the deletion operation.
*
* This function can be invoked after any other function on this class.
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
* // Remove the youngest
* var res_youngest = table.delete().orderBy(['age', 'name']).limit(1).execute();
*
* // Remove the males above 15 year old
* var res_males = table.delete().where('gender="male" and age > 15').execute();
*
* // Remove the females above 15 year old
* var res_males = table.delete().where('gender=:heorshe and age > :limit').bind('heorshe', 'female').bind('limit', 15).execute();
*
* // Removes all the records
* var res_all = table.delete().execute();
* \endcode
*/
Resultset TableDelete::execute(ExecuteOptions options){}
#endif
shcore::Value TableDelete::execute(const shcore::Argument_list &args)
{
  mysqlx::Resultset *result = NULL;

  try
  {
    args.ensure_count(0, "TableDelete.execute");

    result = new mysqlx::CollectionResultset(boost::shared_ptr< ::mysqlx::Result>(_delete_statement->execute()));
  }
  CATCH_AND_TRANSLATE_CRUD_EXCEPTION("TableDelete.execute");

  return result ? shcore::Value::wrap(result) : shcore::Value::Null();
}