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

#ifndef MODULES_UTIL_BINLOG_BINLOG_DUMPER_H_
#define MODULES_UTIL_BINLOG_BINLOG_DUMPER_H_

#include <atomic>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "mysqlshdk/libs/storage/idirectory.h"
#include "mysqlshdk/libs/utils/atomic_flag.h"

#include "modules/util/binlog/dump_binlogs_options.h"
#include "modules/util/common/dump/basenames.h"
#include "modules/util/dump/progress_thread.h"

namespace mysqlsh {
namespace binlog {

class Binlog_dumper final {
 public:
  Binlog_dumper() = delete;

  explicit Binlog_dumper(const Dump_binlogs_options &options);

  Binlog_dumper(const Binlog_dumper &) = delete;
  Binlog_dumper(Binlog_dumper &&) = delete;

  Binlog_dumper &operator=(const Binlog_dumper &) = delete;
  Binlog_dumper &operator=(Binlog_dumper &&) = delete;

  ~Binlog_dumper() = default;

  void run();

  void interrupt();

  void async_interrupt();

 private:
  class Dumper;
  class Dump_progress;

  struct Compression {
    bool enabled;
    mysqlshdk::storage::Compression type;
    std::string extension;
    std::string name;
  };

  struct Binlog_task {
    std::string name;
    std::string basename;
    uint64_t total_size = 0;
    uint64_t begin = 0;
    uint64_t end = 0;

    struct {
      std::string gtid_set;
      uint64_t data_bytes = 0;
      uint64_t file_bytes = 0;
      uint64_t events = 0;
      std::string begin;
      std::string end;
    } stats;
  };

  struct Stats {
    std::size_t binlogs = 0;
    std::atomic_size_t binlogs_written{0};
    uint64_t data_bytes = 0;
    std::atomic_uint64_t data_bytes_written{0};
    std::atomic_uint64_t file_bytes_written{0};
    uint64_t events_written = 0;
    std::string gtid_set;
  };

  void do_run();

  void create_tasks();

  void create_output_dirs();

  void write_begin_metadata() const;
  void write_end_metadata() const;

  void write_binlog_metadata(const Binlog_task &task) const;

  void dump();

  void summarize() const;

  void clean_up();
  void do_clean_up();
  void clean_up_dump_dir();
  void clean_up_root_dir();

  inline bool compressed() const noexcept { return m_compression.enabled; }

  std::unique_ptr<mysqlshdk::storage::IFile> make_binlog_file(
      const Binlog_task &task) const;

  const Dump_binlogs_options &m_options;

  bool m_dry_run;
  Compression m_compression;

  shcore::atomic_flag m_worker_interrupt;
  std::atomic_bool m_interrupted{false};
  std::atomic_bool m_cleaning_up{false};

  bool m_root_dir_created = false;
  std::unique_ptr<mysqlshdk::storage::IDirectory> m_root_dir;

  bool m_dump_dir_created = false;
  std::unique_ptr<mysqlshdk::storage::IDirectory> m_dump_dir;

  dump::common::Basenames m_basenames;
  std::vector<Binlog_task> m_binlog_tasks;

  Stats m_dump_stats;

  // keep this last
  mutable dump::Progress_thread m_progress_thread;
};

}  // namespace binlog
}  // namespace mysqlsh

#endif  // MODULES_UTIL_BINLOG_BINLOG_DUMPER_H_
