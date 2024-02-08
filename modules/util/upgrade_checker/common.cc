/*
 * Copyright (c) 2017, 2024, Oracle and/or its affiliates.
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

#include "modules/util/upgrade_checker/common.h"

#include <sstream>
#include <utility>

#include "mysqlshdk/libs/utils/utils_file.h"
#include "mysqlshdk/libs/utils/utils_path.h"
#include "mysqlshdk/libs/utils/utils_string.h"
#include "mysqlshdk/libs/utils/utils_translate.h"

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

const std::string &get_translation(const char *item) {
  static shcore::Translation translation = []() {
    std::string path = shcore::get_share_folder();
    path = shcore::path::join_path(path, "upgrade_checker.msg");
    if (!shcore::path::exists(path))
      throw std::runtime_error(
          path + ": not found, shell installation likely invalid");
    return shcore::read_translation_from_file(path.c_str());
  }();

  static const std::string k_empty = "";

  auto it = translation.find(item);
  if (it == translation.end()) return k_empty;
  return it->second;
}

std::string resolve_tokens(const std::string &text,
                           const Token_definitions &replacements) {
  auto updated{text};
  for (const auto &replacement : replacements) {
    updated =
        shcore::str_replace(updated, replacement.first, replacement.second);
  }
  return updated;
}

}  // namespace upgrade_checker
}  // namespace mysqlsh
