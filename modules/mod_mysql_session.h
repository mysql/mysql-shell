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
#include "mysql.h"
#include "base_session.h"

#include <boost/enable_shared_from_this.hpp>

namespace shcore
{
  class Shell_core;
  class Proxy_object;
};

namespace mysh
{
  class DatabaseObject;

  namespace mysql
  {
    class ClassicSchema;
    /**
    * Enables interaction with a MySQL Server using the MySQL Protocol.
    *
    * Provides facilities to execute queries and retrieve database objects.
    *
    * \b Dynamic \b Properties
    *
    * In addition to the properties documented above, when a session object is created the schemas available on the target
    * MySQL Server are cached.
    *
    * A dynamic property is added to the session object in order to access each available ClassicSchema as a session member.
    *
    * These dynamic properties are named as the ClassicSchema's name, so the schemas are accessible as follows:
    *
    * \code{.js}
    * // Establishes the connection.
    * var mysql = require('mysql').mysql;
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
    class SHCORE_PUBLIC ClassicSession : public ShellBaseSession, public boost::enable_shared_from_this<ClassicSession>
    {
    public:
      ClassicSession();
      ClassicSession(const ClassicSession& session);
      virtual ~ClassicSession() { try { shcore::Argument_list a; close(a); } catch (...) {} };

      // Virtual methods from object bridge
      virtual std::string class_name() const { return "ClassicSession"; };
      virtual shcore::Value get_member(const std::string &prop) const;
      std::vector<std::string> get_members() const;

      // Virtual methods from ISession
      virtual shcore::Value connect(const shcore::Argument_list &args);
      virtual shcore::Value close(const shcore::Argument_list &args);
      virtual shcore::Value run_sql(const shcore::Argument_list &args);
      virtual shcore::Value create_schema(const shcore::Argument_list &args);
      virtual shcore::Value startTransaction(const shcore::Argument_list &args);
      virtual shcore::Value commit(const shcore::Argument_list &args);
      virtual shcore::Value rollback(const shcore::Argument_list &args);
      virtual shcore::Value drop_schema(const shcore::Argument_list &args);
      virtual shcore::Value drop_schema_object(const shcore::Argument_list &args, const std::string& type);

      virtual bool is_connected() const { return _conn ? true : false; }
      virtual shcore::Value get_status(const shcore::Argument_list &args);

      virtual shcore::Value get_schema(const shcore::Argument_list &args) const;
      virtual shcore::Value set_current_schema(const shcore::Argument_list &args);

      virtual std::string db_object_exists(std::string &type, const std::string &name, const std::string& owner);

      static boost::shared_ptr<shcore::Object_bridge> create(const shcore::Argument_list &args);

      Connection *connection();

      virtual uint64_t get_connection_id() const { return (uint64_t)_conn->get_thread_id(); }

#ifdef DOXYGEN
      String uri; //!< Same as getUri()
      Map schemas; //!< Same as getSchemas()
      ClassicSchema defaultSchema; //!< Same as getDefaultSchema()
      ClassicSchema currentSchema; //!< Same as getCurrentSchema()

      ClassicSchema createSchema(String name);
      ClassicSchema getSchema(String name);
      ClassicSchema getDefaultSchema();
      ClassicSchema getCurrentSchema();
      ClassicSchema setCurrentSchema(String name);
      Map getSchemas();
      String getUri();
      ClassicResult runSql(String query);
      Undefined close();
      ClassicResult startTransaction();
      ClassicResult commit();
      ClassicResult rollback();
      ClassicResult dropSchema(String name);
      ClassicResult dropTable(String schema, String name);
      ClassicResult dropView(String schema, String name);

#endif
    protected:
      virtual int get_default_port() { return 3306; };

    private:
      void init();
      std::string _retrieve_current_schema();
      void _load_schemas();
      void _remove_schema(const std::string& name);
      boost::shared_ptr<Connection> _conn;
      std::string _default_schema;
      boost::shared_ptr<shcore::Value::Map_type> _schemas;
    };
  };
};

#endif
