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

#ifndef MYSQLSHDK_INCLUDE_SHELLCORE_BASE_SESSION_H_
#define MYSQLSHDK_INCLUDE_SHELLCORE_BASE_SESSION_H_

#include <functional>

#include "scripting/types.h"
#include "scripting/types_cpp.h"
#include "shellcore/ishell_core.h"
#include "mysqlshdk/libs/db/connection_options.h"

namespace mysqlsh {
#if DOXYGEN_CPP
//! Abstraction layer with core elements for all the session types
#endif
class SHCORE_PUBLIC ShellBaseSession : public shcore::Cpp_object_bridge {
public:
  ShellBaseSession();
  ShellBaseSession(const ShellBaseSession& s);
  virtual ~ShellBaseSession();

  // Virtual methods from object bridge
  virtual std::string &append_descr(std::string &s_out, int indent = -1, int quote_strings = 0) const;
  virtual std::string &append_repr(std::string &s_out) const;
  virtual void append_json(shcore::JSON_dumper& dumper) const;
  virtual bool operator == (const Object_bridge &other) const;

  // Virtual methods from ISession
  virtual void connect(const mysqlshdk::db::Connection_options& data) = 0;
  virtual void close() = 0;

  virtual bool is_open() const = 0;
  virtual shcore::Value::Map_type_ref get_status() = 0;
  virtual std::string get_node_type() { return "mysql";  }
  virtual void create_schema(const std::string& name) = 0;
  virtual void drop_schema(const std::string &name) = 0;
  virtual void set_current_schema(const std::string &name) = 0;
  virtual shcore::Object_bridge_ref get_schema(const std::string &name);

  // This function should be execute_sql, but BaseSession and ClassicSession
  // Have another function with the same signature except the return value
  // Using this name temporarily, at the end only one execute_sql
  virtual shcore::Object_bridge_ref raw_execute_sql(const std::string& query) = 0;

  std::string uri(mysqlshdk::db::uri::Tokens_mask format =
                      mysqlshdk::db::uri::formats::full_no_password()) const;

  virtual SessionType session_type() const = 0;

  virtual std::string get_ssl_cipher() const = 0;

  virtual std::string db_object_exists(std::string &type,
                                       const std::string &name,
                                       const std::string &owner) = 0;

  virtual void set_option(const char *option, int value) {}
  virtual uint64_t get_connection_id() const { return 0; }
  virtual std::string query_one_string(const std::string &query,
                                       int field = 0) = 0;

  // NOTE(rennox): These two are used in DBA, that's why we let them here
  // Where they are used, assume a connection is established, so the values must
  // exist, we do not validate for nulls to let it crash on purpose since it
  // would reveal a failing logic elsewhere
  std::string get_user() { return _connection_options.get_user(); }
  std::string get_host() {return _connection_options.get_host(); }
  const mysqlshdk::db::Connection_options & get_connection_options() {
    return _connection_options;
  }

  virtual void reconnect();

  std::string get_default_schema();

  virtual std::string get_current_schema() = 0;

  virtual void start_transaction() = 0;
  virtual void commit() = 0;
  virtual void rollback() = 0;

  virtual void kill_query() = 0;

protected:
  std::string get_quoted_name(const std::string& name);
  mysqlshdk::db::Connection_options _connection_options;

 protected:
  // Wrap around query executions, to make them cancellable
  // If a lot of statements must be executed in a loop, it may be a good idea
  // to wrap the whole loop with an Interruptible block, so that a single
  // handler is used for the whole loop.
  class Interruptible {
   public:
    explicit Interruptible(ShellBaseSession *owner) : _owner(owner) {
      _owner->begin_query();
    }

    ~Interruptible() { _owner->end_query(); }

   private:
    ShellBaseSession *_owner;
  };

  mutable std::shared_ptr<shcore::Value::Map_type> _schemas;
  std::function<void(const std::string&, bool exists)> update_schema_cache;

  int _tx_deep;

 private:
  void init();

  friend class Query_guard;
  void begin_query();
  void end_query();
  mutable int _guard_active = 0;

#ifdef FRIEND_TEST
  FRIEND_TEST(Interrupt_mysql, sql_classic);
  FRIEND_TEST(Interrupt_mysql, sql_node);
#endif
};

}  // namespace mysqlsh

#endif  // MYSQLSHDK_INCLUDE_SHELLCORE_BASE_SESSION_H_
