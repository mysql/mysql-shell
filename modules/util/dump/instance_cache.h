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

#ifndef MODULES_UTIL_DUMP_INSTANCE_CACHE_H_
#define MODULES_UTIL_DUMP_INSTANCE_CACHE_H_

#include <cinttypes>
#include <functional>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "modules/util/common/dump/server_info.h"
#include "mysqlshdk/libs/db/column.h"
#include "mysqlshdk/libs/db/filtering_options.h"
#include "mysqlshdk/libs/db/query_helper.h"
#include "mysqlshdk/libs/db/result.h"
#include "mysqlshdk/libs/db/session.h"
#include "mysqlshdk/libs/parser/mysql_parser_utils.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/version.h"

namespace mysqlshdk::db {
class SHCORE_PUBLIC IRow;
}

namespace mysqlsh {
namespace dump {

struct Instance_cache {
  struct Column {
    std::string name;
    std::string quoted_name;
    bool csv_unsafe = false;
    bool generated = false;
    bool auto_increment = false;
    bool nullable = true;
    mysqlshdk::db::Type type = mysqlshdk::db::Type::Null;
  };

  class Index final {
   public:
    Index() = default;

    Index(const Index &) = default;
    Index(Index &&) = default;

    Index &operator=(const Index &) = default;
    Index &operator=(Index &&) = default;

    ~Index() = default;

    inline const std::vector<const Column *> &columns() const {
      return m_columns;
    }

    inline const std::string &columns_sql() const { return m_columns_sql; }

    void add_column(const Column *column);

   private:
    std::vector<const Column *> m_columns;

    std::string m_columns_sql;
  };

  struct Histogram {
    std::string column;
    std::size_t buckets = 0;
  };

  struct Partition {
    std::string name;
    std::string quoted_name;
    uint64_t row_count = 0;
    uint64_t average_row_length = 0;
  };

  struct Table {
    uint64_t row_count = 0;
    uint64_t average_row_length = 0;
    std::string engine;
    // upper case name of the secondary engine
    std::string secondary_engine;
    std::string create_options;
    std::string comment;
    std::unordered_map<std::string, Index> indexes;
    const Index *primary_key = nullptr;
    // indexes are ordered to ensure repeatability of the selection algorithm
    std::vector<const Index *> primary_key_equivalents;
    std::vector<const Index *> unique_keys;
    std::vector<const Column *> columns;
    std::vector<Column> all_columns;
    std::vector<Histogram> histograms;
    std::vector<std::string> triggers;  // order of triggers is important
    std::vector<Partition> partitions;
  };

  struct View : public Table {
    std::string character_set_client;
    std::string collation_connection;
    std::set<mysqlshdk::parser::Table_reference> table_references;
  };

  struct Routine {
    struct Library_reference {
      std::string schema;
      std::string library;
      bool exists;
    };

    std::vector<Library_reference> library_references;
  };

  struct Schema {
    std::string collation;
    std::unordered_map<std::string, Table> tables;
    std::unordered_map<std::string, Table *> tables_lowercase;
    std::unordered_map<std::string, View> views;
    std::unordered_map<std::string, View *> views_lowercase;
    std::unordered_set<std::string> events;
    std::unordered_map<std::string, Routine> functions;
    std::unordered_map<std::string, Routine> procedures;
    std::unordered_set<std::string> libraries;
  };

  struct Stats {
    uint64_t schemas = 0;
    uint64_t tables = 0;
    uint64_t views = 0;
    uint64_t events = 0;
    uint64_t routines = 0;
    uint64_t libraries = 0;
    uint64_t triggers = 0;
    uint64_t users = 0;
  };

  bool has_ndbinfo = false;
  bool has_library_ddl = false;
  std::string user;
  std::string hostname;
  common::Server_info server;

  std::unordered_map<std::string, Schema> schemas;
  std::unordered_map<std::string, Schema *> schemas_lowercase;
  std::vector<shcore::Account> users;
  std::vector<shcore::Account> roles;

  Stats total;
  Stats filtered;
};

class Instance_cache_builder final {
  using Filtering_options = mysqlshdk::db::Filtering_options;

 public:
  using Partition_filters = std::unordered_map<
      std::string,
      std::unordered_map<std::string, std::unordered_set<std::string>>>;

  Instance_cache_builder() = delete;

  Instance_cache_builder(
      const std::shared_ptr<mysqlshdk::db::ISession> &session,
      const Filtering_options &filters, Instance_cache &&cache = {});

