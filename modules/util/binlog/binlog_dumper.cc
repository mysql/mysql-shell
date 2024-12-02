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

#include "modules/util/binlog/binlog_dumper.h"

#include <mysql/binlog/event/binlog_event.h>
#include <mysql/binlog/event/control_events.h>
#include <mysql/gtid/gtidset.h>

#include <algorithm>
#include <cassert>
#include <mutex>
#include <string>
#include <utility>

#include "mysqlshdk/include/scripting/shexcept.h"
#include "mysqlshdk/include/shellcore/console.h"
#include "mysqlshdk/include/shellcore/shell_init.h"
#include "mysqlshdk/libs/db/mysql/binary_log.h"
#include "mysqlshdk/libs/db/mysql/session.h"
#include "mysqlshdk/libs/textui/text_progress.h"
#include "mysqlshdk/libs/utils/logger.h"
#include "mysqlshdk/libs/utils/strformat.h"
#include "mysqlshdk/libs/utils/thread_pool.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_json.h"
#include "mysqlshdk/libs/utils/utils_string.h"

#include "modules/util/binlog/constants.h"
#include "modules/util/common/dump/dump_version.h"
#include "modules/util/common/dump/server_info.h"
#include "modules/util/common/dump/utils.h"
#include "modules/util/dump/common_errors.h"

namespace mysqlsh {
namespace binlog {

namespace {

// 255 characters total:
// - 240 - base name
// -   5 - base name ordinal number
// -  10 - extension
constexpr std::size_t k_max_basename_length = 240;

}  // namespace

class Binlog_dumper::Dumper final {
 public:
  Dumper(const Binlog_dumper *parent, Binlog_task *task, Stats *stats)
      : m_parent(parent),
        m_task(task),
        m_stats(stats),
        m_description_event(BINLOG_VERSION, parent->m_options.source_instance()
                                                .version.number.get_base()
                                                .c_str()) {}

  const mysql::gtid::Gtid_set &dump() {
    log_info("Dumping '%s' - %" PRIu64 " bytes", m_task->name.c_str(),
             (m_task->end ? m_task->end : m_task->total_size) - m_task->begin);

    m_task->stats.begin = current_time();

    mysqlsh::Mysql_thread mysql_thread;
    mysqlshdk::db::mysql::Binary_log binlog{
        mysqlshdk::db::mysql::open_session(
            m_parent->m_options.connection_options())
            ->release_connection(),
        m_task->name,
        std::max<uint64_t>(
            m_task->begin,
            mysqlshdk::db::mysql::Binary_log::k_binlog_magic_size)};

    if (!m_parent->m_dry_run) {
      if (m_task->end) {
        dump<true>(std::move(binlog));
      } else {
        dump<false>(std::move(binlog));
      }
    }

    m_task->stats.end = current_time();

    log_info("Dumping '%s' - done", m_task->name.c_str());

    return m_gtid_set;
  }

 private:
  static inline std::string current_time() {
    return dump::Progress_thread::Duration::current_time();
  }

  template <bool LAST_FILE>
  void dump(mysqlshdk::db::mysql::Binary_log binlog) {
    const auto output = output_file();

    output->open(mysqlshdk::storage::Mode::WRITE);
    write(mysqlshdk::db::mysql::Binary_log::k_binlog_magic,
          mysqlshdk::db::mysql::Binary_log::k_binlog_magic_size);

    mysqlshdk::db::mysql::Binary_log_event event;

    while (!m_parent->m_worker_interrupt.test()) {
      event = binlog.next_event();

      if (!event.buffer) [[unlikely]] {
        break;
      }

      handle_event(event);
      write(event.buffer, event.size);
      ++m_task->stats.events;

      if constexpr (LAST_FILE) {
        if (event.next_position >= m_task->end) [[unlikely]] {
          break;
        }
      }
    }

    output->close();

    if (m_compressed) {
      // if file is compressed, closing it could flush some data
      const auto diff = output->file_size() - m_task->stats.file_bytes;

      if (diff) {
        m_task->stats.file_bytes += diff;
        m_stats->file_bytes_written += diff;
      }
    }

    m_task->stats.gtid_set = m_gtid_set.to_string();
  }

  std::unique_ptr<mysqlshdk::storage::IFile> output_file() {
    auto output = m_parent->make_binlog_file(*m_task);

    m_output = output.get();
    m_compressed =
        dynamic_cast<mysqlshdk::storage::Compressed_file *>(m_output);

    return output;
  }

