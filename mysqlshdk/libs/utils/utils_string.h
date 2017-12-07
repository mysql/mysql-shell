/*
 * Copyright (c) 2017, Oracle and/or its affiliates. All rights reserved.
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

#ifndef MYSQLSHDK_LIBS_UTILS_UTILS_STRING_H_
#define MYSQLSHDK_LIBS_UTILS_UTILS_STRING_H_

#include <algorithm>
#include <cctype>
#include <cstring>
#include <stdexcept>
#include <string>
#include <vector>
#include <utility>
#include <vector>

#include "scripting/common.h"

namespace shcore {

/** Convert a copy of an ASCII string to uppercase and return */
inline std::string str_upper(const std::string &s) {
  std::string r(s);
  std::transform(r.begin(), r.end(), r.begin(), ::toupper);
  return r;
}

/** Convert a copy of an ASCII string to lowercase and return */
inline std::string str_lower(const std::string &s) {
  std::string r(s);
  std::transform(r.begin(), r.end(), r.begin(), ::tolower);
  return r;
}

/** Compares 2 strings case insensitive (for ascii) */
inline int str_casecmp(const char *a, const char *b) {
#ifdef _WIN32
  return ::_stricmp(a, b);
#else
  return ::strcasecmp(a, b);
#endif
}

inline int str_casecmp(const std::string &a, const std::string &b) {
  return str_casecmp(a.c_str(), b.c_str());
}

inline bool str_caseeq(const char *a, const char *b) {
#ifdef _WIN32
  return ::_stricmp(a, b) == 0;
#else
  return ::strcasecmp(a, b) == 0;
#endif
}

inline bool str_caseeq(const char *a, const char *b, size_t n) {
#ifdef _WIN32
  return ::_strnicmp(a, b, n) == 0;
#else
  return ::strncasecmp(a, b, n) == 0;
#endif
}

/** Checks whether a string has another as a prefix */
inline bool str_beginswith(const char *s, const char *prefix) {
  return strncmp(s, prefix, strlen(prefix)) == 0;
}

inline bool str_beginswith(const std::string &s, const std::string &prefix) {
  return s.compare(0, prefix.length(), prefix) == 0;
}

inline bool str_ibeginswith(const char *s, const char *prefix) {
#ifdef _WIN32
  return ::_strnicmp(s, prefix, strlen(prefix)) == 0;
#else
  return strncasecmp(s, prefix, strlen(prefix)) == 0;
#endif
}

inline bool str_ibeginswith(const std::string &s, const std::string &prefix) {
  #ifdef _WIN32
    return ::_strnicmp(s.c_str(), prefix.c_str(), prefix.size()) == 0;
  #else
    return strncasecmp(s.c_str(), prefix.c_str(), prefix.size()) == 0;
  #endif
}

/** Checks whether a string has another as a suffix */
inline bool str_endswith(const char *s, const char *suffix) {
  size_t l = strlen(suffix);
  size_t sl = strlen(s);
  if (l > sl)
    return false;
  return strncmp(s + sl - l, suffix, l) == 0;
}

inline bool str_endswith(const std::string &s, const std::string &suffix) {
  if (suffix.length() > s.length())
    return false;
  return s.compare(s.length() - suffix.length(), suffix.length(), suffix) == 0;
}

inline bool str_iendswith(const char *s, const char *suffix) {
  size_t l = strlen(suffix);
  size_t sl = strlen(s);
  if (l > sl)
    return false;
#ifdef _WIN32
  return ::_strnicmp(s + sl - l, suffix, l) == 0;
#else
  return strncasecmp(s + sl - l, suffix, l) == 0;
#endif
}

inline bool str_iendswith(const std::string &s, const std::string &suffix) {
  if (suffix.length() > s.length())
    return false;
#ifdef _WIN32
  return ::_strnicmp(s.c_str() + s.length() - suffix.length(), suffix.c_str(),
                     suffix.length()) == 0;
#else
  return strncasecmp(s.c_str() + s.length() - suffix.length(), suffix.c_str(),
                     suffix.length()) == 0;
#endif
}

/** Partition a string in 2 at a separator, if present */
inline std::pair<std::string, std::string> str_partition(
    const std::string &s, const std::string &sep) {
  auto p = s.find(sep);
  if (p == std::string::npos)
    return std::make_pair(s, "");
  else
    return std::make_pair(s.substr(0, p), s.substr(p + sep.length()));
}

/**
 * Splits string based on each of the individual characters of the separator
 * string
 *
 * @param input The string to be split
 * @param separator_chars String containing characters wherein the input string
 *   is split on any of the characters
 * @param maxsplit max number of parts to break into or -1 for no limit
 * @param compress Boolean value which when true ensures consecutive separators
 * do not generate new elements in the split
 *
 * @returns vector of splitted strings
 */
inline std::vector<std::string> str_split(
    const std::string &input, const std::string &separator_chars = " \r\n\t",
    int maxsplit = -1, bool compress = false) {
  std::vector<std::string> ret_val;
  size_t index = 0, new_find = 0;

  while (new_find != std::string::npos) {
    if (maxsplit--)
      new_find = input.find_first_of(separator_chars, index);
    else
      new_find = std::string::npos;

    if (new_find != std::string::npos) {
      // When compress is enabled, consecutive separators
      // do not generate new elements
      if (new_find > index || !compress || new_find == 0)
        ret_val.push_back(input.substr(index, new_find - index));

      index = new_find + 1;
    } else {
      ret_val.push_back(input.substr(index));
    }
  }
  return ret_val;
}

/** Strip a string out of blank chars */
std::string SHCORE_PUBLIC str_strip(const std::string &s,
                                    const std::string &chars = " \r\n\t");
std::string SHCORE_PUBLIC str_lstrip(const std::string &s,
                                     const std::string &chars = " \r\n\t");
std::string SHCORE_PUBLIC str_rstrip(const std::string &s,
                                     const std::string &chars = " \r\n\t");

/** Return a formatted a string

  Throws invalid_argument on encoding error
*/
#if __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ > 4)
std::string SHCORE_PUBLIC str_format(const char *formats, ...)
    __attribute__((__format__(__printf__, 1, 2)));
#elif _MSC_VER
std::string SHCORE_PUBLIC
str_format(_In_z_ _Printf_format_string_ const char *format, ...);
#else
std::string SHCORE_PUBLIC str_format(const char *formats, ...);
#endif

template <typename Iter>
std::string SHCORE_PUBLIC str_join(Iter begin, Iter end,
                                   const std::string &sep) {
  std::string s;
  if (begin != end) {
    s.append(*begin);
    while (++begin != end) {
      s.append(sep);
      s.append(*begin);
    }
  }
  return s;
}

template <typename C>
std::string SHCORE_PUBLIC str_join(const C &container, const std::string &sep) {
  return str_join(container.begin(), container.end(), sep);
}

std::string SHCORE_PUBLIC str_replace(const std::string &s,
                                      const std::string &from,
                                      const std::string &to);


std::string SHCORE_PUBLIC bits_to_string(uint64_t bits, int nbits);
std::pair<uint64_t, int> SHCORE_PUBLIC string_to_bits(const std::string &s);

/**
 * Escape `quote` and `\` chars.
 *
 * @param s String to escape.
 * @param quote `'` `"`
 * @return Quote escaped string.
 */
std::string quote_string(const std::string &s, char quote);

// Macro to turn a symbol into a string
#define STRINGIFY(s) STRINGIFY_(s)
#define STRINGIFY_(s) #s


}  // namespace shcore

#endif  // MYSQLSHDK_LIBS_UTILS_UTILS_STRING_H_
