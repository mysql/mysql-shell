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

#ifndef MODULES_UTIL_DUMP_DUMP_OPTIONS_H_
#define MODULES_UTIL_DUMP_DUMP_OPTIONS_H_

#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>

#include "mysqlshdk/include/scripting/types.h"
#include "mysqlshdk/include/scripting/types_cpp.h"
#include "mysqlshdk/libs/db/filtering_options.h"
#include "mysqlshdk/libs/storage/compressed_file.h"
#include "mysqlshdk/libs/utils/version.h"

#include "modules/util/common/common_options.h"
#include "modules/util/dump/compatibility_option.h"
#include "modules/util/dump/instance_cache.h"
#include "modules/util/import_table/dialect.h"

namespace mysqlsh {
namespace dump {

enum class Dry_run {
  DISABLED,
  WRITE_EMPTY_DATA_FILES,
  DONT_WRITE_ANY_FILES,
};

class Dump_options : public mysqlsh::common::Common_options {
 public:
  using Filtering_options = mysqlshdk::db::Filtering_options;

  Dump_options(const Dump_options &) = default;
  Dump_options(Dump_options &&) = default;

  Dump_options &operator=(const Dump_options &) = default;
  Dump_options &operator=(Dump_options &&) = default;

  ~Dump_options() override = default;

  static const shcore::Option_pack_def<Dump_options> &options();

  static const mysqlshdk::utils::Version &current_version();

  // setters
  void set_output_url(const std::string &url);

  void set_compression(mysqlshdk::storage::Compression compression) {
    m_compression = compression;
  }

  void set_dry_run_mode(Dry_run dry_run) { m_dry_run_mode = dry_run; }

  void disable_index_files() { m_write_index_files = false; }

  void dont_rename_data_files() { m_rename_data_files = false; }

  // getters
  const std::string &output_url() const { return m_output_url; }

  const shcore::Dictionary_t &original_options() const { return m_options; }

  bool use_base64() const { return m_use_base64; }

  int64_t max_rate() const { return m_max_rate; }

  mysqlshdk::storage::Compression compression() const { return m_compression; }

  const mysqlshdk::storage::Compression_options &compression_options() const {
    return m_compression_options;
  }

  const import_table::Dialect &dialect() const { return m_dialect; }

  const std::string &character_set() const { return m_character_set; }

  bool mds_compatibility() const { return m_is_mds; }

  const Compatibility_options &compatibility_options() const {
    return m_compatibility_options;
  }

  Filtering_options &filters() { return m_filtering_options; }
  const Filtering_options &filters() const { return m_filtering_options; }

  const mysqlshdk::utils::Version &target_version() const {
    if (m_target_version.has_value()) {
      return *m_target_version;
    } else {
      return current_version();
    }
  }

  bool implicit_target_version() const { return !m_target_version.has_value(); }

  const Instance_cache_builder::Partition_filters &included_partitions() const {
    return m_partitions;
  }

  const std::string &where(const std::string &schema,
                           const std::string &table) const;

  Dry_run dry_run_mode() const { return m_dry_run_mode; }

  bool is_dry_run() const { return Dry_run::DISABLED != m_dry_run_mode; }

  bool write_index_files() const { return m_write_index_files; }

  bool rename_data_files() const { return m_rename_data_files; }

  virtual bool split() const = 0;

  virtual uint64_t bytes_per_chunk() const = 0;

  virtual std::size_t threads() const = 0;

  virtual std::size_t worker_threads() const { return threads(); }

  virtual bool is_export_only() const = 0;

  virtual bool use_single_file() const = 0;

  virtual bool dump_ddl() const = 0;

  virtual bool dump_data() const = 0;

  virtual bool consistent_dump() const = 0;

  virtual bool skip_consistency_checks() const = 0;

  virtual bool skip_upgrade_checks() const = 0;

  virtual bool dump_events() const = 0;

  virtual bool dump_routines() const = 0;

  virtual bool dump_triggers() const = 0;

  virtual bool dump_users() const = 0;

  virtual bool use_timezone_utc() const = 0;

  virtual bool dump_binlog_info() const = 0;

  virtual bool checksum() const = 0;

 protected:
  explicit Dump_options(const char *name);

  virtual void on_set_output_url(const std::string &url);

  void on_validate() const override;

  void enable_mds_compatibility() { m_is_mds = true; }

  void set_compatibility_option(Compatibility_option c) {
    m_compatibility_options |= c;
  }

  void set_target_version(const mysqlshdk::utils::Version &version,
                          bool validate = true);

  void set_where_clause(const std::map<std::string, std::string> &where);

  void set_where_clause(const std::string &schema, const std::string &table,
                        const std::string &where);

  void set_partitions(
      const std::map<std::string, std::unordered_set<std::string>> &partitions);

  void set_partitions(const std::string &schema, const std::string &table,
                      const std::unordered_set<std::string> &partitions);

  bool exists(const std::string &schema) const;

  bool exists(const std::string &schema, const std::string &table) const;

  std::set<std::string> find_missing(
      const std::unordered_set<std::string> &schemas) const;

  std::set<std::string> find_missing(
      const std::string &schema,
      const std::unordered_set<std::string> &tables) const;

  // currently used by dumpTables(), dumpSchemas() and dumpInstance()
  Filtering_options m_filtering_options;

  mutable bool m_filter_conflicts = false;

 protected:
  void on_start_unpack(const shcore::Dictionary_t &options);

 private:
  void on_unpacked_options();

  void set_string_option(const std::string &option, const std::string &value);

  std::set<std::string> find_missing_impl(
      const std::string &subquery,
      const std::unordered_set<std::string> &objects) const;

  void validate_partitions() const;

  // input arguments
  std::string m_output_url;
  shcore::Dictionary_t m_options;

  // not configurable
  bool m_use_base64 = true;

  // common options
  int64_t m_max_rate = 0;
  mysqlshdk::storage::Compression m_compression =
      mysqlshdk::storage::Compression::ZSTD;
  mysqlshdk::storage::Compression_options m_compression_options;

  std::string m_character_set = "utf8mb4";

  import_table::Dialect m_dialect;
  import_table::Dialect m_dialect_unpacker;

  Dry_run m_dry_run_mode = Dry_run::DISABLED;

  bool m_write_index_files = true;

  bool m_rename_data_files = true;

  // schema -> table -> condition
  std::unordered_map<std::string, std::unordered_map<std::string, std::string>>
      m_where;

  // schema -> table -> partitions
  Instance_cache_builder::Partition_filters m_partitions;

  // these options are unpacked elsewhere, but are here 'cause we're returning
  // a reference

  // currently used by dumpTables(), dumpSchemas() and dumpInstance()
  bool m_is_mds = false;
  Compatibility_options m_compatibility_options;
  std::optional<mysqlshdk::utils::Version> m_target_version;
};

}  // namespace dump
}  // namespace mysqlsh

#endif  // MODULES_UTIL_DUMP_DUMP_OPTIONS_H_
