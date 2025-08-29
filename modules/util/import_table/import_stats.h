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

#ifndef MODULES_UTIL_IMPORT_TABLE_IMPORT_STATS_H_
#define MODULES_UTIL_IMPORT_TABLE_IMPORT_STATS_H_

#include <array>
#include <atomic>
#include <string>

namespace mysqlsh {
namespace import_table {

enum Thread_state {
  IDLE = 0,
  READING = 1,
  COMMITTING = 2,
  ERROR = 3,
  LAST = 4,
};

/**
 * Statistics which are frequently updated, can be used to track progress.
 */
struct Progress_stats {
  explicit Progress_stats(size_t number_of_threads) {
    thread_states[Thread_state::IDLE] = number_of_threads;
  }

  // number of uncompressed bytes processed
  std::atomic<size_t> data_bytes{0};
  // number of physical bytes processed
  std::atomic<size_t> file_bytes{0};
  // states of various threads
  std::array<std::atomic<size_t>, Thread_state::LAST> thread_states{};
};

/**
 * Statistics which are rarely updated, only once load finishes.
 */
struct Load_stats {
  std::atomic<size_t> records{0};
  std::atomic<size_t> deleted{0};
  std::atomic<size_t> skipped{0};
  std::atomic<size_t> warnings{0};
  // number of uncompressed bytes processed
  std::atomic<size_t> data_bytes{0};
  // number of physical bytes processed
  std::atomic<size_t> file_bytes{0};
  // number of files processed
  std::atomic<size_t> files_processed{0};

  std::string to_string() const {
    return std::string{"Records: " + std::to_string(records) +
                       "  Deleted: " + std::to_string(deleted) +
                       "  Skipped: " + std::to_string(skipped) +
                       "  Warnings: " + std::to_string(warnings)};
  }

  Load_stats &operator+=(const Load_stats &rhs) {
    records += rhs.records;
    deleted += rhs.deleted;
    skipped += rhs.skipped;
    warnings += rhs.warnings;
    data_bytes += rhs.data_bytes;
    file_bytes += rhs.file_bytes;
    files_processed += rhs.files_processed;

    return *this;
  }
};

}  // namespace import_table
}  // namespace mysqlsh

#endif  // MODULES_UTIL_IMPORT_TABLE_IMPORT_STATS_H_
