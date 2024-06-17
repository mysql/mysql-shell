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

#ifndef _UTILS_SQLSTRING_H_
#define _UTILS_SQLSTRING_H_

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS 1
#endif
#include <inttypes.h>
#include <string>
#include <string_view>

#include "scripting/common.h"

#include <stdexcept>
#include <type_traits>

namespace shcore {
enum SqlStringFlags { QuoteOnlyIfNeeded = 1 << 0 };

SHCORE_PUBLIC std::string escape_sql_string(
    std::string_view string,
    bool wildcards = false);  // "strings" or 'strings'
SHCORE_PUBLIC std::string escape_backticks(
    std::string_view string);  // `identifier`
/**
 * Escapes the SQL wildcard characters ('%', '_') in the given string.
 *
 * @param string A string to be processed.
 *
 * @return The input string with all the wildcard characters escaped.
 */
SHCORE_PUBLIC std::string escape_wildcards(std::string_view string);

SHCORE_PUBLIC std::string quote_sql_string(std::string_view identifier);

SHCORE_PUBLIC std::string quote_identifier(std::string_view identifier,
                                           char q = '`');
SHCORE_PUBLIC std::string quote_identifier_if_needed(std::string_view ident,
                                                     char q = '`');

/**
 * Checks if the given string contains a MySQL wildcard character (escaped or
 * not).
 *
 * @param s String to be checked.
 *
 * @returns true If the given string contains a wildcard.
 */
SHCORE_PUBLIC bool has_sql_wildcard(std::string_view s);

/**
 * Checks if the given string contains an unescaped MySQL wildcard character.
 *
 * @param s String to be checked.
 *
 * @returns true If the given string contains an unescaped wildcard.
 */
SHCORE_PUBLIC bool has_unescaped_sql_wildcard(std::string_view s);

/**
 * Case-sensitive SQL wildcard match.
 *
 * @param str Input matched against pattern.
 * @param pattern Pattern with wildcards.
 *
 * @returns true if str matches pattern
 *
 * @note MySQL uses case-insensitive wildcard match in most cases, this one is
 * used (only?) in case of grant privileges at database level.
 */
SHCORE_PUBLIC bool match_sql_wild(std::string_view str,
                                  std::string_view pattern);

/**
 * Maintains the following order of strings:
 *  1. no wildcards
 *  2. strings containing wildcards and non-wildcard characters
 *  3. single multi-wildcard character('%')
 *  4. empty string
 * NOTE: order within groups is not lexicographic, second group is ordered by
 *       the position of the first wildcard character, descending
 */
struct SHCORE_PUBLIC SQL_wild_compare {
  bool operator()(const std::string &a, const std::string &b) const;
};

class SHCORE_PUBLIC sqlstring {
 public:
  struct sqlstringformat final {
    explicit sqlstringformat(const int flags) : _flags(flags) {}
    int _flags = 0;
  };

 private:
  mutable std::string _formatted;
  mutable std::string _format_string_left;
  sqlstringformat _format;

  std::string consume_until_next_escape();
  int next_escape();

  sqlstring &append(std::string_view s);

 public:
  static const sqlstring null;

  sqlstring();
  sqlstring(const char *format_string, const sqlstringformat format);
  sqlstring(std::string format_string, const sqlstringformat format);
  sqlstring(const char *format_string, const int format)
      : sqlstring(format_string, sqlstringformat(format)) {}
  sqlstring(const std::string &format_string, const int format)
      : sqlstring(format_string, sqlstringformat(format)) {}
  sqlstring(const sqlstring &copy);
  void done() const;

  operator std::string() const;
  explicit operator std::string_view() const;
  sqlstring &operator=(const sqlstring &) = default;
  std::string str() const;
  std::string_view str_view() const;
  std::size_t size() const;

  //! modifies formatting options
  sqlstring &operator<<(const sqlstringformat);

  //! replaces a ? in the format string with any integer numeric value
  template <typename T>
  typename std::enable_if<std::is_integral<T>::value, sqlstring &>::type
  operator<<(const T value) {
    int esc = next_escape();
    if (esc != '?')
      throw std::invalid_argument(
          "Error formatting SQL query: invalid escape for numeric argument");
    append(std::to_string(value));
    append(consume_until_next_escape());
    return *this;
  }
  //! replaces a ? in the format string with a float numeric value
  sqlstring &operator<<(const float val) { return operator<<((double)val); }
  //! replaces a ? in the format string with a double numeric value
  sqlstring &operator<<(const double);
  //! replaces a ? in the format string with a quoted string value or ! with a
  //! back-quoted identifier value
  sqlstring &operator<<(std::string_view);
  //! replaces a ? in the format string with a quoted string value or ! with a
  //! back-quoted identifier value is the value is NULL, ? will be replaced with
  //! a NULL. ! will raise an exception
  sqlstring &operator<<(const char *);
  //! replaces a ? or ! with the content of the other string verbatim
  sqlstring &operator<<(const sqlstring &);
};

/**
 * Variadic query formatter
 * @param s    format string with placeholders, as in sqlstring
 * @param args values to substitute for placeholders
 *
 * @return query string with placeholders substituted
 */
template <typename... Args>
inline std::string sqlformat(std::string s, Args &&...args) {
  sqlstring sqls(std::move(s), 0);

  if constexpr (sizeof...(args) > 0) (sqls << ... << args);

  sqls.done();

  return sqls.str();
}

}  // namespace shcore

inline shcore::sqlstring operator"" _sql(const char *str, std::size_t length) {
  return shcore::sqlstring(std::string(str, length), 0);
}

#endif
