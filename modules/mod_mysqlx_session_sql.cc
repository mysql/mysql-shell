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
#include "mod_mysqlx_session_sql.h"
#include "mod_mysqlx_session.h"
#include "mod_mysqlx_resultset.h"
#include "shellcore/common.h"
#include "mysqlx.h"

using namespace mysh::mysqlx;
using namespace shcore;

SqlExecute::SqlExecute(boost::shared_ptr<NodeSession> owner) :
_session(owner), Dynamic_object()
{
  // Exposes the methods available for chaining
  add_method("sql", boost::bind(&SqlExecute::sql, this, _1), "data");
  add_method("bind", boost::bind(&SqlExecute::bind, this, _1), "data");
  add_method("execute", boost::bind(&SqlExecute::execute, this, _1), "data");

  // Registers the dynamic function behavior
  register_dynamic_function("sql", "");
  register_dynamic_function("bind", "sql, bind");
  register_dynamic_function("execute", "sql, bind");

  // Initial function update
  update_functions("");
}

#ifdef DOXYGEN
/**
* Sets the sql statement to execute.
* This method can be invoked any number of times, each time will reset the sql statement to the value of the parameter.
* After this method, the following methods can be invoked: bind, execute.
* \sa execute(), bind()
* \param sql
* \return the same instance where this method was invoked.
*/
SqlExecute SqlExecute::sql(String sql)
{}
#endif
shcore::Value SqlExecute::sql(const shcore::Argument_list &args)
{
  // Each method validates the received parameters
  args.ensure_count(1, "SqlExecute::sql");

  try
  {
    _sql = args.string_at(0);

    // Updates the exposed functions
    update_functions("sql");
  }
  CATCH_AND_TRANSLATE_CRUD_EXCEPTION("SqlExecute::sql");
  //}

  return Value(boost::static_pointer_cast<Object_bridge>(shared_from_this()));
}

#ifdef DOXYGEN

/**
* Registers a parameter with the SQL statement.
* This method can be invoked any number of times, each time will add the parameter to the list of parameters.
* After this method the following methods can be invoked: execute.
* TODO: the bind methods seems to lack a facility to associate a parameter name with its value.
* \param val the parameter name
* \sa bind()
* \return the same instance where this method was invoked.
*/
SqlExecute SqlExecute::bind(val)
{}

/**
* Registers a set of parameters with the SQL statement.
* This method can be invoked any number of times, each time will add the parameters to the list of parameters.
* After this method the following methods can be invoked: execute.
* \param {val, val, ...} the array of parameter names.
* \return the same instance where this method was invoked.
*/
SqlExecute SqlExecute::bind({val, val, ...})
{}
#endif
shcore::Value SqlExecute::bind(const shcore::Argument_list &args)
{
  args.ensure_count(1, "SqlExecute::sql");

  if (args[0].type == shcore::Array)
  {
    shcore::Value::Array_type_ref array = args.array_at(0);
    Value::Array_type::iterator index, end = array->end();

    for (index = array->begin(); index != end; index++)
      _parameters.push_back(*index);
  }
  else
    _parameters.push_back(args[0]);

  return Value(Object_bridge_ref(this));
}

#ifdef DOXYGEN
/**
* Executes the sql statement with the bound parameters and returns a resulset with the value.
* This method can be invoked any number of times. Can be invoked after the following methods: sql (at least once), bind (zero or more).
*/
Resultset SqlExecute::execute(String sql)
{}
#endif
shcore::Value SqlExecute::execute(const shcore::Argument_list &args)
{
  args.ensure_count(0, "SqlExecute::execute");

  boost::shared_ptr<NodeSession> session(boost::static_pointer_cast<NodeSession>(_session.lock()));

  return session->executeSql(_sql, _parameters);
}