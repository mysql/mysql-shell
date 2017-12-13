/*
 * Copyright (c) 2014, 2017, Oracle and/or its affiliates. All rights reserved.
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

#include "mod_mysqlx_session.h"

#include <stdlib.h>
#include <string.h>
#include <set>
#include <memory>
#include <string>
#include <algorithm>
#include <vector>
#include "modules/devapi/mod_mysqlx_session.h"

#include "modules/devapi/mod_mysqlx_constants.h"
#include "modules/devapi/mod_mysqlx_expression.h"
#include "modules/devapi/mod_mysqlx_resultset.h"
#include "modules/devapi/mod_mysqlx_schema.h"
#include "modules/devapi/mod_mysqlx_session_sql.h"
#include "modules/mod_utils.h"
#include "mysqlxtest_utils.h"
#include "scripting/object_factory.h"
#include "scripting/proxy_object.h"
#include "shellcore/shell_core.h"
#include "shellcore/base_shell.h"  // for options
#include "shellcore/shell_notifications.h"
#include "shellcore/utils_help.h"
#include "utils/logger.h"
#include "utils/utils_file.h"
#include "utils/utils_general.h"
#include "utils/utils_sqlstring.h"
#include "utils/utils_path.h"
#include "utils/utils_string.h"
#include "utils/utils_time.h"
#include "mysqlshdk/libs/utils/utils_uuid.h"

#if _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#define mystrdup _strdup
#else
#define mystrdup strdup
#endif

using namespace mysqlsh;
using namespace shcore;
using namespace mysqlsh::mysqlx;

#ifdef WIN32
#define strcasecmp _stricmp
#endif
REGISTER_HELP(SESSION_PARENTS, "ShellBaseSession");

// Documentation of BaseSession class
REGISTER_HELP(SESSION_DETAIL,
    "Document Store functionality can be used through this object, in addition "
    "to SQL.");
REGISTER_HELP(SESSION_DETAIL1,
    "This class allows performing database operations such as:");
REGISTER_HELP(SESSION_DETAIL2, "@li Schema management operations.");
REGISTER_HELP(SESSION_DETAIL3, "@li Access to relational tables.");
REGISTER_HELP(SESSION_DETAIL4, "@li Access to Document Store collections.");
REGISTER_HELP(SESSION_DETAIL5, "@li Enabling/disabling warning generation.");
REGISTER_HELP(SESSION_DETAIL6, "@li Retrieval of connection information.");

// Documentation of Session class
REGISTER_HELP(SESSION_INTERACTIVE_BRIEF,
              "Represents the currently open MySQL session.");

Session::Session() : _case_sensitive_table_names(false), _savepoint_counter(0) {
  init();
  _session = mysqlshdk::db::mysqlx::Session::create();
}

// Documentation of isOpen function
REGISTER_HELP(SESSION_ISOPEN_BRIEF, "Returns true if session is "\
  "known to be open.");
REGISTER_HELP(SESSION_ISOPEN_RETURNS, "@returns A boolean value "\
  "indicating if the session is still open.");
REGISTER_HELP(SESSION_ISOPEN_DETAIL, "Returns true if the session is "\
  "still open and false otherwise. Note: may return true if connection "\
  "is lost.");

/**
 * $(SESSION_ISOPEN_BRIEF)
 *
 * $(SESSION_ISOPEN_RETURNS)
 *
 * $(SESSION_ISOPEN_DETAIL)
 */
#if DOXYGEN_JS
Bool Session::isOpen() {}
#elif DOXYGEN_PY
bool Session::is_open() {}
#endif
bool Session::is_open() const {
  return _session->is_open();
}

shcore::Value Session::_is_open(const shcore::Argument_list &args) {
  args.ensure_count(0, get_function_name("isOpen").c_str());

  return shcore::Value(is_open());
}


Session::Session(std::shared_ptr<mysqlshdk::db::mysqlx::Session> session)
    : _case_sensitive_table_names(false) {
  init();

  // TODO(alfredo) maybe can remove _connection_options ivar
  _connection_options = session->get_connection_options();
  _session = session;

  _connection_id = _session->get_connection_id();
}

Session::Session(const Session &s)
    : ShellBaseSession(s),
      std::enable_shared_from_this<Session>(s),
      _case_sensitive_table_names(false),
      _savepoint_counter(0) {
  init();
  _session = mysqlshdk::db::mysqlx::Session::create();
}

Session::~Session() {
  if (is_open())
    close();
}

void Session::init() {
  add_method("close", std::bind(&Session::_close, this, _1), "data");
  add_method("setFetchWarnings",
             std::bind(&Session::set_fetch_warnings, this, _1), "data");
  add_method("startTransaction",
             std::bind(&Session::_start_transaction, this, _1), "data");
  add_method("commit", std::bind(&Session::_commit, this, _1), "data");
  add_method("rollback", std::bind(&Session::_rollback, this, _1), "data");

  add_method("createSchema", std::bind(&Session::_create_schema, this, _1),
             "data");
  add_method("getSchema", std::bind(&Session::_get_schema, this, _1),
             "name", shcore::String, NULL);
  add_method("getSchemas", std::bind(&Session::get_schemas, this, _1),
             NULL);
  add_method("dropSchema", std::bind(&Session::_drop_schema, this, _1),
             "data");
  add_method("setSavepoint", std::bind(&Session::_set_savepoint, this, _1),
             NULL);
  add_method("releaseSavepoint",
             std::bind(&Session::_release_savepoint, this, _1), NULL);
  add_method("rollbackTo", std::bind(&Session::_rollback_to, this, _1), NULL);
  add_property("uri", "getUri");
  add_property("defaultSchema", "getDefaultSchema");
  add_property("currentSchema", "getCurrentSchema");

  add_method("isOpen", std::bind(&Session::_is_open, this, _1), NULL);
  add_method("sql", std::bind(&Session::sql, this, _1), "sql",
            shcore::String, NULL);
  add_method("setCurrentSchema",
            std::bind(&Session::_set_current_schema, this, _1), "name",
            shcore::String, NULL);
  add_method("quoteName", std::bind(&Session::quote_name, this, _1), "name",
            shcore::String, NULL);

  _schemas.reset(new shcore::Value::Map_type);

  // Prepares the cache handling
  auto generator = [this](const std::string &name) {
    return shcore::Value::wrap<Schema>(new Schema(shared_from_this(), name));
  };
  update_schema_cache = [generator, this](const std::string &name,
                                          bool exists) {
    DatabaseObject::update_cache(name, generator, true, _schemas);
  };

  _session_uuid = shcore::Id_generator::new_document_id();
}

