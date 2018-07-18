/*
 * Copyright (c) 2014, 2018, Oracle and/or its affiliates. All rights reserved.
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
#define __STDC_FORMAT_MACROS 1

#include "shellcore/shell_resultset_dumper.h"

#include <algorithm>
#include <cinttypes>

#include "modules/devapi/mod_mysqlx_resultset.h"
#include "modules/mod_mysql_resultset.h"
#include "mysqlshdk/include/shellcore/base_shell.h"
#include "shellcore/interrupt_handler.h"
#include "utils/utils_string.h"

class Field_formatter {
 public:
  Field_formatter(bool align_right, size_t width,
                  const mysqlsh::Column &column) {
    _allocated = std::max<size_t>(1024, width + 1);
    _zerofill = column.is_zerofill() ? column.get_length() : 0;
    _align_right = align_right;
    _buffer = new char[_allocated];
    _column_width = width;
  }

  Field_formatter() {
    _buffer = nullptr;
    _allocated = 0;
  }

  Field_formatter(Field_formatter &&other) {
    this->operator=(std::move(other));
  }

  void operator=(Field_formatter &&other) {
    _allocated = other._allocated;
    _zerofill = other._zerofill;
    _align_right = other._align_right;
    _buffer = other._buffer;
    _column_width = other._column_width;

    other._buffer = nullptr;
    other._allocated = 0;
  }

  ~Field_formatter() { delete[] _buffer; }

  bool put(const shcore::Value &value) {
    reset();

    switch (value.type) {
      case shcore::String:
        if (value.value.s->length() < _allocated && _column_width > 0) {
          append(value.value.s->c_str(), value.value.s->length());
        } else {
          // text too long (or not padded), let caller dump it raw
          return false;
        }
        break;

      case shcore::Null:
        append("NULL", 4);
        break;

      case shcore::Bool:
        append(value.as_bool() ? "1" : "0", 1);
        break;

      case shcore::Float:
      case shcore::Integer:
      case shcore::UInteger: {
        std::string tmp = value.descr();
        if (_zerofill > tmp.length()) {
          tmp = std::string(_zerofill - tmp.length(), '0').append(tmp);
        }
        append(tmp.data(), tmp.length());
        break;
      }

      case shcore::Object: {
        std::string val = value.descr();
        append(val.data(), val.length());
        break;
      }

      default:
        append("????", 4);
        break;
    }
    return true;
  }

  const char *c_str() const { return _buffer; }

 private:
  void reset() { memset(_buffer, ' ', _allocated); }

  inline void append(const char *text, size_t length) {
    if (_column_width > 0) {
      if (_align_right)
        memcpy(_buffer + _column_width - length, text, length);
      else
        memcpy(_buffer, text, length);
      _buffer[_column_width] = 0;
    } else {
      memcpy(_buffer, text, length);
      _buffer[length] = 0;
    }
  }

 private:
  char *_buffer;
  size_t _allocated;
  size_t _column_width;
  size_t _zerofill;
  bool _align_right;
};

// TODO(alfredo) this should be used instead of the below check once
// code is changed to use mysqlshdk::db
// static bool is_numeric_type(mysqlshdk::db::Type type) {
//   return (type == mysqlshdk::db::Type::Integer ||
//     type == mysqlshdk::db::Type::UInteger ||
//     type == mysqlshdk::db::Type::Float ||
//     type == mysqlshdk::db::Type::Double ||
//     type == mysqlshdk::db::Type::Decimal);
// }

static bool is_numeric_type(shcore::Value value) {
  std::string id = value.descr();
  if (id.length() > 7) id = id.substr(6, id.length() - 7);
  return (id == "BIT" || id == "TINYINT" || id == "SMALLINT" ||
          id == "MEDIUMINT" || id == "INT" || id == "INTEGER" || id == "LONG" ||
          id == "BIGINT" || id == "FLOAT" || id == "DECIMAL" || id == "DOUBLE");
}

ResultsetDumper::ResultsetDumper(
    std::shared_ptr<mysqlsh::ShellBaseResult> target,
    shcore::Interpreter_delegate *output_handler, bool buffer_data)
    : _output_handler(output_handler),
      _resultset(target),
      _buffer_data(buffer_data),
      _cancelled(false) {
  _format = mysqlsh::current_shell_options()->get().output_format;
  _interactive = mysqlsh::current_shell_options()->get().interactive;
  _show_warnings = mysqlsh::current_shell_options()->get().show_warnings;
}

void ResultsetDumper::dump() {
  std::string type = _resultset->class_name();

  _cancelled = false;

  // Buffers the data remaining on the record
  if (_buffer_data) {
    _resultset->buffer();
  }

  {
    shcore::Interrupt_handler intr([this]() {
      _cancelled = true;
      return true;
    });
    if (_format.find("json") == 0)
      dump_json();
    else
      dump_normal();
  }
  if (_cancelled)
    _output_handler->print(
        _output_handler->user_data,
        "Result printing interrupted, rows may be missing from the output.\n");

  // Restores data
  if (_buffer_data) _resultset->rewind();
}

void ResultsetDumper::dump_json() {
  shcore::Value resultset(
      std::static_pointer_cast<shcore::Object_bridge>(_resultset));

  _output_handler->print_value(_output_handler->user_data, resultset, "");
}

void ResultsetDumper::dump_normal() {
  std::string output;

  std::string class_name = _resultset->class_name();

  if (class_name == "ClassicResult") {
    std::shared_ptr<mysqlsh::mysql::ClassicResult> resultset =
        std::static_pointer_cast<mysqlsh::mysql::ClassicResult>(_resultset);
    if (resultset) dump_normal(resultset);
  } else if (class_name == "SqlResult") {
    std::shared_ptr<mysqlsh::mysqlx::SqlResult> resultset =
        std::static_pointer_cast<mysqlsh::mysqlx::SqlResult>(_resultset);
    if (resultset) dump_normal(resultset);
  } else if (class_name == "RowResult") {
    std::shared_ptr<mysqlsh::mysqlx::RowResult> resultset =
        std::static_pointer_cast<mysqlsh::mysqlx::RowResult>(_resultset);
    if (resultset) dump_normal(resultset);
  } else if (class_name == "DocResult") {
    std::shared_ptr<mysqlsh::mysqlx::DocResult> resultset =
        std::static_pointer_cast<mysqlsh::mysqlx::DocResult>(_resultset);
    if (resultset) dump_normal(resultset);
  } else if (class_name == "Result") {
    std::shared_ptr<mysqlsh::mysqlx::Result> resultset =
        std::static_pointer_cast<mysqlsh::mysqlx::Result>(_resultset);
    if (resultset) dump_normal(resultset);
  }
}

void ResultsetDumper::dump_normal(
    std::shared_ptr<mysqlsh::mysql::ClassicResult> result) {
  std::string output;

  do {
    if (result->has_data(shcore::Argument_list()).as_bool())
      dump_records(output);
    else if (_interactive)
      output = get_affected_stats("row");

    // This information output is only printed in interactive mode
    int warning_count = 0;
    if (_interactive) {
      warning_count = get_warning_and_execution_time_stats(output);

      _output_handler->print(_output_handler->user_data, output.c_str());
    }

    std::string info = result->get_member("info").as_string();
    if (!info.empty()) {
      info = "\n" + info + "\n";
      _output_handler->print(_output_handler->user_data, info.c_str());
    }

    // Prints the warnings if there were any
    if (warning_count && _show_warnings) dump_warnings(true);
  } while (result->next_result(shcore::Argument_list()).as_bool() &&
           !_cancelled);
}

void ResultsetDumper::dump_normal(
    std::shared_ptr<mysqlsh::mysqlx::SqlResult> result) {
  std::string output;

  do {
    if (result->has_data(shcore::Argument_list()).as_bool())
      dump_records(output);
    else if (_interactive)
      output = get_affected_stats("row");

    // This information output is only printed in interactive mode
    if (_interactive) {
      int warning_count = get_warning_and_execution_time_stats(output);

      _output_handler->print(_output_handler->user_data, output.c_str());

      // Prints the warnings if there were any
      if (warning_count && _show_warnings) dump_warnings();
    }
  } while (result->next_result(shcore::Argument_list()).as_bool() &&
           !_cancelled);
}

void ResultsetDumper::dump_normal(
    std::shared_ptr<mysqlsh::mysqlx::RowResult> result) {
  std::string output;

  dump_records(output);

  // This information output is only printed in interactive mode
  if (_interactive) {
    int warning_count = get_warning_and_execution_time_stats(output);

    _output_handler->print(_output_handler->user_data, output.c_str());

    // Prints the warnings if there were any
    if (warning_count && _show_warnings) dump_warnings();
  }
}

void ResultsetDumper::dump_normal(
    std::shared_ptr<mysqlsh::mysqlx::DocResult> result) {
  std::string output;

  shcore::Value documents = result->fetch_all(shcore::Argument_list());
  shcore::Value::Array_type_ref array_docs = documents.as_array();

  if (array_docs->size()) {
    _output_handler->print_value(_output_handler->user_data, documents, "");

    size_t row_count = array_docs->size();
    output = shcore::str_format("%zu %s in set", row_count,
                                (row_count == 1 ? "document" : "documents"));
  } else {
    output = "Empty set";
  }
  // This information output is only printed in interactive mode
  if (_interactive) {
    int warning_count = get_warning_and_execution_time_stats(output);

    _output_handler->print(_output_handler->user_data, output.c_str());

    // Prints the warnings if there were any
    if (warning_count && _show_warnings) dump_warnings();
  }
}

void ResultsetDumper::dump_normal(
    std::shared_ptr<mysqlsh::mysqlx::Result> result) {
  // This information output is only printed in interactive mode
  if (_interactive) {
    std::string output = get_affected_stats("item");
    int warning_count = get_warning_and_execution_time_stats(output);

    _output_handler->print(_output_handler->user_data, output.c_str());

    // Prints the warnings if there were any
    if (warning_count && _show_warnings) dump_warnings();
  }
}

size_t ResultsetDumper::dump_tabbed(shcore::Value::Array_type_ref records) {
  std::shared_ptr<shcore::Value::Array_type> metadata =
      _resultset->get_member("columns").as_array();

  size_t index = 0;
  size_t field_count = metadata->size();
  std::vector<std::string> formats(field_count, "%-");
  std::vector<Field_formatter> fmt;

  // Prints the initial separator line and the column headers
  // TODO: Consider the charset information on the length calculations
  fmt.resize(field_count);
  for (index = 0; index < field_count; index++) {
    auto column = std::static_pointer_cast<mysqlsh::Column>(
        metadata->at(index).as_object());
    fmt[index] = Field_formatter(false, 0, *column);
    _output_handler->print(_output_handler->user_data,
                           column->get_column_label().c_str());
    _output_handler->print(_output_handler->user_data,
                           index < (field_count - 1) ? "\t" : "\n");
  }

  // Now prints the records
  size_t row_index;
  for (row_index = 0; row_index < records->size() && !_cancelled; row_index++) {
    auto row = (*records)[row_index].as_object<mysqlsh::Row>();

    for (size_t field_index = 0; field_index < field_count; field_index++) {
      shcore::Value value = row->get_member(field_index);
      if (fmt[field_index].put(value)) {
        _output_handler->print(_output_handler->user_data,
                               fmt[field_index].c_str());
      } else {
        assert(value.type == shcore::String);
        _output_handler->print(_output_handler->user_data,
                               value.value.s->c_str());
      }
      _output_handler->print(_output_handler->user_data,
                             field_index < (field_count - 1) ? "\t" : "\n");
    }
  }
  return row_index;
}

size_t ResultsetDumper::dump_vertical(shcore::Value::Array_type_ref records) {
  std::shared_ptr<shcore::Value::Array_type> metadata =
      _resultset->get_member("columns").as_array();
  std::string star_separator(27, '*');

  std::vector<Field_formatter> fmt;

  // Calculate length of a longest column description, used to right align
  // column descriptions
  std::size_t max_col_len = 0;
  for (size_t col_index = 0; col_index < metadata->size(); col_index++) {
    auto column = std::static_pointer_cast<mysqlsh::Column>(
        metadata->at(col_index).as_object());
    max_col_len = std::max(max_col_len, column->get_column_label().length());
    fmt.push_back({false, 0, *column});
  }

  for (size_t row_index = 0; row_index < records->size() && !_cancelled;
       row_index++) {
    std::string row_header = star_separator + " " +
                             std::to_string(row_index + 1) + ". row " +
                             star_separator + "\n";

    _output_handler->print(_output_handler->user_data, row_header.c_str());

    for (size_t col_index = 0; col_index < metadata->size(); col_index++) {
      auto column = std::static_pointer_cast<mysqlsh::Column>(
          metadata->at(col_index).as_object());

      auto row = records->at(row_index).as_object<mysqlsh::Row>();
      std::string padding(max_col_len - column->get_column_label().size(), ' ');
      std::string label = padding + column->get_column_label() + ": ";

      _output_handler->print(_output_handler->user_data, label.c_str());
      shcore::Value value = row->get_member(col_index);
      if (fmt[col_index].put(value)) {
        _output_handler->print(_output_handler->user_data,
                               fmt[col_index].c_str());
      } else {
        assert(value.type == shcore::String);
        _output_handler->print(_output_handler->user_data,
                               value.value.s->c_str());
      }
      _output_handler->print(_output_handler->user_data, "\n");
    }

    if (_cancelled) return row_index;
  }
  return records->size();
}

size_t ResultsetDumper::dump_table(shcore::Value::Array_type_ref records) {
  std::shared_ptr<shcore::Value::Array_type> metadata =
      _resultset->get_member("columns").as_array();
  std::vector<uint64_t> max_lengths;
  std::vector<std::shared_ptr<mysqlsh::Column>> column_meta;

  size_t field_count = metadata->size();
  if (field_count == 0) return 0;

  // Updates the max_length array with the maximum length between column name,
  // min column length and column max length
  for (size_t field_index = 0; field_index < field_count; field_index++) {
    auto column = std::static_pointer_cast<mysqlsh::Column>(
        metadata->at(field_index).as_object());

    column_meta.push_back(column);

    if (column->is_zerofill()) {
      max_lengths.push_back(column->get_length());
    } else {
      max_lengths.push_back(0);
    }
    max_lengths[field_index] = std::max<uint64_t>(
        max_lengths[field_index], column->get_column_label().length());
  }

  // Now updates the length with the real column data lengths
  size_t row_index;
  for (row_index = 0; row_index < records->size() && !_cancelled; row_index++) {
    auto row = (*records)[row_index].as_object<mysqlsh::Row>();
    for (size_t field_index = 0; field_index < field_count; field_index++) {
      max_lengths[field_index] =
          std::max<uint64_t>(max_lengths[field_index],
                             row->get_member(field_index).descr().length());
    }
  }
  if (_cancelled) return 0;

  //-----------

  size_t index = 0;
  std::vector<Field_formatter> fmt;

  std::string separator("+");
  for (index = 0; index < field_count; index++) {
    std::string field_separator(max_lengths[index] + 2, '-');
    field_separator.append("+");
    separator.append(field_separator);

    if (column_meta[index]->is_zerofill()) {
      fmt.push_back(
          {true, static_cast<size_t>(max_lengths[index]), *column_meta[index]});
    } else {
      fmt.push_back({is_numeric_type(column_meta[index]->get_type()),
                     static_cast<size_t>(max_lengths[index]),
                     *column_meta[index]});
    }
  }
  separator.append("\n");

  // Prints the initial separator line and the column headers
  // TODO: Consider the charset information on the length calculations
  _output_handler->print(_output_handler->user_data, separator.c_str());
  _output_handler->print(_output_handler->user_data, "| ");
  for (index = 0; index < field_count; index++) {
    std::string format = "%-";
    format.append(std::to_string(max_lengths[index]));
    format.append((index == field_count - 1) ? "s |\n" : "s | ");
    _output_handler->print(
        _output_handler->user_data,
        shcore::str_format(format.c_str(),
                           column_meta[index]->get_column_label().c_str())
            .c_str());
  }
  _output_handler->print(_output_handler->user_data, separator.c_str());

  // Now prints the records
  for (row_index = 0; row_index < records->size() && !_cancelled; row_index++) {
    _output_handler->print(_output_handler->user_data, "| ");

    auto row = (*records)[row_index].as_object<mysqlsh::Row>();

    for (size_t field_index = 0; field_index < field_count; field_index++) {
      shcore::Value value(row->get_member(field_index));
      if (fmt[field_index].put(value)) {
        _output_handler->print(_output_handler->user_data,
                               fmt[field_index].c_str());
      } else {
        assert(value.type == shcore::String);
        _output_handler->print(_output_handler->user_data,
                               value.value.s->c_str());
      }
      if (field_index < field_count - 1)
        _output_handler->print(_output_handler->user_data, " | ");
    }
    _output_handler->print(_output_handler->user_data, " |\n");
  }

  _output_handler->print(_output_handler->user_data, separator.c_str());

  return row_index;
}

std::string ResultsetDumper::get_affected_stats(const std::string &legend) {
  std::string output;

  // Some queries return -1 since affected rows do not apply to them
  int64_t affected_items =
      _resultset->get_member("affectedItemsCount").as_int();
  // if (affected_items == (uint64_t)-1)
  if (affected_items == -1)
    output = "Query OK";
  else
    // In case of Query OK, prints the actual number of affected rows.
    output = shcore::str_format(
        "Query OK, %" PRId64 " %s affected", affected_items,
        (affected_items == 1 ? legend : legend + "s").c_str());

  return output;
}

int ResultsetDumper::get_warning_and_execution_time_stats(
    std::string &output_stats) {
  int warning_count = 0;

  if (_interactive) {
    warning_count = _resultset->get_member("warningsCount").as_uint();

    if (warning_count)
      output_stats.append(shcore::str_format(", %d warning%s", warning_count,
                                             (warning_count == 1 ? "" : "s")));

    output_stats.append(" ");
    output_stats.append(shcore::str_format(
        "(%s)", _resultset->get_member("executionTime").as_string().c_str()));
    output_stats.append("\n");
  }

  return warning_count;
}

void ResultsetDumper::dump_records(std::string &output_stats) {
  shcore::Value records = _resultset->call("fetchAll", shcore::Argument_list());
  shcore::Value::Array_type_ref array_records = records.as_array();

  if (array_records->size()) {
    size_t row_count = 0;
    // print rows from result, with stats etc
    if (_format == "vertical")
      row_count = dump_vertical(array_records);
    else if (_format == "table")
      row_count = dump_table(array_records);
    else
      row_count = dump_tabbed(array_records);

    output_stats = shcore::str_format("%zu %s in set", row_count,
                                      (row_count == 1 ? "row" : "rows"));
  } else {
    output_stats = "Empty set";
  }
}

void ResultsetDumper::dump_warnings(bool classic) {
  shcore::Value warnings = _resultset->get_member("warnings");

  if (warnings) {
    shcore::Value::Array_type_ref warning_list = warnings.as_array();
    size_t index = 0, size = warning_list->size();

    while (index < size && !_cancelled) {
      shcore::Value record = warning_list->at(index);
      std::shared_ptr<mysqlsh::Row> row = record.as_object<mysqlsh::Row>();

      std::string code = "code";
      std::string level = "level";
      std::string message = "message";

      unsigned long error = row->get_member(code).as_int();

      std::string type = row->get_member(level).as_string();
      std::string msg = row->get_member(message).as_string();
      _output_handler->print(
          _output_handler->user_data,
          (shcore::str_format("%s (code %ld): %s\n", type.c_str(), error,
                              msg.c_str()))
              .c_str());

      index++;
    }
  }
}

/*Value ResultsetDumper::get_all_records(mysqlsh::mysql::ClassicResult* result)
{
return result->all(shcore::Argument_list());
}
Value ResultsetDumper::get_all_records(mysqlsh::mysqlx::SqlResult* result)
{
return result->fetch_all(shcore::Argument_list());
}

bool ResultsetDumper::move_next_data_set(mysqlsh::mysql::ClassicResult* result)
{
return result->next_result(shcore::Argument_list());
}
bool ResultsetDumper::move_next_data_set(mysqlsh::mysqlx::SqlResult* result)
{
return result->next_result(shcore::Argument_list());
}*/
