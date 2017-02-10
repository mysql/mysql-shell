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

#ifndef _MOD_XSESSION_H_
#define _MOD_XSESSION_H_

#include "mod_common.h"
#include "shellcore/types.h"
#include "shellcore/types_cpp.h"
#include "shellcore/ishell_core.h"
#include "base_session.h"
#include "mod_mysqlx_session_handle.h"
#include "mysqlxtest/mysqlx.h"

namespace shcore {
class Proxy_object;
};

namespace mysqlsh {
class DatabaseObject;
namespace mysqlx {
class Schema;
/**
* \ingroup XDevAPI
* $(BASESESSION_BRIEF)
*
* $(BASESESSION_DETAIL)
*
* $(BASESESSION_DETAIL1)
* $(BASESESSION_DETAIL2)
* $(BASESESSION_DETAIL3)
* $(BASESESSION_DETAIL4)
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
class SHCORE_PUBLIC BaseSession : public ShellDevelopmentSession {
public:
#if DOXYGEN_JS
  String uri; //!< Same as getUri()
  Schema defaultSchema; //!< Same as getDefaultSchema()

  Schema createSchema(String name);
  Schema getSchema(String name);
  Schema getDefaultSchema();
  List getSchemas();
  String getUri();
  Undefined close();
  Undefined setFetchWarnings(Boolean enable);
  Result startTransaction();
  Result commit();
  Result rollback();
  Result dropSchema(String name);
  Result dropTable(String schema, String name);
  Result dropCollection(String schema, String name);
  Result dropView(String schema, String name);
  Bool isOpen();

private:
#elif DOXYGEN_PY
  str uri; //!< Same as get_uri()
  Schema default_schema; //!< Same as get_default_schema()

  Schema create_schema(str name);
  Schema get_schema(str name);
  Schema get_default_schema();
  list get_schemas();
  str get_uri();
  None close();
  None set_fetch_warnings(bool enable);
  Result start_transaction();
  Result commit();
  Result rollback();
  Result drop_schema(str name);
  Result drop_table(str schema, str name);
  Result drop_collection(str schema, str name);
  Result drop_view(str schema, str name);
  Bool is_open();
private:
#endif

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

  shcore::Value executeAdminCommand(const std::string& command, bool expect_data, const shcore::Argument_list &args) const;
  virtual shcore::Value execute_sql(const std::string& query, const shcore::Argument_list &args) const;
  std::shared_ptr< ::mysqlx::Result> execute_sql(const std::string &sql) const;
  virtual bool is_connected() const;
  virtual shcore::Value get_status(const shcore::Argument_list &args);
  virtual shcore::Value get_capability(const std::string& name);

  virtual shcore::Value get_schema(const shcore::Argument_list &args) const;

  virtual shcore::Value get_schemas(const shcore::Argument_list &args) const;

  virtual std::string db_object_exists(std::string &type, const std::string &name, const std::string& owner) const;

  shcore::Value set_fetch_warnings(const shcore::Argument_list &args);

  std::shared_ptr< ::mysqlx::Session> session_obj() const;

  static std::shared_ptr<shcore::Object_bridge> create(const shcore::Argument_list &args);

  bool table_name_compare(const std::string &n1, const std::string &n2);

  virtual void set_option(const char *option, int value);

  virtual uint64_t get_connection_id() const;

protected:
  shcore::Value executeStmt(const std::string &domain, const std::string& command, bool expect_data, const shcore::Argument_list &args) const;
  virtual std::shared_ptr<BaseSession> _get_shared_this() const = 0;
  std::string _retrieve_current_schema();

  virtual int get_default_port() { return 33060; };

  SessionHandle _session;

  bool _case_sensitive_table_names;
  void init();
  uint64_t _connection_id;
private:
  void reset_session();
};

/**
* $(XSESSION_BRIEF)
*
* $(XSESSION_DETAIL)
*
* $(XSESSION_DETAIL1)
*
* \sa BaseSession
*/
class SHCORE_PUBLIC XSession : public BaseSession, public std::enable_shared_from_this<XSession> {
public:
  XSession() {};
  XSession(const XSession& s) : BaseSession(s) {}
  virtual ~XSession() {};
  virtual std::string class_name() const { return "XSession"; };
  static std::shared_ptr<shcore::Object_bridge> create(const shcore::Argument_list &args);

  virtual std::shared_ptr<BaseSession> _get_shared_this() const;
};

/**
* \ingroup XDevAPI
* $(NODESESSION_BRIEF)
*
* $(NODESESSION_DETAIL)
*
* \sa BaseSession
*/
class SHCORE_PUBLIC NodeSession : public BaseSession, public std::enable_shared_from_this<NodeSession> {
public:
#if DOXYGEN_JS
  Schema currentSchema; //!< Same as getCurrentSchema()

  Schema getCurrentSchema();
  Schema setCurrentSchema(String name);
  SqlExecute sql(String sql);
  String quoteName(String id);
  Result setFetchWarnings(Boolean enable);
#elif DOXYGEN_PY
  Schema current_schema; //!< Same as get_current_schema()

  Schema get_current_schema();
  Schema set_current_schema(str name);
  SqlExecute sql(str sql);
  str quote_name(str id);
  Result set_fetch_warnings(bool enable);
#endif
  NodeSession();
  NodeSession(const NodeSession& s);
  virtual ~NodeSession() {};
  virtual std::string class_name() const { return "NodeSession"; };

  virtual shcore::Value get_member(const std::string &prop) const;

  static std::shared_ptr<shcore::Object_bridge> create(const shcore::Argument_list &args);
  virtual std::shared_ptr<BaseSession> _get_shared_this() const;
  shcore::Value sql(const shcore::Argument_list &args);
  shcore::Value quote_name(const shcore::Argument_list &args);

  shcore::Value set_current_schema(const shcore::Argument_list &args);
protected:
  void init();
};
}
}

#endif
