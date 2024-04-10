/*
 * Copyright (c) 2020, 2024, Oracle and/or its affiliates.
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

#include "modules/util/load/dump_loader.h"

#include <mysqld_error.h>

#include <algorithm>
#include <cassert>
#include <condition_variable>
#include <functional>
#include <iterator>
#include <list>
#include <mutex>
#include <string>
#include <string_view>
#include <thread>
#include <utility>
#include <vector>

#include "modules/mod_utils.h"
#include "modules/util/common/dump/utils.h"
#include "modules/util/dump/capability.h"
#include "modules/util/dump/schema_dumper.h"
#include "modules/util/import_table/load_data.h"
#include "modules/util/load/load_errors.h"
#include "modules/util/load/load_progress_log.h"
#include "mysqlshdk/include/scripting/shexcept.h"
#include "mysqlshdk/include/shellcore/console.h"
#include "mysqlshdk/include/shellcore/shell_init.h"
#include "mysqlshdk/libs/db/utils_error.h"
#include "mysqlshdk/libs/mysql/instance.h"
#include "mysqlshdk/libs/mysql/utils.h"
#include "mysqlshdk/libs/storage/compressed_file.h"
#include "mysqlshdk/libs/utils/debug.h"
#include "mysqlshdk/libs/utils/fault_injection.h"
#include "mysqlshdk/libs/utils/strformat.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_json.h"
#include "mysqlshdk/libs/utils/utils_lexing.h"
#include "mysqlshdk/libs/utils/utils_mysql_parsing.h"
#include "mysqlshdk/libs/utils/utils_path.h"
#include "mysqlshdk/libs/utils/utils_sqlstring.h"
#include "mysqlshdk/libs/utils/version.h"

namespace mysqlsh {

#if defined(__clang__)
// don't suggest braces when initializing structures using brace elision
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-braces"
#endif

using mysqlshdk::utils::Version;

FI_DEFINE(dump_loader, [](const mysqlshdk::utils::FI::Args &args) {
  throw std::runtime_error(args.get_string("msg"));
});

// how many seconds the server should wait to finish reading data from client
// basically how long it may take for a block of data to be read from its
// source (download + decompression)
static constexpr const int k_mysql_server_net_read_timeout = 30 * 60;

// number of seconds before server disconnects idle clients
// load can take a long time and some of the connections will be idle
// meanwhile so this needs to be high
static constexpr const int k_mysql_server_wait_timeout = 365 * 24 * 60 * 60;

// the version of the dump we support in this code
static constexpr const int k_supported_dump_version_major = 2;
static constexpr const int k_supported_dump_version_minor = 0;

// Multiplier for bytesPerChunk which determines how big a chunk can actually be
// before we enable sub-chunking for it.
static constexpr const auto k_chunk_size_overshoot_tolerance = 1.5;

namespace {

using Session_ptr = std::shared_ptr<mysqlshdk::db::mysql::Session>;
using Reconnect = Dump_loader::Reconnect;

constexpr auto k_temp_table_comment =
    "mysqlsh-tmp-218ffeec-b67b-4886-9a8c-d7812a1f93eb";

namespace sql {

inline std::shared_ptr<mysqlshdk::db::IResult> query(const Session_ptr &session,
                                                     std::string_view sql) {
  return session->query(sql);
}

template <typename... Args>
inline std::shared_ptr<mysqlshdk::db::IResult> queryf(
    const Session_ptr &session, const char *sql, Args &&... args) {
  return session->queryf(sql, std::forward<Args>(args)...);
}

inline void execute(const Session_ptr &session, std::string_view sql) {
  session->execute(sql);
}

template <typename... Args>
inline void executef(const Session_ptr &session, const char *sql,
                     Args &&... args) {
  session->executef(sql, std::forward<Args>(args)...);
}

namespace ar {  // auto-reconnect

template <typename F, typename... Args>
auto run(const Reconnect &reconnect, const F &execute, Args &&... args) {
  bool reconnected = false;

  while (true) {
    try {
      return execute(std::forward<Args>(args)...);
    } catch (const mysqlshdk::db::Error &e) {
      if (reconnect && !reconnected &&
          mysqlshdk::db::is_server_connection_error(e.code())) {
        log_warning("Session disconnected: %s, attempting to reconnect...",
                    e.format().c_str());
        reconnect();
        reconnected = true;
      } else {
        throw;
      }
    }
  }
}

inline std::shared_ptr<mysqlshdk::db::IResult> query(const Reconnect &reconnect,
                                                     const Session_ptr &session,
                                                     std::string_view sql) {
  return run(reconnect, [&session, sql]() { return session->query(sql); });
}

template <typename... Args>
inline std::shared_ptr<mysqlshdk::db::IResult> queryf(
    const Reconnect &reconnect, const Session_ptr &session, std::string sql,
    Args &&... args) {
  return run(
      reconnect,
      [&session, sql = std::move(sql)](auto &&... a) {
        return session->queryf(std::move(sql), std::forward<decltype(a)>(a)...);
      },
      std::forward<Args>(args)...);
}

inline void execute(const Reconnect &reconnect, const Session_ptr &session,
                    std::string_view sql) {
  return run(reconnect, [&session, sql]() { session->execute(sql); });
}

template <typename... Args>
inline void executef(const Reconnect &reconnect, const Session_ptr &session,
                     std::string sql, Args &&... args) {
  return run(
      reconnect,
      [&session, sql = std::move(sql)](auto &&... a) {
        session->executef(std::move(sql), std::forward<decltype(a)>(a)...);
      },
      std::forward<Args>(args)...);
}

}  // namespace ar
}  // namespace sql

bool histograms_supported(const Version &version) {
  return version > Version(8, 0, 0);
}

compatibility::Deferred_statements preprocess_table_script_for_indexes(
    std::string *script, const std::string &key, bool fulltext_only) {
  compatibility::Deferred_statements stmts;
  auto script_length = script->length();
  std::istringstream stream(*script);
  script->clear();
  mysqlshdk::utils::iterate_sql_stream(
      &stream, script_length,
      [&](std::string_view s, std::string_view delim, size_t, size_t) {
        auto sql = shcore::str_format(
            "%.*s%.*s\n", static_cast<int>(s.length()), s.data(),
            static_cast<int>(delim.length()), delim.data());

        mysqlshdk::utils::SQL_iterator sit(sql);
        if (shcore::str_caseeq(sit.next_token(), "CREATE") &&
            shcore::str_caseeq(sit.next_token(), "TABLE")) {
          assert(stmts.rewritten.empty());

          stmts = compatibility::check_create_table_for_indexes(sql, key,
                                                                fulltext_only);
          sql = stmts.rewritten;
        }
        script->append(sql);
        return true;
      },
      [&key](std::string_view err) {
        THROW_ERROR(SHERR_LOAD_SPLITTING_DDL_FAILED, key.c_str(),
                    std::string{err}.c_str());
      });
  return stmts;
}

void add_invisible_pk(std::string *script, const std::string &key) {
  const auto script_length = script->length();
  std::istringstream stream(*script);

  script->clear();

  mysqlshdk::utils::iterate_sql_stream(
      &stream, script_length,
      [&](std::string_view s, std::string_view delim, size_t, size_t) {
        auto sql = shcore::str_format(
            "%.*s%.*s\n", static_cast<int>(s.length()), s.data(),
            static_cast<int>(delim.length()), delim.data());

        mysqlshdk::utils::SQL_iterator sit(sql);

        if (shcore::str_caseeq(sit.next_token(), "CREATE") &&
            shcore::str_caseeq(sit.next_token(), "TABLE")) {
          compatibility::add_pk_to_create_table(sql, &sql);
        }

        script->append(sql);
        return true;
      },
      [&key](std::string_view err) {
        THROW_ERROR(SHERR_LOAD_SPLITTING_DDL_FAILED, key.c_str(),
                    std::string{err}.c_str());
      });
}

/**
 * Note: statement is going to be executed without a reconnection in case of a
 * lost connection.
 */
void execute_statement(const Session_ptr &session, std::string_view stmt,
                       const std::string &error_prefix, int silent_error = -1) {
  assert(!error_prefix.empty());

  constexpr uint32_t k_max_retry_time = 5 * 60 * 1000;  // 5 minutes
  uint32_t sleep_time = 200;  // start with 200 ms, double it with each sleep
  uint32_t total_sleep_time = 0;

  while (true) {
    try {
      sql::execute(session, stmt);
      return;
    } catch (const mysqlshdk::db::Error &e) {
      const std::string stmt_str{stmt};
      const auto sensitive =
          compatibility::contains_sensitive_information(stmt_str);
      constexpr std::string_view replacement = "'****'";
      std::vector<std::string> replaced;
      const auto &query = sensitive ? compatibility::replace_quoted_strings(
                                          stmt_str, replacement, &replaced)
                                    : stmt_str;
      auto error = e.format();

      for (const auto &replace : replaced) {
        error = shcore::str_replace(error, replace, replacement);
      }

      log_info("Error executing SQL: %s:\n%s", error.c_str(), query.c_str());

      if (ER_LOCK_DEADLOCK == e.code() && total_sleep_time < k_max_retry_time) {
        current_console()->print_note(error_prefix +
                                      ", will retry after delay: " + error);

        if (total_sleep_time + sleep_time > k_max_retry_time) {
          sleep_time = k_max_retry_time - total_sleep_time;
        }

        shcore::sleep_ms(sleep_time);

        total_sleep_time += sleep_time;
        sleep_time *= 2;
      } else {
        if (silent_error != e.code()) {
          current_console()->print_error(
              shcore::str_format("%s: %s: %s", error_prefix.c_str(),
                                 error.c_str(), query.c_str()));
        }

        throw;
      }
    }
  }
}

/**
 * Note: this assumes that script is idempotent and can be executed again in
 * case of a reconnection.
 */
void execute_script(
    const Reconnect &reconnect, const Session_ptr &session,
    const std::string &script, const std::string &error_prefix,
    const std::function<bool(std::string_view, std::string *)> &process_stmt) {
  assert(reconnect);

  sql::ar::run(reconnect, [&]() {
    std::stringstream stream(script);
    std::string new_stmt;

    mysqlshdk::utils::iterate_sql_stream(
        &stream, 1024 * 64,
        [&](std::string_view s, std::string_view, size_t, size_t) {
          if (process_stmt && process_stmt(s, &new_stmt)) {
            s = std::move(new_stmt);
          }

          if (!s.empty()) {
            execute_statement(session, s, error_prefix);
          }

          return true;
        },
        [](std::string_view err) {
          current_console()->print_error(std::string{err});
        });
  });
}

std::string format_table(std::string_view schema, std::string_view table,
                         std::string_view partition, ssize_t chunk) {
  std::string result = "`";
  result += schema;
  result += "`.`";
  result += table;
  result += '`';

  if (!partition.empty()) {
    result += " partition `";
    result += partition;
    result += '`';
  }

  if (chunk >= 0) {
    result += " (chunk ";
    result += std::to_string(chunk);
    result += ')';
  }

  return result;
}

std::string format_table(
    const dump::common::Checksums::Checksum_data *checksum) {
  return format_table(checksum->schema(), checksum->table(),
                      checksum->partition(), checksum->chunk());
}

std::string format_table(const Dump_reader::Table_chunk &chunk) {
  return format_table(chunk.schema, chunk.table, chunk.partition, chunk.index);
}

std::string worker_id(size_t id) {
  return shcore::str_format("[Worker%03zu]: ", id);
}

void mark_temp_table(const Reconnect &reconnect, const Session_ptr &session,
                     const std::string &schema, const std::string &table) {
  sql::ar::executef(reconnect, session, "ALTER TABLE !.! COMMENT=?", schema,
                    table, k_temp_table_comment);
}

std::string format_rows_throughput(const uint64_t rows, double seconds) {
  return mysqlshdk::utils::format_throughput_items("row", "rows", rows, seconds,
                                                   true);
}

std::string use_schema(const std::string &schema, const std::string &script) {
  auto result = shcore::sqlformat("USE !;", schema);
  result += '\n';
  result += script;

  return result;
}

}  // namespace

class Dump_loader::Monitoring final {
 public:
  using Monitor = std::function<void(const Session_ptr &)>;

  explicit Monitoring(Dump_loader *loader) : m_loader(loader) {
    m_thread = mysqlsh::spawn_scoped_thread([this]() { monitoring_thread(); });
  }

  ~Monitoring() {
    m_terminating = true;
    m_thread.join();
  }

  void add(Monitor m) {
    std::lock_guard lock{m_monitors_mutex};
    m_monitors.emplace_back(std::move(m));
  }

  void add_worker(const Worker *w) {
    std::lock_guard lock{m_workers_mutex};
    m_workers.emplace(w);
  }

  void remove_worker(const Worker *w) {
    std::lock_guard lock{m_workers_mutex};
    m_workers.erase(w);
  }

 private:
  void monitoring_thread() {
    try {
      mysqlsh::Mysql_thread mysql_thread;
      const auto session = m_loader->create_session();

      while (!m_terminating) {
        if (m_loader->m_worker_hard_interrupt) {
          kill_queries(session);
        } else {
          monitor(session);
        }

        shcore::sleep_ms(250);
      }
    } catch (const std::exception &e) {
      log_error("Monitoring thread: %s", e.what());
    }
  }

  void kill_queries(const Session_ptr &session) {
    std::lock_guard lock{m_workers_mutex};

    for (const auto worker : m_workers) {
      try {
        // this is a monitoring session, which should be in continuous use, no
        // need to reconnect here
        sql::execute(session,
                     "KILL QUERY " + std::to_string(worker->connection_id()));
      } catch (const std::exception &e) {
        log_warning("Error canceling SQL connection %" PRIu64 ": %s",
                    worker->connection_id(), e.what());
      }
    }
  }

  void monitor(const Session_ptr &session) {
    std::lock_guard lock{m_monitors_mutex};
    for (const auto &m : m_monitors) {
      m(session);
    }
  }

  Dump_loader *m_loader;
  std::atomic_bool m_terminating = false;
  std::thread m_thread;
  std::mutex m_monitors_mutex;
  std::vector<Monitor> m_monitors;
  std::mutex m_workers_mutex;
  std::unordered_set<const Worker *> m_workers;
};

class Dump_loader::Bulk_load_support {
 public:
  explicit Bulk_load_support(Dump_loader *loader)
      : m_loader(loader),
        m_bulk_load_info(loader->m_options.bulk_load_info()) {}

  std::size_t compatible_tables() const noexcept { return m_compatible_tables; }

  void on_bulk_load_start(Worker *worker, Worker::Bulk_load_task *task) {
    {
      std::lock_guard lock{m_monitored_threads_mutex};

      // we want to avoid bloating the progress file with progress update
      // entries, so we try to have the same number of entries as when loading
      // without BULK LOAD: two per loaded file
      const auto next_progress_update = 1.0 / (2 * task->chunk().chunks_total);

      m_monitored_threads.emplace(
          worker->thread_id(), Thread_info{worker, task, next_progress_update,
                                           next_progress_update});
    }

    if (m_bulk_load_info.monitoring && !m_monitoring_started) {
      m_loader->m_monitoring->add(
          [this](const Session_ptr &session) { report_progress(session); });
      m_monitoring_started = true;
    }

    DBUG_EXECUTE_IF("dump_loader_bulk_interrupt", { m_loader->abort(); });
  }

  void on_bulk_load_end(Worker *worker) {
    std::size_t data_bytes_update = 0;
    std::size_t file_bytes_update = 0;

    {
      std::lock_guard lock{m_monitored_threads_mutex};

      if (const auto it = m_monitored_threads.find(worker->thread_id());
          m_monitored_threads.end() != it) {
        const auto &info = it->second;
        const auto &chunk = info.task->chunk();

        data_bytes_update = chunk.data_size - info.data_bytes_reported;
        file_bytes_update = chunk.file_size - info.file_bytes_reported;

        m_monitored_threads.erase(it);
      }
    }

    // report the last chunk of loaded data
    m_loader->m_stats.total_data_bytes += data_bytes_update;
    m_loader->m_stats.total_file_bytes += file_bytes_update;
  }

  static import_table::Import_table_options import_options(
      const Dump_reader::Table_chunk &chunk, const std::string &table) {
    // NOTE: partition is not a part of chunk.options, so it doesn't need to be
    //       removed even if table is partitioned
    auto import_options =
        import_table::Import_table_options::unpack(chunk.options);

    // BULK LOAD does not support REPLACE and IGNORE
    import_options.set_duplicate_handling(
        import_table::Duplicate_handling::Default);
    import_options.set_verbose(false);
    import_options.set_table(table);
    import_options.set_compression(chunk.compression);

    // BULK LOAD does not support column list specification or input
    // preprocessing. Dumper always provides a list of columns, which is a
    // filtered list of all columns (with generated columns removed). Given that
    // BULK LOAD does not support tables with generated columns, we can safely
    // remove this list. Dumper also sometimes provides a list of columns which
    // are encoded, and need to be preprocessed while loading. Since this is not
    // supported, we don't remove the column list in such case, so that the test
    // load fails, and loader falls back to regular load.
    if (import_options.decode_columns().empty()) {
      import_options.clear_columns();
    }

    return import_options;
  }

  std::string load_statement(const Dump_reader::Table_chunk &chunk,
                             const std::string &table, bool dry_run) const {
    return load_statement(chunk, import_options(chunk, table), dry_run);
  }

  std::string load_statement(const Dump_reader::Table_chunk &chunk,
                             const import_table::Import_table_options &options,
                             bool dry_run) const {
    std::string result = "LOAD DATA FROM ";

    switch (m_bulk_load_info.fs) {
      case Load_dump_options::Bulk_load_fs::UNSUPPORTED:
        throw std::logic_error("BULK LOAD: trying to use an unsupported FS");

      case Load_dump_options::Bulk_load_fs::INFILE:
        result += "INFILE";
        break;

      case Load_dump_options::Bulk_load_fs::URL:
        result += "URL";
        break;

      case Load_dump_options::Bulk_load_fs::S3:
        result += "S3";
        break;
    }

    {
      auto path = m_bulk_load_info.file_prefix;
      path += chunk.file->full_path().real();

      if (chunk.chunks_total > 1) {
        // multiple files - use a prefix, trim to the last '@'
        while ('@' != path.back()) {
          path.pop_back();
        }
      } else {
        // only one file - use a full path
        assert(1 == chunk.chunks_total);
      }

      shcore::JSON_dumper json;
      json.start_object();
      json.append("url-prefix", path);

      if (chunk.chunks_total > 1) {
        json.append("url-suffix", "." + chunk.extension);
        json.append_int("url-sequence-start", 0);
        json.append("url-prefix-last-append", "@");
      }

      if (dry_run) {
        json.append("is-dryrun", true);
      }

      json.end_object();

      result += " '";
      result += json.str();
      result += '\'';
    }

    if (chunk.chunks_total > 1) {
      result += " COUNT ";
      result += std::to_string(chunk.chunks_total);
    }

    result += " IN PRIMARY KEY ORDER ";
    result += import_table::Load_data_worker::load_data_body(options);
    result += " ALGORITHM=BULK";

    return result;
  }

  bool ensure_table_compatible(const Dump_reader::Table_chunk &chunk,
                               const Reconnect &reconnect,
                               const Session_ptr &session) {
    // check in cache first
    if (const auto s = m_compatibility_status.find(chunk.schema);
        m_compatibility_status.end() != s) {
      if (const auto t = s->second.find(chunk.table); s->second.end() != t) {
        return t->second;
      }
    }

    auto table_name = chunk.table;
    shcore::on_leave_scope cleanup;

    if (!chunk.partition.empty()) {
      cleanup = shcore::on_leave_scope{
          copy_table_remove_partition(chunk, reconnect, session, &table_name)};

      // bulk load requires non-empty tables, we're creating a new one here to
      // run the compatibility test and need to make sure that if original table
      // has some data, copied has it as well
      sql::ar::executef(reconnect, session,
                        "INSERT IGNORE INTO !.! SELECT * FROM !.! LIMIT 1",
                        chunk.schema, table_name, chunk.schema, chunk.table);
    }

    const auto compatible =
        is_table_compatible(chunk, reconnect, session, table_name);

    m_compatibility_status[chunk.schema][chunk.table] = compatible;

    if (compatible) {
      ++m_compatible_tables;
    }

    return compatible;
  }

