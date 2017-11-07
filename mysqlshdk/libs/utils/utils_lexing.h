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

#ifndef MYSQLSHDK_LIBS_UTILS_UTILS_LEXING_H_
#define MYSQLSHDK_LIBS_UTILS_UTILS_LEXING_H_

#include <cassert>
#include <string>

namespace mysqlshdk {
namespace utils {

namespace internal {
// lookup table of number of bytes to skip when spanning a quoted string
// (single quote and double quote versions)
// skip 2 for escape, 0 for '\0', 0 for end quote and 1 for the rest
constexpr char k_quoted_string_span_skips_sq[256] = {
    0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};

constexpr char k_quoted_string_span_skips_dq[256] = {
    0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};
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
inline size_t span_quoted_string_dq(const std::string &s, size_t offset) {
  assert(!s.empty());
  assert(offset < s.length());
  assert(s[offset] == '"');
  assert(s[s.size()] == '\0');

  // skip opening quote
  ++offset;

  int last_ch = 0;
  for (;;) {
    // Tight-loop to span until terminating 0 or closing quote
    while (internal::k_quoted_string_span_skips_dq[last_ch = s[offset]] > 0) {
      offset += internal::k_quoted_string_span_skips_dq[last_ch];
    }
    // Now check why we exited the loop
    if (last_ch == '\0') {
      // there was a '\0', but the string is not over, continue spanning
      if (offset < s.length()) {
        ++offset;
        continue;
      }
      // if this was indeed the terminator, it means we exited the loop
      // without seeing a "
      return std::string::npos;
    }
    // the only other possible way to exit the loop must be the end quote
    assert(last_ch == '"');
    return offset + 1;
  }
}

inline size_t span_quoted_string_sq(const std::string &s, size_t offset) {
  assert(!s.empty());
  assert(offset < s.length());
  assert(s[offset] == '\'');
  assert(s[s.size()] == '\0');

  // skip opening quote
  ++offset;

  int last_ch = 0;
  for (;;) {
    // Tight-loop to span until terminating 0 or closing quote
    while (internal::k_quoted_string_span_skips_sq[last_ch = s[offset]] > 0) {
      offset += internal::k_quoted_string_span_skips_sq[last_ch];
    }
    // Now check why we exited the loop
    if (last_ch == '\0') {
      // there was a '\0', but the string is not over, continue spanning
      if (offset < s.length()) {
        ++offset;
        continue;
      }
      // if this was indeed the terminator, it means we exited the loop
      // without seeing a '
      return std::string::npos;
    }
    // the only other possible way to exit the loop must be the end quote
    assert(last_ch == '\'');
    return offset + 1;
  }
}

inline size_t span_to_eol(const std::string &s, size_t offset) {
  offset = s.find('\n', offset);
  if (offset == std::string::npos)
    return s.length();
  return offset;
}

inline size_t span_cstyle_comment(const std::string &s, size_t offset) {
  assert(!s.empty());
  assert(s[offset] == '/' && s[offset + 1] == '*');

  offset += 2;
  size_t p = s.find("*/", offset);
  if (p == std::string::npos)
    return std::string::npos;
  return p + 2;
}

}  // namespace utils
}  // namespace mysqlshdk

#endif  // MYSQLSHDK_LIBS_UTILS_UTILS_LEXING_H_
