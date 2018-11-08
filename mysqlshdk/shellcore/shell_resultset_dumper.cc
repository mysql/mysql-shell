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
#include "mysqlshdk/include/shellcore/base_shell.h"
#include "mysqlshdk/include/shellcore/console.h"
#include "mysqlshdk/libs/db/column.h"
#include "mysqlshdk/libs/utils/dtoa.h"
#include "mysqlshdk/libs/utils/strformat.h"
#include "mysqlshdk/libs/utils/utils_json.h"
#include "shellcore/interrupt_handler.h"
#include "utils/utils_general.h"
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
  Field_formatter(ResultFormat format, const mysqlshdk::db::Column &column)
      : m_buffer(nullptr),
        m_max_display_length(0),
        m_max_buffer_length(0),
        m_max_mb_holes(0),
        m_format(format),
        m_type(column.get_type()),
        m_is_numeric(column.is_numeric()) {
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
    m_type = other.m_type;

    m_is_numeric = other.m_is_numeric;
  }

  void process(const mysqlshdk::db::IRow *row, size_t index) {
    // This function is meant to be called only for tables
    assert(m_format == ResultFormat::TABLE);
    size_t dlength{0};
    size_t blength{0};

    if (row->is_null(index)) {
      dlength = blength = 4;
    } else if (m_is_numeric) {
      auto str = get_number_string(row, index);
      dlength = blength = str.length();
    } else if (m_type == mysqlshdk::db::Type::Bytes) {
      auto data = row->get_string_data(index);
      auto fsizes = get_utf8_sizes(data.first, data.second, m_flags);
      dlength = std::get<0>(fsizes);
      blength = std::get<1>(fsizes);
    } else {
      auto data = row->get_as_string(index);
      auto fsizes = get_utf8_sizes(data.c_str(), data.length(), m_flags);
      dlength = std::get<0>(fsizes);
      blength = std::get<1>(fsizes);
    }

    m_max_mb_holes = std::max<size_t>(m_max_mb_holes, blength - dlength);

    m_max_display_length = std::max<size_t>(m_max_display_length, dlength);

    m_max_buffer_length = std::max<size_t>(m_max_buffer_length, blength);

    m_display_lengths.push_back(dlength);
    m_buffer_lengths.push_back(blength);

    m_max_mb_holes = std::max<size_t>(m_max_mb_holes, blength - dlength);
  }

  ~Field_formatter() {}

  std::string get_number_string(const mysqlshdk::db::IRow *row, size_t index) {
    std::string tmp;
    if (m_type == mysqlshdk::db::Type::Float) {
      auto value = row->get_float(index);
      char buffer[32];
      size_t len;
      len = my_gcvt(value, MY_GCVT_ARG_FLOAT, sizeof(buffer) - 1, buffer, NULL);
      tmp.assign(buffer, len);
    } else if (m_type == mysqlshdk::db::Type::Integer) {
      return std::to_string(row->get_int(index));
    } else if (m_type == mysqlshdk::db::Type::UInteger) {
      return std::to_string(row->get_uint(index));
    } else if (m_type == mysqlshdk::db::Type::Double) {
      auto value = row->get_double(index);
      char buffer[32];
      size_t len;
      len =
          my_gcvt(value, MY_GCVT_ARG_DOUBLE, sizeof(buffer) - 1, buffer, NULL);
      tmp.assign(buffer, len);
    } else {
      return row->get_as_string(index);
    }

    return tmp;
  }

  bool put(const mysqlshdk::db::IRow *row, size_t index) {
    reset();

    if (row->is_null(index)) {
      append("NULL", 4);
    } else if (m_is_numeric) {
      std::string tmp = get_number_string(row, index);

      if (m_zerofill > tmp.length()) {
        tmp = std::string(m_zerofill - tmp.length(), '0').append(tmp);

        // Updates the display length with the new size
        if (m_format == ResultFormat::TABLE) {
          m_display_lengths[0] = tmp.length();
        }
      }
      append(tmp.data(), tmp.length());
    } else if (m_type == mysqlshdk::db::Type::Bytes) {
      auto data = row->get_string_data(index);
      return append(data.first, data.second);
    } else if (m_type == mysqlshdk::db::Type::Bit) {
      auto value = row->get_bit(index);
      auto tmp = shcore::lexical_cast<std::string>(value);
      return append(tmp.data(), tmp.length());
    } else {
      auto data = row->get_as_string(index);
      append(data.c_str(), data.size());
    }

    return true;
  }

  const char *c_str() const { return m_buffer.get(); }
  size_t get_max_display_length() const { return m_max_display_length; }
  size_t get_max_buffer_length() const { return m_max_buffer_length; }

 private:
  std::unique_ptr<char[]> m_buffer;
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
  mysqlshdk::db::Type m_type;
  bool m_is_numeric;

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

