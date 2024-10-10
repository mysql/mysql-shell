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

#ifndef MODULES_UTIL_BINLOG_LOAD_BINLOGS_OPTIONS_H_
#define MODULES_UTIL_BINLOG_LOAD_BINLOGS_OPTIONS_H_

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "mysqlshdk/include/scripting/types_cpp.h"

#include "mysqlshdk/libs/db/session.h"

#include "modules/util/binlog/binlog_dump_info.h"
#include "modules/util/common/common_options.h"
#include "modules/util/common/dump/server_info.h"

namespace mysqlsh {
namespace binlog {

class Load_binlogs_options final : public common::Common_options {
 public:
  Load_binlogs_options();

  Load_binlogs_options(const Load_binlogs_options &other) = default;
  Load_binlogs_options(Load_binlogs_options &&other) = default;

  Load_binlogs_options &operator=(const Load_binlogs_options &other) = default;
  Load_binlogs_options &operator=(Load_binlogs_options &&other) = default;

  ~Load_binlogs_options() override = default;

  static const shcore::Option_pack_def<Load_binlogs_options> &options();

  void set_url(const std::string &url);

  std::string full_path() const noexcept {
    return m_dump->full_path().masked();
  }

  const std::string &first_gtid_set() const noexcept {
    return m_first_gtid_set;
  }

  const std::string &stop_at_gtid() const noexcept { return m_stop_at_gtid; }

  const dump::common::Binlog::File &stop_at_file() const noexcept {
    return m_stop_at;
  }

  const std::vector<Binlog_file> &binlogs() const noexcept { return m_binlogs; }

  bool has_compressed_binlogs() const noexcept {
    return m_has_compressed_binlogs;
  }

  bool dry_run() const noexcept { return m_dry_run; }

 private:
  // options handling
  void set_stop_before(const std::string &stop_before);

  // URLs handling
  void read_dump(std::unique_ptr<mysqlshdk::storage::IDirectory> dir);

  // fetches basic information, validates preconditions
  void on_set_session(
      const std::shared_ptr<mysqlshdk::db::ISession> &session) override;

  // initialize dump information
  void on_configure() override;
  void gather_binlogs();

  // input URL
  std::string m_url;

  // options
  // WL15977-FR3.2.1.2
  bool m_ignore_version = false;
  // WL15977-FR3.2.2.2
  bool m_ignore_gtid_gap = false;
  // WL15977-FR3.2.4.2
  bool m_dry_run = false;

  // information about the target instance
  dump::common::Server_version m_target_version;
  dump::common::Binlog m_start_from;

  std::string m_first_gtid_set;
  std::string m_stop_at_gtid;
  dump::common::Binlog::File m_stop_at;

  // information about the dump which is going to be loaded
  std::shared_ptr<Binlog_dump_info> m_dump;
  std::vector<Binlog_file> m_binlogs;
  bool m_has_compressed_binlogs = false;
};

}  // namespace binlog
}  // namespace mysqlsh

#endif  // MODULES_UTIL_BINLOG_LOAD_BINLOGS_OPTIONS_H_
