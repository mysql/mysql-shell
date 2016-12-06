/*
 * Copyright (c) 2014, 2016, Oracle and/or its affiliates. All rights reserved.
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

#include "mysql_connection.h"
#include "base_session.h"
#include "utils/utils_general.h"

#include "shellcore/obj_date.h"

#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>

using namespace mysqlsh::mysql;

#include "shellcore/object_factory.h"
#include "shellcore/common.h"
#include <stdlib.h>

#define MAX_COLUMN_LENGTH 1024
#define MIN_COLUMN_LENGTH 4

Result::Result(std::shared_ptr<Connection> owner, my_ulonglong affected_rows_, unsigned int warning_count_, uint64_t last_insert_id, const char *info_)
  : _connection(owner), _affected_rows(affected_rows_), _last_insert_id(last_insert_id), _warning_count(warning_count_), _fetched_row_count(0), _execution_time(0), _has_resultset(false) {
  if (info_)
    _info.assign(info_);
}

int Result::fetch_metadata() {
  int num_fields = 0;

  _metadata.clear();

  // res could be NULL on queries not returning data
  std::shared_ptr<MYSQL_RES> res = _result.lock();

  if (res) {
    num_fields = mysql_num_fields(res.get());
    MYSQL_FIELD *fields = mysql_fetch_fields(res.get());

    for (int index = 0; index < num_fields; index++) {
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

Result::~Result() {}

std::unique_ptr<Row> Result::fetch_one() {
  std::unique_ptr<Row> ret_val = nullptr;

  if (has_resultset()) {
    // Loads the first row
    std::shared_ptr<MYSQL_RES> res = _result.lock();

    if (res) {
      MYSQL_ROW mysql_row = mysql_fetch_row(res.get());
      if (mysql_row) {
        unsigned long *lengths;
        lengths = mysql_fetch_lengths(res.get());

        ret_val = std::unique_ptr<Row>(new Row(mysql_row, lengths, &_metadata));

        // Each read row increases the count
        _fetched_row_count++;
      }
    }
  }

  return ret_val;
}

bool Result::next_data_set() {
  return _connection->next_data_set(this);
}

std::unique_ptr<Result> Result::query_warnings() {
  return _connection->run_sql("show warnings");
}

void Result::reset(std::shared_ptr<MYSQL_RES> res, unsigned long duration) {
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
_name_length(name_.length()) {}

Row::Row(MYSQL_ROW row, unsigned long *lengths, std::vector<Field>* metadata) :
_row(row), _lengths(lengths), _metadata(metadata) {}

shcore::Value Row::get_value(int index) {
  if (_row[index] == NULL)
    return shcore::Value::Null();
  else {
    switch ((*_metadata)[index].type()) {
      case MYSQL_TYPE_NULL:
        return shcore::Value::Null();
      case MYSQL_TYPE_DECIMAL:
      case MYSQL_TYPE_DATE:
      case MYSQL_TYPE_NEWDATE:
      case MYSQL_TYPE_TIME:
#if MYSQL_MAJOR_VERSION > 5 || (MYSQL_MAJOR_VERSION == 5 && MYSQL_MINOR_VERSION >= 7)
      case MYSQL_TYPE_TIME2:
#endif
      case MYSQL_TYPE_STRING:
      case MYSQL_TYPE_VARCHAR:
      case MYSQL_TYPE_VAR_STRING:
      case MYSQL_TYPE_NEWDECIMAL:
      case MYSQL_TYPE_TINY_BLOB:
      case MYSQL_TYPE_MEDIUM_BLOB:
      case MYSQL_TYPE_LONG_BLOB:
      case MYSQL_TYPE_BLOB:
      case MYSQL_TYPE_GEOMETRY:
      case MYSQL_TYPE_JSON:

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
#if MYSQL_MAJOR_VERSION > 5 || (MYSQL_MAJOR_VERSION == 5 && MYSQL_MINOR_VERSION >= 7)
      case MYSQL_TYPE_DATETIME2:
      case MYSQL_TYPE_TIMESTAMP2:
#endif
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

std::string Row::get_value_as_string(int index) {
  return _row[index] ? _row[index] : "NULL";
}

//----------------------------------------------
void Connection::throw_on_connection_fail() {
  std::string local_error(mysql_error(_mysql));
  auto local_errno = mysql_errno(_mysql);
  std::string local_sqlstate = mysql_sqlstate(_mysql);
  close();
  throw shcore::Exception::mysql_error_with_code_and_state(local_error, local_errno, local_sqlstate.c_str());
}

Connection::Connection(const std::string &uri_, const char *password)
  : _mysql(NULL) {
  std::string protocol;
  std::string user;
  std::string pass;
  std::string host;
  int port = 3306;
  std::string sock;
  std::string db;
  std::string ssl_ca;
  std::string ssl_cert;
  std::string ssl_key;
  long flags = CLIENT_MULTI_RESULTS;
  int pwd_found;

  _mysql = mysql_init(NULL);

  shcore::parse_mysql_connstring(uri_, protocol, user, pass, host, port, sock, db, pwd_found, ssl_ca, ssl_cert, ssl_key);

  if (password)
    pass.assign(password);

  _uri = shcore::strip_password(uri_);

  setup_ssl(ssl_ca, ssl_cert, ssl_key);
  unsigned int tcp = MYSQL_PROTOCOL_TCP;
  mysql_options(_mysql, MYSQL_OPT_PROTOCOL, &tcp);
  if (!mysql_real_connect(_mysql, host.c_str(), user.c_str(), pass.c_str(), db.empty() ? NULL : db.c_str(), port, sock.empty() ? NULL : sock.c_str(), flags)) {
    throw_on_connection_fail();
  }
}

Connection::Connection(const std::string &host, int port, const std::string &socket, const std::string &user, const std::string &password, const std::string &schema,
  const std::string &ssl_ca, const std::string &ssl_cert, const std::string &ssl_key)
: _mysql(NULL) {
  long flags = CLIENT_MULTI_RESULTS;

  _mysql = mysql_init(NULL);

  std::stringstream str;
  str << user << "@" << host << ":" << port;
  _uri = str.str();

  if (!setup_ssl(ssl_ca, ssl_cert, ssl_key)) {
    unsigned int ssl_mode = SSL_MODE_DISABLED;
    mysql_options(_mysql, MYSQL_OPT_SSL_MODE, &ssl_mode);
  } else {
    unsigned int ssl_mode = SSL_MODE_REQUIRED;
    mysql_options(_mysql, MYSQL_OPT_SSL_MODE, &ssl_mode);
  }

  unsigned int tcp = MYSQL_PROTOCOL_TCP;
  mysql_options(_mysql, MYSQL_OPT_PROTOCOL, &tcp);
  if (!mysql_real_connect(_mysql, host.c_str(), user.c_str(), password.c_str(), schema.empty() ? NULL : schema.c_str(), port, socket.empty() ? NULL : socket.c_str(), flags)) {
    throw_on_connection_fail();
  }
}

bool Connection::setup_ssl(const std::string &ssl_ca, const std::string &ssl_cert, const std::string &ssl_key) {
  if (ssl_ca.empty() && ssl_cert.empty() && ssl_key.empty())
    return false;

  std::string my_ssl_ca_path;
  std::string my_ssl_ca(ssl_ca);

  shcore::normalize_sslca_args(my_ssl_ca, my_ssl_ca_path);

  mysql_ssl_set(_mysql, ssl_key.c_str(), ssl_cert.c_str(), my_ssl_ca.c_str(), my_ssl_ca_path.c_str(), NULL);
  return true;
}

void Connection::close() {
  // This should be logged, for now commenting to
  // avoid having unneeded output on the script mode
  // shcore::print("disconnect\n");
  if (_mysql)
    mysql_close(_mysql);
  _mysql = NULL;
}

std::unique_ptr<Result> Connection::run_sql(const std::string &query) {
  if (_prev_result) {
    _prev_result.reset();

    while (mysql_next_result(_mysql) == 0) {
      MYSQL_RES *trailing_result = mysql_use_result(_mysql);
      mysql_free_result(trailing_result);
    }
  }

  _timer.start();

  if (mysql_real_query(_mysql, query.c_str(), query.length()) != 0) {
    throw shcore::Exception::mysql_error_with_code_and_state(mysql_error(_mysql), mysql_errno(_mysql), mysql_sqlstate(_mysql));
  }

  auto result = std::unique_ptr<Result>(new Result(shared_from_this(),
      mysql_affected_rows(_mysql), mysql_warning_count(_mysql), mysql_insert_id(_mysql), mysql_info(_mysql)));

  next_data_set(result.get(), true);

  return result;
}

template <class T>
static void free_result(T* result) {
  mysql_free_result(result);
  result = NULL;
}

bool Connection::next_data_set(Result *target, bool first_result) {
  bool ret_val = false;

  // Skips fetching a record on the first result
  int more_results = 0;

  if (!first_result) {
    _prev_result.reset();
    more_results = mysql_next_result(_mysql);
  }

  Result *real_target = dynamic_cast<Result *> (target);

  // If there are more results
  if (more_results == 0) {
    // Retrieves the next result
    MYSQL_RES* result = mysql_use_result(_mysql);

    if (result)
      _prev_result = std::shared_ptr<MYSQL_RES>(result, &free_result<MYSQL_RES>);

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

Connection::~Connection() {
  _prev_result.reset();
  close();
}
