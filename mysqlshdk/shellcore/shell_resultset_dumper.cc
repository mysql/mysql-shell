/*
 * Copyright (c) 2014, 2022, Oracle and/or its affiliates.
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
#include "mysqlshdk/libs/db/row_copy.h"
#include "mysqlshdk/libs/utils/dtoa.h"
#include "mysqlshdk/libs/utils/strformat.h"
#include "mysqlshdk/libs/utils/utils_encoding.h"  // base64 encoding utilities
#include "mysqlshdk/libs/utils/utils_json.h"
#include "shellcore/interrupt_handler.h"
#include "utils/utils_general.h"
#include "utils/utils_string.h"

#define MAX_DISPLAY_LENGTH 1024

// max # of rows to pre-fetch when dumping resultsets with table formatting,
// in order to calculate column widths
static constexpr const int k_pre_fetch_result_rows = 1000;

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

namespace {

enum class ResultFormat { VERTICAL, TABBED, TABLE };

class Field_formatter {
 public:
  Field_formatter(ResultFormat format, const mysqlshdk::db::Column &column)
      : m_allocated(0),
        m_max_display_length(0),
        m_max_buffer_length(0),
        m_max_mb_holes(0),
        m_format(format),
        m_type(column.get_type()),
        m_is_numeric(column.is_numeric()) {
    m_zerofill = column.is_zerofill() ? column.get_length() : 0;

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
    } else if (m_type == mysqlshdk::db::Type::Bit) {
      auto value = row->get_bit(index);
      auto str = shcore::lexical_cast<std::string>(value);
      dlength = blength = str.length();
    } else if (m_type == mysqlshdk::db::Type::Bytes) {
      // TODO (anyone): Implement support for --skip-binary-as-hex
      auto data = row->get_string_data(index);
      dlength = blength = 2 + data.second * 2;
    } else {
      auto data = row->get_as_string(index);
      auto fsizes = get_utf8_sizes(data.c_str(), data.length(), m_flags);
      dlength = std::get<0>(fsizes);
      blength = std::get<1>(fsizes);
    }

    m_max_mb_holes = std::max<size_t>(m_max_mb_holes, blength - dlength);
    m_max_display_length = std::max<size_t>(m_max_display_length, dlength);
    m_max_buffer_length = std::max<size_t>(m_max_buffer_length, blength);
  }

  ~Field_formatter() {}

  std::string get_number_string(const mysqlshdk::db::IRow *row, size_t index) {
    if (m_type == mysqlshdk::db::Type::Float) {
      return shcore::ftoa(row->get_float(index));
    } else if (m_type == mysqlshdk::db::Type::Integer) {
      return std::to_string(row->get_int(index));
    } else if (m_type == mysqlshdk::db::Type::UInteger) {
      return std::to_string(row->get_uint(index));
    } else if (m_type == mysqlshdk::db::Type::Double) {
      return shcore::dtoa(row->get_double(index));
    } else {
      return row->get_as_string(index);
    }
  }

  bool put(const mysqlshdk::db::IRow *row, size_t index) {
    reset();

    std::string tmp;
    const char *data;
    size_t length;

    size_t display_size;
    size_t buffer_size;

    if (row->is_null(index)) {
      data = "NULL";
      display_size = buffer_size = length = 4;
    } else if (m_is_numeric) {
      tmp = get_number_string(row, index);

      if (m_zerofill > tmp.length()) {
        tmp = std::string(m_zerofill - tmp.length(), '0').append(tmp);
      }
      data = tmp.data();
      display_size = buffer_size = length = tmp.length();
    } else if (m_type == mysqlshdk::db::Type::Bit) {
      auto value = row->get_bit(index);
      tmp = shcore::lexical_cast<std::string>(value);
      data = tmp.data();
      display_size = buffer_size = length = tmp.length();
    } else if (m_type == mysqlshdk::db::Type::Bytes) {
      std::tie(data, length) = row->get_string_data(index);

      tmp = shcore::string_to_hex(data, length);
      data = tmp.data();
      length = tmp.size();

      std::tie(display_size, buffer_size) =
          get_utf8_sizes(data, length, m_flags);
    } else {
      tmp = row->get_as_string(index);
      data = tmp.data();
      length = tmp.length();

      std::tie(display_size, buffer_size) =
          get_utf8_sizes(data, length, m_flags);
    }

    if (!append(data, length, display_size, buffer_size)) {
      if (m_is_numeric || m_type == mysqlshdk::db::Type::Bit) {
        // if a number is larger than expected (e.g. floating pt with lots of
        // decimals)
        m_buffer.assign(data, length);
      } else {
        return false;
      }
    }
    return true;
  }

  const std::string &str() const { return m_buffer; }
  size_t get_max_display_length() const { return m_max_display_length; }
  size_t get_max_buffer_length() const { return m_max_buffer_length; }

 private:
  std::string m_buffer;
  size_t m_allocated;
  size_t m_zerofill;
  bool m_align_right;

  size_t m_max_display_length;
  size_t m_max_buffer_length;
  size_t m_max_mb_holes;

  ResultFormat m_format;
  Print_flags m_flags;
  mysqlshdk::db::Type m_type;
  bool m_is_numeric;

  void reset() {
    // sets the buffer only once
    if (m_buffer.empty()) {
      if (m_format == ResultFormat::TABLE) {
        m_allocated = std::max<size_t>(m_max_display_length,
                                       m_max_buffer_length + m_max_mb_holes);

        if (m_allocated > MAX_DISPLAY_LENGTH) m_allocated = MAX_DISPLAY_LENGTH;
      } else {
        m_allocated = MAX_DISPLAY_LENGTH;
      }
    }
    m_buffer.resize(m_allocated);

    memset(&m_buffer[0], ' ', m_buffer.size());
  }

  bool append(const char *text, size_t length, size_t display_size,
              size_t buffer_size) {
    if (buffer_size > m_allocated) return false;

    size_t next_index = 0;
    if (m_format == ResultFormat::TABLE) {
      if (m_align_right && m_max_display_length > display_size) {
        next_index = m_max_display_length - display_size;
      }
    }

    auto buffer = &m_buffer[0];
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
      // If some multibyte characters were found, we need to truncate the buffer
      // adding the 'lost' characters
      buffer[std::min(m_allocated,
                      m_max_display_length + (buffer_size - display_size))] = 0;
    } else {
      m_buffer.resize(next_index);
    }

    return true;
  }
};

class Console_printer : public Resultset_printer {
 public:
  Console_printer() : m_console(mysqlsh::current_console()) {}

  void print(const std::string &s) override { m_console->print(s); }

  void println(const std::string &s) override { m_console->println(s); }

  void raw_print(const std::string &s) override {
    m_console->raw_print(s, mysqlsh::Output_stream::STDOUT, false);
  }

 private:
  std::shared_ptr<IConsole> m_console;
};

class String_printer : public Resultset_printer {
 public:
  String_printer() = default;

  void print(const std::string &s) override { raw_print(s); }

  void println(const std::string &s) override {
    print(s);
    print("\n");
  }

  void raw_print(const std::string &s) override { m_output += s; }

  void reset() { m_output.clear(); }

  std::string output() const { return m_output; }

 private:
  std::string m_output;
};

}  // namespace

Resultset_dumper_base::Resultset_dumper_base(
    mysqlshdk::db::IResult *target, std::unique_ptr<Resultset_printer> printer,
    const std::string &wrap_json, const std::string &format)
    : m_result(target),
      m_wrap_json(wrap_json),
      m_format(format),
      m_printer(std::move(printer)) {
  if (m_format == "ndjson") m_format = "json/raw";
}

Resultset_dumper::Resultset_dumper(mysqlshdk::db::IResult *target)
    : Resultset_dumper(target,
                       mysqlsh::current_shell_options()->get().wrap_json,
                       mysqlsh::current_shell_options()->get().result_format,
                       mysqlsh::current_shell_options()->get().show_warnings,
                       mysqlsh::current_shell_options()->get().interactive) {}

Resultset_dumper::Resultset_dumper(mysqlshdk::db::IResult *target,
                                   const std::string &wrap_json,
                                   const std::string &format,
                                   bool show_warnings, bool show_stats)
    : Resultset_dumper_base(target, std::make_unique<Console_printer>(),
                            wrap_json, format),
      m_show_warnings(show_warnings),
      m_show_stats(show_stats) {}

size_t Resultset_dumper::dump(const std::string &item_label, bool is_query,
                              bool is_doc_result) {
  std::string output;
  size_t total_count = 0;
  shcore::Interrupt_handler intr([this]() {
    m_cancelled = true;
    return true;
  });

  if (m_wrap_json != "off") {
    total_count = dump_json(item_label, is_doc_result);
  } else {
    bool first = true;
    do {
      size_t count = 0;
      // Prints a blank line between multi-results
      if (first) {
        first = false;
      } else {
        m_printer->print("\n");
      }

      if (m_result->has_resultset()) {
        if (is_doc_result || m_format.find("json") != std::string::npos)
          count = dump_documents(is_doc_result);
        else if (m_format == "vertical")
          count = dump_vertical();
        else if (m_format == "table")
          count = dump_table();
        else
          count = dump_tabbed();

        total_count += count;

        if (count)
          output =
              shcore::str_format("%zu %s%s in set", count, item_label.c_str(),
                                 (count == 1 ? "" : "s"));
        else
          output = "Empty set";
      } else if (m_show_stats && !is_query) {
        // Starts only make sense on non read only operations
        output = get_affected_stats(item_label);
      }

      // This information output is only printed in interactive mode
      int warning_count = 0;
      if (m_show_stats) {
        warning_count = get_warning_and_execution_time_stats(&output);

        m_printer->print(output);
      }

      if (!is_query) {
        std::string info = m_result->get_info();
        if (!info.empty()) {
          info = "\n" + info + "\n";
          m_printer->print(info);
        }
      }

      // Prints the warnings if there were any
      if (warning_count && m_show_warnings) dump_warnings();

    } while (m_result->next_resultset() && !m_cancelled);
  }

  if (m_cancelled)
    m_printer->println(
        "Result printing interrupted, rows may be missing from the output.");

  return total_count;
}

/**
 * This utility function creates a JSON document including all the fields in a
 * given row and appends it to a JSON_dumper object.
 */
