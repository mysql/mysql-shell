/*
 * Copyright (c) 2017, Oracle and/or its affiliates. All rights reserved.
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

#include "mysqlshdk/libs/db/mysql/session.h"
#include <sstream>
#include <vector>

#include "utils/utils_general.h"

namespace mysqlshdk {
namespace db {
namespace mysql {
//-------------------------- Session Implementation ----------------------------
void Session_impl::throw_on_connection_fail() {
  auto exception =
      shcore::database_error(mysql_error(_mysql), mysql_errno(_mysql),
                             mysql_error(_mysql), mysql_sqlstate(_mysql));
  close();
  throw exception;
}

Session_impl::Session_impl() : _mysql(NULL), _tx_deep(0) {}

void Session_impl::connect(
    const mysqlshdk::db::Connection_options &connection_options) {
  long flags = CLIENT_MULTI_RESULTS | CLIENT_CAN_HANDLE_EXPIRED_PASSWORDS;
  int port = 0;
  std::string host;
  std::string user;

  if (connection_options.has_port())
    port = connection_options.get_port();
  else if (!connection_options.has_socket())
    port = 3306;

  host = connection_options.has_host() ? connection_options.get_host()
                                       : "localhost";

  user = connection_options.has_user() ? connection_options.get_user()
                                       : shcore::get_system_user();

  _mysql = mysql_init(NULL);

  setup_ssl(connection_options.get_ssl_options());
  if (port != 0) {
    unsigned int tcp = MYSQL_PROTOCOL_TCP;
    mysql_options(_mysql, MYSQL_OPT_PROTOCOL, &tcp);
  }

#ifdef _WIN32
  // Enable pipe connection if required
  if (!connection_options.has_port() &&
      ((connection_options.has_host() &&
        connection_options.get_host() == ".") ||
       connection_options.has_pipe())) {
    unsigned int pipe = MYSQL_PROTOCOL_PIPE;
    mysql_options(_mysql, MYSQL_OPT_PROTOCOL, &pipe);
  }
#endif

  if (!mysql_real_connect(_mysql, host.c_str(), user.c_str(),
                          connection_options.has_password()
                              ? connection_options.get_password().c_str()
                              : "",
                          connection_options.has_schema()
                              ? connection_options.get_schema().c_str()
                              : NULL,
                          port,
                          connection_options.has_socket()
                              ? connection_options.get_socket().c_str()
                              : NULL,
                          flags)) {
    throw_on_connection_fail();
  }
}

bool Session_impl::setup_ssl(
    const mysqlshdk::db::Ssl_options &ssl_options) const {
  int value;

  if (ssl_options.has_ca())
    mysql_options(_mysql, MYSQL_OPT_SSL_CA, (ssl_options.get_ca().c_str()));

  if (ssl_options.has_capath())
    mysql_options(_mysql, MYSQL_OPT_SSL_CAPATH,
                  (ssl_options.get_capath().c_str()));

  if (ssl_options.has_crl())
    mysql_options(_mysql, MYSQL_OPT_SSL_CRL, (ssl_options.get_crl().c_str()));

  if (ssl_options.has_crlpath())
    mysql_options(_mysql, MYSQL_OPT_SSL_CRLPATH,
                  (ssl_options.get_crlpath().c_str()));

  if (ssl_options.has_ciphers())
    mysql_options(_mysql, MYSQL_OPT_SSL_CIPHER,
                  (ssl_options.get_ciphers().c_str()));

  if (ssl_options.has_tls_version())
    mysql_options(_mysql, MYSQL_OPT_TLS_VERSION,
                  (ssl_options.get_tls_version().c_str()));

  if (ssl_options.has_cert())
    mysql_options(_mysql, MYSQL_OPT_SSL_CERT, (ssl_options.get_cert().c_str()));

  if (ssl_options.has_key())
    mysql_options(_mysql, MYSQL_OPT_SSL_KEY, (ssl_options.get_key().c_str()));

  if (ssl_options.has_mode())
    value = ssl_options.get_mode();
  else
    value = static_cast<int>(mysqlshdk::db::Ssl_mode::Preferred);

  mysql_options(_mysql, MYSQL_OPT_SSL_MODE, &value);

  return true;
}

void Session_impl::close() {
  // This should be logged, for now commenting to
  // avoid having unneeded output on the script mode
  if (_prev_result)
    _prev_result.reset();

  if (_mysql)
    mysql_close(_mysql);
  _mysql = nullptr;
}

std::shared_ptr<IResult> Session_impl::query(const std::string &sql,
                                             bool buffered) {
  return run_sql(sql, buffered);
}

void Session_impl::execute(const std::string &sql) {
  auto result = run_sql(sql, true);
}

std::shared_ptr<IResult> Session_impl::run_sql(const std::string &query,
                                               bool buffered) {
  if (_mysql == nullptr)
    throw std::logic_error("Not connected");
  if (_prev_result) {
    _prev_result.reset();
  } else {
    MYSQL_RES *unread_result = mysql_use_result(_mysql);
    mysql_free_result(unread_result);
  }

  // Discards any pending result
  while (mysql_next_result(_mysql) == 0) {
    MYSQL_RES *trailing_result = mysql_use_result(_mysql);
    mysql_free_result(trailing_result);
  }

  _timer.start();

  if (mysql_real_query(_mysql, query.c_str(), query.length()) != 0) {
    _timer.end();
    throw shcore::database_error(mysql_error(_mysql), mysql_errno(_mysql),
                                 mysql_error(_mysql), mysql_sqlstate(_mysql));
  }

  std::shared_ptr<Result> result(
      new Result(shared_from_this(), mysql_affected_rows(_mysql),
                 mysql_warning_count(_mysql), mysql_insert_id(_mysql),
                 mysql_info(_mysql)));

  _timer.end();

  prepare_fetch(result.get(), buffered);

  return std::static_pointer_cast<IResult>(result);
}

template <class T>
static void free_result(T *result) {
  mysql_free_result(result);
  result = NULL;
}

bool Session_impl::next_data_set() {
  if (_prev_result)
    _prev_result.reset();

  return mysql_next_result(_mysql) == 0;
}

void Session_impl::prepare_fetch(Result *target, bool buffered) {
  Result *real_target = dynamic_cast<Result *>(target);

  // Sets the execution time performing the last query
  unsigned long execution_time = _timer.raw_duration();
  _timer.start();

  MYSQL_RES *result;

  if (buffered)
    result = mysql_store_result(_mysql);
  else
    result = mysql_use_result(_mysql);

  if (result)
    _prev_result = std::shared_ptr<MYSQL_RES>(result, &free_result<MYSQL_RES>);

  if (_prev_result) {
    // We need to update the received result object with the information
    // for the next result set
    real_target->reset(_prev_result);

    real_target->fetch_metadata();
  }
  _timer.end();

  // Adds the execution time loading the result data
  execution_time += _timer.raw_duration();

  real_target->_fetch_started = true;
  real_target->_execution_time = execution_time;
}

void Session_impl::start_transaction() {
  if (_tx_deep == 0)
    execute("start transaction");

  _tx_deep++;
}

void Session_impl::commit() {
  _tx_deep--;

  assert(_tx_deep >= 0);

  if (_tx_deep == 0)
    execute("commit");
}

void Session_impl::rollback() {
  _tx_deep--;

  assert(_tx_deep >= 0);

  if (_tx_deep == 0)
    execute("rollback");
}

Session_impl::~Session_impl() {
  _prev_result.reset();
  close();
}

//----------------------------- Session Recorder ------------------------------

#ifdef MOCK_RECORDING
Mock_record *Mock_record::_instance = NULL;

void Session_recorder::connect(const std::string &uri, const char *password) {
  if (password)
    Mock_record::get() << "EXPECT_CALL(session, connect(\"" << uri << "\", \""
                       << password << "\"));" << std::endl;
  else
    Mock_record::get() << "EXPECT_CALL(session, connect(\"" << uri << "\"));"
                       << std::endl;

  Session::connect(uri, password);
}

Session_recorder::Session_recorder() : Session() {
  /*const char *outfile = std::getenv("MOCK_RECORDING_FILE");
  if (outfile) {
  std::cerr << "Enabled mock recording at: " << outfile << std::endl;
  _file.open(outfile, std::ofstream::trunc | std::ofstream::out);
  }*/
}

