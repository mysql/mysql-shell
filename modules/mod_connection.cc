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
#include "shellcore/obj_date.h"

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
                                     std::string &db, int &pwd_found)
  {
    // format is [protocol://][user[:pass]]@host[:port][/db] or user[:pass]@::socket[/db], like what cmdline utilities use
    pwd_found = 0;
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
      pwd_found = 1;
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

  std::string strip_password(const std::string &connstring)
  {
    std::string remaining = connstring;
    std::string password;

    std::string::size_type p;
    p = remaining.find("://");
    if (p != std::string::npos)
    {
      remaining = remaining.substr(p+3);
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
      password = user_part.substr(p+1);
      if (!password.empty())
      {
        std::string uri_stripped = connstring;
        std::string::size_type i = uri_stripped.find(":" + password);
        if (i != std::string::npos)
          uri_stripped.erase(i, password.length()+1);

        return uri_stripped;
      }
    }

    // no password to strip, return original one
    return connstring;
  }
}

//----------------------------- Base_connection ----------------------------------------
Base_connection::Base_connection(const std::string &uri, const char *password)
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
  int pwd_found;

  if (!parse_mysql_connstring(uri, protocol, user, _pwd, host, port, sock, db, pwd_found))
    throw shcore::Exception::argument_error("Could not parse URI for MySQL connection");

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

Field::Field(const std::string& catalog, const std::string& db, const std::string& table, const std::string& otable, const std::string& name, const std::string& oname, int length, int type, int flags, int decimals, int charset):
_catalog(catalog),
_db(db),
_table(table),
_org_table(otable),
_name(name),
_org_name(oname),
_length(length),
_type(type),
_flags(flags),
_decimals(decimals),
_charset(charset),
_max_length(0),
_name_length(name.length())
{
}

//----------------------------- Base_resultset ----------------------------------------

Base_resultset::Base_resultset(boost::shared_ptr<Base_connection> owner, uint64_t affected_rows, int warning_count, const char* info, boost::shared_ptr<shcore::Value::Map_type> options)
: _key_by_index(false), _has_resultset(false), _fetched_row_count(0), _affected_rows(affected_rows), _warning_count(warning_count), _owner(owner)
{
  if (options && options->get_bool("key_by_index", false))
    _key_by_index = true;

  // Info may be NULL so validation is needed
  if (info)
    _info.assign(info);

  add_method("next_result", boost::bind(&Base_resultset::next_result, this, _1), NULL);
  add_method("fetch_one", boost::bind(&Base_resultset::next, this, _1), NULL);
  add_method("fetch_all", boost::bind(&Base_resultset::fetch_all, this, _1), NULL);
  add_method("fetch_metadata", boost::bind(&Base_resultset::get_metadata, this, _1), NULL);
  add_method("__paged_output__", boost::bind(&Base_resultset::print, this, _1), NULL);
}


shcore::Value Base_resultset::next(const shcore::Argument_list &args)
{
  std::string function = class_name() + "::next";
  bool raw = false;

  args.ensure_count(0, 1, function.c_str());

  if (args.size() == 1)
    raw = args.bool_at(0);

  std::auto_ptr<Base_row> row(next_row());

  // Returns either the row as data_array, document or NULL
  return row.get() ? raw ? row->as_data_array() : row->as_document() : shcore::Value::Null();
}

shcore::Value Base_resultset::next_result(const shcore::Argument_list &args)
{
  return shcore::Value(next_result());
}

bool Base_resultset::next_result()
{
  boost::shared_ptr<Base_connection> owner = _owner.lock();
  if (owner)
    return owner->next_result(this);
  else
    return false;
}


shcore::Value Base_resultset::get_metadata(const shcore::Argument_list &args)
{
  std::string function = class_name() + "::get_metadata";

  args.ensure_count(0,function.c_str());

  boost::shared_ptr<shcore::Value::Array_type> array(new shcore::Value::Array_type);
  int num_fields = _metadata.size();

  for (int i = 0; i < num_fields; i++)
  {
    boost::shared_ptr<shcore::Value::Map_type> map(new shcore::Value::Map_type);

    (*map)["catalog"] = shcore::Value(_metadata[i].catalog());
    (*map)["db"] = shcore::Value(_metadata[i].db());
    (*map)["table"] = shcore::Value(_metadata[i].table());
    (*map)["org_table"] = shcore::Value(_metadata[i].org_table());
    (*map)["name"] = shcore::Value(_metadata[i].name());
    (*map)["org_name"] = shcore::Value(_metadata[i].org_name());
    (*map)["charset"] = shcore::Value(int(_metadata[i].charset()));
    (*map)["length"] = shcore::Value(int(_metadata[i].length()));
    (*map)["type"] = shcore::Value(int(_metadata[i].type()));
    (*map)["flags"] = shcore::Value(int(_metadata[i].flags()));
    (*map)["decimal"] = shcore::Value(int(_metadata[i].decimals()));

    array->push_back(shcore::Value(map));
  }

  return shcore::Value(array);
}


