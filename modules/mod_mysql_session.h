/*
 * Copyright (c) 2015, 2017, Oracle and/or its affiliates. All rights reserved.
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

// Interactive session access module
// Exposed as "session" in the shell

#ifndef _MOD_SESSION_H_
#define _MOD_SESSION_H_

#include "modules/mod_common.h"
#include "scripting/types.h"
#include "scripting/types_cpp.h"
#include "shellcore/ishell_core.h"
#include "shellcore/base_session.h"
#include "mysqlshdk/libs/db/connection_options.h"
#include "mysqlshdk/libs/db/mysql/session.h"

namespace shcore {
class Shell_core;
class Proxy_object;
};

namespace mysqlsh {
class DatabaseObject;

namespace mysql {
class ClassicSchema;
class ClassicResult;
class Connection;
/**
* \ingroup ShellAPI
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
class SHCORE_PUBLIC ClassicSession : public ShellBaseSession, public std::enable_shared_from_this<ClassicSession> {
public:
  ClassicSession();
  ClassicSession(const ClassicSession& session);
  virtual ~ClassicSession();

// We need to hide this from doxygen to avoid warnings
#if !defined DOXYGEN_JS && !defined DOXYGEN_PY
  std::shared_ptr<ClassicResult> execute_sql(const std::string& query);
#endif

  // Virtual methods from object bridge
  virtual std::string class_name() const { return "ClassicSession"; };

  virtual shcore::Value get_member(const std::string &prop) const;

  // Virtual methods from ISession
  virtual void connect(const mysqlshdk::db::Connection_options& data);
  virtual void close();
  virtual void create_schema(const std::string& name);
  virtual void drop_schema(const std::string &name);
  virtual void set_current_schema(const std::string &name);
  virtual void start_transaction();
  virtual void commit();
  virtual void rollback();

  virtual std::string get_current_schema();

  virtual shcore::Value query(const shcore::Argument_list &args);

  shcore::Value _close(const shcore::Argument_list &args);
  virtual shcore::Value run_sql(const shcore::Argument_list &args);
  virtual shcore::Value _start_transaction(const shcore::Argument_list &args);
  virtual shcore::Value _commit(const shcore::Argument_list &args);
  virtual shcore::Value _rollback(const shcore::Argument_list &args);
  shcore::Value _is_open(const shcore::Argument_list &args);

  virtual shcore::Value::Map_type_ref get_status();

  virtual std::string db_object_exists(std::string &type,
                                       const std::string &name,
                                       const std::string &owner);

  static std::shared_ptr<shcore::Object_bridge> create(
      const shcore::Argument_list &args);

  virtual std::shared_ptr<mysqlshdk::db::ISession> get_core_session() {
    return _session;
  }

  virtual uint64_t get_connection_id() const;
  virtual std::string query_one_string(const std::string &query, int field = 0);
  virtual std::string get_ssl_cipher() const;
private:
  shcore::Value execute_sql(const std::string &query,
                            const shcore::Array_t &args);
  virtual shcore::Object_bridge_ref raw_execute_sql(const std::string& query);
public:
  virtual SessionType session_type() const {
    return SessionType::Classic;
  }

  virtual void kill_query();

#if DOXYGEN_JS
  String uri; //!< Same as getUri()
  String getUri();
  ClassicResult runSql(String query, Array args = []);
  ClassicResult query(String query, Array args = []);
  Undefined close();
  ClassicResult startTransaction();
  ClassicResult commit();
  ClassicResult rollback();
#elif DOXYGEN_PY
  str uri; //!< Same as get_uri()
  str get_uri();
  ClassicResult run_sql(str query, list args = []);
  ClassicResult query(str query, list args = []);
  None close();
  ClassicResult start_transaction();
  ClassicResult commit();
  ClassicResult rollback();
#endif

  /**
  * $(SHELLBASESESSION_ISOPEN_BRIEF)
  *
  * $(SHELLBASESESSION_ISOPEN_RETURNS)
  *
  * $(SHELLBASESESSION_ISOPEN_DETAIL)
  */
#if DOXYGEN_JS
  Bool isOpen() {}
#elif DOXYGEN_PY
  bool is_open() {}
#endif
  virtual bool is_open() const;

private:
  void init();
  std::shared_ptr<mysqlshdk::db::mysql::Session> _session;
  shcore::Value _run_sql(const std::string& function,
                         const shcore::Argument_list &args);
};
};
};

#endif
