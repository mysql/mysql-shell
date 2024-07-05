/*
 * Copyright (c) 2017, 2024, Oracle and/or its affiliates.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.0,
 * as published by the Free Software Foundation.
 *
 * This program is designed to work with certain software (including
 * but not limited to OpenSSL) that is licensed under separate terms,
 * as designated in a particular file or component or in included license
 * documentation.  The authors of MySQL hereby grant you an additional
 * permission to link the program and your derivative works with the
 * separately licensed software that they have either included with
 * the program or referenced in the documentation.
 *
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

#include <cstring>
#include <fstream>
#include <iostream>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#ifdef _WIN32
#include <winsock2.h>
using socket_t = SOCKET;
#else
using socket_t = int;
#endif /* _WIN32 */

#include "mysqlshdk/libs/db/connection_options.h"

#include "mysqlshdk/libs/utils/error.h"
#include "mysqlshdk/libs/utils/utils_sqlstring.h"
#include "mysqlshdk/libs/utils/version.h"

namespace mysqlshdk {
namespace db {

class SHCORE_PUBLIC IResult;

inline bool is_mysql_client_error(int code) {
  return code >= 2000 && code <= 2999;
}

inline bool is_mysql_server_error(int code) {
  return !is_mysql_client_error(code) && code > 0 && code < 50000;
}

/**
 * @brief Base interface for query attribute values.
 *
 */
struct IQuery_attribute_value {
  virtual ~IQuery_attribute_value() {}
};

/**
 * @brief Normalized query attribute.
 *
 * This class represents a normalized query attribute which in general consist
 * of  a name/value pair.
 *
 * The value must be valid for the target connector, so the
 * IQuery_attribute_value interface is used at this level.
 */
struct Query_attribute {
  Query_attribute(std::string n,
                  std::unique_ptr<IQuery_attribute_value> v) noexcept;

  std::string name;
  std::unique_ptr<IQuery_attribute_value> value;
};

class Error : public shcore::Error {
 public:
  Error() : Error("", 0, nullptr) {}

  Error(const char *what, int code) : Error(what, code, nullptr) {}

  Error(const std::string &what, int code) : shcore::Error(what, code) {}

  Error(const char *what, int code, const char *sqlstate)
      : shcore::Error(what, code), sqlstate_(sqlstate ? sqlstate : "") {}

  const char *sqlstate() const { return sqlstate_.c_str(); }

  std::string format() const override {
    if (sqlstate_.empty())
      return "MySQL Error " + std::to_string(code()) + ": " + what();
    else
      return "MySQL Error " + std::to_string(code()) + " (" + sqlstate() +
             "): " + what();
  }

 private:
  std::string sqlstate_;
};

class SHCORE_PUBLIC ISession {
 public:
  // Connection
  virtual void connect(const std::string &uri) {
    connect(mysqlshdk::db::Connection_options(uri));
  }

  void connect(const mysqlshdk::db::Connection_options &data);

  // thread safe
  virtual uint64_t get_connection_id() const = 0;

  virtual socket_t get_socket_fd() const = 0;

  // thread safe
  virtual const mysqlshdk::db::Connection_options &get_connection_options()
      const = 0;

  virtual const char *get_ssl_cipher() const = 0;

  virtual mysqlshdk::utils::Version get_server_version() const = 0;

  // Execution
  virtual std::shared_ptr<IResult> querys(
      const char *sql, size_t len, bool buffered = false,
      const std::vector<Query_attribute> &query_attributes = {}) = 0;

  inline std::shared_ptr<IResult> query(
      std::string_view sql, bool buffered = false,
      const std::vector<mysqlshdk::db::Query_attribute> &query_attributes =
          {}) {
    return querys(sql.data(), sql.length(), buffered, query_attributes);
  }

  /**
   * Executes a query with a user-defined function (UDF)
   *
   * Due to how UDFs are implemented in the server, they behave differently than
   * non UDFs, which means a UDF aware query method must used when dealing with
   * them.
   *
   * @param sql The SQL query with an UDF to execute
   * @param buffered True if the results should be buffered (read all at once),
   * false if they should be read row-by-row
   * @return std::shared_ptr<IResult> the result set of the query
   */
  virtual std::shared_ptr<IResult> query_udf(std::string_view sql,
                                             bool buffered = false) = 0;

  virtual void executes(const char *sql, size_t len) = 0;

  inline void execute(std::string_view sql) {
    executes(sql.data(), sql.length());
  }

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
  inline std::shared_ptr<IResult> queryf(std::string sql, Args &&...args) {
    return query(
        shcore::sqlformat(std::move(sql), std::forward<Args>(args)...));
  }

  template <typename... Args>
  inline void executef(std::string sql, Args &&...args) {
    execute(shcore::sqlformat(std::move(sql), std::forward<Args>(args)...));
  }

  // Disconnection
  void close();

  virtual bool is_open() const = 0;

  virtual const Error *get_last_error() const = 0;

  virtual uint32_t get_server_status() const = 0;

  virtual ~ISession() = default;

  virtual std::string escape_string(const std::string_view s) const = 0;

  bool has_sql_mode() const { return static_cast<bool>(m_sql_mode); }

  std::string get_sql_mode() {
    if (!m_sql_mode && is_open()) refresh_sql_mode();
    return m_sql_mode.value_or("");
  }

  bool ansi_quotes_enabled() {
    if (!m_sql_mode && is_open()) refresh_sql_mode();
    return m_ansi_quotes_enabled;
  }

  bool no_backslash_escapes_enabled() {
    if (!m_sql_mode && is_open()) refresh_sql_mode();
    return m_no_backslash_escapes_enabled;
  }

  bool dollar_quoted_strings() {
    if (!m_dollar_quoted_strings.has_value() && is_open()) {
      // - $foo became deprecated in 8.0.32, but is still allowed as of now
      // - create function xxx() returns int
      //    deterministic language javascript as
      //    $tag$
      //      code;
      //    $tag$;
      // became valid in 8.1.0, although it's only actually usable in 8.2.0
      // - $tag$ as a generic string quote is planned for the future

      m_dollar_quoted_strings =
          get_server_version() >= mysqlshdk::utils::Version(8, 1, 0);
    }
    return m_dollar_quoted_strings.value_or(false);
  }

  void refresh_sql_mode();

 protected:
  virtual void do_connect(const mysqlshdk::db::Connection_options &data) = 0;
  virtual void do_close() = 0;

 private:
  std::optional<std::string> m_sql_mode;
  bool m_ansi_quotes_enabled = false;
  bool m_no_backslash_escapes_enabled = false;
  std::optional<bool> m_dollar_quoted_strings;
};

}  // namespace db
}  // namespace mysqlshdk
#endif  // MYSQLSHDK_LIBS_DB_SESSION_H_
