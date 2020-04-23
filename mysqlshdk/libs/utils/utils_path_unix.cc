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

#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_path.h"
#include "mysqlshdk/libs/utils/utils_string.h"

#include <errno.h>
#include <limits.h>
#include <pwd.h>
#include <unistd.h>
#include <cstdlib>
#include <string>

namespace shcore {
namespace path {

const char *k_valid_path_separators = "/";

/*
 * Join two or more pathname components, inserting '/' as needed.
 * If any component is an absolute path, all previous path components
 * will be discarded.  An empty last part will result in a path that
 * ends with a separator.
 * @param components vector of strings with the paths to join
 *
 * @return the concatenation of all paths with exactly one directory separator
 *         following each non-empty part except the last, meaning that the
 *         result will only end in a separator if the last part is empty.
 */
std::string SHCORE_PUBLIC
join_path(const std::vector<std::string> &components) {
  std::string path, s;
  if (!components.empty())
    path = components.at(0);
  else
    return "";
  for (size_t i = 1; i < components.size(); ++i) {
    s = components.at(i);
    if (!s.empty() && s.front() == '/')
      // absolute path, so discard any previous results
      path = s;
    else if (path.empty() || path.back() == '/')
      // not an absolute path, and previous results are either empty or already
      // have a path separator
      path += s;
    else
      // not an absolute path but previous results don't yet have path separator
      // so add one
      path += "/" + s;
  }
  return path;
}

/*
 * Split a pathname into a std::pair (drive, tail) where drive is a drive
 * specification or the empty string. Useful on DOS/Windows/NT. On Unix,
 * the drive is always empty.
 * On all systems, drive + tail are the same as the original path argument.
 * @param string with the path name to be split
 *
 * @return a pair (drive, tail) where the drive is either a drive specification
 *         or empty string. On Unix systems drive will always be empty string.
 *
 */
std::pair<std::string, std::string> SHCORE_PUBLIC
splitdrive(const std::string &path) {
  return std::make_pair("", path);
}

std::string SHCORE_PUBLIC home() {
  const char *env_home = getenv("HOME");
  if (env_home != nullptr) {
    return std::string{env_home};
  }

  auto uid = getuid();
  auto pwd = getpwuid(uid);
  if (pwd != nullptr) {
    return std::string{pwd->pw_dir};
  }

  return std::string{};
}

std::string SHCORE_PUBLIC home(const std::string &username) {
  auto pwd = getpwnam(username.c_str());
  if (pwd != nullptr) {
    return std::string{pwd->pw_dir};
  }

  return std::string{};
}

std::string SHCORE_PUBLIC expand_user(const std::string &path) {
  const char sep[] = "/";
  return detail::expand_user(path, sep);
}

std::tuple<std::string, std::string> SHCORE_PUBLIC
split_extension(const std::string &path) {
  const char sep[] = "/";
  return detail::split_extension(path, sep);
}

// TODO(.) should use normpath() to match windows functionality
std::string SHCORE_PUBLIC normalize(const std::string &path) {
  if (path.size() == 0) {
    return ".";
  }

  const std::string sep = "/";
  auto chunks = str_split(path, sep);
  std::vector<std::string> p;

  std::string norm;

  // Prepend initial slashes
  if (str_beginswith(path, "/")) {
    norm = "/";
  }

  if (str_beginswith(path, "//") && !str_beginswith(path, "///")) {
    norm = "//";
  }

  for (decltype(chunks)::size_type i = 0; i < chunks.size(); i++) {
    // skip "/" and "."
    if (chunks[i] == "" || chunks[i] == ".") {
      continue;
    }

    if (chunks[i] != ".." || (norm.empty() && p.empty()) ||
        (!p.empty() && p.back() == "..")) {
      p.push_back(chunks[i]);
    } else if (!p.empty()) {
      p.pop_back();
    }
  }

  norm += join_path(p);

  if (norm.empty()) {
    norm = ".";
  }

  return norm;
}

std::string SHCORE_PUBLIC dirname(const std::string &path) {
  size_t xx = detail::span_dirname(path);
  if (xx == std::string::npos) return ".";
  return path.substr(0, xx);
}

std::string SHCORE_PUBLIC basename(const std::string &path) {
  size_t end = path.find_last_not_of(k_valid_path_separators);
  if (end == std::string::npos)  // only separators
    return path.substr(0, 1);
  end++;  // go to after the last char
  size_t p = detail::span_dirname(path);
  if (p == std::string::npos || p == path.size() || p == 0 || p == end)
    return path.substr(0, end);
  size_t pp = path.find_first_not_of(k_valid_path_separators, p);
  if (pp != std::string::npos) p = pp;
  return path.substr(p, end - p);
}

bool exists(const std::string &path) { return access(path.c_str(), F_OK) == 0; }

std::string SHCORE_PUBLIC tmpdir() {
  const char *dir = nullptr;

  if (!(dir = getenv("TMPDIR")) && !(dir = P_tmpdir)) {
    dir = "/tmp";
  }

  return dir;
}

bool SHCORE_PUBLIC is_absolute(const std::string &path) {
  return path.length() > 0 && is_path_separator(expand_user(path)[0]);
}

std::string SHCORE_PUBLIC getcwd() {
  char path[PATH_MAX] = {0};
  if (nullptr == ::getcwd(path, PATH_MAX)) {
    throw std::runtime_error("Failed to get current working directory: " +
                             errno_to_string(errno));
  }
  return path;
}

std::string SHCORE_PUBLIC get_canonical_path(const std::string &path) {
  auto *res = realpath(path.c_str(), nullptr);
  if (res == nullptr) {
    // handle error
    throw std::runtime_error(shcore::str_format(
        "Error attempting to get the canonical path for '%s': %s (%d)",
        path.c_str(), strerror(errno), errno));
  }

  std::unique_ptr<char, decltype(&free)> deleter{res, free};
  return std::string(res);
}

}  // namespace path
}  // namespace shcore
