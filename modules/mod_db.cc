/*
 * Copyright (c) 2014, Oracle and/or its affiliates. All rights reserved.
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

#include "mod_db.h"

#include "shellcore/object_factory.h"
#include "shellcore/shell_core.h"
#include "shellcore/lang_base.h"
#include "../utils/utils_time.h"

#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/format.hpp>
#include <boost/pointer_cast.hpp>


#define MAX_COLUMN_LENGTH 1024
#define MIN_COLUMN_LENGTH 4

using namespace mysh;
using namespace shcore;


#include <iostream>


Db::Db(Shell_core *shc)
: _shcore(shc)
{
  add_method("sql", boost::bind(&Db::sql, this, _1),
             "stmt", shcore::String,
             "*options", shcore::Map,
             NULL);

  add_method("connect_add", boost::bind(&Db::connect_add, this, _1),
             "uri", shcore::String,
             "*password", shcore::String,
             NULL);
}


Db::~Db()
{
}


Value Db::connect(const Argument_list &args)
{
  args.ensure_count(1, 2, "Db::connect");

  if (!_conns.empty())
  {
    _shcore->print("Closing old connections...\n");
    _conns.clear();
  }

  if (args[0].type == String)
  {
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
    _conns.push_back(boost::shared_ptr<Mysql_connection>(new Mysql_connection(uri, pwd)));
    _shcore->print("Connection object assigned to variable db\n");
  }
  else if (args[0].type == Map)
  {
    boost::shared_ptr<Value::Map_type> object(args.map_at(0));

    std::string name = object->get_string("name", "");
    boost::shared_ptr<Value::Array_type> servers(object->get_array("servers"));
    _shcore->print("Connecting to server group "+name+"...\n");
    for (Value::Array_type::const_iterator iter = servers->begin(); iter != servers->end(); ++iter)
    {
      _shcore->print(iter->descr(false)+"...\n");
      _conns.push_back(boost::shared_ptr<Mysql_connection>(new Mysql_connection(iter->as_string())));
    }
  }
  return Value::Null();
}


Value Db::connect_add(const Argument_list &args)
{
  args.ensure_count(1, "Db::connect_add");

  std::string uri = args.string_at(0);
  _shcore->print("Connecting to "+uri+"...\n");
  _conns.push_back(boost::shared_ptr<Mysql_connection>(new Mysql_connection(uri)));
  _shcore->print("Connection object assigned to variable db\n");

  return Value::Null();
}


Value Db::sql(const Argument_list &args)
{
  if (_conns.empty())
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
    for (std::vector<boost::shared_ptr<Mysql_connection> >::iterator c = _conns.begin();
         c != _conns.end(); ++c)
    {
      try
      {
        if (_conns.size() > 1)
          _shcore->print((*c)->uri()+":\n");

        MySQL_timer timer;
        timer.start();

        MYSQL_RES *result = (*c)->raw_sql(statement);

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
            my_ulonglong rows = (*c)->affected_rows();

            // TODO: Verify this should print Query OK... or Query Error.
            // Checks for a -1 value, which would indicate an error.
            // Even so the old client printed Query OK on this case for some reason.
            if (rows == ~(my_ulonglong) 0)
              output = "Query OK";
            else
              // In case of Query OK, prints the actual number of affected rows.
              output = (boost::format("Query OK, %lld %s affected") % rows % (rows == 1 ? "row" : "rows")).str();
          }

          unsigned int warnings = (*c)->warning_count();
          if (warnings)
            output.append((boost::format(", %d warning%s") % warnings % (warnings == 1 ? "" : "s")).str());

          output.append(" ");
          output.append((boost::format("(%s)") % timer.format_legacy(true)).str());
          output.append("\n\n");

          _shcore->print(output);

          const char* info = (*c)->get_info();
          if (info)
          {
            std::string data(info);
            data.append("\n\n");
            _shcore->print(data);
          }

          if (warnings)
            internal_sql(*c, "show warnings",boost::bind(&Db::print_warnings, this, _1, 0));

        } while((result = (*c)->next_result()));

        if (_conns.size() > 1)
          _shcore->print("\n");
      }
      catch (shcore::Exception &exc)
      {
        print_exception(exc);
        internal_sql(*c, "show_warnings",boost::bind(&Db::print_warnings, this, _1, (*exc.error())["code"]));
      }
    }
  }
  
  return Value::Null();
}

void Db::internal_sql(boost::shared_ptr<Mysql_connection> conn, const std::string& sql, boost::function<void (MYSQL_RES* data)> result_handler)
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

void Db::print_warnings(MYSQL_RES* data, unsigned long main_error_code)
{
  MYSQL_ROW row = NULL;
  while ((row = mysql_fetch_row(data)))
  {
    unsigned long error = boost::lexical_cast<unsigned long>(row[1]);
    if (error != main_error_code)
      _shcore->print((boost::format("%s (Code %s): %s\n") % row[0] % row[1] % row[2]).str());
  }
}


void Db::print_result(MYSQL_RES *res)
{
  // At this point it means there were no errors.

  print_table(res);
}

void Db::print_exception(const shcore::Exception &e)
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


void Db::print_table(MYSQL_RES *res)
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

void Db::print_json(MYSQL_RES *res)
{
  
}
void Db::print_vertical(MYSQL_RES *res)
{
  
}

std::string Db::class_name() const
{
  return "Db";
}


std::string &Db::append_descr(std::string &s_out, int indent, int quote_strings) const
{
  s_out.append("<Db>");
  return s_out;
}


std::string &Db::append_repr(std::string &s_out) const
{
  return append_descr(s_out, false);
}


std::vector<std::string> Db::get_members() const
{
  std::vector<std::string> members(Cpp_object_bridge::get_members());
  members.push_back("conns");
  return members;
}


bool Db::operator == (const Object_bridge &other) const
{
  return this == &other;
}


Value Db::get_member(const std::string &prop) const
{
  if (prop == "conns")
  {
    Value list(Value::new_array());
    for (std::vector<boost::shared_ptr<Mysql_connection> >::const_iterator c = _conns.begin();
         c != _conns.end(); ++c)

      list.as_array()->push_back(Value(boost::static_pointer_cast<Object_bridge, Mysql_connection >(*c)));

    return list;
  }

  if (_conns.empty())
  {
    _shcore->print("Not connected. Use connect(<uri>) or \\connect <uri>\n");
    return Value::Null();
  }

  return Cpp_object_bridge::get_member(prop);
}


void Db::set_member(const std::string &prop, Value value)
{
}
