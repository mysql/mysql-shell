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

#ifndef MODULES_UTIL_BINLOG_BINLOG_LOADER_H_
#define MODULES_UTIL_BINLOG_BINLOG_LOADER_H_

#include <atomic>
#include <cstdint>

#include "mysqlshdk/libs/utils/atomic_flag.h"

#include "modules/util/binlog/load_binlogs_options.h"
#include "modules/util/dump/progress_thread.h"

namespace mysqlsh {
namespace binlog {

class Binlog_loader final {
 public:
  Binlog_loader() = delete;

  explicit Binlog_loader(const Load_binlogs_options &options);

  Binlog_loader(const Binlog_loader &) = delete;
  Binlog_loader(Binlog_loader &&) = delete;

  Binlog_loader &operator=(const Binlog_loader &) = delete;
  Binlog_loader &operator=(Binlog_loader &&) = delete;

  ~Binlog_loader() = default;

  void run();

  void interrupt();

  void async_interrupt();

 private:
  class File_reader;
  class Process_writer;
  class Process_reader;
  class Statement_executor;
  class Load_progress;

  struct Stats {
    std::size_t binlogs = 0;
    std::atomic_size_t binlogs_read{0};
    uint64_t data_bytes = 0;
    std::atomic_uint64_t data_bytes_read{0};
    uint64_t file_bytes = 0;
    std::atomic_uint64_t file_bytes_read{0};
    std::atomic_uint64_t statements_executed{0};
  };

  void do_run();

  void summarize() const;

  void initialize();
  void load();

  const Load_binlogs_options &m_options;

  shcore::atomic_flag m_worker_interrupt;

  Stats m_load_stats;

  // keep this last
  mutable dump::Progress_thread m_progress_thread;
};

}  // namespace binlog
}  // namespace mysqlsh

#endif  // MODULES_UTIL_BINLOG_BINLOG_LOADER_H_
