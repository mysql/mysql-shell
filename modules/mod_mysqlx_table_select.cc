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
  register_dynamic_function("bind", "select, where, groupBy, having, orderBy, offset, limit");
  register_dynamic_function("execute", "select, where, groupBy, having, orderBy, offset, limit, bind");

  // Initial function update
  update_functions("");
}

#ifdef DOXYGEN
/**
* Creates a new select statement and returns it.
* The method must be executed first. After it is invoked, the following methods can be invoked: where, groupBy, orderBy, limit, bind, execute.
* \sa where(), groupBy(), orderBy(), limit(), bind(), execute().
* \param [field, field, ...], a optional array of Strings representing field names, which will become the projection (the columns in the result set) of the select statement. 
*   If no field list is provided then the select returns all the fields in the underlying table.
* \return a new TableSelect object.
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
TableSelect TableSelect::select([field, field, ...])
{}
#endif
shcore::Value TableSelect::select(const shcore::Argument_list &args)
{
  // Each method validates the received parameters
  args.ensure_count(0, 1, "TableSelect::select");

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
    CATCH_AND_TRANSLATE_CRUD_EXCEPTION("TableSelect::select");
  }

  return Value(boost::static_pointer_cast<Object_bridge>(shared_from_this()));
}

#ifdef DOXYGEN
/**
* Sets the expression with the search criteria, those rows that met it will be retrieved in the result.
* This method can be invoked after the select method.
* After this method invocation the following methods can be invoked: groupBy, orderBy, limit, bind, execute.
* \sa select(), groupBy(), orderBy(), limit(), bind(), execute().
* \param searchCriteria an String with the expression criteria to use to filter out those rows that don't meet it.
* \return the same TableSelect object where the method was applied.
*/
TableSelect TableSelect::where(searchCriteria)
{}
#endif
shcore::Value TableSelect::where(const shcore::Argument_list &args)
{
  args.ensure_count(1, "TableSelect::where");

  try
  {
    _select_statement->where(args.string_at(0));

    update_functions("where");
  }
  CATCH_AND_TRANSLATE_CRUD_EXCEPTION("TableSelect::where");

  return Value(boost::static_pointer_cast<Object_bridge>(shared_from_this()));
}

#ifdef DOXYGEN
/**
* Sets a grouping criteria for the resultset, if used, the TableSelect operation will group the records using the stablished criteria.
* This method can be invoked after the following method have been invoked: where.
* This method can be invoked after the methods find and fields. And after method groupBy invocation the following methods can be invoked: having, sort, limit, bind, execute.
*
* \sa select(), where(), having(), sort(), limit(), bind(), execute().
* \param searchExprStr: A list of string expressions identifying the grouping criteria.
* \return the same TableSelect object where the method was applied.
*/
TableSelect TableSelect::groupBy(List searchExprStr)
{}
#endif
shcore::Value TableSelect::group_by(const shcore::Argument_list &args)
{
  args.ensure_count(1, "TableSelect::groupBy");

  try
  {
    std::vector<std::string> fields;

    parse_string_list(args, fields);

    if (fields.size() == 0)
      throw shcore::Exception::argument_error("Grouping criteria can not be empty");

    _select_statement->groupBy(fields);

    update_functions("groupBy");
  }
  CATCH_AND_TRANSLATE_CRUD_EXCEPTION("TableSelect::groupBy");

  return Value(boost::static_pointer_cast<Object_bridge>(shared_from_this()));
}

#ifdef DOXYGEN
/**
* Sets a condition for records to be considered in agregate function operations, if used the CollectionFind operation will only consider the records matching the stablished criteria.
* Calling this method is allowed only for the first time and only if the grouping criteria has been set by calling TableSelect.groupBy(groupCriteria), after that its usage is forbidden since the internal class state has been updated to handle the rest of the Select operation.
* After this method invocation the following methods can be invoked: sort, limit, bind, execute.
*
* \sa groupBy(), sort(), limit(), bind(), execute().
* \param searchCondition: A condition on the agregate functions used on the grouping criteria.
* \return the same TableSelect object where the method was applied.
*/
TableSelect TableSelect::having(String searchCondition)
{}
#endif
shcore::Value TableSelect::having(const shcore::Argument_list &args)
{
  args.ensure_count(1, "TableSelect::having");

  try
  {
    _select_statement->having(args.string_at(0));

    update_functions("having");
  }
  CATCH_AND_TRANSLATE_CRUD_EXCEPTION("TableSelect::having");

  return Value(boost::static_pointer_cast<Object_bridge>(shared_from_this()));
}

