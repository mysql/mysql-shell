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
#include "mod_mysqlx_session_sql.h"
#include "mod_mysqlx_session.h"
#include "mod_mysqlx_resultset.h"
#include "shellcore/common.h"
#include "mysqlx.h"
#include "mysqlxtest_utils.h"

using namespace std::placeholders;
using namespace mysh::mysqlx;
using namespace shcore;

SqlExecute::SqlExecute(std::shared_ptr<NodeSession> owner) :
Dynamic_object(), _session(owner)
{
  // Exposes the methods available for chaining
  add_method("sql", std::bind(&SqlExecute::sql, this, _1), "data");
  add_method("bind", std::bind(&SqlExecute::bind, this, _1), "data");
  add_method("__shell_hook__", std::bind(&SqlExecute::execute, this, _1), "data");
  add_method("execute", std::bind(&SqlExecute::execute, this, _1), "data");

  // Registers the dynamic function behavior
  register_dynamic_function("sql", "");
  register_dynamic_function("bind", "sql, bind");
  register_dynamic_function("execute", "sql, bind");
  register_dynamic_function("__shell_hook__", "sql, bind");

  // Initial function update
  update_functions("");
}

//! Sets the sql statement to be executed by this handler.
#if DOXYGEN_CPP
//! \param args should contain a string containing the SQL statement to be executed.
#else
//! \param statement A string containing the SQL statement to be executed.
#endif
/**
* \return This SqlExecute object.
*
* This function is called automatically when NodeSession.sql(sql) is called.
*
* Parameter binding is supported and can be done by using the \b ? placeholder instead of passing values directly on the SQL statement.
* Parameters are bound in positional order.
*
* The actual execution of the SQL statement will occur when the execute() function is called.
*
* After this function invocation, the following functions can be invoked:
* - bind(Value value)
* - bind(List values)
* - execute().
*/
#if DOXYGEN_JS
SqlExecute SqlExecute::sql(String statement){}
#elif DOXYGEN_PY
SqlExecute SqlExecute::sql(str statement){}
#endif
shcore::Value SqlExecute::sql(const shcore::Argument_list &args)
{
  // Each method validates the received parameters
  args.ensure_count(1, get_function_name("sql").c_str());

  try
  {
    _sql = args.string_at(0);

    // Updates the exposed functions
    update_functions("sql");
  }
  CATCH_AND_TRANSLATE_CRUD_EXCEPTION(get_function_name("sql"));
  //}

  return Value(std::static_pointer_cast<Object_bridge>(shared_from_this()));
}

#if DOXYGEN_CPP
/**
* Registers value to be bound on the execution of the SQL statement.
* \param args should contain the value to be bound, possible values include:
* \li A value to be bound.
* \li A list of values to be bound.
* \return This SqlExecute object.
*
* This method can be invoked any number of times, each time the received parameter will be added to an internal binding list.
*
* This function can be invoked after:
* - sql(String statement)
* - bind(Value value)
* - bind(List values)
*
* After this function invocation, the following functions can be invoked:
* - bind(Value value)
* - bind(List values)
* - execute().
*/
#else
/**
* Registers a parameter to be bound on the execution of the SQL statement.
* \param value the value to be bound.
* \return This SqlExecute object.
*
* This method can be invoked any number of times, each time the received parameter will be added to an internal binding list.
*
* This function can be invoked after:
* - sql(String statement)
* - bind(Value value)
* - bind(List values)
*
* After this function invocation, the following functions can be invoked:
* - bind(Value value)
* - bind(List values)
* - execute().
*/
#if DOXYGEN_JS
SqlExecute SqlExecute::bind(Value value){}
#elif DOXYGEN_PY
SqlExecute SqlExecute::bind(Value value){}
#endif

/**
* Registers a list of parameter to be bound on the execution of the SQL statement.
* \param values the value list to be bound.
* \return This SqlExecute object.
*
* This method can be invoked any number of times, each time the received parameter will be added to an internal binding list.
*
* This function can be invoked after:
* - sql(String statement)
* - bind(Value value)
* - bind(List values)
*
* After this function invocation, the following functions can be invoked:
* - bind(Value value)
* - bind(List values)
* - execute().
*/
#if DOXYGEN_JS
SqlExecute SqlExecute::bind(List values){}
#elif DOXYGEN_PY
SqlExecute SqlExecute::bind(list values){}
#endif
#endif
shcore::Value SqlExecute::bind(const shcore::Argument_list &args)
{
  args.ensure_count(1, get_function_name("bind").c_str());

  if (args[0].type == shcore::Array)
  {
    shcore::Value::Array_type_ref array = args.array_at(0);
    Value::Array_type::iterator index, end = array->end();

    for (index = array->begin(); index != end; index++)
      _parameters.push_back(*index);
  }
  else
    _parameters.push_back(args[0]);

  return Value(std::static_pointer_cast<Object_bridge>(shared_from_this()));
}

/**
* Executes the sql statement.
* \return A SqlResult object.
*
* This function can be invoked after:
* - sql(String statement)
* - bind(Value value)
* - bind(List values)
*/
#if DOXYGEN_JS
SqlResult SqlExecute::execute(){}
#elif DOXYGEN_PY
SqlResult SqlExecute::execute(){}
#endif
shcore::Value SqlExecute::execute(const shcore::Argument_list &args)
{
  args.ensure_count(0, get_function_name("execute").c_str());

  std::shared_ptr<NodeSession> session(std::static_pointer_cast<NodeSession>(_session.lock()));

  return session->execute_sql(_sql, _parameters);
}