std::string Session::get_uuid() {
  std::string new_uuid = shcore::Id_generator::new_document_id();

  // To guarantee the same Node id us used
  // (in case a random number is generated when MAC address is not available)
  // We copy the _session_uuid on top of the new uuid node
  std::copy(_session_uuid.begin(),
            _session_uuid.begin() + 12,
            new_uuid.begin());

  return new_uuid;
}

void Session::connect(const mysqlshdk::db::Connection_options& data) {
  try {
    _connection_options = data;

    _session->connect(_connection_options);

    _connection_id = _session->get_connection_id();

    if (_connection_options.has_schema())
      update_schema_cache(_connection_options.get_schema(), true);
  }
  CATCH_AND_TRANSLATE();
}

void Session::set_option(const char *option, int value) {
  if (strcmp(option, "trace_protocol") == 0) {
    if (is_open())
      _session->enable_protocol_trace(value != 0);
  } else {
    throw shcore::Exception::argument_error(
        std::string("Unknown option ").append(option));
  }
}

uint64_t Session::get_connection_id() const {
  return _session->get_connection_id();
}

bool Session::table_name_compare(const std::string &n1,
                                     const std::string &n2) {
  if (_case_sensitive_table_names)
    return n1 == n2;
  else
    return strcasecmp(n1.c_str(), n2.c_str()) == 0;
}

// Documentation of close function
REGISTER_HELP(SESSION_CLOSE_BRIEF, "Closes the session.");
REGISTER_HELP(SESSION_CLOSE_DETAIL, "After closing the session it is "
  "still possible to make read only operations to gather metadata info, like "
  "getTable(name) or getSchemas().");

/**
* $(SESSION_CLOSE_BRIEF)
*
* $(SESSION_CLOSE_DETAIL)
*/
#if DOXYGEN_JS
Undefined Session::close() {}
#elif DOXYGEN_PY
None Session::close() {}
#endif
Value Session::_close(const shcore::Argument_list &args) {
  args.ensure_count(0, get_function_name("close").c_str());

  close();

  return shcore::Value();
}

void Session::close() {
  try {
    // Connection must be explicitly closed, we can't rely on the
    // automatic destruction because if shared across different objects
    // it may remain open

    log_warning(
        "Closing session: %s",
        uri(mysqlshdk::db::uri::formats::scheme_user_transport()).c_str());

    if (_session->is_open()) {
      _session->close();
    }
  } catch (std::exception &e) {
    log_warning("Error occurred closing session: %s", e.what());
  }

  _session = mysqlshdk::db::mysqlx::Session::create();
}

// Documentation of createSchema function
REGISTER_HELP(SESSION_CREATESCHEMA_BRIEF,
  "Creates a schema on the database and returns the corresponding object.");
REGISTER_HELP(SESSION_CREATESCHEMA_PARAM,
  "@param name A string value indicating the schema name.");
REGISTER_HELP(SESSION_CREATESCHEMA_RETURNS,
  "@returns The created schema object.");
REGISTER_HELP(SESSION_CREATESCHEMA_EXCEPTION,
  "@exception An exception is thrown if an error occurs creating the schema.");

/**
* $(SESSION_CREATESCHEMA_BRIEF)
*
* $(SESSION_CREATESCHEMA_PARAM)
* $(SESSION_CREATESCHEMA_RETURNS)
*
* $(SESSION_CREATESCHEMA_EXCEPTION)
*/
#if DOXYGEN_JS
Schema Session::createSchema(String name) {}
#elif DOXYGEN_PY
Schema Session::create_schema(str name) {}
#endif
Value Session::_create_schema(const shcore::Argument_list &args) {
  args.ensure_count(1, get_function_name("createSchema").c_str());

  std::string name;
  try {
    name = args.string_at(0);

    create_schema(name);
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name("createSchema"));

  return (*_schemas)[name];
}

void Session::create_schema(const std::string &name) {
  std::string statement = sqlstring("create schema ! charset='utf8mb4'", 0)
                          << name;
  execute_sql(statement);

  // if reached this point it indicates that there were no errors
  update_schema_cache(name, true);
}

void Session::set_current_schema(const std::string &name) {
  execute_sql(sqlstring("use !", 0) << name);
}

// Documentation of startTransaction function
REGISTER_HELP(SESSION_STARTTRANSACTION_BRIEF,
  "Starts a transaction context on the server.");
REGISTER_HELP(SESSION_STARTTRANSACTION_RETURNS,
  "@returns A SqlResult object.");
REGISTER_HELP(SESSION_STARTTRANSACTION_DETAIL,
  "Calling this function will turn off the autocommit mode on the server.");
REGISTER_HELP(SESSION_STARTTRANSACTION_DETAIL1,
  "All the operations executed after calling this function will take place "
  "only when commit() is called.");
REGISTER_HELP(SESSION_STARTTRANSACTION_DETAIL2,
  "All the operations executed after calling this function, will be discarded "
  "is rollback() is called.");
REGISTER_HELP(SESSION_STARTTRANSACTION_DETAIL3,
  "When commit() or rollback() are called, the server autocommit mode "
  "will return back to it's state before calling startTransaction().");

