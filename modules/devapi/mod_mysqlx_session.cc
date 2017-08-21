/*
 * Copyright (c) 2014, 2017, Oracle and/or its affiliates. All rights reserved.
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

#include "mod_mysqlx_session.h"

#include <stdlib.h>
#include <string.h>
#include <set>
#include <memory>
#include <string>
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
#include "shellcore/shell_notifications.h"
#include "shellcore/utils_help.h"
#include "utils/logger.h"
#include "utils/utils_file.h"
#include "utils/utils_general.h"
#include "utils/utils_sqlstring.h"
#include "utils/utils_string.h"
#include "utils/utils_time.h"

#if _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#define mystrdup _strdup
#else
#define mystrdup strdup
#endif

using namespace mysqlsh;
using namespace shcore;
using namespace mysqlsh::mysqlx;

REGISTER_OBJECT(mysqlx, NodeSession);
REGISTER_OBJECT(mysqlx, Expression);
REGISTER_OBJECT(mysqlx, Type);
REGISTER_OBJECT(mysqlx, IndexType);

#ifdef WIN32
#define strcasecmp _stricmp
#endif

// Documentation of NodeSession class
REGISTER_HELP(NODESESSION_BRIEF,
              "Represents the currently open MySQL session.");
REGISTER_HELP(NODESESSION_DETAIL,
              "Document Store functionality can be used through this "
              "object, in addition to SQL.");

NodeSession::NodeSession() : _case_sensitive_table_names(false) {
  init();
}

// Documentation of isOpen function
REGISTER_HELP(NODESESSION_ISOPEN_BRIEF,
              "Verifies if the session is still open.");
/**
 * $(NODESESSION_ISOPEN_BRIEF)
 */
#if DOXYGEN_JS
Bool NodeSession::isOpen() {}
#elif DOXYGEN_PY
bool NodeSession::is_open() {}
#endif
bool NodeSession::is_open() const {
  return _session->valid();
}

shcore::Value NodeSession::_is_open(const shcore::Argument_list &args) {
  args.ensure_count(0, get_function_name("isOpen").c_str());

  return shcore::Value(is_open());
}

NodeSession::NodeSession(const NodeSession &s)
    : ShellBaseSession(s), _case_sensitive_table_names(false) {
  init();
}

NodeSession::~NodeSession() {
  if (is_open())
    close();
}

void NodeSession::init() {
  add_method("close", std::bind(&NodeSession::_close, this, _1), "data");
  add_method("setFetchWarnings",
             std::bind(&NodeSession::set_fetch_warnings, this, _1), "data");
  add_method("startTransaction",
             std::bind(&NodeSession::_start_transaction, this, _1), "data");
  add_method("commit", std::bind(&NodeSession::_commit, this, _1), "data");
  add_method("rollback", std::bind(&NodeSession::_rollback, this, _1), "data");

  add_method("createSchema", std::bind(&NodeSession::_create_schema, this, _1),
             "data");
  add_method("getSchema", std::bind(&NodeSession::_get_schema, this, _1),
             "name", shcore::String, NULL);
  add_method("getSchemas", std::bind(&NodeSession::get_schemas, this, _1),
             NULL);
  add_method("dropSchema", std::bind(&NodeSession::_drop_schema, this, _1),
             "data");
  add_method("dropTable",
             std::bind(&NodeSession::drop_schema_object, this, _1, "Table"),
             "data");
  add_method(
      "dropCollection",
      std::bind(&NodeSession::drop_schema_object, this, _1, "Collection"),
      "data");
  add_method("dropView",
             std::bind(&NodeSession::drop_schema_object, this, _1, "View"),
             "data");
  add_property("uri", "getUri");
  add_property("defaultSchema", "getDefaultSchema");
  add_property("currentSchema", "getCurrentSchema");

  add_method("isOpen", std::bind(&NodeSession::_is_open, this, _1), NULL);
  add_method("sql", std::bind(&NodeSession::sql, this, _1), "sql",
            shcore::String, NULL);
  add_method("setCurrentSchema",
            std::bind(&NodeSession::_set_current_schema, this, _1), "name",
            shcore::String, NULL);
  add_method("quoteName", std::bind(&NodeSession::quote_name, this, _1), "name",
            shcore::String, NULL);

  _schemas.reset(new shcore::Value::Map_type);
  _session = mysqlshdk::db::mysqlx::Session::create();

  // Prepares the cache handling
  auto generator = [this](const std::string &name) {
    return shcore::Value::wrap<Schema>(new Schema(shared_from_this(), name));
  };
  update_schema_cache = [generator, this](const std::string &name,
                                          bool exists) {
    DatabaseObject::update_cache(name, generator, true, _schemas);
  };
}

