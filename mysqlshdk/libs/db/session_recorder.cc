/*
 * Copyright (c) 2017, Oracle and/or its affiliates. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.0,
 * as published by the Free Software Foundation.
 *
 * This program is also distributed with certain software (including
 * but not limited to OpenSSL) that is licensed under separate terms, as
 * designated in a particular file or component or in included license
 * documentation.  The authors of MySQL hereby grant you an additional
 * permission to link the program and your derivative works with the
 * separately licensed software that they have included with MySQL.
 * This program is distributed in the hope that it will be useful,  but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 * the GNU General Public License, version 2.0, for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include <memory>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "mysqlshdk/libs/db/row_copy.h"
#include "mysqlshdk/libs/db/session_recorder.h"
#include "mysqlshdk/libs/utils/utils_string.h"
#include "utils/utils_general.h"

namespace mysqlshdk {
namespace db {
Mock_record* Mock_record::_instance = NULL;

void Session_recorder::connect(
    const mysqlshdk::db::Connection_options& connection_options) {
  _target->connect(connection_options);
  // TODO(rennox): Create the recording of this call...
}

std::shared_ptr<IResult> Session_recorder::query(const std::string& sql,
                                                 bool /*buffered*/) {
  std::shared_ptr<IResult> ret_val;

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

void Session_recorder::close() {
  Mock_record::get() << "EXPECT_CALL(session, close());" << std::endl;
  _target->close();
}

const char* Session_recorder::get_ssl_cipher() const {
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
  while (first || _target->next_resultset()) {
    if (!first)
      Mock_record::get() << "," << std::endl;

    first = false;

    auto metadata = _target->get_metadata();
    _all_metadata.push_back(&metadata);

    std::vector<std::unique_ptr<IRow> > rows;
    auto row = _target->fetch_one();
    while (row) {
      rows.push_back(std::unique_ptr<IRow>(new Row_copy(*row)));
      row = _target->fetch_one();
    }

    save_current_result(metadata, rows);

    _all_results.push_back(std::move(rows));

    std::vector<std::unique_ptr<Warning> > warnings;
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

    for (uint32_t index = 0; index < row->num_fields(); index++) {
      if (row->is_null(index)) {
        Mock_record::get() << "\"___NULL___\"";
      } else {
        switch (row->get_type(index)) {
          case Type::Null:
            Mock_record::get() << "\"___NULL___\"";
            break;
          case Type::String:
            Mock_record::get() << "\"" << row->get_string(index) << "\"";
            break;
          case Type::UInteger:
            Mock_record::get() << "\"" << row->get_uint(index) << "\"";
            break;
          case Type::Integer:
            Mock_record::get() << "\"" << row->get_int(index) << "\"";
            break;
          case Type::Float:
            Mock_record::get() << "\"" << row->get_float(index) << "\"";
            break;
          case Type::Decimal:
          case Type::Double:
            Mock_record::get() << "\"" << row->get_double(index) << "\"";
            break;
          case Type::Bytes:
            Mock_record::get() << "\"" << row->get_string(index) << "\"";
            break;
          case Type::Geometry:
          case Type::Json:
          case Type::Date:
          case Type::DateTime:
          case Type::Time:
          case Type::Enum:
          case Type::Set:
            Mock_record::get() << "\"" << row->get_string(index) << "\"";
            break;
          case Type::Bit:
            Mock_record::get() << "\"" << row->get_bit(index) << "\"";
            break;
        }
      }
      if (index < (row->num_fields() - 1))
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

const IRow* Result_recorder::fetch_one() {
  if (_row_index < _all_results[_rset_index].size())
    return _all_results[_rset_index][_row_index++].get();
  return nullptr;
}

bool Result_recorder::next_resultset() {
  Mock_record::get() << "EXPECT_CALL(result, next_dataset());" << std::endl;

  if (_rset_index < _all_results.size()) {
    _rset_index++;
    _row_index = 0;
    _warning_index = 0;
  }

  return _rset_index < _all_results.size();
}

std::unique_ptr<Warning> Result_recorder::fetch_one_warning() {
  if (_rset_index < _all_results.size())
    return std::move(_all_warnings[_rset_index][_warning_index++]);
  return nullptr;
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
  return to_string(type);
}
}  // namespace db
}  // namespace mysqlshdk
