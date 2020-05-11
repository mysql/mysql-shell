/*
 * Copyright (c) 2018, 2020, Oracle and/or its affiliates. All rights reserved.
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
#include <memory>
#include <utility>
#include "modules/util/import_table/helpers.h"
#include "mysqlshdk/include/shellcore/console.h"
#include "mysqlshdk/include/shellcore/shell_init.h"
#include "mysqlshdk/libs/rest/error.h"
#include "mysqlshdk/libs/storage/compressed_file.h"
#include "mysqlshdk/libs/utils/utils_path.h"
#include "mysqlshdk/libs/utils/utils_string.h"

namespace mysqlsh {
namespace import_table {
namespace {
int local_infile_init_nop(void ** /* buffer */, const char *filename,
                          void * /* userdata */) {
  mysqlsh::current_console()->print_error(
      "Premature request for \"" + std::string(filename) +
      "\" local infile transfer. Rogue server?");
  return 1;
}

int local_infile_read_nop(void * /* userdata */, char * /* buffer */,
                          unsigned int /* length */) {
  return -1;
}

void local_infile_end_nop(void * /* userdata */) {}

int local_infile_error_nop(void * /* userdata */, char * /* error_msg */,
                           unsigned int /* error_msg_len */) {
  return CR_LOAD_DATA_LOCAL_INFILE_REJECTED;
}
}  // namespace

int local_infile_init(void **buffer, const char * /* filename */,
                      void *userdata) {
  File_info *file_info = static_cast<File_info *>(userdata);
  // todo(kg): we can get rid of file open and close (in local_infile_end()).
  //           We can open it when constructing File_info object.
  try {
    file_info->filehandler->open(mysqlshdk::storage::Mode::READ);
  } catch (const std::runtime_error &ex) {
    mysqlsh::current_console()->print_error(ex.what());
    return 1;
  }

  if (file_info->range_read) {
    off64_t offset = file_info->filehandler->seek(file_info->chunk_start);
    if (offset == static_cast<off64_t>(-1)) {
      return 1;
    }
  }

  *buffer = file_info;

  file_info->rate_limit = mysqlshdk::utils::Rate_limit(file_info->max_rate);
  return 0;
}

int local_infile_read(void *userdata, char *buffer, unsigned int length) {
  File_info *file_info = static_cast<File_info *>(userdata);

  ssize_t bytes;
  if (file_info->range_read) {
    size_t len = std::min({static_cast<size_t>(length), file_info->bytes_left});

    bytes = file_info->filehandler->read(buffer, len);
    if (bytes == -1) return bytes;

    file_info->bytes_left -= bytes;
  } else {
    bytes = file_info->filehandler->read(buffer, length);
    if (bytes == -1) return bytes;
  }

  *(file_info->prog_bytes) += bytes;
  file_info->bytes += bytes;

  if (file_info->rate_limit.enabled()) {
    file_info->rate_limit.throttle(bytes);
  }

  if (*file_info->user_interrupt) {
    return -1;
  }

  if (file_info->prog) {
    std::unique_lock<std::mutex> lock(*(file_info->prog_mutex),
                                      std::try_to_lock);
    if (lock.owns_lock()) {
      file_info->prog->current(*(file_info->prog_bytes));
      file_info->prog->show_status();
    }
  }

  return bytes;
}

void local_infile_end(void *userdata) {
  File_info *file_info = static_cast<File_info *>(userdata);

  if (file_info->prog) {
    std::unique_lock<std::mutex> lock(*(file_info->prog_mutex),
                                      std::try_to_lock);
    if (lock.owns_lock()) {
      file_info->prog->current(*(file_info->prog_bytes));
      file_info->prog->show_status();
    }
  }

  file_info->filehandler->close();
}

int local_infile_error(void *userdata, char *error_msg,
                       unsigned int error_msg_len) {
  File_info *file_info = static_cast<File_info *>(userdata);
  if (*file_info->user_interrupt) {
    snprintf(error_msg, error_msg_len, "Interrupted");
  } else {
    snprintf(error_msg, error_msg_len, "Unknown error during DATA LOAD");
  }
  return CR_UNKNOWN_ERROR;
}

Load_data_worker::Load_data_worker(
    const Import_table_options &options, int64_t thread_id,
    mysqlshdk::textui::IProgress *progress, std::mutex *output_mutex,
    std::atomic<size_t> *prog_sent_bytes, volatile bool *interrupt,
    shcore::Synchronized_queue<Range> *range_queue,
    std::vector<std::exception_ptr> *thread_exception, Stats *stats)
    : m_opt(options),
      m_thread_id(thread_id),
      m_progress(progress),
      m_prog_sent_bytes(*prog_sent_bytes),
      m_output_mutex(*output_mutex),
      m_interrupt(*interrupt),
      m_range_queue(range_queue),
      m_thread_exception(*thread_exception),
      m_stats(*stats) {}

