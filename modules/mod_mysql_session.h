/*
 * Copyright (c) 2015, 2022, Oracle and/or its affiliates.
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

#include <memory>

#include "modules/mod_common.h"
#include "mysqlshdk/libs/db/connection_options.h"
#include "mysqlshdk/libs/db/mysql/session.h"
#include "scripting/types.h"
#include "scripting/types_cpp.h"
#include "shellcore/base_session.h"
#include "shellcore/ishell_core.h"

namespace shcore {
class Shell_core;
class Proxy_object;
}  // namespace shcore

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
 */
#if DOXYGEN_JS
/**
 * \snippet mysql_session.js ClassicSession: SQL execution example
 */
#elif DOXYGEN_PY
/**
 * \snippet mysql_session.py ClassicSession: SQL execution example
 */
#endif

class SHCORE_PUBLIC ClassicSession
    : public ShellBaseSession,
      public std::enable_shared_from_this<ClassicSession> {
 public:
  ClassicSession();
  explicit ClassicSession(
      std::shared_ptr<mysqlshdk::db::mysql::Session> session);
  ClassicSession(const ClassicSession &session);
  virtual ~ClassicSession();

  // Virtual methods from object bridge
  std::string class_name() const override { return "ClassicSession"; };
  shcore::Value get_member(const std::string &prop) const override;

  // Virtual methods from ISession
  void connect(const mysqlshdk::db::Connection_options &data) override;
  void close() override;
  void create_schema(const std::string &name) override;
  void drop_schema(const std::string &name) override;
  void set_current_schema(const std::string &name) override;
  void start_transaction() override;
  void commit() override;
  void rollback() override;
  std::string get_current_schema() override;

  std::shared_ptr<ClassicResult> _start_transaction();
  std::shared_ptr<ClassicResult> _commit();
  std::shared_ptr<ClassicResult> _rollback();

  std::shared_ptr<ClassicResult> query(const std::string &query,
                                       const shcore::Array_t &args = {});

  std::shared_ptr<ClassicResult> run_sql(const std::string &query,
                                         const shcore::Array_t &args = {});

  shcore::Value::Map_type_ref get_status() override;

  std::string db_object_exists(std::string &type, const std::string &name,
                               const std::string &owner) override;

  static std::shared_ptr<shcore::Object_bridge> create(
      const mysqlshdk::db::Connection_options &co);

  std::shared_ptr<mysqlshdk::db::ISession> get_core_session() const override {
    return _session;
  }

  mysqlshdk::db::mysql::Session *session() const;

  uint64_t get_connection_id() const override;
  std::string query_one_string(const std::string &query,
                               int field = 0) override;
  std::string get_ssl_cipher() const override;
  std::shared_ptr<mysqlshdk::db::IResult> execute_sql(
      const std::string &query, const shcore::Array_t &args = {}) override;

  socket_t _get_socket_fd() const;

 public:
  SessionType session_type() const override { return SessionType::Classic; }

  void kill_query() override;

#if DOXYGEN_JS
  String uri;     //!< $(CLASSICSESSION_GETURI_BRIEF)
  String sshUri;  //!< $(CLASSICSESSION_GETSSHURI_BRIEF)
  String getUri();
  String getSshUri();
  ClassicResult runSql(String query, Array args = []);
  ClassicResult query(String query, Array args = []);
  Undefined close();
  ClassicResult startTransaction();
  ClassicResult commit();
  ClassicResult rollback();
  Bool isOpen();
#elif DOXYGEN_PY
  str uri;      //!< Same as get_uri()
  str ssh_uri;  //!< Same as get_ssh_uri()
  str get_uri();
  str get_ssh_uri();
  ClassicResult run_sql(str query, list args = []);
  ClassicResult query(str query, list args = []);
  None close();
  ClassicResult start_transaction();
  ClassicResult commit();
  ClassicResult rollback();
  bool is_open();
#endif

  bool is_open() const override;

 private:
  void init();
  std::shared_ptr<mysqlshdk::db::mysql::Session> _session;
};
}  // namespace mysql
}  // namespace mysqlsh

#endif