  std::function<void()> copy_table_remove_partition(
      const Dump_reader::Table_chunk &chunk, const Reconnect &reconnect,
      const Session_ptr &session, std::string *new_table) const {
    assert(new_table);

    auto copied = m_loader->temp_table_name();

    // CREATE TABLE ... LIKE does not preserve DATA DIRECTORY or INDEX DIRECTORY
    // table options, nor foreign key definitions, but DIRECTORY options are not
    // supported in OCI, while partitioned InnoDB tables cannot have foreign
    // keys
    // statement is not idempotent - do not reconnect
    sql::executef(session, "CREATE TABLE !.! LIKE !.!", chunk.schema, copied,
                  chunk.schema, chunk.table);

    log_info("Created a temporary table for BULK LOAD: `%s`.`%s`",
             chunk.schema.c_str(), copied.c_str());

    auto drop_table = [reconnect, session, schema = chunk.schema,
                       table = copied]() {
      sql::ar::executef(reconnect, session, "DROP TABLE IF EXISTS !.!", schema,
                        table);

      log_info("Deleted the temporary table for BULK LOAD: `%s`.`%s`",
               schema.c_str(), table.c_str());
    };
    shcore::on_leave_scope cleanup{drop_table};

    mark_temp_table(reconnect, session, chunk.schema, copied);
    // statement is not idempotent - do not reconnect
    sql::executef(session, "ALTER TABLE !.! REMOVE PARTITIONING", chunk.schema,
                  copied);

    *new_table = std::move(copied);

    cleanup.cancel();
    return drop_table;
  }

  void on_bulk_load_thread_start() {
    std::lock_guard lock{m_running_threads_mutex};
    ++m_running_threads;
  }

  void on_bulk_load_thread_end() {
    {
      std::lock_guard lock{m_running_threads_mutex};
      assert(m_running_threads > 0);
      --m_running_threads;
    }

    m_running_threads_cv.notify_all();
  }

  bool wait_for_retry(const mysqlshdk::db::Error &e) {
    if (m_loader->m_worker_interrupt) {
      return false;
    }

    const auto code = e.code();

    // ER_BULK_EXECUTOR_ERROR is reported in case of various resource-related
    // problems, but also in case of failed initialization/finalization, for now
    // we don't filter error messages...
    if (ER_BULK_EXECUTOR_ERROR != code && ER_BULK_LOAD_RESOURCE != code) {
      return false;
    }

    std::unique_lock lock{m_running_threads_mutex};

    if (1 == m_running_threads) {
      // there's only one thread running, there's no recovery from this
      return false;
    }

    m_running_threads_cv.wait(lock,
                              [this, want_threads = m_running_threads - 1]() {
                                // wait for some other threads to finish
                                return m_running_threads <= want_threads;
                              });

    return true;
  }

 private:
  struct Thread_info {
    Worker *worker;
    Worker::Bulk_load_task *task;
    const double update_progress_every;
    double next_progress_update;
    std::size_t data_bytes_reported = 0;
    std::size_t file_bytes_reported = 0;
  };

  void report_progress(const Session_ptr &session) {
    std::lock_guard lock{m_monitored_threads_mutex};

    if (m_monitored_threads.empty()) {
      return;
    }

    // we don't filter here, we should be the only ones to BULK LOAD
    // no reconnection - we're using the monitoring session
    const auto result =
        sql::query(session,
                   "SELECT THREAD_ID, WORK_ESTIMATED, WORK_COMPLETED FROM "
                   "performance_schema.events_stages_current WHERE "
                   "EVENT_NAME='stage/bulk_load_sorted/loading'");

    std::size_t data_bytes_update = 0;
    std::size_t file_bytes_update = 0;

    while (const auto row = result->fetch_one()) {
      const auto thread_id = row->get_uint(0);  // NOT NULL

      if (const auto it = m_monitored_threads.find(thread_id);
          m_monitored_threads.end() != it) {
        const auto estimated = row->get_uint(1, 0);  // NULL

        if (!estimated) {
          continue;
        }

        const auto completed = row->get_uint(2, 0);  // NULL

        if (!completed) {
          continue;
        }

        auto &info = it->second;
        const auto &chunk = info.task->chunk();
        const auto progress =
            std::min(static_cast<double>(completed) / estimated, 1.0);

        data_bytes_update += update_progress(progress, chunk.data_size,
                                             &info.data_bytes_reported);
        file_bytes_update += update_progress(progress, chunk.file_size,
                                             &info.file_bytes_reported);

        if (progress >= info.next_progress_update) {
          m_loader->post_worker_event(
              info.worker, Worker_event::BULK_LOAD_PROGRESS,
              shcore::make_dict(
                  "schema", chunk.schema, "table", chunk.table, "partition",
                  chunk.partition, "data_bytes",
                  static_cast<uint64_t>(info.data_bytes_reported)));

          while (info.next_progress_update <= progress) {
            info.next_progress_update += info.update_progress_every;
          }
        }
      }
    }

    m_loader->m_stats.total_data_bytes += data_bytes_update;
    m_loader->m_stats.total_file_bytes += file_bytes_update;
  }

  static size_t update_progress(double progress, std::size_t total,
                                std::size_t *current) {
    const auto updated = std::min(static_cast<size_t>(progress * total), total);

    if (updated <= *current) {
      // protect from overflow
      return 0;
    }

    const auto delta = updated - *current;
    *current = updated;

    return delta;
  }

  bool is_table_compatible(const Dump_reader::Table_chunk &chunk,
                           const Reconnect &reconnect,
                           const Session_ptr &session,
                           const std::string &table) const {
    try {
      sql::ar::execute(reconnect, session, load_statement(chunk, table, true));
      return true;
    } catch (const mysqlshdk::db::Error &e) {
      log_info("Table %s is not compatible with BULK LOAD: %s",
               schema_object_key(chunk.schema, chunk.table).c_str(),
               e.format().c_str());
      return false;
    }
  }

  Dump_loader *m_loader;
  const Load_dump_options::Bulk_load_info &m_bulk_load_info;

  bool m_monitoring_started = false;
  std::mutex m_monitored_threads_mutex;
  std::unordered_map<uint64_t, Thread_info> m_monitored_threads;

  std::unordered_map<std::string, std::unordered_map<std::string, bool>>
      m_compatibility_status;
  std::size_t m_compatible_tables = 0;

  std::mutex m_running_threads_mutex;
  int m_running_threads = 0;
  std::condition_variable m_running_threads_cv;
};

void Dump_loader::Worker::Task::set_id(size_t id) {
  m_id = id;
  m_log_id = worker_id(m_id);
}

void Dump_loader::Worker::Task::handle_current_exception(
    Worker *worker, Dump_loader *loader, const std::string &error) {
  worker->handle_current_exception(loader, error);
}

bool Dump_loader::Worker::Schema_ddl_task::execute(Worker *worker,
                                                   Dump_loader *loader) {
  log_debug("%swill execute DDL for schema %s", log_id(), schema().c_str());

  loader->post_worker_event(worker, Worker_event::SCHEMA_DDL_START);

  if (!m_script.empty()) {
    try {
      log_info("%s DDL script for schema `%s`",
               m_resuming ? "Re-executing" : "Executing", schema().c_str());

      if (!loader->m_options.dry_run()) {
        auto transforms = loader->m_default_sql_transforms;

        transforms.add_execute_conditionally(
            [this, loader](std::string_view type, const std::string &name) {
              bool execute = true;

              if (shcore::str_caseeq(type, "EVENT")) {
                execute = loader->m_dump->include_event(schema(), name);
              } else if (shcore::str_caseeq(type, "FUNCTION", "PROCEDURE")) {
                execute = loader->m_dump->include_routine(schema(), name);
              }

              return execute;
            });

        // execute sql
        execute_script(worker->reconnect_callback(), worker->session(),
                       m_script,
                       shcore::str_format("While processing schema `%s`",
                                          schema().c_str()),
                       transforms);
      }
    } catch (const std::exception &e) {
      current_console()->print_error(shcore::str_format(
          "Error processing schema `%s`: %s", schema().c_str(), e.what()));

      if (loader->m_options.force()) {
        loader->add_skipped_schema(schema());
      } else {
        handle_current_exception(
            worker, loader,
            shcore::str_format("While executing DDL for schema %s: %s",
                               schema().c_str(), e.what()));
        return false;
      }
    }
  }

  // need to update status from the worker thread, 'cause the main thread may be
  // looking for a task and doesn't have time to process events
  loader->m_schema_ddl_ready[schema()] = true;

  log_debug("%sdone", log_id());
  ++loader->m_ddl_executed;
  loader->post_worker_event(worker, Worker_event::SCHEMA_DDL_END);

  return true;
}

bool Dump_loader::Worker::Table_ddl_task::execute(Worker *worker,
                                                  Dump_loader *loader) {
  log_debug("%swill execute DDL file for table %s", log_id(), key().c_str());

  loader->post_worker_event(worker, Worker_event::TABLE_DDL_START);

  try {
    pre_process(loader);

    if (!loader->m_options.dry_run()) {
      // this is here to detect if data is loaded into a non-existing schema
      sql::ar::executef(worker->reconnect_callback(), worker->session(),
                        "use !", m_schema.c_str());
    }

    load_ddl(worker, loader);

    post_process(worker, loader);
  } catch (const std::exception &e) {
    handle_current_exception(
        worker, loader,
        shcore::str_format("While executing DDL script for %s: %s",
                           key().c_str(), e.what()));
    return false;
  }

  log_debug("%sdone", log_id());
  ++loader->m_ddl_executed;
  loader->post_worker_event(worker, Worker_event::TABLE_DDL_END);

  return true;
}

void Dump_loader::Worker::Table_ddl_task::pre_process(Dump_loader *loader) {
  if (!m_placeholder && (loader->m_options.load_ddl() ||
                         loader->m_options.load_deferred_indexes())) {
    if (loader->m_options.defer_table_indexes() !=
        Load_dump_options::Defer_index_mode::OFF) {
      extract_deferred_statements(loader);
    }

    if (loader->m_options.load_ddl() && loader->should_create_pks() &&
        !loader->m_options.auto_create_pks_supported() &&
        !loader->m_dump->has_primary_key(m_schema, m_table)) {
      add_invisible_pk(&m_script, key());
    }
  }
}

void Dump_loader::Worker::Table_ddl_task::extract_deferred_statements(
    Dump_loader *loader) {
  const auto &table_name = key();
  const auto fulltext_only = loader->m_options.defer_table_indexes() ==
                             Load_dump_options::Defer_index_mode::FULLTEXT;

  m_deferred_statements = std::make_unique<compatibility::Deferred_statements>(
      preprocess_table_script_for_indexes(&m_script, table_name,
                                          fulltext_only));
}

void Dump_loader::Worker::Table_ddl_task::post_process(Worker *worker,
                                                       Dump_loader *loader) {
  if ((m_status == Load_progress_log::DONE ||
       loader->m_options.ignore_existing_objects()) &&
      m_deferred_statements && !m_deferred_statements->empty()) {
    remove_duplicate_deferred_statements(worker, loader);
  }
}

void Dump_loader::Worker::Table_ddl_task::remove_duplicate_deferred_statements(
    Worker *worker, Dump_loader *loader) {
  // this handles the case where the table was already created (either in a
  // previous run or by the user) and some indexes may already have been
  // re-created

  const auto &table_name = key();
  const auto fulltext_only = loader->m_options.defer_table_indexes() ==
                             Load_dump_options::Defer_index_mode::FULLTEXT;

  std::string ct;

  try {
    ct = sql::ar::query(worker->reconnect_callback(), worker->session(),
                        "show create table " + table_name)
             ->fetch_one()
             ->get_string(1);
  } catch (const mysqlshdk::db::Error &e) {
    if ((ER_BAD_DB_ERROR == e.code() || ER_NO_SUCH_TABLE == e.code()) &&
        loader->m_options.dry_run()) {
      // table may not exists if we're running in dryRun mode, this is not an
      // error; there are no duplicates
      return;
    } else {
      // in any other case we throw an exception, table should exist at this
      // point
      throw;
    }
  }

  const auto recreated = compatibility::check_create_table_for_indexes(
      ct, table_name, fulltext_only);

  const auto remove_duplicates = [&table_name](
                                     const std::vector<std::string> &needles,
                                     std::vector<std::string> *haystack) {
    for (const auto &n : needles) {
      const auto pos = std::remove(haystack->begin(), haystack->end(), n);

      if (haystack->end() != pos) {
        log_info(
            "Table %s already contains '%s', it's not going to be deferred.",
            table_name.c_str(), n.c_str());

        haystack->erase(pos, haystack->end());
      }
    }
  };

  remove_duplicates(recreated.index_info.fulltext,
                    &m_deferred_statements->index_info.fulltext);
  remove_duplicates(recreated.index_info.spatial,
                    &m_deferred_statements->index_info.spatial);
  remove_duplicates(recreated.index_info.regular,
                    &m_deferred_statements->index_info.regular);
  remove_duplicates(recreated.foreign_keys,
                    &m_deferred_statements->foreign_keys);

  if (!recreated.secondary_engine.empty()) {
    if (m_deferred_statements->has_alters()) {
      // recreated table already has a secondary engine (possibly table was
      // modified by the user), but not all indexes were applied, we're unable
      // to continue, as ALTER TABLE statements will fail
      THROW_ERROR(SHERR_LOAD_SECONDARY_ENGINE_ERROR, table_name.c_str());
    } else {
      // recreated table has all the indexes and the secondary engine in place,
      // nothing more to do
      m_deferred_statements->secondary_engine.clear();
    }
  }
}

void Dump_loader::Worker::Table_ddl_task::load_ddl(Worker *worker,
                                                   Dump_loader *loader) {
  if (m_status == Load_progress_log::DONE || !loader->m_options.load_ddl()) {
    return;
  }

  log_debug("%sExecuting %stable DDL for %s", log_id(),
            m_placeholder ? "placeholder " : "", key().c_str());
  log_info("%s%s DDL script for %s%s", log_id(),
           (m_status == Load_progress_log::INTERRUPTED ? "Re-executing"
                                                       : "Executing"),
           key().c_str(),
           (m_placeholder
                ? " (placeholder for view)"
                : (m_deferred_statements && m_deferred_statements->has_alters()
                       ? " (indexes removed for deferred creation)"
                       : "")));

  if (m_exists) {
    // BUG#35102738: do not recreate existing tables due to BUG#35154429
    log_info("%sskipping DDL script for %s, table already exists", log_id(),
             key().c_str());
    return;
  }

  if (!loader->m_options.dry_run()) {
    try {
      // execute sql
      execute_script(worker->reconnect_callback(), worker->session(), m_script,
                     shcore::str_format("%sError processing table %s", log_id(),
                                        key().c_str()),
                     loader->m_default_sql_transforms);
    } catch (const std::exception &e) {
      if (!loader->m_options.force()) throw;
    }
  }
}

std::string Dump_loader::Worker::Table_data_task::query_comment() const {
  std::string query_comment = "/* mysqlsh loadDump(), thread " +
                              std::to_string(id()) + ", table " +
                              shcore::str_replace(key(), "*/", "*\\/");
  if (chunk_index() >= 0) {
    query_comment += ", chunk ID: " + std::to_string(chunk_index());
  }
  query_comment += " */ ";
  return query_comment;
}

bool Dump_loader::Worker::Load_data_task::execute(Worker *worker,
                                                  Dump_loader *loader) {
  const auto masked_path = m_chunk.file->full_path().masked();

  log_debug("%swill load %s for table %s", log_id(), masked_path.c_str(),
            key().c_str());

  try {
    on_load_start(worker, loader);

    // do work
    if (!loader->m_options.dry_run()) {
      // load the data
      load(loader, worker);
    }

    log_debug("%sdone", log_id());
    on_load_end(worker, loader);
  } catch (const std::exception &e) {
    handle_current_exception(worker, loader,
                             shcore::str_format("While loading %s: %s",
                                                masked_path.c_str(), e.what()));
    return false;
  }

  return true;
}

void Dump_loader::Worker::Load_data_task::load(Dump_loader *loader,
                                               Worker *worker) {
  loader->m_num_threads_loading++;

  shcore::on_leave_scope cleanup([this, loader]() {
    std::lock_guard<std::mutex> lock(loader->m_tables_being_loaded_mutex);
    auto it = loader->m_tables_being_loaded.find(key());
    while (it != loader->m_tables_being_loaded.end() && it->first == key()) {
      if (it->second == chunk().file_size) {
        loader->m_tables_being_loaded.erase(it);
        break;
      }
      ++it;
    }

    loader->m_num_threads_loading--;
  });

  do_load(worker, loader);

  if (loader->m_thread_exceptions[id()]) {
    std::rethrow_exception(loader->m_thread_exceptions[id()]);
  }

  // NOTE: total_data_bytes and total_file_bytes are used by the loader to
  // measure the progress, and need to be updated while data is being loaded

  loader->m_stats.total_records += stats.total_records;
  loader->m_stats.total_deleted += stats.total_deleted;
  loader->m_stats.total_skipped += stats.total_skipped;
  loader->m_stats.total_warnings += stats.total_warnings;
  loader->m_stats.total_files_processed += stats.total_files_processed;
}

void Dump_loader::Worker::Load_chunk_task::on_load_start(Worker *worker,
                                                         Dump_loader *loader) {
  FI_TRIGGER_TRAP(dump_loader, mysqlshdk::utils::FI::Trigger_options(
                                   {{"op", "BEFORE_LOAD_START"},
                                    {"schema", schema()},
                                    {"table", table()},
                                    {"chunk", std::to_string(chunk().index)}}));

  loader->post_worker_event(worker, Worker_event::LOAD_START);

  FI_TRIGGER_TRAP(dump_loader, mysqlshdk::utils::FI::Trigger_options(
                                   {{"op", "AFTER_LOAD_START"},
                                    {"schema", schema()},
                                    {"table", table()},
                                    {"chunk", std::to_string(chunk().index)}}));
}

