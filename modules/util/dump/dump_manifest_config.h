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

#ifndef MODULES_UTIL_DUMP_DUMP_MANIFEST_CONFIG_H_
#define MODULES_UTIL_DUMP_DUMP_MANIFEST_CONFIG_H_

#include <memory>
#include <string>

#include "mysqlshdk/libs/oci/oci_bucket_config.h"
#include "mysqlshdk/libs/oci/oci_par.h"
#include "mysqlshdk/libs/storage/config.h"

#include "modules/util/dump/dump_manifest_options.h"

namespace mysqlsh {
namespace dump {

class Dump_manifest_write_config : public mysqlshdk::oci::Oci_bucket_config {
 public:
  Dump_manifest_write_config() = delete;

  explicit Dump_manifest_write_config(const Dump_manifest_options &options);

  Dump_manifest_write_config(const Dump_manifest_write_config &) = delete;
  Dump_manifest_write_config(Dump_manifest_write_config &&) = default;

  Dump_manifest_write_config &operator=(const Dump_manifest_write_config &) =
      delete;
  Dump_manifest_write_config &operator=(Dump_manifest_write_config &&) =
      default;

  ~Dump_manifest_write_config() override = default;

  const std::string &par_expire_time() const { return m_par_expire_time; }

 private:
  std::string describe_self() const override;

  std::unique_ptr<mysqlshdk::storage::IFile> file(
      const std::string &path) const override;

  std::unique_ptr<mysqlshdk::storage::IDirectory> directory(
      const std::string &path) const override;

  std::string m_par_expire_time;
};

using Dump_manifest_write_config_ptr =
    std::shared_ptr<const Dump_manifest_write_config>;

class Dump_manifest_read_config
    : public mysqlshdk::oci::PAR_config<mysqlshdk::oci::PAR_type::MANIFEST> {
 public:
  using PAR_config::PAR_config;

 private:
  std::unique_ptr<mysqlshdk::storage::IFile> file(
      const std::string &path) const override;

  std::unique_ptr<mysqlshdk::storage::IDirectory> directory(
      const std::string &path) const override;
};

using Dump_manifest_read_config_ptr =
    std::shared_ptr<const Dump_manifest_read_config>;

}  // namespace dump
}  // namespace mysqlsh

#endif  // MODULES_UTIL_DUMP_DUMP_MANIFEST_CONFIG_H_