/**
* $(SESSION_STARTTRANSACTION_BRIEF)
*
* $(SESSION_STARTTRANSACTION_RETURNS)
*
* $(SESSION_STARTTRANSACTION_DETAIL)
*
* $(SESSION_STARTTRANSACTION_DETAIL1)
* $(SESSION_STARTTRANSACTION_DETAIL2)
* $(SESSION_STARTTRANSACTION_DETAIL3)
*/
#if DOXYGEN_JS
Result Session::startTransaction() {}
#elif DOXYGEN_PY
Result Session::start_transaction() {}
#endif
shcore::Value Session::_start_transaction(
    const shcore::Argument_list &args) {
  args.ensure_count(0, get_function_name("startTransaction").c_str());

  shcore::Value ret_val;
  try {
    ret_val = _execute_sql("start transaction", shcore::Argument_list());
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name("startTransaction"));

  return ret_val;
}

// Documentation of commit function
REGISTER_HELP(
    SESSION_COMMIT_BRIEF,
    "Commits all the operations executed after a call to startTransaction().");
REGISTER_HELP(SESSION_COMMIT_RETURNS, "@returns A SqlResult object.");
REGISTER_HELP(SESSION_COMMIT_DETAIL,
              "All the operations executed after calling startTransaction() "
              "will take place when this function is called.");
REGISTER_HELP(SESSION_COMMIT_DETAIL1,
              "The server autocommit mode will return back to it's state "
              "before calling startTransaction().");

/**
 * $(SESSION_COMMIT_BRIEF)
 *
 * $(SESSION_COMMIT_RETURNS)
 *
 * $(SESSION_COMMIT_DETAIL)
 *
 * $(SESSION_COMMIT_DETAIL1)
 */
#if DOXYGEN_JS
Result Session::commit() {}
#elif DOXYGEN_PY
Result Session::commit() {}
#endif
shcore::Value Session::_commit(const shcore::Argument_list &args) {
  args.ensure_count(0, get_function_name("commit").c_str());

  shcore::Value ret_val;

  try {
    ret_val = _execute_sql("commit", shcore::Argument_list());
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name("commit"));

  return ret_val;
}

// Documentation of rollback function
REGISTER_HELP(
    SESSION_ROLLBACK_BRIEF,
    "Discards all the operations executed after a call to startTransaction().");
REGISTER_HELP(SESSION_ROLLBACK_RETURNS, "@returns A SqlResult object.");
REGISTER_HELP(SESSION_ROLLBACK_DETAIL,
              "All the operations executed after calling startTransaction() "
              "will be discarded when this function is called.");
REGISTER_HELP(SESSION_ROLLBACK_DETAIL1,
              "The server autocommit mode will return back to it's state "
              "before calling startTransaction().");

/**
 * $(SESSION_ROLLBACK_BRIEF)
 *
 * $(SESSION_ROLLBACK_RETURNS)
 *
 * $(SESSION_ROLLBACK_DETAIL)
 *
 * $(SESSION_ROLLBACK_DETAIL1)
 */
#if DOXYGEN_JS
Result Session::rollback() {}
#elif DOXYGEN_PY
Result Session::rollback() {}
#endif
shcore::Value Session::_rollback(const shcore::Argument_list &args) {
  args.ensure_count(0, get_function_name("rollback").c_str());

  shcore::Value ret_val;

  try {
    ret_val = _execute_sql("rollback", shcore::Argument_list());
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name("rollback"));

  return ret_val;
}

REGISTER_HELP(
    SESSION_SETSAVEPOINT_BRIEF,
    "Creates or replaces a transaction savepoint with the given name.");
REGISTER_HELP(SESSION_SETSAVEPOINT_PARAM,
              "@param name Optional string with the name to be assigned to the "
              "transaction save point.");
REGISTER_HELP(SESSION_SETSAVEPOINT_RETURNS,
              "@returns The name of the transaction savepoint.");
REGISTER_HELP(
    SESSION_SETSAVEPOINT_DETAIL,
    "When working with transactions, using savepoints allows rolling back "
    "operations executed after the savepoint without terminating the "
    "transaction.");
REGISTER_HELP(SESSION_SETSAVEPOINT_DETAIL1,
              "Use this function to set a savepoint within a transaction.");
REGISTER_HELP(
    SESSION_SETSAVEPOINT_DETAIL2,
    "If this function is called with the same name of another savepoint set "
    "previously, the original savepoint will be deleted and a new one will be "
    "created.");
REGISTER_HELP(
    SESSION_SETSAVEPOINT_DETAIL3,
    "If the name is not provided an auto-generated name as 'TXSP#' will "
    "be assigned, where # is a consecutive number that guarantees uniqueness "
    "of the savepoint at Session level.");
/**
 * $(SESSION_SETSAVEPOINT_BRIEF)
 *
 * $(SESSION_SETSAVEPOINT_PARAM)
 *
 * $(SESSION_SETSAVEPOINT_RETURNS)
 *
 * $(SESSION_SETSAVEPOINT_DETAIL)
 *
 * $(SESSION_SETSAVEPOINT_DETAIL1)
 *
 * $(SESSION_SETSAVEPOINT_DETAIL2)
 *
 * $(SESSION_SETSAVEPOINT_DETAIL3)
 */
#if DOXYGEN_JS
String Session::setSavepoint(String name) {}
#elif DOXYGEN_PY
str Session::set_savepoint(str name) {}
#endif
shcore::Value Session::_set_savepoint(const shcore::Argument_list &args) {
  args.ensure_count(0, 1, get_function_name("setSavepoint").c_str());

  std::string new_name;
  try {
    if (args.size() == 1)
      new_name = args.string_at(0);
    else
      new_name = "TXSP" + std::to_string(++_savepoint_counter);

    _session->execute(sqlstring("savepoint !", 0) << new_name);
  } CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name("setSavepoint"));

  return shcore::Value(new_name);
}

REGISTER_HELP(SESSION_RELEASESAVEPOINT_BRIEF,
              "Removes a savepoint defined on a transaction.");
REGISTER_HELP(
    SESSION_RELEASESAVEPOINT_PARAM,
    "@param name string with the name of the savepoint to be removed.");
