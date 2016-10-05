/*
 * Copyright (c) 2015, 2016, Oracle and/or its affiliates. All rights reserved.
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

// Interactive session access module
// Exposed as "session" in the shell

#ifndef _MOD_SESSION_H_
#define _MOD_SESSION_H_

#include "mod_common.h"
#include "shellcore/types.h"
#include "shellcore/types_cpp.h"
#include "shellcore/ishell_core.h"
#include "mysql_connection.h"
#include "base_session.h"

namespace shcore {
class Shell_core;
class Proxy_object;
};

namespace mysh {
class DatabaseObject;

namespace mysql {
class ClassicSchema;
/**
* $(CLASSICSESSION_BRIEF)
*
* $(CLASSICSESSION_DETAIL)
*
* \code{.js}
* // Establishes the connection.
* var mysql = require('mysql');
* var session = mysql.getClassicSession("myuser@localhost", pwd);
*
* // Getting a schema through the getSchema function
* var schema = session.getSchema("sakila");
*
* // Getting a schema through a session property
* var schema = session.sakila;
* \endcode
*
* \sa mysql.getClassicSession(String connectionData, String password)
* \sa mysql.getClassicSession(Map connectionData, String password)
*/
class SHCORE_PUBLIC ClassicSession : public ShellDevelopmentSession, public std::enable_shared_from_this<ClassicSession> {
public:
  ClassicSession();
  ClassicSession(const ClassicSession& session);
  virtual ~ClassicSession() { try { shcore::Argument_list a; close(a); } catch (...) {} };

  // Virtual methods from object bridge
  virtual std::string class_name() const { return "ClassicSession"; };

  virtual shcore::Value get_member(const std::string &prop) const;

  // Virtual methods from ISession
  virtual shcore::Value connect(const shcore::Argument_list &args);
  virtual shcore::Value close(const shcore::Argument_list &args);
  virtual shcore::Value run_sql(const shcore::Argument_list &args) const;
  virtual shcore::Value create_schema(const shcore::Argument_list &args);
  virtual shcore::Value startTransaction(const shcore::Argument_list &args);
  virtual shcore::Value commit(const shcore::Argument_list &args);
  virtual shcore::Value rollback(const shcore::Argument_list &args);
  virtual shcore::Value drop_schema(const shcore::Argument_list &args);
  virtual shcore::Value drop_schema_object(const shcore::Argument_list &args, const std::string& type);

  virtual shcore::Value get_status(const shcore::Argument_list &args);

  virtual shcore::Value get_schema(const shcore::Argument_list &args) const;

  virtual shcore::Value get_schemas(const shcore::Argument_list &args) const;

  shcore::Value set_current_schema(const shcore::Argument_list &args);

  virtual std::string db_object_exists(std::string &type, const std::string &name, const std::string& owner) const;

  static std::shared_ptr<shcore::Object_bridge> create(const shcore::Argument_list &args);

  Connection *connection();

  virtual uint64_t get_connection_id() const { return (uint64_t)_conn->get_thread_id(); }

  virtual shcore::Value execute_sql(const std::string& query, const shcore::Argument_list &args) const;

#if DOXYGEN_JS
  String uri; //!< Same as getUri()
  ClassicSchema defaultSchema; //!< Same as getDefaultSchema()
  ClassicSchema currentSchema; //!< Same as getCurrentSchema()

  ClassicSchema createSchema(String name);
  ClassicSchema getSchema(String name);
  ClassicSchema getDefaultSchema();
  ClassicSchema getCurrentSchema();
  ClassicSchema setCurrentSchema(String name);
  List getSchemas();
  String getUri();
  ClassicResult runSql(String query);
  Undefined close();
  ClassicResult startTransaction();
  ClassicResult commit();
  ClassicResult rollback();
  ClassicResult dropSchema(String name);
  ClassicResult dropTable(String schema, String name);
  ClassicResult dropView(String schema, String name);
#elif DOXYGEN_PY
  str uri; //!< Same as get_uri()
  ClassicSchema default_schema; //!< Same as get_default_schema()
  ClassicSchema current_schema; //!< Same as get_current_schema()

  ClassicSchema create_schema(str name);
  ClassicSchema get_schema(str name);
  ClassicSchema get_default_schema();
  ClassicSchema get_current_schema();
  ClassicSchema set_current_schema(str name);
  list get_schemas();
  str get_uri();
  ClassicResult run_sql(str query);
  None close();
  ClassicResult start_transaction();
  ClassicResult commit();
  ClassicResult rollback();
  ClassicResult drop_schema(str name);
  ClassicResult drop_table(str schema, str name);
  ClassicResult drop_view(str schema, str name);
#endif

  /**
  * \brief Verifies if the session is still open.
  */
#if DOXYGEN_JS
  Bool isOpen() {}
#elif DOXYGEN_PY
  bool is_open() {}
#endif
  virtual bool is_connected() const { return _conn ? true : false; }

  virtual int get_default_port() { return 3306; };

private:
  void init();
  std::string _retrieve_current_schema();
  void _remove_schema(const std::string& name);
  std::shared_ptr<Connection> _conn;
};
};
};

#endif
