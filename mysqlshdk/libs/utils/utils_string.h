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

#ifndef __mysh__utils_string__
#define __mysh__utils_string__

#include <string>
#include <algorithm>
#include <cctype>
#include <cstring>
#include <stdexcept>

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


inline bool str_caseeq(const char *a, const char *b) {
#ifdef _WIN32
  return ::_stricmp(a, b) == 0;
#else
  return ::strcasecmp(a, b) == 0;
#endif
}

/** Checks whether a string has another as a prefix */
inline bool str_beginswith(const char *s, const char *prefix) {
  return strncmp(s, prefix, strlen(prefix)) == 0;
}

inline bool str_beginswith(const std::string &s, const char *prefix) {
  return s.compare(0, strlen(prefix), prefix) == 0;
}

inline bool str_beginswith(const std::string &s, const std::string &prefix) {
  return s.compare(0, prefix.length(), prefix) == 0;
}

/** Checks whether a string has another as a suffix */
inline bool str_endswith(const char *s, const char *suffix) {
  size_t l = strlen(suffix);
  size_t sl = strlen(s);
  if (l > sl)
    return false;
  return strncmp(s + sl - l, suffix, l) == 0;
}

inline bool str_endswith(const std::string &s, const char *suffix) {
  size_t l = strlen(suffix);
  if (l > s.length())
    return false;
  return s.compare(s.length() - l, l, suffix) == 0;
}

inline bool str_endswith(const std::string &s, const std::string &suffix) {
  if (suffix.length() > s.length())
    return false;
  return s.compare(s.length() - suffix.length(), suffix.length(), suffix) == 0;
}

/** Partition a string in 2 at a separator, if present */
inline std::pair<std::string, std::string> str_partition(const std::string &s, const std::string &sep) {
  auto p = s.find(sep);
  if (p == std::string::npos)
    return std::make_pair(s, "");
  else
    return std::make_pair(s.substr(0, p), s.substr(p+sep.length()));
}

/** Strip a string out of blank chars */
std::string SHCORE_PUBLIC str_strip(const std::string &s, const std::string &chars = " \r\n\t");
std::string SHCORE_PUBLIC str_lstrip(const std::string &s, const std::string &chars = " \r\n\t");
std::string SHCORE_PUBLIC str_rstrip(const std::string &s, const std::string &chars = " \r\n\t");

/** Return a formatted a string

  Throws invalid_argument on encoding error
*/
#if __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ > 4)
std::string SHCORE_PUBLIC str_format(const char* formats, ...) __attribute__((__format__(__printf__, 1, 2)));
#elif _MSC_VER
std::string SHCORE_PUBLIC str_format(_In_z_ _Printf_format_string_ const char* format, ...);
#else
std::string SHCORE_PUBLIC str_format(const char* formats, ...);
#endif

}

#endif /* defined(__mysh__utils_string__) */
