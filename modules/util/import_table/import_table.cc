/*
 * Copyright (c) 2018, 2024, Oracle and/or its affiliates.
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

#include "modules/util/import_table/import_table.h"

#include <algorithm>
#include <array>
#include <cstring>
#include <limits>
#include <utility>

#include "modules/util/dump/console_with_progress.h"
#include "mysqlshdk/include/shellcore/shell_options.h"
#include "mysqlshdk/libs/storage/backend/in_memory/allocated_file.h"
#include "mysqlshdk/libs/storage/backend/in_memory/allocator.h"
#include "mysqlshdk/libs/storage/backend/in_memory/threaded_file.h"
#include "mysqlshdk/libs/storage/backend/in_memory/virtual_file_adapter.h"
#include "mysqlshdk/libs/storage/idirectory.h"
#include "mysqlshdk/libs/textui/text_progress.h"
#include "mysqlshdk/libs/utils/natural_compare.h"
#include "mysqlshdk/libs/utils/strformat.h"
#include "mysqlshdk/libs/utils/utils_file.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_path.h"

#include "modules/util/import_table/chunk_file.h"
#include "modules/util/import_table/load_data.h"
#include "modules/util/import_table/scanner.h"

namespace mysqlsh {
namespace import_table {

Import_table::Import_table(const Import_table_options &options)
    : m_opt(options),
      m_interrupt(nullptr),
      m_progress_thread("Import table", options.show_progress()) {
  m_thread_exception.resize(options.threads_size(), nullptr);
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

bool Import_table::any_exception() const {
  return std::any_of(m_thread_exception.begin(), m_thread_exception.end(),
                     [](std::exception_ptr p) -> bool { return p != nullptr; });
}

void Import_table::progress_setup() {
  dump::Progress_thread::Throughput_config config;

  config.space_before_item = false;
  config.current = [this]() -> uint64_t { return m_prog_file_bytes; };
  config.total = [this]() {
    return m_prog_total_file_bytes.value_or(m_total_file_size);
  };

  config.left_label = [this]() {
    std::string label;

    if (const auto loading =
            m_stats.thread_states[Thread_state::READING].load()) {
      label += std::to_string(loading) + " thds loading - ";
    }

    if (const auto committing =
            m_stats.thread_states[Thread_state::COMMITTING].load()) {
      label += std::to_string(committing) + " thds committing - ";
    }

    constexpr std::array k_progress_spin = {'-', '\\', '|', '/'};
    static thread_local size_t progress_idx = 0;

    if (label.length() > 2) {
      label[label.length() - 2] = k_progress_spin[progress_idx++];

      if (progress_idx >= k_progress_spin.size()) {
        progress_idx = 0;
      }
    }

    return label;
  };

  m_progress_thread.start_stage("Parallel load data", std::move(config));
}

void Import_table::progress_shutdown() {
  if (interrupted()) {
    m_progress_thread.terminate();
  } else {
    m_progress_thread.finish();
  }
}

void Import_table::spawn_workers() {
  const int64_t num_workers = m_opt.threads_size();
  for (int64_t i = 0; i < num_workers; i++) {
    Load_data_worker worker(m_opt, i, &m_prog_sent_bytes, &m_prog_file_bytes,
                            m_interrupt, &m_range_queue, &m_thread_exception,
                            &m_stats);
    std::thread t = mysqlsh::spawn_scoped_thread(&Load_data_worker::operator(),
                                                 std::move(worker));
    m_threads.emplace_back(std::move(t));
  }
}

void Import_table::chunk_file() {
  Chunk_file chunk;
  chunk.set_chunk_size(m_opt.bytes_per_chunk());
  chunk.set_handle_creator(
      [this]() { return m_opt.create_file_handle(m_opt.single_file()); });
  chunk.set_dialect(m_opt.dialect());
  chunk.set_rows_to_skip(m_opt.skip_rows_count());
  chunk.set_output_queue(&m_range_queue);
  chunk.start();

  m_range_queue.shutdown(m_opt.threads_size());
}

void Import_table::build_queue() {
  m_total_file_size = 0;

  for (const auto &glob_item : m_opt.filelist_from_user()) {
    if (glob_item.find('*') != std::string::npos ||
        glob_item.find('?') != std::string::npos) {
      const auto dir = m_opt.create_file_handle(glob_item)->parent();

      if (!dir->exists()) {
        std::string errmsg{"Directory " + dir->full_path().masked() +
                           " does not exist."};
        current_console()->print_error(errmsg);
        noncritical_errors.emplace_back(std::move(errmsg));
        continue;
      }

      const auto list_files =
          dir->filter_files_sorted(shcore::path::basename(glob_item));

      for (const auto &file_info : list_files) {
        File_import_info task;
        task.file = m_opt.create_file_handle(dir->file(file_info.name()));
        task.range_read = false;
        task.is_guard = false;

        m_total_file_size += file_info.size();
        m_has_compressed_files |= task.file->is_compressed();
        m_range_queue.push(std::move(task));
      }
    } else {
      File_import_info task;
      task.file = m_opt.create_file_handle(glob_item);
      task.range_read = false;
      task.is_guard = false;

      if (!task.file->exists()) {
        std::string errmsg{"File " + task.file->full_path().masked() +
                           " does not exist."};
        current_console()->print_error(errmsg);
        noncritical_errors.emplace_back(std::move(errmsg));
        continue;
      }

      m_total_file_size += task.file->file_size();
      m_has_compressed_files |= task.file->is_compressed();
      m_range_queue.push(std::move(task));
    }
  }

  m_range_queue.shutdown(m_opt.threads_size());
}

void Import_table::import() {
  progress_setup();
  shcore::on_leave_scope cleanup_progress([this]() { progress_shutdown(); });
  spawn_workers();

  if (m_opt.is_multifile()) {
    build_queue();
  } else {
    // NOTE: m_total_file_size needs to be set below
    m_has_compressed_files = m_opt.is_compressed(m_opt.single_file());

    if (m_opt.dialect_supports_chunking()) {
      scan_file();
    } else {
      build_queue();
    }
  }

  join_workers();
}

std::string Import_table::import_summary() const {
  using mysqlshdk::utils::format_bytes;
  using mysqlshdk::utils::format_seconds;
  using mysqlshdk::utils::format_throughput_bytes;

  const auto seconds = m_progress_thread.duration().seconds();
  bool plural = false;
  std::string msg;

  if (m_opt.is_multifile()) {
    plural = m_stats.total_files_processed != 1;

    msg += std::to_string(m_stats.total_files_processed) + " file";

    if (plural) {
      msg += 's';
    }
  } else {
    msg += "File '" + m_opt.masked_full_path() + "'";
  }

  msg += " (" + format_bytes(m_stats.total_data_bytes);

  if (m_has_compressed_files) {
    msg += " uncompressed, " + format_bytes(m_total_file_size) + " compressed";
  }

  msg += ") ";
  msg += (plural ? "were" : "was");
  msg += " imported in " + format_seconds(seconds) + " at " +
         format_throughput_bytes(m_stats.total_data_bytes, seconds);

  if (m_has_compressed_files) {
    msg += " uncompressed, " +
           format_throughput_bytes(m_total_file_size, seconds) + " compressed";
  }

  return msg;
}

std::string Import_table::rows_affected_info() {
  return "Total rows affected in " + m_opt.schema() + "." + m_opt.table() +
         ": " + m_stats.to_string();
}

void Import_table::scan_file() {
  using mysqlshdk::storage::Mode;
  using mysqlshdk::storage::in_memory::Allocated_file;
  using mysqlshdk::storage::in_memory::Allocator;
  using mysqlshdk::storage::in_memory::Scoped_data_block;
  using mysqlshdk::storage::in_memory::threaded_file;
  using mysqlshdk::storage::in_memory::Threaded_file_config;
  using mysqlshdk::storage::in_memory::Virtual_file_adapter;

  // if file is big enough, we fetch it in 1MB chunks
  static constexpr std::size_t k_one_mb = 1024 * 1024;
  const std::size_t block_size =
      m_opt.file_size() >= k_one_mb * m_opt.threads_size() &&
              m_opt.bytes_per_chunk() >= k_one_mb
          ? k_one_mb
          : 8192;
  // each thread fetches block_size bytes, a page holds enough memory for all
  // threads
  m_allocator = std::make_unique<Allocator>(m_opt.threads_size() * block_size,
                                            block_size);

  Threaded_file_config config;
  config.file_path = m_opt.single_file();
  config.config = m_opt.storage_config();
  config.threads = m_opt.threads_size();
  config.max_memory = m_opt.bytes_per_chunk();
  config.allocator = m_allocator.get();

  auto source = threaded_file(std::move(config));
  source->open(Mode::READ);
  shcore::on_leave_scope close_source([&source]() {
    if (source) {
      source->close();
    }
  });

  Scanner scanner{m_opt.dialect(), m_opt.skip_rows_count()};
  const auto make_chunk = [this]() {
    return std::make_unique<Allocated_file>(m_opt.single_file(),
                                            m_allocator.get(), true);
  };
  std::unique_ptr<Allocated_file> chunk;
  File_import_info info;
  std::size_t extracted_blocks = 0;
  std::size_t scheduled_chunks = 0;
  const auto schedule_chunk = [&]() {
    while (true) {
      if (interrupted()) {
        return;
      }

      // wait for at least one thread to be available
      if (scheduled_chunks - m_stats.total_files_processed >=
          static_cast<std::size_t>(m_opt.threads_size())) {
        shcore::sleep_ms(100);
      } else {
        break;
      }
    }

    info.range.second = chunk->size();
    info.is_guard = false;
    info.range_read = true;
    info.file = std::make_unique<Virtual_file_adapter>(std::move(chunk));

    assert(extracted_blocks > 0);
    const auto file_offset = block_size * (extracted_blocks - 1);
    info.context = " @ file bytes range [" +
                   std::to_string(file_offset + info.range.first) + ", " +
                   std::to_string(file_offset + info.range.second) + ")";

    m_range_queue.push(std::move(info));
    ++scheduled_chunks;
  };

  // If file is compressed, the total size needs to correspond to the
  // uncompressed size, because otherwise progress will go over 100%
  // (Load_data_worker works with an in-memory file and has no information
  // regarding the size of compressed reads). We set it here to zero and update
  // it once its known. When total is zero, progress is displayed as follows:
  //   ?% (58.11 MB / ?), 28.15 MB/s
  if (m_has_compressed_files) {
    m_prog_total_file_bytes = 0;
  }

  m_total_file_size = m_opt.file_size();
  std::size_t total_size = 0;

  // skip rows, then schedule blocks
  while (!interrupted()) {
    auto block = source->extract();

    if (!block->size) {
      if (chunk && chunk->size() > 0) {
        schedule_chunk();
      }

      break;
    }

    ++extracted_blocks;
    total_size += block->size;

    if (const auto offset = scanner.scan(block->memory, block->size);
        offset >= 0) {
      if (!chunk || chunk->size() - info.range.first + offset >=
                        m_opt.bytes_per_chunk()) {
        if (chunk) {
          if (offset > 0) {
            // row does not start at the beginning, block needs to be duplicated
            // and used by both previous chunk and next chunk
            auto new_block = Scoped_data_block::new_block(m_allocator.get());

            // copy the smaller part of the memory
            if (static_cast<std::size_t>(offset) < block->size / 2) {
              // new block is the last block of the previous chunk
              new_block->size = offset;
              ::memcpy(new_block->memory, block->memory, offset);
              chunk->append(std::move(new_block));
            } else {
              // old block is the last block of the previous chunk, new block
              // goes to the next chunk and pretends that it has all the data of
              // the old block
              new_block->size = block->size;
              ::memcpy(new_block->memory + offset, block->memory + offset,
                       block->size - offset);
              block->size = offset;
              chunk->append(std::move(block));
              block = std::move(new_block);
            }
          }

          schedule_chunk();
        }

        chunk = make_chunk();
        info.range.first = offset;
      }
    }

    if (chunk) {
      chunk->append(std::move(block));
    }
  }

  if (m_has_compressed_files) {
    m_prog_total_file_bytes = total_size;
  }

  m_range_queue.shutdown(m_opt.threads_size());
}

}  // namespace import_table
}  // namespace mysqlsh
