/*
 * Copyright (c) 2018, 2025, Oracle and/or its affiliates.
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

#include "modules/util/import_table/load_data.h"

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#if defined(_WIN32)
#include <io.h>
using off64_t = __int64;
using ssize_t = __int64;
#elif defined(__APPLE__)
#include <unistd.h>
using off64_t = off_t;
#else
#include <unistd.h>
#endif

#include <mysql.h>

#include <algorithm>
#include <cassert>
#include <cinttypes>
#include <functional>
#include <memory>
#include <type_traits>
#include <utility>

#include "modules/util/import_table/helpers.h"
#include "mysqlshdk/include/scripting/types.h"
#include "mysqlshdk/include/shellcore/console.h"
#include "mysqlshdk/include/shellcore/scoped_contexts.h"
#include "mysqlshdk/include/shellcore/shell_init.h"
#include "mysqlshdk/libs/rest/error.h"
#include "mysqlshdk/libs/storage/backend/in_memory/synchronized_file.h"
#include "mysqlshdk/libs/storage/backend/in_memory/virtual_file.h"
#include "mysqlshdk/libs/utils/utils_path.h"
#include "mysqlshdk/libs/utils/utils_string.h"

namespace mysqlsh {
namespace import_table {
namespace {
int local_infile_init_nop(void ** /* buffer */, const char *filename,
                          void * /* userdata */) noexcept {
  mysqlsh::current_console()->print_error(
      "Premature request for \"" + std::string(filename) +
      "\" local infile transfer. Rogue server?");
  return 1;
}

int local_infile_read_nop(void * /* userdata */, char * /* buffer */,
                          unsigned int /* length */) noexcept {
  return -1;
}

void local_infile_end_nop(void * /* userdata */) noexcept {}

int local_infile_error_nop(void * /* userdata */, char * /* error_msg */,
                           unsigned int /* error_msg_len */) noexcept {
  return CR_LOAD_DATA_LOCAL_INFILE_REJECTED;
}

}  // namespace