Resultset_dumper::Resultset_dumper(mysqlshdk::db::IResult *target,
                                   bool buffer_data)
    : _rset(target), _buffer_data(buffer_data), _cancelled(false) {
  auto opts = mysqlsh::current_shell_options()->get();
  _format = opts.wrap_json == "off" ? opts.result_format : opts.wrap_json;
  _interactive = opts.interactive;
  _show_warnings = opts.show_warnings;
}

void Resultset_dumper::dump(const std::string &item_label, bool is_query,
                            bool is_doc_result) {
  std::string output;
  shcore::Interrupt_handler intr([this]() {
    _cancelled = true;
    return true;
  });

  if (_format.find("json") == 0)
    dump_json(item_label, is_doc_result);
  else
    do {
      size_t count = 0;
      if (_rset->has_resultset()) {
        // Data requires to be buffered on table format because it will be
        // traversec once for proper formatting and once for printing it
        if (_buffer_data || _format == "table") _rset->buffer();

        if (is_doc_result)
          count = dump_documents();
        else {
          // print rows from result, with stats etc
          if (_format == "vertical")
            count = dump_vertical();
          else if (_format == "table")
            count = dump_table();
          else
            count = dump_tabbed();
        }

        if (count)
          output =
              shcore::str_format("%zu %s%s in set", count, item_label.c_str(),
                                 (count == 1 ? "" : "s"));
        else
          output = "Empty set";
      }

      // Starts only make sense on non read only operations
      else if (_interactive && !is_query)
        output = get_affected_stats(item_label);

      // This information output is only printed in interactive mode
      int warning_count = 0;
      if (_interactive) {
        warning_count = get_warning_and_execution_time_stats(output);

        mysqlsh::current_console()->print(output);
      }

      if (!is_query) {
        std::string info = _rset->get_info();
        if (!info.empty()) {
          info = "\n" + info + "\n";
          mysqlsh::current_console()->print(info);
        }
      }

      // Prints the warnings if there were any
      if (warning_count && _show_warnings) dump_warnings();
    } while (_rset->next_resultset() && !_cancelled);

  if (_cancelled)
    mysqlsh::current_console()->println(
        "Result printing interrupted, rows may be missing from the output.");

  // Restores data
  if (_buffer_data) _rset->rewind();
}

size_t Resultset_dumper::dump_documents() {
  auto metadata = _rset->get_metadata();
  auto row = _rset->fetch_one();
  size_t row_count = 0;

  if (!row) return row_count;

  // When dumping a collection, format should be json unless json/raw is
  // specified
  shcore::JSON_dumper dumper(_format != "json/raw");

  dumper.start_array();

  while (row) {
    dumper.append_json(row->get_string(0));

    row_count++;
    row = _rset->fetch_one();
  }

  dumper.end_array();

  auto console = mysqlsh::current_console();

  console->raw_print(dumper.str(), mysqlsh::Output_stream::STDOUT, false);
  console->raw_print("\n", mysqlsh::Output_stream::STDOUT, false);

  return row_count;
}

size_t Resultset_dumper::dump_tabbed() {
  auto metadata = _rset->get_metadata();
  auto row = _rset->fetch_one();
  size_t row_index = 0;

  if (!row) return row_index;

  size_t index = 0;
  size_t field_count = metadata.size();
  std::vector<std::string> formats(field_count, "%-");
  std::vector<Field_formatter> fmt;

  auto console = mysqlsh::current_console();

  // Prints the initial separator line and the column headers
  for (index = 0; index < field_count; index++) {
    auto column = metadata[index];

    fmt.emplace_back(ResultFormat::TABBED, column);
    console->print(column.get_column_label().c_str());
    console->print(index < (field_count - 1) ? "\t" : "\n");
  }

  // Now prints the records
  while (row && !_cancelled) {
    for (size_t field_index = 0; field_index < field_count; field_index++) {
      auto column = metadata[field_index];
      if (fmt[field_index].put(row, field_index)) {
        console->print(fmt[field_index].c_str());
      } else {
        mysqlshdk::db::is_string_type(column.get_type());
        console->print(row->get_string(field_index));
      }
      console->print(field_index < (field_count - 1) ? "\t" : "\n");
    }

    row = _rset->fetch_one();
    row_index++;
  }

  return row_index;
}

