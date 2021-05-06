/*
 * Copyright (c) 2018, 2021, Oracle and/or its affiliates.
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
#include "mysqlshdk/libs/storage/ifile.h"
#include "mysqlshdk/libs/textui/text_progress.h"
#include "mysqlshdk/libs/utils/rate_limit.h"
#include "mysqlshdk/libs/utils/synchronized_queue.h"

namespace mysqlsh {
namespace import_table {

struct Transaction_options {
  uint64_t max_trx_size = 0;  //< 0 disables the sub-chunking
  uint64_t skip_bytes = 0;    //< start transaction at this offset
  const std::vector<uint64_t> *offsets =
      nullptr;  //< offsets which point to end of a row
  std::function<void()> transaction_started;
  std::function<void(uint64_t)> transaction_finished;
};

class Transaction_buffer {
 public:
  struct Dumper_Tx_buffer {};
  struct General_Tx_buffer {};

  Transaction_buffer() = default;

  Transaction_buffer(Dumper_Tx_buffer, Dialect dialect,
                     mysqlshdk::storage::IFile *file,
                     const Transaction_options &options = {})
      : m_dialect(dialect), m_file(file), m_options(options) {
    assert(Dialect::default_() == dialect);
    find_first_row_boundary_after =
        &Transaction_buffer::find_first_row_boundary_after_impl_dumper;
    find_last_row_boundary_before =
        &Transaction_buffer::find_last_row_boundary_before_impl_dumper;
  }

  Transaction_buffer(General_Tx_buffer, Dialect dialect,
                     mysqlshdk::storage::IFile *file,
                     uint64_t max_transaction_size)
      : m_dialect(dialect), m_file(file) {
    m_options.max_trx_size = max_transaction_size;
    if (m_dialect.fields_escaped_by.empty()) {
      find_first_row_boundary_after =
          &Transaction_buffer::find_first_row_boundary_after_impl_no_escape;
      find_last_row_boundary_before =
          &Transaction_buffer::find_last_row_boundary_before_impl_no_escape;
    } else {
      find_first_row_boundary_after =
          &Transaction_buffer::find_first_row_boundary_after_impl_escape;
      find_last_row_boundary_before =
          &Transaction_buffer::find_last_row_boundary_before_impl_escape;
    }
  }

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

  uint64_t find_first_row_boundary_after_impl_dumper() const;
  uint64_t find_last_row_boundary_before_impl_dumper(uint64_t limit);

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

  std::size_t m_offset_index = 0;  //< points to the last offset used
  uint64_t m_current_offset = 0;

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
  std::string filename;    //< Import data filename path
  std::unique_ptr<mysqlshdk::storage::IFile> filehandler = nullptr;
  size_t chunk_start = 0;   //< File chunk start offset
  size_t bytes_left = 0;    //< Bytes left to read from file
  bool range_read = false;  //< Reading whole file vs chunk range

  size_t bytes = 0;  //< bytes send to MySQL server
  std::atomic<size_t>
      *prog_bytes;  //< Pointer cumulative bytes send to MySQL Server
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
  volatile bool &m_interrupt;
  shcore::Synchronized_queue<File_import_info> *m_range_queue;
  std::vector<std::exception_ptr> &m_thread_exception;
  Stats &m_stats;
  std::string m_query_comment;
};

}  // namespace import_table
}  // namespace mysqlsh

#endif  // MODULES_UTIL_IMPORT_TABLE_LOAD_DATA_H_
