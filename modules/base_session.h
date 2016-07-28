/*
 * Copyright (c) 2015, 2016, Oracle and/or its affiliates. All rights reserved.
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

#include "mod_common.h"
#include "shellcore/types.h"
#include "shellcore/types_cpp.h"
#include "shellcore/ishell_core.h"

namespace mysh
{
#if DOXYGEN_CPP
  //! Abstraction layer with core elements for all the session types
#endif
  class SHCORE_PUBLIC ShellBaseSession : public shcore::Cpp_object_bridge
  {
  public:
    ShellBaseSession();
    ShellBaseSession(const ShellBaseSession& s);
    virtual ~ShellBaseSession() {};

    // Virtual methods from object bridge
    virtual std::string &append_descr(std::string &s_out, int indent = -1, int quote_strings = 0) const;
    virtual std::string &append_repr(std::string &s_out) const;
    virtual void append_json(shcore::JSON_dumper& dumper) const;

    virtual bool operator == (const Object_bridge &other) const;

    virtual shcore::Value get_member(const std::string &prop) const;

    // Virtual methods from ISession
    virtual shcore::Value connect(const shcore::Argument_list &args) = 0;
    virtual shcore::Value close(const shcore::Argument_list &args) = 0;
    virtual bool is_connected() const = 0;
    virtual shcore::Value get_status(const shcore::Argument_list &args) = 0;
    virtual shcore::Value get_capability(const std::string &name) { return shcore::Value(); }
    std::string uri() { return _uri; };

    virtual std::string db_object_exists(std::string &type, const std::string &name, const std::string& owner) const = 0;

    virtual void set_option(const char *option, int value) {}
    virtual uint64_t get_connection_id() const { return 0; }

  protected:
    std::string get_quoted_name(const std::string& name);
    virtual int get_default_port() = 0;

    // These will be stored in the instance, it's possible later
    // we expose functions to retrieve this data (not now tho)
    std::string _user;
    std::string _password;
    std::string _host;
    int _port;
    std::string _sock;
    std::string _schema;
    std::string _ssl_ca;
    std::string _ssl_cert;
    std::string _ssl_key;
    std::string _auth_method;
    std::string _uri;

    void load_connection_data(const shcore::Argument_list &args);
  private:
    void init();

    shcore::Value is_open(const shcore::Argument_list &args);

  };

#if DOXYGEN_CPP
  //! Abstraction layer with core elements for development sessions
  //! This is the parent class for development sessions implemented in both protocols
#endif
  class SHCORE_PUBLIC ShellDevelopmentSession : public ShellBaseSession
  {
  public:
    ShellDevelopmentSession();
    ShellDevelopmentSession(const ShellDevelopmentSession& s);
    virtual ~ShellDevelopmentSession() {};

    virtual shcore::Value get_member(const std::string &prop) const;

    virtual shcore::Value create_schema(const shcore::Argument_list &args) = 0;
    virtual shcore::Value drop_schema(const shcore::Argument_list &args) = 0;
    virtual shcore::Value drop_schema_object(const shcore::Argument_list &args, const std::string& type) = 0;

    virtual shcore::Value get_schema(const shcore::Argument_list &args) const = 0;
    virtual shcore::Value get_schemas(const shcore::Argument_list &args) const = 0;

  protected:
    std::string _default_schema;
    mutable boost::shared_ptr<shcore::Value::Map_type> _schemas;
    std::function<void(const std::string&, bool exists)> update_schema_cache;

  private:
    void init();
  };

  // Abstraction layer with core elements for admin sessions
  // This is the parent class for admin sessions implemented in both protocols
  class SHCORE_PUBLIC ShellAdminSession : public ShellBaseSession
  {
    // NOTE: At the moment this class is just to keep abstraction consistent
  protected:
    mutable boost::shared_ptr<shcore::Value::Map_type> _farms;
    mutable std::string _default_farm;
  };

  boost::shared_ptr<mysh::ShellDevelopmentSession> SHCORE_PUBLIC connect_session(const shcore::Argument_list &args, SessionType session_type);
  boost::shared_ptr<mysh::ShellAdminSession> SHCORE_PUBLIC connect_admin_session(const shcore::Argument_list &args);
};

#endif