size_t Resultset_dumper::dump_vertical() {
  auto metadata = _rset->get_metadata();

  std::string star_separator(27, '*');

  std::vector<Field_formatter> fmt;

  // Calculate length of a longest column description, used to right align
  // column descriptions
  std::size_t max_col_len = 0;
  for (const auto &column : metadata) {
    max_col_len = std::max(max_col_len, column.get_column_label().length());
    fmt.emplace_back(ResultFormat::VERTICAL, column);
  }
  auto console = mysqlsh::current_console();

  auto row = _rset->fetch_one();
  size_t row_index = 0;
  while (row) {
    std::string row_header = star_separator + " " +
                             std::to_string(row_index + 1) + ". row " +
                             star_separator + "\n";

    console->print(row_header);

    for (size_t col_index = 0; col_index < metadata.size(); col_index++) {
      auto column = metadata[col_index];

      std::string padding(max_col_len - column.get_column_label().size(), ' ');
      std::string label = padding + column.get_column_label() + ": ";

      console->print(label);
      if (fmt[col_index].put(row, col_index)) {
        console->print(fmt[col_index].c_str());
      } else {
        assert(mysqlshdk::db::is_string_type(column.get_type()));
        console->print(row->get_string(col_index));
      }
      console->raw_print("\n", mysqlsh::Output_stream::STDOUT, false);
    }

    row = _rset->fetch_one();
    row_index++;

    if (_cancelled) return row_index;
  }
  return row_index;
}

size_t Resultset_dumper::dump_json(const std::string &item_label,
                                   bool is_doc_result) {
  shcore::JSON_dumper dumper(_format == "json");

  dumper.start_object();
  dumper.append_string("hasData");
  dumper.append_bool(_rset->has_resultset());

  dumper.append_string(item_label + "s");
  dumper.start_array();

  size_t row_count = 0;
  if (_rset->has_resultset()) {
    auto metadata = _rset->get_metadata();
    auto row = _rset->fetch_one();
    while (row) {
      if (is_doc_result) {
        dumper.append_json(row->get_string(0));
      } else {
        dumper.start_object();

        for (size_t col_index = 0; col_index < metadata.size(); col_index++) {
          auto column = metadata[col_index];

          dumper.append_string(column.get_column_label());
          auto type = column.get_type();
          if (row->is_null(col_index)) {
            dumper.append_null();
          } else if (mysqlshdk::db::is_string_type(type)) {
            if (type == mysqlshdk::db::Type::Json) {
              dumper.append_json(row->get_string(col_index));
            } else if (type == mysqlshdk::db::Type::Bytes) {
              auto data = row->get_string_data(col_index);
              dumper.append_string(data.first, data.second);
            } else {
              auto data = row->get_as_string(col_index);
              dumper.append_string(data.c_str(), data.length());
            }
          } else if (type == mysqlshdk::db::Type::Integer) {
            dumper.append_int64(row->get_int(col_index));
          } else if (type == mysqlshdk::db::Type::UInteger) {
            dumper.append_uint64(row->get_uint(col_index));
          } else if (type == mysqlshdk::db::Type::Float) {
            dumper.append_float(row->get_float(col_index));
          } else if (type == mysqlshdk::db::Type::Double) {
            dumper.append_float(row->get_double(col_index));
          } else if (type == mysqlshdk::db::Type::Decimal) {
            dumper.append_float(row->get_float(col_index));
          } else if (type == mysqlshdk::db::Type::Bit) {
            dumper.append_int64(row->get_bit(col_index));
          }
        }
        dumper.end_object();
      }
      row_count++;
      row = _rset->fetch_one();
    }
  }

  dumper.end_array();

  dumper.append_string("executionTime");
  dumper.append_string(
      mysqlshdk::utils::format_seconds(_rset->get_execution_time()));
  if (!is_doc_result) {
    dumper.append_string("affectedRowCount");
    dumper.append_uint64(_rset->get_affected_row_count());
  }
  dumper.append_string("affectedItemsCount");
  dumper.append_uint64(_rset->get_affected_row_count());
  dumper.append_string("warningCount");
  dumper.append_int64(_rset->get_warning_count());
  dumper.append_string("warningsCount");
  dumper.append_uint64(_rset->get_warning_count());
  dumper.append_string("warnings");
  dumper.start_array();
  auto warning = _rset->fetch_one_warning();
  while (warning) {
    std::string level;
    switch (warning->level) {
      case mysqlshdk::db::Warning::Level::Note:
        level = "Note";
        break;
      case mysqlshdk::db::Warning::Level::Warn:
        level = "Warning";
        break;
      case mysqlshdk::db::Warning::Level::Error:
        level = "Error";
        break;
    }
    dumper.start_object();
    dumper.append_string("Level", level);
    dumper.append_string("Code");
    dumper.append_int(warning->code);
    dumper.append_string("Message", warning->msg);
    dumper.end_object();
    warning = _rset->fetch_one_warning();
  }
  dumper.end_array();

  dumper.append_string("info");
  dumper.append_string(_rset->get_info());

  dumper.append_string("autoIncrementValue");
  dumper.append_int64(_rset->get_auto_increment_value());

  dumper.end_object();

  auto console = mysqlsh::current_console();

  console->raw_print(dumper.str(), mysqlsh::Output_stream::STDOUT, false);
  console->raw_print("\n", mysqlsh::Output_stream::STDOUT, false);

  return row_count;
}