  Instance_cache_builder(const Instance_cache_builder &) = delete;
  Instance_cache_builder(Instance_cache_builder &&) = default;

  ~Instance_cache_builder() = default;

  Instance_cache_builder &operator=(const Instance_cache_builder &) = delete;
  Instance_cache_builder &operator=(Instance_cache_builder &&) = delete;

  Instance_cache_builder &metadata(const Partition_filters &partitions);

  Instance_cache_builder &users();

  Instance_cache_builder &events();

  Instance_cache_builder &routines();

  Instance_cache_builder &libraries();

  Instance_cache_builder &triggers();

  Instance_cache_builder &binlog_info();

  Instance_cache build();

 private:
  using Object_filters = Filtering_options::Object_filters::Filter;
  using Trigger_filters = Filtering_options::Trigger_filters::Filter;

  using Iterate_schema = mysqlshdk::db::Iterate_schema;
  using Iterate_table = mysqlshdk::db::Iterate_table;

  void filter_schemas();

  void filter_tables();

  void fetch_metadata(const Partition_filters &partitions);

  void fetch_version();

  void fetch_server_metadata();

  void fetch_ndbinfo();

  void fetch_view_metadata();

  void fetch_columns();

  void fetch_table_indexes();

  void fetch_table_histograms();

  void fetch_table_partitions(const Partition_filters &partitions);

  void iterate_schemas(
      const Iterate_schema &info,
      const std::function<void(const std::string &, Instance_cache::Schema *,
                               const mysqlshdk::db::IRow *)> &callback);

  std::vector<std::unique_ptr<mysqlshdk::db::Warning>> iterate_tables(
      const Iterate_table &info,
      const std::function<void(const std::string &, const std::string &,
                               Instance_cache::Table *,
                               const mysqlshdk::db::IRow *)> &callback);

  std::vector<std::unique_ptr<mysqlshdk::db::Warning>> iterate_views(
      const Iterate_table &info,
      const std::function<void(const std::string &, const std::string &,
                               Instance_cache::View *,
                               const mysqlshdk::db::IRow *)> &callback);

  std::vector<std::unique_ptr<mysqlshdk::db::Warning>> iterate_tables_and_views(
      const Iterate_table &info,
      const std::function<void(const std::string &, const std::string &,
                               Instance_cache::Table *,
                               const mysqlshdk::db::IRow *)> &table_callback,
      const std::function<void(const std::string &, const std::string &,
                               Instance_cache::View *,
                               const mysqlshdk::db::IRow *)> &view_callback);

  inline bool has_tables() const { return m_has_tables; }

  inline void set_has_tables() { m_has_tables = true; }

  inline bool has_views() const { return m_has_views; }

  inline void set_has_views() { m_has_views = true; }

  inline std::shared_ptr<mysqlshdk::db::IResult> query(
      std::string_view sql) const {
    return m_session->query(sql);
  }

  /**
   * Counts the number of rows in the given information_schema table, optionally
   * using a condition.
   *
   * @param table Name of the information_schema table.
   * @param where Optional condition.
   * @param column Optional column to use to do the counting.
   *
   * @returns The number of rows.
   */
  uint64_t count(const std::string &table, const std::string &where = {},
                 const std::string &column = "*") const;

  /**
   * Counts the number of rows in the given information_schema table matching
   * the schema filter, optionally using an additional condition.
   *
   * @param info Information about the information_schema table.
   * @param where Optional condition.
   *
   * @returns The number of rows.
   */
  uint64_t count(const Iterate_schema &info,
                 const std::string &where = {}) const;

  /**
   * Counts the number of rows in the given information_schema table matching
   * the table filter, optionally using an additional condition.
   *
   * @param info Information about the information_schema table.
   * @param where Optional condition.
   *
   * @returns The number of rows.
   */
  uint64_t count(const Iterate_table &info,
                 const std::string &where = {}) const;

  void initialize_lowercase_names();

  std::shared_ptr<mysqlshdk::db::ISession> m_session;

  Instance_cache m_cache;

  mysqlshdk::db::Query_helper m_query_helper;

  const Filtering_options &m_filters;

  std::string m_schema_filter;

  std::string m_table_filter;

  bool m_has_tables = false;

  bool m_has_views = false;
};

}  // namespace dump
}  // namespace mysqlsh

#endif  // MODULES_UTIL_DUMP_INSTANCE_CACHE_H_
