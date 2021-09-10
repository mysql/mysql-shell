/*
 * Copyright (c) 2020, 2021, Oracle and/or its affiliates.
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

#include "modules/util/dump/dump_tables_options.h"

#include <set>

#include "mysqlshdk/libs/utils/utils_string.h"

namespace mysqlsh {
namespace dump {

const shcore::Option_pack_def<Dump_tables_options>
    &Dump_tables_options::options() {
  static const auto opts =
      shcore::Option_pack_def<Dump_tables_options>()
          .include<Ddl_dumper_options>()
          .optional("all", &Dump_tables_options::m_dump_all)
          .on_log(&Dump_tables_options::on_log_options);

  return opts;
}

void Dump_tables_options::set_schema(const std::string &schema) {
  m_schema = schema;
  m_included_schemas.emplace(m_schema);
}

void Dump_tables_options::set_tables(const std::vector<std::string> &tables) {
  m_has_tables = !tables.empty();

  assert(!m_schema.empty());

  if (m_has_tables) {
    m_tables = {tables.begin(), tables.end()};
    m_included_tables[m_schema].insert(m_tables.begin(), m_tables.end());
  }
}

void Dump_tables_options::validate_options() const {
  Ddl_dumper_options::validate_options();

  if (m_schema.empty()) {
    throw std::invalid_argument(
        "The 'schema' parameter cannot be an empty string.");
  }

  if (!m_dump_all && !m_has_tables) {
    throw std::invalid_argument(
        "The 'tables' parameter cannot be an empty list.");
  }

  if (m_dump_all && m_has_tables) {
    throw std::invalid_argument(
        "When the 'all' parameter is set to true, the 'tables' parameter must "
        "be an empty list.");
  }

  if (!exists(m_schema)) {
    throw std::invalid_argument("The requested schema '" + m_schema +
                                "' was not found in the database.");
  }

  if (!m_dump_all) {
    const auto missing = find_missing(m_schema, m_tables);

    if (!missing.empty()) {
      throw std::invalid_argument(
          "Following tables were not found in the schema '" + m_schema +
          "': " + shcore::str_join(missing, ", ", [](const std::string &t) {
            return "'" + t + "'";
          }));
    }
  }
}

}  // namespace dump
}  // namespace mysqlsh
