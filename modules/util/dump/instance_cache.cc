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

#include "modules/util/dump/instance_cache.h"

#include <mysqld_error.h>

#include <algorithm>
#include <cctype>
#include <iterator>
#include <stdexcept>
#include <string_view>
#include <utility>

#include "mysqlshdk/include/shellcore/console.h"
#include "mysqlshdk/libs/db/mysql/result.h"
#include "mysqlshdk/libs/db/query_helper.h"
#include "mysqlshdk/libs/mysql/utils.h"
#include "mysqlshdk/libs/utils/debug.h"
#include "mysqlshdk/libs/utils/logger.h"
#include "mysqlshdk/libs/utils/profiling.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_net.h"
#include "mysqlshdk/libs/utils/utils_sqlstring.h"
#include "mysqlshdk/libs/utils/utils_string.h"

#include "modules/util/dump/dump_errors.h"
#include "modules/util/dump/schema_dumper.h"

namespace mysqlsh {
namespace dump {

namespace {

constexpr std::string_view k_procedure_type = "PROCEDURE";

bool has_vector_store_comment(std::string_view comment) {
  static constexpr std::string_view k_genai_options = "GENAI_OPTIONS=";
  static constexpr std::string_view k_embed_model_id = "EMBED_MODEL_ID=";

  // search for options
  auto start = comment.find(k_genai_options);

  if (std::string_view::npos == start) {
    // not found
    return false;
  }

  // move to the beginning of options
  start += k_genai_options.length();
  // get the options string
  const auto options = comment.substr(start, comment.find(" ", start));
  // find the model
  const auto model = options.find(k_embed_model_id);

  if (std::string_view::npos == model) {
    // not found
    return false;
  }

  // ensure that this is an exact match
  return 0 == model ||
         !std::isalpha(static_cast<unsigned char>(options[model - 1]));
}

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

[[maybe_unused]] void use_unsupported_collation(std::string *collation) {
  if ("utf8mb4_pl_0900_as_cs" == *collation ||
      "utf8mb4_0900_ai_ci" == *collation) {
    *collation = "utf8mb4_uca1400_polish_nopad_ai_cs";
  }
}

}  // namespace

void Instance_cache::Index::add_column(const Column *column) {
  m_columns.emplace_back(column);

  if (!m_columns_sql.empty()) {
    m_columns_sql += ',';
  }

  m_columns_sql += column->quoted_name;
}

Instance_cache_builder::Instance_cache_builder(
    const std::shared_ptr<mysqlshdk::db::ISession> &session,
    const mysqlshdk::db::Filtering_options &filters, Instance_cache &&cache)
    : m_session(session),
      m_cache(std::move(cache)),
      m_query_helper(filters),
      m_filters(filters) {
  m_query_helper.set_schema_filter();
  m_query_helper.set_table_filter();

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
  sd.use_filters(&m_filters);

  m_cache.users = sd.get_users();
  m_cache.roles = sd.get_roles();

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
  info.where = m_query_helper.event_filter(info);

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
  info.where = m_query_helper.routine_filter(info);

  iterate_schemas(info,
                  [this](const std::string &, Instance_cache::Schema *schema,
                         const mysqlshdk::db::IRow *row) {
                    auto &target = row->get_string(2) == k_procedure_type
                                       ? schema->procedures
                                       : schema->functions;  // ROUTINE_TYPE

                    target.emplace(row->get_string(1),
                                   Instance_cache::Routine{});  // ROUTINE_NAME

                    ++m_cache.filtered.routines;
                  });

  // the total number of routines within the filtered schemas
  m_cache.total.routines = count(info);

  if (compatibility::supports_library_ddl(m_cache.server.version.number)) {
    // the routine_libraries view has ROUTINE_SCHEMA, ROUTINE_NAME and
    // ROUTINE_TYPE columns, so these do not need to be changed, routine filter
    // is valid as well
    info.extra_columns.emplace_back(
        "IFNULL(l.library_schema, rl.library_schema)");  // NOT NULL
    info.extra_columns.emplace_back(
        "IFNULL(l.library_name, rl.library_name)");  // NOT NULL
    info.extra_columns.emplace_back(
        "l.library_schema IS NOT NULL");  // NOT NULL

    // we left join the routine_libraries and libraries views to get the correct
    // casing of the library name and detect missing libraries
    info.table_name =
        "routine_libraries AS rl "
        "LEFT JOIN information_schema.libraries AS l "
        "ON rl.library_schema=l.library_schema "
        "AND rl.library_name=l.library_name";

    auto has_library_ddl = m_cache.has_library_ddl;

    iterate_schemas(info, [&has_library_ddl](const std::string &,
                                             Instance_cache::Schema *schema,
                                             const mysqlshdk::db::IRow *row) {
      auto &target = row->get_string(2) == k_procedure_type
                         ? schema->procedures
                         : schema->functions;  // ROUTINE_TYPE

      auto &routine = target.at(row->get_string(1));  // ROUTINE_NAME
      routine.library_references.emplace_back(
          Instance_cache::Routine::Library_reference{
              row->get_string(3),      // LIBRARY_SCHEMA
              row->get_string(4),      // LIBRARY_NAME
              1 == row->get_int(5)});  // EXISTS

      has_library_ddl = true;
    });

    m_cache.has_library_ddl = has_library_ddl;
  }

  fetch_routine_parameters();

  return *this;
}

Instance_cache_builder &Instance_cache_builder::libraries() {
  if (!compatibility::supports_library_ddl(m_cache.server.version.number)) {
    return *this;
  }

  Profiler profiler{"fetching libraries"};

  Iterate_schema info;
  info.schema_column = "LIBRARY_SCHEMA";  // NOT NULL
  info.extra_columns = {
      "LIBRARY_NAME",  // NOT NULL
  };
  info.table_name = "libraries";
  // library names are case insensitive
  info.where = m_query_helper.library_filter(info);

  iterate_schemas(
      info, [this](const std::string &, Instance_cache::Schema *schema,
                   const mysqlshdk::db::IRow *row) {
        schema->libraries.emplace(row->get_string(1));  // LIBRARY_NAME

        ++m_cache.filtered.libraries;
      });

  // the total number of libraries within the filtered schemas
  m_cache.total.libraries = count(info);

  if (m_cache.filtered.libraries) {
    m_cache.has_library_ddl = true;
  }

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
    info.where = m_query_helper.trigger_filter(info);

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

  m_cache.server.binlog = common::binlog(m_session, m_cache.server.version);

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

  const auto filter = m_query_helper.schema_filter("SCHEMA_NAME");

  if (!filter.empty()) {
    sql += " WHERE " + filter;
  }

  {
    const auto result = query(sql);

    while (const auto row = result->fetch_one()) {
      const auto schema = row->get_string(0);  // SCHEMA_NAME

      m_cache.schemas[schema].default_collation =
          row->get_string(1);  // DEFAULT_COLLATION_NAME

      DBUG_EXECUTE_IF("dumper_unsupported_collation", {
        use_unsupported_collation(&m_cache.schemas[schema].default_collation);
      });
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
      table_column,       // NOT NULL
      "TABLE_TYPE",       // NOT NULL
      "TABLE_ROWS",       // can be NULL
      "AVG_ROW_LENGTH",   // can be NULL
      "ENGINE",           // can be NULL
      "CREATE_OPTIONS",   // can be NULL
      "TABLE_COMMENT",    // can be NULL in 8.0
      "TABLE_COLLATION",  // can be NULL
  };
  info.table_name = "tables";
  info.where = m_query_helper.table_filter(schema_column, table_column);

  iterate_schemas(info, [this](const std::string &,
                               Instance_cache::Schema *schema,
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
    target.collation = row->get_string(8, "");        // TABLE_COLLATION

    DBUG_EXECUTE_IF("dumper_unsupported_collation",
                    { use_unsupported_collation(&target.collation); });

    if (is_table) {
      set_has_tables();

      ++m_cache.filtered.tables;

      DBUG_EXECUTE_IF("dumper_average_row_length_0",
                      { target.average_row_length = 0; });
    } else {
      set_has_views();

      ++m_cache.filtered.views;
    }

    if (!target.create_options.empty()) {
      static constexpr std::string_view k_secondary_engine =
          "SECONDARY_ENGINE=\"";

      if (auto pos = target.create_options.find(k_secondary_engine);
          std::string::npos != pos) {
        pos += k_secondary_engine.length();

        if (const auto end = target.create_options.find('"', pos);
            std::string::npos != end) {
          target.secondary_engine = shcore::str_upper(
              std::string_view{target.create_options}.substr(pos, end - pos));
        }
      }
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

  m_cache.server.version = common::server_version(m_session);
}

void Instance_cache_builder::fetch_server_metadata() {
  Profiler profiler{"fetching server metadata"};

  m_cache.user = m_session->get_connection_options().get_user();
  m_cache.hostname = mysqlshdk::utils::Net::get_hostname();

  m_cache.server.sysvars = common::server_variables(m_session);

  if (2 == m_cache.server.sysvars.lower_case_table_names) {
    initialize_lowercase_names();
  }

  m_cache.server.topology = common::replication_topology(m_session);
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

  // BUG#36509026 - we're fetching view definitions to extract table references,
  // starting with 8.0.13 we get those from I_S.VIEW_TABLE_USAGE
  static const mysqlshdk::utils::Version k_has_view_table_usage{8, 0, 13};

  if (m_cache.server.version.number < k_has_view_table_usage) {
    info.extra_columns.emplace_back("VIEW_DEFINITION");  // can be NULL in 8.x
  }

  info.table_name = "views";

  iterate_views(info, [this](const std::string &schema, const std::string &,
                             Instance_cache::View *view,
                             const mysqlshdk::db::IRow *row) {
    view->character_set_client = row->get_string(2);  // CHARACTER_SET_CLIENT
    view->collation_connection = row->get_string(3);  // COLLATION_CONNECTION

    DBUG_EXECUTE_IF("dumper_unsupported_collation", {
      use_unsupported_collation(&view->collation_connection);
    });

    if (row->num_fields() > 4) {
      for (auto &ref : mysqlshdk::parser::extract_table_references(
               row->get_string(4, {}),  // VIEW_DEFINITION
               m_cache.server.version.number)) {
        if (ref.schema.empty()) {
          ref.schema = schema;
        }

        view->table_references.emplace(std::move(ref));
      }
    }
  });

  if (m_cache.server.version.number >= k_has_view_table_usage) {
    Iterate_table usage;
    usage.schema_column = "VIEW_SCHEMA";  // NOT NULL
    usage.table_column = "VIEW_NAME";     // NOT NULL
    usage.extra_columns = {
        "TABLE_SCHEMA",  // NOT NULL
        "TABLE_NAME"     // NOT NULL
    };
    usage.table_name = "view_table_usage";

    iterate_views(
        usage, [](const std::string &, const std::string &,
                  Instance_cache::View *view, const mysqlshdk::db::IRow *row) {
          view->table_references.emplace(mysqlshdk::parser::Table_reference{
              row->get_string(2),    // TABLE_SCHEMA
              row->get_string(3)});  // TABLE_NAME
        });
  }
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
      "COLUMN_COMMENT",    // NOT NULL
      "COLLATION_NAME",    // can be NULL
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
                        mysqlshdk::db::Type::Geometry == column.type ||
                        mysqlshdk::db::Type::Vector == column.type;
    const auto extra = row->get_string(7, "");  // EXTRA
    column.generated = extra.find(" GENERATED") != std::string::npos;
    column.auto_increment = extra.find("auto_increment") != std::string::npos;
    column.nullable = shcore::str_caseeq(row->get_string(5),
                                         "YES");  // IS_NULLABLE
    column.is_innodb_vector_store_column =
        mysqlshdk::db::Type::Vector == column.type &&
        has_vector_store_comment(row->get_string(8));  // COLUMN_COMMENT
    column.collation = row->get_string(9, "");         // COLLATION_NAME

    DBUG_EXECUTE_IF("dumper_unsupported_collation",
                    { use_unsupported_collation(&column.collation); });

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
      const auto vector_store_table =
          "InnoDB" == t.engine && "RAPID" == t.secondary_engine;

      t.all_columns.reserve(table.second.size());

      for (auto &column : table.second) {
        auto &c = column.second;

        if (vector_store_table && c.is_innodb_vector_store_column) {
          t.is_innodb_vector_store_table = true;
          ++m_cache.innodb_vector_store_tables;
        }

        t.all_columns.emplace_back(std::move(c));

        if (!t.all_columns.back().generated) {
          t.columns.emplace_back(&t.all_columns.back());
        }
      }
    }
  }

  m_cache.has_innodb_vector_store_tables = m_cache.innodb_vector_store_tables;

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
      "COLUMN_NAME",  // can be NULL
      "SEQ_IN_INDEX"  // NOT NULL
  };
  info.table_name = "statistics";
  info.where = "NON_UNIQUE=0";

  constexpr std::string_view k_primary_index = "PRIMARY";
  struct Index_info {
    std::vector<Instance_cache::Column *> columns;
  };
  // schema -> table -> index name -> index info
  // indexes are ordered to ensure repeatability of the selection algorithm
  std::unordered_map<
      std::string,
      std::unordered_map<std::string, std::map<std::string, Index_info>>>
      indexes;

  iterate_tables(info, [&indexes](const std::string &schema_name,
                                  const std::string &table_name,
                                  Instance_cache::Table *t,
                                  const mysqlshdk::db::IRow *row) {
    // INDEX_NAME can be NULL in 8.0, as per output of 'SHOW COLUMNS', but
    // it's not likely, as it's NOT NULL in definition of mysql.indexes
    // hidden table

    // COLUMN_NAME column is NULL if this is a functional key
    Instance_cache::Column *column_ptr = nullptr;

    if (!row->is_null(3)) {
      const auto column =
          std::find_if(t->all_columns.begin(), t->all_columns.end(),
                       [name = row->get_string(3)](const auto &c) {
                         return name == c.name;
                       });  // COLUMN_NAME

      // column will not be found if user is missing SELECT privilege and it
      // was not possible to fetch column information
      if (t->all_columns.end() != column) {
        column_ptr = &(*column);
      }
    }

    auto &index_info =
        indexes[schema_name][table_name][row->get_string(2, {})];  // INDEX_NAME
    const auto seq = row->get_uint(4);  // SEQ_IN_INDEX

    // SEQ_IN_INDEX is 1-based
    if (seq > index_info.columns.size()) {
      index_info.columns.resize(seq);
    }

    index_info.columns[seq - 1] = column_ptr;
  });

  for (const auto &schema : indexes) {
    for (const auto &table : schema.second) {
      auto &t = m_cache.schemas.at(schema.first).tables.at(table.first);

      for (const auto &index : table.second) {
        Instance_cache::Index new_index;
        bool nullable = false;
        bool add_index = true;

        for (const auto &column : index.second.columns) {
          if (!column) {
            // skip this index
            add_index = false;
            break;
          }

          new_index.add_column(column);
          nullable |= column->nullable;
        }

        if (add_index) {
          auto ptr = &t.indexes.emplace(index.first, std::move(new_index))
                          .first->second;

          if (k_primary_index == index.first) {
            t.primary_key = ptr;
          } else if (!nullable) {
            t.primary_key_equivalents.emplace_back(ptr);
          } else {
            t.unique_keys.emplace_back(ptr);
          }
        }
      }
    }
  }
}

void Instance_cache_builder::fetch_table_histograms() {
  Profiler profiler{"fetching table histograms"};

  if (!has_tables() || !m_cache.server.version.is_8_0) {
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

  const auto result = query(
      m_query_helper.build_query(info, m_query_helper.schema_filter(info)));

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

  const auto result = query(m_query_helper.build_query(
      info, m_query_helper.schema_and_table_filter(info)));

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
  auto filter = m_query_helper.schema_filter(info);

  if (!filter.empty() && !where.empty()) {
    filter += " AND ";
  }

  filter += where;

  return count(info.table_name, filter);
}

uint64_t Instance_cache_builder::count(const Iterate_table &info,
                                       const std::string &where) const {
  auto filter = m_query_helper.schema_and_table_filter(info);

  if (!filter.empty() && !where.empty()) {
    filter += " AND ";
  }

  filter += where;

  return count(info.table_name, filter);
}

void Instance_cache_builder::initialize_lowercase_names() {
  for (auto &schema : m_cache.schemas) {
    auto &s = schema.second;

    m_cache.schemas_lowercase.emplace(shcore::utf8_lower(schema.first), &s);

    for (auto &table : s.tables) {
      s.tables_lowercase.emplace(shcore::utf8_lower(table.first),
                                 &table.second);
    }

    for (auto &view : s.views) {
      s.views_lowercase.emplace(shcore::utf8_lower(view.first), &view.second);
    }
  }
}

void Instance_cache_builder::fetch_routine_parameters() {
  Profiler profiler{"fetching routine parameters"};

  Iterate_schema info;
  info.schema_column = "SPECIFIC_SCHEMA";  // NOT NULL
  info.extra_columns = {
      "SPECIFIC_NAME",     // NOT NULL
      "ROUTINE_TYPE",      // NOT NULL
      "ORDINAL_POSITION",  // NOT NULL
      "PARAMETER_NAME",    // can be NULL
      "COLLATION_NAME",    // can be NULL
  };
  info.table_name = "parameters";
  // routine names are case insensitive
  info.where = m_query_helper.routine_filter(info);

  iterate_schemas(info, [](const std::string &, Instance_cache::Schema *schema,
                           const mysqlshdk::db::IRow *row) {
    auto &target = row->get_string(2) == k_procedure_type
                       ? schema->procedures
                       : schema->functions;         // ROUTINE_TYPE
    auto &routine = target.at(row->get_string(1));  // ROUTINE_NAME
    const auto position = row->get_uint(3);         // ORDINAL_POSITION
    Instance_cache::Parameter parameter{
        row->get_string(4, ""),  // PARAMETER_NAME
        row->get_string(5, ""),  // COLLATION_NAME
    };

    DBUG_EXECUTE_IF("dumper_unsupported_collation",
                    { use_unsupported_collation(&parameter.collation); });

    // if ORDINAL_POSITION is 0, this is a return value
    if (0 == position) {
      routine.return_value = std::move(parameter);
    } else {
      if (routine.parameters.size() < position) {
        routine.parameters.resize(position);
      }

      routine.parameters[position - 1] = std::move(parameter);
    }
  });
}

}  // namespace dump
}  // namespace mysqlsh
