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

#ifndef MYSQLSHDK_LIBS_UTILS_UTILS_FILTERING_H_
#define MYSQLSHDK_LIBS_UTILS_UTILS_FILTERING_H_

#include <functional>
#include <memory>
#include <string>

#include "mysqlshdk/include/shellcore/scoped_contexts.h"
#include "mysqlshdk/libs/utils/utils_sqlstring.h"

namespace mysqlshdk {
namespace utils {
namespace detail {
template <typename T, typename R = T>
const R &key(const T &t) {
  return t;
}

template <typename T, typename R = T>
const R &value(const T &t) {
  return t;
}

template <typename T1, typename T2>
const T1 &key(const std::pair<T1, T2> &t) {
  return t.first;
}

template <typename T1, typename T2>
const T2 &value(const std::pair<T1, T2> &t) {
  return t.second;
}
}  // namespace detail

template <typename T, typename U = T>
void find_matches(const T &included, const T &excluded,
                  const std::string &prefix,
                  const std::function<void(const U &, const U &,
                                           const std::string &)> &callback) {
  const auto included_is_smaller = included.size() < excluded.size();
  const auto &needle = included_is_smaller ? included : excluded;
  const auto &haystack = !included_is_smaller ? included : excluded;

  for (const auto &object : needle) {
    const auto &k = detail::key(object);
    const auto found = haystack.find(k);

    if (haystack.end() != found) {
      auto full_name = prefix;

      if (!full_name.empty()) {
        full_name += ".";
      }

      full_name += shcore::quote_identifier(k);

      callback(detail::value(included_is_smaller ? object : *found),
               detail::value(!included_is_smaller ? object : *found),
               full_name);
    }
  }
}

template <typename T, typename U = T>
bool error_on_conflicts(const T &included, const T &excluded,
                        const std::string &object_label,
                        const std::string &option_suffix,
                        const std::string &schema_name) {
  bool has_conflicts = false;

  find_matches<T, U>(included, excluded, schema_name,
                     [&object_label, &option_suffix, &has_conflicts](
                         const auto &, auto &, const std::string &full_name) {
                       has_conflicts = true;
                       mysqlsh::current_console()->print_error(
                           "Both include" + option_suffix + " and exclude" +
                           option_suffix + " options contain " + object_label +
                           " " + full_name + ".");
                     });

  return has_conflicts;
}

}  // namespace utils
}  // namespace mysqlshdk

#endif  // MYSQLSHDK_LIBS_UTILS_UTILS_FILTERING_H_