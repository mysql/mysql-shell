/*
 * Copyright (c) 2015, 2016, Oracle and/or its affiliates. All rights reserved.
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

#ifndef _UTILS_SQLSTRING_H_
#define _UTILS_SQLSTRING_H_

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS 1
#endif
#include <inttypes.h>
#include <string>

#include "scripting/common.h"

#include <boost/utility/enable_if.hpp>
#include <boost/type_traits/is_signed.hpp>
#include <stdexcept>

namespace shcore {
enum SqlStringFlags {
  QuoteOnlyIfNeeded = 1 << 0,
  UseAnsiQuotes = 1 << 1
};

SHCORE_PUBLIC std::string escape_sql_string(const std::string &string, bool wildcards = false); // "strings" or 'strings'
SHCORE_PUBLIC std::string escape_backticks(const std::string &string);  // `identifier`
SHCORE_PUBLIC std::string quote_identifier(const std::string& identifier, const char quote_char);
SHCORE_PUBLIC std::string quote_identifier_if_needed(const std::string &ident, const char quote_char);

class SHCORE_PUBLIC sqlstring {
public:
  struct sqlstringformat {
    int _flags;
    sqlstringformat(const int flags) : _flags(flags) {}
  };

private:
  std::string _formatted;
  std::string _format_string_left;
  sqlstringformat _format;

  std::string consume_until_next_escape();
  int next_escape();

  sqlstring& append(const std::string &s);
public:
  static const sqlstring null;

  sqlstring();
  sqlstring(const char* format_string, const sqlstringformat format);
  sqlstring(const sqlstring &copy);
  bool done() const;

  operator std::string() const;
  std::string str() const;

  //! modifies formatting options
  sqlstring &operator <<(const sqlstringformat);

  //! replaces a ? in the format string with any integer numeric value
  template<typename T>
  typename boost::enable_if_c<boost::is_integral<T>::value, sqlstring &>::type
  operator <<(const T value) {
    int esc = next_escape();
    if (esc != '?')
        throw std::invalid_argument("Error formatting SQL query: invalid escape for numeric argument");
    append(std::to_string(value));
    append(consume_until_next_escape());
    return *this;
  }
  //! replaces a ? in the format string with a float numeric value
  sqlstring &operator <<(const float val) { return operator<<((double)val); }
  //! replaces a ? in the format string with a double numeric value
  sqlstring &operator <<(const double);
  //! replaces a ? in the format string with a quoted string value or ! with a back-quoted identifier value
  sqlstring &operator <<(const std::string&);
  //! replaces a ? in the format string with a quoted string value or ! with a back-quoted identifier value
  //! is the value is NULL, ? will be replaced with a NULL. ! will raise an exception
  sqlstring &operator <<(const char*);
  //! replaces a ? or ! with the content of the other string verbatim
  sqlstring &operator <<(const sqlstring&);
};
};

#endif
