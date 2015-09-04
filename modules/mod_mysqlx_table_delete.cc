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
  register_dynamic_function("bind", "delete, where, orderBy, limit");
  register_dynamic_function("execute", "delete, where, orderBy, limit, bind");

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
* This function is called automatically when Table.delete(searchCondition) is called.
*
* The actual deletion of the records will occur only when the execute method is called.
*
* This function can be invoked only once after:
*
* - delete()
*
* After this function invocation, the following functions can be invoked:
*
* - orderBy(List sortExprStr)
* - limit(Integer numberOfRows)
* - execute(ExecuteOptions options).
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
* - execute(ExecuteOptions options).
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
* - execute(ExecuteOptions options).
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
TableDelete TableDelete::bind({ var:val, var : val, ... }){}
#endif
shcore::Value TableDelete::bind(const shcore::Argument_list &UNUSED(args))
{
  throw shcore::Exception::logic_error("TableDelete.bind: not yet implemented.");

  return Value(Object_bridge_ref(this));
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
* // Remove the males
* var res_males = table.delete().where('gender="male"').execute();
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

    result = new mysqlx::Collection_resultset(boost::shared_ptr< ::mysqlx::Result>(_delete_statement->execute()));
  }
  CATCH_AND_TRANSLATE_CRUD_EXCEPTION("TableInsert.execute");

  return result ? shcore::Value::wrap(result) : shcore::Value::Null();
}