void Session_recorder::connect(const std::string &host, int port,
                               const std::string &socket,
                               const std::string &user,
                               const std::string &password,
                               const std::string &schema,
                               const mysqlshdk::db::Ssl_options &ssl_options) {
  Mock_record::get() << "mysqlshdk::db::Ssl_options ssl_options;" << std::endl;

  if (!ssl_options.ca.is_null())
    Mock_record::get() << "ssl_options.ca = " << *ssl_options.ca << ";"
                       << std::endl;

  if (!ssl_options.capath.is_null())
    Mock_record::get() << "ssl_options.capath = " << *ssl_options.capath << ";"
                       << std::endl;

  if (!ssl_options.cert.is_null())
    Mock_record::get() << "ssl_options.cert = " << *ssl_options.cert << ";"
                       << std::endl;

  if (!ssl_options.ciphers.is_null())
    Mock_record::get() << "ssl_options.ciphers = " << *ssl_options.ciphers
                       << ";" << std::endl;

  if (!ssl_options.crl.is_null())
    Mock_record::get() << "ssl_options.crl = " << *ssl_options.crl << ";"
                       << std::endl;

  if (!ssl_options.crlpath.is_null())
    Mock_record::get() << "ssl_options.crlpath = " << *ssl_options.crlpath
                       << ";" << std::endl;

  if (!ssl_options.key.is_null())
    Mock_record::get() << "ssl_options.key = " << *ssl_options.key << ";"
                       << std::endl;

  if (!ssl_options.tls_version.is_null())
    Mock_record::get() << "ssl_options.tls_version = "
                       << *ssl_options.tls_version << ";" << std::endl;

  Mock_record::get() << "ssl_options.mode = " << ssl_options.mode << ";"
                     << std::endl;
  Mock_record::get() << "ssl_options.skip = " << ssl_options.skip << ";"
                     << std::endl;

  Mock_record::get() << "EXPECT_CALL(session, connect(\"" << host << "\", "
                     << port << ", \"" << socket << "\", \"" << user << "\", \""
                     << password << "\", " << schema << ", ssl_options));"
                     << std::endl;

  Session::connect(host, port, socket, user, password, schema, ssl_options);
}

