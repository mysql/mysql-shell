/*
 * Copyright (c) 2015, 2020, Oracle and/or its affiliates. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.0,
 * as published by the Free Software Foundation.
 *
 * This program is also distributed with certain software (including
 * but not limited to OpenSSL) that is licensed under separate terms, as
 * designated in a particular file or component or in included license
 * documentation.  The authors of MySQL hereby grant you an additional
 * permission to link the program and your derivative works with the
 * separately licensed software that they have included with MySQL.
 * This program is distributed in the hope that it will be useful,  but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 * the GNU General Public License, version 2.0, for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "modules/devapi/mod_mysqlx_session_sql.h"
#include <mysqld_error.h>
#include <memory>
#include "db/mysqlx/mysqlxclient_clean.h"
#include "modules/devapi/mod_mysqlx_session.h"
#include "modules/devapi/protobuf_bridge.h"
#include "mysqlshdk/include/scripting/type_info/custom.h"
#include "mysqlshdk/include/scripting/type_info/generic.h"
#include "mysqlxtest_utils.h"
#include "scripting/common.h"
#include "shellcore/utils_help.h"

using namespace std::placeholders;
using namespace shcore;

namespace mysqlsh {
namespace mysqlx {

REGISTER_HELP_CLASS(SqlExecute, mysqlx);
REGISTER_HELP(
    SQLEXECUTE_BRIEF,
    "Handler for execution SQL statements, supports parameter binding.");
REGISTER_HELP(SQLEXECUTE_DETAIL,
              "This object should only be created by calling the sql function "
              "at a Session instance.");

SqlExecute::SqlExecute(std::shared_ptr<Session> owner)
    : Dynamic_object(), _session(owner), m_execution_count(0) {
  // Exposes the methods available for chaining
  expose("sql", &SqlExecute::sql, "statement");
  expose("bind", &SqlExecute::bind, "value");
  expose("__shell_hook__", &SqlExecute::execute);
  expose("execute", &SqlExecute::execute);

  // Registers the dynamic function behavior
  register_dynamic_function(F::sql, F::bind | F::execute | F::__shell_hook__);

  // Initial function update
  enable_function(F::sql);
}

REGISTER_HELP_FUNCTION(sql, SqlExecute);
REGISTER_HELP_FUNCTION_TEXT(SQLEXECUTE_SQL, R"*(
Sets the sql statement to be executed by this handler.

@param statement A string containing the SQL statement to be executed.

@returns This SqlExecute object.

This function is called automatically when Session.sql(sql) is called.

Parameter binding is supported and can be done by using the \\b ? placeholder
instead of passing values directly on the SQL statement.

Parameters are bound in positional order.

The actual execution of the SQL statement will occur when the execute()
function is called.

After this function invocation, the following functions can be invoked:

@li bind(Value data)
@li execute().
)*");

/**
 * $(SQLEXECUTE_SQL_BRIEF)
 *
 * $(SQLEXECUTE_SQL)
 */
#if DOXYGEN_JS
SqlExecute SqlExecute::sql(String statement) {}
#elif DOXYGEN_PY
SqlExecute SqlExecute::sql(str statement) {}
#endif
std::shared_ptr<SqlExecute> SqlExecute::sql(const std::string &statement) {
  set_sql(statement);

  // Updates the exposed functions
  update_functions(F::sql);

  return shared_from_this();
}

REGISTER_HELP_FUNCTION(bind, SqlExecute);
REGISTER_HELP_FUNCTION_TEXT(SQLEXECUTE_BIND, R"*(
Registers a value or a list of values to be bound on the execution of the SQL
statement.

@param data the value or list of values to be bound.

@returns This SqlExecute object.

This method can be invoked any number of times, each time the received
parameters will be added to an internal binding list.

This function can be invoked after:
@li sql(String statement)
@li bind(Value data)

After this function invocation, the following functions can be invoked:
@li bind(Value data)
@li execute().
)*");
/**
 * $(SQLEXECUTE_BIND_BRIEF)
 *
 * $(SQLEXECUTE_BIND)
 */
