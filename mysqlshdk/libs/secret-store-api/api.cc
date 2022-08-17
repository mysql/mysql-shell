/*
 * Copyright (c) 2018, 2022, Oracle and/or its affiliates.
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

#include "mysql-secret-store/include/mysql-secret-store/api.h"

#include <string.h>

#include "mysqlshdk/libs/utils/utils_file.h"
#include "mysqlshdk/libs/utils/utils_path.h"

#include "mysqlshdk/libs/secret-store-api/helper_invoker.h"
#include "mysqlshdk/libs/secret-store-api/logger.h"

namespace mysql {
namespace secret_store {
namespace api {

namespace {

constexpr auto k_exe_prefix = "mysql-secret-store-";

std::string get_helper_folder() {
  static std::string folder = shcore::get_binary_folder();
  return folder;
}

std::string get_helper_name(const std::string &filename) {
  std::string name = filename.substr(strlen(k_exe_prefix));
  name = name.substr(0, name.find_last_of("."));

  return name;
}

}  // namespace

std::vector<Helper_name> get_available_helpers(
    const std::string &dir) noexcept {
  std::vector<Helper_name> helpers;
  const auto folder = dir.empty() ? get_helper_folder() : dir;

  if (!shcore::is_folder(folder)) {
    return helpers;
  }

  for (const auto &entry : shcore::listdir(folder)) {
    const std::string path = shcore::path::join_path(folder, entry);

    if (entry.compare(0, strlen(k_exe_prefix), k_exe_prefix) == 0 &&
        !shcore::is_folder(path)) {
      Helper_name name{get_helper_name(entry), path};

      // check if helper is valid by running test command
      if (Helper_invoker{name}.version()) {
        helpers.push_back(name);
      }
    }
  }

  return helpers;
}

std::unique_ptr<Helper_interface> get_helper(const Helper_name &name) noexcept {
  return std::unique_ptr<Helper_interface>{new Helper_interface{name}};
}

void set_logger(std::function<void(std::string_view)> logger) noexcept {
  logger::set_logger(std::move(logger));
}

}  // namespace api
}  // namespace secret_store
}  // namespace mysql