Transaction_buffer::Transaction_buffer(const Dialect &dialect,
                                       mysqlshdk::storage::IFile *file,
                                       uint64_t max_transaction_size,
                                       uint64_t skip_bytes)
    : m_dialect(dialect), m_file(file) {
  m_options.max_trx_size = max_transaction_size;
  m_options.skip_bytes = skip_bytes;

  if (m_dialect.lines_terminated_by.empty() ||
      m_dialect.fields_terminated_by == m_dialect.lines_terminated_by) {
    // we're unable to sub-chunk properly in this case
    m_options.max_trx_size = 0;
  } else if (m_dialect == Dialect::default_()) {
    find_first_row_boundary_after =
        &Transaction_buffer::find_first_row_boundary_after_impl_default;
    find_last_row_boundary_before =
        &Transaction_buffer::find_last_row_boundary_before_impl_default;
  } else if (m_dialect.fields_escaped_by.empty()) {
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

  if (!m_file->is_open()) {
    m_file->open(mysqlshdk::storage::Mode::READ);
  }

  // always skip bytes if requested to
  if (m_options.skip_bytes > 0) {
    try {
      m_file->seek(m_options.skip_bytes);
      m_options.skip_bytes = 0;
    } catch (const std::logic_error &) {
      static constexpr std::size_t length = 1024;
      char buffer[length];

      // seek() failed (i.e. it's not implemented), read beginning of the file
      while (m_options.skip_bytes > 0) {
        const auto bytes = m_file->read(
            buffer, std::min<std::size_t>(m_options.skip_bytes, length));

        if (bytes <= 0) {
          break;
        }

        m_options.skip_bytes -= bytes;
      }
    }
  }
}

void Transaction_buffer::before_query() {
  if (m_options.max_trx_size) {
    if (m_options.transaction_started) {
      m_options.transaction_started();
    }
  }
}

void Transaction_buffer::flush_done(bool *out_has_more_data) {
  if (m_options.max_trx_size && m_options.transaction_finished) {
    m_options.transaction_finished(m_trx_size);
  }

  m_trx_size = 0;
  m_trx_end_offset = 0;
  m_partial_row_sent = false;
  m_oversized_rows = 0;

  *out_has_more_data =
      m_options.max_trx_size > 0 && (!m_eof || !m_data.empty());
}

bool Transaction_buffer::flush_pending() const {
  return m_options.max_trx_size > 0 && m_trx_end_offset > 0 &&
         m_trx_end_offset <= m_trx_size;
}

int Transaction_buffer::consume(char *buffer, unsigned int length) {
  if (m_trx_end_offset > 0 && length > m_trx_end_offset - m_trx_size) {
    assert(m_trx_end_offset >= m_trx_size);
    length = m_trx_end_offset - m_trx_size;
  }

  if (length > 0) {
    if (m_data.length() > length) {
      memcpy(buffer, &m_data[0], length);

      memmove(&m_data[0], &m_data[length], m_data.length() - length);

      m_data.resize(m_data.length() - length);
    } else {
      length = m_data.length();
      memcpy(buffer, &m_data[0], length);
      m_data.clear();
    }

    m_trx_size += length;

    if (length > m_options.max_trx_size) {
      // in a single read, we got more bytes than the transaction limit, either
      // we have the whole row in the buffer and its end is past the limit or
      // end was not found in the buffer
      ++m_oversized_rows;

      if (m_on_oversized_row) {
        m_on_oversized_row(m_oversized_rows);
      }
    }
  }

  return length;
}

int Transaction_buffer::read(char *buffer, unsigned int length) {
  if (m_options.max_trx_size == 0) {
    // regular read if truncation is not enabled
    return m_file->read(buffer, length);
  }

  if (m_options.fast_sub_chunking) {
    return fast_sub_chunking(buffer, length);
  }

  const auto read_more = [this](unsigned int count) -> int64_t {
    int64_t bytes = 0;

    if (!m_eof) {
      auto end = m_data.size();
      m_data.resize(end + count);
      bytes = m_file->read(&m_data[end], count);
      if (bytes <= 0) {
        m_data.resize(end);
        if (bytes == 0) m_eof = true;
        return bytes;
      }

      m_data.resize(end + bytes);
    }
    return bytes;
  };

  // return as many bytes as we can from the data we have, as long as we know
  // it will fit in the transaction

retry:
  // 1 - read as much data as we can use (if any)
  if (m_trx_end_offset == 0) {
    const auto bytes = read_more(length);
    if (bytes < 0) return bytes;
  }

  // 2 - check if the data we've read so far would fill the transaction
  if (m_trx_end_offset == 0 && trx_bytes_left() > 0 &&
      static_cast<int64_t>(m_data.length()) >= trx_bytes_left()) {
    // calculate the last row that will fit
    auto last_row_end =
        (this->*find_last_row_boundary_before)(trx_bytes_left());
    if (last_row_end > 0) {
      set_trx_end_offset(last_row_end);
    }
  }

  // 3 - send data if we know it will fit or we have a row boundary
  if (m_trx_end_offset > 0) {
    // if we already know the last row that will fit in the transaction, we
    // can send a full buffer
    return consume(buffer, length);
  } else {
    if (static_cast<int64_t>(m_data.length()) >= trx_bytes_left()) {
      if (!m_partial_row_sent) {
        // we've already read more data than will fit in the transaction, so
        // just find the last row that will fit whole
        auto last_row_end =
            (this->*find_last_row_boundary_before)(trx_bytes_left());

        // if we don't find a newline here, it has to mean the row doesn't fit
        // in the transaction (otherwise there's a bug)
        if (last_row_end > 0) {
          set_trx_end_offset(last_row_end);
          return consume(buffer, length);
        }

        // end-of-row wasn't read yet, even tho we're already over the trx size
        // limit. If we've already sent whole rows, flush them.
        if (m_trx_size > 0) {
          // flush the rows that we've already sent earlier
          set_trx_end_offset(0);
          return 0;
        }
      }

      auto row_end = (this->*find_first_row_boundary_after)();
      if (row_end > 0) {
        // we found EOR, send the rest of the row and flush
        set_trx_end_offset(row_end);
        return consume(buffer, length);
      }

      assert(m_trx_size == 0 || m_partial_row_sent);
      // The first and only row of the transaction is oversized, so the only
      // thing left to do is to send the row data until its end and immediately
      // flush to see what happens.
      m_partial_row_sent = true;
      return consume(buffer, length);
    } else {
      // otherwise, send data at a row boundary to allow safe flushing
      auto last_row_end = (this->*find_last_row_boundary_before)(length);
      if (last_row_end > 0) {
        // EOR found, if we sent a partial row, it's complete now
        m_partial_row_sent = false;

        return consume(buffer, last_row_end);
      }

      // there's a full row in the buffer, but more data than we can return at
      // once
      last_row_end = (this->*find_last_row_boundary_before)(trx_bytes_left());
      if (last_row_end > 0) {
        return consume(buffer, std::min<unsigned int>(length, last_row_end));
      }

      // There are no full rows in the buffer, only the current partial row.
      if (m_trx_size == 0) {
        // If no other rows were sent yet, we can start sending this one
        m_partial_row_sent = true;
        return consume(buffer, length);
      }
      // If we're sending a partial row, then keep sending more since we didn't
      // hit EOR yet
      if (m_partial_row_sent) {
        return consume(buffer, length);
      }

      // If we're here, it means that we've already sent some complete rows
      // to the server, there's still space in the transaction for more but
      // we don't know yet if the partial row that's currently buffered will
      // fit or not.
      // The only thing left to do now is to keep reading more data until we
      // either find EOF, EOR or we find out for sure that the row won't fit.
      if (m_eof) {
        set_trx_end_offset(m_data.length());
        return consume(buffer, length);
      }

      // read some more and check everything again
      goto retry;
    }
  }
}

uint64_t Transaction_buffer::find_first_row_boundary_after_impl_default()
    const {
  assert(m_dialect == Dialect::default_());

  const char needle = m_dialect.lines_terminated_by[0];
  const auto p = m_data.find(needle);

  if (p >= m_data.length()) return 0;

  return p + 1;
}

uint64_t Transaction_buffer::find_last_row_boundary_before_impl_default(
    uint64_t limit) {
  assert(m_dialect == Dialect::default_());

  const char needle = m_dialect.lines_terminated_by[0];
  auto p = limit < m_data.length() ? static_cast<size_t>(limit - 1)
                                   : m_data.length();

  if (p == 0) return 0;

  p = m_data.rfind(needle, p);

  if (p >= m_data.length()) return 0;

  return p + 1;
}

uint64_t Transaction_buffer::find_first_row_boundary_after_impl_no_escape()
    const {
  assert(m_dialect.lines_terminated_by.size());
  assert(!m_dialect.fields_escaped_by.size());

  const auto &needle = m_dialect.lines_terminated_by;
  const auto p = m_data.find(needle);

  if (p >= m_data.length()) return 0;

  return p + needle.size();
}

uint64_t Transaction_buffer::find_last_row_boundary_before_impl_no_escape(
    uint64_t limit) {
  assert(m_dialect.lines_terminated_by.size());
  assert(!m_dialect.fields_escaped_by.size());

  const auto &needle = m_dialect.lines_terminated_by;
  auto p = limit < m_data.length() ? static_cast<size_t>(limit - 1)
                                   : m_data.length();

  if (p < needle.size()) return 0;

  p = m_data.rfind(needle, p);

  if (p >= m_data.length()) return 0;

  return p + needle.size();
}

uint64_t Transaction_buffer::find_first_row_boundary_after_impl_escape() const {
  assert(m_dialect.lines_terminated_by.size());
  assert(m_dialect.fields_escaped_by.size());

  const auto &needle = m_dialect.lines_terminated_by;
  auto p = m_data.find(needle);

  while (p != std::string::npos) {
    if (!(p > 0 && m_data[p - 1] == m_dialect.fields_escaped_by[0])) {
      assert(p < m_data.length());
      return p + needle.size();
    }

    p += needle.size();
    p = m_data.find(needle, p);
  }

  return 0;
}

uint64_t Transaction_buffer::find_last_row_boundary_before_impl_escape(
    uint64_t limit) {
  assert(m_dialect.lines_terminated_by.size());
  assert(m_dialect.fields_escaped_by.size());

  const auto &needle = m_dialect.lines_terminated_by;
  auto p = limit < m_data.length() ? static_cast<size_t>(limit - 1)
                                   : m_data.length();

  if (p < needle.size()) return 0;

  p = m_data.rfind(needle, p);

  while (p != std::string::npos) {
    if (!(p > 0 && m_data[p - 1] == m_dialect.fields_escaped_by[0])) {
      assert(p < m_data.length());
      return p + needle.size();
    }

    if (p < needle.size()) {
      break;
    }

    p -= needle.size();
    p = m_data.rfind(needle, p);
  }

  return 0;
}

int Transaction_buffer::fast_sub_chunking(char *buffer, unsigned int length) {
  if (!m_data.empty()) {
    // if data is stored locally, then a row didn't fit into an empty buffer,
    // consume it first
    return consume(buffer, length);
  }

  if (trx_bytes_left() <= 0) {
    // the row we had stored was longer than the transaction size, or we read
    // exactly that many bytes
    return 0;
  }

  auto bytes =
      m_file->read(buffer, std::min<uint64_t>(length, trx_bytes_left()));

  if (0 == bytes) {
    m_eof = true;
    return bytes;
  }

  if (bytes < 0) {
    // it was not possible to fit a single row into the buffer
    if (m_trx_size) {
      // we have some data in transaction, finish it, we'll see if this row fits
      // into an empty buffer
      return 0;
    } else {
      // row doesn't fit into an empty buffer, store it locally
      const auto virtual_file =
          dynamic_cast<mysqlshdk::storage::in_memory::Virtual_file *>(m_file);
      assert(virtual_file);

      if (!virtual_file) {
        throw std::logic_error(
            "Fast sub-chunking is only possible with virtual FS");
      }

      const auto handle =
          dynamic_cast<mysqlshdk::storage::in_memory::Synchronized_file *>(
              virtual_file->file());
      assert(handle);

      if (!handle) {
        throw std::logic_error(
            "Fast sub-chunking is only possible with synchronized I/O");
      }

      const auto row_length = handle->pending_write_size();

      m_data.resize(row_length);
      bytes = m_file->read(m_data.data(), row_length);

      // this read should succeed
      assert(static_cast<std::size_t>(bytes) == row_length);

      if (bytes <= 0) {
        return bytes;
      }

      return consume(buffer, length);
    }
  }

  m_trx_size += bytes;
  return bytes;
}

// ------

int local_infile_init(void **buffer, const char * /* filename */,
                      void *userdata) noexcept {
  assert(userdata);
  File_info *file_info = static_cast<File_info *>(userdata);
  file_info->compressed_file =
      dynamic_cast<mysqlshdk::storage::Compressed_file *>(
          file_info->filehandler.get());
  file_info->data_bytes = 0;
  file_info->file_bytes = 0;
  file_info->rate_limit = mysqlshdk::utils::Rate_limit(file_info->max_rate);
  *buffer = file_info;

  return 0;
}

int local_infile_read(void *userdata, char *buffer,
                      unsigned int length) noexcept {
  assert(userdata);
  File_info *file_info = static_cast<File_info *>(userdata);

  ssize_t bytes = 0;
  size_t file_bytes = 0;

  try {
    auto len = static_cast<size_t>(length);

    if (file_info->range_read) {
      len = std::min(len, file_info->bytes_left);
    }

    if (file_info->buffer.flush_pending()) {
      bytes = 0;
    } else {
      // read from the file until either EOF, or we read enough data to fill
      // the buffer or the transaction
      bytes = file_info->buffer.read(buffer, len);
      if (bytes < 0) return bytes;
      assert(static_cast<size_t>(bytes) <= len);

      if (file_info->compressed_file) {
        file_bytes = file_info->compressed_file->latest_io_size();
      } else {
        file_bytes = bytes;
      }
    }

    if (file_info->range_read) {
      file_info->bytes_left -= bytes;
    }
  } catch (...) {
    file_info->last_error = std::current_exception();
    return -1;
  }

  *(file_info->prog_data_bytes) += bytes;
  file_info->data_bytes += bytes;

  *(file_info->prog_file_bytes) += file_bytes;
  file_info->file_bytes += file_bytes;

  if (file_info->rate_limit.enabled()) {
    file_info->rate_limit.throttle(bytes);
  }

  if (file_info->user_interrupt->test()) {
    return -1;
  }

  return bytes;
}

void local_infile_end(void *userdata) noexcept {
  assert(userdata);
  const auto file_info = static_cast<File_info *>(userdata);

  if (file_info->on_infile_read_end) {
    file_info->on_infile_read_end();
  }
}

int local_infile_error(void *userdata, char *error_msg,
                       unsigned int error_msg_len) noexcept {
  assert(userdata);
  File_info *file_info = static_cast<File_info *>(userdata);

  if (file_info->last_error) {
    try {
      std::rethrow_exception(file_info->last_error);
    } catch (const std::exception &e) {
      snprintf(error_msg, error_msg_len, "%s", e.what());
    } catch (...) {
      snprintf(error_msg, error_msg_len, "Unknown exception during LOAD DATA");
    }
  } else if (file_info->user_interrupt->test()) {
    snprintf(error_msg, error_msg_len, "Interrupted");
  } else {
    snprintf(error_msg, error_msg_len, "Unknown error during LOAD DATA");
  }
  return CR_UNKNOWN_ERROR;
}

Load_data_worker::Load_data_worker(
    const Import_table_options &options, int64_t thread_id,
    Progress_stats *progress_stats, shcore::atomic_flag *interrupt,
    shcore::Synchronized_queue<File_import_info> *range_queue,
    std::exception_ptr *thread_exception, Load_stats *load_stats,
    const std::string &query_comment)
    : m_opt(options),
      m_thread_id(thread_id),
      m_progress_stats(progress_stats),
      m_interrupt(interrupt),
      m_range_queue(range_queue),
      m_thread_exception(thread_exception),
      m_load_stats(load_stats),
      m_query_comment(query_comment) {
  assert(m_progress_stats);
  assert(m_interrupt);
  assert(m_thread_exception);
  assert(m_load_stats);

  m_state = Thread_state::IDLE;
}

void Load_data_worker::operator()() {
  mysqlsh::Mysql_thread t;

  auto session = mysqlshdk::db::mysql::Session::create();

  // Prevent local infile rogue server attack. Safe local infile callbacks
  // must be set before connecting to the MySQL Server. Otherwise, rogue MySQL
  // Server can ask for arbitrary file from client.
  session->set_local_infile_userdata(nullptr);
  session->set_local_infile_init(local_infile_init_nop);
  session->set_local_infile_read(local_infile_read_nop);
  session->set_local_infile_end(local_infile_end_nop);
  session->set_local_infile_error(local_infile_error_nop);

  try {
    auto const conn_opts = m_opt.connection_options();
    session->connect(conn_opts);
  } catch (...) {
    handle_exception();
    return;
  }

  execute(session, nullptr);
}

void Load_data_worker::init_session(
    const std::shared_ptr<mysqlshdk::db::mysql::Session> &session,
    const Import_table_options &options) {
  const auto execute = [&session](const auto &sql) { session->execute(sql); };
  const auto executef = [&session](const auto &sql, auto &&...args) {
    session->executef(sql, std::forward<decltype(args)>(args)...);
  };

  // this sets the character_set_database and collation_database server
  // variables to the values the schema has
  executef("USE !;", options.schema());

  // SQL mode:
  //  - no_auto_value_on_zero - normally, 0 generates the next sequence
  //    number, use this mode to prevent this behaviour (solves problems if
  //    dump has 0 stored in an AUTO_INCREMENT column)
  execute("SET SQL_MODE = 'no_auto_value_on_zero';");

  // if user has specified the character set, set the session variables
  // related to the client connection
  if (!options.character_set().empty()) {
    executef("SET NAMES ?;", options.character_set());
  }

  // BUG#34173126, BUG#33360787 - loading when global auto-commit is OFF fails
  execute("SET autocommit = 1");
  // set session variables
  execute("SET unique_checks = 0");
  execute("SET foreign_key_checks = 0");
  execute("SET SESSION TRANSACTION ISOLATION LEVEL READ UNCOMMITTED");

  try {
    for (const auto &s : options.session_init_sql()) {
      log_info("Executing custom session init SQL: %s", s.c_str());
      execute(s);
    }
  } catch (const shcore::Error &e) {
    throw shcore::Exception::runtime_error(
        "Error while executing sessionInitSql: " + e.format());
  }
}

std::string Load_data_worker::load_data_body(
    const Import_table_options &options) {
  const std::string on_duplicate_rows = [&options]() {
    switch (options.duplicate_handling()) {
      case Duplicate_handling::Default:
        return "";

      case Duplicate_handling::Replace:
        return "REPLACE ";

      case Duplicate_handling::Ignore:
        return "IGNORE ";
    }

    throw std::logic_error("Unhandled Duplicate_handling value.");
  }();

  const std::string partition =
      options.partition().empty()
          ? ""
          : "PARTITION (" + shcore::quote_identifier(options.partition()) +
                ") ";

  const std::string character_set =
      options.character_set().empty()
          ? ""
          : "CHARACTER SET " +
                shcore::quote_sql_string(options.character_set()) + " ";

  const std::string compression = [&options]() {
    using mysqlshdk::storage::Compression;

    switch (options.compression()) {
      case Compression::NONE:
        return "";

      case Compression::ZSTD:
        return "COMPRESSION='ZSTD' ";

      case Compression::GZIP:
        throw std::logic_error("Unsupported LOAD DATA compression: gzip");
    }

    throw std::logic_error("Unhandled Compression value.");
  }();

  std::string query_body = on_duplicate_rows + "INTO TABLE " +
                           shcore::quote_identifier(options.schema()) + '.' +
                           shcore::quote_identifier(options.table()) + ' ' +
                           partition + character_set + compression +
                           options.dialect().build_sql();

  const auto &decode_columns = options.decode_columns();

  std::string query_columns;

  if (const auto &columns = options.columns(); !columns.empty()) {
    const auto placeholders =
        shcore::str_join(columns, ", ", [&decode_columns](const auto &c) {
          return std::visit(
              [&decode_columns](auto &&value) {
                using T = std::decay_t<decltype(value)>;

                if constexpr (std::is_same_v<T, uint64_t>) {
                  // user defined variable
                  return "@" + std::to_string(value);
                } else if constexpr (std::is_same_v<T, std::string>) {
                  // column name
                  std::string result;

                  if (decode_columns.count(value)) {
                    result += '@';
                  }

                  result += shcore::quote_identifier(value);

                  return result;
                } else {
                  assert(false);
                }
              },
              c);
        });

    query_columns = " (";
    query_columns += placeholders;
    query_columns += ')';
  }

  if (!decode_columns.empty()) {
    query_columns += " SET ";

    for (const auto &it : decode_columns) {
      query_columns += shcore::quote_identifier(it.first);
      query_columns += '=';

      if (it.second == "UNHEX" || it.second == "FROM_BASE64" ||
          it.second == "STRING_TO_VECTOR") {
        query_columns += it.second;
        query_columns += "(@";
        query_columns += shcore::quote_identifier(it.first);
      } else if (!it.second.empty()) {
        query_columns += '(';
        // Append "as is".
        query_columns += it.second;
      }

      query_columns += "),";
    }

    query_columns.pop_back();  // strip the last ,
  }

  if (options.skip_rows_count()) {
    query_body += " IGNORE ";
    query_body += std::to_string(options.skip_rows_count());
    query_body += " LINES";
  }

  query_body += query_columns;

  return query_body;
}

void Load_data_worker::execute(
    const std::shared_ptr<mysqlshdk::db::mysql::Session> &session,
    std::unique_ptr<mysqlshdk::storage::IFile> file,
    const Transaction_options &options) {
  const auto worker_name =
      shcore::str_format("[Worker%03" PRId64 "]: ", m_thread_id);

  try {
    File_info fi;
    fi.worker_id = m_thread_id;
    fi.prog_data_bytes = &m_progress_stats->data_bytes;
    fi.prog_file_bytes = &m_progress_stats->file_bytes;
    fi.user_interrupt = m_interrupt;
    fi.max_rate = m_opt.max_rate();
    fi.on_infile_read_end = [this]() { set_state(Thread_state::COMMITTING); };
    uint64_t max_trx_size = 0;
    const auto query = [&session](const auto &sql) {
      return session->query(sql);
    };

    init_session(session, m_opt);
    session->set_local_infile_userdata(static_cast<void *>(&fi));
    session->set_local_infile_init(local_infile_init);
    session->set_local_infile_read(local_infile_read);
    session->set_local_infile_end(local_infile_end);
    session->set_local_infile_error(local_infile_error);

    const auto query_body = load_data_body(m_opt);

    uint64_t subchunk = 0;
    while (true) {
      if (!fi.continuation) {
        // new file, reset the counter
        subchunk = 0;
      }

      ++subchunk;
      mysqlsh::import_table::File_import_info r;

      const auto format_error_message = [&worker_name, &fi, &r](
                                            const std::string &error,
                                            const std::string &extra = {}) {
        const auto task =
            fi.filehandler ? fi.filehandler->filename() + ": " : "";
        auto msg = worker_name + task + error;

        if (!r.context.empty()) {
          msg += r.context;
        }

        if (!extra.empty()) {
          msg += ": " + extra;
        }

        return msg;
      };

      try {
        if (m_range_queue) {
          if (!fi.continuation) {
            r = m_range_queue->pop();

            if (r.is_guard) {
              break;
            }

            fi.filehandler = std::move(r.file);
            fi.range_read = r.range_read;

            if (r.range_read) {
              fi.bytes_left = r.range.second - r.range.first;
              // maxBytesPerTransaction is ignored in case of a file imported in
              // chunks, because we assume that in this case bytesPerChunk ==
              // maxBytesPerTransaction
              max_trx_size = 0;
            } else {
              fi.bytes_left = 0;
              max_trx_size = m_opt.max_transaction_size();
            }

            fi.buffer =
                Transaction_buffer(m_opt.dialect(), fi.filehandler.get(),
                                   max_trx_size, r.range.first);
          }
        } else {
          if (file != nullptr) {
            fi.filehandler = std::move(file);
            file.reset(nullptr);
            fi.buffer = Transaction_buffer(m_opt.dialect(),
                                           fi.filehandler.get(), options);
          }
          fi.range_read = false;
          fi.bytes_left = 0;
        }
      } catch (const std::exception &e) {
        handle_exception();
        mysqlsh::current_console()->print_error(format_error_message(e.what()));
        throw std::exception(e);
      }

      const auto filename = fi.filehandler->filename();
      std::shared_ptr<mysqlshdk::db::IResult> load_result = nullptr;
      const std::string query_prefix = shcore::sqlformat(
          "LOAD DATA LOCAL INFILE ? ", fi.filehandler->full_path().masked());
      const std::string full_query = query_prefix + query_body;

#ifndef NDEBUG
      log_debug("%s %s %i", worker_name.c_str(), full_query.c_str(),
                m_range_queue != nullptr);
#endif

      fi.buffer.on_oversized_row([&format_error_message](uint64_t i) {
        // this is only printed once
        if (1 == i) {
          mysqlsh::current_console()->print_warning(format_error_message(
              "Attempting to load a row longer than maxBytesPerTransaction."));
        }
      });

      try {
        set_state(Thread_state::READING);
        fi.buffer.before_query();
        load_result = query(m_query_comment + full_query);
        set_state(Thread_state::IDLE);
        fi.buffer.flush_done(&fi.continuation);
        m_load_stats->data_bytes += fi.data_bytes;
        m_load_stats->file_bytes += fi.file_bytes;

        if (!fi.continuation) {
          // increase the counter only when there are no more subchunks
          ++m_load_stats->files_processed;
        }
      } catch (const mysqlshdk::db::Error &e) {
        handle_exception();
        const auto error_msg = format_error_message(e.format(), full_query);
        mysqlsh::current_console()->print_error(error_msg);

        if (fi.buffer.oversized_rows()) {
          mysqlsh::current_console()->print_note(format_error_message(
              "This error has been reported for a sub-chunk which has at least "
              "one row longer than maxBytesPerTransaction (" +
              std::to_string(options.max_trx_size) + " bytes)."));
        }

        throw std::runtime_error(error_msg);
      } catch (const mysqlshdk::rest::Connection_error &e) {
        handle_exception();
        const auto error_msg = format_error_message(e.what());
        mysqlsh::current_console()->print_error(error_msg);
        throw std::runtime_error(error_msg);
      } catch (const std::exception &e) {
        handle_exception();
        const auto error_msg = format_error_message(e.what());
        mysqlsh::current_console()->print_error(error_msg);
        throw std::exception(e);
      }

      const auto warnings_num =
          load_result ? load_result->get_warning_count() : 0;

      {
        const char *mysql_info = session->get_mysql_info();
        const auto status =
            worker_name + filename + ": " +
            (mysql_info ? mysql_info : "ERROR") +
            ((options.max_trx_size == 0 && max_trx_size == 0)
                 ? ""
                 : (fi.continuation
                        ? " - flushed sub-chunk " + std::to_string(subchunk)
                        : " - loading finished in " + std::to_string(subchunk) +
                              " sub-chunks"));

        if (m_opt.verbose()) {
          mysqlsh::current_console()->print_info(status);
        } else {
          log_info("%s", status.c_str());
        }

        if (mysql_info) {
          size_t records = 0;
          size_t deleted = 0;
          size_t skipped = 0;
          size_t warnings = 0;

          sscanf(mysql_info,
                 "Records: %zu  Deleted: %zu  Skipped: %zu  Warnings: %zu\n",
                 &records, &deleted, &skipped, &warnings);
          m_load_stats->records += records;
          m_load_stats->deleted += deleted;
          m_load_stats->skipped += skipped;
          m_load_stats->warnings += warnings;
        }

        if (warnings_num > 0) {
          std::size_t warnings_count = 0;
          const auto console = mysqlsh::current_console();
          auto print_callback = &IConsole::print_error;

          while (const auto w = load_result->fetch_one_warning()) {
            const std::string msg = shcore::str_format(
                "LOAD DATA error for `%s`.`%s` in file '%s', error %" PRIu32
                ": %s",
                m_opt.schema().c_str(), m_opt.table().c_str(), filename.c_str(),
                w->code, w->msg.c_str());

            if (warnings_count++ < m_warnings_to_show) {
              // show first k warnings
              switch (w->level) {
                case mysqlshdk::db::Warning::Level::Error:
                  print_callback = &IConsole::print_error;
                  break;

                case mysqlshdk::db::Warning::Level::Warn:
                  print_callback = &IConsole::print_warning;
                  break;

                case mysqlshdk::db::Warning::Level::Note:
                  print_callback = &IConsole::print_note;
                  break;
              }

              auto load_warning = shcore::make_dict();

              load_warning->emplace("schema", m_opt.schema());
              load_warning->emplace("table", m_opt.table());
              load_warning->emplace("filename", filename);
              load_warning->emplace("level",
                                    mysqlshdk::db::to_string(w->level));
              load_warning->emplace("code", w->code);
              load_warning->emplace("message", w->msg);

              std::invoke(
                  print_callback, console, msg,
                  shcore::make_dict("loadWarning", std::move(load_warning)));
            } else {
              // log remaining warnings
              switch (w->level) {
                case mysqlshdk::db::Warning::Level::Error:
                  log_error("%s", msg.c_str());
                  break;

                case mysqlshdk::db::Warning::Level::Warn:
                  log_warning("%s", msg.c_str());
                  break;

                case mysqlshdk::db::Warning::Level::Note:
                  log_info("%s", msg.c_str());
                  break;
              }
            }
          }

          if (warnings_count > m_warnings_to_show) {
            const auto remaining_warnings_count =
                warnings_count - m_warnings_to_show;

            mysqlsh::current_console()->print_info(
                "Check mysqlsh.log for " +
                std::to_string(remaining_warnings_count) + " more warning" +
                (remaining_warnings_count == 1 ? "" : "s") + ".");
          }
        }
      }

      if (!m_range_queue && !fi.continuation) break;
    }
  } catch (const std::exception &e) {
    if (!*m_thread_exception) {
      mysqlsh::current_console()->print_error(worker_name + e.what());
      handle_exception();
    }
  } catch (...) {
    handle_exception();
  }
}

void Load_data_worker::handle_exception() {
  set_state(Thread_state::ERROR);
  *m_thread_exception = std::current_exception();
}

void Load_data_worker::set_state(Thread_state new_state) {
  if (m_state != new_state) {
    --m_progress_stats->thread_states[m_state];
    m_state = new_state;
    ++m_progress_stats->thread_states[m_state];
  }
}

}  // namespace import_table
}  // namespace mysqlsh
