/*
 * Copyright (c) 2025, Oracle and/or its affiliates.
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

#include "modules/util/load/lakehouse_source_option.h"

#include <stdexcept>

#include "mysqlshdk/libs/oci/oci_par.h"
#include "mysqlshdk/libs/utils/utils_string.h"

namespace mysqlsh {
namespace load {

const shcore::Option_pack_def<Lakehouse_source_option>
    &Lakehouse_source_option::options() {
  static const auto opts =
      shcore::Option_pack_def<Lakehouse_source_option>()
          .optional(par_option(), &Lakehouse_source_option::m_par)
          .optional(bucket_option(),
                    &Lakehouse_source_option::m_resource_principals,
                    &Resource_principals::bucket)
          .optional(namespace_option(),
                    &Lakehouse_source_option::m_resource_principals,
                    &Resource_principals::namespace_)
          .optional(region_option(),
                    &Lakehouse_source_option::m_resource_principals,
                    &Resource_principals::region)
          .optional(prefix_option(),
                    &Lakehouse_source_option::m_resource_principals,
                    &Resource_principals::set_prefix)
          .on_done(&Lakehouse_source_option::on_unpacked_options);

  return opts;
}

void Lakehouse_source_option::on_unpacked_options() {
  // WL16802-FR2.2.1.2: either of these have to be set
  if (m_par.empty() && m_resource_principals.empty()) {
    throw std::invalid_argument(
        shcore::str_format("Invalid value of %s: either '%s' sub-option or "
                           "'%s', '%s', '%s' sub-options have to be set",
                           option_name(), par_option(), bucket_option(),
                           namespace_option(), region_option()));
  }

  // WL16802-FR2.2.1.2: both of these cannot be set
  if (!m_par.empty() && !m_resource_principals.empty()) {
    throw std::invalid_argument(shcore::str_format(
        "Invalid value of %s: both '%s' sub-option and '%s', '%s', '%s' "
        "sub-options cannot be set at the same time",
        option_name(), par_option(), bucket_option(), namespace_option(),
        region_option()));
  }

  // WL16802-FR2.2.1.2: bucket, namespace and region have all be set
  if (m_par.empty() && !m_resource_principals.valid()) {
    throw std::invalid_argument(shcore::str_format(
        "Invalid value of %s: all of '%s', '%s', '%s' sub-options have to be "
        "set",
        option_name(), bucket_option(), namespace_option(), region_option()));
  }

  if (!m_par.empty()) {
    // WL16802-FR2.2.1.3: prefix cannot be used with par
    if (!m_resource_principals.prefix().empty()) {
      throw std::invalid_argument(
          shcore::str_format("Invalid value of %s: both '%s' and '%s "
                             "sub-options cannot be set at the same time",
                             option_name(), par_option(), prefix_option()));
    }

    if (mysqlshdk::oci::PAR_type::NONE == mysqlshdk::oci::parse_par(m_par)) {
      throw std::invalid_argument(shcore::str_format(
          "Invalid value of %s: the '%s' sub-option should be set to a PAR URL",
          option_name(), par_option()));
    }

    if ('/' != m_par.back()) {
      // make sure this is a prefix PAR
      m_par.append(1, '/');
    }
  }
}

}  // namespace load
}  // namespace mysqlsh