void Dump_loader::Worker::Load_chunk_task::do_load(Worker *worker,
                                                   Dump_loader *loader) {
  auto import_options =
      import_table::Import_table_options::unpack(chunk().options);

  // replace duplicate rows by default
  import_options.set_duplicate_handling(
      import_table::Duplicate_handling::Replace);
  import_options.set_verbose(false);
  import_options.set_partition(chunk().partition);

  if (resume()) {
    const auto has_pke = [worker, this]() {
      // Return true if the table has a PK or equivalent (UNIQUE NOT NULL)
      auto res =
          sql::ar::queryf(worker->reconnect_callback(), worker->session(),
                          "SHOW INDEX IN !.!", schema(), table());

      while (auto row = res->fetch_one_named()) {
        if (row.get_int("Non_unique") == 0 && row.get_string("Null").empty()) {
          return true;
        }
      }

      return false;
    };

    // Truncate the table if does not have a PKE, otherwise leave it and rely on
    // duplicate rows being ignored.
    if (!has_pke()) {
      current_console()->print_note(shcore::str_format(
          "Truncating table `%s`.`%s` because of resume and it "
          "has no PK or equivalent key",
          schema().c_str(), table().c_str()));
      sql::ar::executef(worker->reconnect_callback(), worker->session(),
                        "TRUNCATE TABLE !.!", schema(), table());

      m_bytes_to_skip = 0;
    }
  }

  import_table::Load_data_worker op(
      import_options, id(), &loader->m_stats.total_data_bytes,
      &loader->m_stats.total_file_bytes, &loader->m_worker_hard_interrupt,
      nullptr, &loader->m_thread_exceptions[id()], &stats, query_comment());

  {
    // If max_transaction_size > 0, chunk truncation is enabled, where LOAD
    // DATA will be truncated if the transaction size exceeds that value and
    // then retried until the whole chunk is loaded.
    //
    // The max. transaction size depends on server options like
    // max_binlog_cache_size and gr_transaction_size_limit. However, it's not
    // straightforward to calculate the transaction size from CSV data (at least
    // not without a lot of effort and cpu cycles). Thus, we instead use a
    // different approach where we assume that the value of bytesPerChunk used
    // during dumping will be sufficiently small to fit in a transaction. If any
    // chunks are bigger than that value (because approximations made during
    // dumping were not good), we will break them down further here during
    // loading.
    // Ideally, only chunks that are much bigger than the specified
    // bytesPerChunk value will get sub-chunked, chunks that are smaller or just
    // a little bigger will be loaded whole. If they still don't fit, the user
    // should dump with a smaller bytesPerChunk value.
    const auto max_bytes_per_transaction =
        loader->m_options.max_bytes_per_transaction();

    import_table::Transaction_options options;

    // if the maxBytesPerTransaction is not given, it defaults to bytesPerChunk
    // value used during the dump
    options.max_trx_size =
        max_bytes_per_transaction.value_or(loader->m_dump->bytes_per_chunk());

    if (loader->m_options.fast_sub_chunking()) {
      options.fast_sub_chunking = true;
    } else {
      uint64_t max_chunk_size = options.max_trx_size;

      // if maxBytesPerTransaction is not given, only files bigger than
      // k_chunk_size_overshoot_tolerance * bytesPerChunk are affected
      if (!max_bytes_per_transaction.has_value()) {
        max_chunk_size *= k_chunk_size_overshoot_tolerance;
      }

      if (options.max_trx_size > 0 && chunk().data_size < max_chunk_size) {
        // chunk is small enough, so don't sub-chunk
        options.max_trx_size = 0;
      }
    }

    uint64_t subchunk = 0;

    options.transaction_started = [this, &loader, &worker, &subchunk]() {
      log_debug("Transaction for '%s'.'%s'/%zi subchunk %" PRIu64
                " has started",
                schema().c_str(), table().c_str(), chunk().index, subchunk);

      FI_TRIGGER_TRAP(dump_loader,
                      mysqlshdk::utils::FI::Trigger_options(
                          {{"op", "BEFORE_LOAD_SUBCHUNK_START"},
                           {"schema", schema()},
                           {"table", table()},
                           {"chunk", std::to_string(chunk().index)},
                           {"subchunk", std::to_string(subchunk)}}));

      loader->post_worker_event(worker, Worker_event::LOAD_SUBCHUNK_START,
                                shcore::make_dict("subchunk", subchunk));

      FI_TRIGGER_TRAP(dump_loader,
                      mysqlshdk::utils::FI::Trigger_options(
                          {{"op", "AFTER_LOAD_SUBCHUNK_START"},
                           {"schema", schema()},
                           {"table", table()},
                           {"chunk", std::to_string(chunk().index)},
                           {"subchunk", std::to_string(subchunk)}}));
    };

    options.transaction_finished = [this, &loader, &worker,
                                    &subchunk](uint64_t bytes) {
      log_debug("Transaction for '%s'.'%s'/%zi subchunk %" PRIu64
                " has finished, wrote %" PRIu64 " bytes",
                schema().c_str(), table().c_str(), chunk().index, subchunk,
                bytes);

      FI_TRIGGER_TRAP(dump_loader,
                      mysqlshdk::utils::FI::Trigger_options(
                          {{"op", "BEFORE_LOAD_SUBCHUNK_END"},
                           {"schema", schema()},
                           {"table", table()},
                           {"chunk", std::to_string(chunk().index)},
                           {"subchunk", std::to_string(subchunk)}}));

      loader->post_worker_event(
          worker, Worker_event::LOAD_SUBCHUNK_END,
          shcore::make_dict("subchunk", subchunk, "bytes", bytes));

      FI_TRIGGER_TRAP(dump_loader,
                      mysqlshdk::utils::FI::Trigger_options(
                          {{"op", "AFTER_LOAD_SUBCHUNK_END"},
                           {"schema", schema()},
                           {"table", table()},
                           {"chunk", std::to_string(chunk().index)},
                           {"subchunk", std::to_string(subchunk)}}));

      ++subchunk;
    };

    options.skip_bytes = m_bytes_to_skip;
    stats.total_data_bytes += m_bytes_to_skip;

    op.execute(worker->session(), extract_file(), options);
  }
}

void Dump_loader::Worker::Load_chunk_task::on_load_end(Worker *worker,
                                                       Dump_loader *loader) {
  FI_TRIGGER_TRAP(dump_loader, mysqlshdk::utils::FI::Trigger_options(
                                   {{"op", "BEFORE_LOAD_END"},
                                    {"schema", schema()},
                                    {"table", table()},
                                    {"chunk", std::to_string(chunk().index)}}));

  // signal for more work
  loader->post_worker_event(worker, Worker_event::LOAD_END);
}

void Dump_loader::Worker::Bulk_load_task::on_load_start(Worker *worker,
                                                        Dump_loader *loader) {
  loader->post_worker_event(worker, Worker_event::BULK_LOAD_START);

  // if table is partitioned, we load into a temporary table, in that case
  // there's no need to check if table is empty
  if (resume() && !partitioned()) {
    const auto &reconnect = worker->reconnect_callback();
    const auto &session = worker->session();
    const auto result = sql::ar::query(reconnect, session,
                                       "SELECT 1 FROM " + key() + " LIMIT 1");

    if (result->fetch_one()) {
      // this should not happen, as BULK LOAD is atomic
      current_console()->print_note(
          shcore::str_format("Truncating table %s because of resume and it is "
                             "being loaded using BULK LOAD",
                             key().c_str()));
      sql::ar::execute(reconnect, session, "TRUNCATE TABLE " + key());
    }
  }
}

void Dump_loader::Worker::Bulk_load_task::do_load(Worker *worker,
                                                  Dump_loader *loader) {
  log_info("Loading %zu files into table %s using BULK LOAD",
           chunk().chunks_total, key().c_str());

  const auto handle_exception = [loader, this](const char *error) {
    loader->m_thread_exceptions[id()] = std::current_exception();
    current_console()->print_error(
        shcore::str_format("%sBULK LOAD into table %s failed: %s", log_id(),
                           key().c_str(), error));
  };

  try {
    shcore::on_leave_scope cleanup;

    if (partitioned()) {
      cleanup = shcore::on_leave_scope{
          loader->m_bulk_load->copy_table_remove_partition(
              chunk(), worker->reconnect_callback(), worker->session(),
              &m_target_table)};
    }

    bulk_load(worker, loader);

    if (partitioned()) {
      // statement is not idempotent - do not reconnect
      sql::executef(worker->session(),
                    "ALTER TABLE !.! EXCHANGE PARTITION ! WITH TABLE !.! "
                    "WITHOUT VALIDATION",
                    schema(), table(), chunk().partition, schema(),
                    m_target_table);
    }
  } catch (const mysqlshdk::db::Error &e) {
    handle_exception(e.format().c_str());
  } catch (const std::exception &e) {
    handle_exception(e.what());
  } catch (...) {
    handle_exception("unknown error");
  }
}

void Dump_loader::Worker::Bulk_load_task::on_load_end(Worker *worker,
                                                      Dump_loader *loader) {
  // signal for more work
  loader->post_worker_event(worker, Worker_event::BULK_LOAD_END);
}

bool Dump_loader::Worker::Bulk_load_task::partitioned() const {
  return !chunk().partition.empty();
}

void Dump_loader::Worker::Bulk_load_task::bulk_load(Worker *worker,
                                                    Dump_loader *loader) {
  const auto import_options =
      Bulk_load_support::import_options(chunk(), m_target_table);
  const auto &session = worker->session();

  import_table::Load_data_worker::init_session(session, import_options);
  const auto statement = query_comment() + loader->m_bulk_load->load_statement(
                                               chunk(), import_options, false);

  loader->m_bulk_load->on_bulk_load_thread_start();
  shcore::on_leave_scope cleanup{
      [loader]() { loader->m_bulk_load->on_bulk_load_thread_end(); }};

  while (true) {
    try {
      log_debug("%sExecuting bulk load", log_id());
      // this statement has a precondition above, no reconnection
      sql::execute(session, statement);
      break;
    } catch (const mysqlshdk::db::Error &e) {
      log_debug("%sBulk load error: %s", log_id(), e.format().c_str());
      log_info("%sBulk load failed, waiting for retry", log_id());

      if (loader->m_bulk_load->wait_for_retry(e)) {
        log_info("%sRetrying bulk load", log_id());
      } else {
        log_info("%sNot retrying bulk load", log_id());
        throw;
      }
    }
  }

  stats.total_data_bytes += chunk().data_size;
  stats.total_file_bytes += chunk().file_size;
  stats.total_files_processed += chunk().chunks_total;

  if (const auto mysql_info = session->get_mysql_info()) {
    size_t records = 0;
    size_t deleted = 0;
    size_t skipped = 0;
    size_t warnings = 0;

    sscanf(mysql_info,
           "Records: %zu  Deleted: %zu  Skipped: %zu  Warnings: %zu\n",
           &records, &deleted, &skipped, &warnings);

    stats.total_records += records;
    stats.total_deleted += deleted;
    stats.total_skipped += skipped;
    stats.total_warnings += warnings;
  }
}

bool Dump_loader::Worker::Analyze_table_task::execute(Worker *worker,
                                                      Dump_loader *loader) {
  log_debug("%swill analyze table `%s`.`%s`", log_id(), schema().c_str(),
            table().c_str());

  auto console = current_console();

  if (m_histograms.empty() ||
      !histograms_supported(loader->m_options.target_server_version()))
    log_info("Analyzing table `%s`.`%s`", schema().c_str(), table().c_str());
  else
    log_info("Updating histogram for table `%s`.`%s`", schema().c_str(),
             table().c_str());

  loader->post_worker_event(worker, Worker_event::ANALYZE_START);

  // do work

  try {
    if (!loader->m_options.dry_run()) {
      const auto &reconnect = worker->reconnect_callback();
      const auto &session = worker->session();

      if (m_histograms.empty() ||
          !histograms_supported(loader->m_options.target_server_version())) {
        sql::ar::executef(reconnect, session, "ANALYZE TABLE !.!", schema(),
                          table());
      } else {
        for (const auto &h : m_histograms) {
          shcore::sqlstring q(
              "ANALYZE TABLE !.! UPDATE HISTOGRAM ON ! WITH ? BUCKETS", 0);
          q << schema() << table() << h.column << h.buckets;

          std::string sql = q.str();
          log_debug("Executing %s", sql.c_str());
          sql::ar::execute(reconnect, session, sql);
        }
      }
    }
  } catch (const std::exception &e) {
    handle_current_exception(
        worker, loader,
        shcore::str_format("While analyzing table `%s`.`%s`: %s",
                           schema().c_str(), table().c_str(), e.what()));
    return false;
  }

  log_debug("%sdone", log_id());
  ++loader->m_tables_analyzed;

  // signal for more work
  loader->post_worker_event(worker, Worker_event::ANALYZE_END);
  return true;
}

bool Dump_loader::Worker::Index_recreation_task::execute(Worker *worker,
                                                         Dump_loader *loader) {
  log_debug("%swill recreate %zu indexes for table %s", log_id(),
            m_indexes->size(), key().c_str());

  const auto console = current_console();

  if (!m_indexes->empty())
    log_info("%sRecreating indexes for %s", log_id(), key().c_str());

  loader->post_worker_event(worker, Worker_event::INDEX_START);

  // do work
  if (!loader->m_options.dry_run()) {
    loader->m_num_threads_recreating_indexes++;
    shcore::on_leave_scope cleanup(
        [loader]() { loader->m_num_threads_recreating_indexes--; });

    // for the analysis of various index types and their impact on the parallel
    // index creation see: BUG#33976718
    std::list<std::vector<std::string_view>> batches;

    {
      const auto append = [](const std::vector<std::string> &what,
                             std::vector<std::string_view> *where) {
        where->insert(where->end(), what.begin(), what.end());
      };
      auto &first_batch = batches.emplace_back();

      // BUG#34787778 - fulltext indexes need to be added one at a time
      if (!m_indexes->fulltext.empty()) {
        first_batch.emplace_back(m_indexes->fulltext.front());
      }

      if (!m_indexes->spatial.empty()) {
        // we load all indexes at once if:
        //  - server does not support parallel index creation
        auto single_batch =
            loader->m_options.target_server_version() < Version(8, 0, 27);
        //  - table has a virtual column
        single_batch |= m_indexes->has_virtual_columns;
        //  - table has a fulltext index
        single_batch |= !m_indexes->fulltext.empty();

        // if server supports parallel index creation and table does not have
        // virtual columns or fulltext indexes, we add spatial indexes in
        // another batch
        append(m_indexes->spatial,
               single_batch ? &first_batch : &batches.emplace_back());
      }

      append(m_indexes->regular, &first_batch);

      // BUG#34787778 - fulltext indexes need to be added one at a time
      if (!m_indexes->fulltext.empty()) {
        for (auto it = std::next(m_indexes->fulltext.begin()),
                  end = m_indexes->fulltext.end();
             it != end; ++it) {
          batches.emplace_back().emplace_back(*it);
        }
      }
    }

    try {
      const auto &session = worker->session();
      auto current = batches.begin();
      const auto end = batches.end();

      while (end != current) {
        auto query = "ALTER TABLE " + key() + " ";

        for (const auto &definition : *current) {
          query += "ADD ";
          query += definition;
          query += ',';
        }

        // remove last comma
        query.pop_back();

        auto retry = false;

        try {
          // statement is not idempotent - do not reconnect
          execute_statement(
              session, query, "While recreating indexes for table " + key(),
              current->size() <= 1 ? -1 : ER_TEMP_FILE_WRITE_FAILURE);
          loader->m_indexes_recreated += current->size();
        } catch (const shcore::Error &e) {
          if (e.code() == ER_TEMP_FILE_WRITE_FAILURE) {
            if (current->size() <= 1) {
              // we cannot split any more, report the error
              throw;
            } else {
              log_info(
                  "Failed to add indexes: the innodb_tmpdir is full, failed "
                  "query: %s",
                  query.c_str());

              // split this batch in two and retry
              const auto new_size = current->size() / 2;
              std::vector<std::string_view> new_batch;

              std::move(std::next(current->begin(), new_size), current->end(),
                        std::back_inserter(new_batch));
              current->resize(new_size);
              batches.insert(std::next(current), std::move(new_batch));
              retry = true;
              ++loader->m_num_index_retries;
            }
          } else {
            throw;
          }
        }

        if (!retry) {
          ++current;
        }
      }
    } catch (const std::exception &e) {
      handle_current_exception(
          worker, loader,
          shcore::str_format("While recreating indexes for table %s: %s",
                             key().c_str(), e.what()));
      return false;
    }
  }

  log_debug("%sdone", log_id());

  // signal for more work
  loader->post_worker_event(worker, Worker_event::INDEX_END);
  return true;
}

bool Dump_loader::Worker::Checksum_task::execute(Worker *worker,
                                                 Dump_loader *loader) {
  log_debug("%swill verify checksum for: %s, chunk: %zi", log_id(),
            key().c_str(), chunk_index());

  loader->post_worker_event(worker, Worker_event::CHECKSUM_START);

  if (!loader->m_options.dry_run()) {
    ++loader->m_num_threads_checksumming;

    shcore::on_leave_scope cleanup(
        [loader]() { --loader->m_num_threads_checksumming; });

    try {
      m_result = m_checksum->validate(worker->session(), query_comment());
    } catch (const std::exception &e) {
      handle_current_exception(
          worker, loader,
          shcore::str_format("While verifing checksum for: %s, chunk: %zi: %s",
                             key().c_str(), chunk_index(), e.what()));
      return false;
    }
  }

  log_debug("%sdone", log_id());

  // signal for more work
  loader->post_worker_event(worker, Worker_event::CHECKSUM_END);

  return true;
}

Dump_loader::Worker::Worker(size_t id, Dump_loader *owner)
    : m_id(id), m_owner(owner), m_connection_id(0) {}

void Dump_loader::Worker::run() {
  try {
    do_run();
  } catch (const mysqlshdk::db::Error &e) {
    handle_current_exception(e.format());
  } catch (const shcore::Error &e) {
    handle_current_exception(e.format());
  } catch (const std::exception &e) {
    handle_current_exception(e.what());
  } catch (...) {
    handle_current_exception("Unknown exception");
  }
}

void Dump_loader::Worker::do_run() {
  auto console = current_console();

  try {
    m_reconnect_callback = [this]() { connect(); };
    m_reconnect_callback();
  } catch (const shcore::Error &e) {
    handle_current_exception("Error opening connection to MySQL: " +
                             e.format());
    return;
  }

  m_owner->post_worker_event(this, Worker_event::CONNECTED);

  for (;;) {
    m_owner->post_worker_event(this, Worker_event::READY);

    // wait for signal that there's work to do... false means stop worker
    bool work = m_work_ready.pop();
    if (!work || m_owner->m_worker_interrupt) {
      m_owner->post_worker_event(this, Worker_event::EXIT);
      break;
    }

    assert(std::numeric_limits<size_t>::max() != m_task->id());

    if (!m_task->execute(this, m_owner)) break;
  }
}

void Dump_loader::Worker::stop() { m_work_ready.push(false); }

void Dump_loader::Worker::connect() {
  m_session = m_owner->create_session();
  m_connection_id = m_session->get_connection_id();
  m_thread_id = current_thread_id();
}

uint64_t Dump_loader::Worker::current_thread_id() const {
  const auto query = [this](std::string_view q) {
    // no reconnection - session setup step
    return sql::query(session(), q)->fetch_one_or_throw()->get_uint(0);
  };

  if (session()->get_server_version() >= Version(8, 0, 16)) {
    return query("SELECT PS_CURRENT_THREAD_ID()");
  }

  try {
    return query(
        "SELECT THREAD_ID FROM performance_schema.threads WHERE "
        "PROCESSLIST_ID=" +
        std::to_string(connection_id()));
  } catch (const mysqlshdk::db::Error &e) {
    if (ER_TABLEACCESS_DENIED_ERROR != e.code()) {
      throw;
    }
  }

  log_warning("Unable to fetch thread ID for connection ID: %" PRIu64,
              m_connection_id);
  return 0;
}

void Dump_loader::Worker::schedule(std::unique_ptr<Task> task) {
  task->set_id(m_id);
  m_task = std::move(task);
  m_work_ready.push(true);
}

void Dump_loader::Worker::handle_current_exception(Dump_loader *loader,
                                                   const std::string &error) {
  const auto id = this->id();

  if (!loader->m_thread_exceptions[id]) {
    current_console()->print_error(
        shcore::str_format("%s%s", worker_id(id).c_str(), error.c_str()));
    loader->m_thread_exceptions[id] = std::current_exception();
  }

  loader->m_num_errors += 1;
  loader->m_worker_interrupt = true;
  loader->post_worker_event(this, Worker_event::FATAL_ERROR);
}

void Dump_loader::Priority_queue::emplace(Task_ptr task) {
  const auto pos = task->weight() - 1;
  assert(pos < m_tasks.size());
  m_tasks[pos].emplace_back(std::move(task));
  ++m_size;
  m_top_pos = std::max(pos, m_top_pos);
}

[[nodiscard]] Dump_loader::Task_ptr
Dump_loader::Priority_queue::pop_top() noexcept {
  assert(!empty());

  auto task = std::move(m_tasks[m_top_pos].front());
  m_tasks[m_top_pos].pop_front();
  --m_size;

  if (empty()) {
    m_top_pos = 0;
  } else {
    while (0 != m_top_pos) {
      if (!m_tasks[m_top_pos].empty()) {
        break;
      }

      --m_top_pos;
    }
  }

  return task;
}

// ----

Dump_loader::Dump_loader(const Load_dump_options &options)
    : m_options(options),
      m_num_threads_loading(0),
      m_num_threads_recreating_indexes(0),
      m_character_set(options.character_set()),
      m_num_errors(0),
      m_progress_thread("Load dump", options.show_progress()) {
  m_pending_tasks.resize(m_options.threads_count());
}