REGISTER_HELP(
    SESSION_RELEASESAVEPOINT_DETAIL,
    "Removes a named savepoint from the set of savepoints defined on "
    "the current transaction. This does not affect the operations executed on "
    "the transaction since no commit or rollback occurs.");
REGISTER_HELP(
    SESSION_RELEASESAVEPOINT_DETAIL1,
    "It is an error trying to remove a savepoint that does not exist.");
/**
 * $(SESSION_RELEASESAVEPOINT_BRIEF)
 *
 * $(SESSION_RELEASESAVEPOINT_PARAM)
 *
 * $(SESSION_RELEASESAVEPOINT_DETAIL)
 *
 * $(SESSION_RELEASESAVEPOINT_DETAIL1)
 */
#if DOXYGEN_JS
Undefined Session::releaseSavepoint(String name) {
}
#elif DOXYGEN_PY
None Session::release_savepoint(str name) {
}
#endif
shcore::Value Session::_release_savepoint(const shcore::Argument_list &args) {
  args.ensure_count(1, get_function_name("releaseSavepoint").c_str());
  try {
    _session->execute(sqlstring("release savepoint !", 0) << args.string_at(0));
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name("releaseSavepoint"));

  return shcore::Value();
}

REGISTER_HELP(SESSION_ROLLBACKTO_BRIEF,
  "Rolls back the transaction to the named savepoint without terminating the "
  "transaction.");
REGISTER_HELP(SESSION_ROLLBACKTO_PARAM,
  "@param name string with the name of the savepoint for the rollback "
  "operation.");
REGISTER_HELP(SESSION_ROLLBACKTO_DETAIL,
  "Modifications that the current transaction made to rows after the savepoint "
  "was defined will be rolled back.");
REGISTER_HELP(SESSION_ROLLBACKTO_DETAIL1,
  "The given savepoint will not be removed, but any savepoint defined after "
  "the given savepoint was defined will be removed.");
REGISTER_HELP(SESSION_ROLLBACKTO_DETAIL2,
  "It is an error calling this operation with an unexisting savepoint.");
/**
 * $(SESSION_ROLLBACKTO_BRIEF)
 *
 * $(SESSION_ROLLBACKTO_PARAM)
 *
 * $(SESSION_ROLLBACKTO_DETAIL)
 *
 * $(SESSION_ROLLBACKTO_DETAIL1)
 *
 * $(SESSION_ROLLBACKTO_DETAIL2)
 */

#if DOXYGEN_JS
Undefined Session::rollbackTo(String name) {}
#elif DOXYGEN_PY
None Session::rollback_to(str name) {}
#endif
shcore::Value Session::_rollback_to(const shcore::Argument_list &args) {
  args.ensure_count(1, get_function_name("rollbackTo").c_str());
  try {
    _session->execute(sqlstring("rollback to !", 0) << args.string_at(0));
  } CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name("rollbackTo"));

  return shcore::Value();
}

// Documentation of getDefaultSchema function
REGISTER_HELP(SESSION_DEFAULTSCHEMA_BRIEF,
              "Retrieves the Schema "
              "configured as default for the session.");
REGISTER_HELP(SESSION_GETDEFAULTSCHEMA_BRIEF,
              "Retrieves the Schema "
              "configured as default for the session.");
REGISTER_HELP(SESSION_GETDEFAULTSCHEMA_RETURNS,
              "@returns A Schema "
              "object or Null");

#if DOXYGEN_JS || DOXYGEN_PY
/**
 * $(SESSION_GETDEFAULTSCHEMA_BRIEF)
 *
 * $(SESSION_GETDEFAULTSCHEMA_RETURNS)
 */
#if DOXYGEN_JS
Schema Session::getDefaultSchema() {}
#elif DOXYGEN_PY
Schema Session::get_default_schema() {}
#endif

/**
* $(SHELLSESSION_GETURI_BRIEF)
*
* $(SESSION_GETURI_RETURNS)
*/
#if DOXYGEN_JS
String Session::getUri() {}
#elif DOXYGEN_PY
str Session::get_uri() {}
#endif
#endif

std::string Session::get_current_schema() {
  try {
    if (is_open()) {
      auto result = execute_sql("select schema()");
      if (auto row = result->fetch_one()) {
        if (row && !row->is_null(0))
          return row->get_string(0);
      }
    } else {
      throw std::logic_error("Not connected");
    }
  }
  CATCH_AND_TRANSLATE();
  return "";
}

// Documentation of getSchema function
REGISTER_HELP(
    SESSION_GETSCHEMA_BRIEF,
    "Retrieves a Schema object from the current session through it's name.");
REGISTER_HELP(SESSION_GETSCHEMA_PARAM,
              "@param name The name of the Schema object to be retrieved.");
REGISTER_HELP(SESSION_GETSCHEMA_RETURNS,
              "@returns The Schema object with the given name.");
REGISTER_HELP(SESSION_GETSCHEMA_EXCEPTION,
              "@exception An exception is thrown if the given name is not a "
              "valid schema.");

/**
 * $(SESSION_GETSCHEMA_BRIEF)
 *
 * $(SESSION_GETSCHEMA_PARAM)
 * $(SESSION_GETSCHEMA_RETURNS)
 * $(SESSION_GETSCHEMA_EXCEPTION)
 * \sa Schema
 */
#if DOXYGEN_JS
Schema Session::getSchema(String name) {}
#elif DOXYGEN_PY
Schema Session::get_schema(str name) {}
#endif
shcore::Object_bridge_ref Session::get_schema(const std::string &name) {
  shcore::Object_bridge_ref ret_val;
  std::string type = "Schema";

  if (name.empty())
    throw Exception::runtime_error("Schema name must be specified");

  std::string found_name = db_object_exists(type, name, "");

  if (!found_name.empty()) {
    update_schema_cache(found_name, true);

    ret_val = (*_schemas)[found_name].as_object();
  } else {
    update_schema_cache(name, false);

    throw Exception::runtime_error("Unknown database '" + name + "'");
  }

  auto dbobject = std::dynamic_pointer_cast<DatabaseObject>(ret_val);
  if (Base_shell::options().devapi_schema_object_handles)
    dbobject->update_cache();

  return ret_val;
}

