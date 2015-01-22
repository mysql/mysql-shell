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

#include "mod_session.h"

#include "shellcore/object_factory.h"
#include "shellcore/shell_core.h"
#include "shellcore/lang_base.h"
#include "../utils/utils_time.h"

#include "shellcore/proxy_object.h"

#include "mod_db.h"

#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/format.hpp>
#include <boost/pointer_cast.hpp>


#define MAX_COLUMN_LENGTH 1024
#define MIN_COLUMN_LENGTH 4

using namespace mysh;
using namespace shcore;


#include <iostream>


Session::Session(Shell_core *shc)
: _shcore(shc), _show_warnings(false)
{
  _schema_proxy.reset(new Proxy_object(boost::bind(&Session::get_db, this, _1)));

  add_method("sql", boost::bind(&Session::sql, this, _1),
             "stmt", shcore::String,
             "*options", shcore::Map,
             NULL);
}


Session::~Session()
{
}


Value Session::connect(const Argument_list &args)
{
  args.ensure_count(1, 2, "Session::connect");

  if (_conn)
  {
    _shcore->print("Closing old connection...\n");
    _conn.reset();
  }

  std::string uri = args.string_at(0);
  std::string pwd;
  if (args.size() == 1)
  {
    if (!_shcore->password("Enter password: ", pwd))
    {
      _shcore->print("Cancelled");
      return Value::Null();
    }
  }
  else
    pwd = args.string_at(1);

  _shcore->print("Connecting to "+uri+"...\n");
  _conn.reset(new Mysql_connection(uri, pwd));

  return Value::Null();
}


Value Session::sql(const Argument_list &args)
{
  if (!_conn)
  {
    _shcore->print("Not connected\n");
    return Value::Null();
  }

  // Validates for empty statements.
  std::string statement = args.string_at(0);
  if (statement.empty())
    _shcore->print_error("No query specified\n");
  else
  {
    try
    {
      MySQL_timer timer;
      timer.start();

      MYSQL_RES *result = _conn->raw_sql(statement);

      timer.end();

      do
      {
        std::string output;

        if (result)
        {
          // Prints the informative line at the end
          my_ulonglong rows = mysql_num_rows(result);

          if (rows)
          {
            // print rows from result, with stats etc
            print_result(result);

            output = (boost::format("%lld %s in set") % rows % (rows == 1 ? "row" : "rows")).str();
          }
          else
            output = "Empty set";

          mysql_free_result(result);
        }
        else
        {
          my_ulonglong rows = _conn->affected_rows();

          // TODO: Verify this should print Query OK... or Query Error.
          // Checks for a -1 value, which would indicate an error.
          // Even so the old client printed Query OK on this case for some reason.
          if (rows == ~(my_ulonglong) 0)
            output = "Query OK";
          else
            // In case of Query OK, prints the actual number of affected rows.
            output = (boost::format("Query OK, %lld %s affected") % rows % (rows == 1 ? "row" : "rows")).str();
        }

        unsigned int warnings = _conn->warning_count();
        if (warnings)
          output.append((boost::format(", %d warning%s") % warnings % (warnings == 1 ? "" : "s")).str());

        output.append(" ");
        output.append((boost::format("(%s)") % timer.format_legacy(true)).str());
        output.append("\n\n");

        _shcore->print(output);

        const char* info = _conn->get_info();
        if (info)
        {
          std::string data(info);
          data.append("\n\n");
          _shcore->print(data);
        }

        if (warnings && _show_warnings)
          internal_sql(_conn, "show warnings",boost::bind(&Session::print_warnings, this, _1, 0));

      } while((result = _conn->next_result()));
    }
    catch (shcore::Exception &exc)
    {
      print_exception(exc);

      if (_show_warnings)
        internal_sql(_conn, "show warnings",boost::bind(&Session::print_warnings, this, _1, (*exc.error())["code"]));
    }
  }
  
  return Value::Null();
}

void Session::internal_sql(boost::shared_ptr<Mysql_connection> conn, const std::string& sql, boost::function<void (MYSQL_RES* data)> result_handler)
{
  try
  {
    {
      MYSQL_RES *result = conn->raw_sql(sql);
      
      result_handler(result);
    }
  }
  catch (shcore::Exception &exc)
  {
    print_exception(exc);
  }
}

void Session::print_warnings(MYSQL_RES* data, unsigned long main_error_code)
{
  MYSQL_ROW row = NULL;
  while ((row = mysql_fetch_row(data)))
  {
    unsigned long error = boost::lexical_cast<unsigned long>(row[1]);
    if (error != main_error_code)
      _shcore->print((boost::format("%s (Code %s): %s\n") % row[0] % row[1] % row[2]).str());
  }
}


