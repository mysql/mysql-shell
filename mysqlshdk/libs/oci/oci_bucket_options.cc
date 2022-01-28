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

#include "mysqlshdk/libs/oci/oci_bucket_options.h"

#include <memory>
#include <stdexcept>

#include "mysqlshdk/include/scripting/type_info/custom.h"
#include "mysqlshdk/include/scripting/type_info/generic.h"

#include "mysqlshdk/libs/oci/oci_bucket_config.h"

namespace mysqlshdk {
namespace oci {

const shcore::Option_pack_def<Oci_bucket_options>
    &Oci_bucket_options::options() {
  static const auto opts =
      shcore::Option_pack_def<Oci_bucket_options>()
          .optional(bucket_name_option(), &Oci_bucket_options::m_bucket_name)
          .optional(namespace_option(), &Oci_bucket_options::m_namespace)
          .optional(config_file_option(), &Oci_bucket_options::m_config_file)
          .optional(profile_option(), &Oci_bucket_options::m_config_profile)
          .on_done(&Oci_bucket_options::on_unpacked_options);

  return opts;
}

void Oci_bucket_options::on_unpacked_options() const {
  if (m_bucket_name.empty() && !m_namespace.empty()) {
    throw std::invalid_argument(shcore::str_format(
        s_option_error, namespace_option(), bucket_name_option()));
  }

  Bucket_options::on_unpacked_options();
}

std::shared_ptr<Oci_bucket_config> Oci_bucket_options::oci_config() const {
  return std::make_shared<Oci_bucket_config>(*this);
}

std::shared_ptr<storage::backend::object_storage::Config>
Oci_bucket_options::create_config() const {
  return oci_config();
}

}  // namespace oci
}  // namespace mysqlshdk
