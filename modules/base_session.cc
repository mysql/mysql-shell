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

#ifdef HAVE_LIBMYSQLCLIENT
#include "mod_mysql_session.h"
#endif
#include "mod_mysqlx_session.h"

#define MAX_COLUMN_LENGTH 1024
#define MIN_COLUMN_LENGTH 4

using namespace mysh;
using namespace shcore;

bool mysh::parse_mysql_connstring(const std::string &connstring,
                            std::string &protocol, std::string &user, std::string &password,
                            std::string &host, int &port, std::string &sock,
                            std::string &db, int &pwd_found, std::string& ssl_ca, std::string& ssl_cert, std::string& ssl_key)
{
  // format is [user[:pass]]@host[:port][/db] or user[:pass]@::socket[/db], like what cmdline utilities use
  // with SSL [user[:pass]]@host[:port][/db]?ssl_ca=path_to_ca&ssl_cert=path_to_cert&ssl_key=path_to_key
  pwd_found = 0;
  std::string remaining = connstring;

  std::string::size_type p;
  std::string::size_type p_query;
  p = remaining.find("://");
  if (p != std::string::npos)
  {
    protocol = connstring.substr(0, p);
    remaining = remaining.substr(p + 3);
  }

  std::string s = remaining;
  p = remaining.find('/');
  p_query = remaining.find('?');
  if (p != std::string::npos)
  {
    if (p_query == std::string::npos)
      db = remaining.substr(p + 1);
    else
      db = remaining.substr(p + 1, p_query);
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

  std::map<std::string, std::string> ssl_data;
  ssl_data["ssl_ca"] = "";
  ssl_data["ssl_key"] = "";
  ssl_data["ssl_cert"] = "";
  if (p_query != std::string::npos)
  {
    // Parsing SSL data
    std::string::size_type p_next = p_query;
    do
    {
      ++p_next;
      std::string::size_type p_eq = remaining.find('=', p_next);
      if (p_eq == std::string::npos)
        throw Exception::argument_error((boost::format("Expected '=' in connection string uri starting at pos %d.") % p_next).str());
      const std::string name = remaining.substr(p_next, p_eq - p_next);
      p_next = remaining.find('&', p_next + 1);
      const std::string value = remaining.substr(p_eq + 1, p_next - p_eq - 1);

      if (ssl_data.find(name) == ssl_data.end())
        throw Exception::argument_error((boost::format("Unknown key provided %s in connection string uri (expected any of ssl_ca, ssl_cert, ssl_key)") % name).str());

      ssl_data[name] = value;
    } while (p_next != std::string::npos);
  }
  if (!ssl_data["ssl_ca"].empty())
    ssl_ca = ssl_data["ssl_ca"];
  if (!ssl_data["ssl_cert"].empty())
    ssl_cert = ssl_data["ssl_cert"];
  if (!ssl_data["ssl_key"].empty())
    ssl_key = ssl_data["ssl_key"];

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
    std::string uri_stripped = connstring;
    std::string::size_type i = uri_stripped.find(":" + password);
    if (i != std::string::npos)
      uri_stripped.erase(i, password.length() + 1);

    return uri_stripped;
  }

  // no password to strip, return original one
  return connstring;
}

std::string mysh::strip_ssl_args(const std::string &connstring)
{
  std::string result = connstring;
  std::string::size_type pos;
  if ((pos = result.find("ssl_ca=")) != std::string::npos)
  {
    std::string::size_type pos2 = result.find("&");
    result = result.replace(pos, (pos2 == std::string::npos) ? std::string::npos : pos2 - pos + 1, "");
  }
  if ((pos = result.find("ssl_cert=")) != std::string::npos)
  {
    std::string::size_type pos2 = result.find("&");
    result = result.replace(pos, (pos2 == std::string::npos) ? std::string::npos : pos2 - pos + 1, "");
  }
  if ((pos = result.find("ssl_key=")) != std::string::npos)
  {
    std::string::size_type pos2 = result.find("&");
    result = result.replace(pos, (pos2 == std::string::npos) ? std::string::npos : pos2 - pos + 1, "");
  }
  if (result.at(result.size() - 1) == '?')
  {
    result.resize(result.size() - 1);
  }

  return result;
}

boost::shared_ptr<mysh::ShellBaseSession> mysh::connect_session(const shcore::Argument_list &args, SessionType session_type)
{
  boost::shared_ptr<ShellBaseSession> ret_val;

  switch (session_type)
  {
    case Application:
      ret_val.reset(new mysh::mysqlx::XSession());
      break;
    case Node:
      ret_val.reset(new mysh::mysqlx::NodeSession());
      break;
#ifdef HAVE_LIBMYSQLCLIENT
    case Classic:
      ret_val.reset(new mysql::ClassicSession());
      break;
#endif
    default:
      throw shcore::Exception::argument_error("Invalid session type specified for MySQL connection.");
      break;
  }

  ret_val->connect(args);

  return ret_val;
}

ShellBaseSession::ShellBaseSession()
{
  add_method("createSchema", boost::bind(&ShellBaseSession::createSchema, this, _1), "name", shcore::String, NULL);
  add_method("getDefaultSchema", boost::bind(&ShellBaseSession::get_member_method, this, _1, "getDefaultSchema", "defaultSchema"), NULL);
  add_method("getSchema", boost::bind(&ShellBaseSession::get_schema, this, _1), "name", shcore::String, NULL);
  add_method("getSchemas", boost::bind(&ShellBaseSession::get_member_method, this, _1, "getSchemas", "schemas"), NULL);
  add_method("getUri", boost::bind(&ShellBaseSession::get_member_method, this, _1, "getUri", "uri"), NULL);
}

std::string &ShellBaseSession::append_descr(std::string &s_out, int UNUSED(indent), int UNUSED(quote_strings)) const
{
  if (!is_connected())
    s_out.append("<" + class_name() + ":disconnected>");
  else
    s_out.append("<" + class_name() + ":" + uri() + ">");
  return s_out;
}

std::string &ShellBaseSession::append_repr(std::string &s_out) const
{
  return append_descr(s_out, false);
}

void ShellBaseSession::append_json(const shcore::JSON_dumper& dumper) const
{
  dumper.start_object();

  dumper.append_string("class", class_name());
  dumper.append_bool("connected", is_connected());

  if (is_connected())
    dumper.append_string("uri", uri());

  dumper.end_object();
}

std::vector<std::string> ShellBaseSession::get_members() const
{
  std::vector<std::string> members(Cpp_object_bridge::get_members());
  members.push_back("defaultSchema");
  members.push_back("schemas");
  members.push_back("uri");
  return members;
}

shcore::Value ShellBaseSession::get_member_method(const shcore::Argument_list &args, const std::string& method, const std::string& prop)
{
  std::string function = class_name() + "." + method;
  args.ensure_count(0, function.c_str());

  return get_member(prop);
}

bool ShellBaseSession::operator == (const Object_bridge &other) const
{
  return class_name() == other.class_name() && this == &other;
}

std::string ShellBaseSession::get_quoted_name(const std::string& name)
{
  size_t index = 0;
  std::string quoted_name(name);

  while ((index = quoted_name.find("`", index)) != std::string::npos)
  {
    quoted_name.replace(index, 1, "``");
    index += 2;
  }

  quoted_name = "`" + quoted_name + "`";

  return quoted_name;
}