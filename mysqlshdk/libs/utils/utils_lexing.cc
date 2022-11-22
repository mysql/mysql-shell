/*
 * Copyright (c) 2017, 2022, Oracle and/or its affiliates.
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

#include "mysqlshdk/libs/utils/utils_lexing.h"
#include <cctype>

namespace mysqlshdk {
namespace utils {

size_t span_cstyle_sql_comment(std::string_view s, size_t offset) {
  const size_t size = s.size();
  assert(!s.empty());
  assert(offset < size);
  assert(size - offset < 2 || (s[offset] == '/' && s[offset + 1] == '*'));

  if (size < 4) return std::string_view::npos;

  // In general, we try to follow the server's lexing behaviour and not the
  // classic client.

  if (offset + 2 < size && s[offset + 2] == '!') {
    // Conditionals are parsed as follows:
    // - /*! ... */ and /*!9999 ... */ are OK
    // - Nested quoted strings are interpreted as such
    // - Nested comments work in the mysql cli, but are deprecated in the server
    // - Nested line comments (-- ...) are valid
    //
    // Also, the server and old mysql cli lexers have differences in behaviour.
    // In these cases, we follow the server behaviour.

    offset += 3;  // skip /*!

    while (offset < size) {
      switch (s[offset]) {
        case '\'':
          offset = span_quoted_string_sq(s, offset);
          break;
        case '"':
          offset = span_quoted_string_dq(s, offset);
          break;
        case '`':
          offset = span_quoted_sql_identifier_bt(s, offset);
          break;
        case '*':
          // check for end of comment
          ++offset;
          if (offset < size && s[offset] == '/') {
            ++offset;
            return offset;
          }
          break;
        case '\n':
          // check for nested line comments
          ++offset;
          if (offset + 2 < size && s[offset] == '-' && s[offset + 1] == '-' &&
              (s[offset + 2] == ' ' || s[offset + 2] == '\t')) {
            offset = span_to_eol(s, offset + 3);
          }
          break;
        default:
          ++offset;
          break;
      }
    }
    return std::string_view::npos;
  } else {
    // optimizer hints (/*+ ... */) are just like plain comments. The old mysql
    // cli used to understand single line -- comments inside hints, but that's
    // a bug. The server treats those as regular text.
    return span_cstyle_comment(s, offset);
  }
}

SQL_iterator::SQL_iterator(std::string_view str, size_type offset,
                           bool skip_quoted_sql_ids)
    : m_s(str), m_offset(offset - 1), m_skip_quoted(skip_quoted_sql_ids) {
  assert(offset <= str.size());
  // Let's make sure we start from valid SQL
  ++(*this);
}

SQL_iterator &SQL_iterator::operator++() {
  const auto length = m_s.length();

  if (++m_offset > length)
    throw std::out_of_range("SQL_string_iterator offset out of range");

  bool incremented = false;

  while (!incremented && m_offset < length) {
    switch (m_s[m_offset]) {
      case '\'':
        if (m_skip_quoted)
          m_offset = span_quoted_string_sq(m_s, m_offset);
        else
          incremented = true;
        break;
      case '"':
        if (m_skip_quoted)
          m_offset = span_quoted_string_dq(m_s, m_offset);
        else
          incremented = true;
        break;
      case '`':
        if (m_skip_quoted)
          m_offset = span_quoted_sql_identifier_bt(m_s, m_offset);
        else
          incremented = true;
        break;
      case '/':
        if (m_offset + 1 < length && m_s[m_offset + 1] == '*') {
          if (m_offset + 2 < length &&
              (m_s[m_offset + 2] == '!' || m_s[m_offset + 2] == '+')) {
            m_comment_hint = true;
            m_offset += 3;
            while (std::isdigit(m_s[m_offset])) ++m_offset;
          } else {
            m_offset = span_cstyle_comment(m_s, m_offset);
          }
        } else {
          incremented = true;
        }
        break;
      case '*':
        if (m_comment_hint && m_offset + 1 < length &&
            m_s[m_offset + 1] == '/') {
          m_offset += 2;
          m_comment_hint = false;
        } else {
          incremented = true;
        }
        break;
      case '#':
        m_offset = span_to_eol(m_s, m_offset + 1);
        break;
      case '-':
        if (m_offset + 2 < length && m_s[m_offset + 1] == '-' &&
            std::isspace(m_s[m_offset + 2]))
          m_offset = span_to_eol(m_s, m_offset + 2);
        else
          incremented = true;
        break;
      default:
        incremented = true;
    }
    if (m_offset == std::string_view::npos) m_offset = length;
  }

  return *this;
}

std::string_view SQL_iterator::next_token() {
  return next_token_and_offset().first;
}

std::pair<std::string_view, size_t> SQL_iterator::next_token_and_offset() {
  while (valid() && std::isspace(get_char())) ++(*this);
  if (!valid()) return {{}, m_offset};

  const auto start = m_offset;
  if (!m_skip_quoted) {
    switch (get_char()) {
      case '\'':
        m_offset = span_quoted_string_sq(m_s, m_offset);
        return {m_s.substr(start, m_offset - start), start};
      case '"':
        m_offset = span_quoted_string_dq(m_s, m_offset);
        return {m_s.substr(start, m_offset - start), start};
    }
  }

  auto previous = m_offset;
  do {
    const auto c = get_char();
    bool break_time = false;
    switch (c) {
      case ',':
      case ';':
      case '(':
      case ')':
      case '=':
      case '@':
        break_time = true;
        break;
      case '\'':
      case '"':
        break_time = !m_skip_quoted;
        break;
      case '`':
        if (!m_skip_quoted) {
          if (m_offset == start || m_s[m_offset - 1] == '.') {
            m_offset = span_quoted_sql_identifier_bt(m_s, m_offset);
            if (!valid() || get_char() != '.')
              return {m_s.substr(start, m_offset - start), start};
          } else {
            break_time = true;
          }
        }
        break;
      default:
        if (std::isspace(c) || m_offset - previous > 1) break_time = true;
        break;
    }
    if (break_time) break;

    previous = m_offset;
    if (c == ':') {
      ++(*this);
      break;
    }
  } while ((++(*this)).valid());

  if (m_offset == start) ++(*this);
  return {m_s.substr(start, previous + 1 - start), start};
}

std::string_view SQL_iterator::next_sql_function() {
  std::string_view token;
  while (!(token = next_token()).empty()) {
    if (!std::isalpha(token.front())) continue;
    if (valid() && get_char() == '(') break;
  }
  return token;
}

}  // namespace utils
}  // namespace mysqlshdk
