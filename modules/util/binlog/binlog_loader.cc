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

#include "modules/util/binlog/binlog_loader.h"

#include <mysql/binlog/event/binlog_event.h>
#include <mysql/binlog/event/control_events.h>
#include <mysql/gtid/gtid.h>
#include <mysql/gtid/gtidset.h>

#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <cstring>
#include <deque>
#include <functional>
#include <istream>
#include <memory>
#include <mutex>
#include <optional>
#include <streambuf>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "mysqlshdk/include/scripting/shexcept.h"
#include "mysqlshdk/include/shellcore/console.h"
#include "mysqlshdk/include/shellcore/shell_init.h"
#include "mysqlshdk/libs/db/mysql/binary_log.h"
#include "mysqlshdk/libs/db/mysql/session.h"
#include "mysqlshdk/libs/mysql/gtid_utils.h"
#include "mysqlshdk/libs/storage/backend/in_memory/allocator.h"
#include "mysqlshdk/libs/textui/text_progress.h"
#include "mysqlshdk/libs/utils/logger.h"
#include "mysqlshdk/libs/utils/process_launcher.h"
#include "mysqlshdk/libs/utils/strformat.h"
#include "mysqlshdk/libs/utils/thread_pool.h"
#include "mysqlshdk/libs/utils/utils_file.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_mysql_parsing.h"
#include "mysqlshdk/libs/utils/utils_path.h"
#include "mysqlshdk/libs/utils/utils_string.h"

#include "modules/util/binlog/utils.h"
#include "modules/util/dump/common_errors.h"

namespace mysqlsh {
namespace binlog {

namespace {

using mysql::binlog::event::Gtid_event;
using mysqlshdk::db::mysql::Binary_log;
using mysqlshdk::db::mysql::Binary_log_event;

// 1MB page, 8kB blocks
constexpr std::size_t k_page_size = 1048576;
constexpr std::size_t k_block_size = 8192;

#ifndef HAVE_LIBEXEC_DIR
#error HAVE_LIBEXEC_DIR needs to be defined
#endif  // !HAVE_LIBEXEC_DIR

const std::string &mysqlbinlog_path() {
  static std::string k_path =
      shcore::path::join_path(shcore::get_libexec_folder(),
                              "mysqlbinlog"
#ifdef _WIN32
                              ".exe"
#endif  // _WIN32
      );
  return k_path;
}

/**
 * Single producer - single consumer queue.
 */
class Memory_queue {
 public:
  using Memory_block = mysqlshdk::storage::in_memory::Scoped_data_block;

  // 1MB page, 8kB blocks
  explicit Memory_queue(const shcore::atomic_flag *flag)
      : m_flag(flag), m_allocator(k_page_size, k_block_size) {}

  Memory_queue(const Memory_queue &) = delete;
  Memory_queue(Memory_queue &&) = delete;

  Memory_queue &operator=(const Memory_queue &) = delete;
  Memory_queue &operator=(Memory_queue &&) = delete;

  ~Memory_queue() = default;

  inline Memory_block allocate() {
    return Memory_block::new_block(&m_allocator);
  }

  [[nodiscard]] inline bool push(Memory_block block) {
    {
      std::unique_lock lock{m_mutex};

      do {
        if (m_flag->test()) {
          return false;
        }
      } while (!m_popped.wait_for(lock, std::chrono::milliseconds(100),
                                  [this]() { return !full(); }));

      m_queue.push_back(std::move(block));
    }

    m_pushed.notify_one();
    return true;
  }

  [[nodiscard]] inline Memory_block pop() {
    Memory_block block;

    {
      std::unique_lock lock{m_mutex};

      do {
        if (m_flag->test()) {
          return block;
        }
      } while (!m_pushed.wait_for(lock, std::chrono::milliseconds(100),
                                  [this]() { return !m_queue.empty(); }));

      block = std::move(m_queue.front());
      m_queue.pop_front();
    }

    m_popped.notify_one();

    return block;
  }

