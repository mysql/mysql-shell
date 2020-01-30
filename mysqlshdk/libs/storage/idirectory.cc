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

#include "mysqlshdk/libs/storage/idirectory.h"

#include <stdexcept>

#include "mysqlshdk/libs/oci/oci_options.h"
#include "mysqlshdk/libs/storage/backend/directory.h"
#include "mysqlshdk/libs/storage/backend/oci_object_storage.h"
#include "mysqlshdk/libs/storage/utils.h"

namespace mysqlshdk {
namespace storage {

std::unique_ptr<IFile> IDirectory::file(const std::string &name) const {
  return make_file(join_path(full_path(), name));
}

std::unique_ptr<IDirectory> make_directory(
    const std::string &path,
    const std::unordered_map<std::string, std::string> &options) {
  if (!options.empty()) {
    mysqlshdk::oci::Oci_options oci_options;
    std::string os_path;
    bool is_oci = mysqlshdk::oci::parse_oci_options(
        mysqlshdk::oci::Oci_uri_type::DIRECTORY, path, options, &oci_options,
        &os_path);

    if (is_oci) {
      return make_directory(os_path, oci_options);
    }
  }

  const auto scheme = utils::get_scheme(path);
  if (scheme.empty() || utils::scheme_matches(scheme, "file")) {
    return std::make_unique<backend::Directory>(path);
  } else if (utils::scheme_matches(scheme, "oci+os")) {
    throw std::invalid_argument("The osBucketName option is missing.");
  }

  throw std::invalid_argument("Directory handling for " + scheme +
                              " protocol is not supported.");
}

std::unique_ptr<IDirectory> make_directory(
    const std::string &path, const mysqlshdk::oci::Oci_options &options) {
  if (options) {
    return std::make_unique<backend::oci::Directory>(options, path);
  } else {
    return make_directory(path);
  }
}

}  // namespace storage
}  // namespace mysqlshdk
