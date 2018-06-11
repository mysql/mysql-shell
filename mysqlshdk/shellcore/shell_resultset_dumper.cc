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
#include <deque>

#include "ext/linenoise-ng/include/linenoise.h"
#include "modules/devapi/mod_mysqlx_resultset.h"
#include "modules/mod_mysql_resultset.h"
#include "mysqlshdk/include/shellcore/base_shell.h"
#include "mysqlshdk/include/shellcore/console.h"
#include "shellcore/interrupt_handler.h"
#include "utils/utils_string.h"

#define MAX_DISPLAY_LENGTH 1024

namespace mysqlsh {

/* Calculates the required buffer size and display size considering:
 * - Some single byte characters may require injection of escaped sequence \\
 * - Some multibyte characters are displayed in the space of a single character
 * - Some multibyte characters are displayed in the space of two characters
 *
 * For the reasons above, the real buffer required to store the formatted text
 * might be bigger than the original length and the space required on screen
 * might be either lower (mb chars) or bigger (injected escapes) than the
 * original length.
 *
 * This function returns a tuple containing:
 * - Real display character count
 * - Real byte size
 */
std::tuple<size_t, size_t> get_utf8_sizes(const char *text, size_t length,
                                          Print_flags flags) {
  size_t char_count = 0;
  size_t byte_count = 0;

  const char *index = text;
  const char *end = index + length;

#ifdef _WIN32
  // By default, we assume no multibyte content on the string and
  // no escaped characters.
  bool is_multibyte = false;
  byte_count = length;
  char_count = length;

  if (length) {
    // Calculates the required size for the buffer
    int required = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text,
                                       length, nullptr, 0);
    // Required would be 0 in case of error, possible errors include:
    // ERROR_INSUFFICIENT_BUFFER. A supplied buffer size was not large enough,
    // or it was incorrectly set to NULL. ERROR_INVALID_FLAGS. The values
    // supplied for flags were not valid. ERROR_INVALID_PARAMETER. Any of the
    // parameter values was invalid. ERROR_NO_UNICODE_TRANSLATION. Invalid
    // Unicode was found in a string.
    //
    // We know ERROR_INSUFFICIENT_BUFFER is not since we were calculating the
    // buffer size We assume ERROR_INVALID_FLAGS and ERROR_INVALID_PARAMETER are
    // not, since the function succeeds in most of the cases. So only posibility
    // is ERROR_NO_UNICODE_TRANSLATION which would be the case i.e. for binary
    // data. In such case the function simply exits returning the original
    // lengths.
    if (required > 0) {
      std::wstring wstr;
      wstr = std::wstring(required, 0);
      required = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text,
                                     length, &wstr[0], required);

      // If the final number of characters is different from the input
      // length, it means there are characters that are multibyte
      if (required != length) {
        is_multibyte = true;

        // Character count will be totally calculated on the loop
        // not only escaped characters
        char_count = 0;
      }

      auto character = wstr.begin();

      // This loop is used to calculates two things
      // 1) The "screen spaces" to be required by the string
      //    - Some are injected spaces
      //    - For multibyte strings, it includes width calculation
      // 2) The injected bytes due to escape handling
      while (character != wstr.end()) {
        if (*character == '\0') {
          if (flags.is_set(Print_flag::PRINT_0_AS_SPC)) {
            // If it's multibyte, length is being calculated so we need
            // to add 1 on this case, otherwise it is already considered
            if (is_multibyte) char_count++;
          } else if (flags.is_set(Print_flag::PRINT_0_AS_ESC)) {
            char_count += is_multibyte ? 2 : 1;
            byte_count++;
          } else {
            // If not multibyte, we need to remove one char since unescaped \0
            // should not occupy space
            if (!is_multibyte) char_count--;
          }
        } else if (flags.is_set(Print_flag::PRINT_CTRL) &&
                   (*character == '\t' || *character == '\n' ||
                    *character == '\\')) {
          char_count += is_multibyte ? 2 : 1;
          byte_count++;
        } else if (is_multibyte) {
          // Get the character width
          int size = getWcwidth(*character);

          // We ignore control characters which may return -1
          // since no character will really remove screen space
          if (size > 0) char_count += size;
        }

        character++;
      }
    }
  }