  inline void done() {
    {
      std::lock_guard lock{m_mutex};
      m_queue.push_back(Memory_block{});
    }

    m_pushed.notify_one();
  }

  inline bool is_interrupted() const noexcept { return m_flag->test(); }

 private:
  inline bool full() const noexcept {
    static constexpr std::size_t k_max_size = 1280;  // maximum capacity: 10 MB

    return m_queue.size() >= k_max_size;
  }

  const shcore::atomic_flag *m_flag;

  mysqlshdk::storage::in_memory::Allocator m_allocator;
  std::deque<Memory_block> m_queue;

  std::mutex m_mutex;
  std::condition_variable m_pushed;
  std::condition_variable m_popped;
};

}  // namespace

class Binlog_loader::File_reader final {
 public:
  File_reader(const Load_binlogs_options &options, Memory_queue *queue,
              Stats *stats)
      : m_options(options), m_queue(queue), m_stats(stats) {}

  File_reader(const File_reader &) = delete;
  File_reader(File_reader &&) = delete;

  File_reader &operator=(const File_reader &) = delete;
  File_reader &operator=(File_reader &&) = delete;

  ~File_reader() = default;

  void run() {
    using mysqlshdk::utils::format_bytes;

    const Binlog_dump *dump = nullptr;
    std::unique_ptr<mysqlshdk::storage::IDirectory> dir;
    const auto console = current_console();

    bool first_file = true;
    bool last_file = false;

    console->print_status(
        shcore::str_format("Opening dump '%s'", m_options.full_path().c_str()));

    for (const auto &binlog : m_options.binlogs()) {
      if (m_queue->is_interrupted()) [[unlikely]] {
        break;
      }

      last_file = m_stats->binlogs_read == m_stats->binlogs - 1;

      if (binlog.dump != dump) {
        dump = binlog.dump;

        console->print_status(shcore::str_format(
            "  Loading dump '%s' created at %s UTC",
            dump->directory_name().c_str(), dump->timestamp().c_str()));
        dir = dump->make_directory();
      }

      console->print_status(shcore::str_format(
          "    Loading binary log file '%.*s', GTID set: %s (%s)",
          static_cast<int>(binlog.name.length()), binlog.name.data(),
          binlog.gtid_set.c_str(), format_bytes(binlog.data_bytes).c_str()));

      Input_file file{Binlog_dump::make_file(dir.get(), binlog), this};

      if (first_file) {
        Binlog_event_filter filter{std::move(file), Output_file{m_queue}};

        find_beginning(&filter);

        if (last_file && should_filter_last_file()) {
          filter_last_file(&filter);
        } else if (!m_options.dry_run()) {
          // copy remaining events
          filter.filter([](const Binary_log_event &) {
            return Binlog_event_filter::Action::Write;
          });
        }
      } else if (last_file) {
        if (should_filter_last_file()) {
          Binlog_event_filter filter{std::move(file), Output_file{m_queue}};
          filter_last_file(&filter);
        } else {
          read_file(std::move(file));
        }
      } else {
        read_file(std::move(file));
      }

      ++m_stats->binlogs_read;
      first_file = false;
    }

    m_queue->done();
  }

 private:
  class Input_file final {
   public:
    Input_file(std::unique_ptr<mysqlshdk::storage::IFile> file,
               File_reader *parent)
        : m_file(std::move(file)), m_parent(parent) {
      m_compressed =
          dynamic_cast<mysqlshdk::storage::Compressed_file *>(m_file.get());
      m_file->open(mysqlshdk::storage::Mode::READ);
    }

    Input_file(const Input_file &) = delete;
    Input_file(Input_file &&) = default;

    Input_file &operator=(const Input_file &) = delete;
    Input_file &operator=(Input_file &&) = default;

    ~Input_file() {
      if (m_file) {
        m_file->close();
      }
    }

