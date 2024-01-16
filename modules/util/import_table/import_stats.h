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

struct Stats {
  std::atomic<size_t> total_records{0};
  std::atomic<size_t> total_deleted{0};
  std::atomic<size_t> total_skipped{0};
  std::atomic<size_t> total_warnings{0};
  // total number of uncompressed bytes processed
  std::atomic<size_t> total_data_bytes{0};
  // total number of physical bytes processed
  std::atomic<size_t> total_file_bytes{0};
  std::atomic<size_t> total_files_processed{0};

  std::array<std::atomic<size_t>, Thread_state::LAST> thread_states{};

  std::string to_string() const {
    return std::string{"Records: " + std::to_string(total_records) +
                       "  Deleted: " + std::to_string(total_deleted) +
                       "  Skipped: " + std::to_string(total_skipped) +
                       "  Warnings: " + std::to_string(total_warnings)};
  }
};

}  // namespace import_table
}  // namespace mysqlsh

#endif  // MODULES_UTIL_IMPORT_TABLE_IMPORT_STATS_H_
