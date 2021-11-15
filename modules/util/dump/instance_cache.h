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

#ifndef MODULES_UTIL_DUMP_INSTANCE_CACHE_H_
#define MODULES_UTIL_DUMP_INSTANCE_CACHE_H_

#include <functional>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "mysqlshdk/libs/db/session.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/version.h"

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

    inline bool valid() const noexcept { return !m_columns.empty(); }

    inline const std::vector<Column *> &columns() const { return m_columns; }

    inline const std::string &columns_sql() const { return m_columns_sql; }

    inline bool primary() const { return m_primary; }

    void reset();

    void add_column(Column *column);

    void set_primary(bool primary);

   private:
    std::vector<Column *> m_columns;

    std::string m_columns_sql;

    bool m_primary = false;
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
    std::string create_options;
    std::string comment;
    Index index;
    std::vector<Column *> columns;
    std::vector<Column> all_columns;
    std::vector<Histogram> histograms;
    std::vector<std::string> triggers;  // order of triggers is important
    std::vector<Partition> partitions;
  };

  struct View {
    std::string character_set_client;
    std::string collation_connection;
    std::vector<std::string> all_columns;
  };

  struct Schema {
    std::string collation;
    std::unordered_map<std::string, Table> tables;
    std::unordered_map<std::string, View> views;
    std::unordered_set<std::string> events;
    std::unordered_set<std::string> functions;
    std::unordered_set<std::string> procedures;
  };

  struct Stats {
    uint64_t schemas = 0;
    uint64_t tables = 0;
    uint64_t views = 0;
    uint64_t events = 0;
    uint64_t routines = 0;
    uint64_t triggers = 0;
    uint64_t users = 0;
  };

  bool has_ndbinfo = false;
  std::string user;
  std::string hostname;
  std::string server;
  mysqlshdk::utils::Version server_version;
  bool server_is_5_6 = false;
  bool server_is_5_7 = false;
  bool server_is_8_0 = false;
  uint32_t explain_rows_idx = 0;
  std::string binlog_file;
  uint64_t binlog_position;
  std::string gtid_executed;
  std::unordered_map<std::string, Schema> schemas;
  std::vector<shcore::Account> users;
  std::vector<shcore::Account> roles;
  Stats total;
  Stats filtered;
};

class Instance_cache_builder final {
 public:
  using Users = std::vector<shcore::Account>;

  using Filter = std::unordered_set<std::string>;
  using Object_filters = std::unordered_map<std::string, Filter>;
  using Trigger_filters = std::unordered_map<std::string, Object_filters>;

  Instance_cache_builder() = delete;

  Instance_cache_builder(
      const std::shared_ptr<mysqlshdk::db::ISession> &session,
      const Filter &included_schemas, const Object_filters &included_tables,
      const Filter &excluded_schemas, const Object_filters &excluded_tables,
      Instance_cache &&cache = {}, bool include_metadata = true);

  Instance_cache_builder(const Instance_cache_builder &) = delete;
  Instance_cache_builder(Instance_cache_builder &&) = default;

  ~Instance_cache_builder() = default;

  Instance_cache_builder &operator=(const Instance_cache_builder &) = delete;
  Instance_cache_builder &operator=(Instance_cache_builder &&) = default;

  Instance_cache_builder &users(const Users &included, const Users &excluded);

  Instance_cache_builder &events(const Object_filters &included,
                                 const Object_filters &excluded);

  Instance_cache_builder &routines(const Object_filters &included,
                                   const Object_filters &excluded);

  Instance_cache_builder &triggers(const Trigger_filters &included,
                                   const Trigger_filters &excluded);

  Instance_cache_builder &binlog_info();

  Instance_cache build();

 private:
  struct Iterate_schema;

  struct Iterate_table;

  struct Query_helper;

  using QH = Query_helper;

  struct Object {
    std::string schema;
    std::string name;
  };

  void filter_schemas();

  void filter_tables();

  void fetch_metadata();

  void fetch_version();

  void fetch_explain_select_rows_index();

  void fetch_server_metadata();

  void fetch_ndbinfo();

  void fetch_view_metadata();

  void fetch_columns();

  void fetch_table_indexes();

  void fetch_table_histograms();

  void fetch_table_partitions();

  void iterate_schemas(
      const Iterate_schema &info,
      const std::function<void(const std::string &, Instance_cache::Schema *,
                               const mysqlshdk::db::IRow *)> &callback);

  void iterate_tables(
      const Iterate_table &info,
      const std::function<void(const std::string &, const std::string &,
                               Instance_cache::Table *,
                               const mysqlshdk::db::IRow *)> &callback);

  void iterate_views(
      const Iterate_table &info,
      const std::function<void(const std::string &, const std::string &,
                               Instance_cache::View *,
                               const mysqlshdk::db::IRow *)> &callback);

  void iterate_tables_and_views(
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

  void set_schema_filter(const Filter &included, const Filter &excluded);

  void set_table_filter(const Object_filters &included,
                        const Object_filters &excluded);

  std::string schema_filter(const std::string &schema_column) const;

  std::string schema_filter(const Iterate_schema &info) const;

  std::string table_filter(const std::string &schema_column,
                           const std::string &table_column) const;

  std::string schema_and_table_filter(const Iterate_table &info) const;

  std::string object_filter(const Iterate_schema &info,
                            const Object_filters &included,
                            const Object_filters &excluded) const;

  std::string trigger_filter(const Iterate_table &info,
                             const Trigger_filters &included,
                             const Trigger_filters &excluded) const;

  template <typename T>
  inline std::shared_ptr<mysqlshdk::db::IResult> query(const T &sql) const {
    return m_session->query_log_error(sql);
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

  std::shared_ptr<mysqlshdk::db::ISession> m_session;

  Instance_cache m_cache;

  std::string m_schema_filter;

  std::string m_table_filter;

  bool m_has_tables = false;

  bool m_has_views = false;
};

}  // namespace dump
}  // namespace mysqlsh

#endif  // MODULES_UTIL_DUMP_INSTANCE_CACHE_H_