#if DOXYGEN_JS
SqlExecute SqlExecute::bind(Value data) {}
#elif DOXYGEN_PY
SqlExecute SqlExecute::bind(Value data) {}
#endif
std::shared_ptr<SqlExecute> SqlExecute::bind(const shcore::Value &data) {
  if (data.type == shcore::Array) {
    for (const auto &v : *data.as_array()) {
      add_bind(v);
    }
  } else {
    add_bind(data);
  }

  return shared_from_this();
}

REGISTER_HELP_FUNCTION(execute, SqlExecute);
REGISTER_HELP_FUNCTION_TEXT(SQLEXECUTE_EXECUTE, R"*(
Executes the sql statement.

@returns A SqlResult object.

This function can be invoked after:
@li sql(String statement)
@li bind(Value data)
)*");
/**
 * $(SQLEXECUTE_EXECUTE_BRIEF)
 *
 * $(SQLEXECUTE_EXECUTE)
 */
#if DOXYGEN_JS
SqlResult SqlExecute::execute() {}
#elif DOXYGEN_PY
SqlResult SqlExecute::execute() {}
#endif

std::shared_ptr<SqlResult> SqlExecute::execute() {
  std::shared_ptr<SqlResult> ret_val;

  if (auto session = _session.lock()) {
    // Prepared statements are used when the statement is executed a
    // more than once after the last statement update
    if (session->allow_prepared_statements() && m_execution_count >= 1) {
      // If the statement has not been prepared, then it gets prepared
      // If the prepare fails because of ER_UNKNOWN_COM_ERROR, then prepare
      // is not supported and should be disabled
      if (!m_prep_stmt.has_stmt_id()) {
        try {
          m_prep_stmt.set_stmt_id(session->session()->next_prep_stmt_id());

          m_prep_stmt.mutable_stmt()->set_type(
              Mysqlx::Prepare::Prepare_OneOfMessage_Type_STMT);
          m_prep_stmt.mutable_stmt()->mutable_stmt_execute()->set_stmt(_sql);
          m_prep_stmt.mutable_stmt()->mutable_stmt_execute()->set_namespace_(
              "sql");

          session->session()->prepare_stmt(m_prep_stmt);
        } catch (const mysqlshdk::db::Error &error) {
          m_prep_stmt.clear_stmt_id();
          if (ER_UNKNOWN_COM_ERROR == error.code()) {
            session->disable_prepared_statements();
            ret_val = execute_sql(session);
          } else {
            throw shcore::Exception::mysql_error_with_code_and_state(
                error.what(), error.code(), error.sqlstate());
          }
        }
      }

      // If preparation succeeded then does normal prepared statement
      // execution any execution error should just be reported
      if (m_prep_stmt.has_stmt_id()) {
        Mysqlx::Prepare::Execute execute;
        execute.set_stmt_id(m_prep_stmt.stmt_id());
        insert_bound_values(_parameters, execute.mutable_args());
        _parameters->clear();

        auto result = std::static_pointer_cast<mysqlshdk::db::mysqlx::Result>(
            session->session()->execute_prep_stmt(execute));
        ret_val = std::make_shared<SqlResult>(result);
      }
    } else {
      ret_val = execute_sql(session);
    }
  } else {
    throw shcore::Exception::logic_error(
        "Unable to execute sql, no Session available");
  }
  m_execution_count++;

  return ret_val;
}

std::shared_ptr<SqlResult> SqlExecute::execute_sql(
    const std::shared_ptr<mysqlsh::mysqlx::Session> &session) {
  std::shared_ptr<SqlResult> result;
  try {
    auto sresult = std::dynamic_pointer_cast<mysqlshdk::db::mysqlx::Result>(
        session->execute_sql(_sql, _parameters));

    result = std::make_shared<SqlResult>(sresult);
    _parameters->clear();
  } catch (...) {
    _parameters->clear();
    throw;
  }

  return result;
}

}  // namespace mysqlx
}  // namespace mysqlsh
