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

#include "modules/util/dump/dump_manifest_options.h"

#include <memory>
#include <stdexcept>

#include "mysqlshdk/include/scripting/type_info/custom.h"
#include "mysqlshdk/include/scripting/type_info/generic.h"

#include "modules/util/dump/dump_manifest_config.h"

namespace mysqlsh {
namespace dump {

const shcore::Option_pack_def<Dump_manifest_options>
    &Dump_manifest_options::options() {
  static const auto opts =
      shcore::Option_pack_def<Dump_manifest_options>()
          .optional(bucket_name_option(), &Dump_manifest_options::m_bucket_name)
          .optional(namespace_option(), &Dump_manifest_options::m_namespace)
          .optional(config_file_option(), &Dump_manifest_options::m_config_file)
          .optional(profile_option(), &Dump_manifest_options::m_config_profile)
          .optional(par_manifest_option(),
                    &Dump_manifest_options::set_par_manifest)
          .optional(par_expire_time_option(),
                    &Dump_manifest_options::m_par_expire_time)
          .on_done(&Dump_manifest_options::on_unpacked_options);

  return opts;
}

void Dump_manifest_options::on_unpacked_options() const {
  Oci_bucket_options::on_unpacked_options();

  if (m_bucket_name.empty() && m_par_manifest) {
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
  m_par_manifest = enabled;
}

}  // namespace dump
}  // namespace mysqlsh
