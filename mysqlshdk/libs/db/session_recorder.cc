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

#include <sstream>
#include <string>
#include <vector>

#include "mysqlshdk/libs/db/session_recorder.h"
#include "utils/utils_general.h"

namespace mysqlshdk {
namespace db {
Mock_record* Mock_record::_instance = NULL;

void Session_recorder::connect(const std::string& uri, const char* password) {
  if (password)
    Mock_record::get() << "EXPECT_CALL(session, connect(\"" << uri << "\", \""
                       << password << "\"));" << std::endl;
  else
    Mock_record::get() << "EXPECT_CALL(session, connect(\"" << uri << "\"));"
                       << std::endl;

  _target->connect(uri, password);
}

Session_recorder::Session_recorder(ISession* target) : _target(target) {}

void Session_recorder::connect(const std::string& host, int port,
                               const std::string& socket,
                               const std::string& user,
                               const std::string& password,
                               const std::string& schema,
                               const mysqlshdk::utils::Ssl_info& ssl_info) {
  if (ssl_info.has_data()) {
    Mock_record::get() << "mysqlshdk::utils::Ssl_info ssl_info;" << std::endl;

    if (!ssl_info.ca.is_null())
      Mock_record::get() << "ssl_info.ca = " << *ssl_info.ca << ";"
                         << std::endl;

    if (!ssl_info.capath.is_null())
      Mock_record::get() << "ssl_info.capath = " << *ssl_info.capath << ";"
                         << std::endl;

    if (!ssl_info.cert.is_null())
      Mock_record::get() << "ssl_info.cert = " << *ssl_info.cert << ";"
                         << std::endl;

    if (!ssl_info.ciphers.is_null())
      Mock_record::get() << "ssl_info.ciphers = " << *ssl_info.ciphers << ";"
                         << std::endl;

    if (!ssl_info.crl.is_null())
      Mock_record::get() << "ssl_info.crl = " << *ssl_info.crl << ";"
                         << std::endl;

    if (!ssl_info.crlpath.is_null())
      Mock_record::get() << "ssl_info.crlpath = " << *ssl_info.crlpath << ";"
                         << std::endl;

    if (!ssl_info.key.is_null())
      Mock_record::get() << "ssl_info.key = " << *ssl_info.key << ";"
                         << std::endl;

    if (!ssl_info.tls_version.is_null())
      Mock_record::get() << "ssl_info.tls_version = " << *ssl_info.tls_version
                         << ";" << std::endl;

    Mock_record::get() << "ssl_info.mode = " << ssl_info.mode << ";"
                       << std::endl;
  }

  Mock_record::get() << "EXPECT_CALL(session, connect(\"" << host << "\", "
                     << port << ", \"" << socket << "\", \"" << user << "\", \""
                     << password << "\", " << schema << ", ssl_info));"
                     << std::endl;

  _target->connect(host, port, socket, user, password, schema, ssl_info);
}

std::unique_ptr<IResult> Session_recorder::query(const std::string& sql,
                                                 bool /*buffered*/) {
  std::unique_ptr<IResult> ret_val;

  // While mock recording, all is buffered
  try {
    auto result = _target->query(sql, true);

    Mock_record::get() << std::endl
                       << "============= Stored Query ============="
                       << std::endl
                       << "session.expect_query(\"" << sql << "\")."
                       << std::endl;
    ret_val.reset(new Result_recorder(result.get()));
    Mock_record::get() << "========================================"
                       << std::endl
                       << std::endl;
  } catch (const std::exception& err) {
    Mock_record::get()
        << std::endl
        << "============= Failed Query =============" << std::endl
        << "session.expect_query(\"" << sql << "\").then_throw();" << std::endl
        << std::endl
        << "// An exception will be produced, enclose the failing code below"
        << std::endl
        << "EXPECT_ANY_THROW(<Code that generated the above call>);"
        << std::endl
        << "========================================" << std::endl
        << std::endl;
    throw;
  }

  return ret_val;
}

void Session_recorder::execute(const std::string& sql) {
  Mock_record::get() << "EXPECT_CALL(session, execute(\"" << sql << "\"));"
                     << std::endl;
  _target->execute(sql);
}

void Session_recorder::start_transaction() {
  Mock_record::get() << "EXPECT_CALL(session, start_transaction());"
                     << std::endl;
  _target->start_transaction();
}

void Session_recorder::commit() {
  Mock_record::get() << "EXPECT_CALL(session, commit());" << std::endl;
  _target->commit();
}

void Session_recorder::rollback() {
  Mock_record::get() << "EXPECT_CALL(session, rollback());" << std::endl;
  _target->rollback();
}

void Session_recorder::close() {
  Mock_record::get() << "EXPECT_CALL(session, close());" << std::endl;
  _target->close();
}

const char* Session_recorder::get_ssl_cipher() {
  const char* cipher = _target->get_ssl_cipher();
  Mock_record::get()
      << "EXPECT_CALL(session, get_ssl_cipher().WillOnce(Return(\"" << cipher
      << "\"));" << std::endl;
  return cipher;
}

Result_recorder::Result_recorder(IResult* target) : _target(target) {
  save_result();

  _rset_index = 0;
  _row_index = 0;
}

void Result_recorder::save_result() {
  Mock_record::get() << "then_return({" << std::endl;

  Mock_record::get() << "\"<Executed SQL>\"," << std::endl;

  bool first = true;
  while (first || _target->next_data_set()) {
    if (!first)
      Mock_record::get() << "," << std::endl;

    first = false;

    auto metadata = _target->get_metadata();
    _all_metadata.push_back(&metadata);

    std::vector<std::unique_ptr<IRow> > rows;
    auto row = _target->fetch_one();
    while (row) {
      rows.push_back(std::move(row));
      row = _target->fetch_one();
    }

    save_current_result(metadata, rows);

    _all_results.push_back(std::move(rows));

    std::vector<std::unique_ptr<IRow> > warnings;
    auto warning = _target->fetch_one_warning();
    while (warning) {
      warnings.push_back(std::move(warning));
      warning = _target->fetch_one_warning();
    }

    _all_warnings.push_back(std::move(warnings));
  }

  Mock_record::get() << "});" << std::endl;
}

void Result_recorder::save_current_result(
    const std::vector<Column>& columns,
    const std::vector<std::unique_ptr<IRow> >& rows) {
  std::string names = "{\"" + columns[0].get_column_label() + "\"";
  std::string types = "{" + map_column_type(columns[0].get_type());

  for (size_t index = 1; index < columns.size(); index++) {
    names.append(", \"" + columns[index].get_column_label() + "\"");
    types.append(", " + map_column_type(columns[index].get_type()));
  }

  names.append("}");
  types.append("}");

  // Result Opening, names, types and Rows Opening
  Mock_record::get() << "{ " << std::endl
                     << "   " << names << "," << std::endl
                     << "   " << types << "," << std::endl
                     << "   {";

  if (!rows.empty())
    Mock_record::get() << std::endl;

  bool first = true;
  for (auto& row : rows) {
    if (!first)
      Mock_record::get() << "}," << std::endl;  // Row End (Not Last)
    else
      first = false;

    Mock_record::get() << "     {";  // Row Start

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
  }

  // Last Row Closing
  if (!rows.empty())
    Mock_record::get() << "}" << std::endl << "   ";

  // Rows Closing and Result Closing
  Mock_record::get() << "}";

  if (rows.empty())
    Mock_record::get() << "  // No Records...";

  Mock_record::get() << std::endl << "}" << std::endl;
}

std::unique_ptr<IRow> Result_recorder::fetch_one() {
  std::unique_ptr<IRow> ret_val;

  if (_row_index < _all_results[_rset_index].size())
    ret_val.reset(_all_results[_rset_index][_row_index++].release());

  return ret_val;
}

bool Result_recorder::next_data_set() {
  Mock_record::get() << "EXPECT_CALL(result, next_dataset());" << std::endl;

  if (_rset_index < _all_results.size()) {
    _rset_index++;
    _row_index = 0;
    _warning_index = 0;
  }

  return _rset_index < _all_results.size();
}

std::unique_ptr<IRow> Result_recorder::fetch_one_warning() {
  std::unique_ptr<IRow> ret_val;
  if (_rset_index < _all_results.size())
    ret_val.reset(_all_warnings[_rset_index][_warning_index++].release());

  return ret_val;
}

int64_t Result_recorder::get_auto_increment_value() const {
  auto ret_val = _target->get_auto_increment_value();
  Mock_record::get()
      << "EXPECT_CALL(result, get_auto_increment_value().WillOnce(Return("
      << ret_val << "));" << std::endl;
  return ret_val;
}

bool Result_recorder::has_resultset() {
  auto ret_val = _target->has_resultset();
  Mock_record::get() << "EXPECT_CALL(result, has_resultset().WillOnce(Return("
                     << (ret_val ? "true" : "false") << "));" << std::endl;
  return ret_val;
}

uint64_t Result_recorder::get_affected_row_count() const {
  auto ret_val = _target->get_affected_row_count();
  Mock_record::get()
      << "EXPECT_CALL(result, get_affected_row_count().WillOnce(Return("
      << ret_val << "));" << std::endl;
  return ret_val;
}
uint64_t Result_recorder::get_fetched_row_count() const {
  auto ret_val = _target->get_fetched_row_count();
  Mock_record::get()
      << "EXPECT_CALL(result, get_fetched_row_count().WillOnce(Return("
      << ret_val << "));" << std::endl;
  return ret_val;
}

uint64_t Result_recorder::get_warning_count() const {
  auto ret_val = _target->get_warning_count();
  Mock_record::get()
      << "EXPECT_CALL(result, get_warning_count().WillOnce(Return(" << ret_val
      << "));" << std::endl;
  return ret_val;
}

unsigned long Result_recorder::get_execution_time() const {
  auto ret_val = _target->get_execution_time();
  Mock_record::get()
      << "EXPECT_CALL(result, get_execution_time().WillOnce(Return(" << ret_val
      << "));" << std::endl;
  return ret_val;
}

std::string Result_recorder::get_info() const {
  auto ret_val = _target->get_info();
  Mock_record::get() << "EXPECT_CALL(result, get_info().WillOnce(Return(\""
                     << ret_val << "\"));" << std::endl;
  return ret_val;
}

const std::vector<Column>& Result_recorder::get_metadata() const {
  Mock_record::get() << "EXPECT_CALL(result, get_metadata());" << std::endl;
  if (_rset_index < _all_results.size())
    return *_all_metadata[_rset_index];
  else
    return _empty_metadata;
}

std::string Result_recorder::map_column_type(Type type) {
  switch (type) {
    case Type::Null:
      return "Type::Null";
    case Type::Decimal:
      return "Type::Decimal";
    case Type::Date:
      return "Type::Date";
    case Type::NewDate:
      return "Type::NewDate";
    case Type::Time:
      return "Type::Time";
    case Type::String:
      return "Type::String";
    case Type::VarChar:
      return "Type::Varchar";
    case Type::VarString:
      return "Type::VarString";
    case Type::NewDecimal:
      return "Type::NewDecimal";
    case Type::TinyBlob:
      return "Type::TinyBlob";
    case Type::MediumBlob:
      return "Type::MediumBlob";
    case Type::LongBlob:
      return "Type::LongBlob";
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
    case Type::Long:
      return "Type::Long";
    case Type::LongLong:
      return "Type::LongLong";
    case Type::Float:
      return "Type::Float";
    case Type::Double:
      return "Type::Double";
    case Type::DateTime:
      return "Type::DateTime";
    case Type::Timestamp:
      return "Type::Timestamp";
    case Type::Bit:
      return "Type::Bit";
    case Type::Enum:
      return "Type::Enum";
    case Type::Set:
      return "Type::Set";
  }

  throw std::runtime_error("Invalid column type found");

  return "";
}
}  // namespace db
}  // namespace mysqlshdk
