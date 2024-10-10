/*
 * Copyright (c) 2024, Oracle and/or its affiliates.
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

#ifndef MODULES_UTIL_BINLOG_BINLOG_DUMP_INFO_H_
#define MODULES_UTIL_BINLOG_BINLOG_DUMP_INFO_H_

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "mysqlshdk/libs/storage/compressed_file.h"
#include "mysqlshdk/libs/storage/idirectory.h"
#include "mysqlshdk/libs/storage/ifile.h"
#include "mysqlshdk/libs/utils/utils_json.h"
#include "mysqlshdk/libs/utils/version.h"

#include "modules/util/common/dump/server_info.h"

namespace mysqlsh {
namespace binlog {

class Binlog_dump;

struct Binlog_file {
  std::string_view name;
  std::string_view basename;
  std::string extension;
  mysqlshdk::storage::Compression compression;
  bool compressed = false;

  std::string gtid_set;
  uint64_t data_bytes = 0;
  uint64_t file_bytes = 0;
  uint64_t events = 0;

  uint64_t begin = 0;
  uint64_t end = 0;

  const Binlog_dump *dump;
};

/**
 * Information about a single binary log dump.
 */
class Binlog_dump final {
 public:
  Binlog_dump() = delete;

  Binlog_dump(mysqlshdk::storage::IDirectory *root,
              const std::string &dir_name);

  Binlog_dump(const Binlog_dump &) = delete;
  Binlog_dump(Binlog_dump &&) = default;

  Binlog_dump &operator=(const Binlog_dump &) = delete;
  Binlog_dump &operator=(Binlog_dump &&) = default;

  ~Binlog_dump() = default;

  const mysqlshdk::utils::Version &dump_version() const noexcept {
    return m_version;
  }

  const dump::common::Server_info &source_instance() const noexcept {
    return m_source;
  }

  const dump::common::Binlog &start_from() const noexcept {
    return m_start_from;
  }

  const dump::common::Binlog &end_at() const noexcept { return m_end_at; }

  const std::string &gtid_set() const noexcept { return m_gtid_set; }

  const std::string &directory_name() const noexcept { return m_dir_name; }

  const std::string &full_path() const noexcept { return m_full_path; }

  const std::string &timestamp() const noexcept { return m_timestamp; }

  std::unique_ptr<mysqlshdk::storage::IDirectory> make_directory() const;

  std::unique_ptr<mysqlshdk::storage::IFile> make_file(
      const Binlog_file &file) const;

  static std::unique_ptr<mysqlshdk::storage::IFile> make_file(
      mysqlshdk::storage::IDirectory *dir, const Binlog_file &file);

  std::vector<Binlog_file> binlog_files() const;

 private:
  void read_begin_metadata(const shcore::json::JSON &md);

  void read_end_metadata(const shcore::json::JSON &md);

  mysqlshdk::storage::IDirectory *m_root;
  std::string m_dir_name;
  std::string m_full_path;

  mysqlshdk::utils::Version m_version;
  dump::common::Server_info m_source;
  dump::common::Binlog m_start_from;
  dump::common::Binlog m_end_at;
  std::string m_gtid_set;

  std::vector<std::string> m_binlogs;
  std::unordered_map<std::string, std::string> m_basenames;

  std::string m_timestamp;
};

/**
 * Information about all the binary log dumps.
 */
class Binlog_dump_info final {
 public:
  Binlog_dump_info() = delete;

  explicit Binlog_dump_info(
      std::unique_ptr<mysqlshdk::storage::IDirectory> root);

  Binlog_dump_info(const Binlog_dump_info &) = delete;
  Binlog_dump_info(Binlog_dump_info &&) = default;

  Binlog_dump_info &operator=(const Binlog_dump_info &) = delete;
  Binlog_dump_info &operator=(Binlog_dump_info &&) = default;

  ~Binlog_dump_info() = default;

  const std::vector<Binlog_dump> &dumps() const noexcept { return m_dumps; }

  mysqlshdk::Masked_string full_path() const noexcept {
    return m_root->full_path();
  }

 private:
  std::unique_ptr<mysqlshdk::storage::IDirectory> m_root;
  std::vector<Binlog_dump> m_dumps;
};

}  // namespace binlog
}  // namespace mysqlsh

#endif  // MODULES_UTIL_BINLOG_BINLOG_DUMP_INFO_H_