void NodeSession::connect(const mysqlshdk::db::Connection_options& data) {
  try {
    _connection_options = data;

    _session->connect(_connection_options);

    _connection_id = _session->get_connection_id();

    if (_connection_options.has_schema())
      update_schema_cache(_connection_options.get_schema(), true);
  }
  CATCH_AND_TRANSLATE();
}

void NodeSession::set_option(const char *option, int value) {
  if (strcmp(option, "trace_protocol") == 0) {
    if (is_open())
      _session->enable_protocol_trace(value != 0);
  } else {
    throw shcore::Exception::argument_error(
        std::string("Unknown option ").append(option));
  }
}

uint64_t NodeSession::get_connection_id() const {
  return _session->get_connection_id();
}

bool NodeSession::table_name_compare(const std::string &n1,
                                     const std::string &n2) {
  if (_case_sensitive_table_names)
    return n1 == n2;
  else
    return strcasecmp(n1.c_str(), n2.c_str()) == 0;
}

// Documentation of close function
REGISTER_HELP(NODESESSION_CLOSE_BRIEF, "Closes the session.");
REGISTER_HELP(NODESESSION_CLOSE_DETAIL,
              "After closing the session it is "
              "still possible to make read only operations "
              "to gather metadata info, like getTable(name) or getSchemas().");

/**
 * $(NODESESSION_CLOSE_BRIEF)
 *
 * $(NODESESSION_CLOSE_DETAIL)
 */
#if DOXYGEN_JS
Undefined NodeSession::close() {}
#elif DOXYGEN_PY
None NodeSession::close() {}
#endif
Value NodeSession::_close(const shcore::Argument_list &args) {
  args.ensure_count(0, get_function_name("close").c_str());

  close();

  return shcore::Value();
}

void NodeSession::close() {
  bool did_close = false;
  try {
    // Connection must be explicitly closed, we can't rely on the
    // automatic destruction because if shared across different objects
    // it may remain open

    log_warning("Closing session: %s", uri().c_str());

    if (_session->valid()) {
      did_close = true;
      _session->close();
    }
  } catch (std::exception &e) {
    log_warning("Error occurred closing session: %s", e.what());
  }

  if (did_close) {
    try {
      // this shouldn't be getting clled from teh d-tor...
      ShellNotifications::get()->notify("SN_SESSION_CLOSED", shared_from_this());
    } catch (std::bad_weak_ptr) {
      ShellNotifications::get()->notify("SN_SESSION_CLOSED", {});
    }
  }

  _session = mysqlshdk::db::mysqlx::Session::create();
}

// Documentation of createSchema function
REGISTER_HELP(
    NODESESSION_CREATESCHEMA_BRIEF,
    "Creates a schema on the database and returns the corresponding object.");
REGISTER_HELP(NODESESSION_CREATESCHEMA_PARAM,
              "@param name A string value indicating the schema name.");
REGISTER_HELP(NODESESSION_CREATESCHEMA_RETURNS,
              "@returns The created schema object.");
REGISTER_HELP(NODESESSION_CREATESCHEMA_EXCEPTION,
              "@exception An exception is thrown if an error occurs creating "
              "the Session.");

/**
 * $(NODESESSION_CREATESCHEMA_BRIEF)
 *
 * $(NODESESSION_CREATESCHEMA_PARAM)
 * $(NODESESSION_CREATESCHEMA_RETURNS)
 *
 * $(NODESESSION_CREATESCHEMA_EXCEPTION)
 */
