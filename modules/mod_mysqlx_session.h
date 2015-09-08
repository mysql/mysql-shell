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

// Interactive session access module for MySQLx sessions
// Exposed as "session" in the shell

#ifndef _MOD_XSESSION_H_
#define _MOD_XSESSION_H_

#include "mod_common.h"
#include "shellcore/types.h"
#include "shellcore/types_cpp.h"
#include "shellcore/ishell_core.h"
#include "base_session.h"
#include "mysqlx.h"

#include <boost/enable_shared_from_this.hpp>

#ifdef __GNUC__
#define ATTR_UNUSED __attribute__((unused))
#else
#define ATTR_UNUSED
#endif

namespace shcore
{
  class Proxy_object;
};

/*
* Helper function to ensure the exceptions generated on the mysqlx_connector
* are properly translated to the corresponding shcore::Exception type
*/
static void ATTR_UNUSED translate_exception()
{
  try
  {
    throw;
  }
  catch (::mysqlx::Error &e)
  {
    throw shcore::Exception::error_with_code("Server", e.what(), e.error());
  }
  catch (std::runtime_error &e)
  {
    throw shcore::Exception::runtime_error(e.what());
  }
  catch (std::logic_error &e)
  {
    throw shcore::Exception::logic_error(e.what());
  }
  catch (...)
  {
    throw;
  }
}

#define CATCH_AND_TRANSLATE()   \
  catch (...)                   \
{ translate_exception(); }

namespace mysh
{
  class DatabaseObject;
  /**
  * Encloses the functions and classes available to interact with an X Protocol enabled MySQL Product.
  *
  * The objects contained on this module provide a full API to interact with the different MySQL Products implementing the
  * X Protocol.
  *
  * In the case of a MySQL Server the API will enable doing operations on the different database objects such as schema management operations and both table and
  * collection management and CRUD operations. (CRUD: Create, Read, Update, Delete).
  *
  * Intention of the module is to provide a full API for development through scripting languages such as JavaScript and Python, this would be normally achieved through a normal session.
  *
  * If specialized SQL work is required, SQL execution is also available through a NodeSession, which is intended to work specifically with MySQL Servers.
  */
  namespace mysqlx
  {
#ifdef DOXYGEN
    XSession getSession(String connectionData, String password);
    XSession getSession(Map connectionData, String password);
    NodeSession getNodeSession(String connectionData, String password);
    NodeSession getNodeSession(Map connectionData, String password);
#endif

    class Schema;
    /**
    * Base functionality for Session classes through the X Protocol.
    *
    * This class encloses the core functionaliti to be made available on both the XSession and NodeSession classes, such functionality includes
    *
    * - Accessing available schemas.
    * - Schema management operations.
    * - Enabling/disabling warning generation.
    * - Retrieval of connection information.
    *
    * \b Dynamic \b Properties
    *
    * In addition to the properties documented above, when a session object is created the schemas available on the target
    * MySQL Server are cached.
    *
    * A dynamic property is added to the session object in order to access each available Schema as a session member.
    *
    * These dynamic properties are named as the Schema's name, so the schemas are accessible as follows:
    *
    * \code{.js}
    * // Establishes the connection.
    * var mysqlx = require('mysqlx').mysqlx;
    * var session = mysqlx.getSession("myuser@localhost", pwd);
    *
    * // Getting a schema through the getSchema function
    * var schema = session.getSchema("sakila");
    *
    * // Getting a schema through a session property
    * var schema = session.sakila;
    * \endcode
    *
    * \sa mysqlx.getSession(String connectionData, String password)
    * \sa mysqlx.getSession(Map connectionData, String password)
    * \sa mysqlx.getNodeSession(String connectionData, String password)
    * \sa mysqlx.getNodeSession(Map connectionData, String password)
    */
    class SHCORE_PUBLIC BaseSession : public ShellBaseSession
    {
    public:
      BaseSession();
      virtual ~BaseSession() { flush_last_result(); }

