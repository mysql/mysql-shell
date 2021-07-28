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

#include "modules/util/import_table/import_table.h"

#include <algorithm>
#include <limits>
#include <utility>

#include "modules/util/dump/console_with_progress.h"
#include "modules/util/import_table/chunk_file.h"
#include "modules/util/import_table/load_data.h"
#include "mysqlshdk/include/shellcore/shell_options.h"
#include "mysqlshdk/libs/storage/idirectory.h"
#include "mysqlshdk/libs/textui/text_progress.h"
#include "mysqlshdk/libs/utils/natural_compare.h"
#include "mysqlshdk/libs/utils/strformat.h"
#include "mysqlshdk/libs/utils/utils_file.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_path.h"

namespace mysqlsh {
namespace import_table {

Import_table::Import_table(const Import_table_options &options)
    : m_console(std::make_shared<dump::Console_with_progress>(m_progress,
                                                              &m_output_mutex)),
      m_opt(options),
      m_interrupt(nullptr) {
  m_thread_exception.resize(options.threads_size(), nullptr);

  m_use_json = (mysqlsh::current_shell_options()->get().wrap_json != "off");

  if (m_opt.show_progress()) {
    if (m_use_json) {
      m_progress = std::make_unique<mysqlshdk::textui::Json_progress>();
    } else {
      m_progress = std::make_unique<mysqlshdk::textui::Text_progress>();
    }
  } else {
    m_progress = std::make_unique<mysqlshdk::textui::IProgress>();
  }

  m_progress->total(m_opt.file_size());
}

Import_table::~Import_table() {
  m_range_queue.shutdown(m_opt.threads_size());
  join_workers();
}

void Import_table::join_workers() {
  for (auto &t : m_threads) {
    if (t.joinable()) {
      t.join();
    }
  }
}

void Import_table::rethrow_exceptions() {
  for (const auto &exc : m_thread_exception) {
    if (exc) {
      std::rethrow_exception(exc);
    }
  }

  // Non critical errors appear during import. Throwing an error sets required
  // non-zero exit code.
  if (!noncritical_errors.empty()) {
    throw std::runtime_error(noncritical_errors[0]);
  }
}

bool Import_table::any_exception() {
  return std::any_of(m_thread_exception.begin(), m_thread_exception.end(),
                     [](std::exception_ptr p) -> bool { return p != nullptr; });
}

void Import_table::progress_shutdown() {
  m_progress->current(m_prog_sent_bytes);
  m_progress->show_status(true);
  m_progress->shutdown();
}

void Import_table::spawn_workers() {
  const int64_t num_workers = m_opt.threads_size();
  for (int64_t i = 0; i < num_workers; i++) {
    Load_data_worker worker(m_opt, i, m_progress.get(), &m_output_mutex,
                            &m_prog_sent_bytes, m_interrupt, &m_range_queue,
                            &m_thread_exception, &m_stats);
    std::thread t = mysqlsh::spawn_scoped_thread(&Load_data_worker::operator(),
                                                 std::move(worker));
    m_threads.emplace_back(std::move(t));
  }
}

void Import_table::chunk_file() {
  Chunk_file chunk;
  chunk.set_chunk_size(m_opt.bytes_per_chunk());
  chunk.set_file_handle(m_opt.file_handle());
  chunk.set_dialect(m_opt.dialect());
  chunk.set_rows_to_skip(m_opt.skip_rows_count());
  chunk.set_output_queue(&m_range_queue);
  chunk.start();

  m_range_queue.shutdown(m_opt.threads_size());
}

void Import_table::build_queue() {
  size_t total_bytes = 0;
  for (const auto &glob_item : m_opt.filelist_from_user()) {
    const auto &oci_opts = m_opt.get_oci_options();
    if (glob_item.find('*') != std::string::npos ||
        glob_item.find('?') != std::string::npos) {
      auto glob_fh = mysqlshdk::storage::make_file(glob_item, oci_opts);
      auto glob_full_path = glob_fh->full_path();
      auto dir = mysqlshdk::storage::make_directory(
          glob_fh->parent()->full_path().real(), oci_opts);
      if (!dir->exists()) {
        std::string errmsg{"Directory " + dir->full_path().masked() +
                           " does not exists."};
        m_console.get()->print_error(errmsg);
        noncritical_errors.emplace_back(std::move(errmsg));
        continue;
      }
      auto list_files = dir->filter_files(shcore::path::basename(glob_item));
      std::sort(list_files.begin(), list_files.end(),
                [](const auto &lhs, const auto &rhs) {
                  return shcore::natural_compare(
                      lhs.name.begin(), lhs.name.end(), rhs.name.begin(),
                      rhs.name.end());
                });

      for (const auto &file_info : list_files) {
        auto fh = m_opt.create_file_handle(dir->file(file_info.name));
        File_import_info task;
        task.file_path = fh->full_path().real();
        task.file_size = file_info.size;
        task.file_handler = fh.release();
        // todo(kg): impl. content size method. Extract content size information
        // from .gz and .zst headers
        // task.content_size = fh.size();
        task.range_read = false;
        task.is_guard = false;
        total_bytes += task.file_size.get_safe();
        m_progress->total(total_bytes);
        m_range_queue.push(std::move(task));
      }
    } else {
      auto glob_fh = mysqlshdk::storage::make_file(glob_item, oci_opts);
      if (!glob_fh->exists()) {
        std::string errmsg{"File " + glob_fh->full_path().masked() +
                           " does not exists."};
        m_console.get()->print_error(errmsg);
        noncritical_errors.emplace_back(std::move(errmsg));
        continue;
      }
      File_import_info task;
      task.file_path = glob_item;
      task.file_size = glob_fh->file_size();
      // task.content_size = fh.size();
      task.range_read = false;
      task.is_guard = false;

      total_bytes += task.file_size.get_safe();
      m_progress->total(total_bytes);
      m_range_queue.push(std::move(task));
    }
  }
  m_range_queue.shutdown(m_opt.threads_size());
}

void Import_table::import() {
  m_timer.stage_begin("Parallel load data");
  spawn_workers();

  if (m_opt.is_multifile()) {
    build_queue();
  } else {
    const std::string &path = m_opt.filelist_from_user()[0];
    const auto extension = std::get<1>(shcore::path::split_extension(path));
    if (extension == ".gz" || extension == ".zst") {
      // cannot chunk compressed files
      build_queue();
    } else {
      chunk_file();
    }
  }

  join_workers();
  m_timer.stage_end();
  progress_shutdown();
}

std::string Import_table::import_summary() const {
  using mysqlshdk::utils::format_bytes;
  using mysqlshdk::utils::format_seconds;
  using mysqlshdk::utils::format_throughput_bytes;

  if (m_opt.is_multifile()) {
    return std::string{
        std::to_string(m_stats.total_files_processed) + " file(s) (" +
        format_bytes(m_stats.total_bytes) + ") was imported in " +
        format_seconds(m_timer.total_seconds_elapsed()) + " at " +
        format_throughput_bytes(m_stats.total_bytes,
                                m_timer.total_seconds_elapsed())};
  }
  const auto filesize = m_opt.file_size();
  return std::string{
      "File '" + m_opt.full_path().masked() + "' (" + format_bytes(filesize) +
      ") was imported in " + format_seconds(m_timer.total_seconds_elapsed()) +
      " at " +
      format_throughput_bytes(filesize, m_timer.total_seconds_elapsed())};
}

std::string Import_table::rows_affected_info() {
  return "Total rows affected in " + m_opt.schema() + "." + m_opt.table() +
         ": " + m_stats.to_string();
}

}  // namespace import_table
}  // namespace mysqlsh
