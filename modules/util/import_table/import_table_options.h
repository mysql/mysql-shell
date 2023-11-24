/*
 * Copyright (c) 2018, 2023, Oracle and/or its affiliates.
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
#include <vector>

#include "modules/util/import_table/dialect.h"
#include "mysqlshdk/include/scripting/type_info/custom.h"
#include "mysqlshdk/include/scripting/type_info/generic.h"
#include "mysqlshdk/include/scripting/types_cpp.h"
#include "mysqlshdk/libs/aws/s3_bucket_options.h"
#include "mysqlshdk/libs/azure/blob_storage_options.h"
#include "mysqlshdk/libs/db/connection_options.h"
#include "mysqlshdk/libs/db/mysql/session.h"
#include "mysqlshdk/libs/oci/oci_bucket_options.h"
#include "mysqlshdk/libs/storage/config.h"
#include "mysqlshdk/libs/storage/ifile.h"

namespace mysqlsh {
namespace import_table {

using Connection_options = mysqlshdk::db::Connection_options;

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

  bool is_compressed(const std::string &path) const;

  void set_filenames(const std::vector<std::string> &filenames);

  const std::vector<std::string> &filelist_from_user() const {
    return m_filelist_from_user;
  }

  const std::string &single_file() const {
    assert(!is_multifile());
    return m_filelist_from_user[0];
  }

  uint64_t max_rate() const;

  bool replace_duplicates() const { return m_replace_duplicates; }

  void set_replace_duplicates(bool flag) { m_replace_duplicates = flag; }

  const shcore::Array_t &columns() const { return m_columns; }

  const std::map<std::string, std::string> &decode_columns() const {
    return m_decode_columns;
  }

  bool show_progress() const { return m_show_progress; }

  uint64_t skip_rows_count() const { return m_skip_rows_count; }

  int64_t threads_size() const { return m_threads_size; }

  const std::string &table() const { return m_table; }

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
  shcore::Array_t m_columns;
  std::map<std::string, std::string> m_decode_columns;
  bool m_replace_duplicates = false;
  uint64_t m_max_rate = 0;
  bool m_show_progress = isatty(fileno(stdout)) ? true : false;
  uint64_t m_skip_rows_count = 0;
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

 private:
  size_t calc_thread_size();

  std::shared_ptr<mysqlshdk::db::ISession> m_base_session;
  size_t m_file_size;
  std::string m_full_path;
};

}  // namespace import_table
}  // namespace mysqlsh

#endif  // MODULES_UTIL_IMPORT_TABLE_IMPORT_TABLE_OPTIONS_H_
