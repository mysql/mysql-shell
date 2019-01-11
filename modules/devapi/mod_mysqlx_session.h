/*
 * Copyright (c) 2015, 2019, Oracle and/or its affiliates. All rights reserved.
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

#ifndef MODULES_DEVAPI_MOD_MYSQLX_SESSION_H_
#define MODULES_DEVAPI_MOD_MYSQLX_SESSION_H_

#include <memory>
#include <string>
#include "db/mysqlx/mysqlxclient_clean.h"
#include "db/mysqlx/session.h"
#include "modules/mod_common.h"
#include "scripting/types.h"
#include "scripting/types_cpp.h"
#include "shellcore/base_session.h"
#include "utils/nullable.h"

namespace shcore {
class Proxy_object;
};

namespace mysqlsh {
class DatabaseObject;
namespace mysqlx {

class Schema;
/**
 * \ingroup XDevAPI
 * @brief $(SESSION_BRIEF)
 *
 * $(SESSION_DETAIL)
 *
 * $(SESSION_DETAIL1)
 * $(SESSION_DETAIL2)
 * $(SESSION_DETAIL3)
 * $(SESSION_DETAIL4)
 * $(SESSION_DETAIL5)
 *
 * #### JavaScript Examples
 *
 * \include "concepts/Working_with_a_Session_Object.js"
 *
 * #### Python Examples
 *
 * \include "concepts/Working_with_a_Session_Object.py"
 *
 * \sa mysqlx.getSession(String connectionData, String password)
 * \sa mysqlx.getSession(Map connectionData, String password)
 */
