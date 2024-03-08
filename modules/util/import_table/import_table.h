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

#ifndef MODULES_UTIL_IMPORT_TABLE_IMPORT_TABLE_H_
#define MODULES_UTIL_IMPORT_TABLE_IMPORT_TABLE_H_

#include <atomic>
#include <exception>
#include <memory>
#include <optional>
#include <string>
#include <thread>
#include <vector>

#include "modules/util/dump/progress_thread.h"
#include "modules/util/import_table/chunk_file.h"
#include "modules/util/import_table/import_table_options.h"
#include "mysqlshdk/libs/utils/synchronized_queue.h"

#include "modules/util/import_table/import_stats.h"

namespace mysqlshdk::storage::in_memory {

class Allocator;

}  // namespace mysqlshdk::storage::in_memory

namespace mysqlsh {
namespace import_table {

class Import_table final {
 public:
  Import_table() = delete;
  explicit Import_table(const Import_table_options &options);
  Import_table(const Import_table &other) = delete;
  Import_table(Import_table &&other) = delete;

  Import_table &operator=(const Import_table &other) = delete;
  Import_table &operator=(Import_table &&other) = delete;

  ~Import_table();

  void interrupt(volatile bool *interrupt) { m_interrupt = interrupt; }

  void import();
  bool any_exception() const;
  void rethrow_exceptions();

  std::string import_summary() const;
  std::string rows_affected_info();

 private:
  void spawn_workers(bool skip_rows);
  void join_workers();
  void chunk_file();
  void build_queue();
  void progress_setup();
  void progress_shutdown();
  void scan_file();

  inline bool interrupted() const {
    return (m_interrupt && *m_interrupt) || any_exception();
  }

  std::atomic<size_t> m_prog_sent_bytes{0};
  std::atomic<size_t> m_prog_file_bytes{0};
  std::optional<size_t> m_prog_total_file_bytes;
  size_t m_total_file_size = 0;
  bool m_has_compressed_files = false;

  std::unique_ptr<mysqlshdk::storage::in_memory::Allocator> m_allocator;

  shcore::Synchronized_queue<File_import_info> m_range_queue;

  const Import_table_options &m_opt;
  // when loading a single file in chunks, we skip the rows while chunking;
  // in such case we don't want the worker to do this, hence we clear this
  // option and use a cached value locally
  uint64_t m_skip_rows_count;
  Stats m_stats;

  volatile bool *m_interrupt;
  std::vector<std::thread> m_threads;
  std::vector<std::exception_ptr> m_thread_exception;

  // Store messages from errors that are not critical for import procedure, but
  // required for setting non-zero exit code.
  std::vector<std::string> noncritical_errors;

  // progress thread needs to be placed after any of the fields it uses, in
  // order to ensure that it is destroyed (and stopped) before any of those
  // fields
  dump::Progress_thread m_progress_thread;
};

}  // namespace import_table
}  // namespace mysqlsh

#endif  // MODULES_UTIL_IMPORT_TABLE_IMPORT_TABLE_H_
