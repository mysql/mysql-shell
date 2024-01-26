/*
 * Copyright (c) 2024, Oracle and/or its affiliates.
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

#include <cassert>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "modules/util/upgrade_checker/custom_check.h"

#include "mysqlshdk/libs/db/result.h"

namespace mysqlsh {
namespace upgrade_checker {

using mysqlshdk::utils::Version;

class Invalid_engine_foreign_key_check : public Upgrade_check {
 public:
  Invalid_engine_foreign_key_check();

  std::vector<Upgrade_issue> run(
      const std::shared_ptr<mysqlshdk::db::ISession> &session,
      const Upgrade_info &server_info, Checker_cache *cache) override;

 protected:
  Upgrade_issue::Level get_level() const override {
    return Upgrade_issue::ERROR;
  }
};

Invalid_engine_foreign_key_check::Invalid_engine_foreign_key_check()
    : Upgrade_check("mysqlInvalidEngineForeignKeyCheck") {}

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

  auto result = session->query(
      "select id, for_name, ref_name from "
      "information_schema.innodb_sys_foreign");
  const mysqlshdk::db::IRow *row = nullptr;
  while ((row = result->fetch_one())) {
    auto table = cache->get_table(row->get_string(1));
    auto ref_table = cache->get_table(row->get_string(2));
    assert(table);
    assert(ref_table);
    // skip cases where the engines match
    if (table && ref_table && table->engine == ref_table->engine) continue;

    FKInfo fk;
    fk.schema = table->schema_name;
    fk.table = row->get_string(1);
    fk.table_name = table->name;
    fk.ref_table = row->get_string(2);
    fk.ref_engine = ref_table->engine;

    foreign_keys.emplace(row->get_string(0), std::move(fk));
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

  std::vector<Upgrade_issue> issues;
  for (const auto &fk : foreign_keys) {
    Upgrade_issue problem;

    problem.schema = fk.second.schema;
    problem.description = "column has invalid foreign key to column '" +
                          fk.second.ref_columns + "' from table '" +
                          fk.second.ref_table +
                          "' that is from a different database engine (" +
                          fk.second.ref_engine + ").";
    problem.table = fk.second.table_name;
    problem.column = fk.second.columns;
    problem.level = get_level();

    issues.emplace_back(std::move(problem));
  }

  return issues;
}

std::unique_ptr<Upgrade_check> get_invalid_engine_foreign_key_check() {
  return std::make_unique<Invalid_engine_foreign_key_check>();
}

}  // namespace upgrade_checker
}  // namespace mysqlsh