  inline void write(const void *buffer, size_t length) {
    assert(m_output);

    const auto bytes_written = m_output->write(buffer, length);

    if (bytes_written < 0) [[unlikely]] {
      throw std::runtime_error{
          shcore::str_format("Failed to write to binary log file '%s'",
                             m_output->full_path().masked().c_str())};
    }

    m_task->stats.data_bytes += length;
    m_stats->data_bytes_written += length;

    const auto file_bytes =
        m_compressed ? m_compressed->latest_io_size() : bytes_written;

    m_task->stats.file_bytes += file_bytes;
    m_stats->file_bytes_written += file_bytes;
  }

  inline void handle_event(
      const mysqlshdk::db::mysql::Binary_log_event &event) {
    using mysql::binlog::event::Log_event_type;

    const auto type = static_cast<Log_event_type>(event.type);

    if (Log_event_type::FORMAT_DESCRIPTION_EVENT != type &&
        Log_event_type::GTID_LOG_EVENT != type &&
        Log_event_type::GTID_TAGGED_LOG_EVENT != type) {
      return;
    }

    const char *buffer = reinterpret_cast<const char *>(event.buffer);

    if (Log_event_type::FORMAT_DESCRIPTION_EVENT == type) {
      using mysql::binlog::event::Format_description_event;
      m_description_event =
          Format_description_event{buffer, &m_description_event};
    } else {
      mysql::binlog::event::Gtid_event gtid{buffer, &m_description_event};
      const auto gno = gtid.get_gno();

      if (m_gtid_set.add(gtid.get_tsid(), mysql::gtid::Gno_interval{gno, gno}))
          [[unlikely]] {
        throw std::runtime_error{
            shcore::str_format("Failed to add GTID '%s:%" PRId64 "'",
                               gtid.get_tsid().to_string().c_str(), gno)};
      }
    }
  }

  const Binlog_dumper *m_parent;
  Binlog_task *m_task;
  Stats *m_stats;

  mysqlshdk::storage::IFile *m_output;
  mysqlshdk::storage::Compressed_file *m_compressed;

  mysql::binlog::event::Format_description_event m_description_event;
  mysql::gtid::Gtid_set m_gtid_set;
};

class Binlog_dumper::Dump_progress final {
 public:
  Dump_progress(dump::Progress_thread *progress_thread, const Stats &stats,
                bool compressed)
      : m_stats(stats), m_compressed(compressed) {
    dump::Progress_thread::Throughput_config config;

    config.space_before_item = false;
    config.current = [this]() -> uint64_t {
      return m_stats.data_bytes_written;
    };
    config.total = [this]() { return m_stats.data_bytes; };
    config.right_label = [this]() {
      return shcore::str_format(
          "%s, %zu / %zu binlogs done",
          m_compressed ? compressed_throughput().c_str() : "",
          m_stats.binlogs_written.load(), m_stats.binlogs);
    };

    m_stage =
        progress_thread->start_stage("Dumping binary logs", std::move(config));
  }

  Dump_progress(const Dump_progress &) = delete;
  Dump_progress(Dump_progress &&) = delete;

  Dump_progress &operator=(const Dump_progress &) = delete;
  Dump_progress &operator=(Dump_progress &&) = delete;

  ~Dump_progress() {
    if (m_stage) {
      m_stage->finish();
    }
  }

 private:
  std::string compressed_throughput() {
    using mysqlshdk::utils::format_throughput_bytes;

    m_throughput.push(m_stats.file_bytes_written);
    return shcore::str_format(
        ", %s compressed",
        format_throughput_bytes(m_throughput.rate(), 1.0).c_str());
  }

  dump::Progress_thread::Stage *m_stage;
  const Stats &m_stats;
  bool m_compressed;

