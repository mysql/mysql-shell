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

#include "base_session.h"

#include "shellcore/object_factory.h"
#include "shellcore/shell_core.h"
#include "shellcore/lang_base.h"
#include "shellcore/common.h"

#include "shellcore/proxy_object.h"

#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/format.hpp>
#include <boost/pointer_cast.hpp>

#include "mod_mysql_session.h"
#include "mod_mysqlx_session.h"

#define MAX_COLUMN_LENGTH 1024
#define MIN_COLUMN_LENGTH 4

using namespace mysh;
using namespace shcore;

#include <iostream>

bool mysh::parse_mysql_connstring(const std::string &connstring,
                            std::string &protocol, std::string &user, std::string &password,
                            std::string &host, int &port, std::string &sock,
                            std::string &db, int &pwd_found)
{
  // format is [protocol://][user[:pass]]@host[:port][/db] or user[:pass]@::socket[/db], like what cmdline utilities use
  pwd_found = 0;
  std::string remaining = connstring;

  std::string::size_type p;
  p = remaining.find("://");
  if (p != std::string::npos)
  {
    protocol = connstring.substr(0, p);
    remaining = remaining.substr(p + 3);
  }

  std::string s = remaining;
  p = remaining.find('/');
  if (p != std::string::npos)
  {
    db = remaining.substr(p + 1);
    s = remaining.substr(0, p);
  }
  p = s.rfind('@');
  std::string user_part;
  std::string server_part = (p == std::string::npos) ? s : s.substr(p + 1);

  if (p == std::string::npos)
  {
    // by default, connect using the current OS username
#ifdef _WIN32
    //XXX find out current username here
#else
    const char *tmp = getenv("USER");
    user_part = tmp ? tmp : "";
#endif
  }
  else
    user_part = s.substr(0, p);

  if ((p = user_part.find(':')) != std::string::npos)
  {
    user = user_part.substr(0, p);
    password = user_part.substr(p + 1);
    pwd_found = 1;
  }
  else
    user = user_part;

  p = server_part.find(':');
  if (p != std::string::npos)
  {
    host = server_part.substr(0, p);
    server_part = server_part.substr(p + 1);
    p = server_part.find(':');
    if (p != std::string::npos)
      sock = server_part.substr(p + 1);
    else
    if (!sscanf(server_part.substr(0, p).c_str(), "%i", &port))
      return false;
  }
  else
    host = server_part;
  return true;
}

std::string mysh::strip_password(const std::string &connstring)
{
  std::string remaining = connstring;
  std::string password;

  std::string::size_type p;
  p = remaining.find("://");
  if (p != std::string::npos)
  {
    remaining = remaining.substr(p + 3);
  }

  std::string s = remaining;
  p = remaining.find('/');
  if (p != std::string::npos)
  {
    s = remaining.substr(0, p);
  }
  p = s.rfind('@');
  std::string user_part;

  if (p == std::string::npos)
  {
    // by default, connect using the current OS username
#ifdef _WIN32
    //XXX find out current username here
#else
    const char *tmp = getenv("USER");
    user_part = tmp ? tmp : "";
#endif
  }
  else
    user_part = s.substr(0, p);

  if ((p = user_part.find(':')) != std::string::npos)
  {
    password = user_part.substr(p + 1);
    if (!password.empty())
    {
      std::string uri_stripped = connstring;
      std::string::size_type i = uri_stripped.find(":" + password);
      if (i != std::string::npos)
        uri_stripped.erase(i, password.length() + 1);

      return uri_stripped;
    }
  }

  // no password to strip, return original one
  return connstring;
}

boost::shared_ptr<mysh::BaseSession> mysh::connect_session(const shcore::Argument_list &args)
{
  std::string protocol;
  std::string user;
  std::string pass;
  std::string host;
  std::string sock;
  std::string db;

  int pwd_found;
  int port = 0;

  std::string uri = args.string_at(0);

  boost::shared_ptr<BaseSession> ret_val;

  if (!parse_mysql_connstring(uri, protocol, user, pass, host, port, sock, db, pwd_found))
    throw shcore::Exception::argument_error("Could not parse URI for MySQL connection");

  if (protocol.empty() || protocol == "mysql")
  {
    ret_val.reset(new mysql::Session());
  }
  else if (protocol == "mysqlx")
  {
    ret_val.reset(new mysh::mysqlx::NodeSession());
  }
  else
    throw shcore::Exception::argument_error("Invalid protocol specified for MySQL connection.");

  ret_val->connect(args);

  return ret_val;
}

BaseSession::BaseSession()
{
  add_method("getDefaultSchema", boost::bind(&BaseSession::get_member_method, this, _1, "getDefaultSchema", "defaultSchema"), NULL);
  add_method("getSchema", boost::bind(&BaseSession::get_schema, this, _1), "name", shcore::String, NULL);
  add_method("getSchemas", boost::bind(&BaseSession::get_member_method, this, _1, "getSchemas", "schemas"), NULL);
  add_method("getUri", boost::bind(&BaseSession::get_member_method, this, _1, "getUri", "uri"), NULL);
  add_method("setDefaultSchema", boost::bind(&BaseSession::set_default_schema, this, _1), "name", shcore::String, NULL);
}

std::string &BaseSession::append_descr(std::string &s_out, int UNUSED(indent), int UNUSED(quote_strings)) const
{
  if (!is_connected())
    s_out.append("<" + class_name() + ":disconnected>");
  else
    s_out.append("<" + class_name() + ":" + uri() + ">");
  return s_out;
}

std::string &BaseSession::append_repr(std::string &s_out) const
{
  return append_descr(s_out, false);
}

std::vector<std::string> BaseSession::get_members() const
{
  std::vector<std::string> members(Cpp_object_bridge::get_members());
  members.push_back("defaultSchema");
  members.push_back("schemas");
  members.push_back("uri");
  return members;
}

shcore::Value BaseSession::get_member_method(const shcore::Argument_list &args, const std::string& method, const std::string& prop)
{
  std::string function = class_name() + "::" + method;
  args.ensure_count(0, function.c_str());

  return get_member(prop);
}

bool BaseSession::operator == (const Object_bridge &other) const
{
  return class_name() == other.class_name() && this == &other;
}