/*
 * Copyright (c) 2020, 2023, Oracle and/or its affiliates.
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
#include "mysqlshdk/libs/db/mysql/result.h"
#include "mysqlshdk/libs/utils/debug.h"
#include "mysqlshdk/libs/utils/logger.h"
#include "mysqlshdk/libs/utils/profiling.h"
#include "mysqlshdk/libs/utils/utils_net.h"
#include "mysqlshdk/libs/utils/utils_sqlstring.h"
#include "mysqlshdk/libs/utils/utils_string.h"

#include "modules/util/dump/dump_errors.h"
#include "modules/util/dump/schema_dumper.h"

namespace mysqlsh {
namespace dump {

namespace {

// ASCII NUL (U+0000) is not permitted in quoted or unquoted identifiers
const std::string k_template_marker{"\0", 1};
const std::string k_schema_var{"s"};
const std::string k_schema_template =
    k_template_marker + k_schema_var + k_template_marker;
const std::string k_table_var{"t"};
const std::string k_table_template =
    k_template_marker + k_table_var + k_template_marker;

class Profiler final {
 public:
  Profiler() = delete;

  explicit Profiler(const char *msg) : m_msg(msg) {
    log_debug("-> %s", m_msg);

    m_duration.start();
  }

  Profiler(const Profiler &) = delete;
  Profiler(Profiler &&) = delete;

  Profiler &operator=(const Profiler &) = delete;
  Profiler &operator=(Profiler &&) = delete;

  ~Profiler() {
    m_duration.finish();

    log_debug("<- %s took %f seconds", m_msg, m_duration.seconds_elapsed());
  }

 private:
  const char *m_msg = nullptr;

  mysqlshdk::utils::Duration m_duration;
};

}  // namespace

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

  static std::string compare(const std::string &column,
                             const std::string &value) {
    return shcore::sqlstring(column + "=?", 0) << value;
  }

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

  static std::string build_query(const Iterate_schema &info,
                                 const std::string &filter) {
    std::string query = "SELECT " + info.schema_column;

    build_query(info, filter, &query);

    return query;
  }

  static std::string build_query(const Iterate_table &info,
                                 const std::string &filter) {
    std::string query =
        "SELECT " + info.schema_column + "," + info.table_column;

    build_query(info, filter, &query);

    return query;
  }

  static std::string case_sensitive_match(const Iterate_table &info,
                                          const Object &table) {
    return "(" + case_sensitive_compare(info.schema_column, table.schema) +
           "=0 AND " + case_sensitive_compare(info.table_column, table.name) +
           "=0)";
  }

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

 private:
  static void build_query(const Iterate_schema &info, const std::string &filter,
                          std::string *query) {
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
};

Instance_cache_builder::Instance_cache_builder(
    const std::shared_ptr<mysqlshdk::db::ISession> &session,
    const common::Filtering_options &filters, Instance_cache &&cache)
    : m_session(session), m_cache(std::move(cache)), m_filters(filters) {
  set_schema_filter();
  set_table_filter();

  for (const auto &schema : m_cache.schemas) {
    if (!schema.second.tables.empty()) {
      set_has_tables();
    }

    if (!schema.second.views.empty()) {
      set_has_views();
    }

    if (has_tables() && has_views()) {
      break;
    }
  }

  if (m_cache.schemas.empty()) {
    fetch_version();
    fetch_explain_select_rows_index();

    filter_schemas();
    filter_tables();
  }
}

Instance_cache_builder &Instance_cache_builder::metadata(
    const Partition_filters &partitions) {
  fetch_metadata(partitions);
  return *this;
}

Instance_cache_builder &Instance_cache_builder::users() {
  Profiler profiler{"fetching users"};

  Schema_dumper sd{m_session};

  const auto &users = m_filters.users();
  m_cache.users = sd.get_users(users);
  m_cache.roles = sd.get_roles(users);

  m_cache.filtered.users = m_cache.users.size();
  m_cache.total.users = count("user_privileges", {}, "DISTINCT grantee");

  return *this;
}

Instance_cache_builder &Instance_cache_builder::events() {
  Profiler profiler{"fetching events"};

  Iterate_schema info;
  info.schema_column = "EVENT_SCHEMA";  // NOT NULL
  info.extra_columns = {
      "EVENT_NAME"  // NOT NULL
  };
  info.table_name = "events";
  // event names are case insensitive
  info.where = object_filter(info, m_filters.events().included(),
                             m_filters.events().excluded());

  iterate_schemas(info,
                  [this](const std::string &, Instance_cache::Schema *schema,
                         const mysqlshdk::db::IRow *row) {
                    schema->events.emplace(row->get_string(1));  // EVENT_NAME

                    ++m_cache.filtered.events;
                  });

  // the total number of events within the filtered schemas
  m_cache.total.events = count(info);

  return *this;
}

Instance_cache_builder &Instance_cache_builder::routines() {
  Profiler profiler{"fetching routines"};

  Iterate_schema info;
  info.schema_column = "ROUTINE_SCHEMA";  // NOT NULL
  info.extra_columns = {
      "ROUTINE_NAME",  // NOT NULL
      "ROUTINE_TYPE",  // NOT NULL
  };
  info.table_name = "routines";
  // routine names are case insensitive
  info.where = object_filter(info, m_filters.routines().included(),
                             m_filters.routines().excluded());

  const std::string procedure = "PROCEDURE";

  iterate_schemas(info, [&procedure, this](const std::string &,
                                           Instance_cache::Schema *schema,
                                           const mysqlshdk::db::IRow *row) {
    auto &target = row->get_string(2) == procedure
                       ? schema->procedures
                       : schema->functions;  // ROUTINE_TYPE

    target.emplace(row->get_string(1));  // ROUTINE_NAME

    ++m_cache.filtered.routines;
  });

  // the total number of routines within the filtered schemas
  m_cache.total.routines = count(info);

  return *this;
}

Instance_cache_builder &Instance_cache_builder::triggers() {
  Profiler profiler{"fetching triggers"};

  if (has_tables()) {
    Iterate_table info;
    info.schema_column = "TRIGGER_SCHEMA";     // NOT NULL
    info.table_column = "EVENT_OBJECT_TABLE";  // NOT NULL
    info.extra_columns = {
        "TRIGGER_NAME",  // NOT NULL
        "ACTION_ORDER"   // NOT NULL
    };
    info.table_name = "triggers";
    // trigger names are case sensitive
    info.where = trigger_filter(info, m_filters.triggers().included(),
                                m_filters.triggers().excluded());

    // schema -> table -> triggers
    std::unordered_map<
        std::string,
        std::unordered_map<std::string, std::multimap<uint64_t, std::string>>>
        triggers;

    iterate_tables(info, [&triggers, this](const std::string &schema_name,
                                           const std::string &table_name,
                                           Instance_cache::Table *,
                                           const mysqlshdk::db::IRow *row) {
      triggers[schema_name][table_name].emplace(
          row->get_uint(3),
          row->get_string(2));  // ACTION_ORDER, TRIGGER_NAME

      ++m_cache.filtered.triggers;
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

    // the total number of triggers within the filtered tables
    m_cache.total.triggers = count(info);
  }

  return *this;
}

Instance_cache_builder &Instance_cache_builder::binlog_info() {
  Profiler profiler{"fetching binlog info"};

  m_cache.binlog = Schema_dumper{m_session}.binlog();

  return *this;
}

Instance_cache Instance_cache_builder::build() { return std::move(m_cache); }

void Instance_cache_builder::filter_schemas() {
  Profiler profiler{"filtering schemas"};

  std::string sql =
      "SELECT "
      "SCHEMA_NAME,"             // NOT NULL
      "DEFAULT_COLLATION_NAME "  // NOT NULL
      "FROM information_schema.schemata";

  const auto filter = schema_filter("SCHEMA_NAME");

  if (!filter.empty()) {
    sql += " WHERE " + filter;
  }

  {
    const auto result = query(sql);

    while (const auto row = result->fetch_one()) {
      const auto schema = row->get_string(0);  // SCHEMA_NAME

      m_cache.schemas[schema].collation =
          row->get_string(1);  // DEFAULT_COLLATION_NAME
    }
  }

  m_cache.filtered.schemas = m_cache.schemas.size();
  // the total number of schemas in an instance
  m_cache.total.schemas = count("schemata");
}

void Instance_cache_builder::filter_tables() {
  Profiler profiler{"filtering tables"};

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
  info.where = table_filter(schema_column, table_column);

  iterate_schemas(
      info, [this](const std::string &, Instance_cache::Schema *schema,
                   const mysqlshdk::db::IRow *row) {
        const auto table_type = row->get_string(2);  // TABLE_TYPE

        if ("SYSTEM VIEW" == table_type) {
          return;
        }

        const auto table_name = row->get_string(1);  // TABLE_NAME
        const auto is_table = "BASE TABLE" == table_type;
        Instance_cache::Table &target =
            is_table ? schema->tables[table_name] : schema->views[table_name];

        target.row_count = row->get_uint(3, 0);           // TABLE_ROWS
        target.average_row_length = row->get_uint(4, 0);  // AVG_ROW_LENGTH
        target.engine = row->get_string(5, "");           // ENGINE
        target.create_options = row->get_string(6, "");   // CREATE_OPTIONS
        target.comment = row->get_string(7, "");          // TABLE_COMMENT

        if (is_table) {
          set_has_tables();

          ++m_cache.filtered.tables;

          DBUG_EXECUTE_IF("dumper_average_row_length_0",
                          { target.average_row_length = 0; });
        } else {
          set_has_views();

          ++m_cache.filtered.views;
        }
      });

  // the total number of tables and views within the filtered schemas
  m_cache.total.tables = count(info, "'BASE TABLE'=TABLE_TYPE");
  m_cache.total.views = count(info, "'VIEW'=TABLE_TYPE");
}

void Instance_cache_builder::fetch_metadata(
    const Partition_filters &partitions) {
  Profiler profiler{"fetching metadata"};

  fetch_ndbinfo();
  fetch_server_metadata();
  fetch_view_metadata();
  fetch_columns();
  fetch_table_indexes();
  fetch_table_histograms();
  fetch_table_partitions(partitions);
}

void Instance_cache_builder::fetch_version() {
  Profiler profiler{"fetching version"};

  m_cache.server_version = Schema_dumper{m_session}.server_version();
}

void Instance_cache_builder::fetch_explain_select_rows_index() {
  m_cache.explain_rows_idx =
      query("EXPLAIN SELECT 1")->field_names()->field_index("rows");
}

void Instance_cache_builder::fetch_server_metadata() {
  Profiler profiler{"fetching server metadata"};

  const auto &co = m_session->get_connection_options();

  m_cache.user = co.get_user();

  {
    const auto result = query("SELECT @@GLOBAL.HOSTNAME;");

    if (const auto row = result->fetch_one()) {
      m_cache.server = row->get_string(0);
    } else {
      m_cache.server = co.has_host() ? co.get_host() : "localhost";
    }
  }

  m_cache.hostname = mysqlshdk::utils::Net::get_hostname();

  Schema_dumper dumper{m_session};
  m_cache.gtid_executed = dumper.gtid_executed();

  if (m_cache.server_version.version >= mysqlshdk::utils::Version(8, 0, 16)) {
    m_cache.partial_revokes = dumper.partial_revokes();
  }
}

void Instance_cache_builder::fetch_ndbinfo() {
  Profiler profiler{"fetching ndbinfo"};

  try {
    const auto result = query("SHOW VARIABLES LIKE 'ndbinfo\\_version'");
    m_cache.has_ndbinfo = nullptr != result->fetch_one();
  } catch (const mysqlshdk::db::Error &) {
    log_error("Failed to check if instance is a part of NDB cluster.");
  }
}

void Instance_cache_builder::fetch_view_metadata() {
  Profiler profiler{"fetching view metadata"};

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

void Instance_cache_builder::fetch_columns() {
  Profiler profiler{"fetching columns"};

  if (!has_tables() && !has_views()) {
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
      table_columns;
  // schema -> view -> columns
  std::unordered_map<
      std::string, std::unordered_map<
                       std::string, std::map<uint64_t, Instance_cache::Column>>>
      view_columns;

  const auto create_column = [](const mysqlshdk::db::IRow *row) {
    Instance_cache::Column column;
    // these can be NULL in 8.0, as per output of 'SHOW COLUMNS', but it's
    // not likely, as they're NOT NULL in definition of mysql.columns hidden
    // table
    column.name = row->get_string(2, "");  // COLUMN_NAME
    column.quoted_name = shcore::quote_identifier(column.name);
    const auto data_type = row->get_string(3, "");  // DATA_TYPE
    column.type = mysqlshdk::db::dbstring_to_type(
        data_type, row->get_string(6));  // COLUMN_TYPE
    column.csv_unsafe = shcore::str_iendswith(data_type, "binary", "blob") ||
                        mysqlshdk::db::Type::Bit == column.type ||
                        mysqlshdk::db::Type::Geometry == column.type;
    const auto extra = row->get_string(7, "");  // EXTRA
    column.generated = extra.find(" GENERATED") != std::string::npos;
    column.auto_increment = extra.find("auto_increment") != std::string::npos;
    column.nullable = shcore::str_caseeq(row->get_string(5),
                                         "YES");  // IS_NULLABLE

    return column;
  };

  iterate_tables_and_views(
      info,
      [&table_columns, &create_column](
          const std::string &schema_name, const std::string &table_name,
          Instance_cache::Table *, const mysqlshdk::db::IRow *row) {
        table_columns[schema_name][table_name].emplace(
            row->get_uint(4),  // ORDINAL_POSITION
            create_column(row));
      },
      [&view_columns, &create_column](
          const std::string &schema_name, const std::string &view_name,
          Instance_cache::View *, const mysqlshdk::db::IRow *row) {
        view_columns[schema_name][view_name].emplace(
            row->get_uint(4),  // ORDINAL_POSITION
            create_column(row));
      });

  for (auto &schema : table_columns) {
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

  for (auto &schema : view_columns) {
    auto &s = m_cache.schemas.at(schema.first);

    for (auto &view : schema.second) {
      auto &v = s.views.at(view.first);

      v.all_columns.reserve(view.second.size());

      for (auto &column : view.second) {
        v.all_columns.emplace_back(std::move(column.second));

        if (!v.all_columns.back().generated) {
          v.columns.emplace_back(&v.all_columns.back());
        }
      }
    }
  }
}

void Instance_cache_builder::fetch_table_indexes() {
  Profiler profiler{"fetching table indexes"};

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
  struct Index_info {
    std::vector<Instance_cache::Column *> columns;
    bool unsafe = false;
  };
  // schema -> table -> index name -> index info
  // indexes are ordered to ensure repeatability of the algorithm
  std::unordered_map<
      std::string,
      std::unordered_map<std::string, std::map<std::string, Index_info>>>
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
          auto &index_info = indexes[schema_name][table_name]
                                    [row->get_string(2, {})];  // INDEX_NAME
          const auto seq = row->get_uint(4);                   // SEQ_IN_INDEX

          // SEQ_IN_INDEX is 1-based
          if (seq > index_info.columns.size()) {
            index_info.columns.resize(seq);
          }

          index_info.columns[seq - 1] = &(*column);
          // BUG#35180061 - do not use indexes on ENUM columns
          index_info.unsafe |= (mysqlshdk::db::Type::Enum == column->type);
        } else {
          assert(t->all_columns.empty());
        }
      });

  for (auto &schema : indexes) {
    for (auto &table : schema.second) {
      for (auto index = table.second.begin(); index != table.second.end();) {
        if (index->second.unsafe) {
          index = table.second.erase(index);
        } else {
          ++index;
        }
      }

      if (table.second.empty()) {
        continue;
      }

      auto &t = m_cache.schemas.at(schema.first).tables.at(table.first);

      auto index = table.second.find(primary_index);
      bool primary = false;

      if (table.second.end() == index) {
        std::vector<bool> current;

        const auto mark_integer_columns = [](decltype(index) idx) {
          std::vector<bool> result;

          result.reserve(idx->second.columns.size());

          for (const auto &c : idx->second.columns) {
            result.emplace_back(c->type == mysqlshdk::db::Type::Integer ||
                                c->type == mysqlshdk::db::Type::UInteger);
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
          const auto size = it->second.columns.size();

          if (size < index->second.columns.size()) {
            // prefer shorter index
            use_index(it);
          } else if (size == index->second.columns.size()) {
            // if indexes have equal length, prefer the one with longer run of
            // integer columns
            auto candidate = mark_integer_columns(it);
            auto candidate_column = it->second.columns.begin();
            auto current_column = index->second.columns.begin();

            for (std::size_t i = 0; i < size; ++i) {
              if (candidate[i] == current[i]) {
                // both columns are of the same type, check if they are nullable
                if ((*candidate_column)->nullable ==
                    (*current_column)->nullable) {
                  // both columns are NULL or NOT NULL
                  if (!current[i]) {
                    // both columns are not integers, use the current index
                    break;
                  }
                  // else, both column are integers, check next column
                } else {
                  // prefer NOT NULL column
                  if (!(*candidate_column)->nullable) {
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

      for (auto &column : index->second.columns) {
        t.index.add_column(column);
      }
    }
  }
}

void Instance_cache_builder::fetch_table_histograms() {
  Profiler profiler{"fetching table histograms"};

  if (!has_tables() || !m_cache.server_version.is_8_0) {
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

void Instance_cache_builder::fetch_table_partitions(
    const Partition_filters &partitions) {
  Profiler profiler{"fetching table partitions"};

  if (!has_tables()) {
    return;
  }

  Iterate_table info;
  info.schema_column = "TABLE_SCHEMA";  // NOT NULL
  info.table_column = "TABLE_NAME";     // NOT NULL
  info.extra_columns = {
      "PARTITION_NAME",     // NOT NULL due to condition
      "SUBPARTITION_NAME",  // can be NULL
      "TABLE_ROWS",         // NOT NULL
      "AVG_ROW_LENGTH",     // NOT NULL
  };
  info.table_name = "partitions";
  info.where = "PARTITION_NAME IS NOT NULL";

  const auto include_partition =
      [&partitions](const std::string &schema, const std::string &table,
                    const std::string &partition,
                    const std::string &subpartition) {
        // if table is not on the list, all partitions from that table are
        // included
        const auto s = partitions.find(schema);

        if (partitions.end() == s) {
          return true;
        }

        const auto t = s->second.find(table);

        if (s->second.end() == t || t->second.empty()) {
          return true;
        }

        // we have a list of partitions for that table, include partition only
        // if it's on the list
        for (const auto &p : t->second) {
          if (partition == p || subpartition == p) {
            return true;
          }
        }

        return false;
      };

  iterate_tables(
      info, [&include_partition](const std::string &s, const std::string &t,
                                 Instance_cache::Table *table,
                                 const mysqlshdk::db::IRow *row) {
        if (shcore::str_caseeq(table->engine, "NDB", "NDBCLUSTER")) {
          // Partition selection is disabled for tables employing a storage
          // engine that supplies automatic partitioning, such as NDB. Ignore
          // such tables.
          return;
        }

        auto partition = row->get_string(2);         // PARTITION_NAME
        auto subpartition = row->get_string(3, {});  // SUBPARTITION_NAME

        if (!include_partition(s, t, partition, subpartition)) {
          return;
        }

        Instance_cache::Partition p;

        p.name = std::move(subpartition.empty() ? partition : subpartition);
        p.quoted_name = shcore::quote_identifier(p.name);
        p.row_count = row->get_uint(4, 0);           // TABLE_ROWS
        p.average_row_length = row->get_uint(5, 0);  // AVG_ROW_LENGTH

        table->partitions.emplace_back(std::move(p));
      });
}

void Instance_cache_builder::iterate_schemas(
    const Iterate_schema &info,
    const std::function<void(const std::string &, Instance_cache::Schema *,
                             const mysqlshdk::db::IRow *)> &callback) {
  Profiler profiler{"iterating schemas"};

  const auto result = query(QH::build_query(info, schema_filter(info)));

  std::string current_schema;
  Instance_cache::Schema *schema = nullptr;

  while (const auto row = result->fetch_one()) {
    {
      auto schema_name = row->get_string(0);

      if (schema_name != current_schema) {
        current_schema = std::move(schema_name);

        const auto it = m_cache.schemas.find(current_schema);

        if (it != m_cache.schemas.end()) {
          schema = &it->second;
        } else {
          schema = nullptr;
        }
      }
    }

    if (schema) {
      callback(current_schema, schema, row);
    }
  }
}

void Instance_cache_builder::iterate_tables_and_views(
    const Iterate_table &info,
    const std::function<void(const std::string &, const std::string &,
                             Instance_cache::Table *,
                             const mysqlshdk::db::IRow *)> &table_callback,
    const std::function<void(const std::string &, const std::string &,
                             Instance_cache::View *,
                             const mysqlshdk::db::IRow *)> &view_callback) {
  Profiler profiler{"iterating tables and views"};

  const auto result =
      query(QH::build_query(info, schema_and_table_filter(info)));

  std::string current_schema;
  Instance_cache::Schema *schema = nullptr;

  std::string current_object;
  Instance_cache::Table *table = nullptr;
  Instance_cache::View *view = nullptr;

  while (const auto row = result->fetch_one()) {
    {
      auto schema_name = row->get_string(0);

      if (schema_name != current_schema) {
        current_schema = std::move(schema_name);
        current_object.clear();

        const auto it = m_cache.schemas.find(current_schema);

        if (it != m_cache.schemas.end()) {
          schema = &it->second;
        } else {
          schema = nullptr;
          table = nullptr;
          view = nullptr;
        }
      }
    }

    if (schema) {
      auto object_name = row->get_string(1);

      if (object_name != current_object) {
        current_object = std::move(object_name);

        const auto table_it = schema->tables.find(current_object);

        if (table_it != schema->tables.end()) {
          table = &table_it->second;
          view = nullptr;
        } else {
          table = nullptr;

          const auto view_it = schema->views.find(current_object);

          if (view_it != schema->views.end()) {
            view = &view_it->second;
          } else {
            view = nullptr;
          }
        }
      }
    }

    if (table && table_callback) {
      table_callback(current_schema, current_object, table, row);
    }

    if (view && view_callback) {
      view_callback(current_schema, current_object, view, row);
    }
  }
}

void Instance_cache_builder::iterate_tables(
    const Iterate_table &info,
    const std::function<void(const std::string &, const std::string &,
                             Instance_cache::Table *,
                             const mysqlshdk::db::IRow *)> &callback) {
  Profiler profiler{"iterating tables"};

  iterate_tables_and_views(info, callback, {});
}

void Instance_cache_builder::iterate_views(
    const Iterate_table &info,
    const std::function<void(const std::string &, const std::string &,
                             Instance_cache::View *,
                             const mysqlshdk::db::IRow *)> &callback) {
  Profiler profiler{"iterating views"};

  iterate_tables_and_views(info, {}, callback);
}

void Instance_cache_builder::set_schema_filter() {
  m_schema_filter = "";

  const auto &included = m_filters.schemas().included();
  const auto &excluded = m_filters.schemas().excluded();

  if (!included.empty()) {
    m_schema_filter +=
        QH::case_sensitive_compare(k_schema_template, included, true);
  }

  if (!excluded.empty()) {
    if (!included.empty()) {
      m_schema_filter += " AND ";
    }

    m_schema_filter +=
        QH::case_sensitive_compare(k_schema_template, excluded, false);
  }
}

void Instance_cache_builder::set_table_filter() {
  m_table_filter = "";

  const auto &included = m_filters.tables().included();
  const auto &excluded = m_filters.tables().excluded();

  if (!included.empty()) {
    m_table_filter += "(" +
                      QH::build_case_sensitive_filter(
                          included, k_schema_template, k_table_template) +
                      ")";
  }

  for (const auto &schema : excluded) {
    if (!m_table_filter.empty()) {
      m_table_filter += " AND ";
    }

    m_table_filter +=
        "NOT " + QH::case_sensitive_match(schema.first, schema.second,
                                          k_schema_template, k_table_template);
  }
}

std::string Instance_cache_builder::schema_filter(
    const std::string &schema_column) const {
  return shcore::str_subvars(
      m_schema_filter,
      [&schema_column](std::string_view) { return schema_column; },
      k_template_marker, k_template_marker);
}

std::string Instance_cache_builder::schema_filter(
    const Iterate_schema &info) const {
  return schema_filter(info.schema_column);
}

std::string Instance_cache_builder::table_filter(
    const std::string &schema_column, const std::string &table_column) const {
  return shcore::str_subvars(
      m_table_filter,
      [&schema_column, &table_column](std::string_view var) {
        if (var == k_schema_var) return schema_column;
        if (var == k_table_var) return table_column;
        throw std::logic_error("Unknown variable: " + std::string{var});
      },
      k_template_marker, k_template_marker);
}

std::string Instance_cache_builder::schema_and_table_filter(
    const Iterate_table &info) const {
  auto result = schema_filter(info.schema_column);
  auto filter = table_filter(info.schema_column, info.table_column);

  if (!result.empty() && !filter.empty()) {
    result += " AND ";
  }

  result += filter;

  return result;
}

std::string Instance_cache_builder::object_filter(
    const Iterate_schema &info, const Object_filters &included,
    const Object_filters &excluded) const {
  std::string filter;
  const auto &object_column = info.extra_columns[0];

  if (!included.empty()) {
    filter += "(" +
              QH::build_filter(included, info.schema_column, object_column) +
              ")";
  }

  for (const auto &schema : excluded) {
    if (!filter.empty()) {
      filter += " AND ";
    }

    filter += "NOT " + QH::match(schema.first, schema.second,
                                 info.schema_column, object_column);
  }

  return filter;
}

std::string Instance_cache_builder::trigger_filter(
    const Iterate_table &info, const Trigger_filters &included,
    const Trigger_filters &excluded) const {
  const auto filter_triggers = [&info](const std::string &schema,
                                       const Object_filters &tables) {
    const auto &trigger_column = info.extra_columns[0];
    const auto schema_filter =
        QH::case_sensitive_compare(info.schema_column, schema) + "=0";
    std::string filter;

    for (const auto &table : tables) {
      filter += "(" + schema_filter + " AND " +
                QH::case_sensitive_compare(info.table_column, table.first) +
                "=0";

      if (!table.second.empty()) {
        // only the specified triggers are used
        filter += " AND" + QH::case_sensitive_compare(trigger_column,
                                                      table.second, true);
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

uint64_t Instance_cache_builder::count(const std::string &table,
                                       const std::string &where,
                                       const std::string &column) const {
  std::string sql =
      "SELECT COUNT(" + column + ") FROM information_schema." + table;

  if (!where.empty()) {
    sql += " WHERE " + where;
  }

  return query(sql)->fetch_one()->get_uint(0);
}

uint64_t Instance_cache_builder::count(const Iterate_schema &info,
                                       const std::string &where) const {
  auto filter = schema_filter(info);

  if (!filter.empty() && !where.empty()) {
    filter += " AND ";
  }

  filter += where;

  return count(info.table_name, filter);
}

uint64_t Instance_cache_builder::count(const Iterate_table &info,
                                       const std::string &where) const {
  auto filter = schema_and_table_filter(info);

  if (!filter.empty() && !where.empty()) {
    filter += " AND ";
  }

  filter += where;

  return count(info.table_name, filter);
}

}  // namespace dump
}  // namespace mysqlsh
