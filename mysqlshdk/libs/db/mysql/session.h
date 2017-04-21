/*
 * Copyright (c) 2017, Oracle and/or its affiliates. All rights reserved.
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

// MySQL DB access module, for use by plugins and others
// For the module that implements interactive DB functionality see mod_db

#ifndef _CORELIBS_DB_MYSQL_SESSION_H_
#define _CORELIBS_DB_MYSQL_SESSION_H_

#include <mysql.h>
#include <set>

#include "utils/utils_time.h"
#include "mysqlshdk/libs/db/session.h"
#include "mysqlshdk/libs/db/mysql/result.h"
#include "mysqlshdk/libs/db/ssl_info.h"

namespace mysqlshdk {
namespace db {
namespace mysql {
/*
 * Session implementation for the MySQL protocol.
 *
 * This class can only be used by the Session class in order to guarantee
 * that any instance is always contained on a shared pointer, this is required
 * because the results returned by the query function require a reference to
 * the Session that generated them.
 */
class Session_impl : public std::enable_shared_from_this<Session_impl> {
  friend class Session; // The Session class instantiates this class
  friend class Result;  // The Reslt class uses some functions of this class

public:
  ~Session_impl();

private:
  Session_impl();
  void connect(const std::string &uri, const char *password = NULL);
  void connect(const std::string &host, int port, const std::string &socket, const std::string &user,
                       const std::string &password, const std::string &schema,
                       const mysqlshdk::utils::Ssl_info& ssl_info);

  IResult* query(const std::string& sql, bool buffered);
  void execute(const std::string& sql);

  void start_transaction();
  void commit();
  void rollback();

  void close();

  bool next_data_set();
  void prepare_fetch(Result *target, bool buffered);

  std::string uri() { return _uri; }

  // Utility functions to retriev session status
  unsigned long get_thread_id() { _prev_result.reset(); return mysql_thread_id(_mysql); }
  unsigned long get_protocol_info() { _prev_result.reset(); return mysql_get_proto_info(_mysql); }
  const char* get_connection_info() { _prev_result.reset(); return mysql_get_host_info(_mysql); }
  const char* get_server_info() { _prev_result.reset(); return mysql_get_server_info(_mysql); }
  const char* get_stats() { _prev_result.reset(); return mysql_stat(_mysql); }
  const char* get_ssl_cipher() { _prev_result.reset(); return mysql_get_ssl_cipher(_mysql); }

  Result* run_sql(const std::string &sql, bool lazy_fetch = true);
  bool setup_ssl(const mysqlshdk::utils::Ssl_info& ssl_info);
  void throw_on_connection_fail();
  std::string _uri;
  MYSQL *_mysql;
  MySQL_timer _timer;
  int _tx_deep;

  std::shared_ptr<MYSQL_RES> _prev_result;
};

class SHCORE_PUBLIC Session : public ISession, public std::enable_shared_from_this<Session> {
public:
  Session() { _impl.reset(new Session_impl()); }
  virtual void connect(const std::string &uri, const char *password = NULL) {
    _impl->connect(uri, password);
  }

  virtual void connect(const std::string &host, int port, const std::string &socket, const std::string &user,
                       const std::string &password, const std::string &schema,
                       const struct mysqlshdk::utils::Ssl_info& ssl_info) {
    _impl->connect(host, port, socket, user, password, schema, ssl_info);
  }

  virtual std::unique_ptr<IResult> query(const std::string& sql, bool buffered) { return std::move(std::unique_ptr<IResult>(_impl->query(sql, buffered))); }
  virtual void execute(const std::string& sql) { _impl->execute(sql); }
  virtual void start_transaction() { _impl->start_transaction(); }
  virtual void commit() { _impl->commit(); }
  virtual void rollback() { _impl->rollback(); }
  virtual void close() { _impl->close(); }
  virtual const char* get_ssl_cipher() { return _impl->get_ssl_cipher(); }

private:
  std::shared_ptr<Session_impl> _impl;
};
}
}
}
#endif