void dump_json_row(shcore::JSON_dumper *dumper,
                   const std::vector<mysqlshdk::db::Column> &metadata,
                   const mysqlshdk::db::IRow *row) {
  dumper->start_object();

  for (size_t col_index = 0; col_index < metadata.size(); col_index++) {
    auto column = metadata[col_index];

    dumper->append_string(column.get_column_label());
    auto type = column.get_type();
    if (row->is_null(col_index)) {
      dumper->append_null();
    } else if (mysqlshdk::db::is_string_type(type)) {
      if (type == mysqlshdk::db::Type::Json) {
        dumper->append_json(row->get_string(col_index));
      } else if (type == mysqlshdk::db::Type::Bytes) {
        auto data = row->get_string_data(col_index);
        std::string encoded;
        size_t binary_limit = data.second;
        if (mysqlsh::current_shell_options()->get().binary_limit > 0) {
          binary_limit = std::min(
              data.second,
              mysqlsh::current_shell_options()->get().binary_limit + 1);
        }

        shcore::encode_base64(
            static_cast<const unsigned char *>(
                static_cast<const void *>(data.first)),
            // At most binary-limit + 1 bytes shuold be sent, when the extra
            // byte is sent, it will be an indicator for the consumer of the
            // data that a truncation happened
            binary_limit, &encoded);
        dumper->append_string(encoded);
      } else {
        auto data = row->get_as_string(col_index);
        dumper->append_string(data.c_str(), data.length());
      }
    } else if (type == mysqlshdk::db::Type::Integer) {
      dumper->append_int64(row->get_int(col_index));
    } else if (type == mysqlshdk::db::Type::UInteger) {
      dumper->append_uint64(row->get_uint(col_index));
    } else if (type == mysqlshdk::db::Type::Float) {
      dumper->append_float(static_cast<double>(row->get_float(col_index)));
    } else if (type == mysqlshdk::db::Type::Double) {
      dumper->append_float(row->get_double(col_index));
    } else if (type == mysqlshdk::db::Type::Decimal) {
      dumper->append_float(static_cast<double>(row->get_float(col_index)));
    } else if (type == mysqlshdk::db::Type::Bit) {
      dumper->append_int64(row->get_bit(col_index));
    }
  }
  dumper->end_object();
}

