/*
 * Copyright (c) 2015, 2017, Oracle and/or its affiliates. All rights reserved.
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

#include "modules/devapi/mod_mysqlx_session_sql.h"
#include <memory>
#include "modules/devapi/mod_mysqlx_resultset.h"
#include "modules/devapi/mod_mysqlx_session.h"
#include "scripting/common.h"
#include "db/mysqlx/mysqlxclient_clean.h"
#include "mysqlxtest_utils.h"
#include "scripting/common.h"
#include "shellcore/utils_help.h"

using namespace std::placeholders;
using namespace mysqlsh::mysqlx;
using namespace shcore;

// Documentation of SqlExecute class
REGISTER_HELP(
    SQLEXECUTE_BRIEF,
    "Handler for execution SQL statements, supports parameter binding.");
REGISTER_HELP(
    SQLEXECUTE_DETAIL,
    "This object should only be created by calling the sql function a NodeSession instance.");

SqlExecute::SqlExecute(std::shared_ptr<NodeSession> owner)
    : Dynamic_object(), _session(owner) {
  // Exposes the methods available for chaining
  add_method("sql", std::bind(&SqlExecute::sql, this, _1), "data");
  add_method("bind", std::bind(&SqlExecute::bind, this, _1), "data");
  add_method("__shell_hook__", std::bind(&SqlExecute::execute, this, _1),
             "data");
  add_method("execute", std::bind(&SqlExecute::execute, this, _1), "data");

  // Registers the dynamic function behavior
  register_dynamic_function("sql", "");
  register_dynamic_function("bind", "sql, bind");
  register_dynamic_function("execute", "sql, bind");
  register_dynamic_function("__shell_hook__", "sql, bind");

  // Initial function update
  update_functions("");
}

// Documentation of sql function
REGISTER_HELP(SQLEXECUTE_SQL_BRIEF,
              "Sets the sql statement to be executed by this handler.");
REGISTER_HELP(
    SQLEXECUTE_SQL_PARAM,
    "@param statement A string containing the SQL statement to be executed.");
REGISTER_HELP(SQLEXECUTE_SQL_RETURNS, "@returns This SqlExecute object.");
REGISTER_HELP(
    SQLEXECUTE_SQL_DETAIL,
    "This function is called automatically when NodeSession.sql(sql) is called.");
REGISTER_HELP(
    SQLEXECUTE_SQL_DETAIL1,
    "Parameter binding is supported and can be done by using the \\b ? placeholder instead of passing values directly on the SQL statement.");
REGISTER_HELP(SQLEXECUTE_SQL_DETAIL2,
              "Parameters are bound in positional order.");
REGISTER_HELP(
    SQLEXECUTE_SQL_DETAIL3,
    "The actual execution of the SQL statement will occur when the execute() function is called.");
REGISTER_HELP(
    SQLEXECUTE_SQL_DETAIL4,
    "After this function invocation, the following functions can be invoked:");
REGISTER_HELP(SQLEXECUTE_SQL_DETAIL5, "@li bind(Value value)");
REGISTER_HELP(SQLEXECUTE_SQL_DETAIL6, "@li bind(List values)");
REGISTER_HELP(SQLEXECUTE_SQL_DETAIL7, "@li execute().");

/**
* $(SQLEXECUTE_SQL_BRIEF)
*
* $(SQLEXECUTE_SQL_PARAM)
* $(SQLEXECUTE_SQL_RETURNS)
*
* $(SQLEXECUTE_SQL_DETAIL)
*
* $(SQLEXECUTE_SQL_DETAIL1)
* $(SQLEXECUTE_SQL_DETAIL2)
*
* $(SQLEXECUTE_SQL_DETAIL3)
*
* $(SQLEXECUTE_SQL_DETAIL4)
* $(SQLEXECUTE_SQL_DETAIL5)
* $(SQLEXECUTE_SQL_DETAIL6)
* $(SQLEXECUTE_SQL_DETAIL7)
*/
#if DOXYGEN_JS
SqlExecute SqlExecute::sql(String statement) {}
#elif DOXYGEN_PY
SqlExecute SqlExecute::sql(str statement) {}
#endif
shcore::Value SqlExecute::sql(const shcore::Argument_list &args) {
  // Each method validates the received parameters
  args.ensure_count(1, get_function_name("sql").c_str());

  try {
    _sql = args.string_at(0);

    // Updates the exposed functions
    update_functions("sql");
  }
  CATCH_AND_TRANSLATE_CRUD_EXCEPTION(get_function_name("sql"));

  return Value(std::static_pointer_cast<Object_bridge>(shared_from_this()));
}

// Documentation of bind function
REGISTER_HELP(
    SQLEXECUTE_BIND_BRIEF,
    "Registers a parameter to be bound on the execution of the SQL statement.");
REGISTER_HELP(
    SQLEXECUTE_BIND_BRIEF1,
    "Registers a list of parameter to be bound on the execution of the SQL statement.");
REGISTER_HELP(SQLEXECUTE_BIND_PARAM, "@param value the value to be bound.");
REGISTER_HELP(SQLEXECUTE_BIND_PARAM1,
              "@param values the value list to be bound.");