Dump_loader::~Dump_loader() = default;

Session_ptr Dump_loader::create_session() {
  // no reconnection here - this is a session setup
  auto session = std::dynamic_pointer_cast<mysqlshdk::db::mysql::Session>(
      establish_session(m_options.connection_options(), false));

  // Make sure we don't get affected by user customizations of sql_mode
  sql::execute(session, "SET SQL_MODE = 'NO_AUTO_VALUE_ON_ZERO'");

  // Set timeouts to larger values since worker threads may get stuck
  // downloading data for some time before they have a chance to get back to
  // doing MySQL work.
  sql::executef(session, "SET SESSION net_read_timeout = ?",
                k_mysql_server_net_read_timeout);

  // This is the time until the server kicks out idle connections. Our
  // connections should last for as long as the dump lasts even if they're
  // idle.
  sql::executef(session, "SET SESSION wait_timeout = ?",
                k_mysql_server_wait_timeout);

  // Disable binlog if requested by user
  if (m_options.skip_binlog()) {
    try {
      sql::execute(session, "SET sql_log_bin=0");
    } catch (const mysqlshdk::db::Error &e) {
      THROW_ERROR(SHERR_LOAD_FAILED_TO_DISABLE_BINLOG, e.format().c_str());
    }
  }

  sql::execute(session, "SET sql_quote_show_create = 1");

  sql::execute(session, "SET foreign_key_checks = 0");
  sql::execute(session, "SET unique_checks = 0");

  if (!m_character_set.empty())
    sql::executef(session, "SET NAMES ?", m_character_set);

  if (m_dump->tz_utc()) sql::execute(session, "SET TIME_ZONE='+00:00'");

  if (m_options.load_ddl() && m_options.auto_create_pks_supported()) {
    // target server supports automatic creation of primary keys, we need to
    // explicitly set the value of session variable, so we won't end up creating
    // primary keys when user doesn't want to do that
    const auto create_pks = should_create_pks();

    // we toggle the session variable only if the global value is different from
    // the value requested by the user, as it requires at least
    // SESSION_VARIABLES_ADMIN privilege; in case of MDS (where users do no
    // have this privilege) we expect that user has set the appropriate
    // compatibility option during the dump and this variable is not going to be
    // toggled
    if (m_options.sql_generate_invisible_primary_key() != create_pks) {
      sql::executef(session,
                    "SET @@SESSION.sql_generate_invisible_primary_key=?",
                    create_pks);
    }
  }

  try {
    for (const auto &s : m_options.session_init_sql()) {
      log_info("Executing custom session init SQL: %s", s.c_str());
      sql::execute(session, s);
    }
  } catch (const shcore::Error &e) {
    throw shcore::Exception::runtime_error(
        "Error while executing sessionInitSql: " + e.format());
  }

  return session;
}

std::function<bool(const std::string &, const std::string &)>
Dump_loader::filter_user_script_for_mds() const {
  // In MDS, the list of grants that a service user can have is restricted,
  // even in "root" accounts.
  // User accounts have at most:
  // - a subset of global privileges
  // - full DB privileges on any DB
  // - a subset of DB privileges on mysql.* and sys.*

  // Trying to grant or revoke any of those will result in an error, because
  // the user doing the load will lack these privileges.

  // Global privileges are stripped during dump with the
  // strip_restricted_grants compat option, but revokes have to be stripped
  // at load time. This is because:
  // - we can't revoke what we don't have
  // - anything we don't have will be implicitly revoked anyway
  // Thus, stripping privs during dump will only work as intended if it's
  // loaded in MDS, where the implicit revokes will happen. If a stripped
  // dump is loaded by a root user in a non-MDS instance, accounts
  // can end up without the expected original revokes.

  mysqlshdk::mysql::Instance instance(m_session);

  auto allowed_global_grants =
      mysqlshdk::mysql::get_global_grants(instance, "administrator", "%");
  if (allowed_global_grants.empty())
    current_console()->print_warning("`administrator` role not found!");

  // get list of DB level privileges revoked from the administrator role
  auto restrictions =
      mysqlshdk::mysql::get_user_restrictions(instance, "administrator", "%");

#ifndef NDEBUG
  for (const auto &r : restrictions) {
    log_debug("Restrictions: schema=%s privileges=%s", r.first.c_str(),
              shcore::str_join(r.second, ", ").c_str());
  }
#endif

  // filter the users script
  return [restrictions = std::move(restrictions),
          allowed_global_grants = std::move(allowed_global_grants)](
             const std::string &priv_type, const std::string &priv_level) {
    // strip global privileges
    if (priv_level == "*.*") {
      // return true if the priv should be stripped
      return std::find_if(allowed_global_grants.begin(),
                          allowed_global_grants.end(),
                          [&priv_type](const std::string &priv) {
                            return shcore::str_caseeq(priv, priv_type);
                          }) == allowed_global_grants.end();
    }

    std::string schema, object;
    shcore::split_priv_level(priv_level, &schema, &object);

    if (object.empty() && schema == "*") return false;

    // strip DB privileges
    // only schema.* revokes are expected, this needs to be reviewed if
    // object specific revokes are ever added
    for (const auto &r : restrictions) {
      if (r.first == schema) {
        // return true if the priv should be stripped
        return std::find_if(r.second.begin(), r.second.end(),
                            [&priv_type](const std::string &priv) {
                              return shcore::str_caseeq(priv, priv_type);
                            }) != r.second.end();
      }
    }

    return false;
  };
}

void Dump_loader::on_dump_begin() {
  std::string pre_script = m_dump->begin_script();

  current_console()->print_status("Executing common preamble SQL");

  if (!m_options.dry_run())
    execute_script(m_reconnect_callback, m_session, pre_script,
                   "While executing preamble SQL", m_default_sql_transforms);
}

void Dump_loader::on_dump_end() {
  std::string post_script = m_dump->end_script();

  // Execute schema end scripts
  for (const std::string &schema : m_dump->schemas()) {
    on_schema_end(schema);
  }

  const auto console = current_console();
  console->print_status("Executing common postamble SQL");

  if (!m_options.dry_run())
    execute_script(m_reconnect_callback, m_session, post_script,
                   "While executing postamble SQL", m_default_sql_transforms);

  // Update GTID_PURGED only when requested by the user
  if (m_options.update_gtid_set() != Load_dump_options::Update_gtid_set::OFF) {
    auto status = m_load_log->status(progress::Gtid_update{});
    if (status == Load_progress_log::Status::DONE) {
      console->print_status("GTID_PURGED already updated");
      log_info("GTID_PURGED already updated");
    } else if (!m_dump->gtid_executed().empty()) {
      if (m_dump->gtid_executed_inconsistent()) {
        console->print_warning(
            "The gtid update requested, but gtid_executed was not guaranteed "
            "to be consistent during the dump");
      }

      try {
        m_load_log->log(progress::start::Gtid_update{});

        const auto query = m_options.is_mds() ? "CALL sys.set_gtid_purged(?)"
                                              : "SET GLOBAL GTID_PURGED=?";

        if (m_options.update_gtid_set() ==
            Load_dump_options::Update_gtid_set::REPLACE) {
          console->print_status("Resetting GTID_PURGED to dumped gtid set");
          log_info("Setting GTID_PURGED to %s",
                   m_dump->gtid_executed().c_str());

          if (!m_options.dry_run()) {
            // statement is not idempotent - do not reconnect
            sql::executef(m_session, query, m_dump->gtid_executed());
          }
        } else {
          console->print_status("Appending dumped gtid set to GTID_PURGED");
          log_info("Appending %s to GTID_PURGED",
                   m_dump->gtid_executed().c_str());

          if (!m_options.dry_run()) {
            // statement is not idempotent - do not reconnect
            sql::executef(m_session, query, "+" + m_dump->gtid_executed());
          }
        }
        m_load_log->log(progress::end::Gtid_update{});
      } catch (const std::exception &e) {
        console->print_error(std::string("Error while updating GTID_PURGED: ") +
                             e.what());
        throw;
      }
    } else {
      console->print_warning(
          "gtid update requested but, gtid_executed not set in dump");
    }
  }

  // check if redo log is disabled and print a reminder if so
  auto res = sql::ar::query(m_reconnect_callback, m_session,
                            "SELECT VARIABLE_VALUE = 'OFF' FROM "
                            "performance_schema.global_status "
                            "WHERE variable_name = 'Innodb_redo_log_enabled'");
  if (auto row = res->fetch_one()) {
    if (row->get_int(0, 0)) {
      console->print_note(
          "The redo log is currently disabled, which causes MySQL to not be "
          "crash safe! Do not forget to enable it again before putting this "
          "instance in production.");
    }
  }
}

void Dump_loader::on_schema_end(const std::string &schema) {
  if (m_options.load_deferred_indexes()) {
    const auto &fks = m_dump->deferred_schema_fks(schema);
    if (!fks.empty()) {
      log_info("Recreating FOREIGN KEY constraints for schema %s",
               shcore::quote_identifier(schema).c_str());
      if (!m_options.dry_run()) {
        for (const auto &q : fks) {
          try {
            // statement is not idempotent - do not reconnect
            sql::execute(m_session, q);
          } catch (const std::exception &e) {
            current_console()->print_error(
                "Error while restoring FOREIGN KEY constraint in schema `" +
                schema + "` with query: " + q);
            throw;
          }
        }
      }
    }
  }

  std::list<Dump_reader::Name_and_file> triggers;

  m_dump->schema_table_triggers(schema, &triggers);

  for (const auto &it : triggers) {
    const auto &table = it.first;
    const auto status =
        m_load_log->status(progress::Triggers_ddl{schema, table});

    log_debug("Triggers DDL for `%s`.`%s` (%s)", schema.c_str(), table.c_str(),
              to_string(status).c_str());

    if (m_options.load_ddl()) {
      m_load_log->log(progress::start::Triggers_ddl{schema, table});

      if (status != Load_progress_log::DONE) {
        log_info("Executing triggers SQL for `%s`.`%s`", schema.c_str(),
                 table.c_str());

        it.second->open(mysqlshdk::storage::Mode::READ);
        const auto script = mysqlshdk::storage::read_file(it.second.get());
        it.second->close();

        if (!m_options.dry_run()) {
          auto transforms = m_default_sql_transforms;

          transforms.add_execute_conditionally(
              [this, &schema, &table](std::string_view type,
                                      const std::string &name) {
                if (!shcore::str_caseeq(type, "TRIGGER")) return true;
                return m_dump->include_trigger(schema, table, name);
              });

          execute_script(m_reconnect_callback, m_session,
                         use_schema(schema, script),
                         "While executing triggers SQL", transforms);
        }
      }

      m_load_log->log(progress::end::Triggers_ddl{schema, table});
    }
  }

  {
    const auto &queries = m_dump->queries_on_schema_end(schema);

    if (!queries.empty()) {
      log_info("Executing finalization queries for schema %s",
               shcore::quote_identifier(schema).c_str());

      if (!m_options.dry_run()) {
        for (const auto &q : queries) {
          try {
            // statement is not idempotent - do not reconnect
            sql::execute(m_session, q);
          } catch (const std::exception &e) {
            current_console()->print_error(
                "Error while executing finalization queries for schema `" +
                schema + "` with query: " + q);
            throw;
          }
        }
      }
    }
  }
}

bool Dump_loader::should_fetch_table_ddl(bool placeholder) const {
  return m_options.load_ddl() ||
         (!placeholder && m_options.load_deferred_indexes());
}

bool Dump_loader::handle_table_data() {
  bool scheduled = false;
  bool scanned = false;
  std::unordered_multimap<std::string, size_t> tables_being_loaded;
  Dump_reader::Table_chunk chunk;

  // Note: job scheduling should preferably load different tables per thread,
  //       each partition is treated as a different table

  do {
    {
      std::lock_guard<std::mutex> lock(m_tables_being_loaded_mutex);
      tables_being_loaded = m_tables_being_loaded;
    }
    if (m_dump->next_table_chunk(tables_being_loaded, &chunk)) {
      log_debug3("Scheduling chunk: %s", format_table(chunk).c_str());

      if (bulk_load_supported(chunk)) {
        scheduled = schedule_bulk_load(std::move(chunk));
      } else {
        scheduled = schedule_table_chunk(std::move(chunk));
      }
    } else {
      scheduled = false;

      if (m_dump->status() != Dump_reader::Status::COMPLETE && !scanned) {
        scan_for_more_data(false);
        scanned = true;
      } else {
        break;
      }
    }
  } while (!scheduled);

  return scheduled;
}

bool Dump_loader::schedule_table_chunk(Dump_reader::Table_chunk chunk) {
  if (!chunk.chunked) {
    chunk.index = -1;
  }

  const auto status = m_load_log->status(progress::Table_chunk{
      chunk.schema, chunk.table, chunk.partition, chunk.index});

  if (status == Load_progress_log::DONE) {
    m_dump->on_chunk_loaded(chunk);
  }

  log_debug("Table data for %s (%s)", format_table(chunk).c_str(),
            to_string(status).c_str());

  uint64_t bytes_to_skip = 0;
  const auto resuming = status == Load_progress_log::INTERRUPTED;

  // if task was interrupted, check if any of the subchunks were loaded, if
  // yes then we need to skip them
  if (resuming) {
    uint64_t subchunk = 0;

    while (m_load_log->status(progress::Table_subchunk{
               chunk.schema, chunk.table, chunk.partition, chunk.index,
               subchunk}) == Load_progress_log::DONE) {
      bytes_to_skip += m_load_log->table_subchunk_size(progress::Table_subchunk{
          chunk.schema, chunk.table, chunk.partition, chunk.index, subchunk});
      ++subchunk;
    }

    if (subchunk > 0) {
      log_debug(
          "Loading table data for %s was interrupted after "
          "%" PRIu64 " subchunks were loaded, skipping %" PRIu64 " bytes",
          format_table(chunk).c_str(), subchunk, bytes_to_skip);
    }
  }

  if (!m_options.load_data() || status == Load_progress_log::DONE ||
      !is_table_included(chunk.schema, chunk.table)) {
    return false;
  }

  log_debug("Scheduling chunk for table %s (%s)", format_table(chunk).c_str(),
            chunk.file->full_path().masked().c_str());

  mark_table_as_in_progress(chunk);
  push_pending_task(load_chunk_file(std::move(chunk), resuming, bytes_to_skip));

  return true;
}

size_t Dump_loader::handle_worker_events(
    const std::function<bool()> &schedule_next) {
  const auto to_string = [](Worker_event::Event event) {
    switch (event) {
      case Worker_event::Event::CONNECTED:
        return "CONNECTED";
      case Worker_event::Event::READY:
        return "READY";
      case Worker_event::Event::FATAL_ERROR:
        return "FATAL_ERROR";
      case Worker_event::Event::SCHEMA_DDL_START:
        return "SCHEMA_DDL_START";
      case Worker_event::Event::SCHEMA_DDL_END:
        return "SCHEMA_DDL_END";
      case Worker_event::Event::TABLE_DDL_START:
        return "TABLE_DDL_START";
      case Worker_event::Event::TABLE_DDL_END:
        return "TABLE_DDL_END";
      case Worker_event::Event::LOAD_START:
        return "LOAD_START";
      case Worker_event::Event::LOAD_END:
        return "LOAD_END";
      case Worker_event::Event::INDEX_START:
        return "INDEX_START";
      case Worker_event::Event::INDEX_END:
        return "INDEX_END";
      case Worker_event::Event::ANALYZE_START:
        return "ANALYZE_START";
      case Worker_event::Event::ANALYZE_END:
        return "ANALYZE_END";
      case Worker_event::Event::EXIT:
        return "EXIT";
      case Worker_event::Event::LOAD_SUBCHUNK_START:
        return "LOAD_SUBCHUNK_START";
      case Worker_event::Event::LOAD_SUBCHUNK_END:
        return "LOAD_SUBCHUNK_END";
      case Worker_event::Event::CHECKSUM_START:
        return "CHECKSUM_START";
      case Worker_event::Event::CHECKSUM_END:
        return "CHECKSUM_END";
      case Worker_event::Event::BULK_LOAD_START:
        return "BULK_LOAD_START";
      case Worker_event::Event::BULK_LOAD_PROGRESS:
        return "BULK_LOAD_PROGRESS";
      case Worker_event::Event::BULK_LOAD_END:
        return "BULK_LOAD_END";
    }
    return "";
  };

  std::list<Worker *> idle_workers;
  const auto thread_count = m_options.threads_count();

  while (idle_workers.size() < m_workers.size()) {
    Worker_event event;

    // Wait for events from workers, but update progress and check for ^C
    // every now and then
    for (;;) {
      auto event_opt = m_worker_events.try_pop(std::chrono::seconds{1});
      if (event_opt && event_opt->worker) {
        event = std::move(*event_opt);
        break;
      }
    }

    log_debug2("Got event %s from worker %zi", to_string(event.event),
               event.worker->id());

    switch (event.event) {
      case Worker_event::LOAD_START:
        on_chunk_load_start(event.worker->id(),
                            static_cast<const Worker::Load_chunk_task *>(
                                event.worker->current_task()));
        break;

      case Worker_event::LOAD_END:
        on_chunk_load_end(event.worker->id(),
                          static_cast<const Worker::Load_chunk_task *>(
                              event.worker->current_task()));
        break;

      case Worker_event::LOAD_SUBCHUNK_START:
        on_subchunk_load_start(event.worker->id(), event.details,
                               static_cast<const Worker::Load_chunk_task *>(
                                   event.worker->current_task()));
        break;

      case Worker_event::LOAD_SUBCHUNK_END:
        on_subchunk_load_end(event.worker->id(), event.details,
                             static_cast<const Worker::Load_chunk_task *>(
                                 event.worker->current_task()));
        break;

      case Worker_event::BULK_LOAD_START: {
        auto task =
            static_cast<Worker::Bulk_load_task *>(event.worker->current_task());

        on_bulk_load_start(event.worker->id(), task);

        assert(m_bulk_load);
        m_bulk_load->on_bulk_load_start(event.worker, task);
        break;
      }

      case Worker_event::BULK_LOAD_PROGRESS:
        on_bulk_load_progress(event.details);
        break;

      case Worker_event::BULK_LOAD_END: {
        auto task =
            static_cast<Worker::Bulk_load_task *>(event.worker->current_task());

        assert(m_bulk_load);
        m_bulk_load->on_bulk_load_end(event.worker);

        on_bulk_load_end(event.worker->id(), task);
        break;
      }

      case Worker_event::SCHEMA_DDL_START:
        on_schema_ddl_start(event.worker->id(),
                            static_cast<const Worker::Schema_ddl_task *>(
                                event.worker->current_task()));
        break;

      case Worker_event::SCHEMA_DDL_END:
        on_schema_ddl_end(event.worker->id(),
                          static_cast<const Worker::Schema_ddl_task *>(
                              event.worker->current_task()));
        break;

      case Worker_event::TABLE_DDL_START:
        on_table_ddl_start(event.worker->id(),
                           static_cast<const Worker::Table_ddl_task *>(
                               event.worker->current_task()));
        break;

      case Worker_event::TABLE_DDL_END:
        on_table_ddl_end(event.worker->id(),
                         static_cast<Worker::Table_ddl_task *>(
                             event.worker->current_task()));
        break;

      case Worker_event::INDEX_START:
        on_index_start(event.worker->id(),
                       static_cast<const Worker::Index_recreation_task *>(
                           event.worker->current_task()));
        break;

      case Worker_event::INDEX_END:
        on_index_end(event.worker->id(),
                     static_cast<const Worker::Index_recreation_task *>(
                         event.worker->current_task()));
        break;

      case Worker_event::ANALYZE_START:
        on_analyze_start(event.worker->id(),
                         static_cast<const Worker::Analyze_table_task *>(
                             event.worker->current_task()));
        break;

      case Worker_event::ANALYZE_END:
        on_analyze_end(event.worker->id(),
                       static_cast<const Worker::Analyze_table_task *>(
                           event.worker->current_task()));
        break;

      case Worker_event::CHECKSUM_START: {
        const auto task =
            static_cast<Worker::Checksum_task *>(event.worker->current_task());
        on_checksum_start(task->checksum_info());
        break;
      }

      case Worker_event::CHECKSUM_END: {
        const auto task =
            static_cast<Worker::Checksum_task *>(event.worker->current_task());
        on_checksum_end(task->checksum_info(), task->result());
        break;
      }

      case Worker_event::READY:
        if (const auto task = event.worker->current_task()) {
          m_current_weight -= task->weight();
          task->done();
        }
        break;

      case Worker_event::FATAL_ERROR:
        if (!m_abort) {
          current_console()->print_error("Aborting load...");
        }
        m_worker_interrupt = true;
        m_abort = true;

        clear_worker(event.worker);
        break;

      case Worker_event::EXIT:
        clear_worker(event.worker);
        break;

      case Worker_event::CONNECTED:
        assert(m_monitoring);
        m_monitoring->add_worker(event.worker);
        break;
    }

    // schedule more work if the worker became free
    if (event.event == Worker_event::READY) {
      // no more work to do
      if (m_worker_interrupt || (!schedule_next() && m_pending_tasks.empty())) {
        idle_workers.push_back(event.worker);
      } else {
        assert(!m_pending_tasks.empty());

        const auto pending_weight = m_pending_tasks.top()->weight();

        if (m_current_weight + pending_weight > thread_count) {
          // the task is too heavy, wait till more threads are idle
          idle_workers.push_back(event.worker);
        } else {
          event.worker->schedule(m_pending_tasks.pop_top());
          m_current_weight += pending_weight;

          // free any idle threads which were waiting for a heavy task
          const auto available = thread_count - m_current_weight;

          for (uint64_t i = 0; i < available; ++i) {
            if (idle_workers.empty()) {
              break;
            }

            m_worker_events.push(
                {Worker_event::READY, idle_workers.front(), {}});
            idle_workers.pop_front();
          }
        }
      }
    }
  }

  size_t num_idle_workers = idle_workers.size();
  // put all idle workers back into the queue, so that they can get assigned
  // new tasks if more becomes available later
  for (auto *worker : idle_workers) {
    m_worker_events.push({Worker_event::READY, worker, {}});
  }
  return num_idle_workers;
}

