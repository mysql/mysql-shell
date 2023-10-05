/*
 * Copyright (c) 2017, 2023, Oracle and/or its affiliates.
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

#ifndef MYSQLSHDK_LIBS_UTILS_UTILS_LEXING_H_
#define MYSQLSHDK_LIBS_UTILS_UTILS_LEXING_H_

#include <cassert>
#include <cctype>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

#include "mysqlshdk/libs/utils/utils_string.h"

namespace mysqlshdk {
namespace utils {

namespace internal {

// lookup table of number of bytes to skip when spanning a quoted string
// (single quote and double quote versions)
// skip 2 for escape, 0 for '\0', 0 for end quote and 1 for the rest
inline constexpr char k_quoted_string_span_skips_sq[2][256] = {
    // backslash escapes
    {0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
     1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1,
     1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
     1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 1,
     1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
     1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
     1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
     1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
     1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
     1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
     1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
    // no backslash escapes
    {0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
     1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1,
     1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
     1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
     1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
     1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
     1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
     1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
     1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
     1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
     1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}};

inline constexpr char k_quoted_string_span_skips_dq[2][256] = {
    // backslash escapes
    {0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
     1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
     1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
     1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 1,
     1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
     1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
     1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
     1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
     1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
     1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
     1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
    // no backslash escapes
    {0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
     1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
     1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
     1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
     1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
     1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
     1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
     1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
     1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
     1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
     1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}};

inline constexpr char k_keyword_chars[] =
    "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_";

inline constexpr char k_identifier_chars[] =
    "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_$";

// same as k_keyword_chars (identifier chars except $)
inline constexpr char k_dollar_quoted_string_span_chars[256] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 1,
    0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};

}  // namespace internal

/*
 * Fast scanning of a quoted string (_dq for " or _sq for ') until the closing
 * quote or end of string.
 * Escaped quotes ("foo\"bar") are handled.
 * Double quoting ("foo""bar")is not handled, but can be handled by calling
 * this twice.
 *
 * Note that this will not process escapes or modify the input, it simply finds
 * the boundary of the string.
 *
 * @param s the string to be lexed. The first character must be a quote char.
 * @return offset after the end of the closing quote or npos on error
 */
inline size_t span_quoted_string_dq(std::string_view s, size_t offset) {
  assert(!s.empty());
  assert(offset < s.length());
  assert(s[offset] == '"');

  // skip opening quote
  ++offset;

  unsigned char last_ch = 0;
  for (;;) {
    // Tight-loop to span until terminating 0 or closing quote
    while (offset < s.length() &&
           internal::k_quoted_string_span_skips_dq
                   [0][last_ch = static_cast<unsigned char>(s[offset])] > 0) {
      offset += internal::k_quoted_string_span_skips_dq[0][last_ch];
    }

    if (offset >= s.length()) {
      // we exited the loop without seeing a "
      return std::string_view::npos;
    }

    if (last_ch == '\0') {
      // there was a '\0', but the string is not over, continue spanning
      ++offset;
      continue;
    }

    // the only other possible way to exit the loop must be the end quote
    assert(last_ch == '"');
    return offset + 1;
  }
}

inline size_t span_quoted_string_sq(std::string_view s, size_t offset) {
  assert(!s.empty());
  assert(offset < s.length());
  assert(s[offset] == '\'');

  // skip opening quote
  ++offset;

  unsigned char last_ch = 0;
  for (;;) {
    // Tight-loop to span until terminating 0 or closing quote
    while (offset < s.length() &&
           internal::k_quoted_string_span_skips_sq
                   [0][last_ch = static_cast<unsigned char>(s[offset])] > 0) {
      offset += internal::k_quoted_string_span_skips_sq[0][last_ch];
    }

    if (offset >= s.length()) {
      // we exited the loop without seeing a '
      return std::string_view::npos;
    }

    if (last_ch == '\0') {
      // there was a '\0', but the string is not over, continue spanning
      ++offset;
      continue;
    }

    // the only other possible way to exit the loop must be the end quote
    assert(last_ch == '\'');
    return offset + 1;
  }
}

