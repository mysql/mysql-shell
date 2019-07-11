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

#ifndef MODULES_REPORTS_UTILS_OPTIONS_H_
#define MODULES_REPORTS_UTILS_OPTIONS_H_

#include <memory>
#include <string>
#include <utility>

namespace mysqlsh {
namespace reports {

/**
 * Utilities which allow to automatically generate options-related code.
 *
 * 1. Define a set of options used by the report:
 * #define OPTIONS                                                             \
 *   XX(name, Type, "Brief description", "Shortcut", {"Detailed description"}, \
 *      required, native_t) \
 *   XX(opt, Bool, "Brief", "o", {"Details"}, false, bool) \
 *   XX(list, Integer, "Brief") \
 *   ...
 *
 *    The first three values (name, type and brief description) are required.
 *
 * 2. In order to generate a function which will return all options use:
 * #define X X_GET_OPTIONS
 *   GET_OPTIONS
 * #undef X
 *
 * 3. In order to generate a structure called 'o' which will hold values of all
 *    options use:
 * #define X X_OPTIONS_STRUCT
 *   OPTIONS_STRUCT
 * #undef X
 *
 * 4. In order to parse all options from dictionary 'options' into structure 'o'
 *    use:
 * #define X X_PARSE_OPTIONS
 *   PARSE_OPTIONS
 * #undef X
 */

namespace detail {
template <shcore::Value_type E>
struct to_type {};

template <>
struct to_type<shcore::Value_type::String> {
  using type = std::string;
};

template <>
struct to_type<shcore::Value_type::Bool> {
  using type = bool;
};

template <>
struct to_type<shcore::Value_type::Integer> {
  using type = int64_t;
};

template <>
struct to_type<shcore::Value_type::Float> {
  using type = double;
};

}  // namespace detail

#define EXPAND(x) x

#define NARGS_(_8, _7, _6, _5, _4, _3, _2, _1, N, ...) N
#define NARGS(...) EXPAND(NARGS_(__VA_ARGS__, 8, 7, 6, 5, 4, 3, 2, 1, 0))

#define CONCAT_(a, b) a##b
#define CONCAT(a, b) CONCAT_(a, b)

#define X_8(_1, _2, _3, _4, _5, _6, _7, _8) X(_1, _2, _3, _4, _5, _6, _7, _8)
#define X_7(_1, t, _3, _4, _5, _6, _7) \
  X_8(_1, t, _3, _4, _5, _6, _7,       \
      mysqlsh::reports::detail::to_type<shcore::Value_type::t>::type)
#define X_6(_1, _2, _3, _4, _5, _6) X_7(_1, _2, _3, _4, _5, _6, false)
#define X_5(_1, _2, _3, _4, _5) X_6(_1, _2, _3, _4, _5, false)
#define X_4(_1, _2, _3, _4) X_5(_1, _2, _3, _4, {})
#define X_3(_1, _2, _3) X_4(_1, _2, _3, "")

#define XX(...) EXPAND(CONCAT(X_, NARGS(__VA_ARGS__))(__VA_ARGS__))

#define X_GET_OPTIONS(n, t, b, s, d, r, e, _8)                               \
  {                                                                          \
    auto n = std::make_shared<Report::Option>(#n, shcore::Value_type::t, r); \
    n->short_name = s;                                                       \
    n->brief = b;                                                            \
    n->details = d;                                                          \
    n->empty = e;                                                            \
    o.emplace_back(std::move(n));                                            \
  }

#define GET_OPTIONS                  \
  static Report::Options options() { \
    Report::Options o;               \
    OPTIONS                          \
    return o;                        \
  }

#define X_OPTIONS_STRUCT(n, _2, _3, _4, _5, _6, _7, et) et n{};

#define OPTIONS_STRUCT       \
  struct options__struct__ { \
    OPTIONS                  \
  };                         \
  options__struct__ o;

#define X_PARSE_OPTIONS(n, _2, _3, _4, _5, r, _7, _8) \
  r ? u.required(#n, &o.n) : u.optional(#n, &o.n);

#define PARSE_OPTIONS                   \
  {                                     \
    shcore::Option_unpacker u(options); \
    OPTIONS                             \
    u.end();                            \
  }

}  // namespace reports
}  // namespace mysqlsh

#endif  // MODULES_REPORTS_UTILS_OPTIONS_H_
