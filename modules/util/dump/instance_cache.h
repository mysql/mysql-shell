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
  };

  struct Schema {
    std::string collation;
    std::unordered_map<std::string, Table> tables;
    std::unordered_map<std::string, View> views;
    std::unordered_set<std::string> events;
    std::unordered_set<std::string> functions;
    std::unordered_set<std::string> procedures;
  };

  bool has_ndbinfo = false;
  std::string user;
  std::string hostname;
  std::string server;
  mysqlshdk::utils::Version server_version;
  bool server_is_5_6 = false;
  bool server_is_5_7 = false;
  bool server_is_8_0 = false;
  std::string binlog_file;
  uint64_t binlog_position;
  std::string gtid_executed;
  std::unordered_map<std::string, Schema> schemas;
  std::vector<shcore::Account> users;
  std::vector<shcore::Account> roles;
};

class Instance_cache_builder final {
 public:
  using Users = std::vector<shcore::Account>;

  using Object_filters = std::set<std::string>;
  using Table_filters = std::unordered_map<std::string, Object_filters>;

  Instance_cache_builder() = delete;

  Instance_cache_builder(
      const std::shared_ptr<mysqlshdk::db::ISession> &session,
      const Object_filters &included_schemas,
      const Table_filters &included_tables,
      const Object_filters &excluded_schemas,
      const Table_filters &excluded_tables, bool include_metadata = true);

  Instance_cache_builder(
      const std::shared_ptr<mysqlshdk::db::ISession> &session,
      Instance_cache &&cache, bool include_metadata = true);

  Instance_cache_builder(const Instance_cache_builder &) = delete;
  Instance_cache_builder(Instance_cache_builder &&) = default;

  ~Instance_cache_builder() = default;

  Instance_cache_builder &operator=(const Instance_cache_builder &) = delete;
  Instance_cache_builder &operator=(Instance_cache_builder &&) = default;

  Instance_cache_builder &users(const Users &included, const Users &excluded);

  Instance_cache_builder &events();

  Instance_cache_builder &routines();

  Instance_cache_builder &triggers();

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

  using Schemas = std::vector<std::string>;
  using Tables = std::vector<Object>;

  void filter_schemas(const Object_filters &included,
                      const Object_filters &excluded);

  void filter_tables(const Table_filters &included,
                     const Table_filters &excluded);

  void fetch_metadata();

  void fetch_version();

  void fetch_server_metadata();

  void fetch_ndbinfo();

  void fetch_view_metadata();

  void fetch_table_columns();

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

  void set_schemas_list(Schemas &&schemas);

  bool has_tables() const { return !m_tables.empty(); }

  bool has_views() const { return !m_views.empty(); }

  void add_table(const std::string &schema, const std::string &table);

  void add_view(const std::string &schema, const std::string &view);

  std::shared_ptr<mysqlshdk::db::ISession> m_session;

  Instance_cache m_cache;

  Schemas m_schemas;
  Tables m_tables;
  Tables m_views;
};

}  // namespace dump
}  // namespace mysqlsh

#endif  // MODULES_UTIL_DUMP_INSTANCE_CACHE_H_