    inline Memory_queue::Memory_block read() {
      auto block = m_parent->m_queue->allocate();
      block->size = m_file->read(block->memory, k_block_size);

      if (!block->size) [[unlikely]] {
        return block;
      }

      m_parent->m_stats->data_bytes_read += block->size;
      m_parent->m_stats->file_bytes_read +=
          m_compressed ? m_compressed->latest_io_size() : block->size;

      return block;
    }

   private:
    std::unique_ptr<mysqlshdk::storage::IFile> m_file;
    mysqlshdk::storage::Compressed_file *m_compressed;
    File_reader *m_parent;
  };

  class Output_file final {
   public:
    explicit Output_file(Memory_queue *queue) : m_queue(queue) {
      m_block = m_queue->allocate();
      // always write magic bytes, in dry run we simulate an empty binlog file
      (void)write(Binary_log::k_binlog_magic, Binary_log::k_binlog_magic_size);
    }

    Output_file(const Output_file &) = delete;
    Output_file(Output_file &&) = default;

    Output_file &operator=(const Output_file &) = delete;
    Output_file &operator=(Output_file &&) = default;

    ~Output_file() {
      if (m_block->memory && m_block->size) {
        (void)m_queue->push(std::move(m_block));
      }
    }

    [[nodiscard]] inline bool write(const char *buffer, std::size_t length) {
      if (!m_block->memory) [[unlikely]] {
        return false;
      }

      std::size_t bytes = 0;
      std::size_t total = 0;

      while (length > 0) {
        bytes = std::min(length, k_block_size - m_block->size);
        memcpy(m_block->memory + m_block->size, buffer + total, bytes);

        m_block->size += bytes;
        total += bytes;
        length -= bytes;

        if (k_block_size == m_block->size) {
          if (!m_queue->push(std::move(m_block))) [[unlikely]] {
            m_block = Memory_queue::Memory_block{};
            return false;
          }

          m_block = m_queue->allocate();
        }
      }

      return true;
    }

   private:
    Memory_queue *m_queue;
    Memory_queue::Memory_block m_block;
  };

  class Binlog_event_iterator final {
   public:
    explicit Binlog_event_iterator(Input_file file) : m_file(std::move(file)) {
      m_event.resize(k_block_size);
      m_block = m_file.read();
      m_offset = Binary_log::k_binlog_magic_size;
    }

    Binlog_event_iterator(const Binlog_event_iterator &) = delete;
    Binlog_event_iterator(Binlog_event_iterator &&) = default;

    Binlog_event_iterator &operator=(const Binlog_event_iterator &) = delete;
    Binlog_event_iterator &operator=(Binlog_event_iterator &&) = default;

    ~Binlog_event_iterator() = default;

    Binary_log_event next_event() {
      Binary_log_event event;

      if (!read(m_event.data(), k_header_length)) [[unlikely]] {
        memset(&event, 0, sizeof(event));
        return event;
      }

      event = Binary_log::parse_event(m_event.data());

      if (m_event.size() < event.size) {
        m_event.resize(event.size);
        event.buffer = reinterpret_cast<unsigned char *>(m_event.data());
      }

      if (!read(m_event.data() + k_header_length, event.size - k_header_length))
          [[unlikely]] {
        memset(&event, 0, sizeof(event));
      }

      return event;
    }

   private:
    bool read(char *buffer, std::size_t length) {
      if (!m_block->size) [[unlikely]] {
        return false;
      }

      std::size_t bytes = 0;
      std::size_t total = 0;

      while (length > 0) {
        bytes = std::min(length, m_block->size - m_offset);
        memcpy(buffer + total, m_block->memory + m_offset, bytes);

        m_offset += bytes;
        total += bytes;
        length -= bytes;

        if (m_offset == m_block->size) {
          m_block = m_file.read();
          m_offset = 0;

          if (!m_block->size) [[unlikely]] {
            break;
          }
        }
      }

      return 0 == length;
    }

    static constexpr auto k_header_length = Binary_log_event::k_header_length;

    Input_file m_file;

    std::vector<char> m_event;