#else
  std::mblen(NULL, 0);
  while (index < end) {
    int width = std::mblen(index, end - index);

    // handles single byte characters
    if (width == 1) {
      // Controls characters to be printed add one extra char to the output
      if (flags.is_set(Print_flag::PRINT_CTRL) &&
          (*index == '\t' || *index == '\n' || *index == '\\')) {
        char_count++;
        byte_count++;
      }

      // The character itself
      char_count++;
      byte_count++;
      index++;
    } else if (width == 0) {
      // handles the \0 character
      index += 1;

      // Printed as a space
      if (flags.is_set(Print_flag::PRINT_0_AS_SPC)) {
        char_count += 1;
        byte_count += 1;

        // Escape injection to be printed as \\0
      } else if (flags.is_set(Print_flag::PRINT_0_AS_ESC)) {
        char_count += 2;
        byte_count += 2;
      } else {
        // No char_count but byte is needed
        byte_count++;
      }
    } else if (width == -1) {
      // If a so weird character was found, then it makes no sense to continue
      // processing since the final measure will not be accurate anyway, so we
      // return the original data
      // This could be the case on any binary data column.
      char_count = length;
      byte_count = length;
      break;
    } else {
      // Multibyte characters handling
      // We need to find out whether they are printed in single or double space
      wchar_t mbchar;
      int size = 0;
      if (std::mbtowc(&mbchar, index, width) > 0) {
        size = getWcwidth(mbchar);
        // We ignore control characters which may return -1
        // since no character will really remove screen space
        if (size < 0) size = 0;
      } else {
        size = 1;
      }

      char_count += size;
      byte_count += width;
      index += width;
    }
  }
#endif

  std::tuple<size_t, size_t> ret_val{char_count, byte_count};

  return ret_val;
}

enum class ResultFormat { VERTICAL, TABBED, TABLE };

class Field_formatter {
 public:
  Field_formatter(ResultFormat format, const mysqlsh::Column &column)
      : m_buffer(nullptr),
        m_max_display_length(0),
        m_max_buffer_length(0),
        m_max_mb_holes(0),
        m_format(format) {
    m_zerofill = column.is_zerofill() ? column.get_length() : 0;
    m_binary = column.is_binary();

    switch (m_format) {
      case ResultFormat::TABBED:
        m_flags = Print_flags(Print_flag::PRINT_0_AS_ESC);
        m_flags.set(Print_flag::PRINT_CTRL);
        m_align_right = false;
        break;
      case ResultFormat::VERTICAL:
        m_flags = Print_flags(Print_flag::PRINT_0_AS_SPC);
        m_align_right = false;
        break;
      case ResultFormat::TABLE:
        m_flags = Print_flags(Print_flag::PRINT_0_AS_SPC);
        m_align_right = column.is_zerofill() || column.is_numeric();

        // Gets the column name display/buffer sizes
        auto col_sizes =
            get_utf8_sizes(column.get_column_label().c_str(),
                           column.get_column_label().length(),
                           Print_flags(Print_flag::PRINT_0_AS_ESC));

        m_max_mb_holes = std::get<1>(col_sizes) - std::get<0>(col_sizes);
        m_max_display_length = std::max(std::get<0>(col_sizes), m_zerofill);
        m_max_buffer_length = std::get<1>(col_sizes);
        break;
    }
  }

  Field_formatter(Field_formatter &&other) {
    this->operator=(std::move(other));
  }

