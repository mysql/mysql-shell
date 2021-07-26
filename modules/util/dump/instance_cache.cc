/*
 * Copyright (c) 2020, 2021, Oracle and/or its affiliates.
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

#include <mysqld_error.h>

#include <algorithm>
#include <iterator>
#include <stdexcept>
#include <utility>

#include "mysqlshdk/include/shellcore/console.h"
#include "mysqlshdk/libs/utils/debug.h"
#include "mysqlshdk/libs/utils/logger.h"
#include "mysqlshdk/libs/utils/utils_net.h"
#include "mysqlshdk/libs/utils/utils_sqlstring.h"
#include "mysqlshdk/libs/utils/utils_string.h"

#include "modules/util/dump/schema_dumper.h"

namespace mysqlsh {
namespace dump {

void Instance_cache::Index::reset() {
  m_columns.clear();
  m_columns_sql.clear();
}

void Instance_cache::Index::add_column(Column *column) {
  m_columns.emplace_back(column);

  if (!m_columns_sql.empty()) {
    m_columns_sql += ", ";
  }

  m_columns_sql += column->quoted_name;
}

void Instance_cache::Index::set_primary(bool primary) { m_primary = primary; }

struct Instance_cache_builder::Iterate_schema {
  std::string schema_column;
  std::vector<std::string> extra_columns;
  std::string table_name;
  std::string where;
};

struct Instance_cache_builder::Iterate_table
    : public Instance_cache_builder::Iterate_schema {
  std::string table_column;
};

struct Instance_cache_builder::Query_helper {
  static std::string case_sensitive_compare(const std::string &column,
                                            const std::string &value) {
    // we're forcing case-sensitive collation on all platforms (effectively
    // ignoring lower_case_table_names system variable)
    // STRCMP is used to overcome problems with comparison in 5.7
    return shcore::sqlstring("STRCMP(" + column + " COLLATE utf8_bin,?)", 0)
           << value;
  }

  template <typename It>
  static std::string case_sensitive_compare(const std::string &column, It begin,
                                            It end, bool equals) {
    return "(" +
           shcore::str_join(begin, end, "&",
                            [&column](const auto &v) {
                              return case_sensitive_compare(column, v);
                            }) +
           ")" + (!equals ? "!" : "") + "=0";
  }

  template <typename C>
  static std::string case_sensitive_compare(const std::string &column,
                                            const C &c, bool equals) {
    return case_sensitive_compare(column, c.begin(), c.end(), equals);
  }

  static std::string build_query(const Iterate_schema &info) {
    std::string query;

    query = "SELECT " + info.schema_column;

    if (!info.extra_columns.empty()) {
      query += "," + shcore::str_join(info.extra_columns, ",");
    }

    query += " FROM information_schema." + info.table_name + " WHERE ";

    if (!info.where.empty()) {
      query += info.where + " AND ";
    }

    return query;
  }

  static std::string build_query(const Iterate_table &info) {
    std::string query =
        "SELECT " + info.schema_column + "," + info.table_column;

    if (!info.extra_columns.empty()) {
      query += "," + shcore::str_join(info.extra_columns, ",");
    }

    query += " FROM information_schema." + info.table_name + " WHERE ";

    if (!info.where.empty()) {
      query += info.where + " AND ";
    }

    return query;
  }

  static std::string match_table(const Iterate_table &info,
                                 const Object &table) {
    return "(" + case_sensitive_compare(info.schema_column, table.schema) +
           "=0 AND " + case_sensitive_compare(info.table_column, table.name) +
           "=0)";
  }

  template <typename It>
  static std::string match_tables(const Iterate_table &info, It begin, It end) {
    return "(" +
           shcore::str_join(
               begin, end, "OR",
               [&info](const auto &v) { return match_table(info, v); }) +
           ")";
  }

  template <typename T>
  static std::string match_tables(const std::string &schema, const T &tables,
                                  const std::string &schema_column,
                                  const std::string &table_column) {
    return "(" + case_sensitive_compare(schema_column, schema) + "=0 AND " +
           case_sensitive_compare(table_column, tables, true) + ")";
  }

  template <typename C>
  static std::string build_objects_filter(const C &map,
                                          const std::string &schema_column,
                                          const std::string &table_column) {
    std::string filter;

    for (const auto &schema : map) {
      if (!schema.second.empty()) {
        filter += match_tables(schema.first, schema.second, schema_column,
                               table_column) +
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
};

Instance_cache_builder::Instance_cache_builder(
    const std::shared_ptr<mysqlshdk::db::ISession> &session,
    const Object_filters &included_schemas,
    const Table_filters &included_tables,
    const Object_filters &excluded_schemas,
    const Table_filters &excluded_tables, bool include_metadata)
    : m_session(session) {
  fetch_version();
  fetch_explain_select_rows_index();

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
  Schemas schemas;

  for (const auto &schema : m_cache.schemas) {
    schemas.emplace_back(schema.first);

    for (const auto &table : schema.second.tables) {
      add_table(schema.first, table.first);
    }

    for (const auto &view : schema.second.views) {
      add_view(schema.first, view.first);
    }
  }

  set_schemas_list(std::move(schemas));

  if (include_metadata) {
    fetch_metadata();
  }
}

Instance_cache_builder &Instance_cache_builder::users(const Users &included,
                                                      const Users &excluded) {
  Schema_dumper sd{m_session};

  m_cache.users = sd.get_users(included, excluded);
  m_cache.roles = sd.get_roles(included, excluded);

  return *this;
}

Instance_cache_builder &Instance_cache_builder::events() {
  Iterate_schema info;
  info.schema_column = "EVENT_SCHEMA";  // NOT NULL
  info.extra_columns = {
      "EVENT_NAME"  // NOT NULL
  };
  info.table_name = "events";

  iterate_schemas(info, [](const std::string &, Instance_cache::Schema *schema,
                           const mysqlshdk::db::IRow *row) {
    schema->events.emplace(row->get_string(1));  // EVENT_NAME
  });

  return *this;
}

Instance_cache_builder &Instance_cache_builder::routines() {
  Iterate_schema info;
  info.schema_column = "ROUTINE_SCHEMA";  // NOT NULL
  info.extra_columns = {
      "ROUTINE_TYPE",  // NOT NULL
      "ROUTINE_NAME"   // NOT NULL
  };
  info.table_name = "routines";

  const std::string procedure = "PROCEDURE";

  iterate_schemas(
      info, [&procedure](const std::string &, Instance_cache::Schema *schema,
                         const mysqlshdk::db::IRow *row) {
        auto &target = row->get_string(1) == procedure
                           ? schema->procedures
                           : schema->functions;  // ROUTINE_TYPE

        target.emplace(row->get_string(2));  // ROUTINE_NAME
      });

  return *this;
}

Instance_cache_builder &Instance_cache_builder::triggers() {
  if (has_tables()) {
    Iterate_table info;
    info.schema_column = "TRIGGER_SCHEMA";     // NOT NULL
    info.table_column = "EVENT_OBJECT_TABLE";  // NOT NULL
    info.extra_columns = {
        "TRIGGER_NAME",  // NOT NULL
        "ACTION_ORDER"   // NOT NULL
    };
    info.table_name = "triggers";

    // schema -> table -> triggers
    std::unordered_map<
        std::string,
        std::unordered_map<std::string, std::multimap<uint64_t, std::string>>>
        triggers;

    iterate_tables(info, [&triggers](const std::string &schema_name,
                                     const std::string &table_name,
                                     Instance_cache::Table *,
                                     const mysqlshdk::db::IRow *row) {
      triggers[schema_name][table_name].emplace(
          row->get_uint(3),
          row->get_string(2));  // ACTION_ORDER, TRIGGER_NAME
    });

    for (auto &schema : triggers) {
      auto &s = m_cache.schemas.at(schema.first);

      for (auto &table : schema.second) {
        auto &t = s.tables.at(table.first);

        for (auto &trigger : table.second) {
          t.triggers.emplace_back(std::move(trigger.second));
        }
      }
    }
  }

  return *this;
}

Instance_cache_builder &Instance_cache_builder::binlog_info() {
  try {
    const auto result = m_session->query("SHOW MASTER STATUS;");

    if (const auto row = result->fetch_one()) {
      m_cache.binlog_file = row->get_string(0);    // File
      m_cache.binlog_position = row->get_uint(1);  // Position
    }
  } catch (const mysqlshdk::db::Error &e) {
    if (e.code() == ER_SPECIFIC_ACCESS_DENIED_ERROR) {
      current_console()->print_warning(
          "Could not fetch the binary log information: " + e.format());
    } else {
      throw;
    }
  }

  return *this;
}

Instance_cache Instance_cache_builder::build() { return std::move(m_cache); }

void Instance_cache_builder::filter_schemas(const Object_filters &included,
                                            const Object_filters &excluded) {
  std::string query =
      "SELECT "
      "SCHEMA_NAME,"             // NOT NULL
      "DEFAULT_COLLATION_NAME "  // NOT NULL
      "FROM information_schema.schemata";

  if (!included.empty() || !excluded.empty()) {
    query += " WHERE ";
  }

  const std::string schema_column = "SCHEMA_NAME";

  if (!included.empty()) {
    query += QH::case_sensitive_compare(schema_column, included, true);
  }

  if (!excluded.empty()) {
    if (!included.empty()) {
      query += " AND ";
    }

    query += QH::case_sensitive_compare(schema_column, excluded, false);
  }

  Schemas schemas;

  {
    const auto result = m_session->query(query);

    while (const auto row = result->fetch_one()) {
      auto schema = row->get_string(0);  // SCHEMA_NAME

      m_cache.schemas[schema].collation =
          row->get_string(1);  // DEFAULT_COLLATION_NAME
      schemas.emplace_back(std::move(schema));
    }
  }

  set_schemas_list(std::move(schemas));
}

void Instance_cache_builder::filter_tables(const Table_filters &included,
                                           const Table_filters &excluded) {
  const std::string schema_column = "TABLE_SCHEMA";
  const std::string table_column = "TABLE_NAME";

  Iterate_schema info;
  info.schema_column = schema_column;  // NOT NULL
  info.extra_columns = {
      table_column,      // NOT NULL
      "TABLE_TYPE",      // NOT NULL
      "TABLE_ROWS",      // can be NULL
      "AVG_ROW_LENGTH",  // can be NULL
      "ENGINE",          // can be NULL
      "CREATE_OPTIONS",  // can be NULL
      "TABLE_COMMENT"    // can be NULL in 8.0
  };
  info.table_name = "tables";

  if (!included.empty()) {
    info.where +=
        "(" + QH::build_objects_filter(included, schema_column, table_column) +
        ")";
  }

  for (const auto &schema : excluded) {
    if (!info.where.empty()) {
      info.where += " AND ";
    }

    info.where += "NOT " + QH::match_tables(schema.first, schema.second,
                                            schema_column, table_column);
  }

  iterate_schemas(info, [this](const std::string &schema_name,
                               Instance_cache::Schema *schema,
                               const mysqlshdk::db::IRow *row) {
    const auto table_name = row->get_string(1);  // TABLE_NAME
    const auto table_type = row->get_string(2);  // TABLE_TYPE

    if ("BASE TABLE" == table_type) {
      auto &table = schema->tables[table_name];
      table.row_count = row->get_uint(3, 0);           // TABLE_ROWS
      table.average_row_length = row->get_uint(4, 0);  // AVG_ROW_LENGTH
      table.engine = row->get_string(5, "");           // ENGINE
      table.create_options = row->get_string(6, "");   // CREATE_OPTIONS
      table.comment = row->get_string(7, "");          // TABLE_COMMENT

      add_table(schema_name, table_name);

      DBUG_EXECUTE_IF("dumper_average_row_length_0",
                      { table.average_row_length = 0; });
    } else if ("VIEW" == table_type) {
      // nop, just to insert the view
      schema->views[table_name].character_set_client.clear();

      add_view(schema_name, table_name);
    }
  });
}

void Instance_cache_builder::fetch_metadata() {
  fetch_ndbinfo();
  fetch_server_metadata();
  fetch_view_metadata();
  fetch_table_columns();
  fetch_table_indexes();
  fetch_table_histograms();
  fetch_table_partitions();
}

void Instance_cache_builder::fetch_version() {
  const auto result = m_session->query("SELECT @@GLOBAL.VERSION;");

  if (const auto row = result->fetch_one()) {
    using mysqlshdk::utils::Version;

    m_cache.server_version = Version(row->get_string(0));

    if (m_cache.server_version < Version(5, 7, 0)) {
      m_cache.server_is_5_6 = true;
    } else if (m_cache.server_version < Version(8, 0, 0)) {
      m_cache.server_is_5_7 = true;
    } else {
      m_cache.server_is_8_0 = true;
    }
  } else {
    throw std::runtime_error("Failed to fetch version of the server.");
  }
}

void Instance_cache_builder::fetch_explain_select_rows_index() {
  m_cache.explain_rows_idx =
      m_session->query("EXPLAIN SELECT 1")->field_names()->field_index("rows");
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

  m_cache.hostname = mysqlshdk::utils::Net::get_hostname();

  try {
    const auto result = m_session->query("SELECT @@GLOBAL.GTID_EXECUTED;");

    if (const auto row = result->fetch_one()) {
      m_cache.gtid_executed = row->get_string(0);
    }
  } catch (const mysqlshdk::db::Error &e) {
    log_error("Failed to fetch value of @@GLOBAL.GTID_EXECUTED: %s.",
              e.format().c_str());
    current_console()->print_warning(
        "Failed to fetch value of @@GLOBAL.GTID_EXECUTED.");
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
  if (!has_views()) {
    return;
  }

  Iterate_table info;
  info.schema_column = "TABLE_SCHEMA";  // NOT NULL
  info.table_column = "TABLE_NAME";     // NOT NULL
  info.extra_columns = {
      "CHARACTER_SET_CLIENT",  // NOT NULL
      "COLLATION_CONNECTION"   // NOT NULL
  };
  info.table_name = "views";

  iterate_views(info, [](const std::string &, const std::string &,
                         Instance_cache::View *view,
                         const mysqlshdk::db::IRow *row) {
    view->character_set_client = row->get_string(2);  // CHARACTER_SET_CLIENT
    view->collation_connection = row->get_string(3);  // COLLATION_CONNECTION
  });
}

void Instance_cache_builder::fetch_table_columns() {
  if (!has_tables()) {
    return;
  }

  Iterate_table info;
  info.schema_column = "TABLE_SCHEMA";  // NOT NULL
  info.table_column = "TABLE_NAME";     // NOT NULL
  info.extra_columns = {
      "COLUMN_NAME",       // can be NULL in 8.0
      "DATA_TYPE",         // can be NULL in 8.0
      "ORDINAL_POSITION",  // NOT NULL
      "IS_NULLABLE",       // NOT NULL
      "COLUMN_TYPE",       // NOT NULL
      "EXTRA",             // can be NULL in 8.0
  };
  info.table_name = "columns";

  // schema -> table -> columns
  std::unordered_map<
      std::string, std::unordered_map<
                       std::string, std::map<uint64_t, Instance_cache::Column>>>
      columns;

  iterate_tables(info, [&columns](const std::string &schema_name,
                                  const std::string &table_name,
                                  Instance_cache::Table *,
                                  const mysqlshdk::db::IRow *row) {
    Instance_cache::Column column;
    // these can be NULL in 8.0, as per output of 'SHOW COLUMNS', but it's
    // not likely, as they're NOT NULL in definition of mysql.columns hidden
    // table
    column.name = row->get_string(2, "");  // COLUMN_NAME
    column.quoted_name = shcore::quote_identifier(column.name);
    const auto data_type = row->get_string(3, "");  // DATA_TYPE
    column.type = mysqlshdk::db::dbstring_to_type(
        data_type, row->get_string(6));  // COLUMN_TYPE
    column.csv_unsafe =
        shcore::str_iendswith(data_type.c_str(), "binary", "blob") ||
        mysqlshdk::db::Type::Bit == column.type ||
        mysqlshdk::db::Type::Geometry == column.type;
    column.generated = row->get_string(7, "").find(" GENERATED") !=
                       std::string::npos;  // EXTRA
    column.nullable =
        shcore::str_caseeq(row->get_string(5).c_str(), "YES");  // IS_NULLABLE

    columns[schema_name][table_name].emplace(
        row->get_uint(4),
        std::move(column));  // ORDINAL_POSITION
  });

  for (auto &schema : columns) {
    auto &s = m_cache.schemas.at(schema.first);

    for (auto &table : schema.second) {
      auto &t = s.tables.at(table.first);

      t.all_columns.reserve(table.second.size());

      for (auto &column : table.second) {
        t.all_columns.emplace_back(std::move(column.second));

        if (!t.all_columns.back().generated) {
          t.columns.emplace_back(&t.all_columns.back());
        }
      }
    }
  }
}

void Instance_cache_builder::fetch_table_indexes() {
  if (!has_tables()) {
    return;
  }

  Iterate_table info;
  info.schema_column = "TABLE_SCHEMA";  // NOT NULL
  info.table_column = "TABLE_NAME";
  info.extra_columns = {
      "INDEX_NAME",   // can be NULL in 8.0
      "COLUMN_NAME",  // can be NULL in 8.0
      "SEQ_IN_INDEX"  // NOT NULL
  };
  info.table_name = "statistics";
  info.where = "COLUMN_NAME IS NOT NULL AND NON_UNIQUE=0";

  const std::string primary_index = "PRIMARY";
  // schema -> table -> index name -> columns
  std::unordered_map<
      std::string,
      std::unordered_map<
          std::string,
          std::map<std::string, std::map<uint64_t, Instance_cache::Column *>>>>
      indexes;

  iterate_tables(
      info,
      [&indexes](const std::string &schema_name, const std::string &table_name,
                 Instance_cache::Table *t, const mysqlshdk::db::IRow *row) {
        // INDEX_NAME can be NULL in 8.0, as per output of 'SHOW COLUMNS', but
        // it's not likely, as it's NOT NULL in definition of mysql.indexes
        // hidden table
        //
        // NULL values in COLUMN_NAME are filtered out
        const auto column =
            std::find_if(t->all_columns.begin(), t->all_columns.end(),
                         [name = row->get_string(3)](const auto &c) {
                           return name == c.name;
                         });  // COLUMN_NAME

        // column will not be found if user is missing SELECT privilege and it
        // was not possible to fetch column information
        if (t->all_columns.end() != column) {
          indexes[schema_name][table_name][row->get_string(2, "")].emplace(
              row->get_uint(4), &(*column));  // INDEX_NAME, SEQ_IN_INDEX
        } else {
          assert(t->all_columns.empty());
        }
      });

  for (const auto &schema : indexes) {
    auto &s = m_cache.schemas.at(schema.first);

    for (const auto &table : schema.second) {
      auto &t = s.tables.at(table.first);

      auto index = table.second.find(primary_index);
      bool primary = false;

      if (table.second.end() == index) {
        std::vector<bool> current;

        const auto mark_integer_columns = [](decltype(index) idx) {
          std::vector<bool> result;

          result.reserve(idx->second.size());

          for (const auto &c : idx->second) {
            result.emplace_back(
                c.second->type == mysqlshdk::db::Type::Integer ||
                c.second->type == mysqlshdk::db::Type::UInteger);
          }

          return result;
        };

        const auto use_given_index =
            [&index, &current](decltype(index) idx, std::vector<bool> &&cols) {
              index = idx;
              current = std::move(cols);
            };

        const auto use_index = [&use_given_index,
                                &mark_integer_columns](decltype(index) idx) {
          use_given_index(idx, mark_integer_columns(idx));
        };

        use_index(table.second.begin());

        // skip first index
        for (auto it = std::next(index); it != table.second.end(); ++it) {
          const auto size = it->second.size();

          if (size < index->second.size()) {
            // prefer shorter index
            use_index(it);
          } else if (size == index->second.size()) {
            // if indexes have equal length, prefer the one with longer run of
            // integer columns
            auto candidate = mark_integer_columns(it);
            auto candidate_column = it->second.begin();
            auto current_column = index->second.begin();

            for (std::size_t i = 0; i < size; ++i) {
              if (candidate[i] == current[i]) {
                // both columns are of the same type, check if they are nullable
                if (candidate_column->second->nullable ==
                    current_column->second->nullable) {
                  // both columns are NULL or NOT NULL
                  if (!current[i]) {
                    // both columns are not integers, use the current index
                    break;
                  }
                  // else, both column are integers, check next column
                } else {
                  // prefer NOT NULL column
                  if (!candidate_column->second->nullable) {
                    // candidate is NOT NULL, use that
                    use_given_index(it, std::move(candidate));
                  }
                  // else current column is NOT NULL
                  break;
                }
              } else {
                // columns are different
                if (candidate[i]) {
                  // candidate column is an integer, use candidate index
                  use_given_index(it, std::move(candidate));
                }
                // else, current column is an integer, use the current index
                break;
              }

              ++candidate_column;
              ++current_column;
            }
          }
          // else, candidate is longer, ignore it
        }
      } else {
        primary = true;
      }

      t.index.set_primary(primary);

      for (auto &column : index->second) {
        t.index.add_column(column.second);
      }
    }
  }
}

void Instance_cache_builder::fetch_table_histograms() {
  if (!has_tables() || !m_cache.server_is_8_0) {
    return;
  }

  try {
    Iterate_table info;
    info.schema_column = "SCHEMA_NAME";  // NOT NULL
    info.table_column = "TABLE_NAME";    // NOT NULL
    info.extra_columns = {
        "COLUMN_NAME",  // NOT NULL
        "JSON_EXTRACT(HISTOGRAM,'$.\"number-of-buckets-specified\"')"  // NOT
                                                                       // NULL
    };
    info.table_name = "column_statistics";

    iterate_tables(
        info, [](const std::string &, const std::string &,
                 Instance_cache::Table *table, const mysqlshdk::db::IRow *row) {
          Instance_cache::Histogram histogram;

          histogram.column = row->get_string(2);  // COLUMN_NAME
          histogram.buckets = shcore::lexical_cast<std::size_t>(
              row->get_string(3));  // number-of-buckets-specified

          table->histograms.emplace_back(std::move(histogram));
        });

    for (auto &schema : m_cache.schemas) {
      for (auto &table : schema.second.tables) {
        std::sort(
            table.second.histograms.begin(), table.second.histograms.end(),
            [](const auto &l, const auto &r) { return l.column < r.column; });
      }
    }
  } catch (const mysqlshdk::db::Error &e) {
    log_error("Failed to fetch table histograms: %s.", e.format().c_str());
    current_console()->print_warning("Failed to fetch table histograms.");
  }
}

void Instance_cache_builder::fetch_table_partitions() {
  Iterate_table info;
  info.schema_column = "TABLE_SCHEMA";  // NOT NULL
  info.table_column = "TABLE_NAME";     // NOT NULL
  info.extra_columns = {
      "IFNULL(SUBPARTITION_NAME, PARTITION_NAME)",  // NOT NULL
      "TABLE_ROWS",                                 // NOT NULL
      "AVG_ROW_LENGTH",                             // NOT NULL
  };
  info.table_name = "partitions";
  info.where = "PARTITION_NAME IS NOT NULL";

  iterate_tables(info, [](const std::string &, const std::string &,
                          Instance_cache::Table *table,
                          const mysqlshdk::db::IRow *row) {
    Instance_cache::Partition partition;

    partition.name = row->get_string(2);  // PARTITION_NAME or SUBPARTITION_NAME
    partition.quoted_name = shcore::quote_identifier(partition.name);
    partition.row_count = row->get_uint(3, 0);           // TABLE_ROWS
    partition.average_row_length = row->get_uint(4, 0);  // AVG_ROW_LENGTH

    table->partitions.emplace_back(std::move(partition));
  });
}

void Instance_cache_builder::iterate_schemas(
    const Iterate_schema &info,
    const std::function<void(const std::string &, Instance_cache::Schema *,
                             const mysqlshdk::db::IRow *)> &callback) {
  const auto query = QH::build_query(info);

  auto begin = m_schemas.begin();
  const auto end = m_schemas.end();
  auto size = m_schemas.size();

  while (end != begin) {
    size = std::min<decltype(size)>(size, std::distance(begin, end));

    try {
      const auto full_query =
          query + QH::case_sensitive_compare(info.schema_column, begin,
                                             begin + size, true);

      const auto result = m_session->query(full_query);

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

      begin = begin + size;
    } catch (const mysqlshdk::db::Error &e) {
      size /= 2;

      log_error("Failed to iterate schemas: %s, %s...", e.format().c_str(),
                size ? "retrying" : "aborting");

      if (!size) {
        throw;
      }
    }
  }
}

void Instance_cache_builder::iterate_tables(
    const Iterate_table &info,
    const std::function<void(const std::string &, const std::string &,
                             Instance_cache::Table *,
                             const mysqlshdk::db::IRow *)> &callback) {
  const auto query = QH::build_query(info);

  auto begin = m_tables.begin();
  const auto end = m_tables.end();
  auto size = m_tables.size();

  while (end != begin) {
    size = std::min<decltype(size)>(size, std::distance(begin, end));

    try {
      const auto full_query =
          query + QH::match_tables(info, begin, begin + size);
      const auto result = m_session->query(full_query);

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

        callback(current_schema, current_table, table, row);
      }

      begin = begin + size;
    } catch (const mysqlshdk::db::Error &e) {
      size /= 2;

      log_error("Failed to iterate tables: %s, %s...", e.format().c_str(),
                size ? "retrying" : "aborting");

      if (!size) {
        throw;
      }
    }
  }
}

void Instance_cache_builder::iterate_views(
    const Iterate_table &info,
    const std::function<void(const std::string &, const std::string &,
                             Instance_cache::View *,
                             const mysqlshdk::db::IRow *)> &callback) {
  const auto query = QH::build_query(info);

  auto begin = m_views.begin();
  const auto end = m_views.end();
  auto size = m_views.size();

  while (end != begin) {
    size = std::min<decltype(size)>(size, std::distance(begin, end));

    try {
      const auto full_query =
          query + QH::match_tables(info, begin, begin + size);
      const auto result = m_session->query(full_query);

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

        callback(current_schema, current_view, view, row);
      }

      begin = begin + size;
    } catch (const mysqlshdk::db::Error &e) {
      size /= 2;

      log_error("Failed to iterate views: %s, %s...", e.format().c_str(),
                size ? "retrying" : "aborting");

      if (!size) {
        throw;
      }
    }
  }
}

void Instance_cache_builder::set_schemas_list(Schemas &&schemas) {
  if (schemas.empty()) {
    throw std::logic_error("Filters for schemas result in an empty set.");
  }

  m_schemas = std::move(schemas);
}

void Instance_cache_builder::add_table(const std::string &schema,
                                       const std::string &table) {
  m_tables.emplace_back(Object{schema, table});
}

void Instance_cache_builder::add_view(const std::string &schema,
                                      const std::string &view) {
  m_views.emplace_back(Object{schema, view});
}

}  // namespace dump
}  // namespace mysqlsh