/**
 * Dumps a JSON document for each row/document contained on the result
 * being processed.
 */
size_t Resultset_dumper_base::dump_documents(bool is_doc_result) {
  const auto &metadata = m_result->get_metadata();
  auto row = m_result->fetch_one();
  size_t row_count = 0;
  const bool as_array = m_format == "json/array";
  const bool pretty = m_format != "json/raw" && m_format != "json/array";

  if (!row) return row_count;

  if (as_array) m_printer->raw_print("[\n");
  while (row) {
    shcore::JSON_dumper dumper(
        pretty, mysqlsh::current_shell_options()->get().binary_limit);

    if (row_count > 0) {
      if (as_array)
        m_printer->raw_print(",\n");
      else
        m_printer->raw_print("\n");
    }

    if (is_doc_result)
      dumper.append_json(row->get_string(0));
    else
      dump_json_row(&dumper, metadata, row);

    m_printer->raw_print(dumper.str());

    row_count++;
    row = m_result->fetch_one();
  }
  m_printer->raw_print("\n");
  if (as_array) m_printer->raw_print("]\n");

  return row_count;
}

size_t Resultset_dumper_base::dump_tabbed() {
  const auto &metadata = m_result->get_metadata();
  auto row = m_result->fetch_one();
  size_t row_index = 0;

  if (!row) return row_index;

  size_t index = 0;
  size_t field_count = metadata.size();
  std::vector<std::string> formats(field_count, "%-");
  std::vector<Field_formatter> fmt;

  // Prints the initial separator line and the column headers
  for (index = 0; index < field_count; index++) {
    auto column = metadata[index];

    fmt.emplace_back(ResultFormat::TABBED, column);
    m_printer->print(column.get_column_label().c_str());
    m_printer->print(index < (field_count - 1) ? "\t" : "\n");
  }

  // Now prints the records
  while (row && !m_cancelled) {
    for (size_t field_index = 0; field_index < field_count; field_index++) {
      auto column = metadata[field_index];
      if (fmt[field_index].put(row, field_index)) {
        m_printer->print(fmt[field_index].str());
      } else {
        mysqlshdk::db::is_string_type(column.get_type());
        m_printer->print(row->get_string(field_index));
      }
      m_printer->print(field_index < (field_count - 1) ? "\t" : "\n");
    }

    row = m_result->fetch_one();
    row_index++;
  }

  return row_index;
}

