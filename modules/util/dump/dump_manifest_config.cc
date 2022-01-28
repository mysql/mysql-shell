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

#include "modules/util/dump/dump_manifest_config.h"

#include <chrono>
#include <stdexcept>

#include "mysqlshdk/libs/utils/utils_time.h"

#include "modules/util/dump/dump_manifest.h"

namespace mysqlsh {
namespace dump {

Dump_manifest_write_config::Dump_manifest_write_config(
    const Dump_manifest_options &options)
    : Oci_bucket_config(options), m_par_expire_time(options.m_par_expire_time) {
  if (m_par_expire_time.empty()) {
    // If not specified sets the expiration time to NOW + 1 Week
    m_par_expire_time = shcore::future_time_rfc3339(std::chrono::hours(24 * 7));
  }
}

std::string Dump_manifest_write_config::describe_self() const {
  return Oci_bucket_config::describe_self() + " (writing manifest)";
}

std::unique_ptr<mysqlshdk::storage::IFile> Dump_manifest_write_config::file(
    const std::string &) const {
  throw std::logic_error("Dump_manifest_write_config::file() - not supported");
}

std::unique_ptr<mysqlshdk::storage::IDirectory>
Dump_manifest_write_config::directory(const std::string &path) const {
  return std::make_unique<Dump_manifest_writer>(
      shared_ptr<Dump_manifest_write_config>(), path);
}

std::unique_ptr<mysqlshdk::storage::IFile> Dump_manifest_read_config::file(
    const std::string &) const {
  throw std::logic_error("Dump_manifest_read_config::file() - not supported");
}

std::unique_ptr<mysqlshdk::storage::IDirectory>
Dump_manifest_read_config::directory(const std::string &path) const {
  validate_url(path);

  return std::make_unique<Dump_manifest_reader>(
      shared_ptr<Dump_manifest_read_config>());
}

}  // namespace dump
}  // namespace mysqlsh
