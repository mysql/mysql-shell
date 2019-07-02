/*
 * Copyright (c) 2019, Oracle and/or its affiliates. All rights reserved.
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

#ifndef MYSQLSHDK_LIBS_UTILS_STRUCTURED_TEXT_H_
#define MYSQLSHDK_LIBS_UTILS_STRUCTURED_TEXT_H_

#include <cstring>
#include <string>
#include "mysqlshdk/libs/utils/utils_string.h"

namespace shcore {
namespace internal {
#ifdef _WIN32
#define MSVC_WORKAROUND extern const __declspec(selectany)
#else
#define MSVC_WORKAROUND constexpr
#endif

MSVC_WORKAROUND char k_kvp_allowed_chars[] =
    "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_.$@";
}  // namespace internal

// Chars that don't need to be quoted in a KVP key or value

inline bool is_kvp_token(const char *s) {
  size_t p = strspn(s, internal::k_kvp_allowed_chars);
  return s[p] == 0;
}

/**
 * Produces a KVP pair, quoting strings when necessary.
 *
 * Use ' for quotes by default, so that they don't get escaped in a JSON string.
 */
inline std::string make_kvp(const char *key, const std::string &value,
                            int quote = '\'') {
  std::string s =
      !is_kvp_token(key) ? quote_string(key, quote) : std::string(key);
  s += "=";
  s += !is_kvp_token(value.c_str()) ? quote_string(value, quote) : value;
  return s;
}

inline std::string make_kvp(const char *key, const char *value,
                            int quote = '\'') {
  std::string s =
      !is_kvp_token(key) ? quote_string(key, quote) : std::string(key);
  s += "=";
  s += !is_kvp_token(value) ? quote_string(value, quote) : value;
  return s;
}

template <typename T>
inline std::string make_kvp(const char *key, T value, int quote = '\'') {
  std::string s =
      !is_kvp_token(key) ? quote_string(key, quote) : std::string(key);
  s += "=";
  s += std::to_string(value);
  return s;
}

}  // namespace shcore

#endif  // MYSQLSHDK_LIBS_UTILS_STRUCTURED_TEXT_H_