size_t Resultset_dumper_base::dump_vertical() {
  return format_vertical(true, true, 0);
}

size_t Resultset_dumper_base::format_vertical(bool has_header, bool align_right,
                                              size_t min_label_width) {
  const auto &metadata = m_result->get_metadata();

  const std::string star_separator(27, '*');

  std::vector<Field_formatter> fmt;

  // Calculate length of a longest column description, used to right align
  // column descriptions
  std::size_t max_col_len = min_label_width;
  for (const auto &column : metadata) {
    max_col_len = std::max(max_col_len, column.get_column_label().length());
    fmt.emplace_back(ResultFormat::VERTICAL, column);
  }

  auto row = m_result->fetch_one();
  size_t row_index = 0;
  while (row && !m_cancelled) {
    if (has_header) {
      std::string row_header = star_separator + " " +
                               std::to_string(row_index + 1) + ". row " +
                               star_separator + "\n";

      m_printer->print(row_header);
    }

    for (size_t col_index = 0; col_index < metadata.size(); col_index++) {
      auto column = metadata[col_index];

      std::string padding(max_col_len - column.get_column_label().size(), ' ');
      std::string label = column.get_column_label() + ": ";

      if (align_right) {
        label = padding + label;
      } else {
        label += padding;
      }

      m_printer->print(label);
      if (fmt[col_index].put(row, col_index)) {
        m_printer->print(fmt[col_index].str());
      } else {
        assert(mysqlshdk::db::is_string_type(column.get_type()));
        m_printer->print(row->get_string(col_index));
      }
      m_printer->raw_print("\n");
    }

    row = m_result->fetch_one();
    row_index++;
  }
  return row_index;
}