    Memory_queue::Memory_block m_block;
    std::size_t m_offset;
  };

  class Binlog_event_filter final {
   public:
    enum class Action { Skip, Write, Stop };

    Binlog_event_filter(Input_file input, Output_file output)
        : m_iterator(std::move(input)), m_output(std::move(output)) {}

    Binlog_event_filter(const Binlog_event_filter &) = delete;
    Binlog_event_filter(Binlog_event_filter &&) = default;

    Binlog_event_filter &operator=(const Binlog_event_filter &) = delete;
    Binlog_event_filter &operator=(Binlog_event_filter &&) = default;

    ~Binlog_event_filter() = default;

    void filter(
        const std::function<Action(const Binary_log_event &)> &event_action,
        const std::function<Action(const Gtid_event &)> &gtid_action = {}) {
      using mysql::binlog::event::Log_event_type;

      Action action;
      const char *buffer;
      Binary_log_event event;

      while (true) {
        if (m_next_event.has_value()) [[unlikely]] {
          event = std::move(*m_next_event);
          m_next_event.reset();
        } else {
          event = m_iterator.next_event();
        }

        if (!event.buffer) [[unlikely]] {
          break;
        }

        buffer = reinterpret_cast<const char *>(event.buffer);

        switch (static_cast<Log_event_type>(event.type)) {
          case Log_event_type::FORMAT_DESCRIPTION_EVENT:
            m_description_event =
                Format_description_event{buffer, &m_description_event};
            // always write this event
            action = Action::Write;
            break;

          case Log_event_type::PREVIOUS_GTIDS_LOG_EVENT:
            action = Action::Write;
            break;

          case Log_event_type::GTID_LOG_EVENT:
          case Log_event_type::GTID_TAGGED_LOG_EVENT:
            if (gtid_action) {
              action = gtid_action(Gtid_event{buffer, &m_description_event});
              break;
            }

            [[fallthrough]];

          default:
            action = event_action(event);
            break;
        }

        switch (action) {
          case Action::Skip:
            break;

          case Action::Write:
            if (!m_output.write(buffer, event.size)) {
              return;
            }
            break;

          case Action::Stop:
            m_next_event = std::move(event);
            return;
        }
      }

      return;
    }

   private:
    using Format_description_event =
        mysql::binlog::event::Format_description_event;

    Binlog_event_iterator m_iterator;
    Output_file m_output;
    Format_description_event m_description_event{BINLOG_VERSION, ""};
    std::optional<Binary_log_event> m_next_event;
  };

  void read_file(Input_file file) {
    if (m_options.dry_run()) {
      return;
    }

    Memory_queue::Memory_block block;

    while (true) {
      block = file.read();

      if (!block->size || !m_queue->push(std::move(block))) [[unlikely]] {
        break;
      }
    }
  }

  void find_beginning(Binlog_event_filter *filter) {
    mysql::gtid::Gtid_set gtid_set;
    to_gtid_set(m_options.first_gtid_set(), &gtid_set);

    filter->filter(
        [](const Binary_log_event &) {
          return Binlog_event_filter::Action::Skip;
        },
        [&gtid_set](const Gtid_event &gtid) {
          if (gtid_set.contains(
                  mysql::gtid::Gtid{gtid.get_tsid(), gtid.get_gno()})) {
            current_console()->print_status(shcore::str_format(
                "      Found starting GTID: %s:%" PRId64,
                gtid.get_tsid().to_string().c_str(), gtid.get_gno()));
            return Binlog_event_filter::Action::Stop;
          } else {
            return Binlog_event_filter::Action::Skip;
          }
        });
  }

  inline bool should_filter_last_file() const noexcept {
    return !m_options.stop_before_gtid().empty() ||
           !m_options.stop_after_gtid().empty();
  }

