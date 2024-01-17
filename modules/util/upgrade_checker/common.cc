/*
 * Copyright (c) 2017, 2024, Oracle and/or its affiliates.
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

#include "modules/util/upgrade_checker/common.h"

#include <sstream>
#include <utility>

namespace mysqlsh {
namespace upgrade_checker {

std::unordered_map<std::string, Version> k_latest_versions = {
    {"8", Version(MYSH_VERSION)},  // If 8 is given, latest version
                                   // is the current shell version
    {"8.0", Version(LATEST_MYSH_80_VERSION)}};

std::string upgrade_issue_to_string(const Upgrade_issue &problem) {
  std::stringstream ss;
  ss << problem.get_db_object();
  if (!problem.description.empty()) ss << " - " << problem.description;
  return ss.str();
}

const char *Upgrade_issue::level_to_string(const Upgrade_issue::Level level) {
  switch (level) {
    case Upgrade_issue::ERROR:
      return "Error";
    case Upgrade_issue::WARNING:
      return "Warning";
    case Upgrade_issue::NOTICE:
      return "Notice";
  }
  return "Notice";
}

std::string Upgrade_issue::get_db_object() const {
  std::stringstream ss;
  ss << schema;
  if (!table.empty()) ss << "." << table;
  if (!column.empty()) ss << "." << column;
  return ss.str();
}

const Checker_cache::Table_info *Checker_cache::get_table(
    const std::string &schema_table) {
  auto t = tables_.find(schema_table);
  if (t != tables_.end()) return &t->second;
  return nullptr;
}

void Checker_cache::cache_tables(mysqlshdk::db::ISession *session) {
  if (!tables_.empty()) return;

  auto res = session->query(
      R"*(SELECT table_schema, table_name, engine
FROM information_schema.tables
WHERE engine is not null and
  table_schema not in ('sys', 'mysql', 'information_schema', 'performance_schema'))*");
  while (auto row = res->fetch_one()) {
    Table_info table{row->get_string(0), row->get_string(1),
                     row->get_string(2)};

    tables_.emplace(row->get_string(0) + "/" + row->get_string(1),
                    std::move(table));
  }
}

}  // namespace upgrade_checker
}  // namespace mysqlsh