std::shared_ptr<IResult> Session_recorder::query(const std::string &sql,
                                                 bool buffered) {
  // While mock recording, all is buffered
  auto result = Session::query(sql, true);

  Result_recorder *ret_val = new Result_recorder(result);

  Mock_record::get() << "EXPECT_CALL(session, query(\"" << sql << "\", "
                     << buffered << ").WillOnce(Return(&result));" << std::endl;

  return std::shared_ptr<IResult>(ret_val);
}

void Session_recorder::execute(const std::string &sql) {
  Mock_record::get() << "EXPECT_CALL(session, execute(\"" << sql << "\");"
                     << std::endl;
  Session::execute(sql);
}

void Session_recorder::start_transaction() {
  Mock_record::get() << "EXPECT_CALL(session, start_transaction());"
                     << std::endl;
  Session::start_transaction();
}

void Session_recorder::commit() {
  Mock_record::get() << "EXPECT_CALL(session, commit());" << std::endl;
  Session::commit();
}

void Session_recorder::rollback() {
  Mock_record::get() << "EXPECT_CALL(session, rollback());" << std::endl;
  Session::rollback();
}

void Session_recorder::close() {
  Mock_record::get() << "EXPECT_CALL(session, close());" << std::endl;
  Session::close();
}

const char *Session_recorder::get_ssl_cipher() {
  Mock_record::get() << "EXPECT_CALL(session, get_ssl_cipher());" << std::endl;
  return Session::get_ssl_cipher();
}

Result_recorder::Result_recorder(IResult *target) : _target(target) {
  save_result();

  _rset_index = 0;
  _row_index = 0;
}

void Result_recorder::save_result() {
  auto metadata = _target->get_metadata();
  auto rset = _target->fetch_all();
  auto warnings = _target->fetch_warnings();

  _all_metadata.push_back(&metadata);
  _all_results.push_back(rset);
  _all_warnings.push_back(warnings);

  Mock_record::get() << "Mock_result result;" << std::endl;

  save_current_result(metadata, rset);

  while (_target->next_data_set()) {
    auto metadata = _target->get_metadata();
    auto rset = _target->fetch_all();
    auto warnings = _target->fetch_warnings();

    _all_metadata.push_back(&metadata);
    _all_results.push_back(rset);
    _all_warnings.push_back(warnings);

    save_current_result(metadata, rset);
  }
}

void Result_recorder::save_current_result(const std::vector<Column> &columns,
                                          const std::vector<IRow *> &rows) {
  std::string names = "{\"" + columns[0].get_column_label() + "\"";
  std::string types = "{" + map_column_type(columns[0].get_type());

  for (size_t index = 1; index < columns.size(); index++) {
    names.append(", \"" + columns[index].get_column_label() + "\"");
    types.append(", " + map_column_type(columns[index].get_type()));
  }

  names.append("}");
  types.append("}");

  Mock_record::get() << "auto fake_result = result.add_result(" << names << ", "
                     << types << ");" << std::endl;

  for (auto row : rows) {
    Mock_record::get() << "fake_result->add_row({";

    for (size_t index = 0; index < row->size(); index++) {
      if (row->is_date(index))
        Mock_record::get() << "\"" << row->get_date(index) << "\"";
      else if (row->is_double(index))
        Mock_record::get() << "\"" << row->get_double(index) << "\"";
      else if (row->is_int(index))
        Mock_record::get() << "\"" << row->get_int(index) << "\"";
      else if (row->is_string(index))
        Mock_record::get() << "\"" << row->get_string(index) << "\"";
      else if (row->is_uint(index))
        Mock_record::get() << "\"" << row->get_uint(index) << "\"";
      else if (row->is_null(index))
        Mock_record::get() << "\"___NULL___\"";

      if (index < (row->size() - 1))
        Mock_record::get() << ", ";
    }

    Mock_record::get() << "});" << std::endl;
  }
}