bool Dump_loader::schedule_next_task() {
  if (!handle_table_data()) {
    std::string schema;
    std::string table;

    if (m_options.load_deferred_indexes()) {
      setup_create_indexes_progress();

      if (is_data_load_complete()) {
        m_index_count_is_known = true;
      }

      compatibility::Deferred_statements::Index_info *indexes = nullptr;

      if (m_dump->next_deferred_index(&schema, &table, &indexes)) {
        assert(indexes != nullptr);
        push_pending_task(recreate_indexes(schema, table, indexes));
        return true;
      }
    }

    const auto analyze_tables = m_options.analyze_tables();

    if (Load_dump_options::Analyze_table_mode::OFF != analyze_tables) {
      setup_analyze_tables_progress();

      std::vector<Dump_reader::Histogram> histograms;

      if (m_dump->next_table_analyze(&schema, &table, &histograms)) {
        // if Analyze_table_mode is HISTOGRAM, only analyze tables with
        // histogram info in the dump
        if (Load_dump_options::Analyze_table_mode::ON == analyze_tables ||
            !histograms.empty()) {
          ++m_tables_to_analyze;
          push_pending_task(analyze_table(schema, table, histograms));
          return true;
        }
      } else if (is_data_load_complete()) {
        m_all_analyze_tasks_scheduled = true;
      }
    }

    if (m_options.checksum()) {
      setup_checksum_tables_progress();

      const dump::common::Checksums::Checksum_data *checksum;

      do {
        if (m_dump->next_table_checksum(&checksum)) {
          if (maybe_push_checksum_task(checksum)) {
            ++m_checksum_tasks_to_complete;
            return true;
          } else {
            // task was not scheduled, mark it as complete
            m_dump->on_checksum_end(checksum->schema(), checksum->table(),
                                    checksum->partition());
          }
        } else {
          if (m_dump->all_data_verification_scheduled()) {
            m_all_checksum_tasks_scheduled = true;
          }

          break;
        }
      } while (true);
    }

    return false;
  } else {
    return true;
  }
}

void Dump_loader::interrupt() {
  // 1st ^C does a soft interrupt (stop new tasks but let current work finish)
  // 2nd ^C sends kill to all workers
  if (!m_worker_interrupt) {
    m_worker_interrupt = true;
    current_console()->print_info(
        "^C -- Load interrupted. Canceling remaining work. "
        "Press ^C again to abort current tasks and rollback active "
        "transactions (slow).");
  } else {
    hard_interrupt();
  }
}

void Dump_loader::hard_interrupt() {
  abort();
  current_console()->print_info(
      "^C -- Aborting active transactions. This may take a while...");
}

void Dump_loader::abort() {
  m_worker_interrupt = true;
  m_worker_hard_interrupt = true;
}

void Dump_loader::run() {
  try {
    m_progress_thread.start();
    shcore::on_leave_scope cleanup_progress([this]() {
      if (m_worker_interrupt) {
        m_progress_thread.terminate();
      } else {
        m_progress_thread.finish();
      }
    });

    open_dump();

    spawn_workers();

    if (m_options.bulk_load_info().enabled) {
      m_bulk_load = std::make_unique<Bulk_load_support>(this);
    }

    {
      shcore::on_leave_scope cleanup_workers([this]() { join_workers(); });
      execute_tasks();
    }
  } catch (...) {
    translate_current_exception(m_progress_thread);
  }

  show_summary();

  if (m_worker_interrupt && !m_abort) {
    // If interrupted by the user and not by a fatal error
    throw shcore::cancelled("Aborted");
  }

  for (const auto &e : m_thread_exceptions) {
    if (e) {
      THROW_ERROR(SHERR_LOAD_WORKER_THREAD_FATAL_ERROR);
    }
  }
}

void Dump_loader::show_summary() {
  using mysqlshdk::utils::format_bytes;
  using mysqlshdk::utils::format_items;
  using mysqlshdk::utils::format_seconds;
  using mysqlshdk::utils::format_throughput_bytes;

  const auto console = current_console();

  if (m_stats.total_records == m_rows_previously_loaded) {
    if (m_resuming)
      console->print_info("There was no remaining data left to be loaded.");
    else
      console->print_info("No data loaded.");
  } else {
    assert(m_load_data_stage);
    const auto load_seconds = m_load_data_stage->duration().seconds();

    console->print_info(shcore::str_format(
        "%zi chunks (%s, %s) for %" PRIu64
        " tables in %zi schemas were "
        "loaded in %s (avg throughput %s, %s)",
        m_stats.total_files_processed.load(),
        format_items("rows", "rows", m_stats.total_records).c_str(),
        format_bytes(m_stats.total_data_bytes).c_str(),
        m_dump->tables_to_load(), m_dump->schemas().size(),
        format_seconds(load_seconds, false).c_str(),
        format_throughput_bytes(
            m_stats.total_data_bytes - m_data_bytes_previously_loaded,
            load_seconds)
            .c_str(),
        format_rows_throughput(m_stats.total_records - m_rows_previously_loaded,
                               load_seconds)
            .c_str()));
  }

  if (m_total_ddl_executed) {
    console->print_info(shcore::str_format(
        "%" PRIu64 " DDL files were executed in %s.", m_total_ddl_executed,
        format_seconds(m_total_ddl_execution_seconds, false).c_str()));
  }

  if (m_options.load_users()) {
    std::string msg =
        std::to_string(m_loaded_accounts) + " accounts were loaded";

    if (m_ignored_grant_errors) {
      msg += ", " + std::to_string(m_ignored_grant_errors) +
             " GRANT statement errors were ignored";
    }

    if (m_dropped_accounts) {
      msg += ", " + std::to_string(m_dropped_accounts) +
             " accounts were dropped due to GRANT statement errors";
    }

    console->print_info(msg);
  }

  if (m_bulk_load && m_bulk_load->compatible_tables()) {
    console->print_info(shcore::str_format(
        "%zi table%s loaded using BULK LOAD.", m_bulk_load->compatible_tables(),
        1 == m_bulk_load->compatible_tables() ? " was" : "s were"));
  }

  if (m_stats.total_records != m_rows_previously_loaded) {
    assert(m_load_data_stage);
    const auto load_seconds = m_load_data_stage->duration().seconds();

    console->print_info(shcore::str_format(
        "Data load duration: %s", format_seconds(load_seconds, false).c_str()));
  }

  if (m_indexes_recreated) {
    assert(m_create_indexes_stage);

    console->print_info(shcore::str_format(
        "%" PRIu64 " indexes were recreated in %s.", m_indexes_recreated.load(),
        format_seconds(m_create_indexes_stage->duration().seconds(), false)
            .c_str()));

    if (m_num_index_retries) {
      console->print_info(
          shcore::str_format("There were %zi retries to create indexes.",
                             m_num_index_retries.load()));
    }
  }

  if (m_tables_analyzed) {
    assert(m_analyze_tables_stage);

    console->print_info(shcore::str_format(
        "%" PRIu64 " tables were analyzed in %s.", m_tables_analyzed.load(),
        format_seconds(m_analyze_tables_stage->duration().seconds(), false)
            .c_str()));
  }

  console->print_info(shcore::str_format(
      "Total duration: %s",
      format_seconds(m_progress_thread.duration().seconds(), false).c_str()));

  if (m_stats.total_deleted > 0) {
    // BUG#35304391 - notify about replaced rows
    console->print_info(std::to_string(m_stats.total_deleted) +
                        " rows were replaced");
  }

  if (m_num_errors > 0) {
    console->print_info(shcore::str_format(
        "%zi errors and %zi warnings were reported during the load.",
        m_num_errors.load(), m_stats.total_warnings.load()));
  } else {
    console->print_info(
        shcore::str_format("%zi warnings were reported during the load.",
                           m_stats.total_warnings.load()));
  }

  if (!m_options.dry_run()) {
    if (m_checksum_tasks_completed) {
      assert(m_checksum_tables_stage);
      console->print_info(shcore::str_format(
          "%" PRIu64 " checksums were verified in %s.",
          m_checksum_tasks_completed,
          format_seconds(m_checksum_tables_stage->duration().seconds(), false)
              .c_str()));
    }

    if (m_checksum_errors) {
      console->print_error(shcore::str_format(
          "%zu checksum verification errors were reported during the load.",
          m_checksum_errors));

      if (m_stats.total_warnings) {
        console->print_note(
            "Warnings reported during the load may provide some information on "
            "source of these errors.");
      }

      THROW_ERROR(SHERR_LOAD_CHECKSUM_VERIFICATION_FAILED);
    }
  }
}

void Dump_loader::open_dump() { open_dump(m_options.create_dump_handle()); }

void Dump_loader::open_dump(
    std::unique_ptr<mysqlshdk::storage::IDirectory> dumpdir) {
  auto console = current_console();
  m_dump = std::make_unique<Dump_reader>(std::move(dumpdir), m_options);

  console->print_status("Opening dump...");
  auto status = m_dump->open();

  if (m_dump->dump_version().get_major() > k_supported_dump_version_major ||
      (m_dump->dump_version().get_major() == k_supported_dump_version_major &&
       m_dump->dump_version().get_minor() > k_supported_dump_version_minor)) {
    console->print_error(
        "Dump format has version " + m_dump->dump_version().get_full() +
        " which is not supported by this version of MySQL Shell. "
        "Please upgrade MySQL Shell to load it.");
    THROW_ERROR(SHERR_LOAD_UNSUPPORTED_DUMP_VERSION);
  }

  if (m_dump->dump_version() < Version(dump::Schema_dumper::version())) {
    console->print_note(
        "Dump format has version " + m_dump->dump_version().get_full() +
        " and was created by an older version of MySQL Shell. "
        "If you experience problems loading it, please recreate the dump using "
        "the current version of MySQL Shell and try again.");
  }

  std::string missing_capabilities;
  // 8.0.27 is the version where capabilities were added
  Version minimum_version{8, 0, 27};

  for (const auto &capability : m_dump->capabilities()) {
    if (!dump::capability::is_supported(capability.id)) {
      if (minimum_version < capability.version_required) {
        minimum_version = capability.version_required;
      }

      missing_capabilities += "* ";
      missing_capabilities += capability.description;
      missing_capabilities += "\n\n";
    }
  }

  if (!missing_capabilities.empty()) {
    console->print_error(
        "Dump is using capabilities which are not supported by this version of "
        "MySQL Shell:\n\n" +
        missing_capabilities +
        "The minimum required version of MySQL Shell to load this dump is: " +
        minimum_version.get_base() + ".");
    THROW_ERROR(SHERR_LOAD_UNSUPPORTED_DUMP_CAPABILITIES);
  }

  if (status != Dump_reader::Status::COMPLETE) {
    if (m_options.dump_wait_timeout_ms() > 0) {
      console->print_note(
          "Dump is still ongoing, data will be loaded as it becomes "
          "available.");
    } else {
      console->print_error(
          "Dump is not yet finished. Use the 'waitDumpTimeout' option to "
          "enable concurrent load and set a timeout for when we need to wait "
          "for new data to become available.");
      THROW_ERROR(SHERR_LOAD_INCOMPLETE_DUMP);
    }
  }

  m_dump->validate_options();

  show_metadata();

  // Pick charset
  if (m_character_set.empty()) {
    m_character_set = m_dump->default_character_set();
  }
}

void Dump_loader::check_server_version() {
  const auto console = current_console();
  const auto &source_server = m_dump->server_version();
  const auto &target_server = m_options.target_server_version();
  const auto mds = m_options.is_mds();

  // no reconnection here - we're using the global session
  mysqlshdk::mysql::Instance session(m_options.base_session());

  std::string msg = "Target is MySQL " + target_server.get_full();
  if (mds) msg += " (MySQL HeatWave Service)";
  msg += ". Dump was produced from MySQL " + source_server.get_full();

  console->print_info(msg);

  if (target_server < Version(5, 7, 0)) {
    THROW_ERROR(SHERR_LOAD_UNSUPPORTED_SERVER_VERSION);
  }

  if (m_options.ignore_version() ||
      (Version(5, 7, 0) <= source_server && source_server < Version(8, 0, 0) &&
       Version(8, 0, 0) <= target_server && target_server < Version(9, 0, 0))) {
    // we implicitly enable this transformation when loading 5.7 -> 8.X
    m_default_sql_transforms.add_strip_removed_sql_modes();
  }

  if (mds) {
    if (!m_dump->mds_compatibility()) {
      msg =
          "Destination is a MySQL HeatWave Service DB System instance but the "
          "dump was produced without the compatibility option. ";

      if (m_options.ignore_version()) {
        msg +=
            "The 'ignoreVersion' option is enabled, so loading anyway. If this "
            "operation fails, create the dump once again with the 'ocimds' "
            "option enabled.";

        console->print_warning(msg);
      } else {
        msg +=
            "Please enable the 'ocimds' option when dumping your database. "
            "Alternatively, enable the 'ignoreVersion' option to load anyway.";

        console->print_error(msg);

        THROW_ERROR(SHERR_LOAD_DUMP_NOT_MDS_COMPATIBLE);
      }
    }

    if (const auto &target_version = m_dump->target_version();
        target_version.has_value() && *target_version != target_server) {
      console->print_warning(
          "Destination MySQL version is different than the value of the "
          "'targetVersion' option set when the dump was created: " +
          target_version->get_base());
    }
  }

  if (target_server.get_major() != source_server.get_major()) {
    // BUG#35359364 - we allow for migration between consecutive major versions
    const auto diff = major_version_difference(source_server, target_server);

    if (diff < 0) {
      msg =
          "Destination MySQL version is older than the one where the dump was "
          "created.";
    } else {
      msg =
          "Destination MySQL version is newer than the one where the dump was "
          "created.";
    }

    if (1 != abs(diff)) {
      if (m_options.ignore_version()) {
        msg +=
            " Source and destination have non-consecutive major MySQL "
            "versions. The 'ignoreVersion' option is enabled, so loading "
            "anyway.";
        console->print_warning(msg);
      } else {
        msg +=
            " Loading dumps from non-consecutive major MySQL versions is not "
            "fully supported and may not work. Enable the 'ignoreVersion' "
            "option to load anyway.";
        console->print_error(msg);
        THROW_ERROR(SHERR_LOAD_SERVER_VERSION_MISMATCH);
      }
    } else {
      console->print_note(msg);
    }
  }

  if (m_options.analyze_tables() ==
          Load_dump_options::Analyze_table_mode::HISTOGRAM &&
      !histograms_supported(target_server))
    console->print_warning("Histogram creation enabled but MySQL Server " +
                           target_server.get_base() + " does not support it.");
  if (m_options.update_gtid_set() != Load_dump_options::Update_gtid_set::OFF) {
    // Check if group replication is running
    bool group_replication_running = false;
    try {
      group_replication_running = session.queryf_one_int(
          0, 0,
          "select count(*) from performance_schema.replication_group_members "
          "where MEMBER_ID = @@server_uuid AND MEMBER_STATE IS NOT NULL AND "
          "MEMBER_STATE <> 'OFFLINE';");
    } catch (...) {
    }

    if (group_replication_running) {
      THROW_ERROR(SHERR_LOAD_UPDATE_GTID_GR_IS_RUNNING);
    }

    if (target_server < Version(8, 0, 0)) {
      if (m_options.update_gtid_set() ==
          Load_dump_options::Update_gtid_set::APPEND) {
        THROW_ERROR(SHERR_LOAD_UPDATE_GTID_APPEND_NOT_SUPPORTED);
      }

      if (!m_options.skip_binlog()) {
        THROW_ERROR(SHERR_LOAD_UPDATE_GTID_REQUIRES_SKIP_BINLOG);
      }

      if (!session.queryf_one_int(0, 0,
                                  "select @@global.gtid_executed = '' and "
                                  "@@global.gtid_purged = ''")) {
        THROW_ERROR(SHERR_LOAD_UPDATE_GTID_REPLACE_REQUIRES_EMPTY_VARIABLES);
      }
    } else {
      const char *g = m_dump->gtid_executed().c_str();
      if (m_options.update_gtid_set() ==
          Load_dump_options::Update_gtid_set::REPLACE) {
        if (!session.queryf_one_int(
                0, 0,
                "select GTID_SUBTRACT(?, "
                "GTID_SUBTRACT(@@global.gtid_executed, "
                "@@global.gtid_purged)) = gtid_subtract(?, '')",
                g, g)) {
          THROW_ERROR(SHERR_LOAD_UPDATE_GTID_REPLACE_SETS_INTERSECT);
        }

        if (!session.queryf_one_int(
                0, 0, "select GTID_SUBSET(@@global.gtid_purged, ?);", g)) {
          THROW_ERROR(SHERR_LOAD_UPDATE_GTID_REPLACE_REQUIRES_SUPERSET);
        }
      } else if (!session.queryf_one_int(
                     0, 0,
                     "select GTID_SUBTRACT(@@global.gtid_executed, ?) = "
                     "@@global.gtid_executed",
                     g)) {
        THROW_ERROR(SHERR_LOAD_UPDATE_GTID_APPEND_SETS_INTERSECT);
      }
    }
  }

  if (should_create_pks() && target_server < Version(8, 0, 24)) {
    THROW_ERROR(SHERR_LOAD_INVISIBLE_PKS_UNSUPPORTED_SERVER_VERSION);
  }

  if (m_options.load_users() &&
      m_dump->partial_revokes() != m_options.partial_revokes()) {
    const auto status = [](bool b) { return b ? "enabled" : "disabled"; };

    console->print_warning(shcore::str_format(
        "The dump was created on an instance where the 'partial_revokes' "
        "system variable was %s, however the target instance has it %s. GRANT "
        "statements on object names with wildcard characters (%% or _) will "
        "behave differently.",
        status(m_dump->partial_revokes()),
        status(m_options.partial_revokes())));
  }
}