inline size_t span_quoted_sql_identifier_bt(std::string_view s, size_t offset) {
  const auto length = s.length();
  assert(!s.empty());
  assert(offset < length);
  assert(s[offset] == '`');

  // unlike strings, identifiers don't allow escaping with the \ char
  size_t p = offset + 1;
  for (;;) {
    p = s.find('`', p);
    if (p == std::string_view::npos) {
      break;
    }
    if (p + 1 < length && s[p + 1] == '`') {
      p += 2;
    } else {
      ++p;
      break;
    }
  }
  return p;
}

// Span spaces. If offset is npos, return npos
inline size_t span_spaces(std::string_view s, size_t offset) {
  return s.find_first_not_of(" \t\r\n", offset);
}

inline size_t span_not_spaces(std::string_view s, size_t offset) {
  size_t p = s.find_first_of(" \t\r\n", offset);
  if (p == std::string_view::npos) return s.size();
  return p;
}

inline size_t span_quoted_sql_identifier_dquote(std::string_view s,
                                                size_t offset) {
  const auto length = s.length();
  assert(!s.empty());
  assert(offset < length);
  assert(s[offset] == '"');

  // unlike strings, identifiers don't allow escaping with the \ char
  size_t p = offset + 1;
  for (;;) {
    p = s.find('"', p);
    if (p == std::string_view::npos) {
      break;
    }
    if (p + 1 < length && s[p + 1] == '"') {
      p += 2;
    } else {
      ++p;
      break;
    }
  }
  return p;
}

inline size_t span_keyword(
    std::string_view s, size_t offset,
    const char keyword_chars[] = internal::k_keyword_chars) {
  assert(!s.empty());
  assert(offset < s.length());

  if (std::isalpha(s[offset]) || s[offset] == '_') {
    size_t p;

    p = s.find_first_not_of(keyword_chars, offset + 1);

    if (p == std::string_view::npos) p = s.length();

    return p;
  } else {
    return offset;
  }
}

inline bool is_start_of_unquoted_identifier(char c) {
  return internal::k_dollar_quoted_string_span_chars[static_cast<int>(c)] ||
         c == '$';
}

/** Spans an unquoted identifier or number or a keyword
 *
 * Identifiers can contain any of:
 * - ascii letters
 * - digits
 * - _
 * - $ if not the 1st char (depending on server version)
 * - UTF-8 sequences (chars with high-bit set)
 *
 * This function assumes the input string points to the start of a valid
 * identifier, including those starting with $. Checking for $ at the start
 * of an identifier when it's not allowed must be handled by the caller.
 *
 * @param s
 * @param offset
 * @return size_t
 */
inline size_t span_unquoted_identifier(std::string_view s, size_t offset) {
  assert(!s.empty());
  assert(offset < s.size());
  assert(is_start_of_unquoted_identifier(s[offset]));

  for (;;) {
    size_t p = s.find_first_not_of(internal::k_identifier_chars, offset + 1);
    if (p == std::string_view::npos || p == s.length()) return s.length();
    if (static_cast<unsigned char>(s[p]) <= 0x7f) return p;
    offset = p;
  }
}

inline bool is_dollar_quote_tag(std::string_view s) {
  for (const unsigned char i : s) {
    if (!internal::k_dollar_quoted_string_span_chars[i]) {
      return false;
    }
  }
  return true;
}

inline size_t span_dollar_quoted_string_or_dollar_identifier(std::string_view s,
                                                             size_t offset) {
  // given a string starting with $:
  // - 1st determine the full quoting sequence in the form $tag$, where tag must
  // be a valid identifier-like string or empty
  // - span the string until a matching $tag$ is found
  // OR
  // - if the string starts with a $identifier, then spans until the end of the
  // identifier
  assert(!s.empty());
  assert(offset < s.length());
  assert(s[offset] == '$');

  auto endtag = s.find('$', offset + 1);
  // unclosed $tag$
  if (endtag == std::string_view::npos) {
    // check if this is $identifier<EOL>
    return span_unquoted_identifier(s, offset);
  }
  // validate the tag for allowed chars only
  const auto tag = s.substr(offset, endtag + 1 - offset);
  if (!is_dollar_quote_tag(tag.substr(1, tag.size() - 2))) {
    // this is not a $tag$, so it must be a $identifier<...>$
    return span_unquoted_identifier(s, offset);
  }
  // find the closing tag
  auto end = s.find(tag, offset + tag.size());
  if (end == std::string_view::npos)
    throw std::invalid_argument(
        std::string("Unterminated ").append(tag).append(+" quoted string"));

  return end + tag.size();
}