/**
 * Creates a JSON Document including all the information in the result being
 * processd, this includes:
 *
 * - Metadata
 * - Rows
 * - Statistics
 * - Warnings
 *
 * This function is used when JSON Wrapping is turned ON
 */
size_t Resultset_dumper_base::dump_json(const std::string &item_label,
                                        bool is_doc_result) {
  shcore::JSON_dumper dumper(
      m_wrap_json == "json",
      mysqlsh::current_shell_options()->get().binary_limit);

  dumper.start_object();
  dumper.append_string("hasData");
  dumper.append_bool(m_result->has_resultset());

  dumper.append_string(item_label + "s");
  dumper.start_array();

  size_t row_count = 0;
  if (m_result->has_resultset()) {
    const auto &metadata = m_result->get_metadata();
    auto row = m_result->fetch_one();
    while (row) {
      if (is_doc_result) {
        dumper.append_json(row->get_string(0));
      } else {
        dump_json_row(&dumper, metadata, row);
      }
      row_count++;
      row = m_result->fetch_one();
    }
  }

  dumper.end_array();

  dumper.append_string("executionTime");
  dumper.append_string(
      mysqlshdk::utils::format_seconds(m_result->get_execution_time()));
  if (!is_doc_result) {
    dumper.append_string("affectedRowCount");
    dumper.append_uint64(m_result->get_affected_row_count());
  }
  dumper.append_string("affectedItemsCount");
  dumper.append_uint64(m_result->get_affected_row_count());
  dumper.append_string("warningCount");
  dumper.append_int64(m_result->get_warning_count());
  dumper.append_string("warningsCount");
  dumper.append_uint64(m_result->get_warning_count());
  dumper.append_string("warnings");
  dumper.start_array();
  auto warning = m_result->fetch_one_warning();
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
    warning = m_result->fetch_one_warning();
  }
  dumper.end_array();

  dumper.append_string("info");
  dumper.append_string(m_result->get_info());

  dumper.append_string("autoIncrementValue");
  dumper.append_int64(m_result->get_auto_increment_value());

  dumper.end_object();

  m_printer->raw_print(dumper.str());
  m_printer->raw_print("\n");

  return row_count;
}

