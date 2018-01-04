/*
 * Copyright (c) 2017, 2018, Oracle and/or its affiliates. All rights reserved.
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

#include "mysqlshdk/libs/utils/utils_path.h"

#include <algorithm>
#include <string>

#include "mysqlshdk/libs/utils/utils_string.h"

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

std::string search_stdpath(const std::string &name) {
  char *path = std::getenv("PATH");
  if (!path) {
    return "";
  }
  return search_path_list(name, path);
}


std::string search_path_list(const std::string &name,
                             const std::string &pathlist,
                             const char separator) {
  std::string fname(name);
#ifdef _WIN32
  if (!str_endswith(name, ".exe") && !str_endswith(name, ".com")) {
    fname.append(".exe");
  }
#endif

  std::string sep;
  if (separator != 0) {
    sep.push_back(separator);
  } else {
    sep.push_back(pathlist_separator);
  }
  for (const std::string &dir : str_split(pathlist, sep)) {
    std::string tmp = join_path(dir, fname);
    if (exists(tmp))
      return tmp;
  }
  return "";
}
}  // namespace path
}  // namespace shcore
