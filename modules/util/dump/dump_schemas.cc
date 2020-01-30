/*
 * Copyright (c) 2020, Oracle and/or its affiliates. All rights reserved.
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

#include "modules/util/dump/dump_schemas.h"

#include <algorithm>
#include <iterator>
#include <set>
#include <stdexcept>
#include <unordered_set>
#include <utility>

#include "mysqlshdk/libs/utils/utils_string.h"

#include "modules/util/dump/schema_dumper.h"

namespace mysqlsh {
namespace dump {

Dump_schemas::Dump_schemas(const Dump_schemas_options &options)
    : Dumper(options), m_options(options) {}

void Dump_schemas::create_schema_tasks() {
  std::unordered_set<std::string> all_schemas;

  if (dump_all_schemas()) {
    const auto result = session()->query(
        "SELECT SCHEMA_NAME FROM information_schema.schemata WHERE SCHEMA_NAME "
        "NOT IN ('information_schema', 'mysql', 'ndbinfo', "
        "'performance_schema', 'sys');");

    while (const auto row = result->fetch_one()) {
      all_schemas.emplace(row->get_string(0));
    }
  } else {
    std::set<std::string> missing;

    for (const auto &s : m_options.schemas()) {
      if (!exists(s)) {
        missing.emplace("'" + s + "'");
      }
    }

    if (!missing.empty()) {
      throw std::invalid_argument(
          "Following schemas were not found in the database: " +
          shcore::str_join(missing, ", "));
    }
  }

  const auto &schemas_to_dump =
      dump_all_schemas() ? all_schemas : m_options.schemas();

  for (const auto &s : schemas_to_dump) {
    // if MDS compatibility option is set, mysql schema should not be dumped
    if (excluded_schemas().end() == excluded_schemas().find(s) &&
        ("mysql" != s || !m_options.mds_compatibility())) {
      Schema_task schema;
      schema.name = s;
      schema.tables = get_tables(schema.name);
      schema.views = get_views(schema.name);

      add_schema_task(std::move(schema));
    }
  }
}

std::vector<Dumper::Table_info> Dump_schemas::get_tables(
    const std::string &schema) {
  return get_tables(schema, "BASE TABLE");
}

std::vector<Dumper::Table_info> Dump_schemas::get_views(
    const std::string &schema) {
  return get_tables(schema, "VIEW");
}

std::vector<Dumper::Table_info> Dump_schemas::get_tables(
    const std::string &schema, const std::string &type) {
  std::set<std::string> all_tables;

  const auto result = session()->queryf(
      "SELECT TABLE_NAME FROM information_schema.tables WHERE TABLE_SCHEMA = ? "
      "AND TABLE_TYPE = ?;",
      schema, type);

  while (const auto row = result->fetch_one()) {
    all_tables.emplace(row->get_string(0));
  }

  std::vector<std::string> tables;

  const auto excluded_tables = m_options.excluded_tables().find(schema);

  if (m_options.excluded_tables().end() == excluded_tables) {
    tables.insert(tables.end(), std::make_move_iterator(all_tables.begin()),
                  std::make_move_iterator(all_tables.end()));
  } else {
    std::set_difference(std::make_move_iterator(all_tables.begin()),
                        std::make_move_iterator(all_tables.end()),
                        excluded_tables->second.begin(),
                        excluded_tables->second.end(),
                        std::back_inserter(tables));
  }

  std::vector<Dumper::Table_info> infos;

  for (auto &t : tables) {
    Table_info info;

    info.name = std::move(t);

    infos.emplace_back(std::move(info));
  }

  return infos;
}

const std::unordered_set<std::string> &Dump_schemas::excluded_schemas() const {
  static std::unordered_set<std::string> empty;
  return empty;
}

std::unique_ptr<Schema_dumper> Dump_schemas::schema_dumper(
    const std::shared_ptr<mysqlshdk::db::ISession> &session) const {
  auto dumper = Dumper::schema_dumper(session);

  const auto &options = m_options.compatibility_options();

  dumper->opt_force_innodb = options.is_set(Compatibility_option::FORCE_INNODB);
  dumper->opt_strip_definer =
      options.is_set(Compatibility_option::STRIP_DEFINERS);
  dumper->opt_strip_restricted_grants =
      options.is_set(Compatibility_option::STRIP_RESTRICTED_GRANTS);
  dumper->opt_strip_role_admin =
      options.is_set(Compatibility_option::STRIP_ROLE_ADMIN);
  dumper->opt_strip_tablespaces =
      options.is_set(Compatibility_option::STRIP_TABLESPACES);

  return dumper;
}

}  // namespace dump
}  // namespace mysqlsh
