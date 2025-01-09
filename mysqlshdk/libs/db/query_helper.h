/*
 * Copyright (c) 2020, 2025, Oracle and/or its affiliates.
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

#ifndef MYSQLSHDK_LIBS_DB_QUERY_HELPER_H_
#define MYSQLSHDK_LIBS_DB_QUERY_HELPER_H_

#include <string>
#include <vector>

#include "mysqlshdk/libs/db/filtering_options.h"
#include "mysqlshdk/libs/utils/utils_sqlstring.h"
#include "mysqlshdk/libs/utils/utils_string.h"

namespace mysqlshdk {
namespace db {

struct Iterate_schema {
  std::string schema_column;
  std::vector<std::string> extra_columns;
  std::string table_name;
  std::string where;
};

struct Iterate_table : public Iterate_schema {
  std::string table_column;
};

struct Object {
  std::string schema;
  std::string name;
};

struct Query_helper {
  using Object_filters = Filtering_options::Object_filters::Filter;
  using Trigger_filters = Filtering_options::Trigger_filters::Filter;

  explicit Query_helper(const Filtering_options &filters);

  static std::string case_sensitive_compare(const std::string &column,
                                            const std::string &value);

  template <typename It>
  static std::string case_sensitive_compare(const std::string &column, It begin,
                                            It end, bool equals) {
    return "(" +
           shcore::str_join(begin, end, "&",
                            [&column](const auto &v) {
                              return case_sensitive_compare(column, v);
                            }) +
           ")" + (!equals ? "<>" : "=") + "0";
  }

  template <typename C>
  static std::string case_sensitive_compare(const std::string &column,
                                            const C &c, bool equals) {
    return case_sensitive_compare(column, c.begin(), c.end(), equals);
  }

  static std::string compare(const std::string &column,
                             const std::string &value);

  template <typename It>
  static std::string compare(const std::string &column, It begin, It end,
                             bool equals) {
    return "(" + column + (equals ? "" : " NOT") + " IN(" +
           shcore::str_join(
               begin, end, ",",
               [](const auto &v) { return shcore::sqlstring("?", 0) << v; }) +
           "))";
  }

  template <typename C>
  static std::string compare(const std::string &column, const C &c,
                             bool equals) {
    return compare(column, c.begin(), c.end(), equals);
  }

  static std::string case_sensitive_match(const Iterate_table &info,
                                          const Object &table);

  template <typename It>
  static std::string case_sensitive_match(const Iterate_table &info, It begin,
                                          It end) {
    return "(" +
           shcore::str_join(begin, end, "OR",
                            [&info](const auto &v) {
                              return case_sensitive_match(info, v);
                            }) +
           ")";
  }

  template <typename T>
  static std::string case_sensitive_match(const std::string &schema,
                                          const T &objects,
                                          const std::string &schema_column,
                                          const std::string &object_column) {
    return "(" + case_sensitive_compare(schema_column, schema) + "=0 AND " +
           case_sensitive_compare(object_column, objects, true) + ")";
  }

  template <typename T>
  static std::string match(const std::string &schema, const T &objects,
                           const std::string &schema_column,
                           const std::string &object_column) {
    return "(" + case_sensitive_compare(schema_column, schema) + "=0 AND " +
           compare(object_column, objects, true) + ")";
  }

  template <typename C>
  static std::string build_case_sensitive_filter(
      const C &map, const std::string &schema_column,
      const std::string &object_column) {
    return build_objects_filter<C, typename C::mapped_type>(
        map, schema_column, object_column, case_sensitive_match);
  }

  template <typename C>
  static std::string build_filter(const C &map,
                                  const std::string &schema_column,
                                  const std::string &object_column) {
    return build_objects_filter<C, typename C::mapped_type>(
        map, schema_column, object_column, match);
  }

 public:
  void set_schema_filter(bool case_sensitive = true);

  std::string get_schema_filter(const std::string &schema_id,
                                bool case_sensitive = true) const;

  void set_table_filter();

  std::string schema_filter(
      const std::string &schema_column = "SCHEMA_NAME") const;

  std::string schema_filter(const Iterate_schema &info) const;

  std::string table_filter(const std::string &schema_column,
                           const std::string &table_column) const;

  std::string schema_and_table_filter(const Iterate_table &info = {
                                          {"TABLE_SCHEMA", {}, "tables", ""},
                                          "TABLE_NAME"}) const;

  std::string object_filter(const Iterate_schema &info,
                            const Object_filters &included,
                            const Object_filters &excluded) const;

  std::string trigger_filter(const Iterate_table &info) const;

  std::string event_filter(const Iterate_schema &info) const;
  std::string routine_filter(const Iterate_schema &info) const;
  std::string library_filter(const Iterate_schema &info) const;

  std::string build_query(const Iterate_schema &info,
                          const std::string &filter) const;

  std::string build_query(const Iterate_table &info,
                          const std::string &filter) const;

  std::string schema_and_routine_filter() const;
  std::string schema_and_trigger_filter() const;
  std::string schema_and_event_filter() const;

  const Filtering_options &filters() const { return m_filters; }

 private:
  static void build_query(const Iterate_schema &info, const std::string &filter,
                          std::string *query);

  template <typename C, typename T>
  static std::string build_objects_filter(
      const C &map, const std::string &schema_column,
      const std::string &object_column,
      std::string (*fun)(const std::string &, const T &, const std::string &,
                         const std::string &)) {
    std::string filter;

    for (const auto &schema : map) {
      if (!schema.second.empty()) {
        filter +=
            fun(schema.first, schema.second, schema_column, object_column) +
            "OR";
      }
    }

    if (!filter.empty()) {
      // remove trailing OR
      filter.pop_back();
      filter.pop_back();
    }

    return filter;
  }

  const Filtering_options &m_filters;
  std::string m_schema_filter;
  std::string m_table_filter;
};

}  // namespace db
}  // namespace mysqlshdk

#endif  // MYSQLSHDK_LIBS_DB_QUERY_HELPER_H_