#if DOXYGEN_JS
Schema NodeSession::createSchema(String name) {}
#elif DOXYGEN_PY
Schema NodeSession::create_schema(str name) {}
#endif
Value NodeSession::_create_schema(const shcore::Argument_list &args) {
  args.ensure_count(1, get_function_name("createSchema").c_str());

  std::string name;
  try {
    name = args.string_at(0);

    create_schema(name);
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name("createSchema"));

  return (*_schemas)[name];
}

void NodeSession::create_schema(const std::string &name) {
  std::string statement = sqlstring("create schema ! charset='utf8mb4'", 0)
                          << name;
  execute_sql(statement);

  // if reached this point it indicates that there were no errors
  update_schema_cache(name, true);
}

void NodeSession::set_current_schema(const std::string &name) {
  execute_sql(sqlstring("use !", 0) << name);
}

// Documentation of startTransaction function
REGISTER_HELP(NODESESSION_STARTTRANSACTION_BRIEF,
              "Starts a transaction context on the server.");
REGISTER_HELP(NODESESSION_STARTTRANSACTION_RETURNS,
              "@returns A SqlResult object.");
REGISTER_HELP(
    NODESESSION_STARTTRANSACTION_DETAIL,
    "Calling this function will turn off the autocommit mode on the server.");
REGISTER_HELP(NODESESSION_STARTTRANSACTION_DETAIL1,
              "All the operations executed after calling this function will "
              "take place only when commit() is called.");
REGISTER_HELP(NODESESSION_STARTTRANSACTION_DETAIL2,
              "All the operations executed after calling this function, will "
              "be discarded is rollback() is called.");
REGISTER_HELP(
    NODESESSION_STARTTRANSACTION_DETAIL3,
    "When commit() or rollback() are called, the server autocommit mode "
    "will return back to it's state before calling startTransaction().");

/**
 * $(NODESESSION_STARTTRANSACTION_BRIEF)
 *
 * $(NODESESSION_STARTTRANSACTION_RETURNS)
 *
 * $(NODESESSION_STARTTRANSACTION_DETAIL)
 *
 * $(NODESESSION_STARTTRANSACTION_DETAIL1)
 * $(NODESESSION_STARTTRANSACTION_DETAIL2)
 * $(NODESESSION_STARTTRANSACTION_DETAIL3)
 */
#if DOXYGEN_JS
Result NodeSession::startTransaction() {}
#elif DOXYGEN_PY
Result NodeSession::start_transaction() {}
#endif
shcore::Value NodeSession::_start_transaction(
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
    NODESESSION_COMMIT_BRIEF,
    "Commits all the operations executed after a call to startTransaction().");
REGISTER_HELP(NODESESSION_COMMIT_RETURNS, "@returns A SqlResult object.");
REGISTER_HELP(NODESESSION_COMMIT_DETAIL,
              "All the operations executed after calling startTransaction() "
              "will take place when this function is called.");
REGISTER_HELP(NODESESSION_COMMIT_DETAIL1,
              "The server autocommit mode will return back to it's state "
              "before calling startTransaction().");

/**
 * $(NODESESSION_COMMIT_BRIEF)
 *
 * $(NODESESSION_COMMIT_RETURNS)
 *
 * $(NODESESSION_COMMIT_DETAIL)
 *
 * $(NODESESSION_COMMIT_DETAIL1)
 */
#if DOXYGEN_JS
Result NodeSession::commit() {}
#elif DOXYGEN_PY
Result NodeSession::commit() {}
#endif
shcore::Value NodeSession::_commit(const shcore::Argument_list &args) {
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
    NODESESSION_ROLLBACK_BRIEF,
    "Discards all the operations executed after a call to startTransaction().");
REGISTER_HELP(NODESESSION_ROLLBACK_RETURNS, "@returns A SqlResult object.");
REGISTER_HELP(NODESESSION_ROLLBACK_DETAIL,
              "All the operations executed after calling startTransaction() "
              "will be discarded when this function is called.");
REGISTER_HELP(NODESESSION_ROLLBACK_DETAIL1,
              "The server autocommit mode will return back to it's state "
              "before calling startTransaction().");

/**
 * $(NODESESSION_ROLLBACK_BRIEF)
 *
 * $(NODESESSION_ROLLBACK_RETURNS)
 *
 * $(NODESESSION_ROLLBACK_DETAIL)
 *
 * $(NODESESSION_ROLLBACK_DETAIL1)
 */
