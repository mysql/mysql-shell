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

using namespace mysh;


#include <iostream>

#define MAX_COLUMN_LENGTH 1024
#define MIN_COLUMN_LENGTH 4


Mysql_resultset::Mysql_resultset(boost::shared_ptr<Mysql_connection> owner, boost::shared_ptr<shcore::Value::Map_type> options)
: Base_resultset(options), _owner(owner)
{
}

void Mysql_resultset::fetch_metadata()
{
  // res could be NULL on queries not returning data
  boost::shared_ptr<MYSQL_RES> res = _result.lock();
  // if metadata is not already loaded
  if (res && _metadata.empty())
  {
    int num_fields = mysql_num_fields(res.get());
    MYSQL_FIELD *fields = mysql_fetch_fields(res.get());

    for (int index = 0; index < num_fields; index++)
    {
      _metadata.push_back(Field(fields[index].name, fields[index].max_length, fields[index].name_length, fields[index].type));
    }
  }
}


Mysql_resultset::~Mysql_resultset()
{
}

Base_row *Mysql_resultset::next_row()
{
  Base_row *ret_val = NULL;

  if (has_resultset())
  {
    // Loads the metadata
    fetch_metadata();

    // Loads the first row
    boost::shared_ptr<MYSQL_RES> res = _result.lock();

    if (res)
    {
      MYSQL_ROW mysql_row = mysql_fetch_row(res.get());
      if (mysql_row)
      {
        unsigned long *lengths;
        lengths = mysql_fetch_lengths(res.get());

        ret_val = new Mysql_row(_metadata, _key_by_index, mysql_row, lengths);

        // Each read row increases the count
        _fetched_row_count++;
      }
    }
  }
  return ret_val;
}

bool Mysql_resultset::next_result()
{
  boost::shared_ptr<Mysql_connection> owner = _owner.lock();
  if (owner)
    return _owner.lock()->next_result(this);
  else
    return false;
}

void Mysql_resultset::reset(boost::shared_ptr<MYSQL_RES> res, unsigned long duration, uint64_t affected_rows, int warning_count, const char *info)
{
  _result = res;
  _raw_duration = duration;
  _affected_rows = affected_rows;
  _warning_count = warning_count;
  _info.empty();
  if (info)
    _info.assign(info);

  _has_resultset = (res != NULL);
}

Mysql_row::Mysql_row(std::vector<Field>& metadata, bool key_by_index, MYSQL_ROW row, unsigned long *lengths) : Base_row(metadata, key_by_index), _row(row), _lengths(lengths)
{
}

shcore::Value Mysql_row::get_value(int index)
{
  if (_row[index] == NULL)
    return shcore::Value::Null();
  else
  {
    switch (_fields[index].type())
    {
      case MYSQL_TYPE_NULL:
        return shcore::Value::Null();
      case MYSQL_TYPE_DECIMAL:
      case MYSQL_TYPE_DATE:
      case MYSQL_TYPE_TIME:
      case MYSQL_TYPE_STRING:
      case MYSQL_TYPE_VARCHAR:
      case MYSQL_TYPE_VAR_STRING:
        return shcore::Value(std::string(_row[index], _lengths[index]));

      case MYSQL_TYPE_YEAR:
      case MYSQL_TYPE_TINY:
      case MYSQL_TYPE_SHORT:
      case MYSQL_TYPE_INT24:
      case MYSQL_TYPE_LONG:
      case MYSQL_TYPE_LONGLONG:
        return shcore::Value(boost::lexical_cast<int64_t>(_row[index]));

      case MYSQL_TYPE_FLOAT:
      case MYSQL_TYPE_DOUBLE:
        return shcore::Value(boost::lexical_cast<double>(_row[index]));
        break;

      case MYSQL_TYPE_DATETIME:
      case MYSQL_TYPE_TIMESTAMP:
      case MYSQL_TYPE_DATETIME2:
      case MYSQL_TYPE_TIMESTAMP2:
        return shcore::Value(shcore::Date::unrepr(_row[index]));
        break;
    }
  }

  return shcore::Value();
}

