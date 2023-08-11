/*
 * Copyright (c) 2018, 2023, Oracle and/or its affiliates.
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
#include <exception>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "modules/util/import_table/chunk_file.h"
#include "modules/util/import_table/import_table.h"
#include "modules/util/import_table/import_table_options.h"
#include "mysqlshdk/include/shellcore/shell_options.h"
#include "mysqlshdk/libs/db/mysql/session.h"
#include "mysqlshdk/libs/storage/compressed_file.h"
#include "mysqlshdk/libs/storage/ifile.h"
#include "mysqlshdk/libs/textui/text_progress.h"
#include "mysqlshdk/libs/utils/rate_limit.h"
#include "mysqlshdk/libs/utils/synchronized_queue.h"

namespace mysqlsh {
namespace import_table {

struct Transaction_options {
  uint64_t max_trx_size = 0;  //< 0 disables the sub-chunking
  uint64_t skip_bytes = 0;    //< start transaction at this offset
  std::function<void()> transaction_started;
  std::function<void(uint64_t)> transaction_finished;
};

class Transaction_buffer {
 public:
  Transaction_buffer() = default;

  Transaction_buffer(Dialect dialect, mysqlshdk::storage::IFile *file,
                     const Transaction_options &options = {})
      : Transaction_buffer(dialect, file, options.max_trx_size,
                           options.skip_bytes) {
    m_options = options;
  }

  Transaction_buffer(Dialect dialect, mysqlshdk::storage::IFile *file,
                     uint64_t max_transaction_size, uint64_t skip_bytes);

  void before_query();

  int read(char *buffer, unsigned int length);

  bool flush_pending() const;
  void flush_done(bool *out_has_more_data);

  uint64_t oversized_rows() const { return m_oversized_rows; }

  void on_oversized_row(const std::function<void(uint64_t)> &callback) {
    m_on_oversized_row = callback;
  }

 private:
  int consume(char *buffer, unsigned int length);

  int64_t trx_bytes_left() const { return m_options.max_trx_size - m_trx_size; }

  uint64_t (Transaction_buffer::*find_first_row_boundary_after)() const;
  uint64_t (Transaction_buffer::*find_last_row_boundary_before)(uint64_t);

  uint64_t find_first_row_boundary_after_impl_default() const;
  uint64_t find_last_row_boundary_before_impl_default(uint64_t limit);

  uint64_t find_first_row_boundary_after_impl_no_escape() const;
  uint64_t find_last_row_boundary_before_impl_no_escape(uint64_t limit);

  uint64_t find_first_row_boundary_after_impl_escape() const;
  uint64_t find_last_row_boundary_before_impl_escape(uint64_t limit);

  void set_trx_end_offset(uint64_t end) { m_trx_end_offset = m_trx_size + end; }

  Dialect m_dialect;
  mysqlshdk::storage::IFile *m_file = nullptr;
  Transaction_options m_options;

  uint64_t m_trx_size = 0;  // current trx size
  uint64_t m_trx_end_offset =
      0;  // offset of the end of the trx once we know it
  bool m_partial_row_sent = false;
  bool m_eof = false;

  std::string m_data;

  uint64_t m_oversized_rows = 0;

  std::function<void(uint64_t)> m_on_oversized_row;
};

/**
 * Local infile userdata structure that controls and synchronizes threads which
 * imports data in util.importTable
 */
struct File_info {
  mysqlshdk::utils::Rate_limit rate_limit{};  //< Rate limiter
  int64_t max_rate = 0;    //< Max rate value for rate limiter
  int64_t worker_id = -1;  //< Thread worker id
  std::unique_ptr<mysqlshdk::storage::IFile> filehandler = nullptr;
  mysqlshdk::storage::Compressed_file *compressed_file = nullptr;
  size_t bytes_left = 0;    //< Bytes left to read from file
  bool range_read = false;  //< Reading whole file vs chunk range

  size_t data_bytes = 0;  //< bytes send to MySQL server
  size_t file_bytes = 0;  //< bytes read from the file
  std::atomic<size_t>
      *prog_data_bytes;  //< Cumulative bytes send to MySQL Server
  std::atomic<size_t> *prog_file_bytes;  //< Cumulative bytes read from the file
  volatile bool *user_interrupt = nullptr;  //< Pointer to user interrupt flag

  // data for transaction size limiter
  Transaction_buffer buffer;  //< buffered file wrapper
  std::exception_ptr last_error;
  bool continuation = false;  //< True if we hit transaction limit and we have
                              // more data waiting to sent in Transaction_buffer
};

// Functions for local infile callbacks.
int local_infile_init(void **buffer, const char *filename,
                      void *userdata) noexcept;
int local_infile_read(void *userdata, char *buffer,
                      unsigned int length) noexcept;
void local_infile_end(void *userdata) noexcept;
int local_infile_error(void *userdata, char *error_msg,
                       unsigned int error_msg_len) noexcept;

class Load_data_worker final {
 public:
  Load_data_worker() = delete;
  Load_data_worker(const Import_table_options &opt, int64_t thread_id,
                   std::atomic<size_t> *prog_sent_bytes,
                   std::atomic<size_t> *prog_file_bytes,
                   volatile bool *interrupt,
                   shcore::Synchronized_queue<File_import_info> *range_queue,
                   std::vector<std::exception_ptr> *thread_exception,
                   Stats *stats, const std::string &query_comment = "");
  Load_data_worker(const Load_data_worker &other) = default;
  Load_data_worker(Load_data_worker &&other) = default;

  Load_data_worker &operator=(const Load_data_worker &other) = delete;
  Load_data_worker &operator=(Load_data_worker &&other) = delete;

  ~Load_data_worker() = default;

  void operator()();
  void execute(const std::shared_ptr<mysqlshdk::db::mysql::Session> &session,
               std::unique_ptr<mysqlshdk::storage::IFile> file,
               const Transaction_options &options = {});

 private:
  const Import_table_options &m_opt;
  int64_t m_thread_id;
  std::atomic<size_t> *m_prog_sent_bytes;
  std::atomic<size_t> *m_prog_file_bytes;
  volatile bool &m_interrupt;
  shcore::Synchronized_queue<File_import_info> *m_range_queue;
  std::vector<std::exception_ptr> &m_thread_exception;
  Stats &m_stats;
  std::string m_query_comment;
};

}  // namespace import_table
}  // namespace mysqlsh

#endif  // MODULES_UTIL_IMPORT_TABLE_LOAD_DATA_H_
