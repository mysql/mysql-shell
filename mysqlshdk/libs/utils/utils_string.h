/*
 * Copyright (c) 2017, 2020, Oracle and/or its affiliates. All rights reserved.
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
#include <functional>
#include <stdexcept>
#include <string>
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

struct Case_insensitive_comparator {
  bool operator()(const std::string &a, const std::string &b) const {
    return str_casecmp(a, b) < 0;
  }
};

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

inline bool str_caseeq(const std::string &a, const std::string &b) {
  return a.length() == b.length() && str_caseeq(a.c_str(), b.c_str());
}

template <typename T>
inline bool str_caseeq_mv(const T &) {
  return false;
}

template <typename T, typename... Args>
bool str_caseeq_mv(const std::string &token, const T &val, Args... args) {
  return str_caseeq(token.c_str(), val) ||
         str_caseeq_mv(token.c_str(), args...);
}

template <typename T, typename... Args>
bool str_caseeq_mv(const char *token, const T &val, Args... args) {
  return str_caseeq(token, val) || str_caseeq_mv(token, args...);
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
  if (l > sl) return false;
  return strncmp(s + sl - l, suffix, l) == 0;
}

inline bool str_endswith(const std::string &s, const std::string &suffix) {
  if (suffix.length() > s.length()) return false;
  return s.compare(s.length() - suffix.length(), suffix.length(), suffix) == 0;
}

template <typename... T>
bool str_endswith(const char *s, const char *suffix, T &&... suffixes) {
  return str_endswith(s, suffix) ||
         str_endswith(s, std::forward<T>(suffixes)...);
}

template <typename... T>
bool str_endswith(const std::string &s, const std::string &suffix,
                  T &&... suffixes) {
  return str_endswith(s, suffix) ||
         str_endswith(s, std::forward<T>(suffixes)...);
}

inline bool str_iendswith(const char *s, const char *suffix) {
  size_t l = strlen(suffix);
  size_t sl = strlen(s);
  if (l > sl) return false;
#ifdef _WIN32
  return ::_strnicmp(s + sl - l, suffix, l) == 0;
#else
  return strncasecmp(s + sl - l, suffix, l) == 0;
#endif
}

inline bool str_iendswith(const std::string &s, const std::string &suffix) {
  if (suffix.length() > s.length()) return false;
#ifdef _WIN32
  return ::_strnicmp(s.c_str() + s.length() - suffix.length(), suffix.c_str(),
                     suffix.length()) == 0;
#else
  return strncasecmp(s.c_str() + s.length() - suffix.length(), suffix.c_str(),
                     suffix.length()) == 0;
#endif
}

template <typename... T>
bool str_iendswith(const char *s, const char *suffix, T &&... suffixes) {
  return str_iendswith(s, suffix) ||
         str_iendswith(s, std::forward<T>(suffixes)...);
}

template <typename... T>
bool str_iendswith(const std::string &s, const std::string &suffix,
                   T &&... suffixes) {
  return str_iendswith(s, suffix) ||
         str_iendswith(s, std::forward<T>(suffixes)...);
}

/** Return position of the first difference in the strings or npos if they're
 * the same */
inline size_t str_span(const std::string &s1, const std::string &s2) {
  size_t p = 0;
  while (p < s1.size() && p < s2.size()) {
    if (s1[p] != s2[p]) return p;
    ++p;
  }
  if (p == s1.size() && p == s2.size()) return std::string::npos;
  return p;
}

/** Partition a string in 2 at a separator, if present */
inline std::pair<std::string, std::string> str_partition(
    const std::string &s, const std::string &sep, bool *found_sep = nullptr) {
  auto p = s.find(sep);

  if (found_sep) {
    *found_sep = p != std::string::npos;
  }

  if (p == std::string::npos)
    return std::make_pair(s, "");
  else
    return std::make_pair(s.substr(0, p), s.substr(p + sep.length()));
}

/** Partition a string in 2 after separator, in place, if present */
inline std::pair<std::string, std::string> str_partition_after(
    const std::string &s, const std::string &sep) {
  auto p = s.find(sep);
  if (p == std::string::npos) {
    return std::make_pair(s, "");
  } else {
    p += sep.length();
    return std::make_pair(s.substr(0, p), s.substr(p));
  }
}

/** Partition a string in 2 after separator, in place, if present */
inline std::string str_partition_after_inpl(std::string *s,
                                            const std::string &sep) {
  auto p = s->find(sep);
  if (p == std::string::npos) {
    std::string tmp = *s;
    s->clear();
    return tmp;
  } else {
    std::string tmp = s->substr(0, p + sep.length());
    s->erase(0, p + sep.length());
    return tmp;
  }
}

