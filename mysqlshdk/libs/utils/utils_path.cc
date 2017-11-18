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

#include "mysqlshdk/libs/utils/utils_path.h"

#include <algorithm>
#include <string>

namespace shcore {
namespace path {
namespace detail {
std::string expand_user(const std::string &path, const std::string &sep) {
  if (path.size() > 0) {
    if (path[0] == '~') {
      std::string user_home;
      auto p = std::find_first_of(begin(path), end(path), begin(sep), end(sep));
      auto tilde_prefix_length = std::distance(begin(path), p);

      if (tilde_prefix_length == 1) {
        // `~` prefix
        user_home = home();
      } else if (tilde_prefix_length > 1) {
        // `~loginname` prefix
        auto login_name = std::string(begin(path) + 1, p);
        user_home = home(login_name);
      }

      if (!user_home.empty()) {
        return user_home + std::string(p, end(path));
      }
    }
  }
  return path;
}

size_t span_dirname(const std::string &path) {
  if (path.empty())
    return std::string::npos;

  // trailing / is ignored
  size_t end = path.size();
  end = path.find_last_not_of(k_valid_path_separators, end);
  if (end == 0 || end == std::string::npos)
    return 1;  // path has no text other than separator

  size_t start = path.find_first_of(k_valid_path_separators, 0, end);
  if (start == std::string::npos)
    return std::string::npos;

  // absolute path
  if (start == 0) {
    // ignore multiple leading /
    start = path.find_first_not_of(k_valid_path_separators, start) - 1;

    size_t p = path.find_last_of(k_valid_path_separators, end);
    if (p == start)
      return 1;
    p = path.find_last_not_of(k_valid_path_separators, p);  // merge repeated /
    if (p == start)
      return 1;

    // /foo/bar -> /foo or /foo/bar/ -> /foo
    return p + 1;
  } else {
    // relative path
    return path.find_last_of(k_valid_path_separators, end);
  }
}

}  // namespace detail


std::string SHCORE_PUBLIC basename(const std::string &path) {
  size_t end = path.find_last_not_of(k_valid_path_separators);
  if (end == std::string::npos)  // only separators
    return path.substr(0, 1);
  end++;  // go to after the last char
  size_t p = detail::span_dirname(path);
  if (p == std::string::npos || p == path.size() || p == 0 || p == end)
    return path.substr(0, end);
  size_t pp = path.find_first_not_of(k_valid_path_separators, p);
  if (pp != std::string::npos)
    p = pp;
  return path.substr(p, end - p);
}

}  // namespace path
}  // namespace shcore
