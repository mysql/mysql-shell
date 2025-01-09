/*
 * Copyright (c) 2020, 2025, Oracle and/or its affiliates.
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

#include "modules/util/dump/dump_schemas_options.h"

#include <set>
#include <utility>

#include "mysqlshdk/include/scripting/type_info/custom.h"
#include "mysqlshdk/include/scripting/type_info/generic.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_sqlstring.h"
#include "mysqlshdk/libs/utils/utils_string.h"

namespace mysqlsh {
namespace dump {

const shcore::Option_pack_def<Dump_schemas_options>
    &Dump_schemas_options::options() {
  static const auto opts =
      shcore::Option_pack_def<Dump_schemas_options>()
          .include<Ddl_dumper_options>()
          .include(&Dump_schemas_options::m_filtering_options,
                   &Filtering_options::tables)
          .optional("events", &Dump_schemas_options::m_dump_events)
          .include(&Dump_schemas_options::m_filtering_options,
                   &Filtering_options::events)
          .optional("routines", &Dump_schemas_options::m_dump_routines)
          .include(&Dump_schemas_options::m_filtering_options,
                   &Filtering_options::routines)
          .optional("libraries", &Dump_schemas_options::m_dump_libraries)
          .include(&Dump_schemas_options::m_filtering_options,
                   &Filtering_options::libraries)
          .on_done(&Dump_schemas_options::on_unpacked_options);

  return opts;
}

Dump_schemas_options::Dump_schemas_options()
    : Dump_schemas_options("util.dumpSchemas") {}

Dump_schemas_options::Dump_schemas_options(const char *name)
    : Ddl_dumper_options(name) {}

void Dump_schemas_options::on_unpacked_options() {
  if (mds_compatibility()) {
    // if MDS compatibility option is set, mysql schema should not be dumped
    filters().schemas().exclude("mysql");
  }

  m_filter_conflicts |= filters().tables().error_on_conflicts();
  m_filter_conflicts |= filters().events().error_on_conflicts();
  m_filter_conflicts |= filters().routines().error_on_conflicts();
  m_filter_conflicts |= filters().libraries().error_on_conflicts();
}

void Dump_schemas_options::set_schemas(
    const std::vector<std::string> &schemas) {
  m_schemas = {schemas.begin(), schemas.end()};
  filters().schemas().include(m_schemas);
}

void Dump_schemas_options::on_validate() const {
  Ddl_dumper_options::on_validate();

  if (m_schemas.empty()) {
    throw std::invalid_argument(
        "The 'schemas' parameter cannot be an empty list.");
  }

  const auto missing = find_missing(m_schemas);

  if (!missing.empty()) {
    throw std::invalid_argument(
        "Following schemas were not found in the database: " +
        shcore::str_join(missing, ", ",
                         [](const std::string &s) { return "'" + s + "'"; }));
  }
}

}  // namespace dump
}  // namespace mysqlsh
