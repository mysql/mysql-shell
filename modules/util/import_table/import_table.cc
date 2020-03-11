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

#include "modules/util/import_table/import_table.h"

#include <algorithm>
#include <limits>
#include <utility>

#include "modules/util/dump/console_with_progress.h"
#include "modules/util/import_table/chunk_file.h"
#include "modules/util/import_table/load_data.h"
#include "mysqlshdk/include/shellcore/shell_options.h"
#include "mysqlshdk/libs/textui/text_progress.h"
#include "mysqlshdk/libs/utils/strformat.h"
#include "mysqlshdk/libs/utils/utils_file.h"
#include "mysqlshdk/libs/utils/utils_general.h"

namespace mysqlsh {
namespace import_table {

Import_table::Import_table(const Import_table_options &options)
    : m_console(std::make_shared<dump::Console_with_progress>(m_progress,
                                                              &m_output_mutex)),
      m_opt(options) {
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

void Import_table::join_workers() {
  for (auto &t : m_threads) {
    t.join();
  }
}

void Import_table::rethrow_exceptions() {
  for (const auto &exc : m_thread_exception) {
    if (exc) {
      std::rethrow_exception(exc);
    }
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
  for (int64_t i = 0; i < m_opt.threads_size(); i++) {
    Load_data_worker worker(m_opt, i, m_progress.get(), &m_output_mutex,
                            &m_prog_sent_bytes, m_interrupt, &m_range_queue,
                            &m_thread_exception, &m_stats);
    std::thread t(&Load_data_worker::operator(), std::move(worker));
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

void Import_table::import() {
  m_timer.stage_begin("Parallel load data");
  spawn_workers();
  chunk_file();
  join_workers();
  m_timer.stage_end();
  progress_shutdown();
}

std::string Import_table::import_summary() const {
  using mysqlshdk::utils::format_bytes;
  using mysqlshdk::utils::format_seconds;
  using mysqlshdk::utils::format_throughput_bytes;
  const auto filesize = m_opt.file_size();
  return std::string{
      "File '" + m_opt.full_path() + "' (" + format_bytes(filesize) +
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