      virtual std::vector<std::string> get_members() const;
      virtual shcore::Value get_member(const std::string &prop) const;
      virtual bool has_member(const std::string &prop) const;

      virtual shcore::Value connect(const shcore::Argument_list &args);
      virtual shcore::Value close(const shcore::Argument_list &args);
      virtual shcore::Value sql(const shcore::Argument_list &args);
      virtual shcore::Value createSchema(const shcore::Argument_list &args);
      shcore::Value executeAdminCommand(const std::string& command, const shcore::Argument_list &args);
      shcore::Value executeSql(const std::string& query, const shcore::Argument_list &args);
      virtual bool is_connected() const { return _session ? true : false; }
      virtual std::string uri() const { return _uri; };

      virtual shcore::Value get_schema(const shcore::Argument_list &args) const;
      virtual shcore::Value set_default_schema(const shcore::Argument_list &args);

      virtual void drop_db_object(const std::string &type, const std::string &name, const std::string& owner);
      virtual bool db_object_exists(std::string &type, const std::string &name, const std::string& owner);

      shcore::Value set_fetch_warnings(const shcore::Argument_list &args);

      boost::shared_ptr< ::mysqlx::Session> session_obj() const { return _session; }

      static boost::shared_ptr<shcore::Object_bridge> create(const shcore::Argument_list &args);

      void flush_last_result();

#ifdef DOXYGEN
      String uri; //!< Same as getUri()
      Map schemas; //!< Same as getSchemas()
      Schema defaultSchema; //!< Same as getDefaultSchema()

      Schema createSchema(String name);
      Schema getSchema(String name);
      Schema getDefaultSchema();
      Schema setDefaultSchema(String name);
      Map getSchemas();
      String getUri();
      Undefined close();
      Undefined setFetchWarnings(Bool value);
#endif
    protected:
      ::mysqlx::ArgumentValue get_argument_value(shcore::Value source);
      shcore::Value executeStmt(const std::string &domain, const std::string& command, const shcore::Argument_list &args);
      virtual boost::shared_ptr<BaseSession> _get_shared_this() const = 0;
      boost::shared_ptr< ::mysqlx::Result> _last_result;
      void _update_default_schema(const std::string& name);
      virtual void _load_default_schema();
      virtual void _load_schemas();

      boost::shared_ptr< ::mysqlx::Session> _session;

      boost::shared_ptr<Schema> _default_schema;
      boost::shared_ptr<shcore::Value::Map_type> _schemas;

      std::string _uri;
    };

    /**
    * Enables interaction with an X Protocol enabled MySQL Product.
    *
    * Note that this class inherits the behavior described on the BaseSession class.
    *
    * In the future this class will be improved to support interacting not only with MySQL Server but with other products.
    *
    * \sa BaseSession
    */
    class SHCORE_PUBLIC XSession : public BaseSession, public boost::enable_shared_from_this<XSession>
    {
    public:
      XSession(){};
      virtual ~XSession(){};
      virtual std::string class_name() const { return "XSession"; };
      static boost::shared_ptr<shcore::Object_bridge> create(const shcore::Argument_list &args);

      virtual boost::shared_ptr<BaseSession> _get_shared_this() const;
    };

    /**
    * Enables interaction with an X Protocol enabled MySQL Server, this includes SQL Execution.
    *
    * Note that this class inherits the behavior described on the BaseSession class.
    *
    * \sa BaseSession
    */
    class SHCORE_PUBLIC NodeSession : public BaseSession, public boost::enable_shared_from_this<NodeSession>
    {
    public:
      NodeSession();
      virtual ~NodeSession(){};
      virtual std::string class_name() const { return "NodeSession"; };
      static boost::shared_ptr<shcore::Object_bridge> create(const shcore::Argument_list &args);
      virtual boost::shared_ptr<BaseSession> _get_shared_this() const;
      shcore::Value sql(const shcore::Argument_list &args);
#ifdef DOXYGEN
      Resultset sql(String sql);
#endif
    };
  }
}

#endif
