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

// Interactive session access module for MySQL X sessions
// Exposed as "session" in the shell

#ifndef _MOD_XSESSION_H_
#define _MOD_XSESSION_H_

#include "mod_common.h"
#include "shellcore/types.h"
#include "shellcore/types_cpp.h"
#include "shellcore/ishell_core.h"
#include "base_session.h"
#include "mod_mysqlx_session_handle.h"
#include "mysqlxtest/mysqlx.h"

#include <boost/enable_shared_from_this.hpp>

namespace shcore
{
  class Proxy_object;
};

namespace mysh
{
  class DatabaseObject;
  namespace mysqlx
  {
    class Schema;
    /**
    * Base functionality for Session classes through the X Protocol.
    *
    * This class encloses the core functionality to be made available on both the XSession and NodeSession classes, such functionality includes
    *
    * - Accessing available schemas.
    * - Schema management operations.
    * - Enabling/disabling warning generation.
    * - Retrieval of connection information.
    *
    * #### JavaScript Examples
    *
    * \include "js_dev_api_examples/concepts/Working_with_a_Session_Object.js"
    *
    * #### Python Examples
    *
    * \include "py_dev_api_examples/concepts/Working_with_a_Session_Object.py"
    *
    * \sa mysqlx.getSession(String connectionData, String password)
    * \sa mysqlx.getSession(Map connectionData, String password)
    * \sa mysqlx.getNodeSession(String connectionData, String password)
    * \sa mysqlx.getNodeSession(Map connectionData, String password)
    */
    class SHCORE_PUBLIC BaseSession : public ShellDevelopmentSession
    {
    public:
      BaseSession();
      BaseSession(const BaseSession& s);
      virtual ~BaseSession() { reset_session(); }

      virtual shcore::Value connect(const shcore::Argument_list &args);
      virtual shcore::Value close(const shcore::Argument_list &args);
      virtual shcore::Value sql(const shcore::Argument_list &args);
      virtual shcore::Value create_schema(const shcore::Argument_list &args);
      virtual shcore::Value startTransaction(const shcore::Argument_list &args);
      virtual shcore::Value commit(const shcore::Argument_list &args);
      virtual shcore::Value rollback(const shcore::Argument_list &args);
      virtual shcore::Value drop_schema(const shcore::Argument_list &args);
      virtual shcore::Value drop_schema_object(const shcore::Argument_list &args, const std::string& type);
      virtual shcore::Value set_current_schema(const shcore::Argument_list &args);

      shcore::Value executeAdminCommand(const std::string& command, bool expect_data, const shcore::Argument_list &args) const;
      shcore::Value execute_sql(const std::string& query, const shcore::Argument_list &args);
      virtual bool is_connected() const;
      virtual shcore::Value get_status(const shcore::Argument_list &args);
      virtual shcore::Value get_capability(const std::string& name);

      virtual shcore::Value get_schema(const shcore::Argument_list &args) const;

      virtual shcore::Value get_schemas(const shcore::Argument_list &args) const;

      virtual std::string db_object_exists(std::string &type, const std::string &name, const std::string& owner) const;

      shcore::Value set_fetch_warnings(const shcore::Argument_list &args);

      boost::shared_ptr< ::mysqlx::Session> session_obj() const;

      static boost::shared_ptr<shcore::Object_bridge> create(const shcore::Argument_list &args);

      bool table_name_compare(const std::string &n1, const std::string &n2);

      virtual void set_option(const char *option, int value);

      virtual uint64_t get_connection_id() const;

#ifdef DOXYGEN
      String uri; //!< Same as getUri()
      Schema defaultSchema; //!< Same as getDefaultSchema()

      Schema createSchema(String name);
      Schema getSchema(String name);
      Schema getDefaultSchema();
      List getSchemas();
      String getUri();
      Undefined close();
      Undefined setFetchWarnings(Bool value);
      Result startTransaction();
      Result commit();
      Result rollback();
      Result dropSchema(String name);
      Result dropTable(String schema, String name);
      Result dropCollection(String schema, String name);
      Result dropView(String schema, String name);

#endif
    protected:
      shcore::Value executeStmt(const std::string &domain, const std::string& command, bool expect_data, const shcore::Argument_list &args) const;
      virtual boost::shared_ptr<BaseSession> _get_shared_this() const = 0;
      std::string _retrieve_current_schema();
      void _retrieve_session_info(std::string &current_schema, int &case_sensitive_table_names);

      virtual int get_default_port() { return 33060; };

      SessionHandle _session;

      bool _case_sensitive_table_names;
      void init();
      uint64_t _connection_id;
      void set_connection_id();
    private:
      void reset_session();
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
      XSession(const XSession& s) : BaseSession(s) {}
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
      NodeSession(const NodeSession& s);
      virtual ~NodeSession(){};
      virtual std::string class_name() const { return "NodeSession"; };

      virtual std::vector<std::string> get_members() const;
      virtual shcore::Value get_member(const std::string &prop) const;
      virtual bool has_member(const std::string &prop) const;

      static boost::shared_ptr<shcore::Object_bridge> create(const shcore::Argument_list &args);
      virtual boost::shared_ptr<BaseSession> _get_shared_this() const;
      shcore::Value sql(const shcore::Argument_list &args);
      shcore::Value quote_name(const shcore::Argument_list &args);
#ifdef DOXYGEN
      Schema currentSchema; //!< Same as getCurrentSchema()

      Schema getCurrentSchema();
      Schema setCurrentSchema(String name);
      SqlExecute sql(String sql);
      String quoteName(String id);
#endif
    protected:
      void init();
    };
    }
  }

#endif