IRow *Result_recorder::fetch_one() {
  Mock_record::get() << "EXPECT_CALL(result, fetch_one());" << std::endl;

  if (_row_index < _all_results[_rset_index].size())
    return _all_results[_rset_index][_row_index];
  else
    return nullptr;
}

std::vector<IRow *> Result_recorder::fetch_all() {
  Mock_record::get() << "EXPECT_CALL(result, fetch_all());" << std::endl;
  if (_rset_index < _all_results.size())
    return _all_results[_rset_index];
  else
    return std::vector<IRow *>();
}

bool Result_recorder::next_data_set() {
  Mock_record::get() << "EXPECT_CALL(result, next_dataset());" << std::endl;
  if (_rset_index < _all_results.size()) {
    _rset_index++;
    return true;
  } else
    return false;
}

std::vector<IRow *> Result_recorder::fetch_warnings() {
  Mock_record::get() << "EXPECT_CALL(result, fetch_warnings());" << std::endl;
  if (_rset_index < _all_results.size())
    return _all_warnings[_rset_index];
  else
    return std::vector<IRow *>();
}

int64_t Result_recorder::get_auto_increment_value() const {
  Mock_record::get() << "EXPECT_CALL(result, get_auto_increment_value());"
                     << std::endl;
  return _target->get_auto_increment_value();
}

bool Result_recorder::has_resultset() {
  Mock_record::get() << "EXPECT_CALL(result, has_resultset());" << std::endl;
  return _target->has_resultset();
}

uint64_t Result_recorder::get_affected_row_count() const {
  Mock_record::get() << "EXPECT_CALL(result, get_affected_row_count());"
                     << std::endl;
  return _target->get_affected_row_count();
}
uint64_t Result_recorder::get_fetched_row_count() const {
  Mock_record::get() << "EXPECT_CALL(result, get_fetched_row_count());"
                     << std::endl;
  return _target->get_fetched_row_count();
}

uint64_t Result_recorder::get_warning_count() const {
  Mock_record::get() << "EXPECT_CALL(result, get_warning_count());"
                     << std::endl;
  return _target->get_warning_count();
}

unsigned long Result_recorder::get_execution_time() const {
  Mock_record::get() << "EXPECT_CALL(result, get_execution_time());"
                     << std::endl;
  return _target->get_execution_time();
}

std::string Result_recorder::get_info() const {
  Mock_record::get() << "EXPECT_CALL(result, get_info());" << std::endl;
  return _target->get_info();
}

const std::vector<Column> &Result_recorder::get_metadata() const {
  Mock_record::get() << "EXPECT_CALL(result, get_metadata());" << std::endl;
  if (_rset_index < _all_results.size())
    return *_all_metadata[_rset_index];
  else
    return _empty_metadata;
}

std::string Result_recorder::map_column_type(Type type) {
  switch (type) {
    case Type::Null:

    case Type::Decimal:
      return "Type::Decimal";
    case Type::Date:
      return "Type::Date";
    case Type::NewDate:
      return "Type::NewDate";
    case Type::Time:
      return "Type::Time";
    case Type::Time2:
      return "Type::Time2";
    case Type::String:
      return "Type::String";
    case Type::Varchar:
      return "Type::Varchar";
    case Type::String:
      return "Type::String";
    case Type::NewDecimal:
      return "Type::NewDecimal";
    case Type::TinyBlob:
      return "Type::TinyBlob";
    case Type::MediumBlob:
      return "Type::MediumBlob";
    case Type::IntegerBlob:
      return "Type::IntegerBlob";
    case Type::Blob:
      return "Type::Blob";
    case Type::Geometry:
      return "Type::Geometry";
    case Type::Json:
      return "Type::Json";
    case Type::Year:
      return "Type::Year";
    case Type::Tiny:
      return "Type::Tiny";
    case Type::Short:
      return "Type::Short:";
    case Type::Int24:
      return "Type::Int24;";
    case Type::Integer:
      return "Type::Integer";
    case Type::Integer:
      return "Type::Integer";
    case Type::Float:
      return "Type::Float";
    case Type::Double:
      return "Type::Double";
    case Type::DateTime:
      return "Type::DateTime";
    case Type::Timestamp:
      return "Type::Timestamp";
    case Type::DateTime2:
      return "Type::DateTime2";
    case Type::Timestamp2:
      return "Type::Timestamp2";
    case Type::Bit:
      return "Type::Bit";
    case Type::Enum:
      return "Type::Enum";
    case Type::Set:
      return "Type::Set";
  }
}

#endif
}  // namespace mysql
}  // namespace db
}  // namespace mysqlshdk
