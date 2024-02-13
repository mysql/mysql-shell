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

#include <stdexcept>
#include <utility>

#include "modules/util/upgrade_checker/sql_upgrade_check.h"

#include "modules/util/upgrade_checker/manual_check.h"
#include "modules/util/upgrade_checker/upgrade_check_condition.h"
#include "modules/util/upgrade_checker/upgrade_check_config.h"
#include "modules/util/upgrade_checker/upgrade_check_creators.h"
#include "modules/util/upgrade_checker/upgrade_check_formatter.h"

#include "mysqlshdk/libs/utils/utils_string.h"

namespace mysqlsh {
namespace upgrade_checker {

using mysqlshdk::utils::Version;

Sql_upgrade_check::Sql_upgrade_check(const std::string_view name,
                                     std::vector<std::string> &&queries,
                                     Upgrade_issue::Level level,
                                     const char *minimal_version,
                                     std::forward_list<std::string> &&set_up,
                                     std::forward_list<std::string> &&clean_up)
    : Upgrade_check(name),
      m_queries(queries),
      m_set_up(set_up),
      m_clean_up(clean_up),
      m_level(level),
      m_minimal_version(minimal_version) {}

std::vector<Upgrade_issue> Sql_upgrade_check::run(
    const std::shared_ptr<mysqlshdk::db::ISession> &session,
    const Upgrade_info &server_info) {
  if (m_minimal_version != nullptr &&
      Version(server_info.server_version) < Version(m_minimal_version))
    throw std::runtime_error(shcore::str_format(
        "This check requires server to be at minimum at %s version",
        m_minimal_version));

  for (const auto &stm : m_set_up) session->execute(stm);

  std::vector<Upgrade_issue> issues;
  for (const auto &query : m_queries) {
    auto result = session->query(query);
    const mysqlshdk::db::IRow *row = nullptr;
    while ((row = result->fetch_one()) != nullptr) {
      add_issue(row, &issues);
    }
  }

  for (const auto &stm : m_clean_up) session->execute(stm);

  return issues;
}

void Sql_upgrade_check::add_issue(const mysqlshdk::db::IRow *row,
                                  std::vector<Upgrade_issue> *issues) {
  Upgrade_issue issue = parse_row(row);
  if (!issue.empty()) issues->emplace_back(std::move(issue));
}

Upgrade_issue Sql_upgrade_check::parse_row(const mysqlshdk::db::IRow *row) {
  Upgrade_issue problem;
  auto fields_count = row->num_fields();
  problem.schema = row->get_as_string(0);
  if (fields_count > 2) problem.table = row->get_as_string(1);
  if (fields_count > 3) problem.column = row->get_as_string(2);
  if (fields_count > 1) {
    problem.description = row->get_as_string(3 - (4 - fields_count));
    if (shcore::str_beginswith(problem.description, "##")) {
      problem.description = get_text(problem.description.substr(2).c_str());
    }
  }
  problem.level = m_level;

  return problem;
}

}  // namespace upgrade_checker
}  // namespace mysqlsh