shcore::Value Session::_get_schema(const shcore::Argument_list &args) {
  args.ensure_count(1, get_function_name("getSchema").c_str());
  shcore::Value ret_val;

  try {
    ret_val = shcore::Value(get_schema(args.string_at(0)));
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name("getSchema"));

  return ret_val;
}

// Documentation of  function
REGISTER_HELP(SESSION_GETSCHEMAS_BRIEF,
              "Retrieves the Schemas available on the session.");
REGISTER_HELP(
    SESSION_GETSCHEMAS_RETURNS,
    "@returns A List containing the Schema objects available on the session.");
/**
 * $(SESSION_GETSCHEMAS_BRIEF)
 *
 * $(SESSION_GETSCHEMAS_RETURNS)
 */
#if DOXYGEN_JS
List Session::getSchemas() {}
#elif DOXYGEN_PY
list Session::get_schemas() {}
#endif
shcore::Value Session::get_schemas(const shcore::Argument_list &args) {
  args.ensure_count(0, get_function_name("getSchemas").c_str());

  shcore::Value::Array_type_ref schemas(new shcore::Value::Array_type);

  try {
    if (is_open()) {
      std::shared_ptr<mysqlshdk::db::mysqlx::Result> result =
          execute_sql("show databases;");

      while (const mysqlshdk::db::IRow *row = result->fetch_one()) {
        std::string schema_name;
        if (!row->is_null(0))
          schema_name = row->get_string(0);
        // TODO(alfredo) review whether caching of all schemas shouldn't be off
        if (!schema_name.empty()) {
          update_schema_cache(schema_name, true);

          schemas->push_back((*_schemas)[schema_name]);
        }
      }
    } else {
      throw std::logic_error("Not connected");
    }
  }
  CATCH_AND_TRANSLATE();

  return shcore::Value(schemas);
}

REGISTER_HELP(SESSION_SETFETCHWARNINGS_BRIEF,
              "Enables or disables "
              "warning generation.");
REGISTER_HELP(SESSION_SETFETCHWARNINGS_PARAM,
              "@param enable Boolean "
              "value to enable or disable the warnings.");
REGISTER_HELP(SESSION_SETFETCHWARNINGS_DETAIL,
              "Warnings are generated "
              "sometimes when database operations are "
              "executed, such as SQL statements.");
REGISTER_HELP(SESSION_SETFETCHWARNINGS_DETAIL1,
              "On a Node session the "
              "warning generation is disabled by default. "
              "This function can be used to enable or disable the warning "
              "generation based"
              " on the received parameter.");
REGISTER_HELP(
    SESSION_SETFETCHWARNINGS_DETAIL2,
    "When warning generation "
    "is enabled, the warnings will be available through the result object "
    "returned on the executed operation.");

/**
 * $(SESSION_SETFETCHWARNINGS_BRIEF)
 *
 * $(SESSION_SETFETCHWARNINGS_PARAM)
 *
 * $(SESSION_SETFETCHWARNINGS_DETAIL)
 *
 * $(SESSION_SETFETCHWARNINGS_DETAIL1)
 *
 * $(SESSION_SETFETCHWARNINGS_DETAIL2)
 */
#if DOXYGEN_JS
Result Session::setFetchWarnings(Boolean enable) {}
#elif DOXYGEN_PY
Result Session::set_fetch_warnings(bool enable) {}
#endif
shcore::Value Session::set_fetch_warnings(
    const shcore::Argument_list &args) {
  args.ensure_count(1, get_function_name("setFetchWarnings").c_str());

  bool enable = args.bool_at(0);
  std::string command = enable ? "enable_notices" : "disable_notices";

  shcore::Argument_list command_args;
  command_args.push_back(Value("warnings"));

  shcore::Value ret_val;
  try {
    ret_val = executeAdminCommand(command, false, command_args);
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name("setFetchWarnings"));

  return ret_val;
}

// Documentation of dropSchema function
REGISTER_HELP(SESSION_DROPSCHEMA_BRIEF,
              "Drops the schema with the specified name.");
REGISTER_HELP(SESSION_DROPSCHEMA_RETURNS,
              "@returns Nothing.");

/**
 * $(SESSION_DROPSCHEMA_BRIEF)
 *
 * $(SESSION_DROPSCHEMA_RETURNS)
 *
 */
#if DOXYGEN_JS
Undefined Session::dropSchema(String name) {}
#elif DOXYGEN_PY
None Session::drop_schema(str name) {}
#endif
void Session::drop_schema(const std::string &name) {
  execute_sql(sqlstring("drop schema if exists !", 0) << name, shcore::Argument_list());
  if (_schemas->find(name) != _schemas->end())
    _schemas->erase(name);
}

