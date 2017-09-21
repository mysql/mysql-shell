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

#include <unistd.h>
#include <pwd.h>
#include <cstdlib>
#include <string>

namespace shcore {
namespace path {

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

}  // namespace path
}  // namespace shcore
