/*
 * Copyright (c) 2020, 2022, Oracle and/or its affiliates.
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

#include "mysqlshdk/libs/storage/backend/directory.h"

#include <filesystem>
#include <utility>

#include "mysqlshdk/libs/storage/utils.h"
#include "mysqlshdk/libs/utils/utils_file.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_path.h"
#include "mysqlshdk/libs/utils/utils_string.h"

namespace mysqlshdk {
namespace storage {
namespace backend {

Directory::Directory(const std::string &dir) {
  const auto expanded =
      shcore::path::expand_user(utils::strip_scheme(dir, "file"));
  m_path = shcore::get_absolute_path(expanded);
}

bool Directory::exists() const { return shcore::is_folder(full_path().real()); }

void Directory::create() {
  shcore::create_directory(full_path().real(), false, 0750);
}

Masked_string Directory::full_path() const { return m_path; }

std::unordered_set<IDirectory::File_info> Directory::list_files(
    bool /*hidden_files*/) const {
  return filter_files("");
}

std::unordered_set<IDirectory::File_info> Directory::filter_files(
    const std::string &pattern) const {
  const auto path =
#ifdef _WIN32
      shcore::utf8_to_wide
#endif  // _WIN32
      (full_path().real());
  std::unordered_set<IDirectory::File_info> files;
  std::error_code ec;

  for (auto &entry : std::filesystem::directory_iterator(path, ec)) {
    if (entry.is_regular_file()
#if (__GNUC__ < 8 || (__GNUC__ == 8 && __GNUC_MINOR__ < 4)) && \
    !defined(__clang__)
        // older versions of GCC do not handle filesystems which do not support
        // dirent.d_type (OS sets this field to DT_UNKNOWN in such case) and are
        // unable to detect the entry type; newer versions fall back to stat(),
        // we do the same here
        || (std::filesystem::file_type::regular == entry.status().type())
#endif
    ) {
      auto name =
#ifdef _WIN32
          shcore::wide_to_utf8
#endif  // _WIN32
          (entry.path().filename().native());

      if (pattern.empty() || shcore::match_glob(pattern, name)) {
        files.emplace(std::move(name), [entry = std::move(entry)]() {
          return entry.file_size();
        });
      }
    }
  }

  if (ec) {
    throw std::runtime_error(full_path().masked() + ": " + ec.message());
  }

  return files;
}

std::string Directory::join_path(const std::string &a,
                                 const std::string &b) const {
  return shcore::path::join_path(a, b);
}

}  // namespace backend
}  // namespace storage
}  // namespace mysqlshdk
