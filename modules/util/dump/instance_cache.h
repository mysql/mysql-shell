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
    bool csv_unsafe = false;
  };

  struct Index {
    inline bool valid() const noexcept { return !columns.empty(); }

    inline const std::string &first_column() const {
      if (!valid()) {
        throw std::logic_error("Trying to access invalid index");
      }

      return columns.front();
    }

    std::string order_by() const;

    std::vector<std::string> columns;
    bool primary = false;
  };

  struct Histogram {
    std::string column;
    std::size_t buckets = 0;
  };

  struct Table {
    uint64_t row_count = 0;
    uint64_t average_row_length = 0;
    std::string engine;
    std::string create_options;
    std::string comment;
    Index index;
    std::vector<Column> columns;
    std::vector<Histogram> histograms;
    std::vector<std::string> triggers;  // order of triggers is important
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
  std::string gtid_executed;
  std::unordered_map<std::string, Schema> schemas;
  std::vector<shcore::Account> users;
};

class Instance_cache_builder final {
 public:
  using Objects = std::set<std::string>;
  using Schema_objects = std::unordered_map<std::string, Objects>;
  using Users = std::vector<shcore::Account>;

  Instance_cache_builder() = delete;

  Instance_cache_builder(
      const std::shared_ptr<mysqlshdk::db::ISession> &session,
      const Objects &included_schemas, const Schema_objects &included_tables,
      const Objects &excluded_schemas, const Schema_objects &excluded_tables,
      bool include_metadata = true);

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

  Instance_cache build();

 private:
  struct Iterate_schema;

  struct Iterate_table;

  void filter_schemas(const Objects &included, const Objects &excluded);

  void filter_tables(const Schema_objects &included,
                     const Schema_objects &excluded);

  void fetch_metadata();

  void fetch_version();

  void fetch_server_metadata();

  void fetch_ndbinfo();

  void fetch_view_metadata();

  void fetch_table_columns();

  void fetch_table_indexes();

  void fetch_table_histograms();

  void iterate_schemas(
      const Iterate_schema &info,
      const std::function<void(const std::string &, Instance_cache::Schema *,
                               const mysqlshdk::db::IRow *)> &callback);

  std::string build_query(const Iterate_schema &info);

  void iterate_tables(
      const Iterate_table &info,
      const std::function<void(const std::string &, Instance_cache::Table *,
                               const mysqlshdk::db::IRow *)> &callback);

  void iterate_views(
      const Iterate_table &info,
      const std::function<void(const std::string &, Instance_cache::View *,
                               const mysqlshdk::db::IRow *)> &callback);

  std::string build_query(const Iterate_table &info,
                          const std::function<Instance_cache_builder::Objects(
                              const Instance_cache::Schema &)> &list);

  void set_schemas_list(Objects &&schemas);

  std::shared_ptr<mysqlshdk::db::ISession> m_session;

  Instance_cache m_cache;

  Objects m_schemas;

  bool m_has_tables = false;

  bool m_has_views = false;
};

}  // namespace dump
}  // namespace mysqlsh

#endif  // MODULES_UTIL_DUMP_INSTANCE_CACHE_H_
