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

#ifndef MODULES_UTIL_LOAD_LOAD_DUMP_OPTIONS_H_
#define MODULES_UTIL_LOAD_LOAD_DUMP_OPTIONS_H_

#include <cinttypes>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

#include "mysqlshdk/include/scripting/types.h"
#include "mysqlshdk/libs/aws/s3_bucket_options.h"
#include "mysqlshdk/libs/azure/blob_storage_options.h"
#include "mysqlshdk/libs/db/connection_options.h"
#include "mysqlshdk/libs/db/filtering_options.h"
#include "mysqlshdk/libs/oci/oci_bucket_options.h"
#include "mysqlshdk/libs/storage/config.h"
#include "mysqlshdk/libs/storage/idirectory.h"
#include "mysqlshdk/libs/storage/ifile.h"
#include "mysqlshdk/libs/utils/utils_file.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_sqlstring.h"
#include "mysqlshdk/libs/utils/version.h"

#include "modules/mod_utils.h"
#include "modules/util/import_table/helpers.h"

namespace mysqlsh {

inline std::string schema_object_key(std::string_view schema,
                                     std::string_view table) {
  std::string res;
  res.reserve(schema.size() + table.size() + 5);

  res.append(shcore::quote_identifier(schema));

  if (!table.empty()) {
    res.append(".");
    res.append(shcore::quote_identifier(table));
  }

  return res;
}

inline std::string schema_table_object_key(std::string_view schema,
                                           std::string_view table,
                                           std::string_view partition) {
  auto res = schema_object_key(schema, table);
  if (partition.empty()) return res;

  res.append(".");
  res.append(shcore::quote_identifier(partition));
  return res;
}

#ifdef _WIN32
#undef IGNORE
#endif

class Load_dump_options {
 public:
  using Filtering_options = mysqlshdk::db::Filtering_options;
  using Connection_options = mysqlshdk::db::Connection_options;

  enum class Analyze_table_mode { OFF, HISTOGRAM, ON };

  enum class Defer_index_mode { OFF, FULLTEXT, ALL };

  enum class Update_gtid_set { OFF, REPLACE, APPEND };

  enum class Handle_grant_errors { ABORT, DROP_ACCOUNT, IGNORE };

  enum class Bulk_load_fs { UNSUPPORTED, INFILE, URL, S3 };

  struct Bulk_load_info {
    bool enabled = false;
    bool monitoring = false;
    uint64_t threads = 0;
    Bulk_load_fs fs = Bulk_load_fs::UNSUPPORTED;
    std::string file_prefix;
  };

  Load_dump_options();

  explicit Load_dump_options(const std::string &url);

  Load_dump_options(const Load_dump_options &other) = default;
  Load_dump_options(Load_dump_options &&other) = default;

  Load_dump_options &operator=(const Load_dump_options &other) = default;
  Load_dump_options &operator=(Load_dump_options &&other) = default;

  ~Load_dump_options() = default;

  static const shcore::Option_pack_def<Load_dump_options> &options();

  void validate();

  void on_unpacked_options();

  void on_log_options(const char *msg) const;

  void set_session(const std::shared_ptr<mysqlshdk::db::ISession> &session);

  std::shared_ptr<mysqlshdk::db::ISession> base_session() const {
    return m_base_session;
  }

  const Connection_options &connection_options() const { return m_target; }

  std::string target_import_info(const char *operation = "Loading",
                                 const std::string &extra = {}) const;

  std::unique_ptr<mysqlshdk::storage::IDirectory> create_dump_handle() const;

  std::unique_ptr<mysqlshdk::storage::IFile> create_progress_file_handle()
      const;

  const std::optional<std::string> &progress_file() const {
    return m_progress_file;
  }

  void set_progress_file(const std::string &file);

  const std::string &default_progress_file() const {
    return m_default_progress_file;
  }

  const mysqlshdk::storage::Config_ptr &storage_config() const {
    return m_storage_config;
  }

  void set_storage_config(
      std::shared_ptr<mysqlshdk::storage::Config> storage_config);

  bool show_progress() const { return m_show_progress; }

  void set_show_progress(bool show) { m_show_progress = show; }

  uint64_t threads_count() const { return m_threads_count; }

  uint64_t background_threads_count(uint64_t def) const {
    return m_background_threads_count.value_or(def);
  }

  void set_background_threads_count(uint64_t count) {
    m_background_threads_count = count;
  }

  uint64_t threads_per_add_index() const { return m_threads_per_add_index; }

  uint64_t dump_wait_timeout_ms() const { return m_wait_dump_timeout_ms; }

  void set_dump_wait_timeout_ms(uint64_t timeout_ms) {
    m_wait_dump_timeout_ms = timeout_ms;
  }

  const std::string &character_set() const { return m_character_set; }

  bool load_data() const { return m_load_data; }

  void set_load_data(bool load) { m_load_data = load; }

  bool load_ddl() const { return m_load_ddl; }

  void set_load_ddl(bool load) { m_load_ddl = load; }

  bool load_users() const { return m_load_users; }

  void set_load_users(bool load) { m_load_users = load; }

  bool dry_run() const { return m_dry_run; }

  bool reset_progress() const { return m_reset_progress; }

  bool skip_binlog() const { return m_skip_binlog; }

  bool force() const { return m_force; }

