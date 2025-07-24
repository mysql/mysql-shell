/*
 * Copyright (c) 2017, 2025, Oracle and/or its affiliates.
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

#include "mysqlshdk/libs/utils/utils_path.h"

#include <algorithm>
#include <stdexcept>
#include <string>
#include <tuple>

#include "mysqlshdk/libs/utils/utils_string.h"

namespace shcore {
namespace path {
namespace detail {

std::string expand_user(std::string_view path, std::string_view sep) {
  if (!path.empty() && '~' == path[0]) {
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
      user_home.append(p, end(path));
      return user_home;
    }
  }

  return std::string(path);
}

std::tuple<std::string, std::string> split_extension(const std::string &path,
                                                     const std::string &sep) {
  if (path.empty()) {
    return std::make_tuple("", "");
  }

  const auto basename =
      std::find_first_of(path.crbegin(), path.crend(), begin(sep), end(sep))
          .base();
  auto not_dot = [](const char c) -> bool { return c != '.'; };
  const auto leading_dots = std::find_if(basename, path.end(), not_dot);
  const auto last_dot = path.rfind('.');
  const auto barrier =
      static_cast<size_t>(std::distance(path.begin(), leading_dots));

  if (last_dot != std::string::npos && barrier <= last_dot) {
    return std::make_tuple(path.substr(0, last_dot), path.substr(last_dot));
  }

  return std::make_tuple(path, "");
}

size_t span_dirname(const std::string &path) {
  if (path.empty()) return std::string::npos;

  // trailing / is ignored
  size_t end = path.size();
  end = path.find_last_not_of(k_valid_path_separators, end);
  if (end == 0 || end == std::string::npos)
    return 1;  // path has no text other than separator

  size_t start = std::distance(
      path.begin(),
      std::find_first_of(
          path.begin(), path.end() - end, k_valid_path_separators,
          k_valid_path_separators + strlen(k_valid_path_separators)));
  if (start == std::string::npos) return std::string::npos;

  // absolute path
  if (start == 0) {
    // ignore multiple leading /
    start = path.find_first_not_of(k_valid_path_separators, start) - 1;

    size_t p = path.find_last_of(k_valid_path_separators, end);
    if (p == start) return 1;
    p = path.find_last_not_of(k_valid_path_separators, p);  // merge repeated /
    if (p == start) return 1;

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
    if (exists(tmp)) return tmp;
  }
  return "";
}

bool SHCORE_PUBLIC is_path_separator(char c) {
  return strchr(k_valid_path_separators, c) != nullptr;
}

std::string SHCORE_PUBLIC get_relative_path(const std::string &from,
                                            const std::string &to) {
  if (is_absolute(from) != is_absolute(to)) {
    throw std::runtime_error{
        "Both paths have to be either absolute or relative: '" + from +
        "' and '" + to + "'"};
  }

#ifdef _WIN32
  // split the drive
  auto from_d = splitdrive(from);
  auto to_d = splitdrive(to);

  if (from_d.first != to_d.first) {
    throw std::runtime_error{
        "Cannot compute relative path between different drive letters: '" +
        from + "' and '" + to + "'"};
  }

  // normalize path
  from_d.second = shcore::str_replace(from_d.second, pathlist_separator_s, "/");
  to_d.second = shcore::str_replace(to_d.second, pathlist_separator_s, "/");

  // get the pointers
  auto f = from_d.second.c_str();
  auto t = to_d.second.c_str();
#else
  auto f = from.c_str();
  auto t = to.c_str();
#endif  // _WIN32

  const auto is_separator = [](const char *p) {
    return '\0' == *p || '/' == *p;
  };

  const auto skip_separator = [](const char **p) {
    while ('/' == **p) ++(*p);
  };

  const char *begin;
  bool prepend_dots = true;

  do {
    // skip the separators in both paths
    skip_separator(&f);
    skip_separator(&t);

    // we're at the beginning of next path segment
    begin = t;

    if (!*f) {
      // this is the end of the `from` path, relative path is what we have in
      // `begin`
      prepend_dots = false;
      break;
    }

    // check if both path match
    while (!is_separator(f) && *f == *t) {
      ++f;
      ++t;
    }
    // continue as long as segments in both paths match
  } while (is_separator(f) && is_separator(t));

  std::string result;

  if (prepend_dots) {
    // for each path segment in `from` prepend '..', note that `f` might have
    // ended while we were comparing segments, hence we always prepend at least
    // once
    do {
      result += "../";

      while (!is_separator(f)) {
        ++f;
      }

      skip_separator(&f);
    } while (*f);
  }

  // append the rest of the `to` path
  if (!*begin) {
    // nothing to append, remove last '/', if it's there
    if (!result.empty()) {
      result.pop_back();
    }
  } else {
    result += begin;
  }

  return result;
}

}  // namespace path
}  // namespace shcore
