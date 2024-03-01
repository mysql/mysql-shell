/*
 * Copyright (c) 2020, 2024, Oracle and/or its affiliates.
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

#include "modules/util/dump/dump_instance_options.h"

#include <utility>
#include <vector>

#include "modules/util/common/dump/constants.h"
#include "modules/util/common/dump/utils.h"

#include "mysqlshdk/include/scripting/type_info/custom.h"
#include "mysqlshdk/include/scripting/type_info/generic.h"
#include "mysqlshdk/libs/utils/utils_general.h"

namespace mysqlsh {
namespace dump {

Dump_instance_options::Dump_instance_options() {
  // some users are always excluded
  filters().users().exclude(common::k_excluded_users);

  // some schemas are always excluded
  filters().schemas().exclude(common::k_excluded_schemas);
}

const shcore::Option_pack_def<Dump_instance_options>
    &Dump_instance_options::options() {
  static const auto opts =
      shcore::Option_pack_def<Dump_instance_options>()
          .include<Dump_schemas_options>()
          .include(&Dump_instance_options::m_filtering_options,
                   &mysqlshdk::db::Filtering_options::schemas)
          .optional("users", &Dump_instance_options::m_dump_users)
          .include(&Dump_instance_options::m_filtering_options,
                   &mysqlshdk::db::Filtering_options::users)
          .on_done(&Dump_instance_options::on_unpacked_options)
          .on_log(&Dump_instance_options::on_log_options);

  return opts;
}

void Dump_instance_options::on_unpacked_options() {
  if (!m_dump_users) {
    if (filters().users().excluded().size() > common::k_excluded_users.size()) {
      throw std::invalid_argument(
          "The 'excludeUsers' option cannot be used if the 'users' option is "
          "set to false.");
    }

    if (!filters().users().included().empty()) {
      throw std::invalid_argument(
          "The 'includeUsers' option cannot be used if the 'users' option is "
          "set to false.");
    }
  }

  if (mds_compatibility()) {
    // if MHS compatibility option is set, some users and schemas should be
    // excluded automatically
    filters().users().exclude(common::k_mhs_excluded_users);
    filters().schemas().exclude(common::k_mhs_excluded_schemas);
  }

  m_filter_conflicts |= filters().schemas().error_on_conflicts();
  m_filter_conflicts |= filters().users().error_on_conflicts();
}

void Dump_instance_options::validate_options() const {
  // call method up the chain, Dump_schemas_options has an empty list of schemas
  // and would throw
  Ddl_dumper_options::validate_options();
}

}  // namespace dump
}  // namespace mysqlsh