#ifdef DOXYGEN
/**
* Sets the sorting criteria to be used on the Resultset, if used the TableSelect operation will return the records sorted with the defined criteria.
* Calling this method is allowed only for the first time and only if the search criteria has been set by calling TableSelect.where(), after that its usage is forbidden since the internal class state has been updated to handle the rest of the TableSelect operation.
* And after this method invocation the following methods can be invoked: limit, bind, execute.
*
* \sa limit(), bind(), execute()
* \param sortExprStr: A list containing the sort criteria expressions to be used on the operation.
* \return the same TableSelect object where the method was applied.
*/
TableSelect TableSelect::orderBy(List sortExprStr)
{}
#endif
shcore::Value TableSelect::order_by(const shcore::Argument_list &args)
{
  args.ensure_count(1, "TableSelect::orderBy");

  try
  {
    std::vector<std::string> fields;

    parse_string_list(args, fields);

    if (fields.size() == 0)
      throw shcore::Exception::argument_error("Order criteria can not be empty");

    _select_statement->orderBy(fields);

    update_functions("orderBy");
  }
  CATCH_AND_TRANSLATE_CRUD_EXCEPTION("TableSelect::orderBy");

  return Value(boost::static_pointer_cast<Object_bridge>(shared_from_this()));
}

#ifdef DOXYGEN
/**
* Sets the maximum number of documents to be returned on the select operation, if used the TableSelect operation will return at most numberOfRows documents.
* Calling this method is allowed only for the first time and only if the search criteria has been set by calling TableSelect.where(searchCriteria), after that its usage is forbidden since the internal class state has been updated to handle the rest of the TableSelect operation.
* After this method invocation the following methods can be invoked: offset, bind, execute.
*
* \sa offset(), bind(), execute().
* \param numberOfRows: The maximum number of documents to be retrieved.
* \return the same TableSelect object where the method was applied.
*/
TableSelect TableSelect::limit(Integer numberOfRows)
{}
#endif
shcore::Value TableSelect::limit(const shcore::Argument_list &args)
{
  args.ensure_count(1, "TableSelect::limit");

  try
  {
    _select_statement->limit(args.uint_at(0));

    update_functions("limit");
  }
  CATCH_AND_TRANSLATE_CRUD_EXCEPTION("TableSelect::limit");

  return Value(boost::static_pointer_cast<Object_bridge>(shared_from_this()));
}

#ifdef DOXYGEN
/**
* Sets number of records to skip on the resultset when a limit has been defined.
* Calling this method is allowed only for the first time and only if a limit has been set by calling TableSelect.limit(numberOfRows), after that its usage is forbidden since the internal class state has been updated to handle the rest of the TableSelect operation.
* After this method, the following methods can be invoked: bind, execute.
*
* \sa limit(), bind(), execute()
* \param limitOffset: The number of documents to skip before start including them on the Resultset.
* \return the same TableSelect object where the method was applied.
*/
TableSelect TableSelect::offset(Integer limitOffset)
{}
#endif
shcore::Value TableSelect::offset(const shcore::Argument_list &args)
{
  args.ensure_count(1, "TableSelect::offset");

  try
  {
    _select_statement->offset(args.uint_at(0));

    update_functions("offset");
  }
  CATCH_AND_TRANSLATE_CRUD_EXCEPTION("TableSelect::offset");

  return Value(boost::static_pointer_cast<Object_bridge>(shared_from_this()));
}

#ifdef DOXYGEN
/**
* Sets the bindings mappings for the TableSelect.
* \param { var:val, var : val, ... } a Map a map of key value pairs where each key is the name of a parameter and each value is the value of the parameter.
* \return the same TableSelect object where the method was applied.
*/
TableSelect TableSelect::bind({ var:val, var : val, ... })
{}
#endif
shcore::Value TableSelect::bind(const shcore::Argument_list &UNUSED(args))
{
  throw shcore::Exception::logic_error("TableSelect::bind: not yet implemented.");

  return Value(boost::static_pointer_cast<Object_bridge>(shared_from_this()));
}

#ifdef DOXYGEN
/**
* Executes the TableSelect operation with all the configured options and returns.
* \param opt the execution options (currently not used).
* \return a ResultSet object that can be used to retrieve the results.
*/
ResultSet TableSelect::execute(ExecuteOptions opt)
{}
#endif
shcore::Value TableSelect::execute(const shcore::Argument_list &args)
{
  args.ensure_count(0, "TableSelect::execute");

  return shcore::Value::wrap(new mysqlx::Resultset(boost::shared_ptr< ::mysqlx::Result>(_select_statement->execute())));
}