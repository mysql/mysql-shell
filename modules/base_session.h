/*
 * Copyright (c) 2015, Oracle and/or its affiliates. All rights reserved.
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
  bool MOD_PUBLIC parse_mysql_connstring(const std::string &connstring,
                              std::string &protocol, std::string &user, std::string &password,
                              std::string &host, int &port, std::string &sock,
                              std::string &db, int &pwd_found);

  std::string MOD_PUBLIC strip_password(const std::string &connstring);

  class MOD_PUBLIC BaseSession : public shcore::Cpp_object_bridge
  {
  public:
    BaseSession();
    virtual ~BaseSession() {};

    // Virtual methods from object bridge
    virtual std::string &append_descr(std::string &s_out, int indent = -1, int quote_strings = 0) const;
    virtual std::string &append_repr(std::string &s_out) const;

    virtual bool operator == (const Object_bridge &other) const;

    virtual std::vector<std::string> get_members() const;

    // Virtual methods from ISession
    virtual shcore::Value connect(const shcore::Argument_list &args) = 0;
    virtual void disconnect() = 0;
    virtual shcore::Value executeSql(const shcore::Argument_list &args) = 0;
    virtual bool is_connected() const = 0;
    virtual std::string uri() const = 0;

    virtual shcore::Value get_schema(const shcore::Argument_list &args) const = 0;
    virtual shcore::Value set_default_schema(const shcore::Argument_list &args) = 0;

    // Helper method to retrieve properties using a method
    shcore::Value get_member_method(const shcore::Argument_list &args, const std::string& method, const std::string& prop);
  };

  boost::shared_ptr<mysh::BaseSession> MOD_PUBLIC connect_session(const shcore::Argument_list &args);
};

#endif