/**
 * Splits string based on each of the individual characters of the separator
 * string
 *
 * @param input The string to be split
 * @param separator_chars String containing characters wherein the input string
 *   is split on any of the characters
 * @param maxsplit max number of times to split or -1 for no limit
 * @param compress Boolean value which when true ensures consecutive separators
 * do not generate new elements in the split, but they're still counted towards
 * maxsplit.
 *
 * @returns vector of splitted strings
 */
inline std::vector<std::string> str_split(
    const std::string &input, const std::string &separator_chars = " \r\n\t",
    int maxsplit = -1, bool compress = false) {
  std::vector<std::string> ret_val;
  size_t index = 0, new_find = 0;
  const size_t end = input.size();

  while (new_find < end) {
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

/**
 * Split the given input string and call the functor for each token.
 *
 * @param input the string to be split
 * @param f a functor called once for each split string. If its return value
 *   is false, the splitting is interrupted.
 * @param separator_chars String containing characters wherein the input string
 *   is split on any of the characters
 * @param maxsplit max number of times to split or -1 for no limit
 * @param compress Boolean value which when true ensures consecutive separators
 *   do not generate new elements in the split, but they're still counted
 * towards maxsplit.
 *
 * @returns false if f returns false, true otherwise
 */
inline bool str_itersplit(const std::string &input,
                          const std::function<bool(const std::string &s)> &f,
                          const std::string &separator_chars = " \r\n\t",
                          int maxsplit = -1, bool compress = false) {
  size_t index = 0, new_find = 0;
  const size_t end = input.size();

  while (new_find < end) {
    if (maxsplit--)
      new_find = input.find_first_of(separator_chars, index);
    else
      new_find = std::string::npos;

    if (new_find != std::string::npos) {
      // When compress is enabled, consecutive separators
      // do not generate new elements
      if (new_find > index || !compress || new_find == 0)
        if (!f(input.substr(index, new_find - index))) return false;

      index = new_find + 1;
    } else {
      if (!f(input.substr(index))) return false;
    }
  }
  return true;
}

/** Strip a string out of blank chars */
std::string SHCORE_PUBLIC str_strip(const std::string &s,
                                    const std::string &chars = " \r\n\t");
std::string SHCORE_PUBLIC str_lstrip(const std::string &s,
                                     const std::string &chars = " \r\n\t");
std::string SHCORE_PUBLIC str_rstrip(const std::string &s,
                                     const std::string &chars = " \r\n\t");

inline std::string str_ljust(const std::string &s, size_t width,
                             char pad = ' ') {
  if (s.size() < width) return s + std::string(width - s.size(), pad);
  return s;
}

inline std::string str_rjust(const std::string &s, size_t width,
                             char pad = ' ') {
  if (s.size() < width) return std::string(width - s.size(), pad).append(s);
  return s;
}

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
inline std::string str_join(Iter begin, Iter end, const std::string &sep) {
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

template <typename Iter>
inline std::string str_join(
    Iter begin, Iter end, const std::string &sep,
    std::function<std::string(const typename Iter::value_type &i)> f) {
  std::string s;
  if (begin != end) {
    s.append(f(*begin));
    while (++begin != end) {
      s.append(sep);
      s.append(f(*begin));
    }
  }
  return s;
}

template <typename C>
inline std::string str_join(const C &container, const std::string &sep) {
  return str_join(container.begin(), container.end(), sep);
}

template <typename C>
inline std::string str_join(
    const C &container, const std::string &sep,
    std::function<std::string(const typename C::value_type &i)> f) {
  return str_join(container.begin(), container.end(), sep, f);
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

/**
 * Inverse of quote_string().
 *
 * If the first and the last characters in the given strings match `quote`
 * they are removed as well.
 *
 * @param s String to be processed.
 * @param quote The quote character (`'`, `"`)
 *
 * @return Unquoted string.
 */
std::string unquote_string(const std::string &s, char quote);

// Macro to turn a symbol into a string
#define STRINGIFY(s) STRINGIFY_(s)
#define STRINGIFY_(s) #s

/** Breaks string into lines of specified width without breaking words.
 *
 * @param line long string to break.
 * @param line_width maximum line width
 * @return vector with split lines.
 */
std::vector<std::string> str_break_into_lines(const std::string &line,
                                              std::size_t line_width);

/**
 * Auxiliary function to get the quotes span (i.e., start and end positions)
 * for the given string.
 *
 * If not quote is found then std::string::npos is returned for both elements
 * in the pair. If only one quote is found (no ending quote) then
 * std::string::npos is returned for the second position of the pair.
 *
 * @param quote_char character with the quote to look for.
 * @param str target string to get the start and end quote position.
 * @return return a pair with the position of the starting quote and ending
 *         quote.
 */
std::pair<std::string::size_type, std::string::size_type> get_quote_span(
    const char quote_char, const std::string &str);

/**
 * Convert UTF-8 string to UTF-16/UTF-32 (platform dependent) string.
 *
 * @param utf8 UTF-8 encoded string.
 * @return std::wstring UTF-16/UTF-32 (platform dependent) string.
 */
std::wstring utf8_to_wide(const std::string &utf8);

/**
 * Convert UTF-8 string to UTF-16/UTF-32 (platform dependent) string.
 *
 * @param utf8 Pointer to UTF-8 encoded string.
 * @param utf8_length Length of UTF-8 string in bytes.
 * @return std::wstring UTF-16/UTF-32 (platform dependent) string.
 */
std::wstring utf8_to_wide(const char *utf8, const size_t utf8_length);

/**
 * Convert UTF-8 string to UTF-16/UTF-32 (platform dependent) string.
 *
 * @param utf8 Pointer to UTF-8 encoded string.
 * @return std::wstring UTF-16/UTF-32 (platform dependent) string.
 */
std::wstring utf8_to_wide(const char *utf8);

/**
 * Convert UTF-16/UTF-32 (platform dependent) string to UTF-8 string.
 *
 * @param wide UTF-16/UTF-32 (platform dependent) encoded string.
 * @return std::string UTF-8 encoded string.
 */
std::string wide_to_utf8(const std::wstring &wide);

/**
 * Convert UTF-16/UTF-32 (platform dependent) string to UTF-8 string.
 *
 * @param wide Pointer to UTF-16/UTF-32 (platform dependent) encoded string.
 * @param wide_length Length of UTF-16/UTF-32 string in bytes.
 * @return std::string UTF-8 encoded string.
 */
std::string wide_to_utf8(const wchar_t *wide, const size_t wide_length);

/**
 * Convert UTF-16/UTF-32 (platform dependent) string to UTF-8 string.
 *
 * @param wide Pointer to UTF-16/UTF-32 (platform dependent) encoded string.
 * @return std::string UTF-8 encoded string.
 */
std::string wide_to_utf8(const wchar_t *wide);

/**
 * Truncates the given string to max_length code points.
 *
 * @param str UTF-8 string to be truncated.
 * @param max_length Maximum number of code points.
 *
 * @return Input string truncated to max_length code points.
 */
std::string truncate(const std::string &str, const size_t max_length);

/**
 * Truncates the given string to max_length code points.
 *
 * @param str UTF-8 string to be truncated.
 * @param length Length of string in bytes.
 * @param max_length Maximum number of code points.
 *
 * @return Input string truncated to max_length code points.
 */
std::string truncate(const char *str, const size_t length,
                     const size_t max_length);

/**
 * Truncates the given string to max_length code points.
 *
 * @param str UTF-16/UTF-32 string to be truncated.
 * @param max_length Maximum number of code points.
 *
 * @return Input string truncated to max_length code points.
 */
std::wstring truncate(const std::wstring &str, const size_t max_length);

/**
 * Truncates the given string to max_length code points.
 *
 * @param str UTF-16/UTF-32 string to be truncated.
 * @param length Length of string in bytes.
 * @param max_length Maximum number of code points.
 *
 * @return Input string truncated to max_length code points.
 */
std::wstring truncate(const wchar_t *str, const size_t length,
                      const size_t max_length);

/**
 * Checks if the given string contains only valid UTF-8 code points.
 *
 * @param s String to be checked.
 *
 * @returns true if the given string is a valid UTF-8 string
 */
bool is_valid_utf8(const std::string &s);

/**
 * Generates a percent encoded string based on RFC-3986
 */
std::string pctencode(const std::string &s);

/**
 * Decodes a string that is percent encoded based on RFC-3986
 */
std::string pctdecode(const std::string &s);

/**
 * Returns a string of the given size created with random characters from the
 * provided source.
 */
std::string get_random_string(size_t size, const char *source);

}  // namespace shcore

#endif  // MYSQLSHDK_LIBS_UTILS_UTILS_STRING_H_
