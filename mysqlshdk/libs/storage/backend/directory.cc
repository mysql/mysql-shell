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

#include "mysqlshdk/libs/storage/backend/directory.h"

#include <utility>

#include "mysqlshdk/libs/storage/utils.h"
#include "mysqlshdk/libs/utils/utils_file.h"
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

bool Directory::exists() const { return shcore::is_folder(full_path()); }

void Directory::create() { shcore::create_directory(full_path(), false, 0750); }

std::string Directory::full_path() const { return m_path; }

std::vector<IDirectory::File_info> Directory::list_files(
    bool /*hidden_files*/) const {
  const auto path = full_path();
  std::vector<IDirectory::File_info> files;

  shcore::iterdir(path, [this, &path, &files](const std::string &name) {
    if (shcore::is_file(join_path(path, name))) {
      IDirectory::File_info fi = {name, 0};
      files.emplace_back(std::move(fi));
    }

    return true;
  });

  for (auto &fi : files) {
    fi.size = file(fi.name)->file_size();
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