inline size_t span_to_eol(std::string_view s, size_t offset) {
  offset = s.find('\n', offset);
  if (offset == std::string_view::npos) return s.length();
  return offset + 1;
}

/** Spans a string for C style multi-line comments
 *
 * This does NOT handle MySQL optimizer hint comments or conditional
 * comments, so it should only be used to span non-SQL strings.
 */
inline size_t span_cstyle_comment(std::string_view s, size_t offset) {
  assert(!s.empty());
  assert(offset < s.size());
  assert(s.size() - offset < 2 || (s[offset] == '/' && s[offset + 1] == '*'));

  if (s.size() < 4) return std::string_view::npos;

  offset += 2;

  size_t p = s.find("*/", offset);
  if (p == std::string_view::npos) return std::string_view::npos;
  return p + 2;
}

/** Spans a SQL string for C style multi-line comments
 *
 * This function handles special comments that start with a + (optimizer hints)
 * or ! (conditional statements).
 *
 * If dollar_quote_strings is true, it will also handle $tag$...$tag$ style
 * strings.
 */
size_t span_cstyle_sql_comment(std::string_view s, size_t offset,
                               bool dollar_quote_strings = true);

/** Class enabling iteration over characters in SQL string skipping comments and
 * quoted strings.
 *
 * This iterator casts implicitly to size_t and can be used as substitute for
 * size_t in standard for loop iterating over std::string.
 */
class SQL_iterator {
 public:
  using size_type = std::string_view::size_type;
  using value_type = std::string_view::value_type;

  /** Create SQL_string_iterator.
   *
   * @arg str string containing SQL query.
   * @arg offset offset in str, need to point to valid part of SQL.
   * @arg dollar_quotes if true, treats $tag$ as string quotes
   */
  explicit SQL_iterator(std::string_view str, size_type offset = 0,
                        bool skip_quoted = true, bool dollar_quotes = true);

  SQL_iterator &operator++();

  SQL_iterator operator++(int) {
    SQL_iterator ans = *this;
    ++(*this);
    return ans;
  }

  void advance() { ++(*this); }

  value_type get_char() const {
    assert(m_offset < m_s.length());
    return m_s[m_offset];
  }

  value_type operator*() const { return get_char(); }

  bool operator==(const SQL_iterator &a) const {
    return m_offset == a.m_offset && m_s == a.m_s;
  }

  bool operator!=(const SQL_iterator &a) const { return !(*this == a); }

  explicit operator bool() const { return valid(); }

  size_type position() const { return m_offset; }

  void set_position(size_type position) { m_offset = position; }

  /** Is iterator pointing to valid character inside SQL string */
  bool valid() const { return m_offset < m_s.length(); }

  std::string_view next_token();

  std::pair<std::string_view, size_t> next_token_and_offset();

  std::string_view next_sql_function();

  bool inside_hint() const { return m_comment_hint; }

  template <class... Types>
  bool consume_tokens(Types &&... tokens) {
    if (!valid()) return false;
    return (shcore::str_caseeq(next_token(), std::forward<Types>(tokens)) &&
            ...);
  }

 private:
  std::string_view m_s;
  size_type m_offset;
  bool m_skip_quoted;
  bool m_dollar_quotes;
  bool m_comment_hint = false;
};

}  // namespace utils
}  // namespace mysqlshdk

#endif  // MYSQLSHDK_LIBS_UTILS_UTILS_LEXING_H_
