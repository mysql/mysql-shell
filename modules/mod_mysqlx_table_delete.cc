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
* Creates a TableDelete object and returns it.
* This method must be invoked before any other method, after its invocation the following methods can be invoked: where, orderBy, limit, bind, execute.
* \sa where(), orderBy(), limit(), bind(), execute(), TableInsert
* \return the newly created TableDelete object.
* \code{.js}
* // open a connection
* var mysqlx = require('mysqlx').mysqlx;
* var mysession = mysqlx.getNodeSession("root:123@localhost:33060");
* // create some initial data
* mysession.sql('drop schema if exists js_shell_test;').execute();
* mysession.sql('create schema js_shell_test;').execute();
* mysession.sql('use js_shell_test;').execute();
* mysession.sql('create table table1 (name varchar(50), age integer, gender varchar(20));').execute();
* // create some initial data, populate table
* var schema = mysession.getSchema('js_shell_test');
* var table = schema.getTable('table1');
* var result = table.insert({name: 'jack', age: 17, gender: 'male'}).execute();
* var result = table.insert({name: 'adam', age: 15, gender: 'male'}).execute();
* var result = table.insert({name: 'brian', age: 14, gender: 'male'}).execute();
* // check results
* table.select().execute();
* // delete a record
* var crud = table.delete();
* crud.where('age = 15');
* crud.execute();
* // check results again
* table.select().execute();
* \endcode
*/
TableDelete TableDelete::remove()
{}
#endif
shcore::Value TableDelete::remove(const shcore::Argument_list &args)
{
  // Each method validates the received parameters
  args.ensure_count(0, "TableDelete::delete");

  boost::shared_ptr<Table> table(boost::static_pointer_cast<Table>(_owner.lock()));

  if (table)
  {
    try
    {
      _delete_statement.reset(new ::mysqlx::DeleteStatement(table->_table_impl->remove()));

      // Updates the exposed functions
      update_functions("delete");
    }
    CATCH_AND_TRANSLATE_CRUD_EXCEPTION("TableDelete::delete");
  }

  return Value(boost::static_pointer_cast<Object_bridge>(shared_from_this()));
}

#ifdef DOXYGEN
/**
* Sets the condition to filter out the rows on which the delete operation will be applied.
* This method returns the same TableDelete object where it was applied. To reset the criteria pass an empty string.
* This method can be invoked right after the 'remove' method. The method 'where' itself can be invoked any number of times, each time will replacing the search condition with the new one.
* After this method invocation the following methods can be invoked: where, orderBy, limit, bind, execute.
* \sa where(), orderBy(), limit(), bind(), execute()
* \param searchCriteria an string with any valid expression that evalutes to true/false, or 1/0.
* \return the same TableDelete object where the method was applied.
*/
TableDelete TableDelete::where(searchCriteria)
{}
#endif
shcore::Value TableDelete::where(const shcore::Argument_list &args)
{
  // Each method validates the received parameters
  args.ensure_count(1, "TableDelete::where");

  boost::shared_ptr<Table> table(boost::static_pointer_cast<Table>(_owner.lock()));

  if (table)
  {
    try
    {
      _delete_statement->where(args.string_at(0));

      // Updates the exposed functions
      update_functions("where");
    }
    CATCH_AND_TRANSLATE_CRUD_EXCEPTION("TableDelete::where");
  }

  return Value(boost::static_pointer_cast<Object_bridge>(shared_from_this()));
}

#ifdef DOXYGEN
/**
* Sets the list of columns to to use as order by when applying the remove operation. Each time the method is invoked it adds its args to the list of columns to order by.
* After this method invocation the following methods can be invoked: limit, bind, execute.
* \sa limit(), bind(), execute(), where(), remove()
* \param [expr, expr, ...] a list of expressions as Strings where each represents a column name of the form "columnIdentifier [ ASC | DESC ]", that is after the column identifier optionally 
*   can follow the order direction (ascending or descending) being ascending the default if not provided.
* \return the same TableDelete object where the method was applied.
*/
TableDelete TableDelete::orderBy([expr, expr, ...])
{}
#endif
shcore::Value TableDelete::order_by(const shcore::Argument_list &args)
{
  args.ensure_count(1, "TableDelete::orderBy");

  try
  {
    std::vector<std::string> fields;

    parse_string_list(args, fields);

    if (fields.size() == 0)
      throw shcore::Exception::argument_error("Order criteria can not be empty");

    _delete_statement->orderBy(fields);

    update_functions("orderBy");
  }
  CATCH_AND_TRANSLATE_CRUD_EXCEPTION("TableDelete::orderBy");

  return Value(boost::static_pointer_cast<Object_bridge>(shared_from_this()));
}

#ifdef DOXYGEN
/**
* Sets the limit of rows to which to apply the remove operation. Each time the method is invoked it resets the value of limit.
* After this method has been invoked the following methods can be invoked: bind, execute.
* \sa remove(), where(), orderBy(), bind(), execute().
* \param lim a positive integer representing the number of rows to which to apply the delete operation.
* \return the same TableDelete object where the method was applied.
*/
TableDelete TableDelete::limit(lim)
{}
#endif
shcore::Value TableDelete::limit(const shcore::Argument_list &args)
{
  args.ensure_count(1, "TableDelete::limit");

  try
  {
    _delete_statement->limit(args.uint_at(0));

    update_functions("limit");
  }
  CATCH_AND_TRANSLATE_CRUD_EXCEPTION("TableDelete::limit");

  return Value(boost::static_pointer_cast<Object_bridge>(shared_from_this()));
}

#ifdef DOXYGEN
/**
* TODO: Review this when the method is actually implemented.
* Sets the mappings (parameter names / parameter values) to substitute during the Remove operation execution.
* After this method invocation, the following methods can be invoked: execute.
* \sa execute(), remove(), where(), orderBy(), limit()
* \param { var:val, var : val, ... } a map with a set of key value pairs where each key is a parameter name and parameter value.
* \return the same TableDelete object where the method was applied.
*/
TableDelete TableDelete::bind({ var:val, var : val, ... })
{}
#endif
shcore::Value TableDelete::bind(const shcore::Argument_list &UNUSED(args))
{
  throw shcore::Exception::logic_error("TableDelete::bind: not yet implemented.");

  return Value(Object_bridge_ref(this));
}

#ifdef DOXYGEN
/**
* TODO: review this when managing of execution options is actually implemented.
* Execute the remove operation with the configured options.
* This method can be invoked any number of times. Can be invoked right after the following methods: remove, where, orderBy, limit, bind.
* \sa
* \param opt the execution options, can be...
* \return a ResultSet describing the results of the operation (has no data but affected rows, warning count, etc).
*/
Resultset TableDelete::execute(ExecuteOptions opt)
{}
#endif
shcore::Value TableDelete::execute(const shcore::Argument_list &args)
{
  args.ensure_count(0, "TableDelete::execute");

  return shcore::Value::wrap(new mysqlx::Collection_resultset(boost::shared_ptr< ::mysqlx::Result>(_delete_statement->execute())));
}