class SHCORE_PUBLIC Session : public ShellBaseSession,
                              public std::enable_shared_from_this<Session> {
 public:
#if DOXYGEN_JS
  String uri;            //!< Same as getUri()
  Schema defaultSchema;  //!< Same as getDefaultSchema()
  Schema currentSchema;  //!< Same as getCurrentSchema()

  Schema createSchema(String name);
  Schema getSchema(String name);
  Schema getDefaultSchema();
  Schema getCurrentSchema();
  Schema setCurrentSchema(String name);
  List getSchemas();
  String getUri();
  Undefined close();
  Undefined setFetchWarnings(Boolean enable);
  Result startTransaction();
  Result commit();
  Result rollback();
  Undefined dropSchema(String name);
  Bool isOpen();

  SqlExecute sql(String sql);
  String quoteName(String id);
  String setSavepoint(String name = "");
  Undefined releaseSavepoint(String name);
  Undefined rollbackTo(String name);

 private:
#elif DOXYGEN_PY
  str uri;                //!< Same as get_uri()
  Schema default_schema;  //!< Same as get_default_schema()
  Schema current_schema;  //!< Same as get_current_schema()

  Schema create_schema(str name);
  Schema get_schema(str name);
  Schema get_default_schema();
  Schema get_current_schema();
  Schema set_current_schema(str name);
  list get_schemas();
  str get_uri();
  None close();
  None set_fetch_warnings(bool enable);
  Result start_transaction();
  Result commit();
  Result rollback();
  None drop_schema(str name);
  Bool is_open();

  SqlExecute sql(str sql);
  str quote_name(str id);
  str set_savepoint(str name = "");
  None release_savepoint(str name);
  None rollback_to(str name);

 private:
#endif

  Session();
  explicit Session(std::shared_ptr<mysqlshdk::db::mysqlx::Session> session);
  Session(const Session &s);
  virtual ~Session();

  virtual std::string class_name() const { return "Session"; }

  virtual shcore::Value get_member(const std::string &prop) const;

  virtual std::string get_node_type() { return "X"; }

  virtual SessionType session_type() const { return SessionType::X; }

  static std::shared_ptr<shcore::Object_bridge> create(
      const shcore::Argument_list &args);

  virtual void connect(const mysqlshdk::db::Connection_options &data);
  virtual void close();
  virtual void create_schema(const std::string &name);
  virtual void drop_schema(const std::string &name);
  virtual void set_current_schema(const std::string &name);
  shcore::Object_bridge_ref get_schema(const std::string &name);
  virtual void start_transaction();
  virtual void commit();
  virtual void rollback();

  virtual std::string get_current_schema();

  shcore::Value _close(const shcore::Argument_list &args);
  virtual shcore::Value _create_schema(const shcore::Argument_list &args);
  virtual shcore::Value _start_transaction(const shcore::Argument_list &args);
  virtual shcore::Value _commit(const shcore::Argument_list &args);
  virtual shcore::Value _rollback(const shcore::Argument_list &args);
  shcore::Value _drop_schema(const shcore::Argument_list &args);
  shcore::Value _is_open(const shcore::Argument_list &args);
  shcore::Value _set_current_schema(const shcore::Argument_list &args);
  shcore::Value sql(const shcore::Argument_list &args);
  shcore::Value quote_name(const shcore::Argument_list &args);

  shcore::Value _set_savepoint(const shcore::Argument_list &args);
  shcore::Value _release_savepoint(const shcore::Argument_list &args);
  shcore::Value _rollback_to(const shcore::Argument_list &args);

  virtual bool is_open() const;
  virtual shcore::Value::Map_type_ref get_status();

  virtual std::string get_ssl_cipher() const {
    return _session->get_ssl_cipher() ? _session->get_ssl_cipher() : "";
  }

  shcore::Value _get_schema(const shcore::Argument_list &args);

  shcore::Value get_schemas(const shcore::Argument_list &args);

  virtual std::string db_object_exists(std::string &type,
                                       const std::string &name,
                                       const std::string &owner);

  shcore::Value set_fetch_warnings(const shcore::Argument_list &args);

  bool table_name_compare(const std::string &n1, const std::string &n2);

  virtual void set_option(const char *option, int value);

  virtual uint64_t get_connection_id() const;
  virtual std::string query_one_string(const std::string &query, int field = 0);

  virtual void kill_query();

  mysqlshdk::db::mysqlx::Session *session() { return _session.get(); }

  virtual std::shared_ptr<mysqlshdk::db::ISession> get_core_session() {
    return _session;
  }

 public:
  // TODO(alfredo) legacy - replace with mysqlx calls
  shcore::Value executeAdminCommand(const std::string &command, bool,
                                    const shcore::Argument_list &args);

  // TODO(alfredo) delete this eventually
  virtual shcore::Object_bridge_ref raw_execute_sql(const std::string &query);

 public:
  std::shared_ptr<mysqlshdk::db::mysqlx::Result> execute_sql(
      const std::string &command,
      const shcore::Argument_list &args = shcore::Argument_list());

  shcore::Value _execute_sql(
      const std::string &command,
      const shcore::Argument_list &args = shcore::Argument_list());

  shcore::Value _execute_mysqlx_stmt(const std::string &command,
                                     const shcore::Dictionary_t &args);

  std::shared_ptr<mysqlshdk::db::mysqlx::Result> execute_mysqlx_stmt(
      const std::string &command, const shcore::Dictionary_t &args);

  std::string get_uuid();

  void disable_prepared_statements() { m_allow_prepared_statements = false; }
  bool allow_prepared_statements() { return m_allow_prepared_statements; }

 protected:
  friend class SqlExecute;

  mutable std::shared_ptr<shcore::Value::Map_type> _schemas;
  std::function<void(const std::string &, bool exists)> update_schema_cache;

  std::shared_ptr<mysqlshdk::db::mysqlx::Result> execute_stmt(
      const std::string &ns, const std::string &command,
      const ::xcl::Arguments &args);

  shcore::Value _execute_stmt(const std::string &ns, const std::string &command,
                              const ::xcl::Arguments &args, bool expect_data);

  std::string _retrieve_current_schema();

  std::shared_ptr<mysqlshdk::db::mysqlx::Session> _session;

  // The UUIDs generated by this Session instance MUST guarantee the same
  // host identifier no matter if it is the mac address of a randomly generated
  // number, this UUID is created to store the hist identifier
  std::string _session_uuid;

  bool _case_sensitive_table_names;
  void init();
  uint64_t _connection_id;
  uint64_t _savepoint_counter;

 private:
  bool m_allow_prepared_statements = true;
  void reset_session();
};

}  // namespace mysqlx
}  // namespace mysqlsh

#endif  // MODULES_DEVAPI_MOD_MYSQLX_SESSION_H_
