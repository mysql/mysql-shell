/*
 * Copyright (c) 2014, 1015, Oracle and/or its affiliates. All rights reserved.
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

#include "mysql.h"
#include "base_session.h"

#include "shellcore/obj_date.h"

#include <boost/format.hpp>
#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>

using namespace mysh::mysql;

#include "shellcore/object_factory.h"
#include "shellcore/common.h"

#define MAX_COLUMN_LENGTH 1024
#define MIN_COLUMN_LENGTH 4

Result::Result(boost::shared_ptr<Connection> owner, my_ulonglong affected_rows_, unsigned int UNUSED(warning_count_), const char *info_)
: _connection(owner), _affected_rows(affected_rows_), _last_insert_id(0), _warning_count(0), _fetched_row_count(0), _execution_time(0), _has_resultset(false)
{
  if (info_)
    _info.assign(info_);
}

int Result::fetch_metadata()
{
  int num_fields = 0;

  _metadata.clear();

  // res could be NULL on queries not returning data
  boost::shared_ptr<MYSQL_RES> res = _result.lock();

  if (res)
  {
    num_fields = mysql_num_fields(res.get());
    MYSQL_FIELD *fields = mysql_fetch_fields(res.get());

    for (int index = 0; index < num_fields; index++)
    {
      _metadata.push_back(Field(fields[index].catalog,
        fields[index].db,
        fields[index].table,
        fields[index].org_table,
        fields[index].name,
        fields[index].org_name,
        fields[index].length,
        fields[index].type,
        fields[index].flags,
        fields[index].decimals,
        fields[index].charsetnr));
    }
  }

  return num_fields;
}

Result::~Result()
{
}

Row * Result::next()
{
  Row *ret_val = NULL;

  if (has_resultset())
  {
    // Loads the first row
    boost::shared_ptr<MYSQL_RES> res = _result.lock();

    if (res)
    {
      MYSQL_ROW mysql_row = mysql_fetch_row(res.get());
      if (mysql_row)
      {
        unsigned long *lengths;
        lengths = mysql_fetch_lengths(res.get());

        ret_val = new Row(mysql_row, lengths, &_metadata);

        // Each read row increases the count
        _fetched_row_count++;
      }
    }
  }

  return ret_val;
}

bool Result::next_result()
{
  return _connection->next_result(this);
}

Result *Result::query_warnings()
{
  return _connection->executeSql("show warnings");
}

void Result::reset(boost::shared_ptr<MYSQL_RES> res, unsigned long duration)
{
  _has_resultset = false;
  if (res)
    _has_resultset = true;

  _result = res;
  _execution_time = duration;
}

Field::Field(const std::string& catalog_, const std::string& db_, const std::string& table_, const std::string& otable, const std::string& name_, const std::string& oname, int length_, int type_, int flags_, int decimals_, int charset_) :
_catalog(catalog_),
_db(db_),
_table(table_),
_org_table(otable),
_name(name_),
_org_name(oname),
_length(length_),
_type(type_),
_flags(flags_),
_decimals(decimals_),
_charset(charset_),
_max_length(0),
_name_length(name_.length())
{
}

Row::Row(MYSQL_ROW row, unsigned long *lengths, std::vector<Field>* metadata) :
_row(row), _lengths(lengths), _metadata(metadata)
{
}

shcore::Value Row::get_value(int index)
{
  if (_row[index] == NULL)
    return shcore::Value::Null();
  else
  {
    switch ((*_metadata)[index].type())
    {
      case MYSQL_TYPE_NULL:
        return shcore::Value::Null();
      case MYSQL_TYPE_DECIMAL:
      case MYSQL_TYPE_DATE:
      case MYSQL_TYPE_NEWDATE:
      case MYSQL_TYPE_TIME:
      case MYSQL_TYPE_TIME2:
      case MYSQL_TYPE_STRING:
      case MYSQL_TYPE_VARCHAR:
      case MYSQL_TYPE_VAR_STRING:
      case MYSQL_TYPE_NEWDECIMAL:
      case MYSQL_TYPE_TINY_BLOB:
      case MYSQL_TYPE_MEDIUM_BLOB:
      case MYSQL_TYPE_LONG_BLOB:
      case MYSQL_TYPE_BLOB:
      case MYSQL_TYPE_GEOMETRY:

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

      case MYSQL_TYPE_BIT:
      case MYSQL_TYPE_ENUM:
      case MYSQL_TYPE_SET:
        // TODO: Read these types properly
        break;
    }
  }

  return shcore::Value();
}

std::string Row::get_value_as_string(int index)
{
  return _row[index] ? _row[index] : "NULL";
}

//----------------------------------------------

Connection::Connection(const std::string &uri_, const char *password)
: _mysql(NULL)
{
  std::string protocol;
  std::string user;
  std::string pass;
  std::string host;
  int port = 3306;
  std::string sock;
  std::string db;
  long flags = CLIENT_MULTI_RESULTS;
  int pwd_found;

  _mysql = mysql_init(NULL);

  if (!parse_mysql_connstring(uri_, protocol, user, pass, host, port, sock, db, pwd_found))
    throw shcore::Exception::argument_error("Could not parse URI for MySQL connection");

  if (password)
    pass.assign(password);

  _uri = strip_password(uri_);

  unsigned int tcp = MYSQL_PROTOCOL_TCP;
  mysql_options(_mysql, MYSQL_OPT_PROTOCOL, &tcp);
  if (!mysql_real_connect(_mysql, host.c_str(), user.c_str(), pass.c_str(), db.empty() ? NULL : db.c_str(), port, sock.empty() ? NULL : sock.c_str(), flags))
  {
    throw shcore::Exception::error_with_code_and_state("MySQLError", mysql_error(_mysql), mysql_errno(_mysql), mysql_sqlstate(_mysql));
  }
}

Connection::Connection(const std::string &host, int port, const std::string &socket, const std::string &user, const std::string &password, const std::string &schema)
: _mysql(NULL)
{
  long flags = CLIENT_MULTI_RESULTS;

  _mysql = mysql_init(NULL);

  std::stringstream str;
  str << user << "@" << host << ":" << port;
  _uri = str.str();

  unsigned int tcp = MYSQL_PROTOCOL_TCP;
  mysql_options(_mysql, MYSQL_OPT_PROTOCOL, &tcp);
  if (!mysql_real_connect(_mysql, host.c_str(), user.c_str(), password.c_str(), schema.empty() ? NULL : schema.c_str(), port, socket.empty() ? NULL : socket.c_str(), flags))
  {
    throw shcore::Exception::error_with_code_and_state("MySQLError", mysql_error(_mysql), mysql_errno(_mysql), mysql_sqlstate(_mysql));
  }
}

void Connection::close()
{
  // This should be logged, for now commenting to
  // avoid having unneeded output on the script mode
  // shcore::print("disconnect\n");
  if (_mysql)
    mysql_close(_mysql);
  _mysql = NULL;
}

Result *Connection::executeSql(const std::string &query)
{
  if (_prev_result)
  {
    _prev_result.reset();

    while (mysql_next_result(_mysql) == 0)
    {
      MYSQL_RES *trailing_result = mysql_use_result(_mysql);
      mysql_free_result(trailing_result);
    }
  }

  _timer.start();

  if (mysql_real_query(_mysql, query.c_str(), query.length()) != 0)
  {
    throw shcore::Exception::error_with_code_and_state("MySQLError", mysql_error(_mysql), mysql_errno(_mysql), mysql_sqlstate(_mysql));
  }

  Result* result = new Result(shared_from_this(), mysql_affected_rows(_mysql), mysql_warning_count(_mysql), mysql_info(_mysql));

  next_result(result, true);

  return result;
}

template <class T>
static void free_result(T* result)
{
  mysql_free_result(result);
  result = NULL;
}

bool Connection::next_result(Result *target, bool first_result)
{
  bool ret_val = false;

  // Skips fetching a record on the first result
  int more_results = 0;

  if (!first_result)
  {
    _prev_result.reset();
    more_results = mysql_next_result(_mysql);
  }

  Result *real_target = dynamic_cast<Result *> (target);

  // If there are more results
  if (more_results == 0)
  {
    // Retrieves the next result
    MYSQL_RES* result = mysql_use_result(_mysql);

    if (result)
      _prev_result = boost::shared_ptr<MYSQL_RES>(result, &free_result<MYSQL_RES>);

    // Only returns true when this method was called and there were
    // Additional results
    ret_val = true;
  }

  _timer.end();

  // We need to update the received result object with the information
  // for the next result set
  real_target->reset(_prev_result, _timer.raw_duration());

  if (_prev_result)
    real_target->fetch_metadata();

  return ret_val;
}

Connection::~Connection()
{
  close();
}