#if DOXYGEN_JS
Result NodeSession::rollback() {}
#elif DOXYGEN_PY
Result NodeSession::rollback() {}
#endif
shcore::Value NodeSession::_rollback(const shcore::Argument_list &args) {
  args.ensure_count(0, get_function_name("rollback").c_str());

  shcore::Value ret_val;

  try {
    ret_val = _execute_sql("rollback", shcore::Argument_list());
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name("rollback"));

  return ret_val;
}

// Documentation of getDefaultSchema function
REGISTER_HELP(NODESESSION_DEFAULTSCHEMA_BRIEF,
              "Retrieves the Schema "
              "configured as default for the session.");
REGISTER_HELP(NODESESSION_GETDEFAULTSCHEMA_BRIEF,
              "Retrieves the Schema "
              "configured as default for the session.");
REGISTER_HELP(NODESESSION_GETDEFAULTSCHEMA_RETURNS,
              "@returns A Schema "
              "object or Null");

#if DOXYGEN_JS || DOXYGEN_PY
/**
 * $(NODESESSION_GETDEFAULTSCHEMA_BRIEF)
 *
 * $(NODESESSION_GETDEFAULTSCHEMA_RETURNS)
 */
#if DOXYGEN_JS
Schema NodeSession::getDefaultSchema() {}
#elif DOXYGEN_PY
Schema NodeSession::get_default_schema() {}
#endif

/**
 * $(SHELLNODESESSION_GETURI_BRIEF)
 *
 * $(NODESESSION_GETURI_RETURNS)
 */
#if DOXYGEN_JS
String NodeSession::getUri() {}
#elif DOXYGEN_PY
str NodeSession::get_uri() {}
#endif
#endif

std::string NodeSession::_retrieve_current_schema() {
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
    NODESESSION_GETSCHEMA_BRIEF,
    "Retrieves a Schema object from the current session through it's name.");
REGISTER_HELP(NODESESSION_GETSCHEMA_PARAM,
              "@param name The name of the Schema object to be retrieved.");
REGISTER_HELP(NODESESSION_GETSCHEMA_RETURNS,
              "@returns The Schema object with the given name.");
REGISTER_HELP(NODESESSION_GETSCHEMA_EXCEPTION,
              "@exception An exception is thrown if the given name is not a "
              "valid schema.");

/**
 * $(NODESESSION_GETSCHEMA_BRIEF)
 *
 * $(NODESESSION_GETSCHEMA_PARAM)
 * $(NODESESSION_GETSCHEMA_RETURNS)
 * $(NODESESSION_GETSCHEMA_EXCEPTION)
 * \sa Schema
 */
#if DOXYGEN_JS
Schema NodeSession::getSchema(String name) {}
#elif DOXYGEN_PY
Schema NodeSession::get_schema(str name) {}
#endif
shcore::Object_bridge_ref NodeSession::get_schema(const std::string &name) {
  auto ret_val = ShellBaseSession::get_schema(name);

  auto dbobject = std::dynamic_pointer_cast<DatabaseObject>(ret_val);
  dbobject->update_cache();

  return ret_val;
}

shcore::Value NodeSession::_get_schema(const shcore::Argument_list &args) {
  args.ensure_count(1, get_function_name("getSchema").c_str());
  shcore::Value ret_val;

  try {
    ret_val = shcore::Value(get_schema(args.string_at(0)));
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name("getSchema"));

  return ret_val;
}

// Documentation of  function
REGISTER_HELP(NODESESSION_GETSCHEMAS_BRIEF,
              "Retrieves the Schemas available on the session.");
REGISTER_HELP(
    NODESESSION_GETSCHEMAS_RETURNS,
    "@returns A List containing the Schema objects available on the session.");
/**
 * $(NODESESSION_GETSCHEMAS_BRIEF)
 *
 * $(NODESESSION_GETSCHEMAS_RETURNS)
 */
