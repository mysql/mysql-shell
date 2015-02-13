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

#include "mod_mysql.h"

#include "shellcore/obj_date.h"

#include <boost/format.hpp>
#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>
#include "../utils/utils_time.h"

using namespace mysh;


#include <iostream>

#define MAX_COLUMN_LENGTH 1024
#define MIN_COLUMN_LENGTH 4


Mysql_resultset::Mysql_resultset(MYSQL_RES *res, unsigned long raw_duration, my_ulonglong affected_rows, unsigned int warning_count, const char *info, boost::shared_ptr<shcore::Value::Map_type> options)
: _result(res), _raw_duration(raw_duration), _affected_rows(affected_rows), _warning_count(warning_count), _key_by_index(false)
{
  if (options && options->get_bool("key_by_index", false))
    _key_by_index = true;

  add_method("next", boost::bind(&Mysql_resultset::next, this, _1), NULL);
  add_method("__paged_output__", boost::bind(&Mysql_resultset::print, this, _1), NULL);


  // res could be NULL on queries not returning data
  if (res)
  {
    _fields = mysql_fetch_fields(res);
    _num_fields = mysql_num_fields(res);
  }

  if (info)
    _info.assign(info);
}


Mysql_resultset::~Mysql_resultset()
{
  mysql_free_result(_result);
}


std::vector<std::string> Mysql_resultset::get_members() const
{
  std::vector<std::string> members(shcore::Cpp_object_bridge::get_members());
  members.push_back("count");
  return members;
}

bool Mysql_resultset::operator == (const Object_bridge &other) const
{
  return this == &other;
}

shcore::Value Mysql_resultset::get_member(const std::string &prop) const
{
  if (prop == "count")
    return shcore::Value((int64_t)mysql_num_rows(_result));
  return shcore::Cpp_object_bridge::get_member(prop);
}

shcore::Value Mysql_resultset::next(const shcore::Argument_list &args)
{
  args.ensure_count(0, "Mysql_resultset::next");

  MYSQL_ROW row = mysql_fetch_row(_result);
  if (!row)
    return shcore::Value::Null();

  unsigned long *lengths;
  lengths = mysql_fetch_lengths(_result);

  return row_to_doc(row, lengths);
}