void Load_data_worker::operator()() {
  mysqlsh::Mysql_thread t;

  auto session = mysqlshdk::db::mysql::Session::create();

  // Prevent local infile rogue server attack. Safe local infile callbacks must
  // be set before connecting to the MySQL Server. Otherwise, rogue MySQL Server
  // can ask for arbitrary file from client.
  session->set_local_infile_userdata(nullptr);
  session->set_local_infile_init(local_infile_init_nop);
  session->set_local_infile_read(local_infile_read_nop);
  session->set_local_infile_end(local_infile_end_nop);
  session->set_local_infile_error(local_infile_error_nop);

  try {
    auto const conn_opts = m_opt.connection_options();
    session->connect(conn_opts);
  } catch (...) {
    m_thread_exception[m_thread_id] = std::current_exception();
    return;
  }

  execute(session, m_opt.create_file_handle());
}

void Load_data_worker::execute(
    const std::shared_ptr<mysqlshdk::db::mysql::Session> &session,
    std::unique_ptr<mysqlshdk::storage::IFile> file) {
  mysqlshdk::storage::IFile *raw_file = file.get();

  mysqlshdk::storage::Compression compr;
  try {
    compr = mysqlshdk::storage::from_extension(
        std::get<1>(shcore::path::split_extension(file->filename())));
  } catch (...) {
    compr = mysqlshdk::storage::Compression::NONE;
  }

  file = mysqlshdk::storage::make_file(std::move(file), compr);
  std::string task = file->filename();

  try {
    File_info fi;
    fi.filename = file->full_path();
    fi.filehandler = std::move(file);
    fi.raw_filehandler = raw_file;
    fi.worker_id = m_thread_id;
    fi.prog = m_opt.show_progress() ? m_progress : nullptr;
    fi.prog_bytes = &m_prog_sent_bytes;
    fi.prog_mutex = &m_output_mutex;
    fi.user_interrupt = &m_interrupt;
    fi.max_rate = m_opt.max_rate();
    fi.range_read = m_range_queue ? true : false;

    // set session variables
    session->execute("SET unique_checks = 0");
    session->execute("SET foreign_key_checks = 0");
    session->execute(
        "SET SESSION TRANSACTION ISOLATION LEVEL READ UNCOMMITTED");

    session->set_local_infile_userdata(static_cast<void *>(&fi));
    session->set_local_infile_init(local_infile_init);
    session->set_local_infile_read(local_infile_read);
    session->set_local_infile_end(local_infile_end);
    session->set_local_infile_error(local_infile_error);

    const std::string on_duplicate_rows =
        m_opt.replace_duplicates() ? std::string{"REPLACE "} : std::string{};

    const std::string character_set =
        m_opt.character_set().empty()
            ? ""
            : "CHARACTER SET " +
                  shcore::quote_sql_string(m_opt.character_set()) + " ";

    std::string query_template = "LOAD DATA LOCAL INFILE ? " +
                                 on_duplicate_rows + "INTO TABLE !.! " +
                                 character_set + m_opt.dialect().build_sql();

    const auto &decode_columns = m_opt.decode_columns();

    const auto &columns = m_opt.columns();
    if (!columns.empty()) {
      const std::vector<std::string> x(columns.size(), "!");
      const auto placeholders = shcore::str_join(
          columns, ", ",
          [&decode_columns](const std::string &c) -> std::string {
            if (decode_columns.find(c) != decode_columns.end())
              return "@!";
            else
              return "!";
          });
      query_template += " (" + placeholders + ")";
    }

    if (!decode_columns.empty()) {
      query_template += " SET ";
      for (const auto &it : decode_columns) {
        assert(it.second == "UNHEX" || it.second == "FROM_BASE64" ||
               it.second.empty());

        if (!it.second.empty()) query_template += "! = " + it.second + "(@!),";
      }
      query_template.pop_back();  // strip the last ,
    }

    shcore::sqlstring sql(query_template, 0);
    sql << fi.filename << m_opt.schema() << m_opt.table();
    for (const auto &col : columns) {
      sql << col;
    }
    for (const auto &it : decode_columns) {
      if (!it.second.empty()) sql << it.first << it.first;
    }
    sql.done();

    char worker_name[64];
    snprintf(worker_name, sizeof(worker_name), "[Worker%03u] ",
             static_cast<unsigned int>(m_thread_id));

#ifndef NDEBUG
    log_debug("%s %s %i", worker_name, sql.str().c_str(),
              m_range_queue != nullptr);
#endif

    while (true) {
      mysqlsh::import_table::Range r;

      if (m_range_queue) {
        r = m_range_queue->pop();

        if (r.begin == 0 && r.end == 0) {
          break;
        }

        fi.chunk_start = r.begin;
        fi.bytes_left = r.end - r.begin;
      } else {
        r = {0, 0};
        fi.chunk_start = 0;
        fi.bytes_left = 0;
      }

      std::shared_ptr<mysqlshdk::db::IResult> load_result = nullptr;

      try {
        load_result = session->query(sql);

        m_stats.total_bytes = fi.bytes;
      } catch (const mysqlshdk::db::Error &e) {
        m_thread_exception[m_thread_id] = std::current_exception();
        const std::string error_msg{
            worker_name + task + ": " + e.format() +
            (fi.range_read ? " @ file bytes range [" + std::to_string(r.begin) +
                                 ", " + std::to_string(r.end) + "): "
                           : ": ") +
            sql.str()};
        mysqlsh::current_console()->print_error(error_msg);
        throw std::runtime_error(error_msg);
      } catch (const mysqlshdk::rest::Connection_error &e) {
        m_thread_exception[m_thread_id] = std::current_exception();
        const std::string error_msg{
            worker_name + task + ": " + e.what() +
            (fi.range_read ? " @ file bytes range [" + std::to_string(r.begin) +
                                 ", " + std::to_string(r.end) + ")"
                           : "")};
        mysqlsh::current_console()->print_error(error_msg);
        throw std::runtime_error(error_msg);
      } catch (const std::exception &e) {
        m_thread_exception[m_thread_id] = std::current_exception();
        const std::string error_msg{
            worker_name + task + ": " + e.what() +
            (fi.range_read ? " @ file bytes range [" + std::to_string(r.begin) +
                                 ", " + std::to_string(r.end) + ")"
                           : "")};
        mysqlsh::current_console()->print_error(error_msg);
        throw std::exception(e);
      }

      const auto warnings_num =
          load_result ? load_result->get_warning_count() : 0;

      {
        const char *mysql_info = session->get_mysql_info();
        mysqlsh::current_console()->print_info(
            worker_name + task + ": " + (mysql_info ? mysql_info : "ERROR"));

        if (mysql_info) {
          size_t records = 0;
          size_t deleted = 0;
          size_t skipped = 0;
          size_t warnings = 0;

          sscanf(mysql_info,
                 "Records: %zu  Deleted: %zu  Skipped: %zu  Warnings: %zu\n",
                 &records, &deleted, &skipped, &warnings);
          m_stats.total_records += records;
          m_stats.total_deleted += deleted;
          m_stats.total_skipped += skipped;
          m_stats.total_warnings += warnings;
        }

        if (warnings_num > 0) {
          // show first k warnings, where k = warnings_to_show
          constexpr int warnings_to_show = 5;
          auto w = load_result->fetch_one_warning();

          for (int i = 0; w && i < warnings_to_show;
               w = load_result->fetch_one_warning(), i++) {
            const std::string msg =
                task + " error " + std::to_string(w->code) + ": " + w->msg;

            switch (w->level) {
              case mysqlshdk::db::Warning::Level::Error:
                mysqlsh::current_console()->print_error(msg);
                break;
              case mysqlshdk::db::Warning::Level::Warn:
                mysqlsh::current_console()->print_warning(msg);
                break;
              case mysqlshdk::db::Warning::Level::Note:
                mysqlsh::current_console()->print_note(msg);
                break;
            }
          }

          // log remaining warnings
          size_t remaining_warnings_count = 0;
          for (; w; w = load_result->fetch_one_warning()) {
            remaining_warnings_count++;
            const std::string msg =
                task + " error " + std::to_string(w->code) + ": " + w->msg;

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

          if (remaining_warnings_count > 0) {
            mysqlsh::current_console()->print_info(
                "Check mysqlsh.log for " +
                std::to_string(remaining_warnings_count) + " more warning" +
                (remaining_warnings_count == 1 ? "" : "s") + ".");
          }
        }
      }

      if (!m_range_queue) break;
    }
  } catch (...) {
    m_thread_exception[m_thread_id] = std::current_exception();
  }
}

}  // namespace import_table
}  // namespace mysqlsh
