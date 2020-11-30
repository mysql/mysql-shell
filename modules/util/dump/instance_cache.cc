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

#include "modules/util/dump/instance_cache.h"

#include <algorithm>
#include <iterator>
#include <stdexcept>
#include <utility>

#include "mysqlshdk/libs/utils/debug.h"
#include "mysqlshdk/libs/utils/logger.h"
#include "mysqlshdk/libs/utils/utils_net.h"
#include "mysqlshdk/libs/utils/utils_string.h"
#include "mysqlshdk/libs/utils/version.h"

#include "modules/util/dump/schema_dumper.h"

namespace mysqlsh {
namespace dump {

namespace {

std::string case_sensitive_compare(const std::string &column,
                                   const std::string &value) {
  // we're forcing case-sensitive collation on all platforms (effectively
  // ignoring lower_case_table_names system variable)
  // STRCMP is used to overcome problems with comparison in 5.7
  return shcore::sqlstring("STRCMP(" + column + " COLLATE utf8_bin, ?)", 0)
         << value;
}

template <typename C>
std::string case_sensitive_compare(const std::string &column,
                                   const C &container, bool equals) {
  return "(" +
         shcore::str_join(container, "&",
                          [&column](const auto &v) {
                            return case_sensitive_compare(column, v);
                          }) +
         ")" + (!equals ? "!" : "") + "=0";
}

template <typename C>
Instance_cache_builder::Objects keys(const C &map) {
  Instance_cache_builder::Objects k;

  std::transform(map.begin(), map.end(), std::inserter(k, k.begin()),
                 [](const auto &p) { return p.first; });

  return k;
}

template <typename T>
std::string match_tables(const std::string &schema, const T &tables,
                         const std::string &schema_column,
                         const std::string &table_column) {
  return "(" + case_sensitive_compare(schema_column, schema) + "=0 AND " +
         case_sensitive_compare(table_column, tables, true) + ")";
}

template <typename C>
std::string build_objects_filter(
    const C &map, const std::string &schema_column,
    const std::string &table_column,
    const std::function<Instance_cache_builder::Objects(
        const typename C::mapped_type &)> &list) {
  std::string filter;

  for (const auto &schema : map) {
    const auto l = list(schema.second);

    if (!l.empty()) {
      filter +=
          match_tables(schema.first, l, schema_column, table_column) + "OR";
    }
  }

  if (!filter.empty()) {
    // remove trailing OR
    filter.pop_back();
    filter.pop_back();
  }

  return filter;
}

}  // namespace

std::string Instance_cache::Index::order_by() const {
  return shcore::str_join(
      columns, ",", [](const auto &c) { return shcore::quote_identifier(c); });
}

struct Instance_cache_builder::Iterate_schema {
  std::string schema_column;
  std::vector<std::string> extra_columns;
  std::string table_name;
  std::vector<std::string> order_by;
};

struct Instance_cache_builder::Iterate_table
    : public Instance_cache_builder::Iterate_schema {
  std::string table_column;
  std::string where;
};

Instance_cache_builder::Instance_cache_builder(
    const std::shared_ptr<mysqlshdk::db::ISession> &session,
    const Objects &included_schemas, const Schema_objects &included_tables,
    const Objects &excluded_schemas, const Schema_objects &excluded_tables,
    bool include_metadata)
    : m_session(session) {
  filter_schemas(included_schemas, excluded_schemas);
  filter_tables(included_tables, excluded_tables);

  if (include_metadata) {
    fetch_metadata();
  }
}

Instance_cache_builder::Instance_cache_builder(
    const std::shared_ptr<mysqlshdk::db::ISession> &session,
    Instance_cache &&cache, bool include_metadata)
    : m_session(session), m_cache(std::move(cache)) {
  Objects schemas;

  for (const auto &schema : m_cache.schemas) {
    schemas.emplace(schema.first);
    m_has_tables = !schema.second.tables.empty();
    m_has_views = !schema.second.views.empty();
  }

  set_schemas_list(std::move(schemas));

  if (include_metadata) {
    fetch_metadata();
  }
}

Instance_cache_builder &Instance_cache_builder::users(const Users &included,
                                                      const Users &excluded) {
  m_cache.users = Schema_dumper(m_session).get_users(included, excluded);

  return *this;
}

Instance_cache_builder &Instance_cache_builder::events() {
  Iterate_schema info;
  info.schema_column = "EVENT_SCHEMA";  // NOT NULL
  info.extra_columns = {"EVENT_NAME"};  // NOT NULL
  info.table_name = "events";

  iterate_schemas(info, [](const std::string &, Instance_cache::Schema *schema,
                           const mysqlshdk::db::IRow *row) {
    schema->events.emplace(row->get_string(1));
  });

  return *this;
}

Instance_cache_builder &Instance_cache_builder::routines() {
  Iterate_schema info;
  info.schema_column = "ROUTINE_SCHEMA";  // NOT NULL
  info.extra_columns = {"ROUTINE_TYPE",   // NOT NULL
                        "ROUTINE_NAME"};  // NOT NULL
  info.table_name = "routines";

  const std::string procedure = "PROCEDURE";

  iterate_schemas(
      info, [&procedure](const std::string &, Instance_cache::Schema *schema,
                         const mysqlshdk::db::IRow *row) {
        auto &target = row->get_string(1) == procedure ? schema->procedures
                                                       : schema->functions;

        target.emplace(row->get_string(2));
      });

  return *this;
}

Instance_cache_builder &Instance_cache_builder::triggers() {
  if (m_has_tables) {
    Iterate_table info;
    info.schema_column = "TRIGGER_SCHEMA";     // NOT NULL
    info.table_column = "EVENT_OBJECT_TABLE";  // NOT NULL
    info.extra_columns = {"TRIGGER_NAME"};     // NOT NULL
    info.table_name = "triggers";
    info.order_by = {"ACTION_ORDER"};

    iterate_tables(info, [](const std::string &, Instance_cache::Table *table,
                            const mysqlshdk::db::IRow *row) {
      table->triggers.emplace_back(row->get_string(2));
    });
  }

  return *this;
}

Instance_cache Instance_cache_builder::build() { return std::move(m_cache); }

void Instance_cache_builder::filter_schemas(const Objects &included,
                                            const Objects &excluded) {
  std::string query =
      "SELECT "
      "SCHEMA_NAME, "            // NOT NULL
      "DEFAULT_COLLATION_NAME "  // NOT NULL
      "FROM information_schema.schemata";

  if (!included.empty() || !excluded.empty()) {
    query += " WHERE ";
  }

  const std::string schema_column = "SCHEMA_NAME";

  if (!included.empty()) {
    query += case_sensitive_compare(schema_column, included, true);
  }

  if (!excluded.empty()) {
    if (!included.empty()) {
      query += " AND ";
    }

    query += case_sensitive_compare(schema_column, excluded, false);
  }

  Objects schemas;

  {
    const auto result = m_session->query(query);

    while (const auto row = result->fetch_one()) {
      auto schema = row->get_string(0);

      m_cache.schemas[schema].collation = row->get_string(1);
      schemas.emplace(std::move(schema));
    }
  }

  set_schemas_list(std::move(schemas));
}

void Instance_cache_builder::filter_tables(const Schema_objects &included,
                                           const Schema_objects &excluded) {
  const std::string schema_column = "TABLE_SCHEMA";

  std::string query =
      "SELECT "
      "TABLE_SCHEMA, "    // NOT NULL
      "TABLE_NAME, "      // NOT NULL
      "TABLE_TYPE, "      // NOT NULL
      "TABLE_ROWS, "      // can be NULL
      "AVG_ROW_LENGTH, "  // can be NULL
      "ENGINE, "          // can be NULL
      "CREATE_OPTIONS, "  // can be NULL
      "TABLE_COMMENT "    // can be NULL in 8.0
      "FROM information_schema.tables WHERE " +
      case_sensitive_compare(schema_column, m_schemas, true);

  const std::string table_column = "TABLE_NAME";

  if (!included.empty()) {
    query += " AND (" +
             build_objects_filter(included, schema_column, table_column,
                                  [](const Objects &obj) { return obj; }) +
             ")";
  }

  for (const auto &schema : excluded) {
    query += " AND NOT " + match_tables(schema.first, schema.second,
                                        schema_column, table_column);
  }

  query += " ORDER BY TABLE_SCHEMA";

  {
    const auto result = m_session->query(query);
    std::string current_schema;
    Instance_cache::Schema *schema = nullptr;

    while (const auto row = result->fetch_one()) {
      {
        auto schema_name = row->get_string(0);

        if (schema_name != current_schema) {
          current_schema = std::move(schema_name);
          schema = &m_cache.schemas.at(current_schema);
        }
      }

      const auto table_name = row->get_string(1);
      const auto table_type = row->get_string(2);

      if ("BASE TABLE" == table_type) {
        auto &table = schema->tables[table_name];
        table.row_count = row->get_uint(3, 0);
        table.average_row_length = row->get_uint(4, 0);
        table.engine = row->get_string(5, "");
        table.create_options = row->get_string(6, "");
        table.comment = row->get_string(7, "");
        m_has_tables = true;

        DBUG_EXECUTE_IF("dumper_average_row_length_0",
                        { table.average_row_length = 0; });
      } else if ("VIEW" == table_type) {
        // nop, just to insert the view
        schema->views[table_name].character_set_client = "";
        m_has_views = true;
      }
    }
  }
}

void Instance_cache_builder::fetch_metadata() {
  fetch_ndbinfo();
  fetch_server_metadata();
  fetch_view_metadata();
  fetch_table_columns();
  fetch_table_indexes();
  fetch_table_histograms();
}

void Instance_cache_builder::fetch_server_metadata() {
  const auto &co = m_session->get_connection_options();

  m_cache.user = co.get_user();

  {
    const auto result = m_session->query("SELECT @@GLOBAL.HOSTNAME;");

    if (const auto row = result->fetch_one()) {
      m_cache.server = row->get_string(0);
    } else {
      m_cache.server = co.has_host() ? co.get_host() : "localhost";
    }
  }

  {
    const auto result = m_session->query("SELECT @@GLOBAL.VERSION;");

    if (const auto row = result->fetch_one()) {
      m_cache.server_version = row->get_string(0);
    } else {
      m_cache.server_version = "unknown";
    }
  }

  m_cache.hostname = mysqlshdk::utils::Net::get_hostname();

  {
    const auto result = m_session->query("SELECT @@GLOBAL.GTID_EXECUTED;");

    if (const auto row = result->fetch_one()) {
      m_cache.gtid_executed = row->get_string(0);
    }
  }
}

void Instance_cache_builder::fetch_ndbinfo() {
  try {
    const auto result =
        m_session->query("SHOW VARIABLES LIKE 'ndbinfo\\_version'");
    m_cache.has_ndbinfo = nullptr != result->fetch_one();
  } catch (const mysqlshdk::db::Error &) {
    log_error("Failed to check if instance is a part of NDB cluster.");
  }
}

void Instance_cache_builder::fetch_view_metadata() {
  if (!m_has_views) {
    return;
  }

  Iterate_table info;
  info.schema_column = "TABLE_SCHEMA";            // NOT NULL
  info.table_column = "TABLE_NAME";               // NOT NULL
  info.extra_columns = {"CHARACTER_SET_CLIENT",   // NOT NULL
                        "COLLATION_CONNECTION"};  // NOT NULL
  info.table_name = "views";

  iterate_views(info, [](const std::string &, Instance_cache::View *view,
                         const mysqlshdk::db::IRow *row) {
    view->character_set_client = row->get_string(2);
    view->collation_connection = row->get_string(3);
  });
}

void Instance_cache_builder::fetch_table_columns() {
  if (!m_has_tables) {
    return;
  }

  Iterate_table info;
  info.schema_column = "TABLE_SCHEMA";  // NOT NULL
  info.table_column = "TABLE_NAME";     // NOT NULL
  info.extra_columns = {"COLUMN_NAME",  // can be NULL in 8.0
                        "DATA_TYPE"};   // can be NULL in 8.0
  info.table_name = "columns";
  info.where = "EXTRA <> 'VIRTUAL GENERATED' AND EXTRA <> 'STORED GENERATED'";
  info.order_by = {"ORDINAL_POSITION"};

  iterate_tables(info, [](const std::string &, Instance_cache::Table *table,
                          const mysqlshdk::db::IRow *row) {
    Instance_cache::Column column;
    // these can be NULL in 8.0, as per output of 'SHOW COLUMNS', but it's not
    // likely, as they're NOT NULL in definition of mysql.columns hidden table
    column.name = row->get_string(2, "");
    const auto type = row->get_string(3, "");
    column.csv_unsafe = shcore::str_iendswith(
        type, "binary", "bit", "blob", "geometry", "geomcollection",
        "geometrycollection", "linestring", "point", "polygon");

    table->columns.emplace_back(std::move(column));
  });
}

void Instance_cache_builder::fetch_table_indexes() {
  if (!m_has_tables) {
    return;
  }

  Iterate_table info;
  info.schema_column = "TABLE_SCHEMA";  // NOT NULL
  info.table_column = "TABLE_NAME";
  info.extra_columns = {"INDEX_NAME",    // can be NULL in 8.0
                        "COLUMN_NAME"};  // can be NULL in 8.0
  info.table_name = "statistics";
  info.where = "COLUMN_NAME IS NOT NULL AND NON_UNIQUE = 0";
  info.order_by = {"INDEX_NAME", "SEQ_IN_INDEX"};

  const std::string primary_index = "PRIMARY";
  std::string index_name;
  std::string current_table;

  iterate_tables(
      info, [&primary_index, &index_name, &current_table](
                const std::string &table_name, Instance_cache::Table *table,
                const mysqlshdk::db::IRow *row) {
        if (table_name != current_table) {
          current_table = table_name;
          // table has changed, reset index name
          index_name.clear();
        }

        // this can be NULL in 8.0, as per output of 'SHOW COLUMNS', but it's
        // not likely, as it's NOT NULL in definition of mysql.indexes hidden
        // table
        const auto current_index = row->get_string(2, "");
        const auto primary = (current_index == primary_index);

        if (index_name.empty() || (index_name != current_index && primary)) {
          index_name = current_index;
          table->index.columns.clear();
          table->index.primary = primary;
        }

        if (index_name == current_index) {
          // NULL values in COLUMN_NAME are filtered out
          table->index.columns.emplace_back(row->get_string(3));
        }
      });
}

void Instance_cache_builder::fetch_table_histograms() {
  using mysqlshdk::utils::Version;

  if (!m_has_tables || Version(m_cache.server_version) < Version(8, 0, 0)) {
    return;
  }

  Iterate_table info;
  info.schema_column = "SCHEMA_NAME";               // NOT NULL
  info.table_column = "TABLE_NAME";                 // NOT NULL
  info.extra_columns = {"COLUMN_NAME",              // NOT NULL
                        "JSON_EXTRACT(HISTOGRAM, "  // NOT NULL
                        "'$.\"number-of-buckets-specified\"')"};
  info.table_name = "column_statistics";
  info.order_by = {"COLUMN_NAME"};

  iterate_tables(info, [](const std::string &, Instance_cache::Table *table,
                          const mysqlshdk::db::IRow *row) {
    Instance_cache::Histogram histogram;

    histogram.column = row->get_string(2);
    histogram.buckets = shcore::lexical_cast<std::size_t>(row->get_string(3));

    table->histograms.emplace_back(std::move(histogram));
  });
}

std::string Instance_cache_builder::build_query(const Iterate_schema &info) {
  std::string query = "SELECT " + info.schema_column;

  if (!info.extra_columns.empty()) {
    query += "," + shcore::str_join(info.extra_columns, ",");
  }

  query += " FROM information_schema." + info.table_name + " WHERE " +
           case_sensitive_compare(info.schema_column, m_schemas, true) +
           " ORDER BY " + info.schema_column;

  if (!info.order_by.empty()) {
    query += "," + shcore::str_join(info.order_by, ",");
  }

  return query;
}

void Instance_cache_builder::iterate_schemas(
    const Iterate_schema &info,
    const std::function<void(const std::string &, Instance_cache::Schema *,
                             const mysqlshdk::db::IRow *)> &callback) {
  const auto result = m_session->query(build_query(info));
  std::string current_schema;
  Instance_cache::Schema *schema = nullptr;

  while (const auto row = result->fetch_one()) {
    {
      auto schema_name = row->get_string(0);

      if (schema_name != current_schema) {
        current_schema = std::move(schema_name);
        schema = &m_cache.schemas.at(current_schema);
      }
    }

    callback(current_schema, schema, row);
  }
}

std::string Instance_cache_builder::build_query(
    const Iterate_table &info,
    const std::function<Instance_cache_builder::Objects(
        const Instance_cache::Schema &)> &list) {
  std::string query = "SELECT " + info.schema_column + "," + info.table_column;

  if (!info.extra_columns.empty()) {
    query += "," + shcore::str_join(info.extra_columns, ",");
  }

  query += " FROM information_schema." + info.table_name + " WHERE ";

  if (!info.where.empty()) {
    query += info.where + " AND ";
  }

  query += "(" +
           build_objects_filter(m_cache.schemas, info.schema_column,
                                info.table_column, list) +
           ") ORDER BY " + info.schema_column + "," + info.table_column;

  if (!info.order_by.empty()) {
    query += "," + shcore::str_join(info.order_by, ",");
  }

  return query;
}

void Instance_cache_builder::iterate_tables(
    const Iterate_table &info,
    const std::function<void(const std::string &, Instance_cache::Table *,
                             const mysqlshdk::db::IRow *)> &callback) {
  const auto result = m_session->query(
      build_query(info, [](const Instance_cache::Schema &schema) {
        return keys(schema.tables);
      }));

  std::string current_schema;
  Instance_cache::Schema *schema = nullptr;

  std::string current_table;
  Instance_cache::Table *table = nullptr;

  while (const auto row = result->fetch_one()) {
    {
      auto schema_name = row->get_string(0);

      if (schema_name != current_schema) {
        current_schema = std::move(schema_name);
        current_table.clear();
        schema = &m_cache.schemas.at(current_schema);
      }
    }

    {
      auto table_name = row->get_string(1);

      if (table_name != current_table) {
        current_table = std::move(table_name);
        table = &schema->tables.at(current_table);
      }
    }

    callback(current_table, table, row);
  }
}

void Instance_cache_builder::iterate_views(
    const Iterate_table &info,
    const std::function<void(const std::string &, Instance_cache::View *,
                             const mysqlshdk::db::IRow *)> &callback) {
  const auto result = m_session->query(build_query(
      info,
      [](const Instance_cache::Schema &schema) { return keys(schema.views); }));

  std::string current_schema;
  Instance_cache::Schema *schema = nullptr;

  std::string current_view;
  Instance_cache::View *view = nullptr;

  while (const auto row = result->fetch_one()) {
    {
      auto schema_name = row->get_string(0);

      if (schema_name != current_schema) {
        current_schema = std::move(schema_name);
        current_view.clear();
        schema = &m_cache.schemas.at(current_schema);
      }
    }

    {
      auto view_name = row->get_string(1);

      if (view_name != current_view) {
        current_view = std::move(view_name);
        view = &schema->views.at(current_view);
      }
    }

    callback(current_view, view, row);
  }
}

void Instance_cache_builder::set_schemas_list(Objects &&schemas) {
  if (schemas.empty()) {
    throw std::logic_error("Filters for schemas result in an empty set.");
  }

  m_schemas = std::move(schemas);
}

}  // namespace dump
}  // namespace mysqlsh
