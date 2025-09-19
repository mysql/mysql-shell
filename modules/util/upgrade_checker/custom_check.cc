/*
 * Copyright (c) 2024, 2025, Oracle and/or its affiliates.
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

#include <cassert>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "modules/util/upgrade_checker/custom_check.h"
#include "modules/util/upgrade_checker/upgrade_check_creators.h"

#include "mysql/strings/m_ctype.h"
#include "mysqlshdk/libs/db/result.h"

namespace mysqlsh {
namespace upgrade_checker {

namespace internal {
std::string decode_filename_part(std::string_view text) {
  std::string result;
  result.resize(text.length() * my_charset_utf8mb4_general_ci.mbmaxlen);
  unsigned errors = 0;
  const auto len =
      my_convert(result.data(), result.length(), &my_charset_utf8mb4_general_ci,
                 text.data(), text.length(), &my_charset_filename, &errors);
  result.resize(len);

  if (errors) {
    log_warning("Conversion of table filename '%.*s' had %u errors",
                static_cast<int>(text.length()), text.data(), errors);
  }
  if (!text.empty() && len == 0) {
    // conversion failed, returning original string
    return std::string(text);
  }

  return result;
}

std::string decode_escaped_table_name(const std::string &name) {
  std::string result;
  result.reserve(name.length());

  const auto half_pos = name.find('/');
  if (half_pos == std::string::npos || half_pos + 1 >= name.length()) {
    return decode_filename_part(name);
  }
  result = decode_filename_part(name.substr(0, half_pos));
  result += "/";
  result += decode_filename_part(name.substr(half_pos + 1));

  return result;
}
}  // namespace internal

using mysqlshdk::utils::Version;

class Invalid_engine_foreign_key_check : public Upgrade_check {
 public:
  Invalid_engine_foreign_key_check();

 private:
  std::string get_fk_57_schema_filter(Checker_cache *cache) const;

  std::vector<Upgrade_issue> run(
      const std::shared_ptr<mysqlshdk::db::ISession> &session,
      const Upgrade_info &server_info, Checker_cache *cache) override;
};

Invalid_engine_foreign_key_check::Invalid_engine_foreign_key_check()
    : Upgrade_check(ids::k_invalid_engine_foreign_key_check, Category::SCHEMA) {
}

std::string Invalid_engine_foreign_key_check::get_fk_57_schema_filter(
    Checker_cache *cache) const {
  return cache->query_helper().get_schema_filter(
      "substr(for_name, 1, instr(for_name, '/')-1)", true);
}

std::vector<Upgrade_issue> Invalid_engine_foreign_key_check::run(
    const std::shared_ptr<mysqlshdk::db::ISession> &session,
    const Upgrade_info &, Checker_cache *cache) {
  cache->cache_tables(session.get());

  // These tables don't have indexes and in large DBs JOINs can take literally
  // hours with a few thousand FKs, so we do the JOINs here.
  // Temporary tables would solve the problem in an easier way, but their size
  // is limited and it's too easy to fill them up.

  struct FKInfo {
    std::string schema;
    std::string table;
    std::string table_name;
    std::string ref_table;

    std::string columns;
    std::string ref_columns;
    std::string ref_engine;
  };

  std::unordered_map<std::string, FKInfo> foreign_keys;
  std::vector<Upgrade_issue> issues;

  auto result = session->query(
      "select id, for_name, ref_name from "
      "information_schema.innodb_sys_foreign where " +
      get_fk_57_schema_filter(cache));

  const mysqlshdk::db::IRow *row = nullptr;
  while ((row = result->fetch_one())) {
    // FK sreferences may be stored in different case sensitivity depending on
    // the OS and the MySQL configuration, so we need to search for table
    // references disabling case sensitivity

    auto table_name = internal::decode_escaped_table_name(row->get_string(1));
    auto table = cache->get_table(table_name, false);

    auto ref_name = internal::decode_escaped_table_name(row->get_string(2));
    auto ref_table = cache->get_table(ref_name, false);

    if (table == nullptr) {
      auto problem = create_issue();
      auto tokens = shcore::str_split(table_name, "/");
      problem.schema = tokens[0];
      if (tokens.size() > 1) {
        problem.table = tokens[1];
      }
      problem.description =
          "foreign key '" + row->get_string(0) +
          "' is defined on table whose name can not be decoded.";
      problem.level = Upgrade_issue::ERROR;

      issues.emplace_back(std::move(problem));
    } else if (ref_table == nullptr) {
      auto problem = create_issue();

      problem.schema = table->schema_name;
      problem.table = table->name;
      problem.description = "foreign key '" + row->get_string(0) +
                            "' references an unavailable table";
      problem.level = Upgrade_issue::WARNING;

      issues.emplace_back(std::move(problem));
    } else {
      // skip cases where the engines match
      if (table->engine == ref_table->engine) {
        continue;
      }

      FKInfo fk;
      fk.schema = table->schema_name;
      fk.table = row->get_string(1);
      fk.table_name = table->name;
      fk.ref_table = row->get_string(2);
      fk.ref_engine = ref_table->engine;

      foreign_keys.emplace(row->get_string(0), std::move(fk));
    }
  }

  // Early exit, if there are not even FKs that met the criteria being searched,
  // we are done.
  if (foreign_keys.empty()) {
    return issues;
  }

  result = session->query(
      "select id, for_col_name, ref_col_name from "
      "information_schema.innodb_sys_foreign_cols");

  while ((row = result->fetch_one())) {
    auto fk = foreign_keys.find(row->get_string(0));
    if (fk == foreign_keys.end()) continue;

    if (!fk->second.columns.empty()) fk->second.columns.append(",");
    fk->second.columns.append(row->get_string(1));
    if (!fk->second.ref_columns.empty()) fk->second.ref_columns.append(",");
    fk->second.ref_columns.append(row->get_string(2));
  }

  for (const auto &fk : foreign_keys) {
    auto problem = create_issue();

    problem.schema = fk.second.schema;
    problem.description = "column has invalid foreign key to column '" +
                          fk.second.ref_columns + "' from table '" +
                          fk.second.ref_table +
                          "' that is from a different database engine (" +
                          fk.second.ref_engine + ").";
    problem.table = fk.second.table_name;
    problem.column = fk.second.columns;
    problem.level = Upgrade_issue::ERROR;
    problem.object_type = Upgrade_issue::Object_type::FOREIGN_KEY;

    issues.emplace_back(std::move(problem));
  }

  return issues;
}

std::unique_ptr<Upgrade_check> get_invalid_engine_foreign_key_check() {
  return std::make_unique<Invalid_engine_foreign_key_check>();
}

}  // namespace upgrade_checker
}  // namespace mysqlsh
