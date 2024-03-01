/*
 * Copyright (c) 2020, 2024, Oracle and/or its affiliates.
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

#include "mysqlshdk/libs/db/query_helper.h"

#include <string>

#include "mysqlshdk/libs/utils/utils_sqlstring.h"
#include "mysqlshdk/libs/utils/utils_string.h"

namespace mysqlshdk {
namespace db {

namespace {

// ASCII NUL (U+0000) is not permitted in quoted or unquoted identifiers
const std::string k_template_marker{"\0", 1};
const std::string k_schema_var{"s"};
const std::string k_schema_template =
    k_template_marker + k_schema_var + k_template_marker;
const std::string k_table_var{"t"};
const std::string k_table_template =
    k_template_marker + k_table_var + k_template_marker;
}  // namespace

Query_helper::Query_helper(const Filtering_options &filters)
    : m_filters{filters} {}

std::string Query_helper::case_sensitive_compare(const std::string &column,
                                                 const std::string &value) {
  // we're forcing case-sensitive collation on all platforms (effectively
  // ignoring lower_case_table_names system variable)
  // STRCMP is used to overcome problems with comparison in 5.7
  return shcore::sqlstring("STRCMP(" + column + " COLLATE utf8_bin,?)", 0)
         << value;
}

std::string Query_helper::compare(const std::string &column,
                                  const std::string &value) {
  return shcore::sqlstring(column + "=?", 0) << value;
}

std::string Query_helper::build_query(const Iterate_schema &info,
                                      const std::string &filter) const {
  std::string query = "SELECT " + info.schema_column;

  build_query(info, filter, &query);

  return query;
}

std::string Query_helper::build_query(const Iterate_table &info,
                                      const std::string &filter) const {
  std::string query = "SELECT " + info.schema_column + "," + info.table_column;

  build_query(info, filter, &query);

  return query;
}

std::string Query_helper::case_sensitive_match(const Iterate_table &info,
                                               const Object &table) {
  return "(" + case_sensitive_compare(info.schema_column, table.schema) +
         "=0 AND " + case_sensitive_compare(info.table_column, table.name) +
         "=0)";
}

void Query_helper::build_query(const Iterate_schema &info,
                               const std::string &filter, std::string *query) {
  if (!info.extra_columns.empty()) {
    *query += "," + shcore::str_join(info.extra_columns, ",");
  }

  *query += " FROM information_schema." + info.table_name;

  if (!info.where.empty() || !filter.empty()) {
    *query += " WHERE " + info.where;

    if (!info.where.empty() && !filter.empty()) {
      *query += " AND ";
    }

    *query += filter;
  }
}

void Query_helper::set_schema_filter(bool case_sensitive) {
  m_schema_filter = get_schema_filter(k_schema_template, case_sensitive);
}

std::string Query_helper::get_schema_filter(const std::string &schema_id,
                                            bool case_sensitive) const {
  std::string filter = "";

  const auto &included = m_filters.schemas().included();
  const auto &excluded = m_filters.schemas().excluded();

  if (!included.empty()) {
    if (case_sensitive) {
      filter += case_sensitive_compare(schema_id, included, true);
    } else {
      filter += compare(schema_id, included, true);
    }
  }

  if (!excluded.empty()) {
    if (!included.empty()) {
      filter += " AND ";
    }

    if (case_sensitive) {
      filter += case_sensitive_compare(schema_id, excluded, false);
    } else {
      filter += compare(schema_id, excluded, false);
    }
  }

  return filter;
}

void Query_helper::set_table_filter() {
  m_table_filter = "";

  const auto &included = m_filters.tables().included();
  const auto &excluded = m_filters.tables().excluded();

  if (!included.empty()) {
    m_table_filter += "(" +
                      build_case_sensitive_filter(included, k_schema_template,
                                                  k_table_template) +
                      ")";
  }

  for (const auto &schema : excluded) {
    if (!m_table_filter.empty()) {
      m_table_filter += " AND ";
    }

    m_table_filter +=
        "NOT " + case_sensitive_match(schema.first, schema.second,
                                      k_schema_template, k_table_template);
  }
}

std::string Query_helper::schema_filter(
    const std::string &schema_column) const {
  return shcore::str_subvars(
      m_schema_filter,
      [&schema_column](std::string_view) { return schema_column; },
      k_template_marker, k_template_marker);
}

std::string Query_helper::schema_filter(const Iterate_schema &info) const {
  return schema_filter(info.schema_column);
}

std::string Query_helper::table_filter(const std::string &schema_column,
                                       const std::string &table_column) const {
  return shcore::str_subvars(
      m_table_filter,
      [&schema_column, &table_column](std::string_view var) {
        if (var == k_schema_var) return schema_column;
        if (var == k_table_var) return table_column;
        throw std::logic_error("Unknown variable: " + std::string{var});
      },
      k_template_marker, k_template_marker);
}

std::string Query_helper::schema_and_table_filter(
    const Iterate_table &info) const {
  auto result = schema_filter(info.schema_column);
  auto filter = table_filter(info.schema_column, info.table_column);

  if (!result.empty() && !filter.empty()) {
    result += " AND ";
  }

  result += filter;

  return result;
}

std::string Query_helper::object_filter(const Iterate_schema &info,
                                        const Object_filters &included,
                                        const Object_filters &excluded) const {
  std::string filter;
  const auto &object_column = info.extra_columns[0];

  if (!included.empty()) {
    filter +=
        "(" + build_filter(included, info.schema_column, object_column) + ")";
  }

  for (const auto &schema : excluded) {
    if (!filter.empty()) {
      filter += " AND ";
    }

    filter += "NOT " + match(schema.first, schema.second, info.schema_column,
                             object_column);
  }

  return filter;
}

std::string Query_helper::trigger_filter(const Iterate_table &info) const {
  const auto &included = m_filters.triggers().included();
  const auto &excluded = m_filters.triggers().excluded();
  const auto filter_triggers = [&info](const std::string &schema,
                                       const Object_filters &tables) {
    const auto &trigger_column = info.extra_columns[0];
    const auto schema_filter =
        case_sensitive_compare(info.schema_column, schema) + "=0";
    std::string filter;

    for (const auto &table : tables) {
      filter += "(" + schema_filter + " AND " +
                case_sensitive_compare(info.table_column, table.first) + "=0";

      if (!table.second.empty()) {
        // only the specified triggers are used
        filter +=
            " AND" + case_sensitive_compare(trigger_column, table.second, true);
      }

      filter += ")OR";
    }

    if (!filter.empty()) {
      // remove trailing OR
      filter.pop_back();
      filter.pop_back();
    }

    return filter;
  };

  std::string filter;

  if (!included.empty()) {
    filter += "(" +
              shcore::str_join(included.begin(), included.end(), "OR",
                               [&filter_triggers](const auto &v) {
                                 return filter_triggers(v.first, v.second);
                               }) +
              ")";
  }

  for (const auto &schema : excluded) {
    if (!filter.empty()) {
      filter += " AND ";
    }

    filter += "NOT(" + filter_triggers(schema.first, schema.second) + ")";
  }

  return filter;
}

std::string Query_helper::event_filter(const Iterate_schema &info) const {
  return object_filter(info, m_filters.events().included(),
                       m_filters.events().excluded());
}

std::string Query_helper::routine_filter(const Iterate_schema &info) const {
  return object_filter(info, m_filters.routines().included(),
                       m_filters.routines().excluded());
}

std::string Query_helper::schema_and_routine_filter() const {
  mysqlshdk::db::Iterate_schema routine_data = {
      "ROUTINE_SCHEMA", {"ROUTINE_NAME"}, "routines", ""};

  auto result = schema_filter("ROUTINE_SCHEMA");
  auto filter = routine_filter(routine_data);

  if (!result.empty() && !filter.empty()) {
    result += " AND ";
  }

  result += filter;

  return result;
}

std::string Query_helper::schema_and_trigger_filter() const {
  mysqlshdk::db::Iterate_table trigger_data = {
      {"TRIGGER_SCHEMA", {}, "triggers", ""}, "TRIGGER_NAME"};

  auto result = schema_filter("TRIGGER_SCHEMA");

  auto filter = table_filter("EVENT_OBJECT_SCHEMA", "EVENT_OBJECT_TABLE");
  if (!result.empty() && !filter.empty()) {
    result += " AND ";
  }
  result += filter;

  filter = trigger_filter(trigger_data);
  if (!result.empty() && !filter.empty()) {
    result += " AND ";
  }
  result += filter;

  return result;
}

std::string Query_helper::schema_and_event_filter() const {
  mysqlshdk::db::Iterate_schema event_data = {
      "EVENT_SCHEMA", {"EVENT_NAME"}, "events", ""};

  auto result = schema_filter("EVENT_SCHEMA");
  auto filter = event_filter(event_data);

  if (!result.empty() && !filter.empty()) {
    result += " AND ";
  }

  result += filter;

  return result;
}

}  // namespace db
}  // namespace mysqlshdk