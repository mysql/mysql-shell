/*
 * Copyright (c) 2015, 2021, Oracle and/or its affiliates.
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

#include <list>
#include <memory>
#include <string>
#include <vector>
#include "db/mysqlx/mysqlxclient_clean.h"
#include "db/mysqlx/session.h"
#include "modules/devapi/mod_mysqlx_resultset.h"
#include "modules/mod_common.h"
#include "scripting/types.h"
#include "scripting/types_cpp.h"
#include "shellcore/base_session.h"
#include "utils/nullable.h"

namespace shcore {
class Proxy_object;
}  // namespace shcore

namespace mysqlsh {
class DatabaseObject;
namespace mysqlx {

class Schema;
class SqlExecute;
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
  String uri;            //!< $(SESSION_GETURI_BRIEF)
  Schema defaultSchema;  //!< $(SESSION_GETDEFAULTSCHEMA_BRIEF)
  Schema currentSchema;  //!< $(SESSION_GETCURRENTSCHEMA_BRIEF)

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
  SqlResult runSql(String query, Array args);

 private:
#elif DOXYGEN_PY
  str uri;                //!< $(SESSION_GETURI_BRIEF)
  Schema default_schema;  //!< $(SESSION_GETDEFAULTSCHEMA_BRIEF)
  Schema current_schema;  //!< $(SESSION_GETCURRENTSCHEMA_BRIEF)

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
  SqlResult run_sql(str query, list args);

 private:
#endif

  Session();
  explicit Session(std::shared_ptr<mysqlshdk::db::mysqlx::Session> session);
  Session(const Session &s);
  virtual ~Session();

  std::string class_name() const override { return "Session"; }

  shcore::Value get_member(const std::string &prop) const override;

  std::string get_node_type() override { return "X"; }

  SessionType session_type() const override { return SessionType::X; }

  static std::shared_ptr<shcore::Object_bridge> create(
      const mysqlshdk::db::Connection_options &co);

  void connect(const mysqlshdk::db::Connection_options &data) override;
  void close() override;
  void create_schema(const std::string &name) override;
  void drop_schema(const std::string &name) override;
  void set_current_schema(const std::string &name) override;
  std::shared_ptr<Schema> get_schema(const std::string &name);
  void start_transaction() override;
  void commit() override;
  void rollback() override;

  std::shared_ptr<SqlResult> _start_transaction();
  std::shared_ptr<SqlResult> _commit();
  std::shared_ptr<SqlResult> _rollback();

  std::string get_current_schema() override;

  std::shared_ptr<Schema> _create_schema(const std::string &name);
  std::shared_ptr<Schema> _set_current_schema(const std::string &name);
  std::shared_ptr<SqlExecute> sql(const std::string &statement);
  std::shared_ptr<SqlResult> run_sql(const std::string sql,
                                     const shcore::Array_t &args = {});
  std::string quote_name(const std::string &id);

  std::string set_savepoint(const std::string &name = "");
  void release_savepoint(const std::string &name);
  void rollback_to(const std::string &name);

  bool is_open() const override;
  shcore::Value::Map_type_ref get_status() override;

  std::string get_ssl_cipher() const override {
    return _session->get_ssl_cipher() ? _session->get_ssl_cipher() : "";
  }

  shcore::Array_t get_schemas();

  std::string db_object_exists(std::string &type, const std::string &name,
                               const std::string &owner) override;

  std::shared_ptr<Result> set_fetch_warnings(bool enable);

  bool table_name_compare(const std::string &n1, const std::string &n2);

  void set_option(const char *option, int value) override;

  uint64_t get_connection_id() const override;
  std::string query_one_string(const std::string &query,
                               int field = 0) override;

  void kill_query() override;

  mysqlshdk::db::mysqlx::Session *session() { return _session.get(); }

  std::shared_ptr<mysqlshdk::db::ISession> get_core_session() override {
    return _session;
  }

 public:
  std::shared_ptr<mysqlshdk::db::IResult> execute_sql(
      const std::string &command, const shcore::Array_t &args = {}) override;

  shcore::Value _execute_mysqlx_stmt(const std::string &command,
                                     const shcore::Dictionary_t &args);

  std::shared_ptr<mysqlshdk::db::mysqlx::Result> execute_mysqlx_stmt(
      const std::string &command, const shcore::Dictionary_t &args);

  std::string get_uuid();

  void disable_prepared_statements() { m_allow_prepared_statements = false; }
  bool allow_prepared_statements() { return m_allow_prepared_statements; }

  void _enable_notices(const std::vector<std::string> &notices);
  shcore::Dictionary_t _fetch_notice();

  socket_t _get_socket_fd() const;

 protected:
  friend class SqlExecute;

  mutable std::shared_ptr<shcore::Value::Map_type> _schemas;
  std::function<void(const std::string &, bool exists)> update_schema_cache;

  std::shared_ptr<mysqlshdk::db::mysqlx::Result> execute_stmt(
      const std::string &ns, const std::string &command,
      const ::xcl::Argument_array &args);

  shcore::Value _execute_stmt(const std::string &ns, const std::string &command,
                              const ::xcl::Argument_array &args,
                              bool expect_data);

  std::string _retrieve_current_schema();

  std::shared_ptr<mysqlshdk::db::mysqlx::Session> _session;

  // The UUIDs generated by this Session instance MUST guarantee the same
  // host identifier no matter if it is the mac address of a randomly
  // generated number, this UUID is created to store the hist identifier
  std::string _session_uuid;

  bool _case_sensitive_table_names;
  void init();
  uint64_t _connection_id;
  uint64_t _savepoint_counter;

 private:
  bool m_allow_prepared_statements = true;
  std::list<shcore::Dictionary_t> m_notices;
  bool m_notices_enabled = false;

  void reset_session();
};

}  // namespace mysqlx
}  // namespace mysqlsh

#endif  // MODULES_DEVAPI_MOD_MYSQLX_SESSION_H_