  const Filtering_options &filters() const { return m_filtering_options; }

  inline Filtering_options &filters() { return m_filtering_options; }

  bool ignore_existing_objects() const { return m_ignore_existing_objects; }

  bool drop_existing_objects() const { return m_drop_existing_objects; }

  bool ignore_version() const { return m_ignore_version; }

  Defer_index_mode defer_table_indexes() const { return m_defer_table_indexes; }

  bool load_deferred_indexes() const {
    return m_load_indexes && m_defer_table_indexes != Defer_index_mode::OFF;
  }

  Analyze_table_mode analyze_tables() const { return m_analyze_tables; }

  Update_gtid_set update_gtid_set() const { return m_update_gtid_set; }

  const std::string &target_schema() const { return m_target_schema; }

  const mysqlshdk::utils::Version &target_server_version() const {
    return m_target_server_version;
  }

  bool is_mds() const { return m_is_mds; }

  void set_url(const std::string &url) { m_url = url; }

  bool show_metadata() const { return m_show_metadata; }

  void set_show_metadata(bool show) { m_show_metadata = show; }

  bool should_create_pks(bool default_value) const {
    return m_create_invisible_pks.value_or(default_value);
  }

  bool auto_create_pks_supported() const {
    return m_sql_generate_invisible_primary_key.has_value();
  }

  bool sql_generate_invisible_primary_key() const;

  std::optional<uint64_t> max_bytes_per_transaction() const {
    return m_max_bytes_per_transaction;
  }

  const std::string &server_uuid() const { return m_server_uuid; }

  const std::vector<std::string> &session_init_sql() const {
    return m_session_init_sql;
  }

  Handle_grant_errors on_grant_errors() const { return m_handle_grant_errors; }

  bool partial_revokes() const { return m_partial_revokes; }

  inline int8_t lower_case_table_names() const noexcept {
    return m_lower_case_table_names;
  }

  bool fast_sub_chunking() const { return m_use_fast_sub_chunking; }

  void enable_fast_sub_chunking() { m_use_fast_sub_chunking = true; }

  inline bool checksum() const noexcept { return m_checksum; }

  inline const Bulk_load_info &bulk_load_info() const noexcept {
    return m_bulk_load_info;
  }

 private:
  void set_wait_timeout(const double &timeout_seconds);

  void set_max_bytes_per_transaction(const std::string &value);

  void set_handle_grant_errors(const std::string &action);

  inline std::shared_ptr<mysqlshdk::db::IResult> query(
      std::string_view sql) const {
    return m_base_session->query(sql);
  }

  bool include_object(std::string_view schema, std::string_view object,
                      const std::unordered_set<std::string> &included,
                      const std::unordered_set<std::string> &excluded) const;

  void configure_bulk_load();

  std::string m_url;
  uint64_t m_threads_count = 4;
  std::optional<uint64_t> m_background_threads_count;
  bool m_show_progress = isatty(fileno(stdout)) ? true : false;

  mysqlshdk::oci::Oci_bucket_options m_oci_bucket_options;
  mysqlshdk::aws::S3_bucket_options m_s3_bucket_options;
  mysqlshdk::azure::Blob_storage_options m_blob_storage_options;
  mysqlshdk::storage::Config_ptr m_storage_config;
  mysqlshdk::storage::Config_ptr m_progress_file_config;
  Connection_options m_target;
  std::shared_ptr<mysqlshdk::db::ISession> m_base_session;

  Filtering_options m_filtering_options;

  uint64_t m_wait_dump_timeout_ms = 0;
  bool m_reset_progress = false;
  std::optional<std::string> m_progress_file;
  std::string m_default_progress_file;
  std::string m_character_set;
  bool m_load_data = true;
  bool m_load_ddl = true;
  bool m_load_users = false;
  Analyze_table_mode m_analyze_tables = Analyze_table_mode::OFF;
  bool m_dry_run = false;
  bool m_force = false;
  bool m_skip_binlog = false;
  bool m_ignore_existing_objects = false;
  bool m_drop_existing_objects = false;
  bool m_ignore_version = false;
  Defer_index_mode m_defer_table_indexes = Defer_index_mode::FULLTEXT;
  bool m_load_indexes = true;
  Update_gtid_set m_update_gtid_set = Update_gtid_set::OFF;
  std::string m_target_schema;
  bool m_disable_bulk_load = false;

  mysqlshdk::utils::Version m_target_server_version;
  bool m_is_mds = false;
  bool m_show_metadata = false;
  std::optional<bool> m_create_invisible_pks;
  std::optional<bool> m_sql_generate_invisible_primary_key;

  std::optional<uint64_t> m_max_bytes_per_transaction;

  std::string m_server_uuid;

  std::vector<std::string> m_session_init_sql;

  Handle_grant_errors m_handle_grant_errors = Handle_grant_errors::ABORT;

  // how many threads are used by the server per one ALTER TABLE ... ADD INDEX
  uint64_t m_threads_per_add_index = 1;

  bool m_checksum = false;

  // whether partial revokes are enabled
  bool m_partial_revokes = false;

  int8_t m_lower_case_table_names;

  bool m_use_fast_sub_chunking = false;

  Bulk_load_info m_bulk_load_info;
};

}  // namespace mysqlsh

#endif  // MODULES_UTIL_LOAD_LOAD_DUMP_OPTIONS_H_
