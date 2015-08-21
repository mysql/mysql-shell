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
  namespace mysqlx
  {
    class Schema;
    /**
    * Base functionality for Session classes through the X Protocol.
    * \todo Implement and document createSchema()
    * \todo Implement and document dropSchema()
    */
    class MOD_PUBLIC BaseSession : public ShellBaseSession
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
      shcore::Value executeAdminCommand(const std::string& command, const shcore::Argument_list &args);
      shcore::Value executeSql(const std::string& query, const shcore::Argument_list &args);
      virtual bool is_connected() const { return _session ? true : false; }
      virtual std::string uri() const { return _uri; };

      virtual shcore::Value get_schema(const shcore::Argument_list &args) const;
      virtual shcore::Value set_default_schema(const shcore::Argument_list &args);
      shcore::Value set_fetch_warnings(const shcore::Argument_list &args);

      boost::shared_ptr< ::mysqlx::Session> session_obj() const { return _session; }

      static boost::shared_ptr<shcore::Object_bridge> create(const shcore::Argument_list &args);

      void flush_last_result();

#ifdef DOXYGEN
      String uri; //!< Same as getUri()
      List schemas; //!< Same as getSchemas()
      Schema defaultSchema; //!< Same as getDefaultSchema()

      Undefined connect(String connectionData, String password);
      Undefined connect(Map connectionData, String password);
      Schema getDefaultSchema();
      Schema getSchema(String name);
      List getSchemas();
      String getUri();
      Undefined close();
      Undefined setFetchWarnings(Bool value);
      Schema setDefaultSchema(String name);
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
      bool _show_warnings;
    };

    /**
    * Enables interaction with an X Protocol enabled MySQL Server.
    */
    class MOD_PUBLIC Session : public BaseSession, public boost::enable_shared_from_this<Session>
    {
    public:
      Session(){};
      virtual ~Session(){};
      virtual std::string class_name() const { return "Session"; };
      static boost::shared_ptr<shcore::Object_bridge> create(const shcore::Argument_list &args);

      virtual boost::shared_ptr<BaseSession> _get_shared_this() const;
    };

    /**
    * Enables interaction with an X Protocol enabled MySQL Server, inclusing SQL Execution.
    */
    class MOD_PUBLIC NodeSession : public BaseSession, public boost::enable_shared_from_this<NodeSession>
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
