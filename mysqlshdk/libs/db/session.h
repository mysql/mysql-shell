/*
 * Copyright (c) 2017, 2019, Oracle and/or its affiliates. All rights reserved.
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

// MySQL DB access module, for use by plugins and others
// For the module that implements interactive DB functionality see mod_db

#ifndef MYSQLSHDK_LIBS_DB_SESSION_H_
#define MYSQLSHDK_LIBS_DB_SESSION_H_

#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

#include "mysqlshdk/libs/db/connection_options.h"
#include "mysqlshdk/libs/db/result.h"
#include "mysqlshdk/libs/db/ssl_options.h"
#include "mysqlshdk/libs/utils/utils_sqlstring.h"
#include "mysqlshdk/libs/utils/version.h"
#include "mysqlshdk_export.h"

namespace mysqlshdk {
namespace db {

inline bool is_mysql_client_error(int code) {
  return code >= 2000 && code <= 2999;
}

inline bool is_mysql_server_error(int code) {
  return !is_mysql_client_error(code) && code > 0 && code < 50000;
}

class Error : public std::runtime_error {
 public:
  Error(const char *what, int code) : std::runtime_error(what), code_(code) {}

  Error(const char *what, int code, const char *sqlstate)
      : std::runtime_error(what), code_(code), sqlstate_(sqlstate) {}

  int code() const { return code_; }

  const char *sqlstate() const { return sqlstate_.c_str(); }

  std::string format() const {
    if (sqlstate_.empty())
      return "MySQL Error " + std::to_string(code()) + ": " + what();
    else
      return "MySQL Error " + std::to_string(code()) + " (" + sqlstate() +
             "): " + what();
  }

 private:
  int code_ = 0;
  std::string sqlstate_;
};

class SHCORE_PUBLIC ISession {
 public:
  // Connection
  virtual void connect(const std::string &uri) {
    connect(mysqlshdk::db::Connection_options(uri));
  }

  virtual void connect(const mysqlshdk::db::Connection_options &data) = 0;

  virtual uint64_t get_connection_id() const = 0;

  virtual const mysqlshdk::db::Connection_options &get_connection_options()
      const = 0;

  virtual const char *get_ssl_cipher() const = 0;

  virtual mysqlshdk::utils::Version get_server_version() const = 0;

  // Execution
  virtual std::shared_ptr<IResult> query(const std::string &sql,
                                         bool buffered = false) {
    return querys(sql.data(), sql.length(), buffered);
  }

  virtual void execute(const std::string &sql) {
    executes(sql.data(), sql.length());
  }

  virtual std::shared_ptr<IResult> querys(const char *sql, size_t len,
                                          bool buffered = false) = 0;

  virtual void executes(const char *sql, size_t len) = 0;

  /**
   * Execute query and perform client-side placeholder substitution
   * @param  sql  query with placeholders
   * @param  args values for placeholders
   * @return      query result
   *
   * @example
   * auto result = session->queryf("SELECT * FROM tbl WHERE id = ?", my_id);
   */
  template <typename... Args>
  inline std::shared_ptr<IResult> queryf(const std::string &sql,
                                         const Args &... args) {
    return query(shcore::sqlformat(sql, args...));
  }

  template <typename... Args>
  inline void executef(const std::string &sql, const Args &... args) {
    execute(shcore::sqlformat(sql, args...));
  }

  // Disconnection
  virtual void close() = 0;

  virtual bool is_open() const = 0;

  virtual const Error *get_last_error() const = 0;

  virtual ~ISession() {}

  // TODO(rennox): This is a convenient function as URI is being retrieved from
  // the session in many places, eventually should be removed, if needed URI
  // should be retrieved as get_connection_options().as_uri()
  std::string uri(mysqlshdk::db::uri::Tokens_mask format =
                      mysqlshdk::db::uri::formats::full_no_password()) const {
    return get_connection_options().as_uri(format);
  }
};

}  // namespace db
}  // namespace mysqlshdk
#endif  // MYSQLSHDK_LIBS_DB_SESSION_H_
