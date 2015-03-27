/*
 * Copyright (c) 2014, 2015, Oracle and/or its affiliates. All rights reserved.
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

#include "shellcore/object_factory.h"
#include "shellcore/shell_core.h"
#include "shellcore/lang_base.h"

#include "shellcore/proxy_object.h"

#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/format.hpp>
#include <boost/pointer_cast.hpp>

#if WIN32
#  include <winsock2.h>
#endif

#include <mysql.h>
#include "mod_connection.h"


#define MAX_COLUMN_LENGTH 1024
#define MIN_COLUMN_LENGTH 4

using namespace mysh;
using namespace shcore;


#include <iostream>

namespace mysh {

  bool parse_mysql_connstring(const std::string &connstring,
                                     std::string &protocol, std::string &user, std::string &password,
                                     std::string &host, int &port, std::string &sock,
                                     std::string &db)
  {
    // format is [protocol://][user[:pass]]@host[:port][/db] or user[:pass]@::socket[/db], like what cmdline utilities use
    std::string remaining = connstring;

    std::string::size_type p;
    p = remaining.find("://");
    if (p != std::string::npos)
    {
      protocol = connstring.substr(0,p);
      remaining = remaining.substr(p+3);
    }

    std::string s = remaining;
    p = remaining.find('/');
    if (p != std::string::npos)
    {
      db = remaining.substr(p+1);
      s = remaining.substr(0, p);
    }
    p = s.rfind('@');
    std::string user_part;
    std::string server_part = (p == std::string::npos) ? s : s.substr(p+1);

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
      password = user_part.substr(p+1);
    }
    else
      user = user_part;
    
    p = server_part.find(':');
    if (p != std::string::npos)
    {
      host = server_part.substr(0, p);
      server_part = server_part.substr(p+1);
      p = server_part.find(':');
      if (p != std::string::npos)
        sock = server_part.substr(p+1);
      else
        if (!sscanf(server_part.substr(0, p).c_str(), "%i", &port))
          return false;
    }
    else
      host = server_part;
    return true;
  }
}

//----------------------------- Base_connection ----------------------------------------
Base_connection::Base_connection(const std::string &uri, const std::string &password)
{
  add_method("close", boost::bind(&Base_connection::close, this, _1), NULL);
  add_method("sql", boost::bind(&Base_connection::sql_, this, _1),
             "stmt", shcore::String,
             "*args", shcore::Map,
             NULL);
  add_method("sql_one", boost::bind(&Base_connection::sql_one_, this, _1),
             "stmt", shcore::String,
             NULL);

  std::string protocol;
  std::string user;
  std::string pass;
  std::string host;
  int port = 0;
  std::string sock;
  std::string db;

  if (!parse_mysql_connstring(uri, protocol, user, _pwd, host, port, sock, db))
    throw shcore::Exception::argument_error("Could not parse URI for MySQL connection");

  if (!password.empty())
    _pwd = password;

  _uri = uri;
}


std::string &Base_connection::append_descr(std::string &s_out, int indent, int quote_strings) const
{
  s_out.append("<" + class_name() + ":" + _uri + ">");
  return s_out;
}


std::string &Base_connection::append_repr(std::string &s_out) const
{
  return append_descr(s_out, false);
}


std::vector<std::string> Base_connection::get_members() const
{
  std::vector<std::string> members(Cpp_object_bridge::get_members());
  members.push_back("uri");
  return members;
}


shcore::Value Base_connection::get_member(const std::string &prop) const
{
  if (prop == "uri")
    return shcore::Value(_uri);
  return Cpp_object_bridge::get_member(prop);
}


bool Base_connection::operator == (const Object_bridge &other) const
{
  return this == &other;
}

shcore::Value Base_connection::sql_(const shcore::Argument_list &args)
{
  std::string function = class_name() + "::sql";

  args.ensure_count(1, 2, function.c_str());

  std::string query = args.string_at(0);

  return sql(query, shcore::Value());
}


shcore::Value Base_connection::sql_one_(const shcore::Argument_list &args)
{
  std::string function = class_name() + "::sql_one";

  args.ensure_count(1, function.c_str());

  std::string query = args.string_at(0);

  return sql_one(query);
}

//----------------------------- Base_resultset ----------------------------------------

Base_resultset::Base_resultset(boost::shared_ptr<shcore::Value::Map_type> options)
: _key_by_index(false), _has_resultset(false), _fetched_row_count(0)
{
  if (options && options->get_bool("key_by_index", false))
    _key_by_index = true;

  add_method("next_result", boost::bind(&Base_resultset::next_result, this, _1), NULL);
  add_method("next", boost::bind(&Base_resultset::next, this, _1), NULL);
  add_method("__paged_output__", boost::bind(&Base_resultset::print, this, _1), NULL);
}


shcore::Value Base_resultset::next(const shcore::Argument_list &args)
{
  std::auto_ptr<Base_row> row(next_row());

  // Returns either the row as document or NULL
  return row.get() ? row->as_document() : shcore::Value::Null();
}

shcore::Value Base_resultset::next_result(const shcore::Argument_list &args)
{
  return shcore::Value(next_result());
}

shcore::Value Base_row::as_document()
{
  boost::shared_ptr<shcore::Value::Map_type> map(new shcore::Value::Map_type);

  int num_fields = _fields.size();

  for (int i = 0; i < num_fields; i++)
  {
    std::string key = _key_by_index ? (boost::format("%i") % i).str() : _fields[i].name();

    (*map)[key] = get_value(i);
  }
  return shcore::Value(map);
}

std::vector<std::string> Base_resultset::get_members() const
{
  std::vector<std::string> members(shcore::Cpp_object_bridge::get_members());
  members.push_back("count");
  return members;
}

bool Base_resultset::operator == (const Object_bridge &other) const
{
  return this == &other;
}

shcore::Value Base_resultset::get_member(const std::string &prop) const
{
  // TODO: this property maybe is not appropiate since the count is not available...
  if (prop == "count")
  {
    return shcore::Value((int64_t) 0);
  }

  return shcore::Cpp_object_bridge::get_member(prop);
}

shcore::Value Base_resultset::print(const shcore::Argument_list &args)
{
  std::string output;

  do
  {
    if (has_resultset())
    {
      // Gets the first row to determine there is data to print
      Base_row * row = next_row();
      if (row)
      {
        // print rows from result, with stats etc
        print_result(row);

        output = (boost::format("%lld %s in set") % _fetched_row_count % (_fetched_row_count == 1 ? "row" : "rows")).str();
      }
      else
        output = "Empty set";
    }
    else
    {
      // TODO: Verify this should print Query OK... or Query Error.
      // Checks for a -1 value, which would indicate an error.
      // Even so the old client printed Query OK on this case for some reason.
      if (_affected_rows == ~(my_ulonglong)0)
        output = "Query OK";
      else
        // In case of Query OK, prints the actual number of affected rows.
        output = (boost::format("Query OK, %lld %s affected") % _affected_rows % (_affected_rows == 1 ? "row" : "rows")).str();
    }

    if (_warning_count)
      output.append((boost::format(", %d warning%s") % _warning_count % (_warning_count == 1 ? "" : "s")).str());

    output.append(" ");
    output.append((boost::format("(%s)") % MySQL_timer::format_legacy(_raw_duration, true)).str());
    output.append("\n\n");

    std::cout << output;

    if (!_info.empty())
    {
      std::cout << _info << "\n\n";
    }
  } while (next_result());

  return shcore::Value();
}

void Base_resultset::print_result(Base_row * first_row)
{
  print_table(first_row);
}

void Base_resultset::print_table(Base_row * first_row)
{
  unsigned int index = 0;
  unsigned int field_count = _metadata.size();
  std::vector<std::string> formats(field_count, "%-");

  // Calculates the max column widths and constructs the separator line.
  std::string separator("+");
  for (index = 0; index < field_count; index++)
  {
    unsigned int max_field_length;
    max_field_length = std::max<unsigned int>(_metadata[index].max_length(), _metadata[index].name_length());
    max_field_length = std::max<unsigned int>(max_field_length, MIN_COLUMN_LENGTH);
    _metadata[index].max_length(max_field_length);

    // Creates the format string to print each field
    formats[index].append(boost::lexical_cast<std::string>(max_field_length));
    formats[index].append("s|");

    std::string field_separator(max_field_length, '-');
    field_separator.append("+");
    separator.append(field_separator);
  }
  separator.append("\n");


  // Prints the initial separator line and the column headers
  // TODO: Consider the charset information on the length calculations
  std::cout << separator << "|";
  for (index = 0; index < field_count; index++)
  {
    std::string data = (boost::format(formats[index]) % _metadata[index].name()).str();
    std::cout << data;

    // Once the header is printed, updates the numeric fields formats
    // so they are right aligned
    if (IS_NUM(_metadata[index].type()))
      formats[index] = formats[index].replace(1, 1, "");

  }
  std::cout << "\n" << separator;


  // Now prints the records
  do
  {
    std::cout << "|";

    {
      for (index = 0; index < field_count; index++)
      {
        std::string data = (boost::format(formats[index]) % (first_row->get_value_as_string(index))).str();
        std::cout << data;
      }
      std::cout << "\n";
    }
  }while ((first_row = next_row()));

  std::cout << separator;
}