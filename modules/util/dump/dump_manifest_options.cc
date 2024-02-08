/*
 * Copyright (c) 2022, 2024, Oracle and/or its affiliates.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.0,
 * as published by the Free Software Foundation.
 *
 * This program is designed to work with certain software (including
 * but not limited to OpenSSL) that is licensed under separate terms,
 * as designated in a particular file or component or in included license
 * documentation.  The authors of MySQL hereby grant you an additional
 * permission to link the program and your derivative works with the
 * separately licensed software that they have either included with
 * the program or referenced in the documentation.
 *
 * This program is distributed in the hope that it will be useful,  but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 * the GNU General Public License, version 2.0, for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "modules/util/dump/dump_manifest_options.h"

#include <memory>
#include <stdexcept>

#include "modules/util/dump/dump_manifest_config.h"
#include "mysqlshdk/include/scripting/type_info/custom.h"
#include "mysqlshdk/include/scripting/type_info/generic.h"
#include "mysqlshdk/shellcore/shell_console.h"

namespace mysqlsh {
namespace dump {

const shcore::Option_pack_def<Dump_manifest_options>
    &Dump_manifest_options::options() {
  static const auto opts =
      shcore::Option_pack_def<Dump_manifest_options>()
          .optional(bucket_name_option(),
                    &Dump_manifest_options::m_container_name)
          .optional(namespace_option(), &Dump_manifest_options::m_namespace)
          .optional(config_file_option(), &Dump_manifest_options::m_config_file)
          .optional(profile_option(), &Dump_manifest_options::m_config_profile)
          .optional(par_manifest_option(),
                    &Dump_manifest_options::set_par_manifest, "",
                    shcore::Option_extract_mode::CASE_INSENSITIVE,
                    shcore::Option_scope::DEPRECATED)
          .optional(par_expire_time_option(),
                    &Dump_manifest_options::set_par_expire_time, "",
                    shcore::Option_extract_mode::CASE_INSENSITIVE,
                    shcore::Option_scope::DEPRECATED)
          .on_done(&Dump_manifest_options::on_unpacked_options);

  return opts;
}

void Dump_manifest_options::on_unpacked_options() const {
  Oci_bucket_options::on_unpacked_options();

  if (m_container_name.empty() && m_par_manifest.has_value()) {
    throw std::invalid_argument(shcore::str_format(
        s_option_error, par_manifest_option(), bucket_name_option()));
  }

  if (!par_manifest() && !m_par_expire_time.empty()) {
    throw std::invalid_argument(
        shcore::str_format("The option '%s' cannot be used when the value of "
                           "'%s' option is not True.",
                           par_expire_time_option(), par_manifest_option()));
  }
}

std::shared_ptr<mysqlshdk::storage::backend::object_storage::Config>
Dump_manifest_options::create_config() const {
  if (par_manifest()) {
    return std::make_shared<Dump_manifest_write_config>(*this);
  } else {
    return Oci_bucket_options::create_config();
  }
}

void Dump_manifest_options::set_par_manifest(bool enabled) {
  mysqlsh::current_console()->print_warning(
      "The ociParManifest option is deprecated and will be removed in a future "
      "release. Please use a prefix PAR instead.");

  m_par_manifest = enabled;
}

void Dump_manifest_options::set_par_expire_time(const std::string &value) {
  mysqlsh::current_console()->print_warning(
      "The ociParExpireTime option is deprecated and will be removed in a "
      "future release.");

  m_par_expire_time = value;
}

}  // namespace dump
}  // namespace mysqlsh
