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

// Interactive session access module
// Exposed as "session" in the shell

#ifndef _MOD_CORE_SESSION_H_
#define _MOD_CORE_SESSION_H_

#include "scripting/types.h"
#include "scripting/types_cpp.h"
#include "shellcore/ishell_core.h"
#include "mysqlshdk/libs/db/ssl_info.h"

namespace mysqlsh {
#if DOXYGEN_CPP
//! Abstraction layer with core elements for all the session types
#endif
class SHCORE_PUBLIC ShellBaseSession : public shcore::Cpp_object_bridge {
public:
  ShellBaseSession();
  ShellBaseSession(const ShellBaseSession& s);
  virtual ~ShellBaseSession() {};

  // Virtual methods from object bridge
  virtual std::string &append_descr(std::string &s_out, int indent = -1, int quote_strings = 0) const;
  virtual std::string &append_repr(std::string &s_out) const;
  virtual void append_json(shcore::JSON_dumper& dumper) const;
  virtual bool operator == (const Object_bridge &other) const;

  // Virtual methods from ISession
  virtual void connect(const shcore::Argument_list &args) = 0;
  virtual void close() = 0;

  virtual bool is_open() const = 0;
  virtual shcore::Value::Map_type_ref get_status() = 0;
  virtual std::string get_node_type() { return "mysql";  }
  virtual void create_schema(const std::string& name) = 0;
  virtual void drop_schema(const std::string &name) = 0;
  virtual void set_current_schema(const std::string &name) = 0;
  virtual shcore::Object_bridge_ref get_schema(const std::string &name) const;

  // This function should be execute_sql, but BaseSession and ClassicSession
  // Have another function with the same signature except the return value
  // Using this name temporarily, at the end only one execute_sql
  virtual shcore::Object_bridge_ref raw_execute_sql(const std::string& query) const = 0;

  std::string uri() { return _uri; };
  std::string address();

  virtual std::string db_object_exists(std::string &type, const std::string &name, const std::string& owner) const = 0;

  virtual void set_option(const char *option, int value) {}
  virtual uint64_t get_connection_id() const { return 0; }

  std::string get_user() { return _user; }
  std::string get_password() { return _password; }

  virtual void reconnect();
  virtual int get_default_port() = 0;

  std::string get_ssl_ca() { return _ssl_info.ca; }
  std::string get_ssl_key() { return _ssl_info.key; }
  std::string get_ssl_cert() { return _ssl_info.cert; }

  std::string get_default_schema() { return _default_schema; }

  virtual void start_transaction() = 0;
  virtual void commit() = 0;
  virtual void rollback() = 0;

protected:
  std::string get_quoted_name(const std::string& name);

  // These will be stored in the instance, it's possible later
  // we expose functions to retrieve this data (not now tho)
  std::string _user;
  std::string _password;
  std::string _host;
  int _port;
  std::string _sock;
  std::string _schema;
  std::string _auth_method;
  std::string _uri;
  struct mysqlshdk::utils::Ssl_info _ssl_info;

  void load_connection_data(const shcore::Argument_list &args);

protected:
  std::string _default_schema;
  mutable std::shared_ptr<shcore::Value::Map_type> _schemas;
  std::function<void(const std::string&, bool exists)> update_schema_cache;
  int _tx_deep;
};

std::shared_ptr<mysqlsh::ShellBaseSession> SHCORE_PUBLIC connect_session(const shcore::Argument_list &args, SessionType session_type);
std::shared_ptr<mysqlsh::ShellBaseSession> SHCORE_PUBLIC connect_session(const std::string &uri, const std::string &password, SessionType session_type);
};

#endif