void Session::print_result(MYSQL_RES *res)
{
  // At this point it means there were no errors.

  print_table(res);
}

void Session::print_exception(const shcore::Exception &e)
{
  std::string message = (*e.error())["type"].as_string();
  if ((*e.error()).has_key("code"))
  {
    message.append(" ");
    message.append(((*e.error())["code"].repr()));
                   
    if ((*e.error()).has_key("state") && (*e.error())["state"])
      message.append((boost::format(" (%s)") % ((*e.error())["state"].as_string())).str());
  }
  
  message.append(": ");
  message.append(e.what());
  
  _shcore->print_error(message);
}


void Session::print_table(MYSQL_RES *res)
{
  unsigned int index = 0;
  unsigned int field_count = mysql_num_fields(res);
  std::vector<std::string> formats(field_count, "%-");
  MYSQL_FIELD *fields = mysql_fetch_fields(res);
  
  // Calculates the max column widths and constructs the separator line.
  std::string separator("+");
  for(index = 0; index < field_count; index++)
  {
    unsigned int max_field_length;
    max_field_length = std::max<unsigned int>(fields[index].max_length, fields[index].name_length);
    max_field_length = std::max<unsigned int>(max_field_length, MIN_COLUMN_LENGTH);
    fields[index].max_length = max_field_length;
    
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
  _shcore->print(separator);
  _shcore->print("|");
  for(index = 0; index < field_count; index++)
  {
    std::string data = (boost::format(formats[index]) % fields[index].name).str();
    _shcore->print(data);
    
    // Once the header is printed, updates the numeric fields formats
    // so they are right aligned
    if (IS_NUM(fields[index].type))
      formats[index] = formats[index].replace(1, 1, "");

  }
  _shcore->print("\n");
  _shcore->print(separator);
  
  
  // Now prints the records
  MYSQL_ROW row;
  while((row = mysql_fetch_row(res)))
  {
    _shcore->print("|");
    
    {
      for(index = 0; index < field_count; index++)
      {
        std::string data = (boost::format(formats[index]) % (row[index] ? row[index] : "NULL")).str();
        _shcore->print(data);
      }
      _shcore->print("\n");
    }
  }
  _shcore->print(separator);
}

void Session::print_json(MYSQL_RES *res)
{
  
}
void Session::print_vertical(MYSQL_RES *res)
{
  
}

std::string Session::class_name() const
{
  return "Session";
}


std::string &Session::append_descr(std::string &s_out, int indent, int quote_strings) const
{
  if (!_conn)
    s_out.append("<Session:disconnected>");
  else
    s_out.append("<Session:"+_conn->uri()+">");
  return s_out;
}


std::string &Session::append_repr(std::string &s_out) const
{
  return append_descr(s_out, false);
}


std::vector<std::string> Session::get_members() const
{
  std::vector<std::string> members(Cpp_object_bridge::get_members());
  members.push_back("connection");
  members.push_back("conn");
  members.push_back("dbs");
  members.push_back("schemas");
  return members;
}


Value Session::get_member(const std::string &prop) const
{
  if (prop == "conn" || prop == "connection")
  {
    if (!_conn)
      return Value::Null();
    else
      return Value(boost::static_pointer_cast<Object_bridge>(_conn));
  }
  else if (prop == "dbs" || prop == "schemas")
  {
    return Value(boost::static_pointer_cast<Object_bridge>(_schema_proxy));
  }

  if (!_conn)
  {
    _shcore->print("Not connected. Use connect(<uri>) or \\connect <uri>\n");
    return Value::Null();
  }

  return Cpp_object_bridge::get_member(prop);
}


void Session::set_member(const std::string &prop, Value value)
{
  Cpp_object_bridge::set_member(prop, value);
}


bool Session::operator == (const Object_bridge &other) const
{
  return class_name() == other.class_name() && this == &other;
}


Value Session::get_db(const std::string &schema)
{
  if (_conn)
  {
    boost::shared_ptr<Db> db(new Db(shared_from_this(), schema));
    db->cache_table_names();
    return Value(boost::static_pointer_cast<Object_bridge>(db));
  }
  throw Exception::runtime_error("Session not connected");
}


boost::shared_ptr<Db> Session::default_schema()
{
  if (_conn)
  {
    Value res = _conn->sql_one("select schema()");
    if (res && res.type == Map && res.as_map()->get_type("schema()") == String)
    {
      boost::shared_ptr<Db> db(new Db(shared_from_this(), res.as_map()->get_string("schema()")));
      db->cache_table_names();
      return db;
    }
  }
  return boost::shared_ptr<Db>();
}

