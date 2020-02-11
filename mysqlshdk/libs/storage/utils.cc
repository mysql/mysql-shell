/*
 * Copyright (c) 2020, Oracle and/or its affiliates. All rights reserved.
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

#include "mysqlshdk/libs/storage/utils.h"

#include <stdexcept>

#include "mysqlshdk/libs/utils/utils_string.h"

namespace mysqlshdk {
namespace storage {
namespace utils {

namespace {

constexpr auto k_scheme_delimiter = "://";
constexpr std::size_t k_scheme_delimiter_length = 3;

}  // namespace

std::string get_scheme(const std::string &uri) {
  const auto pos = uri.find(k_scheme_delimiter);

  if (std::string::npos != pos) {
    if (0 == pos) {
      throw std::invalid_argument("URI " + uri + " has an empty scheme.");
    }

    return uri.substr(0, pos);
  } else {
    return "";
  }
}

std::string strip_scheme(const std::string &uri, const std::string &scheme) {
  const auto uri_scheme = get_scheme(uri);

  if (uri_scheme.empty()) {
    return uri;
  } else {
    if (!scheme.empty() && !scheme_matches(uri_scheme, scheme.c_str())) {
      throw std::invalid_argument(
          "URI " + uri + " has an invalid scheme, expected: " + scheme + ".");
    }

    return uri.substr(uri_scheme.length() + k_scheme_delimiter_length);
  }
}

bool scheme_matches(const std::string &scheme, const char *expected) {
  return shcore::str_caseeq(scheme.c_str(), expected);
}

}  // namespace utils
}  // namespace storage
}  // namespace mysqlshdk