size_t Resultset_dumper::dump_table() {
  auto metadata = _rset->get_metadata();

  std::vector<std::string> columns;
  std::vector<Field_formatter> fmt;

  size_t field_count = metadata.size();
  if (field_count == 0) return 0;

  // Updates the max_length array with the maximum length between column name,
  // min column length and column max length
  for (size_t field_index = 0; field_index < field_count; field_index++) {
    auto column = metadata[field_index];

    columns.push_back(column.get_column_label());
    fmt.emplace_back(ResultFormat::TABLE, column);
  }

  std::vector<const mysqlshdk::db::IRow *> records;
  auto row = _rset->fetch_one();
  while (row && !_cancelled) {
    records.push_back(row);
    for (size_t field_index = 0; field_index < field_count; field_index++) {
      fmt[field_index].process(row, field_index);
    }
    row = _rset->fetch_one();
  }

  _rset->rewind();

  if (_cancelled || records.empty()) return 0;

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
  auto console = mysqlsh::current_console();
  console->print(separator);
  console->print("| ");
  for (index = 0; index < field_count; index++) {
    std::string format = "%-";
    format.append(std::to_string(fmt[index].get_max_display_length()));
    format.append((index == field_count - 1) ? "s |\n" : "s | ");
    auto column = metadata[index];
    console->print(
        shcore::str_format(format.c_str(), column.get_column_label().c_str()));
  }
  console->print(separator);

  // Now prints the records
  row = _rset->fetch_one();
  while (row && !_cancelled) {
    console->print("| ");

    for (size_t field_index = 0; field_index < field_count; field_index++) {
      if (fmt[field_index].put(row, field_index)) {
        console->print(fmt[field_index].c_str());
      } else {
        assert(mysqlshdk::db::is_string_type(metadata[field_index].get_type()));
        console->print(row->get_as_string(field_index));
      }
      if (field_index < field_count - 1) console->print(" | ");
    }
    console->print(" |\n");
    row = _rset->fetch_one();
  }

  _rset->rewind();

  console->print(separator.c_str());

  return records.size();
}

std::string Resultset_dumper::get_affected_stats(
    const std::string &item_label) {
  std::string output;

  // Some queries return -1 since affected rows do not apply to them
  int64_t affected_items = _rset->get_affected_row_count();

  if (affected_items == -1)
    output = "Query OK";
  else
    // In case of Query OK, prints the actual number of affected rows.
    output = shcore::str_format(
        "Query OK, %" PRId64 " %s affected", affected_items,
        (affected_items == 1 ? item_label : item_label + "s").c_str());

  return output;
}

int Resultset_dumper::get_warning_and_execution_time_stats(
    std::string &output_stats) {
  int warning_count = 0;

  if (_interactive) {
    warning_count = _rset->get_warning_count();

    if (warning_count)
      output_stats.append(shcore::str_format(", %d warning%s", warning_count,
                                             (warning_count == 1 ? "" : "s")));

    output_stats.append(" ");
    output_stats.append(shcore::str_format(
        "(%s)",
        mysqlshdk::utils::format_seconds(_rset->get_execution_time()).c_str()));
    output_stats.append("\n");
  }

  return warning_count;
}

void Resultset_dumper::dump_warnings() {
  if (_rset->get_warning_count()) {
    auto warning = _rset->fetch_one_warning();
    while (warning && !_cancelled) {
      std::string type;
      switch (warning->level) {
        case mysqlshdk::db::Warning::Level::Note:
          type = "Note";
          break;
        case mysqlshdk::db::Warning::Level::Warn:
          type = "Warning";
          break;
        case mysqlshdk::db::Warning::Level::Error:
          type = "Error";
          break;
      }

      mysqlsh::current_console()->print(
          (shcore::str_format("%s (code %d): %s\n", type.c_str(), warning->code,
                              warning->msg.c_str())));

      warning = _rset->fetch_one_warning();
    }
  }
}

}  // namespace mysqlsh
