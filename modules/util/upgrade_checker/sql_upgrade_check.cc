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
                                     std::vector<Check_query> &&queries,
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
    const Upgrade_info &server_info, Checker_cache *cache) {
  if (m_minimal_version != nullptr &&
      Version(server_info.server_version) < Version(m_minimal_version))
    throw std::runtime_error(shcore::str_format(
        "This check requires server to be at minimum at %s version",
        m_minimal_version));

  for (const auto &stm : m_set_up) session->execute(stm);

  std::vector<Upgrade_issue> issues;
  for (const auto &query : m_queries) {
    auto final_query = shcore::str_subvars(
        query.first,
        [&cache](std::string_view key) {
          const auto qh = cache->query_helper();
          std::string filter;
          if (key.compare("schema_filter") == 0) {
            filter = qh.schema_filter();
          } else if (key.compare("schema_and_table_filter") == 0) {
            filter = qh.schema_and_table_filter();
          } else if (key.compare("schema_and_routine_filter") == 0) {
            filter = qh.schema_and_routine_filter();
          } else if (key.compare("schema_and_trigger_filter") == 0) {
            filter = qh.schema_and_trigger_filter();
          } else if (key.compare("schema_and_event_filter") == 0) {
            filter = qh.schema_and_event_filter();
          } else if (shcore::str_endswith(key, "schema_and_table_filter")) {
            filter = qh.schema_and_table_filter(
                {{shcore::str_replace(key, "schema_and_table_filter",
                                      "TABLE_SCHEMA"),
                  {},
                  "tables",
                  ""},
                 shcore::str_replace(key, "schema_and_table_filter",
                                     "TABLE_NAME")});
          }

          // In both standard UC run as well as in execution from D&L there's
          // always schemas filter at least (to exclude the system schemas), if
          // this fails indicates some error in the logic that allows not
          // excluding the system schemas.
          assert(!filter.empty());

          return filter;
        },
        "<<", ">>");

    auto result = session->query(final_query);
    const mysqlshdk::db::IRow *row = nullptr;
    while ((row = result->fetch_one()) != nullptr) {
      add_issue(row, query.second, &issues);
    }
  }

  for (const auto &stm : m_clean_up) session->execute(stm);

  return issues;
}

void Sql_upgrade_check::add_issue(const mysqlshdk::db::IRow *row,
                                  Upgrade_issue::Object_type object_type,
                                  std::vector<Upgrade_issue> *issues) {
  Upgrade_issue issue = parse_row(row, object_type);
  if (!issue.empty()) issues->emplace_back(std::move(issue));
}

Upgrade_issue Sql_upgrade_check::parse_row(
    const mysqlshdk::db::IRow *row, Upgrade_issue::Object_type object_type) {
  Upgrade_issue problem;
  problem.object_type = object_type;
  auto fields_count = row->num_fields();
  std::string issue_details;
  problem.schema = row->get_as_string(0);
  if (fields_count > 2) problem.table = row->get_as_string(1);
  if (fields_count > 3) problem.column = row->get_as_string(2);
  if (fields_count > 1) {
    issue_details = row->get_as_string(3 - (4 - fields_count));
  }

  // If the issue details start with ##, it is interpreted as a token to be
  // resolved from the messages file
  if (shcore::str_beginswith(issue_details, "##")) {
    problem.description = get_text(issue_details.substr(2).c_str());
  } else {
    // Otherwise tries to get a generic issue description defined in the
    // messages file
    problem.description = get_text("issue");
  }

  // The description was loaded from the messages file, so token resolution is
  // performed
  if (!problem.description.empty()) {
    Token_definitions tokens;
    if (!problem.schema.empty()) {
      tokens["schema"] = problem.schema;
    }

    if (!problem.table.empty()) {
      tokens["table"] = problem.table;
    }

    if (!problem.column.empty()) {
      tokens["column"] = problem.column;
    }

    if (!issue_details.empty()) {
      tokens["details"] = std::move(issue_details);
    }

    tokens["level"] = Upgrade_issue::level_to_string(get_level());

    problem.description = resolve_tokens(problem.description, tokens);
  } else {
    // Otherwise issue_details is taken as the description itself (backward
    // compatibility)
    problem.description = std::move(issue_details);
  }

  problem.level = m_level;

  return problem;
}

}  // namespace upgrade_checker
}  // namespace mysqlsh