shcore::Value Mysql_resultset::print(const shcore::Argument_list &args)
{
	  std::string output;

    if (_result)
    {
      // Prints the informative line at the end
      my_ulonglong rows = mysql_num_rows(_result);

      if (rows)
      {
        // print rows from result, with stats etc
        print_result();

        output = (boost::format("%lld %s in set") % rows % (rows == 1 ? "row" : "rows")).str();
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

    return shcore::Value();
}

void Mysql_resultset::print_result()
{
  print_table();
}

void Mysql_resultset::print_table()
{
  unsigned int index = 0;
  unsigned int field_count = mysql_num_fields(_result);
  std::vector<std::string> formats(field_count, "%-");
  MYSQL_FIELD *fields = mysql_fetch_fields(_result);

  // Calculates the max column widths and constructs the separator line.
  std::string separator("+");
  for (index = 0; index < field_count; index++)
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
  std::cout << separator << "|";
  for (index = 0; index < field_count; index++)
  {
    std::string data = (boost::format(formats[index]) % fields[index].name).str();
    std::cout << data;

    // Once the header is printed, updates the numeric fields formats
    // so they are right aligned
    if (IS_NUM(fields[index].type))
      formats[index] = formats[index].replace(1, 1, "");

  }
  std::cout << "\n" << separator;


  // Now prints the records
  MYSQL_ROW row;
  while ((row = mysql_fetch_row(_result)))
  {
    std::cout << "|";

    {
      for (index = 0; index < field_count; index++)
      {
        std::string data = (boost::format(formats[index]) % (row[index] ? row[index] : "NULL")).str();
        std::cout << data;
      }
      std::cout << "\n";
    }
  }
  std::cout << separator;
}


shcore::Value Mysql_resultset::row_to_doc(const MYSQL_ROW &row, unsigned long *lengths)
{
  boost::shared_ptr<shcore::Value::Map_type> map(new shcore::Value::Map_type);

  for (int i = 0; i < _num_fields; i++)
  {
    std::string key = _key_by_index ? (boost::format("%i") % i).str() : _fields[i].name;

    if (row[i] == NULL)
      map->insert(std::make_pair(key, shcore::Value::Null()));
    else
    {
      switch (_fields[i].type)
      {
        case MYSQL_TYPE_NULL:
          (*map)[key] = shcore::Value::Null();
          break;

        case MYSQL_TYPE_DECIMAL:
        case MYSQL_TYPE_DATE:
        case MYSQL_TYPE_TIME:

        case MYSQL_TYPE_STRING:
        case MYSQL_TYPE_VARCHAR:
        case MYSQL_TYPE_VAR_STRING:
          (*map)[key] = shcore::Value(std::string(row[i], lengths[i]));
          break;

        case MYSQL_TYPE_YEAR:

        case MYSQL_TYPE_TINY:
        case MYSQL_TYPE_SHORT:
        case MYSQL_TYPE_INT24:
        case MYSQL_TYPE_LONG:
        case MYSQL_TYPE_LONGLONG:
          (*map)[key] = shcore::Value(boost::lexical_cast<int64_t>(row[i]));
          break;

        case MYSQL_TYPE_FLOAT:
        case MYSQL_TYPE_DOUBLE:
          (*map)[key] = shcore::Value(boost::lexical_cast<double>(row[i]));
          break;

        case MYSQL_TYPE_DATETIME:
        case MYSQL_TYPE_TIMESTAMP:
        case MYSQL_TYPE_DATETIME2:
        case MYSQL_TYPE_TIMESTAMP2:
          (*map)[key] = shcore::Value(shcore::Date::unrepr(row[i]));
          break;
      }
    }
  }
  return shcore::Value(map);
}


//----------------------------------------------


static bool parse_mysql_connstring(const std::string &connstring,
                                   std::string &user, std::string &password,
                                   std::string &host, int &port, std::string &sock,
                                   std::string &db)
{
  // format is [user[:pass]]@host[:port][/db] or user[:pass]@::socket[/db], like what cmdline utilities use
  std::string s = connstring;
  std::string::size_type p = connstring.find('/');
  if (p != std::string::npos)
  {
    db = connstring.substr(p+1);
    s = connstring.substr(0, p);
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





Mysql_connection::Mysql_connection(const std::string &uri, const std::string &password)
: _mysql(NULL)
{
  add_method("close", boost::bind(&Mysql_connection::close, this, _1), NULL);
  add_method("sql", boost::bind(&Mysql_connection::sql_, this, _1),
             "stmt", shcore::String,
             "*args", shcore::Map,
             NULL);
  add_method("sql_one", boost::bind(&Mysql_connection::sql_one_, this, _1),
             "stmt", shcore::String,
             NULL);

  std::string user;
  std::string pass;
  std::string host;
  int port;
  std::string sock;
  std::string db;
  long flags = 0;

  _mysql = mysql_init(NULL);

  if (!parse_mysql_connstring(uri, user, pass, host, port, sock, db))
    throw shcore::Exception::argument_error("Could not parse URI for MySQL connection");

  if (!password.empty())
    pass = password;

  if (!mysql_real_connect(_mysql, host.c_str(), user.c_str(), pass.c_str(), db.empty() ? NULL : db.c_str(), port, sock.empty() ? NULL : sock.c_str(), flags | CLIENT_MULTI_STATEMENTS))
  {
    throw shcore::Exception::error_with_code_and_state("MySQLError", mysql_error(_mysql), mysql_errno(_mysql), mysql_sqlstate(_mysql));
  }

  //TODO strip password from uri?
  _uri = uri;
}


boost::shared_ptr<shcore::Object_bridge> Mysql_connection::create(const shcore::Argument_list &args)
{
  args.ensure_count(1, 2, "Mysql_connection()");
  return boost::shared_ptr<shcore::Object_bridge>(new Mysql_connection(args.string_at(0),
                                                                       args.size() > 1 ? args.string_at(1) : ""));
}


shcore::Value Mysql_connection::close(const shcore::Argument_list &args)
{
  args.ensure_count(0, "Mysql_connection::close");

  std::cout << "disconnect\n";
  if (_mysql)
    mysql_close(_mysql);
  _mysql = NULL;
  return shcore::Value(shcore::Null);
}


shcore::Value Mysql_connection::sql_(const shcore::Argument_list &args)
{
  std::string query = args.string_at(0);

  args.ensure_count(1, 2, "Mysql_connection::sql");

  return sql(query, shcore::Value());
}


shcore::Value Mysql_connection::sql(const std::string &query, shcore::Value options)
{
  MYSQL_RES *res;

  MySQL_timer timer;
  timer.start();

  if (mysql_real_query(_mysql, query.c_str(), query.length()) != 0)
  {
    throw shcore::Exception::error_with_code_and_state("MySQLError", mysql_error(_mysql), mysql_errno(_mysql), mysql_sqlstate(_mysql));
  }

  res = mysql_store_result(_mysql);

  timer.end();

  Mysql_resultset* result = new Mysql_resultset(res, timer.raw_duration(), mysql_affected_rows(_mysql), mysql_warning_count(_mysql), _mysql->info, options && options.type == shcore::Map ? options.as_map() : boost::shared_ptr<shcore::Value::Map_type>());

  return shcore::Value(boost::shared_ptr<shcore::Object_bridge>(result));
}


shcore::Value Mysql_connection::sql_one_(const shcore::Argument_list &args)
{
  std::string query = args.string_at(0);
  args.ensure_count(1, "Mysql_connection::sql_one");

  return sql_one(query);
}


shcore::Value Mysql_connection::sql_one(const std::string &query)
{
  MYSQL_RES *res;

  if (mysql_real_query(_mysql, query.c_str(), query.length()) != 0)
  {
    throw shcore::Exception::error_with_code_and_state("MySQLError", mysql_error(_mysql), mysql_errno(_mysql), mysql_sqlstate(_mysql));
  }

  res = mysql_store_result(_mysql);
  if (!res)
  {
    throw shcore::Exception::error_with_code_and_state("MySQLError", mysql_error(_mysql), mysql_errno(_mysql), mysql_sqlstate(_mysql));
  }

  //TODO: Should we add time and affected rows??
  Mysql_resultset result(res);
  return result.next(shcore::Argument_list());
}


MYSQL_RES *Mysql_connection::raw_sql(const std::string &query)
{
  if (mysql_real_query(_mysql, query.c_str(), query.length()) != 0)
  {
    throw shcore::Exception::error_with_code_and_state("MySQLError", mysql_error(_mysql), mysql_errno(_mysql), mysql_sqlstate(_mysql));
  }

  return mysql_store_result(_mysql);
}

MYSQL_RES *Mysql_connection::next_result()
{
  MYSQL_RES* next_result = NULL;

  if(mysql_more_results(_mysql))
  {
    // mysql_next_result has the next return values:
    //  0: success and there are more results
    // -1: succeess and this is the last one
    // >1: in case of error
    // So we assign the result on the first two cases
    if (mysql_next_result(_mysql) < 1)
      next_result = mysql_store_result(_mysql);
  }
  
  return next_result;
}

Mysql_connection::~Mysql_connection()
{
  close(shcore::Argument_list());
}

/*
shcore::Value Mysql_connection::stats(const shcore::Argument_list &args)
{
  return shcore::Value();
}
*/

std::string Mysql_connection::class_name() const
{
  return "mysql_connection";
}


std::string &Mysql_connection::append_descr(std::string &s_out, int indent, int quote_strings) const
{
  s_out.append("<mysql_connection:"+_uri+">");
  return s_out;
}


std::string &Mysql_connection::append_repr(std::string &s_out) const
{
  return append_descr(s_out, false);
}


std::vector<std::string> Mysql_connection::get_members() const
{
  std::vector<std::string> members(Cpp_object_bridge::get_members());
  members.push_back("uri");
  return members;
}


bool Mysql_connection::operator == (const Object_bridge &other) const
{
  return this == &other;
}


shcore::Value Mysql_connection::get_member(const std::string &prop) const
{
  if (prop == "uri")
    return shcore::Value(uri());
  return Cpp_object_bridge::get_member(prop);
}


void Mysql_connection::set_member(const std::string &prop, shcore::Value value)
{
}


#include "shellcore/object_factory.h"
namespace {
static struct Auto_register {
  Auto_register()
  {
    shcore::Object_factory::register_factory("mysql", "Connection", &Mysql_connection::create);
  }
} Mysql_connection_register;
};