  void filter_last_file(Binlog_event_filter *filter) {
    const auto write_action = m_options.dry_run()
                                  ? Binlog_event_filter::Action::Skip
                                  : Binlog_event_filter::Action::Write;

    if (!m_options.stop_before_gtid().empty()) {
      const auto stop_at =
          mysqlshdk::mysql::Gtid_range::from_gtid(m_options.stop_before_gtid());

      filter->filter(
          [&write_action](const Binary_log_event &) { return write_action; },
          [&write_action, &stop_at](const Gtid_event &gtid) {
            const auto tsid = gtid.get_tsid().to_string();

            if (stop_at.uuid_tag == tsid &&
                stop_at.begin == static_cast<uint64_t>(gtid.get_gno())) {
              current_console()->print_status(
                  shcore::str_format("      Stopped before GTID: %s:%" PRId64,
                                     tsid.c_str(), gtid.get_gno()));
              return Binlog_event_filter::Action::Stop;
            } else {
              return write_action;
            }
          });
    } else if (!m_options.stop_after_gtid().empty()) {
      const auto stop_at =
          mysqlshdk::mysql::Gtid_range::from_gtid(m_options.stop_after_gtid());
      bool should_stop = false;

      filter->filter(
          [&write_action](const Binary_log_event &) { return write_action; },
          [&write_action, &stop_at, &should_stop](const Gtid_event &gtid) {
            const auto tsid = gtid.get_tsid().to_string();

            if (should_stop) {
              return Binlog_event_filter::Action::Stop;
            } else if (stop_at.uuid_tag == tsid &&
                       stop_at.begin == static_cast<uint64_t>(gtid.get_gno())) {
              // we found the GTID we were looking for, continue for now, stop
              // on a new GTID event (next transaction)
              should_stop = true;
              current_console()->print_status(
                  shcore::str_format("      Stopped after GTID: %s:%" PRId64,
                                     tsid.c_str(), gtid.get_gno()));
              return write_action;
            } else {
              return write_action;
            }
          });
    } else {
      assert(false);
    }
  }

  const Load_binlogs_options &m_options;
  Memory_queue *m_queue;
  Stats *m_stats;
};

class Binlog_loader::Process_writer {
 public:
  Process_writer(Memory_queue *queue, shcore::Process_launcher *process)
      : m_queue(queue), m_process(process) {}

  Process_writer(const Process_writer &) = delete;
  Process_writer(Process_writer &&) = delete;

  Process_writer &operator=(const Process_writer &) = delete;
  Process_writer &operator=(Process_writer &&) = delete;

  ~Process_writer() = default;

  void run() {
    Memory_queue::Memory_block block;

    while (true) {
      block = m_queue->pop();

      if (!block->size) [[unlikely]] {
        break;
      }

      if (m_process->write(block->memory, block->size) <= 0) [[unlikely]] {
        break;
      }
    }

    m_process->finish_writing();
  }

 private:
  Memory_queue *m_queue;
  shcore::Process_launcher *m_process;
};

class Binlog_loader::Process_reader {
 public:
  Process_reader(Memory_queue *queue, shcore::Process_launcher *process)
      : m_queue(queue), m_process(process) {}

  Process_reader(const Process_reader &) = delete;
  Process_reader(Process_reader &&) = delete;

  Process_reader &operator=(const Process_reader &) = delete;
  Process_reader &operator=(Process_reader &&) = delete;

  ~Process_reader() = default;

  void run() {
    Memory_queue::Memory_block block;
    int bytes;

    while (true) {
      block = m_queue->allocate();
      bytes = m_process->read(block->memory, k_block_size);

      if (bytes <= 0) [[unlikely]] {
        break;
      }

      block->size = bytes;

      if (!m_queue->push(std::move(block))) [[unlikely]] {
        break;
      }
    }

    m_queue->done();
  }

 private:
  Memory_queue *m_queue;
  shcore::Process_launcher *m_process;
};

class Binlog_loader::Statement_executor {
 public:
  Statement_executor(const Load_binlogs_options &options, Memory_queue *queue,
                     Stats *stats)
      : m_options(options), m_queue(queue), m_stats(stats) {}

