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

#include <cassert>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include "mysqlshdk/include/scripting/types_cpp.h"
#include "mysqlshdk/libs/db/mysql/session.h"
#include "mysqlshdk/libs/storage/compressed_file.h"
#include "mysqlshdk/libs/storage/ifile.h"

#include "modules/util/common/common_options.h"
#include "modules/util/import_table/dialect.h"

namespace mysqlsh {
namespace import_table {

enum class Duplicate_handling {
  Default,
  Ignore,
  Replace,
};

class Import_table_option_pack : public common::Common_options {
 public:
  Import_table_option_pack();

  Import_table_option_pack(const Import_table_option_pack &) = default;
  Import_table_option_pack(Import_table_option_pack &&) = default;

  Import_table_option_pack &operator=(const Import_table_option_pack &) =
      default;
  Import_table_option_pack &operator=(Import_table_option_pack &&) = default;

  ~Import_table_option_pack() override = default;

  static const shcore::Option_pack_def<Import_table_option_pack> &options();

  void set_decode_columns(const shcore::Dictionary_t &decode_columns);

  const Dialect &dialect() const { return m_dialect; }

  bool is_multifile() const noexcept { return m_is_multifile; }

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

  bool verbose() const { return m_verbose; }

  void set_verbose(bool verbose) { m_verbose = verbose; }

  const std::vector<std::string> &session_init_sql() const {
    return m_session_init_sql;
  }

 private:
  void set_max_transaction_size(const std::string &value);
  void set_bytes_per_chunk(const std::string &value);
  void set_max_rate(const std::string &value);
  bool check_if_multifile();
  void set_replace_duplicates(bool flag);
  void set_columns(const shcore::Array_t &columns);

 protected:
  std::vector<std::string> m_filelist_from_user;
  bool m_is_multifile = false;
  std::string m_table;
  mutable std::string m_schema;
  std::string m_partition;
  std::string m_character_set;
  int64_t m_threads_size = 8;
  std::optional<uint64_t> m_bytes_per_chunk;
  size_t m_max_bytes_per_transaction = 0;
  std::vector<std::variant<std::string, uint64_t>> m_columns;
  std::map<std::string, std::string> m_decode_columns;
  Duplicate_handling m_duplicate_handling = Duplicate_handling::Ignore;
  uint64_t m_max_rate = 0;
  mutable uint64_t m_skip_rows_count = 0;
  Dialect m_dialect;
  bool m_verbose = true;

  std::vector<std::string> m_session_init_sql;
};

class Import_table_options : public Import_table_option_pack {
 public:
  Import_table_options() = default;

  explicit Import_table_options(const Import_table_option_pack &pack);

  static Import_table_options unpack(const shcore::Dictionary_t &options);

  size_t file_size() const {
    assert(!is_multifile());
    return m_file_size;
  }

  const std::string &masked_full_path() const {
    assert(!is_multifile());
    return m_full_path;
  }

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
  using Common_options::on_set_url;
  using Common_options::set_url;

  size_t calc_thread_size();

  void on_validate() const override;

  void on_configure() override;

  size_t m_file_size;
  std::string m_full_path;
  mysqlshdk::storage::Compression m_compression =
      mysqlshdk::storage::Compression::NONE;
};

}  // namespace import_table
}  // namespace mysqlsh

#endif  // MODULES_UTIL_IMPORT_TABLE_IMPORT_TABLE_OPTIONS_H_