  void operator=(Field_formatter &&other) {
    m_allocated = other.m_allocated;
    m_zerofill = other.m_zerofill;
    m_align_right = other.m_align_right;
    m_buffer = std::move(other.m_buffer);
    m_binary = other.m_binary;

    other.m_buffer = nullptr;
    other.m_allocated = 0;

    m_max_display_length = other.m_max_display_length;
    m_max_buffer_length = other.m_max_buffer_length;
    m_max_mb_holes = other.m_max_mb_holes;

    // Length cache for each data to be printed with this formatter
    m_display_lengths = std::move(other.m_display_lengths);
    m_buffer_lengths = std::move(other.m_buffer_lengths);

    m_format = other.m_format;
    m_flags = other.m_flags;
  }

  void process(const shcore::Value &value) {
    // This function is meant to be called only for tables
    assert(m_format == ResultFormat::TABLE);

    auto fsizes =
        get_utf8_sizes(value.descr().c_str(), value.descr().length(), m_flags);

    size_t dlength = std::get<0>(fsizes);
    size_t blength = std::get<1>(fsizes);

    m_max_mb_holes = std::max<size_t>(m_max_mb_holes, blength - dlength);

    m_max_display_length = std::max<size_t>(m_max_display_length, dlength);

    m_max_buffer_length = std::max<size_t>(m_max_buffer_length, blength);

    m_display_lengths.push_back(dlength);
    m_buffer_lengths.push_back(blength);

    m_max_mb_holes = std::max<size_t>(m_max_mb_holes, blength - dlength);
  }

  ~Field_formatter() {}

