/*
 * Copyright (c) 2018, 2019, Oracle and/or its affiliates. All rights reserved.
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

#ifndef MODULES_UTIL_IMPORT_TABLE_LOAD_DATA_H_
#define MODULES_UTIL_IMPORT_TABLE_LOAD_DATA_H_

#include <atomic>
#include <mutex>
#include <string>
#include <vector>

#include "modules/util/import_table/chunk_file.h"
#include "modules/util/import_table/import_table.h"
#include "modules/util/import_table/import_table_options.h"
#include "mysqlshdk/libs/textui/text_progress.h"
#include "mysqlshdk/libs/utils/rate_limit.h"
#include "mysqlshdk/libs/utils/synchronized_queue.h"

namespace mysqlsh {
namespace import_table {

/**
 * Local infile userdata structure that controls and synchronizes threads which
 * imports data in util.importTable
 */
struct File_info {
  mysqlshdk::utils::Rate_limit rate_limit{};  //< Rate limiter
  int64_t max_rate = 0;    //< Max rate value for rate limiter
  int fd = 0;              //< File descriptor with data to import
  int64_t worker_id = -1;  //< Thread worker id
  std::string filename;    //< Import data filename path
  size_t chunk_start = 0;  //< File chunk start offset
  size_t bytes_left = 0;   //< Bytes left to read from file

  std::mutex *prog_mutex;  //< Pointer to mutex for progress bar access
  mysqlshdk::textui::IProgress *prog;  //< Pointer to current progress bar
  std::atomic<size_t>
      *prog_bytes;  //< Pointer cumulative bytes send to MySQL Server
  volatile bool *user_interrupt = nullptr;  //< Pointer to user interrupt flag
};

// Functions for local infile callbacks.
int local_infile_init(void **buffer, const char *filename, void *userdata);
int local_infile_read(void *userdata, char *buffer, unsigned int length);
void local_infile_end(void *userdata);
int local_infile_error(void *userdata, char *error_msg,
                       unsigned int error_msg_len);

class Load_data_worker final {
 public:
  Load_data_worker() = delete;
  Load_data_worker(const Import_table_options &opt, int64_t thread_id,
                   mysqlshdk::textui::IProgress *progress,
                   std::atomic<size_t> *prog_sent_bytes,
                   std::mutex *output_mutex, volatile bool *interrupt,
                   shcore::Synchronized_queue<Range> *range_queue,
                   std::vector<std::exception_ptr> *thread_exception,
                   bool use_json, Stats *stats);
  Load_data_worker(const Load_data_worker &other) = default;
  Load_data_worker(Load_data_worker &&other) = default;

  Load_data_worker &operator=(const Load_data_worker &other) = delete;
  Load_data_worker &operator=(Load_data_worker &&other) = delete;

  ~Load_data_worker() = default;

  void operator()();

 private:
  const Import_table_options &m_opt;
  int64_t m_thread_id;
  mysqlshdk::textui::IProgress *m_progress;
  std::atomic<size_t> &m_prog_sent_bytes;
  std::mutex &m_output_mutex;
  volatile bool &m_interrupt;
  shcore::Synchronized_queue<Range> &m_range_queue;
  std::vector<std::exception_ptr> &m_thread_exception;
  bool m_use_json;
  Stats &m_stats;
};

}  // namespace import_table
}  // namespace mysqlsh

#endif  // MODULES_UTIL_IMPORT_TABLE_LOAD_DATA_H_
