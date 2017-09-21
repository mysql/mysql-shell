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
#include <cstdlib>
#include <string>

#include <ShlObj.h>

namespace shcore {
namespace path {
namespace detail {
class Known_folder_path {
 public:
  Known_folder_path() = default;
  Known_folder_path(const Known_folder_path &other) = delete;
  Known_folder_path(Known_folder_path &&other) = delete;
  Known_folder_path &operator=(const Known_folder_path &other) = delete;
  Known_folder_path &operator=(Known_folder_path &&other) = delete;
  ~Known_folder_path() { CoTaskMemFree(path_); }

  std::string operator()(const KNOWNFOLDERID folder_id) {
    HRESULT hr = SHGetKnownFolderPath(folder_id, 0, nullptr, &path_);
    if (SUCCEEDED(hr)) {
      std::mbstate_t s = std::mbstate_t();
      const wchar_t *p = path_;
      size_t len = std::wcsrtombs(nullptr, &p, 0, &s);
      if (len != static_cast<std::size_t>(-1)) {
        std::string path(len, '\0');
        std::wcsrtombs(&path[0], &p, path.size(), &s);
        return path;
      }
    }

    return std::string{};
  }

 private:
  PWSTR path_ = nullptr;
};
}  // namespace detail

std::string SHCORE_PUBLIC home() {
  const char *env_home = getenv("HOME");
  if (env_home != nullptr) {
    return std::string{env_home};
  }

  const char *env_userprofile = getenv("USERPROFILE");
  if (env_userprofile != nullptr) {
    return std::string{env_userprofile};
  }

  const char *env_homedrive = getenv("HOMEDRIVE");
  const char *env_homepath = getenv("HOMEPATH");
  if (env_homedrive != nullptr && env_homepath != nullptr) {
    return std::string{env_homedrive} + std::string{env_homepath};
  }

  auto profile_path = detail::Known_folder_path()(FOLDERID_Profile);
  return profile_path;
}

std::string SHCORE_PUBLIC home(const std::string & /* username */) {
  // retrieving home directory of another user on Windows is NOT SUPPORTED.
  return std::string{};
}

std::string SHCORE_PUBLIC expand_user(const std::string &path) {
  const char sep[] = "\\/";
  return detail::expand_user(path, sep);
}

}  // namespace path
}  // namespace shcore