  mysqlshdk::textui::Throughput m_throughput;
};

Binlog_dumper::Binlog_dumper(const Dump_binlogs_options &options)
    : m_options(options),
      m_dry_run(m_options.dry_run()),
      m_root_dir(m_options.output()),
      m_basenames(k_max_basename_length),
      m_progress_thread("Dump binlogs", m_options.show_progress()) {
  const auto compression = m_compression.type = m_options.compression();
  m_compression.enabled = mysqlshdk::storage::Compression::NONE != compression;
  m_compression.extension = mysqlshdk::storage::get_extension(compression);
  m_compression.name = mysqlshdk::storage::to_string(compression);
}

void Binlog_dumper::run() {
  if (m_dry_run) {
    current_console()->print_note(
        "The dryRun option is enabled, no files will be created.");
  }

  try {
    do_run();
  } catch (...) {
    clean_up();
    dump::translate_current_exception(m_progress_thread);
  }

  if (m_interrupted) {
    clean_up();
    throw shcore::cancelled("Interrupted by user");
  }
}

void Binlog_dumper::interrupt() { m_worker_interrupt.test_and_set(); }

void Binlog_dumper::async_interrupt() {
  m_interrupted = true;

  const auto context =
      shcore::str_format("Interrupted by user. Canceling %s...",
                         m_cleaning_up ? "clean up" : "dump");

  m_progress_thread.console()->print_warning(context);
}

void Binlog_dumper::do_run() {
  const auto console = current_console();

  if (const auto &source_dump = m_options.previous_dump();
      !source_dump.empty()) {
    console->print_info(shcore::str_format(
        "Starting from previous dump: %s, created at: %s UTC",
        source_dump.c_str(), m_options.previous_dump_timestamp().c_str()));
  }

  console->print_info(
      shcore::str_format("Starting from binary log file: %s",
                         m_options.start_from().file.to_string().c_str()));

  if (m_options.start_from().file == m_options.end_at().file) {
    console->print_note("Nothing to be dumped.");
    return;
  }

  {
    m_progress_thread.start();
    shcore::on_leave_scope finish_progress{
        [this]() { m_progress_thread.finish(); }};

    create_tasks();

    create_output_dirs();
    write_begin_metadata();

    dump();

    write_end_metadata();
  }

  summarize();
}

void Binlog_dumper::create_tasks() {
  const auto &binlogs = m_options.binlogs();
  m_binlog_tasks.reserve(binlogs.size());

  for (const auto &binlog : binlogs) {
    Binlog_task task;
    task.name = binlog.name();
    task.basename =
        m_basenames.get(dump::common::encode_schema_basename(task.name));
    task.total_size = binlog.size();

    m_binlog_tasks.emplace_back(std::move(task));

    m_dump_stats.data_bytes += task.total_size;
  }

  m_binlog_tasks.front().begin = m_options.start_from().file.position;
  m_binlog_tasks.back().end = m_options.end_at().file.position;

  // adjust the total size using the begin and end positions
  m_dump_stats.data_bytes -= m_binlog_tasks.front().begin;

  m_dump_stats.data_bytes -=
      m_binlog_tasks.back().total_size - m_binlog_tasks.back().end;

  m_dump_stats.binlogs = m_binlog_tasks.size();
}

void Binlog_dumper::create_output_dirs() {
  if (m_root_dir->exists()) {
    log_debug("Using root dump directory: %s",
              m_root_dir->full_path().masked().c_str());
  } else {
    if (!m_dry_run) {
      m_root_dir->create();
    }

    m_root_dir_created = true;

    log_debug("Created root dump directory: %s",
              m_root_dir->full_path().masked().c_str());
  }

  using mysqlshdk::utils::fmttime;
  using mysqlshdk::utils::Time_type;

  // YYYY-mm-dd-HH-MM-SS (UTC)
  auto dir_name = fmttime("%Y-%m-%d-%H-%M-%S", Time_type::GMT);

  if (!m_dry_run) {
    const auto dirs = std::move(m_root_dir->list().directories);

    if (dirs.contains(dir_name)) {
      int suffix = 0;
      std::string candidate;

      do {
        candidate = shcore::str_format("%s-%d", dir_name.c_str(), ++suffix);
      } while (dirs.contains(candidate));

      dir_name = std::move(candidate);
    }
  }

  m_dump_dir = m_root_dir->directory(dir_name);

  if (!m_dry_run) {
    m_dump_dir->create();
  }

  m_dump_dir_created = true;

  log_debug("Created dump directory: %s",
            m_dump_dir->full_path().masked().c_str());
}

void Binlog_dumper::write_begin_metadata() const {
  if (m_dry_run) {
    return;
  }

  const auto header = [this](shcore::JSON_dumper *json, bool full) {
    json->append("dumper",
                 shcore::str_format("mysqlsh %s", shcore::get_long_version()));
    json->append("version", dump::common::k_dumper_version);
    json->append("origin", "dumpBinlogs");
    json->append("timestamp", m_progress_thread.duration().started_at());

    json->append("source");
    dump::common::serialize(m_options.source_instance(), json, full, full);
  };

  if (auto root = m_root_dir->file(k_root_metadata_file); !root->exists()) {
    shcore::JSON_dumper json{true};

    json.start_object();
    header(&json, false);
    json.end_object();

    dump::common::write_json(std::move(root), json);
  }

  if (m_dump_dir_created) {
    shcore::JSON_dumper json{true};

    json.start_object();

    header(&json, true);

    if (const auto &options = m_options.original_options()) {
      json.append("options", options);
    }

    {
      json.append("startFrom");
      dump::common::serialize(m_options.start_from(), &json);
    }

    {
      json.append("endAt");
      dump::common::serialize(m_options.end_at(), &json);
    }

    json.append("gtidSet", m_options.gtid_set());

    {
      // ordered list of binlogs
      json.append("binlogs");
      json.start_array();

      for (const auto &binlog : m_binlog_tasks) {
        json.append(binlog.name);
      }

      json.end_array();
    }

    {
      // map of binlogs to their basenames
      json.append("basenames");
      json.start_object();

      for (const auto &binlog : m_binlog_tasks) {
        json.append(binlog.name, binlog.basename);
      }

      json.end_object();
    }

    json.end_object();

    dump::common::write_json(m_dump_dir->file(k_dump_metadata_file), json);
  }
}

void Binlog_dumper::write_end_metadata() const {
  if (m_dry_run || !m_dump_dir_created || m_interrupted) {
    return;
  }

  shcore::JSON_dumper json{true};

  json.start_object();

  json.append("timestamp", dump::Progress_thread::Duration::current_time());

  if (m_options.start_from().gtid_executed.empty()) {
    // we have the dumped GTID set, we can compute the starting gtid_executed
    auto start_from = m_options.start_from();
    const auto session =
        mysqlshdk::db::mysql::open_session(m_options.connection_options());

    start_from.gtid_executed =
        session
            ->queryf("SELECT GTID_SUBTRACT(?,?)",
                     m_options.end_at().gtid_executed, m_dump_stats.gtid_set)
            ->fetch_one_or_throw()
            ->get_string(0);

    json.append("startFrom");
    dump::common::serialize(start_from, &json);

    json.append("gtidSet", m_dump_stats.gtid_set);
  }

  json.end_object();

  dump::common::write_json(m_dump_dir->file(k_done_metadata_file), json);
}

void Binlog_dumper::write_binlog_metadata(const Binlog_task &task) const {
  if (m_dry_run || !m_dump_dir_created || m_interrupted) {
    return;
  }

  shcore::JSON_dumper json{true};

  json.start_object();

  json.append("binlog", task.name);
  if (task.begin) json.append("startPosition", task.begin);
  if (task.end) json.append("endPosition", task.end);

  json.append("extension", m_compression.extension);
  json.append("compression", m_compression.name);

  json.append("gtidSet", task.stats.gtid_set);
  json.append("dataBytes", task.stats.data_bytes);
  json.append("fileBytes", task.stats.file_bytes);
  json.append("eventCount", task.stats.events);

  json.append("begin", task.stats.begin);
  json.append("end", task.stats.end);

  json.end_object();

  dump::common::write_json(m_dump_dir->file(task.basename + ".json"), json);
}

void Binlog_dumper::dump() {
  using mysqlshdk::utils::format_bytes;

  current_console()->print_status(shcore::str_format(
      "Dumping %zu binlogs (%s of data) using %" PRIu64 " threads",
      m_dump_stats.binlogs, format_bytes(m_dump_stats.data_bytes).c_str(),
      m_options.threads()));

  mysql::gtid::Gtid_set gtid_set;
  std::mutex gtid_set_mutex;

  Dump_progress progress{&m_progress_thread, m_dump_stats,
                         m_compression.enabled};

  shcore::Thread_pool pool{m_options.threads(), &m_worker_interrupt};
  pool.start_threads();

  for (auto &task : m_binlog_tasks) {
    if (m_worker_interrupt.test()) {
      break;
    }

    pool.add_task(
        [this, task = &task, &gtid_set, &gtid_set_mutex]() {
          {
            Dumper d{this, task, &m_dump_stats};
            const auto &set = d.dump();

            {
              std::lock_guard lock{gtid_set_mutex};
              gtid_set.add(set);
            }
          }

          write_binlog_metadata(*task);

          return std::string{};
        },
        [this, task = &task](std::string &&) {
          ++m_dump_stats.binlogs_written;
          m_dump_stats.events_written += task->stats.events;
        });
  }

  pool.tasks_done();
  pool.process();

  m_dump_stats.gtid_set = gtid_set.to_string();
}

void Binlog_dumper::summarize() const {
  if (m_dry_run || m_interrupted) {
    return;
  }

  using mysqlshdk::utils::format_bytes;
  using mysqlshdk::utils::format_throughput_bytes;

  const auto console = current_console();

  console->print_status(shcore::str_format(
      "Dump was written to: %s", m_dump_dir->full_path().masked().c_str()));

  console->print_status(shcore::str_format(
      "Total duration: %s", m_progress_thread.duration().to_string().c_str()));

  console->print_status(
      shcore::str_format("Binlogs dumped: %zu", m_dump_stats.binlogs));

  console->print_status(
      shcore::str_format("GTID set dumped: %s", m_dump_stats.gtid_set.c_str()));

  console->print_status(shcore::str_format(
      "%sata size: %s", compressed() ? "Uncompressed d" : "D",
      format_bytes(m_dump_stats.data_bytes_written).c_str()));

  if (compressed()) {
    console->print_status(shcore::str_format(
        "Compressed data size: %s",
        format_bytes(m_dump_stats.file_bytes_written).c_str()));

    console->print_status(shcore::str_format(
        "Compression ratio: %.1f",
        m_dump_stats.data_bytes_written /
            std::max<double>(m_dump_stats.file_bytes_written, 1.0)));
  }

  console->print_status(shcore::str_format("Events written: %" PRIu64,
                                           m_dump_stats.events_written));

  console->print_status(shcore::str_format(
      "Bytes written: %s",
      format_bytes(m_dump_stats.file_bytes_written).c_str()));

  console->print_status(shcore::str_format(
      "Average %sthroughput: %s", compressed() ? "uncompressed " : "",
      format_throughput_bytes(m_dump_stats.data_bytes_written,
                              m_progress_thread.duration().seconds())
          .c_str()));

  if (compressed()) {
    console->print_status(shcore::str_format(
        "Average compressed throughput: %s",
        format_throughput_bytes(m_dump_stats.file_bytes_written,
                                m_progress_thread.duration().seconds())
            .c_str()));
  }
}

void Binlog_dumper::clean_up() {
  if (m_dry_run) {
    return;
  }

  {
    common::Storage_options::Storage_type storage;
    m_options.storage_config(m_root_dir->full_path().real(), &storage);

    if (common::Storage_options::Storage_type::Oci_prefix_par == storage) {
      current_console()->print_warning(
          shcore::str_format("Skipping clean up, it's not possible to remove "
                             "files using the OCI prefix PAR."));
      return;
    }
  }

  m_cleaning_up = true;
  m_worker_interrupt.clear();

  try {
    do_clean_up();
  } catch (const std::exception &e) {
    current_console()->print_error(
        shcore::str_format("Failed to clean up the dump: %s", e.what()));
  }
}

void Binlog_dumper::do_clean_up() {
  if (!m_worker_interrupt.test()) {
    clean_up_dump_dir();
  }

  if (!m_worker_interrupt.test()) {
    clean_up_root_dir();
  }
}

void Binlog_dumper::clean_up_dump_dir() {
  if (!m_dump_dir_created) {
    return;
  }

  const auto files = std::move(m_dump_dir->list().files);

  // we're using a local progress thread to not to mess up with the global one
  dump::Progress_thread progress{"Cleaning up", m_options.show_progress()};
  progress.start();
  shcore::on_leave_scope finish_progress([&progress]() { progress.finish(); });

  std::atomic<std::size_t> current_files{0};
  dump::Progress_thread::Progress_config config;
  config.current = [&current_files]() { return current_files.load(); };
  config.total = [total_files = files.size()]() { return total_files; };
  progress.start_stage("Removing files", std::move(config));

  // use more threads in case of a remote dump
  shcore::Thread_pool pool{
      (m_dump_dir->is_local() ? 1 : 4) * m_options.threads(),
      &m_worker_interrupt};
  pool.start_threads();

  for (const auto &file : files) {
    if (m_worker_interrupt.test()) {
      break;
    }

    pool.add_task(
        [dir = m_dump_dir.get(), name = file.name()]() {
          dir->file(name)->remove();
          return std::string{};
        },
        [&current_files](std::string &&) { ++current_files; });
  }

  pool.tasks_done();
  pool.process();

  if (!m_worker_interrupt.test()) {
    m_dump_dir->remove();
  }
}

void Binlog_dumper::clean_up_root_dir() {
  if (!m_root_dir_created) {
    return;
  }

  if (!m_worker_interrupt.test()) {
    const auto file = m_root_dir->file(k_root_metadata_file);

    if (file->exists()) {
      file->remove();
    }
  }

  if (!m_worker_interrupt.test()) {
    m_root_dir->remove();
  }
}

std::unique_ptr<mysqlshdk::storage::IFile> Binlog_dumper::make_binlog_file(
    const Binlog_task &task) const {
  const auto mmap = mysqlshdk::storage::mmap_options();

  if (m_compression.enabled) {
    return mysqlshdk::storage::make_file(
        m_dump_dir->file(task.basename + m_compression.extension, mmap),
        m_compression.type, m_options.compression_options());
  } else {
    return m_dump_dir->file(task.basename, mmap);
  }
}

}  // namespace binlog
}  // namespace mysqlsh
