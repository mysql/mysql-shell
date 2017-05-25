/*
 * Copyright (c) 2014, 2017, Oracle and/or its affiliates. All rights reserved.
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

#include "shellcore/shell_resultset_dumper.h"
#include "shellcore/shell_core_options.h"
#include "modules/mod_mysql_resultset.h"
#include "modules/devapi/mod_mysqlx_resultset.h"
#include "utils/utils_string.h"

#define MAX_COLUMN_LENGTH 1024
#define MIN_COLUMN_LENGTH 4

using options = shcore::Shell_core_options;

ResultsetDumper::ResultsetDumper(
    std::shared_ptr<mysqlsh::ShellBaseResult> target,
    shcore::Interpreter_delegate* output_handler, bool buffer_data)
    : _output_handler(output_handler),
      _resultset(target),
      _buffer_data(buffer_data) {
  _format = options::get()->get_string(SHCORE_OUTPUT_FORMAT);
  _interactive = options::get()->get_bool(SHCORE_INTERACTIVE);
  _show_warnings = options::get()->get_bool(SHCORE_SHOW_WARNINGS);
}

void ResultsetDumper::dump() {
  std::string type = _resultset->class_name();

  // Buffers the data remaining on the record
  size_t rset, record;
  bool buffered = false;
  if (_buffer_data) {
    _resultset->buffer();

    // Stores the current data set/record position on the result
    buffered = _resultset->tell(rset, record);
  }

  if (_format.find("json") == 0)
    dump_json();
  else
    dump_normal();

  // Restores the data set/record positions on the result
  if (buffered)
    _resultset->seek(rset, record);
}

void ResultsetDumper::dump_json() {
  shcore::Value resultset(std::static_pointer_cast<shcore::Object_bridge>(_resultset));

  _output_handler->print_value(_output_handler->user_data, resultset, "");
}

void ResultsetDumper::dump_normal() {
  std::string output;

  std::string class_name = _resultset->class_name();

  if (class_name == "ClassicResult") {
    std::shared_ptr<mysqlsh::mysql::ClassicResult> resultset = std::static_pointer_cast<mysqlsh::mysql::ClassicResult>(_resultset);
    if (resultset)
      dump_normal(resultset);
  } else if (class_name == "SqlResult") {
    std::shared_ptr<mysqlsh::mysqlx::SqlResult> resultset = std::static_pointer_cast<mysqlsh::mysqlx::SqlResult>(_resultset);
    if (resultset)
      dump_normal(resultset);
  } else if (class_name == "RowResult") {
    std::shared_ptr<mysqlsh::mysqlx::RowResult> resultset = std::static_pointer_cast<mysqlsh::mysqlx::RowResult>(_resultset);
    if (resultset)
      dump_normal(resultset);
  } else if (class_name == "DocResult") {
    std::shared_ptr<mysqlsh::mysqlx::DocResult> resultset = std::static_pointer_cast<mysqlsh::mysqlx::DocResult>(_resultset);
    if (resultset)
      dump_normal(resultset);
  } else if (class_name == "Result") {
    std::shared_ptr<mysqlsh::mysqlx::Result> resultset = std::static_pointer_cast<mysqlsh::mysqlx::Result>(_resultset);
    if (resultset)
      dump_normal(resultset);
  }
}

void ResultsetDumper::dump_normal(std::shared_ptr<mysqlsh::mysql::ClassicResult> result) {
  std::string output;

  do {
    if (result->has_data(shcore::Argument_list()).as_bool())
      dump_records(output);
    else if (_interactive)
      output = get_affected_stats("affectedRowCount", "row");

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
    if (warning_count && _show_warnings)
      dump_warnings(true);
  } while (result->next_data_set(shcore::Argument_list()).as_bool());
}

void ResultsetDumper::dump_normal(std::shared_ptr<mysqlsh::mysqlx::SqlResult> result) {
  std::string output;

  do {
    if (result->has_data(shcore::Argument_list()).as_bool())
      dump_records(output);
    else if (_interactive)
      output = get_affected_stats("affectedRowCount", "row");

    // This information output is only printed in interactive mode
    if (_interactive) {
      int warning_count = get_warning_and_execution_time_stats(output);

      _output_handler->print(_output_handler->user_data, output.c_str());

      // Prints the warnings if there were any
      if (warning_count && _show_warnings)
        dump_warnings();
    }
  } while (result->next_data_set(shcore::Argument_list()).as_bool());
}

void ResultsetDumper::dump_normal(std::shared_ptr<mysqlsh::mysqlx::RowResult> result) {
  std::string output;

  dump_records(output);

  // This information output is only printed in interactive mode
  if (_interactive) {
    int warning_count = get_warning_and_execution_time_stats(output);

    _output_handler->print(_output_handler->user_data, output.c_str());

    // Prints the warnings if there were any
    if (warning_count && _show_warnings)
      dump_warnings();
  }
}

void ResultsetDumper::dump_normal(std::shared_ptr<mysqlsh::mysqlx::DocResult> result) {
  std::string output;

  shcore::Value documents = result->fetch_all(shcore::Argument_list());
  shcore::Value::Array_type_ref array_docs = documents.as_array();

  if (array_docs->size()) {
    _output_handler->print_value(_output_handler->user_data, documents, "");

    int row_count = int(array_docs->size());
    output = shcore::str_format("%d %s in set", row_count, (row_count == 1 ? "document" : "documents"));
  } else
    output = "Empty set";

  // This information output is only printed in interactive mode
  if (_interactive) {
    int warning_count = get_warning_and_execution_time_stats(output);

    _output_handler->print(_output_handler->user_data, output.c_str());

    // Prints the warnings if there were any
    if (warning_count && _show_warnings)
      dump_warnings();
  }
}

void ResultsetDumper::dump_normal(std::shared_ptr<mysqlsh::mysqlx::Result> result) {
  // This information output is only printed in interactive mode
  if (_interactive) {
    std::string output = get_affected_stats("affectedItemCount", "item");
    int warning_count = get_warning_and_execution_time_stats(output);

    _output_handler->print(_output_handler->user_data, output.c_str());

    // Prints the warnings if there were any
    if (warning_count && _show_warnings)
      dump_warnings();
  }
}

void ResultsetDumper::dump_tabbed(shcore::Value::Array_type_ref records) {
  std::shared_ptr<shcore::Value::Array_type> metadata = _resultset->get_member("columns").as_array();

  size_t index = 0;
  size_t field_count = metadata->size();
  std::vector<std::string> formats(field_count, "%-");

  // Prints the initial separator line and the column headers
  // TODO: Consider the charset information on the length calculations
  for (index = 0; index < field_count; index++) {
    std::shared_ptr<mysqlsh::Column> column = std::static_pointer_cast<mysqlsh::Column>(metadata->at(index).as_object());
    _output_handler->print(_output_handler->user_data, column->get_column_label().c_str());
    _output_handler->print(_output_handler->user_data, index < (field_count - 1) ? "\t" : "\n");
  }

  // Now prints the records
  for (size_t row_index = 0; row_index < records->size(); row_index++) {
    std::shared_ptr<mysqlsh::Row> row = (*records)[row_index].as_object<mysqlsh::Row>();

    for (size_t field_index = 0; field_index < field_count; field_index++) {
      std::string raw_value = row->get_member(field_index).descr();
      _output_handler->print(_output_handler->user_data, raw_value.c_str());
      _output_handler->print(_output_handler->user_data, field_index < (field_count - 1) ? "\t" : "\n");
    }
  }
}

void ResultsetDumper::dump_vertical(shcore::Value::Array_type_ref records) {
  std::shared_ptr<shcore::Value::Array_type> metadata = _resultset->get_member("columns").as_array();
  std::string star_separator(27, '*');

  // Calculate length of a longest column description, used to right align
  // column descriptions
  std::size_t max_col_len = 0;
  for (size_t col_index = 0; col_index < metadata->size(); col_index++) {
    std::shared_ptr<mysqlsh::Column> column =
        std::static_pointer_cast<mysqlsh::Column>(metadata->at(col_index).as_object());
    max_col_len = std::max(max_col_len,  column->get_column_label().length());
  }

  for (size_t row_index = 0; row_index < records->size(); row_index++) {
    std::string row_header = star_separator + " " + std::to_string(row_index + 1) +
      ". row " + star_separator + "\n";

    _output_handler->print(_output_handler->user_data, row_header.c_str());

    for (size_t col_index = 0; col_index < metadata->size(); col_index++) {
      std::shared_ptr<mysqlsh::Column> column =
          std::static_pointer_cast<mysqlsh::Column>(metadata->at(col_index).as_object());

      std::shared_ptr<mysqlsh::Row> row = records->at(row_index).as_object<mysqlsh::Row>();
      std::string padding(max_col_len - column->get_column_label().size(), ' ');
      std::string value_row = padding + column->get_column_label() + ": " +
          row->get_member(col_index).descr() + "\n";

      _output_handler->print(_output_handler->user_data, value_row.c_str());
    }
  }
}

void ResultsetDumper::dump_table(shcore::Value::Array_type_ref records) {
  std::shared_ptr<shcore::Value::Array_type> metadata = _resultset->get_member("columns").as_array();
  std::vector<uint64_t> max_lengths;
  std::vector<std::string> column_names;
  std::vector<bool> numerics;

  size_t field_count = metadata->size();

  // Updates the max_length array with the maximum length between column name, min column length and column max length
  for (size_t field_index = 0; field_index < field_count; field_index++) {
    std::shared_ptr<mysqlsh::Column> column = std::static_pointer_cast<mysqlsh::Column>(metadata->at(field_index).as_object());

    column_names.push_back(column->get_column_label());
    numerics.push_back(column->is_numeric());

    max_lengths.push_back(0);
    max_lengths[field_index] = std::max<uint64_t>(max_lengths[field_index], column->get_column_label().length());
  }

  // Now updates the length with the real column data lengths
  size_t row_index;
  for (row_index = 0; row_index < records->size(); row_index++) {
    std::shared_ptr<mysqlsh::Row> row = (*records)[row_index].as_object<mysqlsh::Row>();
    for (size_t field_index = 0; field_index < field_count; field_index++)
      max_lengths[field_index] = std::max<uint64_t>(max_lengths[field_index], row->get_member(field_index).descr().length());
  }

  //-----------

  size_t index = 0;
  std::vector<std::string> formats(field_count, "%-");

  // Calculates the max column widths and constructs the separator line.
  std::string separator("+");
  for (index = 0; index < field_count; index++) {
    // Creates the format string to print each field
    formats[index].append(std::to_string(max_lengths[index]));
    if (index == field_count - 1)
      formats[index].append("s |");
    else
      formats[index].append("s | ");

    std::string field_separator(max_lengths[index] + 2, '-');
    field_separator.append("+");
    separator.append(field_separator);
  }
  separator.append("\n");

  // Prints the initial separator line and the column headers
  // TODO: Consider the charset information on the length calculations
  _output_handler->print(_output_handler->user_data, separator.c_str());
  _output_handler->print(_output_handler->user_data, +"| ");
  for (index = 0; index < field_count; index++) {
    std::string data = shcore::str_format(formats[index].c_str(), column_names[index].c_str());
    _output_handler->print(_output_handler->user_data, data.c_str());

    // Once the header is printed, updates the numeric fields formats
    // so they are right aligned
    if (numerics[index])
    formats[index] = formats[index].replace(1, 1, "");
  }
  _output_handler->print(_output_handler->user_data, "\n");
  _output_handler->print(_output_handler->user_data, separator.c_str());

  // Now prints the records
  for (row_index = 0; row_index < records->size(); row_index++) {
    _output_handler->print(_output_handler->user_data, "| ");

    std::shared_ptr<mysqlsh::Row> row = (*records)[row_index].as_object<mysqlsh::Row>();

    for (size_t field_index = 0; field_index < field_count; field_index++) {
      std::string raw_value = row->get_member(field_index).descr();
      std::string data = shcore::str_format(formats[field_index].c_str(), raw_value.c_str());

      _output_handler->print(_output_handler->user_data, data.c_str());
    }
    _output_handler->print(_output_handler->user_data, "\n");
  }

  _output_handler->print(_output_handler->user_data, separator.c_str());
}

std::string ResultsetDumper::get_affected_stats(const std::string& member, const std::string &legend) {
  std::string output;

  // Some queries return -1 since affected rows do not apply to them
  int64_t affected_items = _resultset->get_member(member).as_int();
  //if (affected_items == (uint64_t)-1)
  if (affected_items == -1)
    output = "Query OK";
  else
    // In case of Query OK, prints the actual number of affected rows.
    output = shcore::str_format("Query OK, %lld %s affected", (long long int)affected_items, (affected_items == 1 ? legend : legend + "s").c_str());

  return output;
}

int ResultsetDumper::get_warning_and_execution_time_stats(std::string& output_stats) {
  int warning_count = 0;

  if (_interactive) {
    warning_count = _resultset->get_member("warningCount").as_uint();

    if (warning_count)
      output_stats.append(shcore::str_format(", %d warning%s", warning_count, (warning_count == 1 ? "" : "s")));

    output_stats.append(" ");
    output_stats.append(shcore::str_format("(%s)", _resultset->get_member("executionTime").as_string().c_str()));
    output_stats.append("\n");
  }

  return warning_count;
}

void ResultsetDumper::dump_records(std::string& output_stats) {
  shcore::Value records = _resultset->call("fetchAll", shcore::Argument_list());
  shcore::Value::Array_type_ref array_records = records.as_array();

  if (array_records->size()) {
    // print rows from result, with stats etc
    if (_format == "vertical")
      dump_vertical(array_records);
    else if (_interactive || _format == "table")
      dump_table(array_records);
    else
      dump_tabbed(array_records);

    int row_count = int(array_records->size());
    output_stats = shcore::str_format("%d %s in set", row_count, (row_count == 1 ? "row" : "rows"));
  } else
    output_stats = "Empty set";
}

void ResultsetDumper::dump_warnings(bool classic) {
  shcore::Value warnings = _resultset->get_member("warnings");

  if (warnings) {
    shcore::Value::Array_type_ref warning_list = warnings.as_array();
    size_t index = 0, size = warning_list->size();

    while (index < size) {
      shcore::Value record = warning_list->at(index);
      std::shared_ptr<mysqlsh::Row> row = record.as_object<mysqlsh::Row>();

      std::string code = "code";
      std::string level = "level";
      std::string message = "message";

      if (classic) {
        code = "Code";
        level = "Level";
        message = "Message";
      }

      unsigned long error = row->get_member(code).as_int();

      std::string type = row->get_member(level).as_string();
      std::string msg = row->get_member(message).as_string();
      _output_handler->print(_output_handler->user_data, (shcore::str_format("%s (code %ld): %s\n", type.c_str(), error, msg.c_str())).c_str());

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
return result->next_data_set(shcore::Argument_list());
}*/