#if DOXYGEN_JS
List NodeSession::getSchemas() {}
#elif DOXYGEN_PY
list NodeSession::get_schemas() {}
#endif
shcore::Value NodeSession::get_schemas(const shcore::Argument_list &args) {
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

REGISTER_HELP(NODESESSION_SETFETCHWARNINGS_BRIEF,
              "Enables or disables "
              "warning generation.");
REGISTER_HELP(NODESESSION_SETFETCHWARNINGS_PARAM,
              "@param enable Boolean "
              "value to enable or disable the warnings.");
REGISTER_HELP(NODESESSION_SETFETCHWARNINGS_DETAIL,
              "Warnings are generated "
              "sometimes when database operations are "
              "executed, such as SQL statements.");
REGISTER_HELP(NODESESSION_SETFETCHWARNINGS_DETAIL1,
              "On a Node session the "
              "warning generation is disabled by default. "
              "This function can be used to enable or disable the warning "
              "generation based"
              " on the received parameter.");
REGISTER_HELP(
    NODESESSION_SETFETCHWARNINGS_DETAIL2,
    "When warning generation "
    "is enabled, the warnings will be available through the result object "
    "returned on the executed operation.");

/**
 * $(NODESESSION_SETFETCHWARNINGS_BRIEF)
 *
 * $(NODESESSION_SETFETCHWARNINGS_PARAM)
 *
 * $(NODESESSION_SETFETCHWARNINGS_DETAIL)
 *
 * $(NODESESSION_SETFETCHWARNINGS_DETAIL1)
 *
 * $(NODESESSION_SETFETCHWARNINGS_DETAIL2)
 */
#if DOXYGEN_JS
Result NodeSession::setFetchWarnings(Boolean enable) {}
#elif DOXYGEN_PY
Result NodeSession::set_fetch_warnings(bool enable) {}
#endif
shcore::Value NodeSession::set_fetch_warnings(
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
REGISTER_HELP(NODESESSION_DROPSCHEMA_BRIEF,
              "Drops the schema with the specified name.");
REGISTER_HELP(NODESESSION_DROPSCHEMA_RETURNS,
              "@returns A SqlResult object if succeeded.");
REGISTER_HELP(NODESESSION_DROPSCHEMA_EXCEPTION,
              "@exception An error is raised if the schema did not exist.");

/**
 * $(NODESESSION_DROPSCHEMA_BRIEF)
 *
 * $(NODESESSION_DROPSCHEMA_RETURNS)
 *
 * $(NODESESSION_DROPSCHEMA_EXCEPTION)
 */
#if DOXYGEN_JS
Result NodeSession::dropSchema(String name) {}
#elif DOXYGEN_PY
Result NodeSession::drop_schema(str name) {}
#endif
void NodeSession::drop_schema(const std::string &name) {
  execute_sql(sqlstring("drop schema !", 0) << name, shcore::Argument_list());

  if (_schemas->find(name) != _schemas->end())
    _schemas->erase(name);
}

shcore::Value NodeSession::_drop_schema(const shcore::Argument_list &args) {
  std::string function = get_function_name("dropSchema");

  args.ensure_count(1, function.c_str());

  try {
    drop_schema(args[0].as_string());
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(function);

  return shcore::Value();
}

// Documentation of dropTable function
REGISTER_HELP(NODESESSION_DROPTABLE_BRIEF,
              "Drops a table from the specified schema.");
REGISTER_HELP(NODESESSION_DROPTABLE_RETURNS,
              "@returns A SqlResult object if succeeded.");
REGISTER_HELP(NODESESSION_DROPTABLE_EXCEPTION,
              "@exception An error is raised if the table did not exist.");

/**
 * $(NODESESSION_DROPTABLE_BRIEF)
 *
 * $(NODESESSION_DROPTABLE_RETURNS)
 *
 * $(NODESESSION_DROPTABLE_EXCEPTION)
 */
#if DOXYGEN_JS
Result NodeSession::dropTable(String schema, String name) {}
#elif DOXYGEN_PY
Result NodeSession::drop_table(str schema, str name) {}
#endif

// Documentation of dropCollection function
REGISTER_HELP(NODESESSION_DROPCOLLECTION_BRIEF,
              "Drops a collection from the specified schema.");
REGISTER_HELP(NODESESSION_DROPCOLLECTION_RETURNS,
              "@returns A SqlResult object if succeeded.");
REGISTER_HELP(NODESESSION_DROPCOLLECTION_EXCEPTION,
              "@exception An error is raised if the collection did not exist.");

/**
 * $(NODESESSION_DROPCOLLECTION_BRIEF)
 *
 * $(NODESESSION_DROPCOLLECTION_RETURNS)
 *
 * $(NODESESSION_DROPCOLLECTION_EXCEPTION)
 */
#if DOXYGEN_JS
Result NodeSession::dropCollection(String schema, String name) {}
#elif DOXYGEN_PY
Result NodeSession::drop_collection(str schema, str name) {}
#endif

// Documentation of dropView function
REGISTER_HELP(NODESESSION_DROPVIEW_BRIEF,
              "Drops a view from the specified schema.");
REGISTER_HELP(NODESESSION_DROPVIEW_RETURNS,
              "@returns A SqlResult object if succeeded.");
REGISTER_HELP(NODESESSION_DROPVIEW_EXCEPTION,
              "@exception An error is raised if the view did not exist.");

/**
 * $(NODESESSION_DROPVIEW_BRIEF)
 *
 * $(NODESESSION_DROPVIEW_RETURNS)
 *
 * $(NODESESSION_DROPVIEW_EXCEPTION)
 */
#if DOXYGEN_JS
Result NodeSession::dropView(String schema, String name) {}
#elif DOXYGEN_PY
Result NodeSession::drop_view(str schema, str name) {}
#endif

shcore::Value NodeSession::drop_schema_object(const shcore::Argument_list &args,
                                              const std::string &type) {
  std::string function = get_function_name("drop" + type);

  args.ensure_count(2, function.c_str());

  if (args[0].type != shcore::String)
    throw shcore::Exception::argument_error(
        function + ": Argument #1 is expected to be a string");

  if (args[1].type != shcore::String)
    throw shcore::Exception::argument_error(
        function + ": Argument #2 is expected to be a string");

  std::string schema = args[0].as_string();
  std::string name = args[1].as_string();

  shcore::Value ret_val;

  try {
    if (type == "View") {
      ret_val =
          _execute_sql(sqlstring("drop view !.!", 0) << schema << name + "",
                       shcore::Argument_list());
    } else {
      shcore::Argument_list command_args;
      command_args.push_back(Value(schema));
      command_args.push_back(Value(name));

      ret_val = executeAdminCommand("drop_collection", false, command_args);
    }

    if (_schemas->count(schema)) {
      std::shared_ptr<Schema> schema_obj =
          std::static_pointer_cast<Schema>((*_schemas)[schema].as_object());
      if (schema_obj)
        schema_obj->_remove_object(name, type);
    }
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name("drop" + type));

  return ret_val;
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
std::string NodeSession::db_object_exists(std::string &type,
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

extern "C" {
unsigned long mysql_get_client_version(void);
}

shcore::Value::Map_type_ref NodeSession::get_status() {
  shcore::Value::Map_type_ref status(new shcore::Value::Map_type);

  (*status)["SESSION_TYPE"] = shcore::Value("Node");

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
        "@@mysqlx_socket, @@mysqlx_port "
        "limit 1");
    row = result->fetch_one();
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
    if (transport_type == mysqlshdk::db::Transport_type::Tcp)
      (*status)["TCP_PORT"] = shcore::Value(
          row->is_null(6) ? "" : row->get_as_string(6));
    else if (transport_type == mysqlshdk::db::Transport_type::Socket)
      (*status)["UNIX_SOCKET"] =
          shcore::Value(row->is_null(5) ? "" : row->get_string(5));

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

void NodeSession::start_transaction() {
  if (_tx_deep == 0)
    execute_sql("start transaction", shcore::Argument_list());

  _tx_deep++;
}

void NodeSession::commit() {
  _tx_deep--;

  assert(_tx_deep >= 0);

  if (_tx_deep == 0)
    execute_sql("commit", shcore::Argument_list());
}

void NodeSession::rollback() {
  _tx_deep--;

  assert(_tx_deep >= 0);

  if (_tx_deep == 0)
    execute_sql("rollback", shcore::Argument_list());
}

std::string NodeSession::query_one_string(const std::string &query, int field) {
  std::shared_ptr<mysqlshdk::db::mysqlx::Result> result = execute_sql(query);
  if (auto row = result->fetch_one()) {
    if (!row->is_null(field)) {
      return row->get_string(field);
    }
  }
  return "";
}

void NodeSession::kill_query() {
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
shcore::Object_bridge_ref NodeSession::raw_execute_sql(
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


Value NodeSession::executeAdminCommand(const std::string &command,
                                       bool expect_data,
                                       const Argument_list &args) {
  std::string function = class_name() + '.' + "executeAdminCommand";
  args.ensure_at_least(1, function.c_str());

  return _execute_stmt("xplugin", command, convert_args(args), expect_data);
}

shcore::Value NodeSession::_execute_mysqlx_stmt(
    const std::string &command, const shcore::Dictionary_t &args) {
  return _execute_stmt("mysqlx", command, convert_args(args), true);
}

std::shared_ptr<mysqlshdk::db::mysqlx::Result> NodeSession::execute_mysqlx_stmt(
    const std::string &command, const shcore::Dictionary_t &args) {
  return execute_stmt("mysqlx", command, convert_args(args));
}

shcore::Value NodeSession::_execute_stmt(const std::string &ns,
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

std::shared_ptr<mysqlshdk::db::mysqlx::Result> NodeSession::execute_stmt(
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

shcore::Value NodeSession::_execute_sql(const std::string &statement,
                                        const shcore::Argument_list &args) {
  MySQL_timer timer;
  timer.start();
  SqlResult *result = new SqlResult(execute_sql(statement, args));
  timer.end();
  result->set_execution_time(timer.raw_duration());
  return shcore::Value::wrap(result);
}

std::shared_ptr<mysqlshdk::db::mysqlx::Result> NodeSession::execute_sql(
    const std::string &sql, const shcore::Argument_list &args) {
  return execute_stmt("sql", sql, convert_args(args));
}

std::shared_ptr<shcore::Object_bridge> NodeSession::create(
    const shcore::Argument_list &args) {
  std::shared_ptr<NodeSession> session(new NodeSession());

  session->connect(
      mysqlsh::get_connection_options(args, mysqlsh::PasswordFormat::STRING));

  shcore::ShellNotifications::get()->notify("SN_SESSION_CONNECTED", session);

  return std::dynamic_pointer_cast<shcore::Object_bridge>(session);
}

// Documentation of sql function
REGISTER_HELP(NODESESSION_SQL_BRIEF,
              "Creates a SqlExecute object to allow "
              "running the received SQL statement on the target MySQL Server.");
REGISTER_HELP(NODESESSION_SQL_PARAM,
              "@param sql A string containing the SQL "
              "statement to be executed.");
REGISTER_HELP(NODESESSION_SQL_RETURN, "@return A SqlExecute object.");
REGISTER_HELP(NODESESSION_SQL_DETAIL,
              "This method creates an SqlExecute "
              "object which is a SQL execution handler.");
REGISTER_HELP(NODESESSION_SQL_DETAIL1,
              "The SqlExecute class has functions "
              "that allow defining the way the statement will be executed "
              "and allows doing parameter binding.");
REGISTER_HELP(NODESESSION_SQL_DETAIL2,
              "The received SQL is set on the "
              "execution handler.");

/**
 * $(NODESESSION_SQL_BRIEF)
 *
 * $(NODESESSION_SQL_PARAM)
 *
 * $(NODESESSION_SQL_RETURN)
 *
 * $(NODESESSION_SQL_DETAIL)
 *
 * $(NODESESSION_SQL_DETAIL1)
 *
 * $(NODESESSION_SQL_DETAIL2)
 *
 * JavaScript Example
 * \code{.js}
 * var sql = session.sql("select * from mydb.students where  age > ?");
 * var result = sql.bind(18).execute();
 * \endcode
 * \sa SqlExecute
 */
#if DOXYGEN_JS
SqlExecute NodeSession::sql(String sql) {}
#elif DOXYGEN_PY
SqlExecute NodeSession::sql(str sql) {}
#endif
shcore::Value NodeSession::sql(const shcore::Argument_list &args) {
  std::shared_ptr<SqlExecute> sql_execute(new SqlExecute(
      std::static_pointer_cast<NodeSession>(shared_from_this())));

  return sql_execute->sql(args);
}

REGISTER_HELP(NODESESSION_URI_BRIEF,
              "Retrieves the URI for the current "
              "session.");
REGISTER_HELP(NODESESSION_GETURI_BRIEF,
              "Retrieves the URI for the current "
              "session.");
REGISTER_HELP(NODESESSION_GETURI_RETURNS,
              "@return A string representing the "
              "connection data.");
/**
 * $(NODESESSION_GETURI_BRIEF)
 *
 * $(NODESESSION_GETURI_RETURNS)
 */

// Documentation of getCurrentSchema function
REGISTER_HELP(NODESESSION_CURRENTSCHEMA_BRIEF,
              "Retrieves the active schema "
              "on the session.");
REGISTER_HELP(NODESESSION_GETCURRENTSCHEMA_BRIEF,
              "Retrieves the active "
              "schema on the session.");
REGISTER_HELP(NODESESSION_GETCURRENTSCHEMA_RETURNS,
              "@return A Schema object "
              "if a schema is active on the session.");

/**
 * $(NODESESSION_GETCURRENTSCHEMA_BRIEF)
 *
 * $(NODESESSION_GETCURRENTSCHEMA_RETURNS)
 */
#if DOXYGEN_JS
Schema NodeSession::getCurrentSchema() {}
#elif DOXYGEN_PY
Schema NodeSession::get_current_schema() {}
#endif

Value NodeSession::get_member(const std::string &prop) const {
  Value ret_val;
  NodeSession *session = const_cast<NodeSession *>(this);

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
    std::string name = session->_retrieve_current_schema();

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
REGISTER_HELP(NODESESSION_QUOTENAME_BRIEF, "Escapes the passed identifier.");
REGISTER_HELP(NODESESSION_QUOTENAME_RETURNS,
              "@return A String containing the escaped identifier.");

/**
 * $(NODESESSION_QUOTENAME_BRIEF)
 *
 * $(NODESESSION_QUOTENAME_RETURNS)
 */
#if DOXYGEN_JS
String NodeSession::quoteName(String id) {}
#elif DOXYGEN_PY
str NodeSession::quote_name(str id) {}
#endif
shcore::Value NodeSession::quote_name(const shcore::Argument_list &args) {
  args.ensure_count(1, get_function_name("quoteName").c_str());

  if (args[0].type != shcore::String)
    throw shcore::Exception::type_error(
        "Argument #1 is expected to be a string");

  std::string id = args[0].as_string();

  return shcore::Value(get_quoted_name(id));
}

// Documentation of setCurrentSchema function
REGISTER_HELP(NODESESSION_SETCURRENTSCHEMA_BRIEF,
              "Sets the current schema for this session, and returns the "
              "schema object for it.");
REGISTER_HELP(NODESESSION_SETCURRENTSCHEMA_PARAM,
              "@param name the name of the new schema to switch to.");
REGISTER_HELP(NODESESSION_SETCURRENTSCHEMA_RETURNS,
              "@return the Schema object for the new schema.");
REGISTER_HELP(NODESESSION_SETCURRENTSCHEMA_DETAIL,
              "At the database level, this is equivalent at issuing the "
              "following SQL query:");
REGISTER_HELP(NODESESSION_SETCURRENTSCHEMA_DETAIL1,
              "  use <new-default-schema>;");

/**
 * $(NODESESSION_SETCURRENTSCHEMA_BRIEF)
 *
 * $(NODESESSION_SETCURRENTSCHEMA_PARAM)
 * $(NODESESSION_SETCURRENTSCHEMA_RETURNS)
 *
 * $(NODESESSION_SETCURRENTSCHEMA_DETAIL)
 * $(NODESESSION_SETCURRENTSCHEMA_DETAIL1)
 */
#if DOXYGEN_JS
Schema NodeSession::setCurrentSchema(String name) {}
#elif DOXYGEN_PY
Schema NodeSession::set_current_schema(str name) {}
#endif
shcore::Value NodeSession::_set_current_schema(
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