shcore::Value Session::_drop_schema(const shcore::Argument_list &args) {
  std::string function = get_function_name("dropSchema");

  args.ensure_count(1, function.c_str());

  try {
    drop_schema(args[0].as_string());
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(function);

  return shcore::Value();
}

/*
 * This function verifies if the given object exist in the database, works for
 * schemas, tables, views and collections.
 * The check for tables, views and collections is done is done based on the
 * type. If type is not specified and an object with the name is found, the type
 * will be returned.
 *
 * Returns the name of the object as exists in the database.
 */
std::string Session::db_object_exists(std::string &type,
                                          const std::string &name,
                                          const std::string &owner) {
  Interruptible intr(this);

  std::shared_ptr<mysqlshdk::db::mysqlx::Result> result;
  if (type == "Schema") {
    result = execute_sql(sqlstring("show databases like ?", 0) << name);
    if (auto row = result->fetch_one()) {
      return row->get_string(0);
    }
  } else {
    shcore::Dictionary_t args = shcore::make_dict();
    args->set("schema", shcore::Value(owner));
    args->set("pattern", shcore::Value(name));
    result = execute_mysqlx_stmt("list_objects", args);
    if (auto row = result->fetch_one()) {
      std::string object_name = row->get_string(0);
      std::string object_type = row->get_string(1);

      if (type.empty()) {
        type = object_type;
        return object_name;
      } else {
        type = shcore::str_upper(type);
        if (type == object_type)
          return object_name;
      }
    }
  }
  return "";
}

shcore::Value::Map_type_ref Session::get_status() {
  shcore::Value::Map_type_ref status(new shcore::Value::Map_type);

  (*status)["SESSION_TYPE"] = shcore::Value("X");

  (*status)["NODE_TYPE"] = shcore::Value(get_node_type());

  (*status)["DEFAULT_SCHEMA"] =
    shcore::Value(_connection_options.has_schema() ?
                  _connection_options.get_schema() : "");

  try {
    std::shared_ptr<mysqlshdk::db::mysqlx::Result> result;

    result = execute_sql("select DATABASE(), USER() limit 1");
    const mysqlshdk::db::IRow *row = result->fetch_one();

    (*status)["CURRENT_SCHEMA"] =
        shcore::Value(row->is_null(0) ? "" : row->get_string(0));
    (*status)["CURRENT_USER"] =
        shcore::Value(row->is_null(1) ? "" : row->get_string(1));
    (*status)["CONNECTION_ID"] = shcore::Value(_session->get_connection_id());
    const char *cipher = _session->get_ssl_cipher();
    if (cipher) {
      result = execute_sql("show session status like 'mysqlx_ssl_version';");
      row = result->fetch_one();
      (*status)["SSL_CIPHER"] =
          shcore::Value(std::string(cipher) + " " + row->get_string(1));
    }
    //(*status)["SKIP_UPDATES"] = shcore::Value(???);

    // (*status)["SERVER_INFO"] = shcore::Value(_conn->get_server_info());

    (*status)["PROTOCOL_VERSION"] = shcore::Value("X protocol");
    unsigned long ver = mysql_get_client_version();
    std::stringstream sv;
    sv << ver/10000 << "." << (ver%10000)/100 << "." << ver % 100;
    (*status)["CLIENT_LIBRARY"] = shcore::Value(sv.str());
    // (*status)["INSERT_ID"] = shcore::Value(???);

    result = execute_sql(
        "select @@character_set_client, @@character_set_connection, "
        "@@character_set_server, @@character_set_database, "
        "concat(@@version, \" \", @@version_comment) as version, "
        "@@mysqlx_socket, @@mysqlx_port, @@datadir "
        "limit 1");
    row = result->fetch_one();
    if (row) {
      (*status)["CLIENT_CHARSET"] =
          shcore::Value(row->is_null(0) ? "" : row->get_string(0));
      (*status)["CONNECTION_CHARSET"] =
          shcore::Value(row->is_null(1) ? "" : row->get_string(1));
      (*status)["SERVER_CHARSET"] =
          shcore::Value(row->is_null(2) ? "" : row->get_string(2));
      (*status)["SCHEMA_CHARSET"] =
          shcore::Value(row->is_null(3) ? "" : row->get_string(3));
      (*status)["SERVER_VERSION"] =
          shcore::Value(row->is_null(4) ? "" : row->get_string(4));

      mysqlshdk::db::Transport_type transport_type =
          mysqlshdk::db::Transport_type::Tcp;
      if (_connection_options.has_transport_type()) {
        transport_type = _connection_options.get_transport_type();
        std::stringstream ss;
        if (_connection_options.has_host())
          ss << _connection_options.get_host() << " via ";
        else
          ss << "localhost via ";
        ss << to_string(transport_type);
        (*status)["CONNECTION"] = shcore::Value(ss.str());
      } else {
        (*status)["CONNECTION"] = shcore::Value("localhost via TCP/IP");
      }
      if (transport_type == mysqlshdk::db::Transport_type::Tcp) {
        (*status)["TCP_PORT"] =
            shcore::Value(row->is_null(6) ? "" : row->get_as_string(6));
      } else if (transport_type == mysqlshdk::db::Transport_type::Socket) {
        const std::string datadir = (row->is_null(5) ? "" : row->get_string(7));
        const std::string socket = (row->is_null(5) ? "" : row->get_string(5));
        const std::string socket_abs_path = shcore::path::normalize(
            shcore::path::join_path(std::vector<std::string>{datadir, socket}));
        (*status)["UNIX_SOCKET"] = shcore::Value(socket_abs_path);
      }
    }

    result = execute_sql("SHOW GLOBAL STATUS LIKE 'Uptime';");
    row = result->fetch_one();
    std::stringstream su;
    su << "Uptime: " << row->get_string(1) << " \n";
    (*status)["SERVER_STATS"] = shcore::Value(su.str());
    // (*status)["PROTOCOL_COMPRESSED"] = row->get_value(3);

    // SAFE UPDATES
  } catch (shcore::Exception &e) {
    (*status)["STATUS_ERROR"] = shcore::Value(e.format());
  }

  return status;
}

void Session::start_transaction() {
  if (_tx_deep == 0)
    execute_sql("start transaction", shcore::Argument_list());

  _tx_deep++;
}

void Session::commit() {
  _tx_deep--;

  assert(_tx_deep >= 0);

  if (_tx_deep == 0)
    execute_sql("commit", shcore::Argument_list());
}

void Session::rollback() {
  _tx_deep--;

  assert(_tx_deep >= 0);

  if (_tx_deep == 0)
    execute_sql("rollback", shcore::Argument_list());
}

std::string Session::query_one_string(const std::string &query, int field) {
  std::shared_ptr<mysqlshdk::db::mysqlx::Result> result = execute_sql(query);
  if (auto row = result->fetch_one()) {
    if (!row->is_null(field)) {
      return row->get_string(field);
    }
  }
  return "";
}

void Session::kill_query() {
  uint64_t cid = get_connection_id();

  auto session = mysqlshdk::db::mysqlx::Session::create();
  try {
    session->connect(_connection_options);
    session->query(shcore::sqlstring("kill query ?", 0) << cid);
  } catch (std::exception &e) {
    log_warning("Error cancelling SQL query: %s", e.what());
  }
}




/* Sql Execution Function */
shcore::Object_bridge_ref Session::raw_execute_sql(
    const std::string &query) {
  return _execute_sql(query, shcore::Argument_list()).as_object();
}

static ::xcl::Argument_value convert(const shcore::Value &value);

static ::xcl::Object convert_map(const shcore::Dictionary_t &args) {
  ::xcl::Object object;
  for (const auto &iter : *args) {
    object[iter.first] = convert(iter.second);
  }
  return object;
}

static ::xcl::Argument_value convert(const shcore::Value &value) {
  switch (value.type) {
    case shcore::Bool:
      return xcl::Argument_value(value.as_bool());
    case shcore::UInteger:
      return xcl::Argument_value(value.as_uint());
    case shcore::Integer:
      return xcl::Argument_value(value.as_int());
    case shcore::String:
      return xcl::Argument_value(value.as_string());
    case shcore::Float:
      return xcl::Argument_value(value.as_double());
    case shcore::Map:
      return xcl::Argument_value(convert_map(value.as_map()));
    case shcore::Null:
    case shcore::Array:
    case shcore::Object:
    case shcore::MapRef:
    case shcore::Function:
    case shcore::Undefined:
      break;
  }
  throw std::invalid_argument("Invalid argument value: " + value.descr());
}

static ::xcl::Arguments convert_args(const shcore::Dictionary_t &args) {
  ::xcl::Arguments cargs;
  cargs.push_back(xcl::Argument_value(convert_map(args)));
  return cargs;
}

static ::xcl::Arguments convert_args(const shcore::Argument_list &args) {
  ::xcl::Arguments cargs;

  for (const shcore::Value &arg : args) {
    cargs.push_back(convert(arg));
  }
  return cargs;
}


Value Session::executeAdminCommand(const std::string &command,
                                       bool expect_data,
                                       const Argument_list &args) {
  std::string function = class_name() + '.' + "executeAdminCommand";
  args.ensure_at_least(1, function.c_str());

  return _execute_stmt("xplugin", command, convert_args(args), expect_data);
}

shcore::Value Session::_execute_mysqlx_stmt(
    const std::string &command, const shcore::Dictionary_t &args) {
  return _execute_stmt("mysqlx", command, convert_args(args), true);
}

std::shared_ptr<mysqlshdk::db::mysqlx::Result> Session::execute_mysqlx_stmt(
    const std::string &command, const shcore::Dictionary_t &args) {
  return execute_stmt("mysqlx", command, convert_args(args));
}

shcore::Value Session::_execute_stmt(const std::string &ns,
                                         const std::string &command,
                                         const ::xcl::Arguments &args,
                                         bool expect_data) {
  MySQL_timer timer;
  timer.start();
  if (expect_data) {
    SqlResult *result =
        new SqlResult(execute_stmt(ns, command, args));
    timer.end();
    result->set_execution_time(timer.raw_duration());
    return shcore::Value::wrap(result);
  } else {
    Result *result = new Result(execute_stmt(ns, command, args));
    timer.end();
    result->set_execution_time(timer.raw_duration());
    return shcore::Value::wrap(result);
  }
  return {};
}

std::shared_ptr<mysqlshdk::db::mysqlx::Result> Session::execute_stmt(
    const std::string &ns, const std::string &command,
    const ::xcl::Arguments &args) {
  try {
    Interruptible intr(this);
    auto result = std::static_pointer_cast<mysqlshdk::db::mysqlx::Result>(
        _session->execute_stmt(ns, command, args));
    result->pre_fetch_rows();
    return result;
  } catch (const mysqlshdk::db::Error &e) {
    if (e.code() == 2006 || e.code() == 5166 || e.code() == 2013 ||
        e.code() == 2000) {
      ShellNotifications::get()->notify(
          "SN_SESSION_CONNECTION_LOST",
          std::dynamic_pointer_cast<Cpp_object_bridge>(shared_from_this()));
    }
      throw;
  }
}

shcore::Value Session::_execute_sql(const std::string &statement,
                                        const shcore::Argument_list &args) {
  MySQL_timer timer;
  timer.start();
  SqlResult *result = new SqlResult(execute_sql(statement, args));
  timer.end();
  result->set_execution_time(timer.raw_duration());
  return shcore::Value::wrap(result);
}

std::shared_ptr<mysqlshdk::db::mysqlx::Result> Session::execute_sql(
    const std::string &sql, const shcore::Argument_list &args) {
  return execute_stmt("sql", sql, convert_args(args));
}

std::shared_ptr<shcore::Object_bridge> Session::create(
    const shcore::Argument_list &args) {
  std::shared_ptr<Session> session(new Session());

  auto connection_options =
      mysqlsh::get_connection_options(args, mysqlsh::PasswordFormat::STRING);

  // DevAPI getClassicSession uses ssl-mode = REQUIRED by default if no
  // ssl-ca or ssl-capath are specified
  if (!connection_options.get_ssl_options().has_mode() &&
      !connection_options.has_value(mysqlshdk::db::kSslCa) &&
      !connection_options.has_value(mysqlshdk::db::kSslCaPath)) {
    connection_options.get_ssl_options().set_mode(
        mysqlshdk::db::Ssl_mode::Required);
  }

  session->connect(connection_options);

  shcore::ShellNotifications::get()->notify("SN_SESSION_CONNECTED", session);

  return std::dynamic_pointer_cast<shcore::Object_bridge>(session);
}

// Documentation of sql function
REGISTER_HELP(SESSION_SQL_BRIEF, "Creates a SqlExecute object to allow "
  "running the received SQL statement on the target MySQL Server.");
REGISTER_HELP(SESSION_SQL_PARAM, "@param sql A string containing the SQL "
  "statement to be executed.");
REGISTER_HELP(SESSION_SQL_RETURN, "@return A SqlExecute object.");
REGISTER_HELP(SESSION_SQL_DETAIL, "This method creates an SqlExecute "
  "object which is a SQL execution handler.");
REGISTER_HELP(SESSION_SQL_DETAIL1, "The SqlExecute class has functions "
  "that allow defining the way the statement will be executed and allows doing "
  "parameter binding.");
REGISTER_HELP(SESSION_SQL_DETAIL2, "The received SQL is set on the execution "
  "handler.");

/**
* $(SESSION_SQL_BRIEF)
*
* $(SESSION_SQL_PARAM)
*
* $(SESSION_SQL_RETURN)
*
* $(SESSION_SQL_DETAIL)
*
* $(SESSION_SQL_DETAIL1)
*
* $(SESSION_SQL_DETAIL2)
*
* JavaScript Example
* \code{.js}
* var sql = session.sql("select * from mydb.students where  age > ?");
* var result = sql.bind(18).execute();
* \endcode
* \sa SqlExecute
*/
#if DOXYGEN_JS
SqlExecute Session::sql(String sql) {}
#elif DOXYGEN_PY
SqlExecute Session::sql(str sql) {}
#endif
shcore::Value Session::sql(const shcore::Argument_list &args) {
  std::shared_ptr<SqlExecute> sql_execute(new SqlExecute(
      std::static_pointer_cast<Session>(shared_from_this())));

  return sql_execute->sql(args);
}

REGISTER_HELP(SESSION_URI_BRIEF, "Retrieves the URI for the current session.");
REGISTER_HELP(SESSION_GETURI_BRIEF, "Retrieves the URI for the current "
  "session.");
REGISTER_HELP(SESSION_GETURI_RETURNS, "@return A string representing the "
  "connection data.");
/**
* $(SESSION_GETURI_BRIEF)
*
* $(SESSION_GETURI_RETURNS)
*/

// Documentation of getCurrentSchema function
REGISTER_HELP(SESSION_CURRENTSCHEMA_BRIEF, "Retrieves the active schema "
  "on the session.");
REGISTER_HELP(SESSION_GETCURRENTSCHEMA_BRIEF, "Retrieves the active "
  "schema on the session.");
REGISTER_HELP(SESSION_GETCURRENTSCHEMA_RETURNS, "@return A Schema object "
  "if a schema is active on the session.");

/**
* $(SESSION_GETCURRENTSCHEMA_BRIEF)
*
* $(SESSION_GETCURRENTSCHEMA_RETURNS)
*/
#if DOXYGEN_JS
Schema Session::getCurrentSchema() {}
#elif DOXYGEN_PY
Schema Session::get_current_schema() {}
#endif

Value Session::get_member(const std::string &prop) const {
  Value ret_val;
  Session *session = const_cast<Session *>(this);

  if (prop == "__connection_info") {
    // FIXME: temporary code until ISession refactoring
    std::string info = _session->get_connection_info();
    return Value(info);
  }

  if (prop == "uri") {
    ret_val = shcore::Value(uri());
  } else if (prop == "defaultSchema") {
    if (_connection_options.has_schema()) {
      ret_val =
          shcore::Value(session->get_schema(_connection_options.get_schema()));
    } else {
      ret_val = Value::Null();
    }
  } else if (prop == "currentSchema") {
    std::string name = session->get_current_schema();

    if (!name.empty()) {
      ret_val = shcore::Value(session->get_schema(name));
    } else {
      ret_val = Value::Null();
    }
  } else {
    ret_val = ShellBaseSession::get_member(prop);
  }
  return ret_val;
}

// Documentation of quoteName function
REGISTER_HELP(SESSION_QUOTENAME_BRIEF, "Escapes the passed identifier.");
REGISTER_HELP(SESSION_QUOTENAME_RETURNS,
              "@return A String containing the escaped identifier.");

/**
 * $(SESSION_QUOTENAME_BRIEF)
 *
 * $(SESSION_QUOTENAME_RETURNS)
 */
#if DOXYGEN_JS
String Session::quoteName(String id) {}
#elif DOXYGEN_PY
str Session::quote_name(str id) {}
#endif
shcore::Value Session::quote_name(const shcore::Argument_list &args) {
  args.ensure_count(1, get_function_name("quoteName").c_str());

  if (args[0].type != shcore::String)
    throw shcore::Exception::type_error(
        "Argument #1 is expected to be a string");

  std::string id = args[0].as_string();

  return shcore::Value(get_quoted_name(id));
}

// Documentation of setCurrentSchema function
REGISTER_HELP(SESSION_SETCURRENTSCHEMA_BRIEF,
              "Sets the current schema for this session, and returns the "
              "schema object for it.");
REGISTER_HELP(SESSION_SETCURRENTSCHEMA_PARAM,
              "@param name the name of the new schema to switch to.");
REGISTER_HELP(SESSION_SETCURRENTSCHEMA_RETURNS,
              "@return the Schema object for the new schema.");
REGISTER_HELP(SESSION_SETCURRENTSCHEMA_DETAIL,
              "At the database level, this is equivalent at issuing the "
              "following SQL query:");
REGISTER_HELP(SESSION_SETCURRENTSCHEMA_DETAIL1,
              "  use <new-default-schema>;");

/**
 * $(SESSION_SETCURRENTSCHEMA_BRIEF)
 *
 * $(SESSION_SETCURRENTSCHEMA_PARAM)
 * $(SESSION_SETCURRENTSCHEMA_RETURNS)
 *
 * $(SESSION_SETCURRENTSCHEMA_DETAIL)
 * $(SESSION_SETCURRENTSCHEMA_DETAIL1)
 */
#if DOXYGEN_JS
Schema Session::setCurrentSchema(String name) {}
#elif DOXYGEN_PY
Schema Session::set_current_schema(str name) {}
#endif
shcore::Value Session::_set_current_schema(
    const shcore::Argument_list &args) {
  args.ensure_count(1, get_function_name("setCurrentSchema").c_str());

  try {
    if (is_open()) {
      std::string name = args[0].as_string();

      set_current_schema(name);
    } else {
      throw std::logic_error("Not connected");
    }
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name("setCurrentSchema"))

  return get_member("currentSchema");
}