std::string Mysql_row::get_value_as_string(int index)
{
  return _row[index] ? _row[index] : "NULL";
}


//----------------------------------------------


Mysql_connection::Mysql_connection(const std::string &uri, const std::string &password)
: Base_connection(uri, password), _mysql(NULL)
{
  std::string protocol;
  std::string user;
  std::string pass;
  std::string host;
  int port = 3306;
  std::string sock;
  std::string db;
  long flags = 0;

  _mysql = mysql_init(NULL);

  if (!parse_mysql_connstring(uri, protocol, user, pass, host, port, sock, db))
    throw shcore::Exception::argument_error("Could not parse URI for MySQL connection");

  if (!password.empty())
    pass = password;

  if (!mysql_real_connect(_mysql, host.c_str(), user.c_str(), pass.c_str(), db.empty() ? NULL : db.c_str(), port, sock.empty() ? NULL : sock.c_str(), flags | CLIENT_MULTI_STATEMENTS))
  {
    throw shcore::Exception::error_with_code_and_state("MySQLError", mysql_error(_mysql), mysql_errno(_mysql), mysql_sqlstate(_mysql));
  }

  //TODO strip password from uri?
  //_uri = uri;
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




shcore::Value Mysql_connection::sql(const std::string &query, shcore::Value options)
{
  if (_prev_result)
    _prev_result.reset();

  _timer.start();

  if (mysql_real_query(_mysql, query.c_str(), query.length()) != 0)
  {
    throw shcore::Exception::error_with_code_and_state("MySQLError", mysql_error(_mysql), mysql_errno(_mysql), mysql_sqlstate(_mysql));
  }


  Mysql_resultset* result = new Mysql_resultset(shared_from_this(), options && options.type == shcore::Map ? options.as_map() : boost::shared_ptr<shcore::Value::Map_type>());

  if (next_result(result, true))
    return shcore::Value(boost::shared_ptr<shcore::Object_bridge>(result));
  else
    return shcore::Value::Null();
}

template <class T>
static void free_result(T* result)
{
  mysql_free_result(result);
  result = NULL;
}

shcore::Value Mysql_connection::sql_one(const std::string &query)
{
  if (_prev_result)
    _prev_result.reset();

  _timer.start();

  if (mysql_real_query(_mysql, query.c_str(), query.length()) != 0)
  {
    throw shcore::Exception::error_with_code_and_state("MySQLError", mysql_error(_mysql), mysql_errno(_mysql), mysql_sqlstate(_mysql));
  }

  Mysql_resultset result(shared_from_this());

  if(next_result(&result, true))
    return result.next(shcore::Argument_list());
  else
    // TODO: Trow exception since nothing was read?
    return shcore::Value::Null();
}

bool Mysql_connection::next_result(Base_resultset *target, bool first_result)
{
  bool ret_val = false;

  // Skips fetching a record on the first result
  int more_results = 0;

  if (!first_result)
    more_results = mysql_next_result(_mysql);

  Mysql_resultset *real_target = dynamic_cast<Mysql_resultset *> (target);

  // If there are more results
  if (more_results == 0)
  {
    // Retrieves the next result
    _prev_result = boost::shared_ptr<MYSQL_RES>(mysql_store_result(_mysql), &free_result<MYSQL_RES>);

    _timer.end();

    // We need to update the received result object with the information 
    // for the next result set
    real_target->reset(_prev_result, _timer.raw_duration(), mysql_affected_rows(_mysql), mysql_warning_count(_mysql), mysql_info(_mysql));

    ret_val = true;
  }
  else
    real_target->reset(boost::shared_ptr<MYSQL_RES>(), 0, 0, 0, NULL);

  return ret_val;
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



#include "shellcore/object_factory.h"
namespace {
static struct Auto_register {
  Auto_register()
  {
    shcore::Object_factory::register_factory("mysql", "Connection", &Mysql_connection::create);
  }
} Mysql_connection_register;
};
