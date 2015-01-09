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

#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/format.hpp>
#include <ctime>

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
  for (std::vector<boost::shared_ptr<Mysql_connection> >::iterator c = _conns.begin();
       c != _conns.end(); ++c)
  {
    if (_conns.size() > 1)
      _shcore->print((*c)->uri(Argument_list()).descr(false)+":\n");

    std::clock_t start = std::clock();

    MYSQL_RES *result = (*c)->raw_sql(args.string_at(0));
    
    double duration = ( std::clock() - start ) / (double) CLOCKS_PER_SEC;
    
    do
    {
      if (result)
      {
        // Prints the informative line at the end
        my_ulonglong rows = mysql_num_rows(result);
        
        if (rows)
        {
          // print rows from result, with stats etc
          print_result(result, duration);
          
          _shcore->print(boost::str(boost::format("%lld %s in set (%s)\n") % rows % (rows == 1 ? "row" : "rows") % format_duration(duration)));
        }
        else
          _shcore->print("Empty set");
      
        mysql_free_result(result);
      }
      else
      {
        my_ulonglong rows = (*c)->affected_rows();
        
        if (rows)
          _shcore->print(boost::str(boost::format("Query OK, %lld %s affected\n") % rows % (rows == 1 ? "row" : "rows")));
        else
          _shcore->print("Query OK\n");
      }
      
    } while((result = (*c)->next_result()));

    if (_conns.size() > 1)
      _shcore->print("\n");
  }
  
  return Value::Null();
}


void Db::print_result(MYSQL_RES *res, double duration)
{
  // At this point it means there were no errors.

  print_table(res);
}

std::string Db::format_duration(double duration)
{
  double temp;
  std::string str_duration;
  
  double minute_seconds = 60.0;
  double hour_seconds = 3600.0;
  double day_seconds = hour_seconds * 24;
  
  if (duration >= day_seconds)
  {
    temp = floor(duration/day_seconds);
    duration -= temp * day_seconds;
    
    str_duration.append(boost::str(boost::format("%d %s") % temp % (temp == 1 ? "day" : "days")));
  }
  
  if (duration >= hour_seconds)
  {
    temp = floor(duration/hour_seconds);
    duration -= temp * hour_seconds;
    
    if (!str_duration.empty())
      str_duration.append(", ");
    
    str_duration.append(boost::str(boost::format("%d %s") % temp % (temp == 1 ? "hour" : "hours")));
  }
  
  if (duration >= minute_seconds)
  {
    temp = floor(duration/minute_seconds);
    duration -= temp * minute_seconds;
    
    if (!str_duration.empty())
      str_duration.append(", ");
    
    str_duration.append(boost::str(boost::format("%d %s") % temp % (temp == 1 ? "minute" : "minute")));
  }
  
  if (duration)
    str_duration.append(boost::str(boost::format("%.2f sec") % duration));
  
  return str_duration;
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
    std::string data = boost::str(boost::format(formats[index]) % fields[index].name);
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
        std::string data = boost::str(boost::format(formats[index]) % (row[index] ? row[index] : "NULL"));
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


std::string &Db::append_descr(std::string &s_out, int indent, bool quote_strings) const
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
  members.push_back("connections");
  return members;
}


bool Db::operator == (const Object_bridge &other) const
{
  return this == &other;
}


Value Db::get_member(const std::string &prop) const
{
  if (prop == "connections")
  {
    Value list(Value::new_array());
    for (std::vector<boost::shared_ptr<Mysql_connection> >::const_iterator c = _conns.begin();
         c != _conns.end(); ++c)
      list.as_array()->push_back(Value(*c));
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