  Statement_executor(const Statement_executor &) = delete;
  Statement_executor(Statement_executor &&) = delete;

  Statement_executor &operator=(const Statement_executor &) = delete;
  Statement_executor &operator=(Statement_executor &&) = delete;

  ~Statement_executor() = default;

  void run() {
    mysqlsh::Mysql_thread mysql_thread;
    const auto session =
        mysqlshdk::db::mysql::open_session(m_options.connection_options());
    Stream stream{m_queue};
    std::istream input{&stream};

    mysqlshdk::utils::iterate_sql_stream(
        &input, k_block_size,
        [&](std::string_view sql, std::string_view, size_t, size_t) {
          if (m_queue->is_interrupted()) [[unlikely]] {
            // if execution was interrupted, we may have a partial statement
            return false;
          }

          if (!sql.empty() && '#' != sql[0]) {
            session->execute(sql);
            ++m_stats->statements_executed;
          }

          return true;
        },
        [](std::string_view err) {
          throw std::runtime_error{
              shcore::str_format("Failed to parse the mysqlbinlog output: %.*s",
                                 static_cast<int>(err.length()), err.data())};
        },
        false, false, true, nullptr, nullptr,
        [&session](std::string_view cmd, bool,
                   size_t) -> std::pair<size_t, bool> {
          if (cmd.length() < 2) {
            return std::make_pair(cmd.length(), false);
          }

          if ('g' == cmd[1] || 'G' == cmd[1]) {
            return std::make_pair(2, true);
          }

          if ('C' == cmd[1]) {
            // \C charset - Switch to another charset.
            // mysqlbinlog sends: /*!\C %s */
            static constexpr std::size_t k_offset = 3;
            std::size_t end = k_offset;
            const auto length = cmd.length();

            while (end < length && cmd[end] != ' ') {
              ++end;
            }

            if (const auto charset = cmd.substr(k_offset, end - k_offset);
                !charset.empty()) {
              session->set_character_set(std::string{charset});
            }

            return std::make_pair(end, false);
          }

          return std::make_pair(2, false);
        });
  }

 private:
  class Stream : public std::streambuf {
   public:
    explicit Stream(Memory_queue *queue) : m_queue(queue) {}

   protected:
    int_type underflow() override {
      m_block = m_queue->pop();

      if (!m_block->size) [[unlikely]] {
        return traits_type::eof();
      }

      setg(m_block->memory, m_block->memory, m_block->memory + m_block->size);

      return traits_type::to_int_type(*m_block->memory);
    }

   private:
    Memory_queue *m_queue;
    Memory_queue::Memory_block m_block;
  };

  const Load_binlogs_options &m_options;
  Memory_queue *m_queue;
  Stats *m_stats;
};

class Binlog_loader::Load_progress final {
 public:
  Load_progress(dump::Progress_thread *progress_thread, const Stats &stats,
                bool compressed)
      : m_stats(stats), m_compressed(compressed) {
    dump::Progress_thread::Throughput_config config;

    config.space_before_item = false;
    config.current = [this]() -> uint64_t { return m_stats.data_bytes_read; };
    config.total = [this]() { return m_stats.data_bytes; };
    config.right_label = [this]() {
      return shcore::str_format(
          "%s, %s, %zu / %zu binlogs done",
          m_compressed ? compressed_throughput().c_str() : "",
          statements_throughput().c_str(), m_stats.binlogs_read.load(),
          m_stats.binlogs);
    };
    config.on_display_started = [this]() {
      using mysqlshdk::utils::format_bytes;

      current_console()->print_status(
          shcore::str_format("Loading %zu binlogs, %s of data", m_stats.binlogs,
                             format_bytes(m_stats.data_bytes).c_str()));
    };

    m_stage =
        progress_thread->start_stage("Loading binary logs", std::move(config));
  }

  Load_progress(const Load_progress &) = delete;
  Load_progress(Load_progress &&) = delete;

