/*
 * Copyright (c) 2022, Oracle and/or its affiliates.
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

#include "mysqlshdk/libs/storage/config.h"

#include <stdexcept>

#include "mysqlshdk/libs/storage/idirectory.h"
#include "mysqlshdk/libs/storage/ifile.h"

namespace mysqlshdk {
namespace storage {

std::unique_ptr<IFile> Config::make_file(const std::string &path) const {
  assert(valid());
  return file(path);
}

std::unique_ptr<IDirectory> Config::make_directory(
    const std::string &path) const {
  assert(valid());
  return directory(path);
}

std::string Config::description() const {
  assert(valid());
  return describe_self();
}

std::string Config::describe(const std::string &url) const {
  auto desc = description();
  auto desc_url = describe_url(url);

  if (!desc_url.empty()) {
    desc += ", " + desc_url;
  }

  return desc;
}

}  // namespace storage
}  // namespace mysqlshdk
