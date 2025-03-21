/*
 * Copyright (c) 2015, 2025, Oracle and/or its affiliates.
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

// Interactive session access module
// Exposed as "session" in the shell

#ifndef MYSQLSHDK_INCLUDE_SHELLCORE_BASE_SESSION_H_
#define MYSQLSHDK_INCLUDE_SHELLCORE_BASE_SESSION_H_

#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>

#include "mysqlshdk/include/scripting/types.h"
#include "mysqlshdk/include/scripting/types/cpp.h"
#include "mysqlshdk/libs/db/connection_options.h"
#include "mysqlshdk/libs/db/session.h"

namespace mysqlsh {
#if DOXYGEN_CPP
//! Abstraction layer with core elements for all the session types
#endif

/**
 * @brief Cache for query attributes to be associated to the next user SQL
 * executed.
 *
 * This class serves as container and validator for the query attributes
 * coming from the 2 different places:
 *
 * - \query_attributes shell command
 * - setQueryAttributes() API
 *
 * Since the defined attributes are meant to be associated to the next user
 * SQL executed, the data needs to be cached while that happens.
 */
class Query_attribute_store {
 public:
  bool set(const std::string &name, const shcore::Value &value);
  bool set(const shcore::Dictionary_t &attributes);
  void handle_errors(bool raise_error = true);
  void clear();
  std::vector<mysqlshdk::db::Query_attribute> get_query_attributes(
      const std::function<
          std::unique_ptr<mysqlshdk::db::IQuery_attribute_value>(
              const shcore::Value &)> &translator_cb) const;

 private:
  // Real store of valid query attributes
  std::unordered_map<std::string, shcore::Value> m_store;

  // Honors the order of the attributes when given through \query_attributes
  std::vector<std::string> m_order;

  // Used to store the list of invalid attributes
  std::vector<std::string> m_exceeded;
  std::vector<std::string> m_invalid_names;
  std::vector<std::string> m_invalid_value_length;
  std::vector<std::string> m_unsupported_type;
};

class SHCORE_PUBLIC ShellBaseSession : public shcore::Cpp_object_bridge {
 public:
  using Client_data = shcore::Dictionary_t;

  static std::shared_ptr<ShellBaseSession> wrap_session(
      std::shared_ptr<mysqlshdk::db::ISession> session);

  ShellBaseSession();
  ShellBaseSession(const ShellBaseSession &s);
  virtual ~ShellBaseSession();

  // Virtual methods from object bridge
  virtual std::string &append_descr(std::string &s_out, int indent = -1,
                                    int quote_strings = 0) const;
  virtual std::string &append_repr(std::string &s_out) const;
  virtual void append_json(shcore::JSON_dumper &dumper) const;
  virtual bool operator==(const Object_bridge &other) const;

  // Virtual methods from ISession
  virtual void connect(const mysqlshdk::db::Connection_options &data) = 0;
  virtual void close() = 0;

  virtual bool is_open() const = 0;
  virtual shcore::Value::Map_type_ref get_status() = 0;

  virtual void set_current_schema(const std::string &name) = 0;

  std::shared_ptr<mysqlshdk::db::IResult> execute_sql(
      std::string_view query, const shcore::Array_t &args = {},
      const std::vector<mysqlshdk::db::Query_attribute> &query_attributes = {},
      bool use_sql_handlers = false);

  std::string uri(mysqlshdk::db::uri::Tokens_mask format =
                      mysqlshdk::db::uri::formats::full_no_password()) const;

  std::string ssh_uri() const;

  virtual SessionType session_type() const = 0;

  virtual std::string get_ssl_cipher() const = 0;

  virtual void set_option([[maybe_unused]] const char *option,
                          [[maybe_unused]] int value) {}
  virtual uint64_t get_connection_id() const { return 0; }
  virtual std::string query_one_string(const std::string &query,
                                       int field = 0) = 0;

  // NOTE(rennox): These two are used in DBA, that's why we let them here
  // Where they are used, assume a connection is established, so the values must
  // exist, we do not validate for nulls to let it crash on purpose since it
  // would reveal a failing logic elsewhere
  std::string get_user() { return _connection_options.get_user(); }
  std::string get_host() { return _connection_options.get_host(); }
  const mysqlshdk::db::Connection_options &get_connection_options() {
    return _connection_options;
  }

  virtual void reconnect();

  std::string get_default_schema();

  virtual std::string get_current_schema() = 0;

  virtual void start_transaction() = 0;
  virtual void commit() = 0;
  virtual void rollback() = 0;

  virtual std::shared_ptr<mysqlshdk::db::ISession> get_core_session() const = 0;

  virtual std::vector<mysqlshdk::db::Query_attribute> query_attributes() const {
    return {};
  }

  inline Query_attribute_store &query_attribute_store() {
    return m_query_attributes;
  }

  void set_query_attributes(const shcore::Dictionary_t &attributes);

  std::function<void(const std::string &, bool exists)> update_schema_cache;

  void set_client_data(const std::string &key, const shcore::Value &value);
  shcore::Value get_client_data(const std::string &key) const;

  virtual std::string track_system_variable(const std::string &variable);
  bool is_sql_mode_tracking_enabled() const { return m_tracking_sql_mode; }

 protected:
  std::string get_quoted_name(std::string_view name);
  // TODO(rennox): Note that these are now stored on the low level session
  // object too, they should be removed from here
  mysqlshdk::db::Connection_options _connection_options;

  std::string get_sql_mode();

 protected:
  std::string sub_query_placeholders(std::string_view query,
                                     const shcore::Array_t &args);

  int _tx_deep;

  Query_attribute_store m_query_attributes;

  Client_data m_client_data;
  bool m_tracking_sql_mode{false};

 private:
  std::string m_active_custom_sql;

  virtual std::shared_ptr<ShellBaseSession> get_shared_this() = 0;

  virtual std::shared_ptr<mysqlshdk::db::IResult> do_execute_sql(
      std::string_view query, const shcore::Array_t &args = {},
      const std::vector<mysqlshdk::db::Query_attribute> &query_attributes =
          {}) = 0;

#ifdef FRIEND_TEST
  FRIEND_TEST(Interrupt_mysql, sql_classic);
#endif
};

}  // namespace mysqlsh

#endif  // MYSQLSHDK_INCLUDE_SHELLCORE_BASE_SESSION_H_
