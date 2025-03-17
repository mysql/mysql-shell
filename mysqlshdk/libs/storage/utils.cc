/*
 * Copyright (c) 2020, 2025, Oracle and/or its affiliates.
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

#include "mysqlshdk/libs/storage/utils.h"

#include <stdexcept>
#include <string_view>
#include <utility>
#include <vector>

#include "mysqlshdk/libs/db/uri_encoder.h"
#include "mysqlshdk/libs/utils/utils_string.h"

namespace mysqlshdk {
namespace storage {
namespace utils {

namespace {

constexpr std::string_view k_scheme_delimiter = "://";

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

    return uri.substr(uri_scheme.length() + k_scheme_delimiter.length());
  }
}

bool scheme_matches(const std::string &scheme, const char *expected) {
  return shcore::str_caseeq(scheme.c_str(), expected);
}

namespace {

std::string remove_dot_segments(const std::string &path) {
  if (path.empty()) {
    return {};
  }

  std::vector<std::string> intermediate;

  for (auto &segment : shcore::str_split(path, "/")) {
    if (segment.empty() || "." == segment) {
      continue;
    }

    if (".." == segment) {
      if (intermediate.empty()) {
        throw std::runtime_error{"Invalid URI path: " + path};
      } else {
        intermediate.pop_back();
      }
    } else {
      intermediate.emplace_back(std::move(segment));
    }
  }

  std::string output;

  if ('/' == path.front()) {
    output += '/';
  }

  output += shcore::str_join(intermediate, "/");

  if ((output.empty() || '/' != output.back()) && '/' == path.back()) {
    output += '/';
  }

  return output;
}

}  // namespace

std::string join_uri_path(const std::string &l, const std::string &r,
                          bool pct_encode) {
  std::string joined;
  joined.reserve(l.length() + 1 + r.length());

  if (!l.empty()) {
    joined += l;
    joined += '/';
  }

  if (pct_encode) {
    joined += db::uri::pctencode_path(r);
  } else {
    joined += r;
  }

  if (const auto stripped = strip_scheme(joined);
      stripped.length() == joined.length()) {
    // no scheme
    return remove_dot_segments(joined);
  } else {
    // find beginning of the path part
    if (auto pos = stripped.find('/'); std::string::npos == pos) {
      // no path
      return joined;
    } else {
      // move the position by the number of characters in scheme
      pos += joined.length() - stripped.length();
      // keep the scheme + authority part unchanged, normalize the path
      return joined.substr(0, pos) + remove_dot_segments(joined.substr(pos));
    }
  }
}

}  // namespace utils
}  // namespace storage
}  // namespace mysqlshdk