void Dump_loader::check_tables_without_primary_key() {
  if (!m_options.load_ddl()) {
    return;
  }

  if (m_options.is_mds() && m_dump->has_tables_without_pk()) {
    bool warning = true;
    std::string msg =
        "The dump contains tables without Primary Keys and it is loaded with "
        "the 'createInvisiblePKs' option set to ";

    if (should_create_pks()) {
      msg +=
          "true, Inbound Replication into an MySQL HeatWave Service DB System "
          "instance with High Availability can";

      if (m_options.target_server_version() < Version(8, 0, 32)) {
        msg += "not";
      } else {
        warning = false;
      }

      msg += " be used with this dump.";
    } else {
      msg +=
          "false, this dump cannot be loaded into an MySQL HeatWave Service "
          "DB System instance with High Availability.";
    }

    if (warning) {
      current_console()->print_warning(msg);
    } else {
      current_console()->print_note(msg);
    }
  }

  if (m_options.target_server_version() < Version(8, 0, 13) ||
      should_create_pks()) {
    return;
  }

  if (sql::ar::query(m_reconnect_callback, m_session,
                     "show variables like 'sql_require_primary_key';")
          ->fetch_one()
          ->get_string(1) != "ON")
    return;

  std::string tbs;
  for (const auto &s : m_dump->tables_without_pk())
    tbs += "schema " + shcore::quote_identifier(s.first) + ": " +
           shcore::str_join(s.second, ", ") + "\n";

  if (!tbs.empty()) {
    const auto error_msg = shcore::str_format(
        "The sql_require_primary_key option is enabled at the destination "
        "server and one or more tables without a Primary Key were found in "
        "the dump:\n%s\n"
        "You must do one of the following to be able to load this dump:\n"
        "- Add a Primary Key to the tables where it's missing\n"
        "- Use the \"createInvisiblePKs\" option to automatically create "
        "Primary Keys on a 8.0.24+ server\n"
        "- Use the \"excludeTables\" option to load the dump without those "
        "tables\n"
        "- Disable the sql_require_primary_key sysvar at the server (note "
        "that the underlying reason for the option to be enabled may still "
        "prevent your database from functioning properly)",
        tbs.c_str());
    current_console()->print_error(error_msg);
    THROW_ERROR(SHERR_LOAD_REQUIRE_PRIMARY_KEY_ENABLED);
  }
}

namespace {

std::vector<std::string> fetch_names(mysqlshdk::db::IResult *result) {
  std::vector<std::string> names;

  while (auto row = result->fetch_one()) {
    names.push_back(row->get_string(0));
  }
  return names;
}

std::shared_ptr<mysqlshdk::db::IResult> query_names(
    const Reconnect &reconnect, const Session_ptr &session,
    const std::string &schema,
    const std::list<Dump_reader::Object_info *> &names,
    const std::string &query_prefix) {
  if (!names.empty()) {
    const auto set = shcore::str_join(names, ",", [](const auto &s) {
      return shcore::quote_sql_string(s->name);
    });

    return sql::ar::queryf(reconnect, session, query_prefix + '(' + set + ')',
                           schema);
  } else {
    return {};
  }
}

}  // namespace

bool Dump_loader::report_duplicates(
    const std::string &what, const std::string &schema,
    std::list<Dump_reader::Object_info *> *objects,
    mysqlshdk::db::IResult *result) {
  bool has_duplicates = false;
  const auto mark_existing =
      [objects](const std::function<bool(const std::string &)> &matches) {
        const auto end = objects->end();

        for (auto it = objects->begin(); it != end; ++it) {
          if (matches((*it)->name)) {
            (*it)->exists = true;
            // erase the object to speed up subsequent searches
            objects->erase(it);

            return true;
          }
        }

        return false;
      };

  while (auto row = result->fetch_one()) {
    const auto name = row->get_string(0);
    const auto msg = "Schema `" + schema + "` already contains " + what +
                     " named `" + name + "`";

    if (m_options.ignore_existing_objects())
      current_console()->print_note(msg);
    else
      current_console()->print_error(msg);

    has_duplicates = true;

    // first use exact match
    if (!mark_existing(
            [&name](const std::string &object) { return name == object; })) {
      // try again using case insensitive comparison
      const auto wide_name = shcore::utf8_to_wide(name);

      if (!mark_existing([&wide_name](const std::string &object) {
            return shcore::str_caseeq(wide_name, shcore::utf8_to_wide(object));
          })) {
        log_info(
            "Could not find in metadata %s named `%s` which was reported as "
            "a duplicate in schema `%s`.",
            what.c_str(), name.c_str(), schema.c_str());
      }
    }
  }

  // all existing objects were removed, mark remaining ones as non-existing
  for (auto object : *objects) {
    assert(!object->exists.has_value());
    object->exists = false;
  }

  return has_duplicates;
}

void Dump_loader::check_existing_objects() {
  auto console = current_console();

  console->print_status("Checking for pre-existing objects...");

  bool has_duplicates = false;

  if (m_options.load_users()) {
    std::set<std::string> accounts;
    for (const auto &a : m_dump->accounts()) {
      if (m_options.filters().users().is_included(a))
        accounts.emplace(shcore::str_lower(
            shcore::str_format("'%s'@'%s'", a.user.c_str(), a.host.c_str())));
    }

    auto result = sql::ar::query(
        m_reconnect_callback, m_session,
        "SELECT DISTINCT grantee FROM information_schema.user_privileges");
    for (auto row = result->fetch_one(); row; row = result->fetch_one()) {
      auto grantee = row->get_string(0);
      if (accounts.count(shcore::str_lower(grantee))) {
        const auto msg = "Account " + grantee + " already exists";

        if (m_options.ignore_existing_objects())
          current_console()->print_note(msg);
        else
          current_console()->print_error(msg);

        has_duplicates = true;
      }
    }
  }

  // Case handling:
  // Partition, subpartition, column, index, stored routine, event, and
  // resource group names are not case-sensitive on any platform, nor are
  // column aliases. Schema, table and trigger names depend on the value of
  // lower_case_table_names

  // Get list of schemas being loaded that already exist
  std::string set = shcore::str_join(
      m_dump->schemas(), ",",
      [](const std::string &s) { return shcore::quote_sql_string(s); });
  if (set.empty()) return;

  auto result =
      sql::ar::query(m_reconnect_callback, m_session,
                     "SELECT schema_name FROM information_schema.schemata "
                     "WHERE schema_name in (" +
                         set + ")");
  std::vector<std::string> dup_schemas = fetch_names(result.get());

  for (const auto &schema : dup_schemas) {
    std::list<Dump_reader::Object_info *> tables;
    std::list<Dump_reader::Object_info *> views;
    std::list<Dump_reader::Object_info *> triggers;
    std::list<Dump_reader::Object_info *> functions;
    std::list<Dump_reader::Object_info *> procedures;
    std::list<Dump_reader::Object_info *> events;

    if (!m_dump->schema_objects(schema, &tables, &views, &triggers, &functions,
                                &procedures, &events))
      continue;

    result = query_names(m_reconnect_callback, m_session, schema, tables,
                         "SELECT table_name FROM information_schema.tables"
                         " WHERE table_schema = ? AND table_name in ");
    if (result)
      has_duplicates |=
          report_duplicates("a table", schema, &tables, result.get());

    result = query_names(m_reconnect_callback, m_session, schema, views,
                         "SELECT table_name FROM information_schema.views"
                         " WHERE table_schema = ? AND table_name in ");
    if (result)
      has_duplicates |=
          report_duplicates("a view", schema, &views, result.get());

    result = query_names(m_reconnect_callback, m_session, schema, triggers,
                         "SELECT trigger_name FROM information_schema.triggers"
                         " WHERE trigger_schema = ? AND trigger_name in ");
    if (result)
      has_duplicates |=
          report_duplicates("a trigger", schema, &triggers, result.get());

    result =
        query_names(m_reconnect_callback, m_session, schema, functions,
                    "SELECT routine_name FROM information_schema.routines"
                    " WHERE routine_schema = ? AND routine_type = 'FUNCTION'"
                    " AND routine_name in ");
    if (result)
      has_duplicates |=
          report_duplicates("a function", schema, &functions, result.get());

    result =
        query_names(m_reconnect_callback, m_session, schema, procedures,
                    "SELECT routine_name FROM information_schema.routines"
                    " WHERE routine_schema = ? AND routine_type = 'PROCEDURE'"
                    " AND routine_name in ");
    if (result)
      has_duplicates |=
          report_duplicates("a procedure", schema, &procedures, result.get());

    result = query_names(m_reconnect_callback, m_session, schema, events,
                         "SELECT event_name FROM information_schema.events"
                         " WHERE event_schema = ? AND event_name in ");
    if (result)
      has_duplicates |=
          report_duplicates("an event", schema, &events, result.get());
  }

  if (has_duplicates) {
    if (m_options.ignore_existing_objects()) {
      console->print_note(
          "One or more objects in the dump already exist in the destination "
          "database but will be ignored because the 'ignoreExistingObjects' "
          "option was enabled.");
    } else {
      console->print_error(
          "One or more objects in the dump already exist in the destination "
          "database. You must either DROP these objects or exclude them from "
          "the load.");
      THROW_ERROR(SHERR_LOAD_DUPLICATE_OBJECTS_FOUND);
    }
  }
}

void Dump_loader::setup_progress_file(bool *out_is_resuming) {
  auto console = current_console();

  m_load_log = std::make_unique<Load_progress_log>();

  if (!m_options.progress_file().has_value() ||
      !m_options.progress_file()->empty()) {
    auto progress_file = m_dump->create_progress_file_handle();
    const auto path = progress_file->full_path().masked();
    bool rewrite_on_flush = !progress_file->is_local();

    auto progress = m_load_log->init(std::move(progress_file),
                                     m_options.dry_run(), rewrite_on_flush);
    if (progress.status != Load_progress_log::PENDING) {
      if (!m_options.reset_progress()) {
        console->print_note(
            "Load progress file detected. Load will be resumed from where it "
            "was left, assuming no external updates were made.");
        console->print_info(
            "You may enable the 'resetProgress' option to discard progress "
            "for this MySQL instance and force it to be completely "
            "reloaded.");
        *out_is_resuming = true;

        log_info("Resuming load, last loaded %s bytes (%s rows)",
                 std::to_string(progress.data_bytes_completed).c_str(),
                 std::to_string(progress.rows_completed).c_str());
        // Recall the partial progress that was made before
        m_data_bytes_previously_loaded = progress.data_bytes_completed;
        m_rows_previously_loaded = progress.rows_completed;

        m_stats.total_data_bytes = progress.data_bytes_completed;
        m_stats.total_file_bytes = progress.file_bytes_completed;
        m_stats.total_records = progress.rows_completed;
      } else {
        console->print_note(
            "Load progress file detected for the instance but "
            "'resetProgress' option was enabled. Load progress will be "
            "discarded and the whole dump will be reloaded.");

        m_load_log->reset_progress();
      }
    } else {
      log_info("Logging load progress to %s", path.c_str());
    }

    m_load_log->set_server_uuid(m_options.server_uuid());
  }
}

void Dump_loader::execute_threaded(const std::function<bool()> &schedule_next) {
  do {
    // handle events from workers and schedule more chunks when a worker
    // becomes available
    size_t num_idle_workers = handle_worker_events(schedule_next);

    if (num_idle_workers == m_workers.size()) {
      // make sure that there's really no more work. schedule_work() is
      // supposed to just return false without doing anything. If it does
      // something (and returns true), then we have a bug.
      assert(!schedule_next() || m_worker_interrupt);
      break;
    }
  } while (!m_worker_interrupt);
}

void Dump_loader::execute_table_ddl_tasks() {
  m_ddl_executed = 0;
  std::atomic<uint64_t> ddl_to_execute = 0;
  std::atomic<bool> all_tasks_scheduled = false;

  dump::Progress_thread::Progress_config config;
  config.current = [this]() -> uint64_t { return m_ddl_executed; };
  config.total = [&ddl_to_execute]() { return ddl_to_execute.load(); };
  config.is_total_known = [&all_tasks_scheduled]() {
    return all_tasks_scheduled.load();
  };

  const auto stage =
      m_progress_thread.start_stage("Executing DDL", std::move(config));
  shcore::on_leave_scope finish_stage([this, stage]() {
    stage->finish();
    m_total_ddl_executed += m_ddl_executed;
    m_total_ddl_execution_seconds += stage->duration().seconds();
  });

  // Create all schemas, all tables and all view placeholders.
  // Views and other objects must only be created after all
  // tables/placeholders from all schemas are created, because there may be
  // cross-schema references.
  Load_progress_log::Status schema_load_status;
  std::string schema;
  std::list<Dump_reader::Name_and_file> tables;
  std::list<Dump_reader::Name_and_file> view_placeholders;

  const auto thread_pool_ptr = m_dump->create_thread_pool();
  const auto pool = thread_pool_ptr.get();
  shcore::Synchronized_queue<std::unique_ptr<Worker::Task>> worker_tasks;

  const auto handle_ddl_files =
      [this, pool, &worker_tasks, &ddl_to_execute](
          const std::string &s, std::list<Dump_reader::Name_and_file> *list,
          bool placeholder, Load_progress_log::Status schema_status) {
// GCC 12 may warn about a possibly uninitialized usage of IFile in the lambda
// capture
#if __GNUC__ >= 12 && !defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#endif
        if (should_fetch_table_ddl(placeholder)) {
          for (auto &item : *list) {
            if (!item.second) {
              continue;
            }

            const auto exists =
                m_options.ignore_existing_objects()
                    ? (placeholder ? m_dump->view_exists(s, item.first)
                                   : m_dump->table_exists(s, item.first))
                    : false;

            if (placeholder && exists) {
              // BUG#35102738: do not recreate existing views due to
              // BUG#35154429
              continue;
            }

            const auto status =
                placeholder
                    ? schema_status
                    : m_load_log->status(progress::Table_ddl{s, item.first});

            ++ddl_to_execute;

            pool->add_task(
                [file = std::move(item.second), s, table = item.first]() {
                  log_debug("Fetching table DDL for %s.%s", s.c_str(),
                            table.c_str());
                  file->open(mysqlshdk::storage::Mode::READ);
                  auto script = mysqlshdk::storage::read_file(file.get());
                  file->close();
                  return script;
                },
                [s, table = item.first, placeholder, &worker_tasks, status,
                 exists](std::string &&data) {
                  worker_tasks.push(std::make_unique<Worker::Table_ddl_task>(
                      s, table, std::move(data), placeholder, status, exists));
                });
          }
        }
#if __GNUC__ >= 12 && !defined(__clang__)
#pragma GCC diagnostic pop
#endif
      };

  log_debug("Begin loading table DDL");

  pool->start_threads();
  const auto &pool_status = pool->process_async();

  while (!m_worker_interrupt) {
    // fetch next schema and process it
    if (!m_dump->next_schema_and_tables(&schema, &tables, &view_placeholders)) {
      break;
    }

    schema_load_status = m_load_log->status(progress::Schema_ddl{schema});

    log_debug("Schema DDL for '%s' (%s)", schema.c_str(),
              to_string(schema_load_status).c_str());

    bool is_schema_ready = true;

    if (schema_load_status == Load_progress_log::DONE ||
        !m_options.load_ddl()) {
      // we track views together with the schema DDL, so no need to
      // load placeholders if schemas was already loaded
    } else {
      handle_ddl_files(schema, &view_placeholders, true, schema_load_status);

      ++ddl_to_execute;
      is_schema_ready = false;

      pool->add_task(
          [this, schema]() {
            log_debug("Fetching schema DDL for %s", schema.c_str());
            return m_dump->fetch_schema_script(schema);
          },
          [&worker_tasks, schema,
           resuming =
               schema_load_status ==
               mysqlsh::Load_progress_log::INTERRUPTED](std::string &&data) {
            worker_tasks.push(std::make_unique<Worker::Schema_ddl_task>(
                                  schema, std::move(data), resuming),
                              shcore::Queue_priority::HIGH);
          },
          shcore::Thread_pool::Priority::HIGH);
    }

    m_schema_ddl_ready[schema] = is_schema_ready;

    handle_ddl_files(schema, &tables, false, schema_load_status);
  }

  all_tasks_scheduled = true;
  auto pending_tasks = ddl_to_execute.load();
  pool->tasks_done();

  {
    std::list<std::unique_ptr<Worker::Task>> schema_tasks;
    std::list<std::unique_ptr<Worker::Task>> table_tasks;

    execute_threaded([this, &pool_status, &worker_tasks, &pending_tasks,
                      &schema_tasks, &table_tasks]() {
      while (!m_worker_interrupt) {
        // if there was an exception in the thread pool, interrupt the process
        if (pool_status == shcore::Thread_pool::Async_state::TERMINATED) {
          m_worker_interrupt = true;
          break;
        }

        std::unique_ptr<Worker::Task> work;

        // don't fetch too many tasks at once, as we may starve worker threads
        auto tasks_to_fetch = m_options.threads_count();

        // move tasks from the queue in the processing thread to the main thread
        while (pending_tasks > 0 && tasks_to_fetch > 0) {
          // just have a peek, we may have other tasks to execute
          auto work_opt = worker_tasks.try_pop(std::chrono::milliseconds{1});
          if (!work_opt) break;

          work = std::move(*work_opt);
          if (!work) break;

          --pending_tasks;
          --tasks_to_fetch;

          (work->table().empty() ? schema_tasks : table_tasks)
              .emplace_back(std::move(work));
        }

        // schedule schema tasks first
        if (!schema_tasks.empty()) {
          work = std::move(schema_tasks.front());
          schema_tasks.pop_front();
        } else if (!table_tasks.empty()) {
          // select table task for a schema which is ready, with the lowest
          // number of concurrent tasks
          const auto end = table_tasks.end();
          auto best = end;
          auto best_count = std::numeric_limits<uint64_t>::max();

          for (auto it = table_tasks.begin(); it != end; ++it) {
            const auto &s = (*it)->schema();

            if (m_schema_ddl_ready[s]) {
              auto count = m_ddl_in_progress_per_schema[s];

              if (count < best_count) {
                best_count = count;
                best = it;
              }

              // the best possible outcome, exit immediately
              if (0 == best_count) {
                break;
              }
            }
          }

          if (end != best) {
            // mark the schema as being loaded
            ++m_ddl_in_progress_per_schema[(*best)->schema()];
            // schedule task for execution
            work = std::move(*best);
            // remove the task
            table_tasks.erase(best);
          }
        }

        // if there's work, schedule it
        if (work) {
          push_pending_task(std::move(work));
          return true;
        }

        // no more work to do, finish
        if (schema_tasks.empty() && table_tasks.empty() && 0 == pending_tasks) {
          break;
        }
      }

      return false;
    });

    // notify the pool in case of an interrupt
    if (m_worker_interrupt) {
      pool->terminate();
    }

    pool->wait_for_process();
  }

  // no need to keep these any more
  m_schema_ddl_ready.clear();
  m_ddl_in_progress_per_schema.clear();

  log_debug("End loading table DDL");
}

