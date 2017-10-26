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
#include "mysqlshdk/libs/utils/utils_string.h"

#include <pwd.h>
#include <unistd.h>
#include <cstdlib>
#include <string>

namespace shcore {
namespace path {

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
    if (s.front() == '/')
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

}  // namespace path
}  // namespace shcore
