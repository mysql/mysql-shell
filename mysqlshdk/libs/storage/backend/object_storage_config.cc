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

#include "mysqlshdk/libs/storage/backend/object_storage_config.h"

#include <stdexcept>

#include "mysqlshdk/libs/storage/utils.h"

#include "mysqlshdk/libs/storage/backend/object_storage.h"
#include "mysqlshdk/libs/storage/backend/object_storage_options.h"

namespace mysqlshdk {
namespace storage {
namespace backend {
namespace object_storage {

Config::Config(const Object_storage_options &options, std::size_t part_size)
    : m_container_name(options.m_container_name),
      m_config_file(options.m_config_file),
      m_part_size(part_size),
      m_container_name_option(options.get_main_option()) {
  assert(!m_container_name.empty());
}

std::string Config::describe_url(const std::string &url) const {
  return "prefix='" + url + "'";
}

std::unique_ptr<storage::IFile> Config::file(const std::string &path) const {
  fail_if_uri(path);

  return std::make_unique<Object>(shared_ptr<Config>(), path);
}

std::unique_ptr<storage::IDirectory> Config::directory(
    const std::string &path) const {
  fail_if_uri(path);

  return std::make_unique<Directory>(shared_ptr<Config>(), path);
}

void Config::fail_if_uri(const std::string &path) const {
  if (!mysqlshdk::storage::utils::get_scheme(path).empty()) {
    throw std::runtime_error(shcore::str_format(
        "The option '%s' can not be used when the path contains a scheme.",
        m_container_name_option.c_str()));
  }
}

Bucket_config::Bucket_config(const Bucket_options &options)
    : Config(options, DEFAULT_MULTIPART_PART_SIZE),
      m_config_profile(options.m_config_profile) {}

}  // namespace object_storage
}  // namespace backend
}  // namespace storage
}  // namespace mysqlshdk
