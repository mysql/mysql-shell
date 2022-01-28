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

#ifndef MODULES_UTIL_DUMP_DUMP_MANIFEST_OPTIONS_H_
#define MODULES_UTIL_DUMP_DUMP_MANIFEST_OPTIONS_H_

#include <memory>
#include <optional>
#include <string>

#include "mysqlshdk/include/scripting/types_cpp.h"
#include "mysqlshdk/libs/oci/oci_bucket_options.h"

namespace mysqlsh {
namespace dump {

class Dump_manifest_options : public mysqlshdk::oci::Oci_bucket_options {
 public:
  Dump_manifest_options() = default;

  Dump_manifest_options(const Dump_manifest_options &) = default;
  Dump_manifest_options(Dump_manifest_options &&) = default;

  Dump_manifest_options &operator=(const Dump_manifest_options &) = default;
  Dump_manifest_options &operator=(Dump_manifest_options &&) = default;

  ~Dump_manifest_options() override = default;

  static constexpr const char *par_manifest_option() {
    return "ociParManifest";
  }

  static constexpr const char *par_expire_time_option() {
    return "ociParExpireTime";
  }

  static const shcore::Option_pack_def<Dump_manifest_options> &options();

  bool par_manifest() const { return m_par_manifest.value_or(false); }

 private:
  friend class Dump_manifest_write_config;

  void on_unpacked_options() const override;

  std::shared_ptr<mysqlshdk::storage::backend::object_storage::Config>
  create_config() const override;

  void set_par_manifest(bool enabled);

  std::optional<bool> m_par_manifest;
  std::string m_par_expire_time;
};

}  // namespace dump
}  // namespace mysqlsh

#endif  // MODULES_UTIL_DUMP_DUMP_MANIFEST_OPTIONS_H_