void Dump_loader::execute_view_ddl_tasks() {
  m_ddl_executed = 0;
  std::atomic<uint64_t> ddl_to_execute = 0;
  std::atomic<bool> all_tasks_scheduled = false;

  dump::Progress_thread::Progress_config config;
  config.current = [this]() -> uint64_t { return m_ddl_executed; };
  config.total = [&ddl_to_execute]() { return ddl_to_execute.load(); };
  config.is_total_known = [&all_tasks_scheduled]() {
    return all_tasks_scheduled.load();
  };

  const auto stage =
      m_progress_thread.start_stage("Executing view DDL", std::move(config));
  shcore::on_leave_scope finish_stage([this, stage]() {
    stage->finish();
    m_total_ddl_executed += m_ddl_executed;
    m_total_ddl_execution_seconds += stage->duration().seconds();
  });

  Load_progress_log::Status schema_load_status;
  std::string schema;
  std::list<Dump_reader::Name_and_file> views;
  std::unordered_map<std::string, uint64_t> views_per_schema;

  const auto thread_pool_ptr = m_dump->create_thread_pool();
  const auto pool = thread_pool_ptr.get();

  log_debug("Begin loading view DDL");

  // the DDL is executed in the main thread, in order to avoid concurrency
  // issues (i.e. one thread removes the placeholder table for a view, while
  // another one tries to create a view which references that deleted view)

  pool->start_threads();

  while (!m_worker_interrupt) {
    if (!m_dump->next_schema_and_views(&schema, &views)) {
      break;
    }

    schema_load_status = m_load_log->status(progress::Schema_ddl{schema});

    if (schema_load_status != Load_progress_log::DONE) {
      if (views.empty()) {
        // there are no views, mark schema as ready
        m_load_log->log(progress::end::Schema_ddl{schema});
      } else {
        ddl_to_execute += views.size();
        views_per_schema[schema] = views.size();

        // GCC 12 may warn about a possibly uninitialized usage of IFile in the
        // lambda capture
#if __GNUC__ >= 12 && !defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#endif
        for (auto &item : views) {
          if (m_options.ignore_existing_objects() &&
              m_dump->view_exists(schema, item.first)) {
            // BUG#35102738: do not recreate existing views due to BUG#35154429
            --ddl_to_execute;
            --views_per_schema[schema];
            continue;
          }

          pool->add_task(
              [file = std::move(item.second), schema, view = item.first]() {
                log_debug("Fetching view DDL for %s.%s", schema.c_str(),
                          view.c_str());
                file->open(mysqlshdk::storage::Mode::READ);
                auto script = mysqlshdk::storage::read_file(file.get());
                file->close();
                return script;
              },
              [this, schema, view = item.first,
               resuming = schema_load_status == Load_progress_log::INTERRUPTED,
               pool, &views_per_schema](std::string &&script) {
                log_info("%s DDL script for view `%s`.`%s`",
                         (resuming ? "Re-executing" : "Executing"),
                         schema.c_str(), view.c_str());

                if (!m_options.dry_run()) {
                  execute_script(
                      m_reconnect_callback, m_session,
                      use_schema(schema, script),
                      shcore::str_format(
                          "Error executing DDL script for view `%s`.`%s`",
                          schema.c_str(), view.c_str()),
                      m_default_sql_transforms);
                }

                m_dump->set_view_exists(schema, view);

                ++m_ddl_executed;

                if (0 == --views_per_schema[schema]) {
                  m_load_log->log(progress::end::Schema_ddl{schema});
                }

                if (m_worker_interrupt) {
                  pool->terminate();
                }
              });
        }
#if __GNUC__ >= 12 && !defined(__clang__)
#pragma GCC diagnostic pop
#endif
      }
    }
  }

  all_tasks_scheduled = true;
  pool->tasks_done();
  pool->process();

  log_debug("End loading view DDL");
}

void Dump_loader::execute_tasks(bool testing) {
  auto console = current_console();

  if (m_options.dry_run())
    console->print_info("dryRun enabled, no changes will be made.");

  check_server_version();

  m_reconnect_callback = [this]() { m_session = create_session(); };
  m_reconnect_callback();

  log_server_version();

  setup_progress_file(&m_resuming);

  // the 1st potentially slow operation, as many errors should be detected
  // before this as possible
  wait_for_metadata();

  handle_schema_option();

  if (!m_resuming && m_options.load_ddl()) check_existing_objects();

  check_tables_without_primary_key();

  size_t num_idle_workers = 0;

  do {
    if (!m_init_done) {
      // process dump metadata first
      on_dump_begin();

      // NOTE: this assumes that all DDL files are already present

      if (m_options.load_ddl() || m_options.load_deferred_indexes()) {
        // exec DDL for all tables in parallel (fetching DDL for
        // thousands of tables from remote storage can be slow)
        if (!m_worker_interrupt) execute_table_ddl_tasks();

        // users have to be loaded after all objects and view placeholders are
        // created, because GRANTs on specific objects require the objects to
        // exist
        // users have to be loaded before view placeholders are replaced with
        // views, because creating a view which uses another view with the
        // DEFINER clause requires that user to exist
        if (!m_worker_interrupt) load_users();

        // exec DDL for all views after all tables and users are created
        if (!m_worker_interrupt && m_options.load_ddl())
          execute_view_ddl_tasks();
      }

      if (!m_worker_interrupt && !testing) setup_temp_tables();

      m_init_done = true;

      if (!m_worker_interrupt && m_options.load_data()) {
        setup_load_data_progress();
      }
    }

    if (!m_worker_interrupt &&
        m_dump->status() != Dump_reader::Status::COMPLETE) {
      scan_for_more_data();
    }

    if (!m_worker_interrupt) {
      m_total_tables_with_data = m_dump->tables_with_data();

      // handle events from workers and schedule more chunks when a worker
      // becomes available
      num_idle_workers =
          handle_worker_events([this]() { return schedule_next_task(); });
    }

    // If the whole dump is already available and there's no more data to be
    // loaded and all workers are idle (done loading), then we're done
    if (!m_worker_interrupt &&
        m_dump->status() == Dump_reader::Status::COMPLETE &&
        !m_dump->data_available() && num_idle_workers == m_workers.size()) {
      if (!m_dump->work_available()) {
        break;
      } else if (m_dump->data_pending()) {
        // dump is complete, there is no more data, but work is still pending
        THROW_ERROR(SHERR_LOAD_CORRUPTED_DUMP_MISSING_DATA);
      }
    }
  } while (!m_worker_interrupt);

  if (!m_worker_interrupt) {
    on_dump_end();
    m_load_log->cleanup();
  }

  log_debug("Import done");
}

void Dump_loader::wait_for_metadata() {
  const auto start_time = std::chrono::steady_clock::now();
  const auto console = current_console();
  bool waited = false;

  // we need to have the whole metadata before we proceed with the load

  while (!m_dump->ready() && !m_worker_interrupt) {
    log_debug("Scanning for metadata");

    m_dump->rescan(&m_progress_thread);

    if (m_dump->ready()) {
      log_debug("Dump metadata available");
      return;
    }

    if (Dump_reader::Status::COMPLETE == m_dump->status()) {
      // dump is complete (the last file was written), but it's not ready,
      // meaning some of the metadata files are missing
      THROW_ERROR(SHERR_LOAD_CORRUPTED_DUMP_MISSING_METADATA);
    }

    if (!waited) {
      console->print_status("Waiting for metadata to become available...");
      waited = true;
    }

    wait_for_dump(start_time);
  }
}

bool Dump_loader::scan_for_more_data(bool wait) {
  const auto start_time = std::chrono::steady_clock::now();
  const auto console = current_console();
  bool waited = false;

  // if there are still idle workers, check if there's more that was dumped
  while (m_dump->status() != Dump_reader::Status::COMPLETE &&
         !m_worker_interrupt) {
    log_debug("Scanning for more data");

    m_dump->rescan();

    if (m_dump->data_available()) {
      log_debug("Dump data available");
      return true;
    }

    if (!wait) {
      break;
    }

    if (m_dump->status() == Dump_reader::Status::DUMPING) {
      if (!waited) {
        console->print_status("Waiting for more data to become available...");
        waited = true;
      }

      wait_for_dump(start_time);
    }
  }

  log_debug("Scan did not find more data");

  return false;
}

void Dump_loader::wait_for_dump(
    std::chrono::steady_clock::time_point start_time) {
  if (m_options.dump_wait_timeout_ms() > 0) {
    const auto current_time = std::chrono::steady_clock::now();
    const auto time_diff =
        std::chrono::duration_cast<std::chrono::milliseconds>(current_time -
                                                              start_time)
            .count();
    if (static_cast<uint64_t>(time_diff) >= m_options.dump_wait_timeout_ms()) {
      current_console()->print_warning(
          "Timeout while waiting for dump to finish. Imported data may be "
          "incomplete.");

      THROW_ERROR(SHERR_LOAD_DUMP_WAIT_TIMEOUT);
    }
  } else {
    // Dump isn't complete yet, but we're not waiting for it
    return;
  }

  if (m_options.dump_wait_timeout_ms() < 1000) {
    shcore::sleep_ms(m_options.dump_wait_timeout_ms());
  } else {
    // wait for at most 5s at a time and try again
    for (uint64_t j = 0;
         j < std::min<uint64_t>(5000, m_options.dump_wait_timeout_ms()) &&
         !m_worker_interrupt;
         j += 1000) {
      shcore::sleep_ms(1000);
    }
  }
}

void Dump_loader::spawn_workers() {
  m_thread_exceptions.resize(m_options.threads_count());

  for (uint64_t i = 0; i < m_options.threads_count(); i++) {
    m_workers.emplace_back(i, this);

    Worker &worker = m_workers.back();

    m_worker_threads.push_back(mysqlsh::spawn_scoped_thread([&worker]() {
      mysqlsh::Mysql_thread mysql_thread;

      worker.run();
    }));
  }

  m_monitoring = std::make_unique<Monitoring>(this);
}

void Dump_loader::join_workers() {
  log_debug("Waiting on worker threads...");
  for (auto &w : m_workers) w.stop();

  for (auto &t : m_worker_threads)
    if (t.joinable()) t.join();
  log_debug("All worker threads stopped");

  m_monitoring.reset();
}

void Dump_loader::clear_worker(Worker *worker) {
  assert(m_monitoring);
  m_monitoring->remove_worker(worker);

  const auto wid = worker->id();
  m_worker_threads[wid].join();
  m_workers.remove_if([wid](const Worker &w) { return w.id() == wid; });
}

void Dump_loader::post_worker_event(Worker *worker, Worker_event::Event event,
                                    shcore::Dictionary_t &&details) {
  m_worker_events.push(Worker_event{event, worker, std::move(details)});
}

void Dump_loader::on_schema_ddl_start(std::size_t,
                                      const Worker::Schema_ddl_task *task) {
  m_load_log->log(progress::start::Schema_ddl{task->schema()});
}

void Dump_loader::on_schema_ddl_end(std::size_t,
                                    const Worker::Schema_ddl_task *) {}

void Dump_loader::on_table_ddl_start(std::size_t worker_id,
                                     const Worker::Table_ddl_task *task) {
  if (!task->placeholder()) {
    m_load_log->log(progress::start::Table_ddl{
        task->schema(), task->table(), {worker_id, task->weight()}});
  }
}

void Dump_loader::on_table_ddl_end(std::size_t, Worker::Table_ddl_task *task) {
  const auto &schema = task->schema();

  if (!task->placeholder()) {
    const auto &table = task->table();

    m_load_log->log(progress::end::Table_ddl{schema, table});

    if (auto indexes = task->steal_deferred_statements();
        indexes && !indexes->empty()) {
      m_indexes_to_recreate +=
          m_dump->add_deferred_statements(schema, table, std::move(*indexes));
    }

    if (m_options.load_ddl()) {
      m_dump->set_table_exists(schema, table);
    }
  }

  on_ddl_done_for_schema(schema);
}

void Dump_loader::on_chunk_load_start(std::size_t worker_id,
                                      const Worker::Load_chunk_task *task) {
  const auto &chunk = task->chunk();

  m_load_log->log(progress::start::Table_chunk{chunk.schema,
                                               chunk.table,
                                               chunk.partition,
                                               chunk.index,
                                               {worker_id, task->weight()}});
}

void Dump_loader::on_chunk_load_end(std::size_t,
                                    const Worker::Load_chunk_task *task) {
  const auto &chunk = task->chunk();
  const auto &stats = task->stats;
  const size_t data_bytes_loaded = stats.total_data_bytes;
  const size_t file_bytes_loaded = stats.total_file_bytes;

  m_load_log->log(progress::end::Table_chunk{
      chunk.schema, chunk.table, chunk.partition, chunk.index,
      data_bytes_loaded, file_bytes_loaded, stats.total_records});

  m_dump->on_chunk_loaded(chunk);

  m_unique_tables_loaded.insert(
      schema_table_object_key(chunk.schema, chunk.table, chunk.partition));

  log_debug("Ended loading chunk %s (%s, %s)", format_table(chunk).c_str(),
            std::to_string(data_bytes_loaded).c_str(),
            std::to_string(file_bytes_loaded).c_str());
}

void Dump_loader::on_subchunk_load_start(std::size_t worker_id,
                                         const shcore::Dictionary_t &event,
                                         const Worker::Load_chunk_task *task) {
  const auto &chunk = task->chunk();

  m_load_log->log(progress::start::Table_subchunk{chunk.schema,
                                                  chunk.table,
                                                  chunk.partition,
                                                  chunk.index,
                                                  event->get_uint("subchunk"),
                                                  {worker_id, task->weight()}});
}

void Dump_loader::on_subchunk_load_end(std::size_t,
                                       const shcore::Dictionary_t &event,
                                       const Worker::Load_chunk_task *task) {
  const auto &chunk = task->chunk();

  m_load_log->log(progress::end::Table_subchunk{
      chunk.schema, chunk.table, chunk.partition, chunk.index,
      event->get_uint("subchunk"), event->get_uint("bytes")});
}

void Dump_loader::on_bulk_load_start(std::size_t worker_id,
                                     const Worker::Bulk_load_task *task) {
  const auto &chunk = task->chunk();

  m_load_log->log(progress::start::Bulk_load{
      chunk.schema, chunk.table, chunk.partition, {worker_id, task->weight()}});
}

void Dump_loader::on_bulk_load_progress(const shcore::Dictionary_t &event) {
  m_load_log->log(progress::update::Bulk_load{
      event->get_string("schema"), event->get_string("table"),
      event->get_string("partition"), event->get_uint("data_bytes")});
}

void Dump_loader::on_bulk_load_end(std::size_t,
                                   const Worker::Bulk_load_task *task) {
  const auto &chunk = task->chunk();
  const auto &stats = task->stats;
  const size_t data_bytes_loaded = stats.total_data_bytes;
  const size_t file_bytes_loaded = stats.total_file_bytes;

  m_load_log->log(progress::end::Bulk_load{
      chunk.schema, chunk.table, chunk.partition, data_bytes_loaded,
      file_bytes_loaded, stats.total_records});

  m_dump->on_table_loaded(chunk);

  m_unique_tables_loaded.insert(
      schema_table_object_key(chunk.schema, chunk.table, chunk.partition));

  log_debug("Ended bulk loading table %s (%s, %s)", format_table(chunk).c_str(),
            std::to_string(data_bytes_loaded).c_str(),
            std::to_string(file_bytes_loaded).c_str());
}

void Dump_loader::on_index_start(std::size_t worker_id,
                                 const Worker::Index_recreation_task *task) {
  assert(m_create_indexes_stage);
  if (!m_create_indexes_stage->is_started()) {
    m_create_indexes_stage->start();
  }

  m_load_log->log(progress::start::Table_indexes{
      task->schema(),
      task->table(),
      {worker_id, task->weight()},
      task->indexes() ? task->indexes()->size() : 0});
}

void Dump_loader::on_index_end(std::size_t,
                               const Worker::Index_recreation_task *task) {
  const auto &schema = task->schema();
  const auto &table = task->table();

  m_dump->on_index_end(schema, table);
  m_load_log->log(progress::end::Table_indexes{schema, table});
}

void Dump_loader::on_analyze_start(std::size_t worker_id,
                                   const Worker::Analyze_table_task *task) {
  assert(m_analyze_tables_stage);
  if (!m_analyze_tables_stage->is_started()) {
    m_analyze_tables_stage->start();
  }

  m_load_log->log(progress::start::Analyze_table{
      task->schema(), task->table(), {worker_id, task->weight()}});
}

void Dump_loader::on_analyze_end(std::size_t,
                                 const Worker::Analyze_table_task *task) {
  const auto &schema = task->schema();
  const auto &table = task->table();

  m_dump->on_analyze_end(schema, table);
  m_load_log->log(progress::end::Analyze_table{schema, table});
}

void Dump_loader::on_checksum_start(
    const dump::common::Checksums::Checksum_data *) {
  assert(m_checksum_tables_stage);
  if (!m_checksum_tables_stage->is_started()) {
    m_checksum_tables_stage->start();
  }
}

void Dump_loader::on_checksum_end(
    const dump::common::Checksums::Checksum_data *checksum,
    const Worker::Checksum_task::Checksum_result &result) {
  assert(checksum);

  ++m_checksum_tasks_completed;
  m_dump->on_checksum_end(checksum->schema(), checksum->table(),
                          checksum->partition());

  if (!result.first) {
    std::string msg =
        "Checksum verification failed for: " + format_table(checksum);

    if (!checksum->boundary().empty()) {
      msg += " (boundary: " + checksum->boundary() + ")";
    }

    if (checksum->result().count != result.second.count) {
      msg += ". Mismatched number of rows, expected: " +
             std::to_string(checksum->result().count) +
             ", actual: " + std::to_string(result.second.count);
    }

    msg += '.';

    report_checksum_error(msg);
  }
}

void Dump_loader::report_checksum_error(const std::string &msg) {
  ++m_checksum_errors;
  current_console()->print_error(msg);
}

bool Dump_loader::maybe_push_checksum_task(
    const dump::common::Checksums::Checksum_data *data) {
  assert(data);

  if (!m_options.checksum()) {
    return false;
  }

  if (!m_dump->table_exists(data->schema(), data->table())) {
    report_checksum_error("Could not verify checksum of " + format_table(data) +
                          ": table does not exist.");
    return false;
  }

  push_pending_task(checksum(data));

  return true;
}

void Dump_loader::Sql_transform::add_strip_removed_sql_modes() {
  // Remove NO_AUTO_CREATE_USER from sql_mode, which doesn't exist in 8.0 but
  // does in 5.7

  add([](std::string_view sql, std::string *out_new_sql) {
    static std::regex re(
        R"*((\/\*![0-9]+\s+)?(SET\s+sql_mode\s*=\s*')(.*)('.*))*",
        std::regex::icase | std::regex::optimize);

    std::cmatch m;
    if (std::regex_match(sql.data(), sql.data() + sql.size(), m, re)) {
      auto modes = shcore::str_split(m[3].str(), ",");
      std::string new_modes;
      for (const auto &mode : modes) {
        if (mode != "NO_AUTO_CREATE_USER")
          new_modes.append(mode).append(1, ',');
      }
      if (!new_modes.empty()) new_modes.pop_back();  // strip last ,

      *out_new_sql = m[1].str() + m[2].str() + new_modes + m[4].str();
    } else {
      *out_new_sql = sql;
    }
  });
}

void Dump_loader::Sql_transform::add_execute_conditionally(
    std::function<bool(std::string_view, const std::string &)> f) {
  add([f = std::move(f)](std::string_view sql, std::string *out_new_sql) {
    *out_new_sql = sql;

    std::string sql_str{sql};
    mysqlshdk::utils::SQL_iterator it(sql_str, 0, false);

    while (it.valid()) {
      auto token = it.next_token();

      if (!shcore::str_caseeq(token, "CREATE", "ALTER", "DROP")) continue;

      auto type = it.next_token();

      if (shcore::str_caseeq(type, "DEFINER")) {
        // =
        it.next_token();
        // user
        it.next_token();
        // type or @
        type = it.next_token();

        if (shcore::str_caseeq(type, "@")) {
          // continuation of an account, host
          it.next_token();
          // type
          type = it.next_token();
        }
      }

      if (shcore::str_caseeq(type, "EVENT", "FUNCTION", "PROCEDURE",
                             "TRIGGER")) {
        auto name = it.next_token();

        if (shcore::str_caseeq(name, "IF")) {
          // NOT or EXISTS
          token = it.next_token();

          if (shcore::str_caseeq(token, "NOT")) {
            // EXISTS
            it.next_token();
          }

          // name follows
          name = it.next_token();
        }

        // name can be either object_name, `object_name` or
        // schema.`object_name`, split_schema_and_table will handle all these
        // cases and unquote the object name
        std::string object_name;
        shcore::split_schema_and_table(std::string{name}, nullptr, &object_name,
                                       true);

        if (!f(type, object_name)) {
          out_new_sql->clear();
        }
      }

      return;
    }
  });
}

