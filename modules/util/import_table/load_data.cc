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

#include "modules/util/import_table/load_data.h"

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#if defined(_WIN32)
#include <io.h>
#define lseek64 _lseeki64
#define off64_t __int64
#define read _read
#elif defined(__APPLE__)
#include <unistd.h>
#define lseek64 lseek
#define off64_t off_t
#else
#include <unistd.h>
#endif

#include <mysql.h>
#include <algorithm>
#include <memory>
#include "modules/util/import_table/helpers.h"
#include "mysqlshdk/include/shellcore/console.h"
#include "mysqlshdk/include/shellcore/shell_init.h"
#include "mysqlshdk/libs/db/mysql/session.h"
#include "mysqlshdk/libs/utils/utils_string.h"

namespace mysqlsh {
namespace import_table {

int local_infile_init(void **buffer, const char *filename, void *userdata) {
  File_info *file_info = static_cast<File_info *>(userdata);
  file_info->fd = detail::file_open(file_info->filename);

  if (file_info->fd < 0) {
    mysqlsh::current_console()->print_error("Cannot open file '" +
                                            file_info->filename + "'\n");
    return 1;
  }

  off64_t offset = lseek64(file_info->fd, file_info->chunk_start, SEEK_SET);
  if (offset == static_cast<off64_t>(-1)) {
    return 1;
  }

  *buffer = file_info;

  file_info->rate_limit = mysqlshdk::utils::Rate_limit(file_info->max_rate);
  return 0;
}

int local_infile_read(void *userdata, char *buffer, unsigned int length) {
  File_info *file_info = static_cast<File_info *>(userdata);

  size_t len = std::min({static_cast<size_t>(length), file_info->bytes_left});

#ifdef _WIN32
  int bytes = _read(file_info->fd, buffer, len);
#else
  ssize_t bytes = read(file_info->fd, buffer, len);
#endif
  if (bytes == -1) return bytes;

  file_info->bytes_left -= bytes;
  *(file_info->prog_bytes) += bytes;

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

  detail::file_close(file_info->fd);
}

int local_infile_error(void *userdata, char *error_msg,
                       unsigned int error_msg_len) {
  File_info *file_info = static_cast<File_info *>(userdata);
  if (*file_info->user_interrupt) {
    snprintf(error_msg, error_msg_len, "Interrupted by user.");
  } else {
    snprintf(error_msg, error_msg_len, "Unknown error during DATA LOAD.");
  }
  return CR_UNKNOWN_ERROR;
}

Load_data_worker::Load_data_worker(
    const Import_table_options &options, int64_t thread_id,
    mysqlshdk::textui::IProgress *progress,
    std::atomic<size_t> *prog_sent_bytes, std::mutex *output_mutex,
    volatile bool *interrupt, shcore::Synchronized_queue<Range> *range_queue,
    std::vector<std::exception_ptr> *thread_exception, bool use_json,
    Stats *stats)
    : m_opt(options),
      m_thread_id(thread_id),
      m_progress(progress),
      m_prog_sent_bytes(*prog_sent_bytes),
      m_output_mutex(*output_mutex),
      m_interrupt(*interrupt),
      m_range_queue(*range_queue),
      m_thread_exception(*thread_exception),
      m_use_json(use_json),
      m_stats(*stats) {}

void Load_data_worker::operator()() {
  try {
    std::shared_ptr<mysqlshdk::db::mysql::Session> session =
        mysqlshdk::db::mysql::Session::create();

    mysqlsh::Mysql_thread t;
    auto const conn_opts = m_opt.connection_options();

    File_info fi;
    fi.filename = m_opt.full_path();
    fi.worker_id = m_thread_id;
    fi.prog = m_opt.show_progress() ? m_progress : nullptr;
    fi.prog_bytes = &m_prog_sent_bytes;
    fi.prog_mutex = &m_output_mutex;
    fi.user_interrupt = &m_interrupt;
    fi.max_rate = m_opt.max_rate();

    session->set_local_infile_userdata(static_cast<void *>(&fi));
    session->set_local_infile_init(local_infile_init);
    session->set_local_infile_read(local_infile_read);
    session->set_local_infile_end(local_infile_end);
    session->set_local_infile_error(local_infile_error);

    session->connect(conn_opts);

    // set session variables
    session->execute("SET unique_checks = 0");
    session->execute("SET foreign_key_checks = 0");
    session->execute(
        "SET SESSION TRANSACTION ISOLATION LEVEL READ UNCOMMITTED");

    const std::string on_duplicate_rows =
        m_opt.replace_duplicates() ? std::string{"REPLACE "} : std::string{};

    std::string query_template = "LOAD DATA LOCAL INFILE ? " +
                                 on_duplicate_rows + "INTO TABLE !.! " +
                                 m_opt.dialect().build_sql();

    auto columns = m_opt.columns();
    if (!columns.empty()) {
      const std::vector<std::string> x(columns.size(), "!");
      const auto placeholders = shcore::str_join(x, ", ");
      query_template += " (" + placeholders + ")";
    }

    shcore::sqlstring sql(query_template, 0);
    sql << fi.filename << m_opt.schema() << m_opt.table();
    for (const auto &col : columns) {
      sql << col;
    }

    char worker_name[64];
    snprintf(worker_name, sizeof(worker_name), "[Worker%03u] ",
             static_cast<unsigned int>(m_thread_id));

    while (true) {
      const auto r = m_range_queue.pop();

      if (r.begin == 0 && r.end == 0) {
        break;
      }

      fi.chunk_start = r.begin;
      fi.bytes_left = r.end - r.begin;

      std::shared_ptr<mysqlshdk::db::IResult> load_result = nullptr;

      try {
        load_result = session->query(sql);
      } catch (const mysqlshdk::db::Error &e) {
        m_thread_exception[m_thread_id] = std::current_exception();
        std::lock_guard<std::mutex> lock(*(fi.prog_mutex));
        m_progress->clear_status();
        const std::string error_msg{
            worker_name + m_opt.schema() + "." + m_opt.table() + ": " +
            e.format() + " @ file bytes range [" + std::to_string(r.begin) +
            ", " + std::to_string(r.end) + ")"};
        mysqlsh::current_console()->print_error(error_msg);
        m_progress->show_status(!m_use_json);
        throw std::runtime_error(error_msg);
      }

      const auto warnings_num =
          load_result ? load_result->get_warning_count() : 0;

      {
        const char *mysql_info = session->get_mysql_info();
        std::lock_guard<std::mutex> lock(*(fi.prog_mutex));
        m_progress->clear_status();
        const std::string msg = worker_name + m_opt.schema() + "." +
                                m_opt.table() + ": " +
                                (mysql_info ? mysql_info : "ERROR");
        mysqlsh::current_console()->print_info(msg);

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
            const std::string msg = "`" + m_opt.schema() + "`.`" +
                                    m_opt.table() + "` error " +
                                    std::to_string(w->code) + ": " + w->msg;

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
            const std::string msg = "`" + m_opt.schema() + "`.`" +
                                    m_opt.table() + "` error " +
                                    std::to_string(w->code) + ": " + w->msg;

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
            mysqlsh::current_console()->println(
                "Check mysqlsh.log for " +
                std::to_string(remaining_warnings_count) + " more warning" +
                (remaining_warnings_count == 1 ? "" : "s") + ".");
          }
        }
        m_progress->show_status(!m_use_json);
      }
    }
  } catch (...) {
    m_thread_exception[m_thread_id] = std::current_exception();
  }
}

}  // namespace import_table
}  // namespace mysqlsh
