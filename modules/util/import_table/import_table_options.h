/*
 * Copyright (c) 2018, 2024, Oracle and/or its affiliates.
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

#ifndef MODULES_UTIL_IMPORT_TABLE_IMPORT_TABLE_OPTIONS_H_
#define MODULES_UTIL_IMPORT_TABLE_IMPORT_TABLE_OPTIONS_H_

#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

#include <cassert>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include "modules/util/import_table/dialect.h"
#include "mysqlshdk/include/scripting/types_cpp.h"
#include "mysqlshdk/libs/aws/s3_bucket_options.h"
#include "mysqlshdk/libs/azure/blob_storage_options.h"
#include "mysqlshdk/libs/db/connection_options.h"
#include "mysqlshdk/libs/db/mysql/session.h"
#include "mysqlshdk/libs/oci/oci_bucket_options.h"
#include "mysqlshdk/libs/storage/compressed_file.h"
#include "mysqlshdk/libs/storage/config.h"
#include "mysqlshdk/libs/storage/ifile.h"

namespace mysqlsh {
namespace import_table {

using Connection_options = mysqlshdk::db::Connection_options;

enum class Duplicate_handling {
  Default,
  Ignore,
  Replace,
};

class Import_table_option_pack {
 public:
  Import_table_option_pack()
      : m_blob_storage_options{
            mysqlshdk::azure::Blob_storage_options::Operation::READ} {};
  Import_table_option_pack(const Import_table_option_pack &other) = default;
  Import_table_option_pack(Import_table_option_pack &&other) = default;

  Import_table_option_pack &operator=(const Import_table_option_pack &other) =
      default;
  Import_table_option_pack &operator=(Import_table_option_pack &&other) =
      default;
  ~Import_table_option_pack() = default;

  static const shcore::Option_pack_def<Import_table_option_pack> &options();
  void set_decode_columns(const shcore::Dictionary_t &decode_columns);

  const Dialect &dialect() const { return m_dialect; }

  bool is_multifile() const noexcept { return m_is_multifile; }

  static bool is_compressed(const std::string &path);

  void set_filenames(const std::vector<std::string> &filenames);

  const std::vector<std::string> &filelist_from_user() const {
    return m_filelist_from_user;
  }

  const std::string &single_file() const {
    assert(!is_multifile());
    return m_filelist_from_user[0];
  }

  uint64_t max_rate() const;

  Duplicate_handling duplicate_handling() const { return m_duplicate_handling; }

  void set_duplicate_handling(Duplicate_handling flag) {
    m_duplicate_handling = flag;
  }

  const std::vector<std::variant<std::string, uint64_t>> &columns() const {
    return m_columns;
  }

  void clear_columns() { m_columns.clear(); }

  const std::map<std::string, std::string> &decode_columns() const {
    return m_decode_columns;
  }

  bool show_progress() const { return m_show_progress; }

  uint64_t skip_rows_count() const { return m_skip_rows_count; }

  void clear_skip_rows_count() const { m_skip_rows_count = 0; }

  int64_t threads_size() const { return m_threads_size; }

  const std::string &table() const { return m_table; }

  void set_table(const std::string &table) { m_table = table; }

  const std::string &schema() const { return m_schema; }

  const std::string &partition() const { return m_partition; }

  void set_partition(const std::string &partition) { m_partition = partition; }

  const std::string &character_set() const { return m_character_set; }

  uint64_t bytes_per_chunk() const;

  size_t max_transaction_size() const;

  const mysqlshdk::storage::Config_ptr &storage_config() const {
    return m_storage_config;
  }

  /**
   * Creates a new file handle using the provided options.
   */
  std::unique_ptr<mysqlshdk::storage::IFile> create_file_handle(
      const std::string &filepath) const;

  std::unique_ptr<mysqlshdk::storage::IFile> create_file_handle(
      std::unique_ptr<mysqlshdk::storage::IFile> file_handler) const;

  bool verbose() const { return m_verbose; }

  void set_verbose(bool verbose) { m_verbose = verbose; }

  const std::vector<std::string> &session_init_sql() const {
    return m_session_init_sql;
  }

 private:
  void set_max_transaction_size(const std::string &value);
  void set_bytes_per_chunk(const std::string &value);
  void set_max_rate(const std::string &value);
  void on_unpacked_options();
  bool check_if_multifile();
  void set_replace_duplicates(bool flag);
  void set_columns(const shcore::Array_t &columns);

 protected:
  std::vector<std::string> m_filelist_from_user;
  bool m_is_multifile = false;
  std::string m_table;
  std::string m_schema;
  std::string m_partition;
  std::string m_character_set;
  int64_t m_threads_size = 8;
  std::optional<uint64_t> m_bytes_per_chunk;
  size_t m_max_bytes_per_transaction = 0;
  std::vector<std::variant<std::string, uint64_t>> m_columns;
  std::map<std::string, std::string> m_decode_columns;
  Duplicate_handling m_duplicate_handling = Duplicate_handling::Ignore;
  uint64_t m_max_rate = 0;
  bool m_show_progress = isatty(fileno(stdout)) ? true : false;
  mutable uint64_t m_skip_rows_count = 0;
  Dialect m_dialect;
  mysqlshdk::oci::Oci_bucket_options m_oci_bucket_options;
  mysqlshdk::aws::S3_bucket_options m_s3_bucket_options;
  mysqlshdk::azure::Blob_storage_options m_blob_storage_options;
  mysqlshdk::storage::Config_ptr m_storage_config;
  bool m_verbose = true;

  std::vector<std::string> m_session_init_sql;
};

class Import_table_options : public Import_table_option_pack {
 public:
  Import_table_options() = default;
  explicit Import_table_options(const Import_table_option_pack &pack);

  static Import_table_options unpack(const shcore::Dictionary_t &options);

  void set_base_session(
      const std::shared_ptr<mysqlshdk::db::ISession> &session) {
    m_base_session = session;
  }

  size_t file_size() const {
    assert(!is_multifile());
    return m_file_size;
  }

  const std::string &masked_full_path() const {
    assert(!is_multifile());
    return m_full_path;
  }

  void validate();

  Connection_options connection_options() const;

  std::string target_import_info() const;

  bool dialect_supports_chunking() const;

  mysqlshdk::storage::Compression compression() const noexcept {
    return m_compression;
  }

  // Compression is only supported by the BULK LOAD
  void set_compression(mysqlshdk::storage::Compression compression) noexcept {
    m_compression = compression;
  }

 private:
  size_t calc_thread_size();

  std::shared_ptr<mysqlshdk::db::ISession> m_base_session;
  size_t m_file_size;
  std::string m_full_path;
  mysqlshdk::storage::Compression m_compression =
      mysqlshdk::storage::Compression::NONE;
};

}  // namespace import_table
}  // namespace mysqlsh

#endif  // MODULES_UTIL_IMPORT_TABLE_IMPORT_TABLE_OPTIONS_H_