  bool put(const shcore::Value &value) {
    reset();

    switch (value.type) {
      case shcore::String: {
        return append(value.value.s->c_str(), value.value.s->length());
        break;
      }
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
        if (m_zerofill > tmp.length()) {
          tmp = std::string(m_zerofill - tmp.length(), '0').append(tmp);

          // Updates the display length with the new size
          if (m_format == ResultFormat::TABLE) {
            m_display_lengths[0] = tmp.length();
          }
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

  const char *c_str() const { return m_buffer.get(); }
  size_t get_max_display_length() const { return m_max_display_length; }
  size_t get_max_buffer_length() const { return m_max_buffer_length; }

 private:
  std::unique_ptr<char> m_buffer;
  size_t m_allocated;
  size_t m_zerofill;
  bool m_binary;
  bool m_align_right;

  size_t m_max_display_length;
  size_t m_max_buffer_length;
  size_t m_max_mb_holes;

  // Length cache for each data to be printed with this formatter
  std::deque<size_t> m_display_lengths;
  std::deque<size_t> m_buffer_lengths;

  ResultFormat m_format;
  Print_flags m_flags;

  void reset() {
    // sets the buffer only once
    if (!m_buffer) {
      if (m_format == ResultFormat::TABLE) {
        m_allocated = std::max<size_t>(m_max_display_length,
                                       m_max_buffer_length + m_max_mb_holes) +
                      1;

        if (m_allocated > MAX_DISPLAY_LENGTH) m_allocated = MAX_DISPLAY_LENGTH;
      } else {
        m_allocated = MAX_DISPLAY_LENGTH;
      }

      m_buffer.reset(new char[m_allocated]);
    }

    memset(m_buffer.get(), ' ', m_allocated);
  }

  bool append(const char *text, size_t length) {
    size_t display_size;
    size_t buffer_size;
    if (m_format == ResultFormat::TABLE) {
      display_size = m_display_lengths.front();
      buffer_size = m_buffer_lengths.front();
      m_display_lengths.pop_front();
      m_buffer_lengths.pop_front();
    } else {
      auto fsizes = get_utf8_sizes(text, length, m_flags);
      display_size = std::get<0>(fsizes);
      buffer_size = std::get<1>(fsizes);
    }

    if (buffer_size > m_allocated) return false;

    size_t next_index = 0;
    if (m_format == ResultFormat::TABLE) {
      if (m_align_right && m_max_display_length > display_size) {
        next_index = m_max_display_length - display_size;
      }
    }

    auto buffer = m_buffer.get();
    for (size_t index = 0; index < length; index++) {
      if (m_flags.is_set(Print_flag::PRINT_0_AS_ESC) && text[index] == '\0') {
        buffer[next_index++] = '\\';
        buffer[next_index++] = '0';
      } else if (m_flags.is_set(Print_flag::PRINT_0_AS_SPC) &&
                 text[index] == '\0') {
        buffer[next_index++] = ' ';
      } else if (m_flags.is_set(Print_flag::PRINT_CTRL) &&
                 text[index] == '\t') {
        buffer[next_index++] = '\\';
        buffer[next_index++] = 't';
      } else if (m_flags.is_set(Print_flag::PRINT_CTRL) &&
                 text[index] == '\n') {
        buffer[next_index++] = '\\';
        buffer[next_index++] = 'n';
      } else if (m_flags.is_set(Print_flag::PRINT_CTRL) &&
                 text[index] == '\\') {
        buffer[next_index++] = '\\';
        buffer[next_index++] = '\\';
      } else {
        buffer[next_index++] = text[index];
      }
    }

    if (m_format == ResultFormat::TABLE) {
      if (buffer_size > display_size) {
        // It means some multibyte characters were found, and so we need to
        // truncate the buffer adding the 'lost' characters
        buffer[m_max_display_length + (buffer_size - display_size)] = 0;
      } else {
        // Ohterwise, we truncate at _column_width
        buffer[m_max_display_length] = 0;
      }
    } else {
      buffer[next_index] = 0;
    }

    return true;
  }
};

ResultsetDumper::ResultsetDumper(
    std::shared_ptr<mysqlsh::ShellBaseResult> target, bool buffer_data)
    : _resultset(target), _buffer_data(buffer_data), _cancelled(false) {
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
    mysqlsh::current_console()->print(
        "Result printing interrupted, rows may be missing from the output.\n");

  // Restores data
  if (_buffer_data) _resultset->rewind();
}

void ResultsetDumper::dump_json() {
  shcore::Value resultset(
      std::static_pointer_cast<shcore::Object_bridge>(_resultset));

  mysqlsh::current_console()->print_value(resultset, "");
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

      mysqlsh::current_console()->print(output);
    }

    std::string info = result->get_member("info").as_string();
    if (!info.empty()) {
      info = "\n" + info + "\n";
      mysqlsh::current_console()->print(info);
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

      mysqlsh::current_console()->print(output);

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

    mysqlsh::current_console()->print(output);

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
    mysqlsh::current_console()->print_value(documents, "");

    size_t row_count = array_docs->size();
    output = shcore::str_format("%zu %s in set", row_count,
                                (row_count == 1 ? "document" : "documents"));
  } else {
    output = "Empty set";
  }
  // This information output is only printed in interactive mode
  if (_interactive) {
    int warning_count = get_warning_and_execution_time_stats(output);

    mysqlsh::current_console()->print(output);

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

    mysqlsh::current_console()->print(output);

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

  auto console = mysqlsh::current_console();

  // Prints the initial separator line and the column headers
  // TODO: Consider the charset information on the length calculations
  for (index = 0; index < field_count; index++) {
    auto column = std::static_pointer_cast<mysqlsh::Column>(
        metadata->at(index).as_object());
    fmt.emplace_back(ResultFormat::TABBED, *column);
    console->print(column->get_column_label().c_str());
    console->print(index < (field_count - 1) ? "\t" : "\n");
  }

  // Now prints the records
  size_t row_index;
  for (row_index = 0; row_index < records->size() && !_cancelled; row_index++) {
    auto row = (*records)[row_index].as_object<mysqlsh::Row>();

    for (size_t field_index = 0; field_index < field_count; field_index++) {
      shcore::Value value = row->get_member(field_index);

      if (fmt[field_index].put(value)) {
        console->print(fmt[field_index].c_str());
      } else {
        assert(value.type == shcore::String);
        console->print(*value.value.s);
      }
      console->print(field_index < (field_count - 1) ? "\t" : "\n");
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
    fmt.emplace_back(ResultFormat::VERTICAL, *column);
  }

  auto console = mysqlsh::current_console();

  for (size_t row_index = 0; row_index < records->size() && !_cancelled;
       row_index++) {
    std::string row_header = star_separator + " " +
                             std::to_string(row_index + 1) + ". row " +
                             star_separator + "\n";

    console->print(row_header);

    for (size_t col_index = 0; col_index < metadata->size(); col_index++) {
      auto column = std::static_pointer_cast<mysqlsh::Column>(
          metadata->at(col_index).as_object());

      auto row = records->at(row_index).as_object<mysqlsh::Row>();
      std::string padding(max_col_len - column->get_column_label().size(), ' ');
      std::string label = padding + column->get_column_label() + ": ";

      console->print(label);
      shcore::Value value = row->get_member(col_index);
      if (fmt[col_index].put(value)) {
        console->print(fmt[col_index].c_str());
      } else {
        assert(value.type == shcore::String);
        console->print(*value.value.s);
      }
      console->print("\n");
    }

    if (_cancelled) return row_index;
  }
  return records->size();
}

size_t ResultsetDumper::dump_table(shcore::Value::Array_type_ref records) {
  std::shared_ptr<shcore::Value::Array_type> metadata =
      _resultset->get_member("columns").as_array();
  std::vector<std::string> columns;
  std::vector<Field_formatter> fmt;

  size_t field_count = metadata->size();
  if (field_count == 0) return 0;

  // Updates the max_length array with the maximum length between column name,
  // min column length and column max length
  for (size_t field_index = 0; field_index < field_count; field_index++) {
    auto column = std::static_pointer_cast<mysqlsh::Column>(
        metadata->at(field_index).as_object());

    columns.push_back(column->get_column_label());
    fmt.emplace_back(ResultFormat::TABLE, *column);
  }

  size_t row_index;
  for (row_index = 0; row_index < records->size() && !_cancelled; row_index++) {
    auto row = (*records)[row_index].as_object<mysqlsh::Row>();
    for (size_t field_index = 0; field_index < field_count; field_index++) {
      fmt[field_index].process(row->get_member(field_index));
    }
  }

  if (_cancelled) return 0;

  //-----------

  size_t index = 0;

  std::string separator("+");
  for (index = 0; index < field_count; index++) {
    std::string field_separator(fmt[index].get_max_display_length() + 2, '-');
    field_separator.append("+");
    separator.append(field_separator);
  }
  separator.append("\n");

  // Prints the initial separator line and the column headers
  // TODO: Consider the charset information on the length calculations
  auto console = mysqlsh::current_console();
  console->print(separator);
  console->print("| ");
  for (index = 0; index < field_count; index++) {
    std::string format = "%-";
    format.append(std::to_string(fmt[index].get_max_display_length()));
    format.append((index == field_count - 1) ? "s |\n" : "s | ");
    auto column = std::static_pointer_cast<mysqlsh::Column>(
        metadata->at(index).as_object());
    console->print(
        shcore::str_format(format.c_str(), column->get_column_label().c_str()));
  }
  console->print(separator);

  // Now prints the records
  for (row_index = 0; row_index < records->size() && !_cancelled; row_index++) {
    console->print("| ");

    auto row = (*records)[row_index].as_object<mysqlsh::Row>();

    for (size_t field_index = 0; field_index < field_count; field_index++) {
      shcore::Value value(row->get_member(field_index));
      if (fmt[field_index].put(value)) {
        console->print(fmt[field_index].c_str());
      } else {
        assert(value.type == shcore::String);
        console->print(*value.value.s);
      }
      if (field_index < field_count - 1) console->print(" | ");
    }
    console->print(" |\n");
  }

  console->print(separator.c_str());

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
      mysqlsh::current_console()->print((shcore::str_format(
          "%s (code %ld): %s\n", type.c_str(), error, msg.c_str())));

      index++;
    }
  }
}

}  // namespace mysqlsh