  Load_progress &operator=(const Load_progress &) = delete;
  Load_progress &operator=(Load_progress &&) = delete;

  ~Load_progress() {
    if (m_stage) {
      m_stage->finish();
    }
  }

 private:
  std::string compressed_throughput() {
    using mysqlshdk::utils::format_throughput_bytes;

    m_compressed_throughput.push(m_stats.file_bytes_read);

    return shcore::str_format(
        ", %s compressed",
        format_throughput_bytes(m_compressed_throughput.rate(), 1.0).c_str());
  }

  std::string statements_throughput() {
    using mysqlshdk::utils::format_throughput_items;

    m_statements_throughput.push(m_stats.statements_executed);

    return format_throughput_items("stmt", "stmts",
                                   m_statements_throughput.rate(), 1.0, true);
  }

  dump::Progress_thread::Stage *m_stage;
  const Stats &m_stats;
  bool m_compressed;

  mysqlshdk::textui::Throughput m_compressed_throughput;
  mysqlshdk::textui::Throughput m_statements_throughput;
};

Binlog_loader::Binlog_loader(const Load_binlogs_options &options)
    : m_options(options),
      m_progress_thread("Load binlogs", m_options.show_progress()) {}

void Binlog_loader::run() {
  if (m_options.dry_run()) {
    current_console()->print_note(
        "The dryRun option is enabled, no data will be loaded.");
  }

  try {
    do_run();
  } catch (...) {
    dump::translate_current_exception(m_progress_thread);
  }

  if (m_worker_interrupt.test()) {
    throw shcore::cancelled("Interrupted by user");
  }
}

void Binlog_loader::interrupt() { m_worker_interrupt.test_and_set(); }

void Binlog_loader::async_interrupt() {
  m_progress_thread.console()->print_warning(
      "Interrupted by user. Stopping load...");
}

void Binlog_loader::do_run() {
  if (m_options.binlogs().empty()) {
    current_console()->print_note("Nothing to be loaded.");
    return;
  }

  {
    m_progress_thread.start();
    shcore::on_leave_scope finish_progress{
        [this]() { m_progress_thread.finish(); }};

    initialize();
    load();
  }

  summarize();
}

void Binlog_loader::initialize() {
  const auto &binlogs = m_options.binlogs();
  m_load_stats.binlogs = binlogs.size();

  for (const auto &binlog : binlogs) {
    m_load_stats.data_bytes += binlog.data_bytes;
    m_load_stats.file_bytes += binlog.file_bytes;
  }
}

void Binlog_loader::load() {
  Memory_queue file_data{&m_worker_interrupt};
  Memory_queue process_data{&m_worker_interrupt};
  const char *k_process_args[] = {
      mysqlbinlog_path().c_str(),
      "-",
      nullptr,
  };
  // TODO(pawel): capture stderr of mysqlbinlog process, display it only when
  // process returns an error code
  shcore::Process_launcher process{k_process_args, false};
#ifdef _WIN32
  process.set_create_process_group();
#endif
  process.start();

  Load_progress progress{&m_progress_thread, m_load_stats,
                         m_options.has_compressed_binlogs()};

  shcore::Thread_pool pool{4, &m_worker_interrupt};
  pool.start_threads();

  pool.add_task(
      [this, file_data = &file_data]() {
        try {
          File_reader{m_options, file_data, &m_load_stats}.run();
        } catch (const std::exception &e) {
          current_console()->print_error(shcore::str_format(
              "While reading from the binary log files: %s", e.what()));
          throw;
        }

        return std::string{};
      },
      [](std::string &&) {});

  pool.add_task(
      [file_data = &file_data, process = &process]() {
        try {
          Process_writer{file_data, process}.run();
        } catch (const std::exception &e) {
          current_console()->print_error(shcore::str_format(
              "While writing to the 'mysqlbinlog' process: %s", e.what()));
          throw;
        }

        return std::string{};
      },
      [](std::string &&) {});

  pool.add_task(
      [this, process_data = &process_data, process = &process]() {
        try {
          Process_reader{process_data, process}.run();
        } catch (const std::exception &e) {
          current_console()->print_error(shcore::str_format(
              "While reading from the 'mysqlbinlog' process: %s", e.what()));
          throw;
        }

        try {
          // give some time for the process to shut down
          shcore::sleep_ms(50);

          bool is_running = false;

          if (!process->check()) {
            // process is still there
            is_running = true;

            // if this flag is set, there was a problem in another thread, no
            // need to wait
            if (!m_worker_interrupt.test()) {
              // wait up to 0.5 second for process to terminate
              for (int i = 0; i < 0; ++i) {
                shcore::sleep_ms(50);

                if (process->check()) {
                  // process has terminated
                  is_running = false;
                  break;
                }
              }
            }
          }

          if (is_running) {
            // process did not finish, terminate it, don't report anything

            // TODO(pawel): this is not thread safe, this operation closes
            // in/out, if another thread is reading/writing, there's an
            // unsynchronized access
            process->kill();
          } else {
            // process has terminated, check if there were any errors
            if (const auto rc = process->wait()) {
              throw std::runtime_error{shcore::str_format(
                  "process has returned an error code: %d", rc)};
            }
          }
        } catch (const std::exception &e) {
          current_console()->print_error(shcore::str_format(
              "While waiting for the 'mysqlbinlog' process to finish: %s",
              e.what()));
          throw;
        }

        return std::string{};
      },
      [](std::string &&) {});

  if (m_options.dry_run()) {
    pool.add_task(
        [&process_data]() {
          // drain the queue
          while (process_data.pop()->memory) {
          }

          return std::string{};
        },
        [](std::string &&) {});
  } else {
    pool.add_task(
        [this, process_data = &process_data]() {
          try {
            Statement_executor{m_options, process_data, &m_load_stats}.run();
          } catch (const std::exception &e) {
            current_console()->print_error(shcore::str_format(
                "While replaying the binary log events: %s", e.what()));
            throw;
          }

          return std::string{};
        },
        [](std::string &&) {});
  }

  pool.tasks_done();

  try {
    pool.process();
  } catch (const std::exception &) {
    throw std::runtime_error{"Loading the binary log files has failed"};
  }
}

void Binlog_loader::summarize() const {
  using mysqlshdk::utils::format_bytes;
  using mysqlshdk::utils::format_throughput_bytes;

  const auto console = current_console();
  const auto compressed = m_options.has_compressed_binlogs();

  console->print_status(shcore::str_format(
      "Total duration: %s", m_progress_thread.duration().to_string().c_str()));

  console->print_status(
      shcore::str_format("Binlogs loaded: %zu", m_load_stats.binlogs));

  console->print_status(
      shcore::str_format("%sata size: %s", compressed ? "Uncompressed d" : "D",
                         format_bytes(m_load_stats.data_bytes).c_str()));

  if (compressed) {
    console->print_status(
        shcore::str_format("Compressed data size: %s",
                           format_bytes(m_load_stats.file_bytes).c_str()));
  }

  console->print_status(
      shcore::str_format("Statements executed: %" PRIu64,
                         m_load_stats.statements_executed.load()));

  console->print_status(shcore::str_format(
      "Average %sthroughput: %s", compressed ? "uncompressed " : "",
      format_throughput_bytes(m_load_stats.data_bytes_read,
                              m_progress_thread.duration().seconds())
          .c_str()));

  if (compressed) {
    console->print_status(shcore::str_format(
        "Average compressed throughput: %s",
        format_throughput_bytes(m_load_stats.file_bytes_read,
                                m_progress_thread.duration().seconds())
            .c_str()));
  }

  console->print_status(shcore::str_format(
      "Average statement throughput: %s",
      format_throughput_bytes(m_load_stats.statements_executed,
                              m_progress_thread.duration().seconds())
          .c_str()));
}

}  // namespace binlog
}  // namespace mysqlsh