shcore::Value Base_resultset::fetch_all(const shcore::Argument_list &args)
{
  std::string function = class_name() + "::fetch_all";

  args.ensure_count(0, 1, function.c_str());

  bool raw = false;

  if (args.size() == 1)
    raw = args.bool_at(0);

  Base_row* row = NULL;

  boost::shared_ptr<shcore::Value::Array_type> array(new shcore::Value::Array_type);

  while((row = next_row()))
  {
    array->push_back(raw ? row->as_data_array() : row->as_document());
    delete row;
  }

  return shcore::Value(array);
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

shcore::Value Base_row::as_data_array()
{
  boost::shared_ptr<shcore::Value::Array_type> array(new shcore::Value::Array_type);

  int num_fields = _fields.size();

  for (int i = 0; i < num_fields; i++)
  {
    array->push_back(get_value(i));
  }
  return shcore::Value(array);
}


std::vector<std::string> Base_resultset::get_members() const
{
  std::vector<std::string> members(shcore::Cpp_object_bridge::get_members());
  members.push_back("fetched_row_count");
  members.push_back("affected_rows");
  members.push_back("warning_count");
  members.push_back("info");
  members.push_back("execution_time");
  return members;
}

bool Base_resultset::operator == (const Object_bridge &other) const
{
  return this == &other;
}

shcore::Value Base_resultset::get_member(const std::string &prop) const
{
  if (prop == "fetched_row_count")
  {
    return shcore::Value((int64_t)_fetched_row_count);
  }
  if (prop == "affected_rows")
  {
    return shcore::Value((int64_t)((_affected_rows == ~(my_ulonglong)0) ? 0 : _affected_rows));
  }
  if (prop == "warning_count")
  {
    return shcore::Value(_warning_count);
  }
  if (prop == "info")
  {
    return shcore::Value(_info);
  }
  if (prop == "execution_time")
  {
    return shcore::Value(MySQL_timer::format_legacy(_raw_duration, true));
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
      shcore::Argument_list args;
      args.push_back(shcore::Value::True());
      shcore::Value records = fetch_all(args);
      shcore::Value::Array_type_ref array_records = records.as_array();

      // Gets the first row to determine there is data to print
      // Base_row * row = next_row();

      if (array_records->size())
      {
        // print rows from result, with stats etc
        print_result(array_records);

        output = (boost::format("%lld %s in set") % _fetched_row_count % (_fetched_row_count == 1 ? "row" : "rows")).str();
      }
      else
        output = "Empty set";
    }
    else
    {
      // Some queries return -1 since affected rows do not apply to them
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

    shcore::print(output);

    if (!_info.empty())
    {
      shcore::print(_info + "\n\n");
    }

    // Prints the warnings if there were any
    if (_warning_count)
      print_warnings();

  } while (next_result());

  return shcore::Value();
}

void Base_resultset::print_result(shcore::Value::Array_type_ref records)
{
  print_table(records);
}

void Base_resultset::print_table(shcore::Value::Array_type_ref records)
{
  //---------
  // Calculates the real field lengths
  size_t row_index;
  for(row_index = 0; row_index < records->size(); row_index++)
  {
    shcore::Value::Array_type_ref record = (*records)[row_index].as_array();

    for(int field_index = 0; field_index < _metadata.size(); field_index++)
    {

      int field_length = (*record)[field_index].repr().length();
      if (field_length > _metadata[field_index].max_length())
        _metadata[field_index].max_length(field_length);
    }
  }
  //-----------

  unsigned int index = 0;
  unsigned int field_count = _metadata.size();
  std::vector<std::string> formats(field_count, "%-");

  // Calculates the max column widths and constructs the separator line.
  std::string separator("+");
  for (index = 0; index < field_count; index++)
  {
    unsigned int max_field_length=0;
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
  shcore::print(separator + "|");
  for (index = 0; index < field_count; index++)
  {
    std::string data = (boost::format(formats[index]) % _metadata[index].name()).str();
    shcore::print(data);

    // Once the header is printed, updates the numeric fields formats
    // so they are right aligned
    if (IS_NUM(_metadata[index].type()))
      formats[index] = formats[index].replace(1, 1, "");

  }

  shcore::print("\n" + separator);

  // Now prints the records
  for(row_index = 0; row_index < records->size(); row_index++)
  {
    shcore::print("|");

    shcore::Value::Array_type_ref record = (*records)[row_index].as_array();

    for(size_t field_index = 0; field_index < _metadata.size(); field_index++)
    {
      std::string raw_value = (*record)[field_index].repr();
      std::string data = (boost::format(formats[field_index]) % (raw_value)).str();

      shcore::print(data);
    }
    shcore::print("\n");
  }

  shcore::print(separator);
}

void Base_resultset::print_warnings()
{

  Value::Map_type_ref options(new shcore::Value::Map_type);
  (*options)["key_by_index"] = Value::True();

  boost::shared_ptr<Base_connection> conn = _owner.lock();

  if (conn)
  {
    Value result_wrapper = conn->sql("show warnings", shcore::Value(options));

    if (result_wrapper)
    {
      boost::shared_ptr<mysh::Base_resultset> result = result_wrapper.as_object<mysh::Base_resultset>();

      if (result)
      {
        Value record;

        while ((record = result->next(Argument_list())))
        {
          boost::shared_ptr<Value::Map_type> row = record.as_map();


          unsigned long error = ((*row)["1"].as_int());

          std::string type = (*row)["0"].as_string();
          std::string msg = (*row)["2"].as_string();
          shcore::print((boost::format("%s (Code %ld): %s\n") % type % error % msg).str());
        }
      }
    }
  }
}
