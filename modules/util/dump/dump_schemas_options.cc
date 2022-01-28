/*
 * Copyright (c) 2020, 2022, Oracle and/or its affiliates.
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
          .optional("excludeTables", &Dump_schemas_options::set_exclude_tables)
          .optional("includeTables", &Dump_schemas_options::set_include_tables)
          .optional("events", &Dump_schemas_options::m_dump_events)
          .optional("excludeEvents", &Dump_schemas_options::set_exclude_events)
          .optional("includeEvents", &Dump_schemas_options::set_include_events)
          .optional("routines", &Dump_schemas_options::m_dump_routines)
          .optional("excludeRoutines",
                    &Dump_schemas_options::set_exclude_routines)
          .optional("includeRoutines",
                    &Dump_schemas_options::set_include_routines)
          .on_done(&Dump_schemas_options::on_unpacked_options)
          .on_log(&Dump_schemas_options::on_log_options);

  return opts;
}

void Dump_schemas_options::on_unpacked_options() {
  if (mds_compatibility()) {
    // if MDS compatibility option is set, mysql schema should not be dumped
    m_excluded_schemas.emplace("mysql");
  }

  error_on_table_filters_conflicts();
  error_on_event_filters_conflicts();
  error_on_routine_filters_conflicts();
}

void Dump_schemas_options::set_schemas(
    const std::vector<std::string> &schemas) {
  m_schemas = {schemas.begin(), schemas.end()};
  m_included_schemas.insert(m_schemas.begin(), m_schemas.end());
}

void Dump_schemas_options::set_exclude_tables(
    const std::vector<std::string> &data) {
  add_object_filters(data, "table", "exclude", &m_excluded_tables);
}

void Dump_schemas_options::set_include_tables(
    const std::vector<std::string> &data) {
  add_object_filters(data, "table", "include", &m_included_tables);
}

void Dump_schemas_options::set_exclude_events(
    const std::vector<std::string> &data) {
  add_object_filters(data, "event", "exclude", &m_excluded_events);
}

void Dump_schemas_options::set_include_events(
    const std::vector<std::string> &data) {
  add_object_filters(data, "event", "include", &m_included_events);
}

void Dump_schemas_options::set_exclude_routines(
    const std::vector<std::string> &data) {
  add_object_filters(data, "routine", "exclude", &m_excluded_routines);
}

void Dump_schemas_options::set_include_routines(
    const std::vector<std::string> &data) {
  add_object_filters(data, "routine", "include", &m_included_routines);
}

void Dump_schemas_options::validate_options() const {
  Ddl_dumper_options::validate_options();

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
