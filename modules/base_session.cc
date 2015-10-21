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
#include "shellcore/server_registry.h"

#include "shellcore/proxy_object.h"

#include "utils/utils_general.h"
#include "utils/utils_file.h"

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

void ShellBaseSession::append_json(shcore::JSON_dumper& dumper) const
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