void Dump_loader::Sql_transform::add_rename_schema(std::string_view new_name) {
  add([new_name = std::string{new_name}](std::string_view sql,
                                         std::string *out_new_sql) {
    std::string sql_str{sql};

    mysqlshdk::utils::SQL_iterator it(sql_str, 0, false);
    const auto token = it.next_token();

    if (shcore::str_caseeq(token, "CREATE")) {
      if (shcore::str_caseeq(it.next_token(), "DATABASE", "SCHEMA")) {
        auto schema = it.next_token();

        if (shcore::str_caseeq(schema, "IF")) {
          // NOT
          it.next_token();
          // EXISTS
          it.next_token();
          // schema follows
          schema = it.next_token();
        }

        *out_new_sql = sql_str.substr(0, it.position() - schema.length()) +
                       shcore::quote_identifier(new_name) +
                       sql_str.substr(it.position());
        return;
      }
    } else if (shcore::str_caseeq(token, "USE")) {
      *out_new_sql = "USE " + shcore::quote_identifier(new_name);
      return;
    }

    *out_new_sql = std::move(sql_str);
  });
}

void Dump_loader::handle_schema_option() {
  if (!m_options.target_schema().empty()) {
    m_dump->replace_target_schema(m_options.target_schema());
    m_default_sql_transforms.add_rename_schema(m_options.target_schema());
  }
}

bool Dump_loader::should_create_pks() const {
  return m_dump->should_create_pks();
}

void Dump_loader::setup_load_data_progress() {
  // Progress mechanics:
  // - if the dump is complete when it's opened, we show progress and
  // throughput relative to total uncompressed size
  //      pct% (current GB / total GB), thrp MB/s

  // - if the dump is incomplete when it's opened, we show # of uncompressed
  // bytes loaded so far and the throughput
  //      current GB compressed ready; current GB loaded, thrp MB/s

  // - when the dump completes during load, we switch to displaying progress
  // relative to the total size
  if (m_load_data_stage) {
    return;
  }

  dump::Progress_thread::Throughput_config config;

  config.space_before_item = false;
  config.initial = [this]() { return m_data_bytes_previously_loaded; };
  config.current = [this]() -> uint64_t {
    if (is_data_load_complete()) {
      // this callback can be called before m_load_data_stage is assigned to
      if (m_load_data_stage) {
        // finish the stage when all data is loaded
        m_load_data_stage->finish(false);
      }
    }

    return m_stats.total_data_bytes;
  };
  config.total = [this]() { return m_dump->filtered_data_size(); };

  config.left_label = [this]() {
    std::string label;

    if (m_dump->status() != Dump_reader::Status::COMPLETE) {
      label += "Dump still in progress";

      if (m_dump->total_file_size()) {
        label += ", " +
                 mysqlshdk::utils::format_bytes(m_dump->total_file_size()) +
                 " ready";
      }

      label += " - ";
    }

    const auto threads_loading = m_num_threads_loading.load();

    if (threads_loading) {
      label += std::to_string(threads_loading) + " thds loading - ";
    }

    const auto threads_indexing = m_num_threads_recreating_indexes.load();

    if (threads_indexing) {
      label += std::to_string(threads_indexing) + " thds indexing - ";
    }

    if (const auto checksumming = m_num_threads_checksumming.load()) {
      label += std::to_string(checksumming) + " thds chksum - ";
    }

    static const char k_progress_spin[] = "-\\|/";
    static size_t progress_idx = 0;

    if (label.length() > 2) {
      label[label.length() - 2] = k_progress_spin[progress_idx++];

      if (progress_idx >= shcore::array_size(k_progress_spin) - 1) {
        progress_idx = 0;
      }
    }

    return label;
  };

  if (m_bulk_load) {
    // BULK LOAD does not update Innodb_rows_inserted, remove this once
    // BUG#36286488 is fixed
    update_rows_throughput(m_rows_previously_loaded);
    m_monitoring->add(
        [this, prev = m_rows_previously_loaded](const Session_ptr &) mutable {
          const auto rows = m_stats.total_records.load();

          // total records count is updated only once a data load task is
          // finished, in order to avoid fluctuation of throughput we update it
          // only if count has changed
          if (rows > prev) {
            prev = rows;
            update_rows_throughput(rows);
          }
        });
  } else {
    m_monitoring->add([this](const Session_ptr &session) {
      // no reconnection - we're using the monitoring session
      update_rows_throughput(
          sql::query(session,
                     "SELECT CAST(VARIABLE_VALUE AS UNSIGNED) FROM "
                     "performance_schema.global_status WHERE "
                     "VARIABLE_NAME='Innodb_rows_inserted'")
              ->fetch_one_or_throw()
              ->get_uint(0));
    });
  }

  config.right_label = [this]() {
    std::string rows_throughput = " (";
    rows_throughput += format_rows_throughput(rows_throughput_rate(), 1.0);
    rows_throughput += ')';

    return shcore::str_format(
        "%s, %zu / %zu tables%s done", rows_throughput.c_str(),
        m_unique_tables_loaded.size(), m_total_tables_with_data,
        m_dump->has_partitions() ? " and partitions" : "");
  };
  config.on_display_started = []() {
    current_console()->print_status("Starting data load");
  };

  m_load_data_stage =
      m_progress_thread.start_stage("Loading data", std::move(config));
}

void Dump_loader::setup_create_indexes_progress() {
  if (m_create_indexes_stage) {
    return;
  }

  m_indexes_recreated = 0;
  m_index_count_is_known = false;

  dump::Progress_thread::Progress_config config;
  config.current = [this]() -> uint64_t { return m_indexes_recreated; };
  config.total = [this]() { return m_indexes_to_recreate; };
  config.is_total_known = [this]() { return m_index_count_is_known; };

  m_create_indexes_stage =
      m_progress_thread.push_stage("Recreating indexes", std::move(config));
}

void Dump_loader::setup_analyze_tables_progress() {
  if (m_analyze_tables_stage) {
    return;
  }

  m_tables_analyzed = 0;
  m_tables_to_analyze = 0;
  m_all_analyze_tasks_scheduled = false;

  dump::Progress_thread::Progress_config config;
  config.current = [this]() -> uint64_t { return m_tables_analyzed; };
  config.total = [this]() { return m_tables_to_analyze; };
  config.is_total_known = [this]() { return m_all_analyze_tasks_scheduled; };

  m_analyze_tables_stage =
      m_progress_thread.push_stage("Analyzing tables", std::move(config));
}

void Dump_loader::setup_checksum_tables_progress() {
  if (m_checksum_tables_stage) {
    return;
  }

  m_checksum_tasks_completed = 0;
  m_checksum_tasks_to_complete = 0;
  m_all_checksum_tasks_scheduled = false;

  dump::Progress_thread::Progress_config config;
  config.current = [this]() { return m_checksum_tasks_completed; };
  config.total = [this]() { return m_checksum_tasks_to_complete; };
  config.is_total_known = [this]() { return m_all_checksum_tasks_scheduled; };

  m_checksum_tables_stage = m_progress_thread.push_stage(
      "Verifying checksum information", std::move(config));
}

void Dump_loader::add_skipped_schema(const std::string &schema) {
  std::lock_guard<std::recursive_mutex> lock(m_skip_schemas_mutex);
  m_skip_schemas.insert(schema);
}

void Dump_loader::on_ddl_done_for_schema(const std::string &schema) {
  // notify that DDL for a schema has finished loading
  auto it = m_ddl_in_progress_per_schema.find(schema);
  assert(m_ddl_in_progress_per_schema.end() != it);

  if (0 == --(it->second)) {
    m_ddl_in_progress_per_schema.erase(it);
  }
}

bool Dump_loader::is_data_load_complete() const {
  return Dump_reader::Status::COMPLETE == m_dump->status() &&
         m_stats.total_data_bytes >= m_dump->filtered_data_size();
}

void Dump_loader::log_server_version() const {
  log_info("Destination server: %s",
           sql::ar::query(m_reconnect_callback, m_session,
                          "SELECT CONCAT(@@version, ' ', @@version_comment)")
               ->fetch_one()
               ->get_string(0)
               .c_str());
}

void Dump_loader::push_pending_task(Task_ptr task) {
  size_t weight = std::min(task->weight(), m_options.threads_count());
  task->set_weight(weight);

  if (weight > 1) {
    log_debug("Pushing new task for %s with weight %zu", task->key().c_str(),
              weight);
  }

  m_pending_tasks.emplace(std::move(task));
}

Dump_loader::Task_ptr Dump_loader::load_chunk_file(
    Dump_reader::Table_chunk chunk, bool resuming,
    uint64_t bytes_to_skip) const {
  log_debug("Loading data for %s", format_table(chunk).c_str());
  assert(!chunk.schema.empty());
  assert(!chunk.table.empty());
  assert(chunk.file);

  // The reason why sending work to worker threads isn't done through a
  // regular queue is because a regular queue would create a static schedule
  // for the chunk loading order. But we need to be able to dynamically
  // schedule chunks based on the current conditions at the time each new
  // chunk needs to be scheduled.
  return std::make_unique<Worker::Load_chunk_task>(std::move(chunk), resuming,
                                                   bytes_to_skip);
}

Dump_loader::Task_ptr Dump_loader::bulk_load_table(
    Dump_reader::Table_chunk chunk, bool resuming) const {
  log_debug("Bulk loading data for %s", format_table(chunk).c_str());
  assert(!chunk.schema.empty());
  assert(!chunk.table.empty());
  assert(chunk.file);

  return std::make_unique<Worker::Bulk_load_task>(
      std::move(chunk), resuming, m_options.bulk_load_info().threads);
}

Dump_loader::Task_ptr Dump_loader::recreate_indexes(
    const std::string &schema, const std::string &table,
    compatibility::Deferred_statements::Index_info *indexes) const {
  log_debug("Recreating indexes for `%s`.`%s`", schema.c_str(), table.c_str());
  assert(!schema.empty());
  assert(!table.empty());

  uint64_t weight = m_options.threads_per_add_index();

  if (weight > 1) {
    if (auto table_size = m_dump->table_data_size(schema, table)) {
      // size per each thread
      table_size /= weight;

      // in case of small tables, we assume that they're not that impactful
      if (table_size <= 10 * 1024 * 1024) {  // 10MiB
        weight = 1;
      }
    }  // else, we don't have the size info, just use the default weight
  }

  DBUG_EXECUTE_IF("dump_loader_force_index_weight", { weight = 4; });

  return std::make_unique<Worker::Index_recreation_task>(schema, table, indexes,
                                                         weight);
}

Dump_loader::Task_ptr Dump_loader::analyze_table(
    const std::string &schema, const std::string &table,
    const std::vector<Dump_reader::Histogram> &histograms) const {
  log_debug("Analyzing table `%s`.`%s`", schema.c_str(), table.c_str());
  assert(!schema.empty());
  assert(!table.empty());

  return std::make_unique<Worker::Analyze_table_task>(schema, table,
                                                      histograms);
}

Dump_loader::Task_ptr Dump_loader::checksum(
    const dump::common::Checksums::Checksum_data *data) const {
  log_debug("Verifying checksum for %s", format_table(data).c_str());
  assert(data);

  return std::make_unique<Worker::Checksum_task>(data);
}

void Dump_loader::load_users() {
  if (!m_options.load_users()) {
    return;
  }

  const auto script = m_dump->users_script();
  const auto console = current_console();

  console->print_status("Executing user accounts SQL...");

  std::function<bool(const std::string &, const std::string &)>
      strip_revoked_privilege;

  if (m_options.is_mds()) {
    strip_revoked_privilege = filter_user_script_for_mds();
  }

  const auto statements = dump::Schema_dumper::preprocess_users_script(
      script,
      [this](const std::string &account) {
        return m_options.filters().users().is_included(account);
      },
      strip_revoked_privilege);

  if (m_options.dry_run()) {
    return;
  }

  std::unordered_set<std::string> all_accounts;
  std::unordered_set<std::string> ignored_accounts;

  const auto reconnect = [&, this]() {
    m_reconnect_callback();

    m_dropped_accounts = 0;
    m_ignored_grant_errors = 0;
    all_accounts.clear();
    ignored_accounts.clear();
  };

  sql::ar::run(reconnect, [&]() {
    std::string new_stmt;

    for (const auto &group : statements) {
      if (!group.account.empty()) {
        all_accounts.emplace(group.account);
      }

      if (ignored_accounts.count(group.account) > 0) {
        continue;
      }

      for (std::string_view stmt : group.statements) {
        // repeated to stop executing statements if one of the grants fails
        if (ignored_accounts.count(group.account) > 0) {
          continue;
        }

        if (m_default_sql_transforms(stmt, &new_stmt)) {
          stmt = std::move(new_stmt);
        }

        if (stmt.empty()) {
          continue;
        }

        try {
          execute_statement(m_session, stmt,
                            "While executing user accounts SQL");
        } catch (const mysqlshdk::db::Error &e) {
          if (dump::Schema_dumper::User_statements::Type::GRANT != group.type) {
            throw;
          }

          switch (m_options.on_grant_errors()) {
            case Load_dump_options::Handle_grant_errors::ABORT:
              throw;

            case Load_dump_options::Handle_grant_errors::DROP_ACCOUNT: {
              console->print_note(
                  "Due to the above error the account " + group.account +
                  " was dropped, the load operation will continue.");
              ignored_accounts.emplace(group.account);

              const auto drop = "DROP USER IF EXISTS " + group.account;
              execute_statement(m_session, drop,
                                "While dropping the account " + group.account);
              ++m_dropped_accounts;
              break;
            }

            case Load_dump_options::Handle_grant_errors::IGNORE:
              console->print_note(
                  "The above error was ignored, the load operation will "
                  "continue.");
              ++m_ignored_grant_errors;
              break;
          }
        }
      }
    }
  });

  m_loaded_accounts = all_accounts.size() - ignored_accounts.size();
}

void Dump_loader::show_metadata(bool force) const {
  if (force || m_options.show_metadata()) {
    m_dump->show_metadata();
  }
}

bool Dump_loader::bulk_load_supported(
    const Dump_reader::Table_chunk &chunk) const {
  // BULK LOAD is not enabled
  if (!m_bulk_load) {
    return false;
  }

  const auto table_name = schema_object_key(chunk.schema, chunk.table);
  const auto no_bulk_load = [&table_name](const std::string &msg) {
    log_info("Table %s will not use BULK LOAD: %s", table_name.c_str(),
             msg.c_str());
  };

  // if data for this table is not complete, then bulk load cannot be used
  if (!chunk.dump_complete) {
    no_bulk_load("data dump for this table is not complete");
    return false;
  }

  // some other chunk was already loaded, i.e. because table is not compatible;
  // table has some data, bulk load cannot be used
  if (0 != chunk.index) {
    // no log here, because it would be printed for each chunk of each of the
    // incompatible tables
    return false;
  }

  // This is the first chunk, if it's in the progress log, then load is being
  // resumed and either: table didn't meet requirements to use the bulk load,
  // previous load was done by an older version which didn't support bulk load,
  // instance previously did not support bulk load. In any case, it's safer to
  // load chunks normally.
  if (Load_progress_log::PENDING !=
      m_load_log->status(progress::Table_chunk{chunk.schema, chunk.table,
                                               chunk.partition, chunk.index})) {
    no_bulk_load("load is resumed, and chunk did not use BULK LOAD before");
    return false;
  }

  // data should not be compressed or use zstd compression
  if (mysqlshdk::storage::Compression::NONE != chunk.compression &&
      mysqlshdk::storage::Compression::ZSTD != chunk.compression) {
    no_bulk_load("unsupported compression: " + to_string(chunk.compression));
    return false;
  }

  // check if this table is compatible with BULK LOAD
  if (!m_bulk_load->ensure_table_compatible(chunk, m_reconnect_callback,
                                            m_session)) {
    no_bulk_load("not compatible");
    return false;
  }

  log_info("Table %s will use BULK LOAD", table_name.c_str());
  return true;
}

bool Dump_loader::schedule_bulk_load(Dump_reader::Table_chunk chunk) {
  // set chunk to an invalid value, so it's not printed out
  chunk.index = -1;
  // mark all chunks as consumed to prevent them from being scheduled
  // individually
  m_dump->consume_table(chunk);

  const auto status = m_load_log->status(
      progress::Bulk_load{chunk.schema, chunk.table, chunk.partition});

  if (status == Load_progress_log::DONE) {
    m_dump->on_table_loaded(chunk);
  }

  log_debug("Bulk loading table data for %s (%s)", format_table(chunk).c_str(),
            to_string(status).c_str());

  if (!m_options.load_data() || status == Load_progress_log::DONE ||
      !is_table_included(chunk.schema, chunk.table)) {
    return false;
  }

  chunk.file_size =
      m_dump->partition_file_size(chunk.schema, chunk.table, chunk.partition);
  chunk.data_size =
      m_dump->partition_data_size(chunk.schema, chunk.table, chunk.partition);

  log_debug("Scheduling bulk load of table %s (%s)",
            format_table(chunk).c_str(),
            chunk.file->full_path().masked().c_str());

  mark_table_as_in_progress(chunk);
  push_pending_task(bulk_load_table(std::move(chunk),
                                    status == Load_progress_log::INTERRUPTED));

  return true;
}

bool Dump_loader::is_table_included(const std::string &schema,
                                    const std::string &table) {
  std::lock_guard<std::recursive_mutex> lock(m_skip_schemas_mutex);

  if (m_skip_schemas.find(schema) != m_skip_schemas.end() ||
      m_skip_tables.find(schema_object_key(schema, table)) !=
          m_skip_tables.end() ||
      !m_dump->include_table(schema, table)) {
    return false;
  }

  return true;
}

void Dump_loader::mark_table_as_in_progress(
    const Dump_reader::Table_chunk &chunk) {
  std::lock_guard<std::mutex> lock(m_tables_being_loaded_mutex);
  m_tables_being_loaded.emplace(
      schema_table_object_key(chunk.schema, chunk.table, chunk.partition),
      chunk.file_size);
}

void Dump_loader::setup_temp_tables() {
  if (m_options.dry_run()) {
    return;
  }

  {
    // drop all existing temporary tables
    const auto result = sql::ar::queryf(
        m_reconnect_callback, m_session,
        "SELECT TABLE_SCHEMA, TABLE_NAME FROM information_schema.tables WHERE "
        "TABLE_COMMENT=?",
        k_temp_table_comment);

    std::vector<std::pair<std::string, std::string>> tables;

    while (auto row = result->fetch_one()) {
      tables.emplace_back(row->get_string(0), row->get_string(1));
    }

    for (const auto &table : tables) {
      log_info("Dropping temporary table: `%s`.`%s`", table.first.c_str(),
               table.second.c_str());

      sql::ar::executef(m_reconnect_callback, m_session,
                        "DROP TABLE IF EXISTS !.!", table.first, table.second);
    }
  }

  {
    // find a unique prefix
    m_temp_table_prefix = "mysqlsh_tmp_";
    std::string suffix;

    do {
      suffix = shcore::get_random_string(5, "0123456789ABCDEF");
    } while (sql::ar::queryf(m_reconnect_callback, m_session,
                             "SELECT 1 FROM information_schema.tables WHERE "
                             "TABLE_NAME LIKE ? '%'",
                             m_temp_table_prefix + suffix)
                 ->fetch_one());

    m_temp_table_prefix += suffix;
    m_temp_table_prefix += '_';
  }
}

std::string Dump_loader::temp_table_name() {
  std::string name = m_temp_table_prefix;
  name += std::to_string(++m_temp_table_suffix);
  return name;
}

void Dump_loader::update_rows_throughput(uint64_t rows) {
  std::unique_lock lock{m_rows_throughput_mutex, std::try_to_lock};

  if (lock.owns_lock()) {
    m_rows_throughput.push(rows);
  }
}

uint64_t Dump_loader::rows_throughput_rate() {
  std::lock_guard lock{m_rows_throughput_mutex};
  return m_rows_throughput.rate();
}

#if defined(__clang__)
// -Wmissing-braces
#pragma clang diagnostic pop
#endif

}  // namespace mysqlsh