REGISTER_HELP(SQLEXECUTE_BIND_RETURNS, "@returns This SqlExecute object.");
REGISTER_HELP(
    SQLEXECUTE_BIND_DETAIL,
    "This method can be invoked any number of times, each time the received parameter "
    "will be added to an internal binding list.");
REGISTER_HELP(SQLEXECUTE_BIND_DETAIL1, "This function can be invoked after:");
REGISTER_HELP(SQLEXECUTE_BIND_DETAIL2, "@li sql(String statement)");
REGISTER_HELP(SQLEXECUTE_BIND_DETAIL3, "@li bind(Value value)");
REGISTER_HELP(SQLEXECUTE_BIND_DETAIL4, "@li bind(List values)");
REGISTER_HELP(
    SQLEXECUTE_BIND_DETAIL5,
    "After this function invocation, the following functions can be invoked:");
REGISTER_HELP(SQLEXECUTE_BIND_DETAIL6, "@li bind(Value value)");
REGISTER_HELP(SQLEXECUTE_BIND_DETAIL7, "@li bind(List values)");
REGISTER_HELP(SQLEXECUTE_BIND_DETAIL8, "@li execute().");

/**
* $(SQLEXECUTE_BIND_BRIEF)
*
* $(SQLEXECUTE_BIND_PARAM)
* $(SQLEXECUTE_BIND_RETURNS)
*
* $(SQLEXECUTE_BIND_DETAIL)
*
* $(SQLEXECUTE_BIND_DETAIL1)
* $(SQLEXECUTE_BIND_DETAIL2)
* $(SQLEXECUTE_BIND_DETAIL3)
* $(SQLEXECUTE_BIND_DETAIL4)
*
* $(SQLEXECUTE_BIND_DETAIL5)
* $(SQLEXECUTE_BIND_DETAIL6)
* $(SQLEXECUTE_BIND_DETAIL7)
* $(SQLEXECUTE_BIND_DETAIL8)
*/
#if DOXYGEN_JS
SqlExecute SqlExecute::bind(Value value) {}
#elif DOXYGEN_PY
SqlExecute SqlExecute::bind(Value value) {}
#endif

/**
* $(SQLEXECUTE_BIND_BRIEF1)
*
* $(SQLEXECUTE_BIND_PARAM1)
* $(SQLEXECUTE_BIND_RETURNS)
*
* $(SQLEXECUTE_BIND_DETAIL)
*
* $(SQLEXECUTE_BIND_DETAIL1)
* $(SQLEXECUTE_BIND_DETAIL2)
* $(SQLEXECUTE_BIND_DETAIL3)
* $(SQLEXECUTE_BIND_DETAIL4)
*
* $(SQLEXECUTE_BIND_DETAIL5)
* $(SQLEXECUTE_BIND_DETAIL6)
* $(SQLEXECUTE_BIND_DETAIL7)
* $(SQLEXECUTE_BIND_DETAIL8)
*/
#if DOXYGEN_JS
SqlExecute SqlExecute::bind(List values) {}
#elif DOXYGEN_PY
SqlExecute SqlExecute::bind(list values) {}
#endif

shcore::Value SqlExecute::bind(const shcore::Argument_list &args) {
  args.ensure_count(1, get_function_name("bind").c_str());

  if (args[0].type == shcore::Array) {
    shcore::Value::Array_type_ref array = args.array_at(0);
    Value::Array_type::iterator index, end = array->end();

    for (index = array->begin(); index != end; index++)
      _parameters.push_back(*index);
  } else {
    _parameters.push_back(args[0]);
  }

  return Value(std::static_pointer_cast<Object_bridge>(shared_from_this()));
}

// Documentation of execute function
REGISTER_HELP(SQLEXECUTE_EXECUTE_BRIEF, "Executes the sql statement.");
REGISTER_HELP(SQLEXECUTE_EXECUTE_RETURNS, "@returns A SqlResult object.");
REGISTER_HELP(SQLEXECUTE_EXECUTE_DETAIL, "This function can be invoked after:");
REGISTER_HELP(SQLEXECUTE_EXECUTE_DETAIL1, "@li sql(String statement)");
REGISTER_HELP(SQLEXECUTE_EXECUTE_DETAIL2, "@li bind(Value value)");
REGISTER_HELP(SQLEXECUTE_EXECUTE_DETAIL3, "@li bind(List values)");

/**
* $(SQLEXECUTE_EXECUTE_BRIEF)
*
* $(SQLEXECUTE_EXECUTE_RETURNS)
*
* $(SQLEXECUTE_EXECUTE_DETAIL)
* $(SQLEXECUTE_EXECUTE_DETAIL1)
* $(SQLEXECUTE_EXECUTE_DETAIL2)
* $(SQLEXECUTE_EXECUTE_DETAIL3)
*/
#if DOXYGEN_JS
SqlResult SqlExecute::execute() {}
#elif DOXYGEN_PY
SqlResult SqlExecute::execute() {}
#endif
shcore::Value SqlExecute::execute(const shcore::Argument_list &args) {
  shcore::Value ret_val;

  args.ensure_count(0, get_function_name("execute").c_str());

  try {
    if (auto session = _session.lock()) {
      ret_val = session->_execute_sql(_sql, _parameters);
    } else {
      throw shcore::Exception::logic_error(
          "Unable to execute sql, no Session available");
    }
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name("execute"));

  return ret_val;
}
