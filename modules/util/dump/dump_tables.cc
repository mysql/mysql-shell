/*
 * Copyright (c) 2020, Oracle and/or its affiliates.
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

#include "modules/util/dump/dump_tables.h"

#include <set>
#include <stdexcept>
#include <utility>

#include "mysqlshdk/libs/utils/utils_string.h"

namespace mysqlsh {
namespace dump {

Dump_tables::Dump_tables(const Dump_tables_options &options)
    : Dumper(options), m_options(options) {}

void Dump_tables::create_schema_tasks() {
  Schema_task schema;
  schema.name = m_options.schema();

  if (!exists(schema.name)) {
    throw std::invalid_argument("The requested schema '" + schema.name +
                                "' was not found in the database.");
  }

  const auto add = [&schema](auto &&table, const std::string &type) {
    Table_info info{std::forward<decltype(table)>(table), ""};

    if ("BASE TABLE" == type) {
      schema.tables.emplace_back(std::move(info));
    } else if ("VIEW" == type) {
      schema.views.emplace_back(std::move(info));
    }
  };

  if (m_options.dump_all()) {
    const auto result = session()->queryf(
        "SELECT TABLE_NAME, TABLE_TYPE FROM information_schema.tables "
        "WHERE TABLE_SCHEMA = ?",
        schema.name);

    while (const auto row = result->fetch_one()) {
      add(row->get_string(0), row->get_string(1));
    }
  } else {
    std::set<std::string> missing;

    for (const auto &table : m_options.tables()) {
      const auto result = session()->queryf(
          "SELECT TABLE_TYPE FROM information_schema.tables "
          "WHERE TABLE_SCHEMA = ? AND TABLE_NAME = ?;",
          schema.name, table);
      const auto row = result->fetch_one();

      if (!row) {
        missing.emplace("'" + table + "'");
      } else {
        add(table, row->get_string(0));
      }
    }

    if (!missing.empty()) {
      throw std::invalid_argument(
          "Following tables were not found in the schema '" + schema.name +
          "': " + shcore::str_join(missing, ", "));
    }
  }

  add_schema_task(std::move(schema));
}

}  // namespace dump
}  // namespace mysqlsh
