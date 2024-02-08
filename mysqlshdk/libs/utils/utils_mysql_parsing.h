/*
 * Copyright (c) 2015, 2024, Oracle and/or its affiliates.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.0,
 * as published by the Free Software Foundation.
 *
 * This program is designed to work with certain software (including
 * but not limited to OpenSSL) that is licensed under separate terms,
 * as designated in a particular file or component or in included license
 * documentation.  The authors of MySQL hereby grant you an additional
 * permission to link the program and your derivative works with the
 * separately licensed software that they have either included with
 * the program or referenced in the documentation.
 *
 * This program is distributed in the hope that it will be useful,  but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 * the GNU General Public License, version 2.0, for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */
#ifndef _UTILS_MYSQL_PARSING_H_
#define _UTILS_MYSQL_PARSING_H_

#include <array>
#include <cassert>
#include <functional>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

namespace mysqlshdk {
namespace utils {

class Sql_splitter {
 public:
  // callback(cmdstr, single_line, line_num) ->
  //                    (real_length, is_delimiter)
  using Command_callback =
      std::function<std::pair<size_t, bool>(std::string_view, bool, size_t)>;

  using Error_callback = std::function<void(std::string_view)>;

  /** Sql_splitter constructor
   *
   * @param cmd_callback Function to call when command is detected.
   * @param err_callback Error callback.
   * @param commands List of keywords defining SQL like commands (e.g. 'use').
   */
  Sql_splitter(Command_callback cmd_callback, Error_callback err_callback,
               std::initializer_list<std::string_view> commands = {});

  void reset();

  void feed_chunk(char *buffer, size_t size);
  void feed(char *buffer, size_t size);

  void set_ansi_quotes(bool enabled) { m_ansi_quotes = enabled; }

  void set_dollar_quoted_strings(bool enabled) {
    m_dollar_quoted_strings = enabled;
  }

  void set_no_backslash_escapes(bool enabled) {
    m_no_backslash_escapes = enabled;
  }

  bool set_delimiter(std::string delim);

  const std::string &delimiter() const { return m_delimiter; }

  struct Range {
    size_t offset;
    size_t length;
    size_t line_num;
  };

  bool next_range(Range *out_range, std::string *out_delim);

  void pack_buffer(std::string *buffer, Range last_range);

  size_t chunk_size() const { return m_end - m_begin; }

  bool eof() const { return m_last_chunk && (m_ptr >= m_end || m_eof); }

  bool is_last_chunk() const { return m_last_chunk; }

  enum class Context {
    kNone,
    kStatement,           // any non-(comments/identifiers) before delimiter
    kIdentifier,          // unquoted identifier or a SQL keyword
    kComment,             // /* ... */
    kCommentHint,         // /*+ ... */ ...;
    kCommentConditional,  // /*! ... */ ...;
    kSQuoteString,        // '...'
    kDQuoteString,        // "..." (only if not ansi_quotes)
    kDollarQuotedString,  // $tag$...$tag$ (only if dollar_quotes)
    kBQuoteIdentifier,    // `...`
    kDQuoteIdentifier     // "..." (ansi_quotes)
  };

  Context context() const {
    return m_context.empty() ? Context::kNone : m_context.back();
  }

 private:
  inline void push(Context c) { m_context.push_back(c); }

  inline void pop() {
    if (!m_context.empty()) m_context.pop_back();
  }

  char *m_begin;
  char *m_end;
  char *m_ptr;

  std::string m_delimiter = ";";
  std::vector<Context> m_context;
  std::string m_dollar_quote;

  Command_callback m_cmd_callback;
  Error_callback m_err_callback;

  std::array<std::vector<std::string>, 'z' - 'a'> m_commands_table;

  size_t m_shrinked_bytes{0};
  size_t m_current_line{1};
  size_t m_total_offset{0};
  bool m_ansi_quotes{false};
  bool m_dollar_quoted_strings{true};
  bool m_no_backslash_escapes{false};
  bool m_last_chunk{false};
  bool m_eof{false};
};

std::string to_string(Sql_splitter::Context context);

std::vector<std::tuple<std::string, std::string, size_t>> split_sql_stream(
    std::istream *stream, size_t chunk_size,
    const Sql_splitter::Error_callback &err_callback, bool ansi_quotes = false,
    bool no_backslash_escapes = false, bool dollar_quotes = true,
    std::string *delimiter = nullptr);

std::vector<std::string> split_sql(const std::string &str,
                                   bool ansi_quotes = false,
                                   bool no_backslash_escapes = false,
                                   bool dollar_quotes = true);

bool iterate_sql_stream(
    std::istream *stream, size_t chunk_size,
    const std::function<bool(std::string_view, std::string_view, size_t,
                             size_t)> &stmt_callback,
    const Sql_splitter::Error_callback &err_callback, bool ansi_quotes = false,
    bool no_backslash_escapes = false, bool dollar_quotes = true,
    std::string *delimiter = nullptr, Sql_splitter **splitter_ptr = nullptr);

}  // namespace utils
}  // namespace mysqlshdk

#endif
