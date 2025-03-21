/*
 * Copyright (c) 2024, 2025, Oracle and/or its affiliates.
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

#ifndef MODULES_UTIL_BINLOG_DUMP_BINLOGS_OPTIONS_H_
#define MODULES_UTIL_BINLOG_DUMP_BINLOGS_OPTIONS_H_

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "mysqlshdk/include/scripting/types/option_pack.h"

#include "mysqlshdk/libs/db/session.h"
#include "mysqlshdk/libs/storage/compressed_file.h"
#include "mysqlshdk/libs/storage/idirectory.h"
#include "mysqlshdk/libs/storage/ifile.h"

#include "modules/util/common/common_options.h"
#include "modules/util/common/dump/server_info.h"

namespace mysqlsh {
namespace binlog {

class Dump_binlogs_options final : public common::Common_options {
 public:
  Dump_binlogs_options();

  Dump_binlogs_options(const Dump_binlogs_options &other) = default;
  Dump_binlogs_options(Dump_binlogs_options &&other) = default;

  Dump_binlogs_options &operator=(const Dump_binlogs_options &other) = default;
  Dump_binlogs_options &operator=(Dump_binlogs_options &&other) = default;

  ~Dump_binlogs_options() override = default;

  static const shcore::Option_pack_def<Dump_binlogs_options> &options();

  const shcore::Dictionary_t &original_options() const noexcept {
    return m_options;
  }

  std::unique_ptr<mysqlshdk::storage::IDirectory> output() const;

  const std::string &previous_dump() const noexcept { return m_previous_dump; }

  const std::string &previous_dump_timestamp() const noexcept {
    return m_previous_dump_timestamp;
  }

  const dump::common::Binlog &start_from() const noexcept {
    return m_start_from_binlog;
  }

  const dump::common::Binlog &end_at() const noexcept {
    return m_source_instance.binlog;
  }

  const std::string &gtid_set() const noexcept { return m_gtid_set; }

  const dump::common::Server_info &source_instance() const noexcept {
    return m_source_instance;
  }

  const std::vector<mysqlshdk::storage::File_info> &binlogs() const noexcept {
    return m_binlogs;
  }

  uint64_t threads() const noexcept { return m_threads; }

  bool dry_run() const noexcept { return m_dry_run; }

  mysqlshdk::storage::Compression compression() const noexcept {
    return m_compression;
  }

  const mysqlshdk::storage::Compression_options &compression_options()
      const noexcept {
    return m_compression_options;
  }

 private:
  // options handling
  void on_start_unpack(const shcore::Dictionary_t &options);

  void set_since(const std::string &since);

  void set_start_from(const std::string &start_from);

  void set_threads(uint64_t threads);

  void set_compression(const std::string &compression);

  void on_unpacked_options();

  // input URLs handling
  void read_dump(std::unique_ptr<mysqlshdk::storage::IDirectory> dir);

  void read_dump_instance(std::unique_ptr<mysqlshdk::storage::IDirectory> dir);

  void read_dump_binlogs(std::unique_ptr<mysqlshdk::storage::IDirectory> dir);

  // fetches basic information, validates preconditions
  void on_set_session(
      const std::shared_ptr<mysqlshdk::db::ISession> &session) override;

  void initialize_source_instance();

  // validate input directories
  void on_validate() const override;

  // initialize dump information
  void on_configure() override;

  // validations
  void verify_startup_options() const;
  void validate_continuity() const;
  void validate_start_from();

  void gather_binlogs();

  void find_start_from(
      const std::vector<mysqlshdk::storage::File_info> &binlogs);

  bool is_same_instance() const noexcept;

  // original options
  shcore::Dictionary_t m_options;

  // options
  std::string m_since;
  std::string m_start_from;
  // WL15977-FR2.2.5.2
  uint64_t m_threads = 4;
  // WL15977-FR2.2.6.2
  bool m_dry_run = false;
  // WL15977-FR2.2.7.2
  mysqlshdk::storage::Compression m_compression =
      mysqlshdk::storage::Compression::ZSTD;
  mysqlshdk::storage::Compression_options m_compression_options;
  // WL15977-FR2.2.9.3
  std::optional<bool> m_ignore_ddl_changes;

  // computed from other options
  dump::common::Binlog m_start_from_binlog;

  std::string m_gtid_set;

  std::string m_previous_server_uuid;
  dump::common::Server_version m_previous_version;
  dump::common::Replication_topology m_previous_topology;
  std::string m_previous_dump;
  std::string m_previous_dump_timestamp;

  // data
  dump::common::Server_info m_source_instance;
  std::vector<mysqlshdk::storage::File_info> m_binlogs;
};

}  // namespace binlog
}  // namespace mysqlsh

#endif  // MODULES_UTIL_BINLOG_DUMP_BINLOGS_OPTIONS_H_