size_t Resultset_dumper_base::dump_table() {
  const auto &metadata = m_result->get_metadata();
  std::vector<Field_formatter> fmt;
  std::vector<mysqlshdk::db::Row_copy> pre_fetched_rows;
  size_t num_records = 0;
  const size_t field_count = metadata.size();
  if (field_count == 0) return 0;

  // Updates the max_length array with the maximum length between column name,
  // min column length and column max length
  for (size_t field_index = 0; field_index < field_count; field_index++) {
    auto column = metadata[field_index];
    fmt.emplace_back(ResultFormat::TABLE, column);
  }

  pre_fetched_rows.reserve(k_pre_fetch_result_rows);
  {
    auto row = m_result->fetch_one();
    while (row && !m_cancelled) {
      pre_fetched_rows.emplace_back(*row);

      for (size_t field_index = 0; field_index < field_count; field_index++) {
        fmt[field_index].process(row, field_index);
      }

      if (pre_fetched_rows.size() >= k_pre_fetch_result_rows) break;

      row = m_result->fetch_one();
    }
  }
  if (m_cancelled || pre_fetched_rows.empty()) return 0;

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
  m_printer->print(separator);
  m_printer->print("| ");
  for (index = 0; index < field_count; index++) {
    std::string format = "%-";
    format.append(std::to_string(fmt[index].get_max_display_length()));
    format.append((index == field_count - 1) ? "s |\n" : "s | ");
    auto column = metadata[index];
    m_printer->print(
        shcore::str_format(format.c_str(), column.get_column_label().c_str()));
  }
  m_printer->print(separator);

  // Print pre-fetched records
  for (const auto &row : pre_fetched_rows) {
    ++num_records;
    m_printer->print("| ");

    for (size_t field_index = 0; field_index < field_count; field_index++) {
      if (fmt[field_index].put(&row, field_index)) {
        m_printer->print(fmt[field_index].str());
      } else {
        assert(mysqlshdk::db::is_string_type(metadata[field_index].get_type()));
        if (row.get_type(field_index) == mysqlshdk::db::Type::Bytes) {
          const char *data;
          size_t length;
          std::tie(data, length) = row.get_string_data(field_index);
          m_printer->print(shcore::string_to_hex(data, length));
        } else {
          m_printer->print(row.get_as_string(field_index));
        }
      }
      if (field_index < field_count - 1) m_printer->print(" | ");
    }
    m_printer->print(" |\n");

    if (m_cancelled) break;
  }
  // Now prints the remaining records
  if (!m_cancelled) {
    auto row = m_result->fetch_one();
    while (row && !m_cancelled) {
      ++num_records;
      m_printer->print("| ");

      for (size_t field_index = 0; field_index < field_count; field_index++) {
        if (fmt[field_index].put(row, field_index)) {
          m_printer->print(fmt[field_index].str());
        } else {
          assert(
              mysqlshdk::db::is_string_type(metadata[field_index].get_type()));
          m_printer->print(row->get_as_string(field_index));
        }
        if (field_index < field_count - 1) m_printer->print(" | ");
      }
      m_printer->print(" |\n");
      row = m_result->fetch_one();
    }
  }
  m_printer->print(separator);

  return num_records;
}

std::string Resultset_dumper::get_affected_stats(
    const std::string &item_label) {
  std::string output;

  // Some queries return -1 since affected rows do not apply to them
  int64_t affected_items = m_result->get_affected_row_count();

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
    std::string *output_stats) {
  int warning_count = 0;

  if (m_show_stats) {
    warning_count = m_result->get_warning_count();

    if (warning_count)
      output_stats->append(shcore::str_format(", %d warning%s", warning_count,
                                              (warning_count == 1 ? "" : "s")));

    output_stats->append(" ");
    output_stats->append(shcore::str_format(
        "(%s)", mysqlshdk::utils::format_seconds(m_result->get_execution_time())
                    .c_str()));
    output_stats->append("\n");
  }

  return warning_count;
}

void Resultset_dumper_base::dump_warnings() {
  auto warning = m_result->fetch_one_warning();
  while (warning && !m_cancelled) {
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

    m_printer->print((shcore::str_format("%s (code %d): %s\n", type.c_str(),
                                         warning->code, warning->msg.c_str())));

    warning = m_result->fetch_one_warning();
  }
}

Resultset_writer::Resultset_writer(mysqlshdk::db::IResult *target)
    : Resultset_dumper_base(
          target, std::make_unique<String_printer>(),
          mysqlsh::current_shell_options()->get().wrap_json,
          mysqlsh::current_shell_options()->get().result_format) {}

std::string Resultset_writer::write_table() {
  return write([this]() { dump_table(); });
}

std::string Resultset_writer::write_vertical() {
  return write([this]() { dump_vertical(); });
}

std::string Resultset_writer::write_status() {
  return write([this]() { format_vertical(false, false, 24); });
}

std::string Resultset_writer::write(const std::function<void()> &dump) {
  const auto printer = dynamic_cast<String_printer *>(m_printer.get());
  printer->reset();

  while (m_result->has_resultset()) {
    dump();
    printer->raw_print("\n");

    m_result->next_resultset();
  }

  auto result = printer->output();
  // remove extra new line
  result.pop_back();

  return result;
}

}  // namespace mysqlsh
