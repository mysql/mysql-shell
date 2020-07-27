/*
 * Copyright (c) 2020, Oracle and/or its affiliates.
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

#include "modules/util/dump/dumper.h"

#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <iterator>
#include <utility>

#include <mysqld_error.h>

#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>

#include "mysqlshdk/include/shellcore/console.h"
#include "mysqlshdk/include/shellcore/interrupt_handler.h"
#include "mysqlshdk/include/shellcore/shell_init.h"
#include "mysqlshdk/include/shellcore/shell_options.h"
#include "mysqlshdk/libs/db/mysql/session.h"
#include "mysqlshdk/libs/db/mysqlx/session.h"
#include "mysqlshdk/libs/storage/compressed_file.h"
#include "mysqlshdk/libs/storage/idirectory.h"
#include "mysqlshdk/libs/storage/utils.h"
#include "mysqlshdk/libs/textui/textui.h"
#include "mysqlshdk/libs/utils/profiling.h"
#include "mysqlshdk/libs/utils/rate_limit.h"
#include "mysqlshdk/libs/utils/std.h"
#include "mysqlshdk/libs/utils/strformat.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_net.h"
#include "mysqlshdk/libs/utils/utils_sqlstring.h"
#include "mysqlshdk/libs/utils/utils_string.h"

#include "modules/mod_utils.h"
#include "modules/util/dump/console_with_progress.h"
#include "modules/util/dump/dialect_dump_writer.h"
#include "modules/util/dump/dump_manifest.h"
#include "modules/util/dump/dump_utils.h"
#include "modules/util/dump/schema_dumper.h"
#include "modules/util/dump/text_dump_writer.h"

namespace mysqlsh {
namespace dump {
using mysqlshdk::storage::Mode;
using mysqlshdk::storage::backend::Memory_file;

namespace {

static constexpr auto k_dump_in_progress_ext = ".dumping";

static constexpr const int k_mysql_server_net_write_timeout = 30 * 60;
static constexpr const int k_mysql_server_wait_timeout = 365 * 24 * 60 * 60;

std::string quote_value(const std::string &value, mysqlshdk::db::Type type) {
  if (is_string_type(type)) {
    return shcore::quote_sql_string(value);
  } else if (mysqlshdk::db::Type::Decimal == type) {
    return "'" + value + "'";
  } else {
    return value;
  }
}

std::string trim_in_progress_extension(const std::string &s) {
  if (shcore::str_iendswith(s, k_dump_in_progress_ext)) {
    return s.substr(0, s.length() - strlen(k_dump_in_progress_ext));
  } else {
    return s;
  }
}

auto ref(const std::string &s) {
  return rapidjson::StringRef(s.c_str(), s.length());
}

std::string to_string(rapidjson::Document *doc) {
  rapidjson::StringBuffer buffer;
  rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
  doc->Accept(writer);
  return std::string{buffer.GetString(), buffer.GetSize()};
}

void write_json(std::unique_ptr<mysqlshdk::storage::IFile> file,
                rapidjson::Document *doc) {
  const auto json = to_string(doc);
  file->open(Mode::WRITE);
  file->write(json.c_str(), json.length());
  file->close();
}

}  // namespace

class Dumper::Synchronize_workers final {
 public:
  Synchronize_workers() = default;
  ~Synchronize_workers() = default;

  void wait_for(const uint16_t count) {
    std::unique_lock<std::mutex> lock(m_mutex);
    m_condition.wait(lock, [this, count]() { return m_count >= count; });
    m_count -= count;
  }

  void notify() {
    {
      std::lock_guard<std::mutex> lock(m_mutex);
      ++m_count;
    }
    m_condition.notify_one();
  }

 private:
  std::mutex m_mutex;
  std::condition_variable m_condition;
  uint16_t m_count = 0;
};

class Dumper::Table_worker final {
 public:
  enum class Exception_strategy { ABORT, CONTINUE };

  Table_worker() = delete;

  Table_worker(std::size_t id, Dumper *dumper, Exception_strategy strategy)
      : m_id(id), m_dumper(dumper), m_strategy(strategy) {}

  Table_worker(const Table_worker &) = delete;
  Table_worker(Table_worker &&) = default;

  Table_worker &operator=(const Table_worker &) = delete;
  Table_worker &operator=(Table_worker &&) = delete;

  ~Table_worker() = default;

  void run() {
    try {
      mysqlsh::Mysql_thread mysql_thread;
      shcore::on_leave_scope close_session([this]() {
        if (m_session) {
          m_session->close();
        }
      });

      open_session();

      m_rate_limit =
          mysqlshdk::utils::Rate_limit(m_dumper->m_options.max_rate());

      while (true) {
        const auto func = m_dumper->m_worker_tasks.pop();

        if (m_dumper->m_worker_interrupt) {
          return;
        }

        if (!func) {
          break;
        }

        func(this);

        if (m_dumper->m_worker_interrupt) {
          return;
        }
      }
    } catch (const std::exception &e) {
      handle_exception(e.what());
    } catch (...) {
      handle_exception("Unknown exception");
    }
  }

 private:
  friend class Dumper;

  void open_session() {
    // notify dumper that the session has been established
    shcore::on_leave_scope notify_dumper(
        [this]() { m_dumper->m_worker_synchronization->notify(); });

    m_session =
        establish_session(m_dumper->session()->get_connection_options(), false);

    m_dumper->start_transaction(m_session);
    m_dumper->on_init_thread_session(m_session);
  }

  std::string prepare_query(const Table_data_task &table) const {
    const auto base64 = m_dumper->m_options.use_base64();
    std::string query = "SELECT SQL_NO_CACHE ";

    for (const auto &column : table.columns) {
      if (column.csv_unsafe) {
        query += shcore::sqlstring(base64 ? "TO_BASE64(!)" : "HEX(!)", 0)
                 << column.name;
      } else {
        query += shcore::sqlstring("!", 0) << column.name;
      }

      query += ",";
    }

    // remove last comma
    query.pop_back();

    query += shcore::sqlstring(" FROM !.!", 0) << table.schema << table.name;

    if (!table.range.begin.empty()) {
      query += shcore::sqlstring(" WHERE ! BETWEEN ", 0) << table.index.name;
      query += quote_value(table.range.begin, table.range.type);
      query += " AND ";
      query += quote_value(table.range.end, table.range.type);

      if (table.include_nulls) {
        query += shcore::sqlstring(" OR ! IS NULL", 0) << table.index.name;
      }
    }

    if (!table.index.name.empty()) {
      query += shcore::sqlstring(" ORDER BY !", 0) << table.index.name;
    }

    query += " " + m_dumper->get_query_comment(table, "dumping");

    return query;
  }

  void dump_table_data(const Table_data_task &table) {
    Dump_write_result bytes_written_per_file;
    Dump_write_result bytes_written_per_update{table.schema, table.name};
    uint64_t rows_written_per_update = 0;
    const uint64_t update_every = 2000;
    uint64_t rows_written_per_idx = 0;
    const uint64_t write_idx_every = 500;
    Dump_write_result bytes_written;
    mysqlshdk::utils::Profile_timer timer;

    timer.stage_begin("dumping");

    const auto result = m_session->query(prepare_query(table));

    shcore::on_leave_scope close_index_file([&table]() {
      if (table.index_file) {
        table.index_file->close();
      }
    });

    table.writer->open();
    if (table.index_file) {
      table.index_file->open(Mode::WRITE);
    }
    bytes_written = table.writer->write_preamble(result->get_metadata());
    bytes_written_per_file += bytes_written;
    bytes_written_per_update += bytes_written;

    while (const auto row = result->fetch_one()) {
      if (m_dumper->m_worker_interrupt) {
        return;
      }

      bytes_written = table.writer->write_row(row);
      bytes_written_per_file += bytes_written;
      bytes_written_per_update += bytes_written;
      ++rows_written_per_update;
      ++rows_written_per_idx;

      if (table.index_file && write_idx_every == rows_written_per_idx) {
        // the idx file contains offsets to the data stream, not to binary one
        const auto offset = mysqlshdk::utils::host_to_network(
            bytes_written_per_file.data_bytes());
        table.index_file->write(&offset, sizeof(uint64_t));

        rows_written_per_idx = 0;
      }

      if (update_every == rows_written_per_update) {
        m_dumper->update_progress(rows_written_per_update,
                                  bytes_written_per_update);

        // we don't know how much data was read from the server, number of
        // bytes written to the dump file is a good approximation
        if (m_rate_limit.enabled()) {
          m_rate_limit.throttle(bytes_written_per_update.data_bytes());
        }

        rows_written_per_update = 0;
        bytes_written_per_update.reset();
      }
    }

    bytes_written = table.writer->write_postamble();
    bytes_written_per_file += bytes_written;
    bytes_written_per_update += bytes_written;

    timer.stage_end();

    if (table.index_file) {
      const auto total = mysqlshdk::utils::host_to_network(
          bytes_written_per_file.data_bytes());
      table.index_file->write(&total, sizeof(uint64_t));
    }

    log_debug("Dump of `%s`.`%s` into '%s' took %f seconds",
              table.schema.c_str(), table.name.c_str(),
              table.writer->output()->full_path().c_str(),
              timer.total_seconds_elapsed());

    m_dumper->finish_writing(table.writer);
    m_dumper->update_progress(rows_written_per_update,
                              bytes_written_per_update);
  }

  void push_table_data_task(Table_data_task &&task) {
    // Table_data_task contains an unique_ptr, it's moved into shared_ptr so it
    // can be move-captured by lambda
    std::shared_ptr<Table_data_task> t =
        std::make_shared<Table_data_task>(std::move(task));
    m_dumper->m_worker_tasks.push(
        [task = std::move(t)](Table_worker *worker) {
          ++worker->m_dumper->m_num_threads_dumping;

          worker->dump_table_data(*task);

          --worker->m_dumper->m_num_threads_dumping;
        },
        shcore::Queue_priority::LOW);
  }

  void create_table_data_task(const Table_task &table) {
    Table_data_task data_task;

    data_task.name = table.name;
    data_task.index = table.index;
    data_task.schema = table.schema;
    data_task.columns = table.columns;
    data_task.writer = m_dumper->get_table_data_writer(
        m_dumper->get_table_data_filename(table.basename));
    if (!m_dumper->m_options.is_export_only()) {
      data_task.index_file = m_dumper->make_file(
          m_dumper->get_table_data_filename(table.basename) + ".idx");
    }
    data_task.id = "1";

    push_table_data_task(std::move(data_task));
  }

  void create_table_data_task(const Table_task &table,
                              Dumper::Range_info &&range, const std::string &id,
                              std::size_t idx, bool last_chunk) {
    Table_data_task data_task;

    data_task.name = table.name;
    data_task.index = table.index;
    data_task.schema = table.schema;
    data_task.columns = table.columns;
    data_task.range = std::move(range);
    data_task.include_nulls = 0 == idx;
    data_task.writer = m_dumper->get_table_data_writer(
        m_dumper->get_table_data_filename(table.basename, idx, last_chunk));
    if (!m_dumper->m_options.is_export_only()) {
      data_task.index_file = m_dumper->make_file(
          m_dumper->get_table_data_filename(table.basename, idx, last_chunk) +
          ".idx");
    }
    data_task.id = id;

    push_table_data_task(std::move(data_task));
  }

  void write_table_metadata(const Table_task &table) {
    m_dumper->write_table_metadata(table, m_session);
  }

  void create_table_data_tasks(const Table_task &table) {
    auto ranges = create_ranged_tasks(table);

    if (0 == ranges) {
      create_table_data_task(table);
      ++ranges;
    }

    current_console()->print_status(
        "Data dump for table " + Dumper::quote(table.schema, table.name) +
        " will be written to " + std::to_string(ranges) + " file" +
        (ranges > 1 ? "s" : ""));

    m_dumper->chunking_task_finished();
  }

  std::size_t create_ranged_tasks(const Table_task &table) {
    if (!m_dumper->is_chunked(table)) {
      return 0;
    }

    auto result = m_session->queryf(
        "SELECT SQL_NO_CACHE MIN(!), MAX(!) FROM !.!;", table.index.name,
        table.index.name, table.schema, table.name);
    result->buffer();
    const auto min_max = result->fetch_one();

    if (min_max->is_null(0)) {
      return 0;
    }

    mysqlshdk::utils::Profile_timer timer;
    timer.stage_begin("chunking");

    std::size_t ranges_count = 0;
    std::string range_end;
    const Range_info total = {min_max->get_as_string(0),
                              min_max->get_as_string(1), min_max->get_type(0)};

    const auto rows_per_chunk =
        m_dumper->m_options.bytes_per_chunk() /
        std::max(UINT64_C(1), get_average_row_length(table));

    const auto generate_ranges = [&table, &ranges_count, &total,
                                  &rows_per_chunk,
                                  this](const auto min, const auto max) {
      const auto estimated_chunks =
          rows_per_chunk > 0
              ? std::max(table.row_count / rows_per_chunk, UINT64_C(1))
              : UINT64_C(1);
      const auto estimated_step = (max - min + 1) / estimated_chunks;
      const auto accuracy = std::max(estimated_step / 10, UINT64_C(10));

      std::string chunk_id;

      const auto next_step =
          table.row_count < 1'000'000
              ? std::function<decltype(min)(const decltype(min),
                                            const decltype(min))>(
                    [](const auto, const auto step) { return step; })
              : [&table, &rows_per_chunk, &accuracy, &chunk_id, this](
                    const auto from, const auto step) {
                  auto left = from;
                  auto right = left + 2 * step;

                  auto middle = from;
                  auto previous_row_count = rows_per_chunk;
                  const auto comment = this->get_query_comment(table, chunk_id);

                  for (int i = 0; i < 10; ++i) {
                    middle = left + (right - left) / 2;

                    if (middle >= right || middle <= left) {
                      break;
                    }

                    const auto rows =
                        m_session
                            ->queryf(
                                "EXPLAIN SELECT COUNT(*) FROM !.! WHERE ! "
                                "BETWEEN ? AND ? ORDER BY ! " +
                                    comment,
                                table.schema, table.name, table.index.name,
                                from, middle, table.index.name)
                            ->fetch_one()
                            ->get_uint(9);

                    uint64_t delta = 0;

                    if (rows > rows_per_chunk) {
                      right = middle;
                      delta = rows - rows_per_chunk;
                    } else {
                      left = middle;
                      delta = rows_per_chunk - rows;
                    }

                    if (delta <= accuracy) {
                      // we're close enough
                      break;
                    }

                    if (rows == previous_row_count) {
                      // we're stuck
                      break;
                    }

                    previous_row_count = rows;
                  }

                  return middle - from;
                };

      auto current = min;
      auto step = static_cast<decltype(min)>(estimated_step);

      while (current <= max) {
        if (m_dumper->m_worker_interrupt) {
          return;
        }

        chunk_id = std::to_string(ranges_count);

        Range_info range;
        range.type = total.type;
        range.begin = std::to_string(current);

        step = next_step(current, step);

        current += step - 1;

        // if current is greater than max or close to it, finish the chunking
        if (current > max || max - current <= step / 4) {
          current = max;
        }

        range.end = std::to_string(current);

        create_table_data_task(table, std::move(range), chunk_id,
                               ranges_count++, current >= max);

        ++current;
      }
    };

    if (mysqlshdk::db::Type::Integer == total.type) {
      generate_ranges(min_max->get_int(0), min_max->get_int(1));
    } else if (mysqlshdk::db::Type::UInteger == total.type) {
      generate_ranges(min_max->get_uint(0), min_max->get_uint(1));
    } else {
      do {
        const auto where =
            0 == ranges_count
                ? ""
                : (shcore::sqlstring(
                       " WHERE ! > " + quote_value(range_end, total.type), 0)
                   << table.index.name)
                      .str();

        const auto chunk_id = std::to_string(ranges_count);
        const auto comment = get_query_comment(table, chunk_id);

        Range_info range;
        range.type = total.type;
        range.begin = m_session
                          ->queryf("SELECT SQL_NO_CACHE ! FROM !.!" + where +
                                       " ORDER BY ! LIMIT 0,1 " + comment,
                                   table.index.name, table.schema, table.name,
                                   table.index.name)
                          ->fetch_one()
                          ->get_as_string(0);

        if (m_dumper->m_worker_interrupt) {
          return 0;
        }

        result = m_session->queryf("SELECT SQL_NO_CACHE ! FROM !.!" + where +
                                       " ORDER BY ! LIMIT ?,1 " + comment,
                                   table.index.name, table.schema, table.name,
                                   table.index.name, rows_per_chunk - 1);

        if (m_dumper->m_worker_interrupt) {
          return 0;
        }

        const auto end = result->fetch_one();
        range.end = end && !end->is_null(0) ? end->get_as_string(0) : total.end;
        range_end = range.end;

        create_table_data_task(table, std::move(range), chunk_id,
                               ranges_count++, range_end == total.end);
      } while (range_end != total.end);
    }

    timer.stage_end();
    log_debug("Chunking of `%s`.`%s` took %f seconds", table.schema.c_str(),
              table.name.c_str(), timer.total_seconds_elapsed());

    return ranges_count;
  }

  uint64_t get_average_row_length(const Table_task &table) const {
    const auto result = m_session->queryf(
        "SELECT AVG_ROW_LENGTH FROM information_schema.tables WHERE "
        "TABLE_SCHEMA = ? AND TABLE_NAME = ?",
        table.schema, table.name);
    return result->fetch_one()->get_uint(0);
  }

  void handle_exception(const char *msg) {
    m_dumper->m_worker_exceptions[m_id] = std::current_exception();
    current_console()->print_error(shcore::str_format("[Worker%03zu]: ", m_id) +
                                   msg);

    if (Exception_strategy::ABORT == m_strategy) {
      m_dumper->emergency_shutdown();
    }
  }

  void dump_schema_ddl(const Schema_task &schema) const {
    const auto quoted = quote(schema);
    current_console()->print_status("Writing DDL for schema " + quoted);

    const auto dumper = m_dumper->schema_dumper(m_session);

    m_dumper->write_ddl(*m_dumper->dump_schema(dumper.get(), schema.name),
                        get_schema_filename(schema.basename));
  }

  void dump_table_ddl(const Schema_task &schema,
                      const Table_info &table) const {
    const auto quoted = quote(schema, table);
    current_console()->print_status("Writing DDL for table " + quoted);

    const auto dumper = m_dumper->schema_dumper(m_session);

    m_dumper->write_ddl(
        *m_dumper->dump_table(dumper.get(), schema.name, table.name),
        get_table_filename(table.basename));

    if (m_dumper->m_options.dump_triggers() &&
        dumper->count_triggers_for_table(schema.name, table.name) > 0) {
      m_dumper->write_ddl(
          *m_dumper->dump_triggers(dumper.get(), schema.name, table.name),
          dump::get_table_data_filename(table.basename, "triggers.sql"));
    }
  }

  void dump_view_ddl(const Schema_task &schema, const Table_info &view) const {
    const auto quoted = quote(schema, view);
    current_console()->print_status("Writing DDL for view " + quoted);

    const auto dumper = m_dumper->schema_dumper(m_session);

    // DDL file with the temporary table
    m_dumper->write_ddl(
        *m_dumper->dump_temporary_view(dumper.get(), schema.name, view.name),
        dump::get_table_data_filename(view.basename, "pre.sql"));

    // DDL file with the view structure
    m_dumper->write_ddl(
        *m_dumper->dump_view(dumper.get(), schema.name, view.name),
        get_table_filename(view.basename));
  }

  std::string get_query_comment(const Table_task &table,
                                const std::string &id) const {
    return m_dumper->get_query_comment(table.schema, table.name, id,
                                       "chunking");
  }

  const std::size_t m_id;
  Dumper *m_dumper;
  Exception_strategy m_strategy;
  mysqlshdk::utils::Rate_limit m_rate_limit;
  std::shared_ptr<mysqlshdk::db::ISession> m_session;
};

class Dumper::Dump_info final {
 public:
  Dump_info() = delete;

  explicit Dump_info(const std::shared_ptr<mysqlshdk::db::ISession> &session) {
    const auto &co = session->get_connection_options();

    m_user = co.get_user();

    {
      const auto result = session->query("SELECT @@GLOBAL.HOSTNAME;");

      if (const auto row = result->fetch_one()) {
        m_server = row->get_string(0);
      } else {
        m_server = co.has_host() ? co.get_host() : "localhost";
      }
    }

    {
      const auto result = session->query("SELECT @@GLOBAL.VERSION;");

      if (const auto row = result->fetch_one()) {
        m_server_version = row->get_string(0);
      } else {
        m_server_version = "unknown";
      }
    }

    m_hostname = mysqlshdk::utils::Net::get_hostname();

    {
      const auto result = session->query("SELECT @@GLOBAL.GTID_EXECUTED;");

      if (const auto row = result->fetch_one()) {
        m_gtid_executed = row->get_string(0);
      }
    }

    m_begin = mysqlshdk::utils::fmttime("%Y-%m-%d %T");

    m_timer.stage_begin("total");
  }

  Dump_info(const Dump_info &) = default;
  Dump_info(Dump_info &&) = default;

  Dump_info &operator=(const Dump_info &) = default;
  Dump_info &operator=(Dump_info &&) = default;

  ~Dump_info() = default;

  void finish() {
    m_timer.stage_end();

    m_end = mysqlshdk::utils::fmttime("%Y-%m-%d %T");

    const auto sec = static_cast<unsigned long long int>(seconds());

    m_duration = shcore::str_format("%02llu:%02llu:%02llus", sec / 3600ull,
                                    (sec % 3600ull) / 60ull, sec % 60ull);
  }

  const std::string &user() const { return m_user; }

  const std::string &hostname() const { return m_hostname; }

  const std::string &server() const { return m_server; }

  const std::string &server_version() const { return m_server_version; }

  const std::string &begin() const { return m_begin; }

  const std::string &end() const { return m_end; }

  const std::string &duration() const { return m_duration; }

  double seconds() const { return m_timer.total_seconds_elapsed(); }

  const std::string &gtid_executed() const { return m_gtid_executed; }

 private:
  std::string m_user;
  std::string m_hostname;
  std::string m_server;
  std::string m_server_version;
  mysqlshdk::utils::Profile_timer m_timer;
  std::string m_begin;
  std::string m_end;
  std::string m_duration;
  std::string m_gtid_executed;
};

class Dumper::Memory_dumper final {
 public:
  Memory_dumper() = delete;

  explicit Memory_dumper(Schema_dumper *dumper)
      : m_dumper(dumper), m_file("/dev/null") {}

  Memory_dumper(const Memory_dumper &) = delete;
  Memory_dumper(Memory_dumper &&) = default;

  Memory_dumper &operator=(const Memory_dumper &) = delete;
  Memory_dumper &operator=(Memory_dumper &&) = default;

  ~Memory_dumper() = default;

  const std::vector<Schema_dumper::Issue> &dump(
      const std::function<void(Memory_dumper *)> &func) {
    m_issues.clear();

    m_file.open(Mode::WRITE);
    func(this);
    m_file.close();

    return issues();
  }

  const std::vector<Schema_dumper::Issue> &issues() const { return m_issues; }

  const std::string &content() const { return m_file.content(); }

  template <typename... Args>
  void dump(std::vector<Schema_dumper::Issue> (Schema_dumper::*func)(
                IFile *, const std20::remove_cvref_t<Args> &...),
            Args &&... args) {
    auto issues = (m_dumper->*func)(&m_file, std::forward<Args>(args)...);

    m_issues.insert(m_issues.end(), std::make_move_iterator(issues.begin()),
                    std::make_move_iterator(issues.end()));
  }

  template <typename... Args>
  void dump(void (Schema_dumper::*func)(IFile *,
                                        const std20::remove_cvref_t<Args> &...),
            Args &&... args) {
    (m_dumper->*func)(&m_file, std::forward<Args>(args)...);
  }

 private:
  Schema_dumper *m_dumper;
  Memory_file m_file;
  std::vector<Schema_dumper::Issue> m_issues;
};

Dumper::Dumper(const Dump_options &options)
    : m_console(std::make_shared<Console_with_progress>(m_progress,
                                                        &m_progress_mutex)),
      m_options(options) {
  m_options.validate();

  if (m_options.use_single_file()) {
    using mysqlshdk::storage::make_file;

    {
      using mysqlshdk::storage::utils::get_scheme;
      using mysqlshdk::storage::utils::scheme_matches;
      using mysqlshdk::storage::utils::strip_scheme;

      const auto scheme = get_scheme(m_options.output_url());

      if (!scheme.empty() && !scheme_matches(scheme, "file")) {
        throw std::invalid_argument("File handling for " + scheme +
                                    " protocol is not supported.");
      }

      if (m_options.output_url().empty() ||
          (!scheme.empty() &&
           strip_scheme(m_options.output_url(), scheme).empty())) {
        throw std::invalid_argument(
            "The name of the output file cannot be empty.");
      }
    }

    m_output_file = make_file(m_options.output_url(), m_options.oci_options());
    m_output_dir = m_output_file->parent();

    if (!m_output_dir->exists()) {
      throw std::invalid_argument(
          "Cannot proceed with the dump, the directory containing '" +
          m_options.output_url() + "' does not exist at the target location " +
          m_output_dir->full_path() + ".");
    }
  } else {
    using mysqlshdk::storage::make_directory;
    if (m_options.oci_options().oci_par_manifest.get_safe()) {
      m_output_dir = std::make_unique<Dump_manifest>(Dump_manifest::Mode::WRITE,
                                                     m_options.oci_options(),
                                                     m_options.output_url());
    } else {
      m_output_dir =
          make_directory(m_options.output_url(), m_options.oci_options());
    }

    if (m_output_dir->exists()) {
      auto files = m_output_dir->list_files(true);

      if (!files.empty()) {
        std::vector<std::string> file_data;
        const auto full_path = m_output_dir->full_path();

        for (const auto &file : files) {
          file_data.push_back(shcore::str_format(
              "%s [size %zu]",
              m_output_dir->join_path(full_path, file.name).c_str(),
              file.size));
        }

        log_error(
            "Unable to dump to %s, the directory exists and is not empty:\n  "
            "%s",
            full_path.c_str(), shcore::str_join(file_data, "\n  ").c_str());

        if (m_options.oci_options()) {
          throw std::invalid_argument(
              "Cannot proceed with the dump, bucket '" +
              *m_options.oci_options().os_bucket_name +
              "' already contains files with the specified prefix '" +
              m_options.output_url() + "'.");
        } else {
          throw std::invalid_argument(
              "Cannot proceed with the dump, the specified directory '" +
              m_options.output_url() +
              "' already exists at the target location " + full_path +
              " and is not empty.");
        }
      }
    }
  }
}

// needs to be defined here due to Dumper::Synchronize_workers being
// incomplete type
Dumper::~Dumper() = default;

void Dumper::run() {
  try {
    do_run();
  } catch (...) {
    kill_workers();
    throw;
  }

  if (m_worker_interrupt) {
    // m_worker_interrupt is also used to signal exceptions from workers,
    // if we're here, then no exceptions were thrown and user pressed ^C
    throw std::runtime_error("Interrupted by user");
  }
}

void Dumper::do_run() {
  m_worker_interrupt = false;

  shcore::Interrupt_handler intr_handler([this]() -> bool {
    current_console()->print_warning("Interrupted by user. Canceling...");
    emergency_shutdown();
    kill_query();
    return false;
  });

  open_session();

  shcore::on_leave_scope terminate_session([this]() { close_session(); });

  create_schema_tasks();

  validate_mds();
  initialize_counters();
  initialize_progress();

  initialize_dump();

  {
    shcore::on_leave_scope read_locks([this]() { release_read_locks(); });

    acquire_read_locks();

    if (m_worker_interrupt) {
      return;
    }

    create_worker_threads();
    wait_for_workers();

    if (m_options.consistent_dump() && !m_worker_interrupt) {
      current_console()->print_info("All transactions have been started");
      lock_instance();
    }
  }

  dump_ddl();

  create_schema_ddl_tasks();
  create_table_tasks();

  if (!m_options.is_dry_run() && !m_worker_interrupt) {
    current_console()->print_status(
        "Running data dump using " + std::to_string(m_options.threads()) +
        " thread" + (m_options.threads() > 1 ? "s" : "") + ".");

    if (m_options.show_progress()) {
      current_console()->print_note(
          "Progress information uses estimated values and may not be "
          "accurate.");
    }
  }

  maybe_push_shutdown_tasks();
  wait_for_all_tasks();

  if (!m_options.is_dry_run() && !m_worker_interrupt) {
    shutdown_progress();
    write_dump_finished_metadata();
    summarize();
  }

  rethrow();
}

const std::shared_ptr<mysqlshdk::db::ISession> &Dumper::session() const {
  return m_session;
}

void Dumper::add_schema_task(Schema_task &&task) {
  m_schema_tasks.emplace_back(std::move(task));
}

std::unique_ptr<Schema_dumper> Dumper::schema_dumper(
    const std::shared_ptr<mysqlshdk::db::ISession> &session) const {
  auto dumper = std::make_unique<Schema_dumper>(session);

  dumper->opt_comments = true;
  dumper->opt_drop_database = false;
  dumper->opt_drop_table = false;
  dumper->opt_drop_view = true;
  dumper->opt_drop_event = true;
  dumper->opt_drop_routine = true;
  dumper->opt_drop_trigger = true;
  dumper->opt_reexecutable = true;
  dumper->opt_tz_utc = m_options.use_timezone_utc();
  dumper->opt_mysqlaas = m_options.mds_compatibility();
  dumper->opt_character_set_results = m_options.character_set();
  dumper->opt_column_statistics = false;

  return dumper;
}

void Dumper::on_init_thread_session(
    const std::shared_ptr<mysqlshdk::db::ISession> &session) const {
  // transaction cannot be started here, as the main thread has to acquire read
  // locks first
  session->execute("SET SQL_MODE = '';");
  session->executef("SET NAMES ?;", m_options.character_set());

  // The amount of time the server should wait for us to read data from it
  // like resultsets. Result reading can be delayed by slow uploads.
  session->executef("SET SESSION net_write_timeout = ?",
                    k_mysql_server_net_write_timeout);

  // Amount of time before server disconnects idle clients.
  session->executef("SET SESSION wait_timeout = ?",
                    k_mysql_server_wait_timeout);

  if (m_options.use_timezone_utc()) {
    session->execute("SET TIME_ZONE = '+00:00';");
  }
}

void Dumper::open_session() {
  auto co = get_classic_connection_options(m_options.session());

  // set read timeout, if not already set by user
  if (!co.has(mysqlshdk::db::kNetReadTimeout)) {
    const auto k_one_day = "86400000";
    co.set(mysqlshdk::db::kNetReadTimeout, k_one_day);
  }

  // set size of max packet (~size of 1 row) we can get from server
  if (!co.has(mysqlshdk::db::kMaxAllowedPacket)) {
    const auto k_one_gb = "1073741824";
    co.set(mysqlshdk::db::kMaxAllowedPacket, k_one_gb);
  }

  m_session = establish_session(co, false);

  on_init_thread_session(m_session);
}

void Dumper::close_session() {
  if (m_session) {
    m_session->close();
  }

  m_session = nullptr;
}

void Dumper::acquire_read_locks() const {
  if (m_options.consistent_dump()) {
    // TODO(pawel): this blocks until all statements finish execution, kill
    //              long running queries or ask user what to do?
    current_console()->print_info("Acquiring global read lock");
    try {
      session()->execute("FLUSH TABLES WITH READ LOCK;");
    } catch (const mysqlshdk::db::Error &e) {
      if (ER_SPECIFIC_ACCESS_DENIED_ERROR == e.code()) {
        current_console()->print_error(
            "Current user lacks privileges to acquire the global read lock. "
            "Please disable consistent dump using " +
            mysqlshdk::textui::bold("consistent") + " option set to false.");
      }

      throw std::runtime_error("Unable to acquire global read lock: " +
                               e.format());
    }
  }
}

void Dumper::release_read_locks() const {
  if (m_options.consistent_dump()) {
    session()->execute("UNLOCK TABLES;");

    if (!m_worker_interrupt) {
      // we've been interrupted, we still need to release locks, but don't
      // inform the user
      current_console()->print_info("Global read lock has been released");
    }
  }
}

void Dumper::start_transaction(
    const std::shared_ptr<mysqlshdk::db::ISession> &session) const {
  if (m_options.consistent_dump()) {
    session->execute(
        "SET SESSION TRANSACTION ISOLATION LEVEL REPEATABLE READ;");
    session->execute("START TRANSACTION WITH CONSISTENT SNAPSHOT;");
  }
}

void Dumper::lock_instance() const {
  if (m_options.consistent_dump()) {
    current_console()->print_info("Locking instance for backup");
    session()->execute("/*!80000 LOCK INSTANCE FOR BACKUP */;");
  }
}

void Dumper::validate_mds() const {
  if (m_options.mds_compatibility() && m_options.dump_ddl()) {
    const auto console = current_console();
    const auto version = m_options.mds_compatibility()->get_base();

    console->print_info(
        "Checking for compatibility with MySQL Database Service " + version);

    if (session()->get_server_version() < mysqlshdk::utils::Version(8, 0, 0)) {
      console->print_note(
          "MySQL Server 5.7 detected, please consider upgrading to 8.0 first. "
          "You can check for potential upgrade issues using util." +
          shcore::get_member_name("checkForServerUpgrade",
                                  shcore::current_naming_style()) +
          "().");
    }

    bool fixed = false;
    bool error = false;

    const auto issues = [&](const auto &memory) {
      for (const auto &issue : memory->issues()) {
        fixed |= issue.fixed;
        error |= !issue.fixed;

        if (issue.fixed) {
          console->print_note(issue.description);
        } else {
          console->print_error(issue.description);
        }
      }
    };

    const auto dumper = schema_dumper(session());

    if (m_options.dump_users()) {
      issues(dump_users(dumper.get()));
    }

    for (const auto &schema : m_schema_tasks) {
      issues(dump_schema(dumper.get(), schema.name));
    }

    for (const auto &schema : m_schema_tasks) {
      for (const auto &table : schema.tables) {
        issues(dump_table(dumper.get(), schema.name, table.name));

        if (m_options.dump_triggers() &&
            dumper->count_triggers_for_table(schema.name, table.name) > 0) {
          issues(dump_triggers(dumper.get(), schema.name, table.name));
        }
      }

      for (const auto &view : schema.views) {
        issues(dump_temporary_view(dumper.get(), schema.name, view.name));
        issues(dump_view(dumper.get(), schema.name, view.name));
      }
    }

    if (error) {
      console->print_info(
          "Compatibility issues with MySQL Database Service " + version +
          " were found. Please use the 'compatibility' option to apply "
          "compatibility adaptations to the dumped DDL.");
      throw std::runtime_error("Compatibility issues were found");
    } else if (fixed) {
      console->print_info("Compatibility issues with MySQL Database Service " +
                          version +
                          " were found and repaired. Please review the changes "
                          "made before loading them.");
    } else {
      console->print_info("Compatibility checks finished.");
    }
  }
}

void Dumper::initialize_counters() {
  m_total_rows = 0;
  m_total_tables = 0;
  m_total_views = 0;
  m_total_schemas = m_schema_tasks.size();

  for (auto &schema : m_schema_tasks) {
    m_total_tables += schema.tables.size();
    m_total_views += schema.views.size();

    for (auto &table : schema.tables) {
      table.row_count = get_row_count(schema, table);
      m_total_rows += table.row_count;
    }
  }
}

void Dumper::initialize_dump() {
  if (m_options.is_dry_run()) {
    return;
  }

  create_output_directory();
  set_basenames();
  write_metadata();
}

void Dumper::create_output_directory() {
  const auto dir = directory();

  if (!dir->exists()) {
    dir->create();
  }
}

void Dumper::set_basenames() {
  for (auto &schema : m_schema_tasks) {
    schema.basename = get_basename(encode_schema_basename(schema.name));

    for (auto &table : schema.tables) {
      table.basename =
          get_basename(encode_table_basename(schema.name, table.name));
    }

    for (auto &view : schema.views) {
      view.basename =
          get_basename(encode_table_basename(schema.name, view.name));
    }
  }
}

void Dumper::create_worker_threads() {
  m_worker_exceptions.clear();
  m_worker_exceptions.resize(m_options.threads());
  m_worker_synchronization = std::make_unique<Synchronize_workers>();

  for (std::size_t i = 0; i < m_options.threads(); ++i) {
    auto t = mysqlsh::spawn_scoped_thread(
        &Table_worker::run,
        Table_worker{i, this, Table_worker::Exception_strategy::ABORT});
    m_workers.emplace_back(std::move(t));
  }
}

void Dumper::wait_for_workers() {
  m_worker_synchronization->wait_for(m_workers.size());
}

void Dumper::maybe_push_shutdown_tasks() {
  if (0 == m_chunking_tasks &&
      m_main_thread_finished_producing_chunking_tasks) {
    m_worker_tasks.shutdown(m_workers.size());
  }
}

void Dumper::chunking_task_finished() {
  --m_chunking_tasks;
  maybe_push_shutdown_tasks();
}

void Dumper::wait_for_all_tasks() {
  for (auto &worker : m_workers) {
    worker.join();
  }

  // when using a single file as an output, it's not closed until the whole
  // dump is done
  if (m_options.use_single_file()) {
    for (const auto &writer : m_worker_writers) {
      close_file(*writer);
    }
  }

  m_workers.clear();
  m_worker_writers.clear();
}

void Dumper::dump_ddl() const {
  if (!m_options.dump_ddl()) {
    return;
  }

  dump_global_ddl();
  dump_users_ddl();
}

void Dumper::dump_global_ddl() const {
  current_console()->print_status("Writing global DDL files");

  if (m_options.is_dry_run()) {
    return;
  }

  const auto dumper = schema_dumper(session());

  {
    // file with the DDL setup
    const auto output = make_file("@.sql");
    output->open(Mode::WRITE);

    dumper->write_comment(output.get());

    output->close();
  }

  {
    // post DDL file (cleanup)
    const auto output = make_file("@.post.sql");
    output->open(Mode::WRITE);

    dumper->write_comment(output.get());

    output->close();
  }
}

void Dumper::dump_users_ddl() const {
  if (!m_options.dump_users()) {
    return;
  }

  current_console()->print_status("Writing users DDL");

  const auto dumper = schema_dumper(session());

  write_ddl(*dump_users(dumper.get()), "@.users.sql");
}

void Dumper::write_ddl(const Memory_dumper &in_memory,
                       const std::string &file) const {
  if (!m_options.mds_compatibility()) {
    // if MDS is on, changes done by compatibility options were printed earlier
    // MDS is off, so no errors here
    const auto console = current_console();

    for (const auto &issue : in_memory.issues()) {
      console->print_note(issue.description);
    }
  }

  if (m_options.is_dry_run()) {
    return;
  }

  const auto output = make_file(file);
  output->open(Mode::WRITE);

  const auto &content = in_memory.content();
  output->write(content.c_str(), content.length());

  output->close();
}

std::unique_ptr<Dumper::Memory_dumper> Dumper::dump_ddl(
    Schema_dumper *dumper,
    const std::function<void(Memory_dumper *)> &func) const {
  auto memory = std::make_unique<Memory_dumper>(dumper);

  memory->dump(func);

  return memory;
}

std::unique_ptr<Dumper::Memory_dumper> Dumper::dump_schema(
    Schema_dumper *dumper, const std::string &schema) const {
  return dump_ddl(dumper, [&schema, this](Memory_dumper *m) {
    m->dump(&Schema_dumper::write_comment, schema, std::string{});
    m->dump(&Schema_dumper::dump_schema_ddl, schema);

    if (m_options.dump_events()) {
      m->dump(&Schema_dumper::dump_events_ddl, schema);
    }

    if (m_options.dump_routines()) {
      m->dump(&Schema_dumper::dump_routines_ddl, schema);
    }
  });
}

std::unique_ptr<Dumper::Memory_dumper> Dumper::dump_table(
    Schema_dumper *dumper, const std::string &schema,
    const std::string &table) const {
  return dump_ddl(dumper, [&schema, &table](Memory_dumper *m) {
    m->dump(&Schema_dumper::write_comment, schema, table);
    m->dump(&Schema_dumper::dump_table_ddl, schema, table);
  });
}

std::unique_ptr<Dumper::Memory_dumper> Dumper::dump_triggers(
    Schema_dumper *dumper, const std::string &schema,
    const std::string &table) const {
  return dump_ddl(dumper, [&schema, &table](Memory_dumper *m) {
    m->dump(&Schema_dumper::write_comment, schema, table);
    m->dump(&Schema_dumper::dump_triggers_for_table_ddl, schema, table);
  });
}

std::unique_ptr<Dumper::Memory_dumper> Dumper::dump_temporary_view(
    Schema_dumper *dumper, const std::string &schema,
    const std::string &view) const {
  return dump_ddl(dumper, [&schema, &view](Memory_dumper *m) {
    m->dump(&Schema_dumper::write_comment, schema, view);
    m->dump(&Schema_dumper::dump_temporary_view_ddl, schema, view);
  });
}

std::unique_ptr<Dumper::Memory_dumper> Dumper::dump_view(
    Schema_dumper *dumper, const std::string &schema,
    const std::string &view) const {
  return dump_ddl(dumper, [&schema, &view](Memory_dumper *m) {
    m->dump(&Schema_dumper::write_comment, schema, view);
    m->dump(&Schema_dumper::dump_view_ddl, schema, view);
  });
}

std::unique_ptr<Dumper::Memory_dumper> Dumper::dump_users(
    Schema_dumper *dumper) const {
  return dump_ddl(dumper, [this](Memory_dumper *m) {
    m->dump(&Schema_dumper::write_comment, std::string{}, std::string{});
    m->dump(&Schema_dumper::dump_grants, m_options.included_users(),
            m_options.excluded_users());
  });
}

void Dumper::create_schema_ddl_tasks() {
  if (!m_options.dump_ddl()) {
    return;
  }

  for (const auto &schema : m_schema_tasks) {
    if (m_options.dump_schema_ddl()) {
      m_worker_tasks.push(
          [&schema](Table_worker *worker) { worker->dump_schema_ddl(schema); },
          shcore::Queue_priority::HIGH);
    }

    for (const auto &view : schema.views) {
      m_worker_tasks.push(
          [&schema, &view](Table_worker *worker) {
            worker->dump_view_ddl(schema, view);
          },
          shcore::Queue_priority::HIGH);
    }

    for (auto &table : schema.tables) {
      m_worker_tasks.push(
          [&schema, &table](Table_worker *worker) {
            worker->dump_table_ddl(schema, table);
          },
          shcore::Queue_priority::HIGH);
    }
  }
}

void Dumper::create_table_tasks() {
  m_chunking_tasks = 0;

  m_main_thread_finished_producing_chunking_tasks = false;

  for (const auto &schema : m_schema_tasks) {
    for (const auto &table : schema.tables) {
      auto task = create_table_task(schema, table);

      if (!m_options.is_dry_run() && should_dump_data(task)) {
        m_worker_tasks.push(
            [task](Table_worker *worker) {
              worker->write_table_metadata(task);
            },
            shcore::Queue_priority::HIGH);
      }

      if (m_options.dump_data()) {
        push_table_task(std::move(task));
      }
    }
  }

  m_main_thread_finished_producing_chunking_tasks = true;
}

Dumper::Table_task Dumper::create_table_task(const Schema_task &schema,
                                             const Table_info &table) {
  Table_task task;
  task.name = table.name;
  task.basename = table.basename;
  task.row_count = table.row_count;
  task.schema = schema.name;
  task.index = choose_index(schema, table);
  task.columns = get_columns(schema, table);

  on_create_table_task(task);

  return task;
}

void Dumper::push_table_task(Table_task &&task) {
  const auto quoted_name = quote(task);

  if (!should_dump_data(task)) {
    current_console()->print_warning("Skipping data dump for table " +
                                     quoted_name);
    return;
  }

  current_console()->print_status("Preparing data dump for table " +
                                  quoted_name);

  const auto &index = task.index;

  if (m_options.split()) {
    if (index.name.empty()) {
      current_console()->print_warning(
          "Could not select a column to be used as an index for table " +
          quoted_name +
          ". Chunking has been disabled for this table, data will be dumped to "
          "a single file.");
    } else {
      current_console()->print_status("Data dump for table " + quoted_name +
                                      " will be chunked using column " +
                                      shcore::quote_identifier(index.name));
    }
  } else {
    current_console()->print_status(
        "Data dump for table " + quoted_name +
        (index.name.empty()
             ? " will not use an index"
             : " will use column " + shcore::quote_identifier(index.name) +
                   " as an index"));
  }

  if (m_options.is_dry_run()) {
    return;
  }

  ++m_chunking_tasks;

  m_worker_tasks.push(
      [task = std::move(task)](Table_worker *worker) {
        ++worker->m_dumper->m_num_threads_chunking;

        worker->create_table_data_tasks(task);

        --worker->m_dumper->m_num_threads_chunking;
      },
      shcore::Queue_priority::MEDIUM);
}

Dumper::Index_info Dumper::choose_index(const Schema_task &schema,
                                        const Table_info &table) const {
  Index_info index;

  // check if there's a primary key or a unique index, use first column in index
  const auto result = session()->queryf(
      "SELECT INDEX_NAME, COLUMN_NAME FROM information_schema.statistics WHERE "
      "NON_UNIQUE = 0 AND SEQ_IN_INDEX = 1 AND TABLE_SCHEMA = ? AND TABLE_NAME "
      "= ?",
      schema.name, table.name);

  while (const auto row = result->fetch_one()) {
    if ("PRIMARY" == row->get_string(0)) {
      index.name = row->get_string(1);
      index.primary = true;
      break;
    } else if (index.name.empty()) {
      index.name = row->get_string(1);
    }
  }

  return index;
}

Dump_writer *Dumper::get_table_data_writer(const std::string &filename) {
  // TODO(pawel): in the future, it's going to be possible to dump into a single
  //              SQL file: use a different type of writer, return the same
  //              pointer each time
  std::lock_guard<std::mutex> lock(m_worker_writers_mutex);

  // create new writer if we're writing to multiple files, or to a single file
  // and writer hasn't been created yet
  if (!m_options.use_single_file() || m_worker_writers.empty()) {
    // if we're writing to a single file, simply use the provided name
    auto file = m_options.use_single_file()
                    ? std::move(m_output_file)
                    : make_file(filename + k_dump_in_progress_ext);
    auto compressed_file =
        mysqlshdk::storage::make_file(std::move(file), m_options.compression());
    std::unique_ptr<Dump_writer> writer;

    if (import_table::Dialect::default_() == m_options.dialect()) {
      writer =
          std::make_unique<Default_dump_writer>(std::move(compressed_file));
    } else if (import_table::Dialect::json() == m_options.dialect()) {
      writer = std::make_unique<Json_dump_writer>(std::move(compressed_file));
    } else if (import_table::Dialect::csv() == m_options.dialect()) {
      writer = std::make_unique<Csv_dump_writer>(std::move(compressed_file));
    } else if (import_table::Dialect::tsv() == m_options.dialect()) {
      writer = std::make_unique<Tsv_dump_writer>(std::move(compressed_file));
    } else if (import_table::Dialect::csv_unix() == m_options.dialect()) {
      writer =
          std::make_unique<Csv_unix_dump_writer>(std::move(compressed_file));
    } else {
      writer = std::make_unique<Text_dump_writer>(std::move(compressed_file),
                                                  m_options.dialect());
    }

    m_worker_writers.emplace_back(std::move(writer));
  }

  return m_worker_writers.back().get();
}

void Dumper::finish_writing(Dump_writer *writer) {
  // close the file if we're writing to multiple files, otherwise the single
  // file is going to be closed when all tasks are finished
  if (!m_options.use_single_file()) {
    close_file(*writer);

    {
      std::lock_guard<std::mutex> lock(m_worker_writers_mutex);

      m_worker_writers.erase(
          std::remove_if(m_worker_writers.begin(), m_worker_writers.end(),
                         [writer](const auto &w) { return w.get() == writer; }),
          m_worker_writers.end());
    }
  }
}

void Dumper::close_file(const Dump_writer &writer) const {
  const auto output = writer.output();

  if (output->is_open()) {
    output->close();
  }

  const auto filename = output->filename();
  const auto trimmed = trim_in_progress_extension(filename);

  if (trimmed != filename) {
    output->rename(trimmed);
  }
}

void Dumper::write_metadata() const {
  if (m_options.is_export_only()) {
    return;
  }

  write_dump_started_metadata();

  for (const auto &schema : m_schema_tasks) {
    write_schema_metadata(schema);
  }
}

void Dumper::write_dump_started_metadata() const {
  if (m_options.is_export_only()) {
    return;
  }

  using rapidjson::Document;
  using rapidjson::StringRef;
  using rapidjson::Type;
  using rapidjson::Value;

  Document doc{Type::kObjectType};
  auto &a = doc.GetAllocator();

  const auto mysqlsh = std::string("mysqlsh ") + shcore::get_long_version();
  doc.AddMember(StringRef("dumper"), ref(mysqlsh), a);
  doc.AddMember(StringRef("version"), StringRef(Schema_dumper::version()), a);

  {
    // list of schemas
    Value schemas{Type::kArrayType};

    for (const auto &schema : m_schema_tasks) {
      schemas.PushBack(ref(schema.name), a);
    }

    doc.AddMember(StringRef("schemas"), std::move(schemas), a);
  }

  {
    // map of basenames
    Value basenames{Type::kObjectType};

    for (const auto &schema : m_schema_tasks) {
      basenames.AddMember(ref(schema.name), ref(schema.basename), a);
    }

    doc.AddMember(StringRef("basenames"), std::move(basenames), a);
  }

  if (m_options.dump_users()) {
    const auto dumper = schema_dumper(session());

    // list of users
    Value users{Type::kArrayType};

    for (const auto &user : dumper->get_users(m_options.included_users(),
                                              m_options.excluded_users())) {
      users.PushBack({user.second.c_str(), a}, a);
    }

    doc.AddMember(StringRef("users"), std::move(users), a);
  }

  doc.AddMember(StringRef("defaultCharacterSet"),
                ref(m_options.character_set()), a);
  doc.AddMember(StringRef("tzUtc"), m_options.use_timezone_utc(), a);
  doc.AddMember(StringRef("tableOnly"), m_options.table_only(), a);

  doc.AddMember(StringRef("user"), ref(m_dump_info->user()), a);
  doc.AddMember(StringRef("hostname"), ref(m_dump_info->hostname()), a);
  doc.AddMember(StringRef("server"), ref(m_dump_info->server()), a);
  doc.AddMember(StringRef("serverVersion"), ref(m_dump_info->server_version()),
                a);
  doc.AddMember(StringRef("gtidExecuted"), ref(m_dump_info->gtid_executed()),
                a);
  doc.AddMember(StringRef("consistent"), m_options.consistent_dump(), a);

  if (m_options.mds_compatibility()) {
    bool compat = m_options.mds_compatibility();
    doc.AddMember(StringRef("mdsCompatibility"), compat, a);
  }

  doc.AddMember(StringRef("begin"), ref(m_dump_info->begin()), a);

  write_json(make_file("@.json"), &doc);
}

void Dumper::write_dump_finished_metadata() const {
  if (m_options.is_export_only()) {
    return;
  }

  using rapidjson::Document;
  using rapidjson::StringRef;
  using rapidjson::Type;
  using rapidjson::Value;

  Document doc{Type::kObjectType};
  auto &a = doc.GetAllocator();

  doc.AddMember(StringRef("end"), ref(m_dump_info->end()), a);
  doc.AddMember(StringRef("dataBytes"), m_data_bytes.load(), a);

  {
    Value schemas{Type::kObjectType};

    for (const auto &schema : m_table_data_bytes) {
      Value tables{Type::kObjectType};

      for (const auto &table : schema.second) {
        tables.AddMember(ref(table.first), table.second, a);
      }

      schemas.AddMember(ref(schema.first), std::move(tables), a);
    }

    doc.AddMember(StringRef("tableDataBytes"), std::move(schemas), a);
  }

  write_json(make_file("@.done.json"), &doc);
}

void Dumper::write_schema_metadata(const Schema_task &schema) const {
  if (m_options.is_export_only()) {
    return;
  }

  using rapidjson::Document;
  using rapidjson::StringRef;
  using rapidjson::Type;
  using rapidjson::Value;

  Document doc{Type::kObjectType};
  auto &a = doc.GetAllocator();

  doc.AddMember(StringRef("schema"), ref(schema.name), a);
  doc.AddMember(StringRef("includesDdl"), m_options.dump_schema_ddl(), a);
  doc.AddMember(StringRef("includesViewsDdl"), m_options.dump_ddl(), a);
  doc.AddMember(StringRef("includesData"), m_options.dump_data(), a);

  {
    // list of tables
    Value tables{Type::kArrayType};

    for (const auto &table : schema.tables) {
      tables.PushBack(ref(table.name), a);
    }

    doc.AddMember(StringRef("tables"), std::move(tables), a);
  }

  if (m_options.dump_ddl()) {
    // list of views
    Value views{Type::kArrayType};

    for (const auto &view : schema.views) {
      views.PushBack(ref(view.name), a);
    }

    doc.AddMember(StringRef("views"), std::move(views), a);
  }

  if (m_options.dump_schema_ddl()) {
    const auto dumper = schema_dumper(session());

    if (m_options.dump_events()) {
      // list of events
      Value events{Type::kArrayType};

      for (const auto &event : dumper->get_events(schema.name)) {
        events.PushBack({event.c_str(), a}, a);
      }

      doc.AddMember(StringRef("events"), std::move(events), a);
    }

    if (m_options.dump_routines()) {
      // list of functions
      Value functions{Type::kArrayType};

      for (const auto &function :
           dumper->get_routines(schema.name, "FUNCTION")) {
        functions.PushBack({function.c_str(), a}, a);
      }

      doc.AddMember(StringRef("functions"), std::move(functions), a);
    }

    if (m_options.dump_routines()) {
      // list of stored procedures
      Value procedures{Type::kArrayType};

      for (const auto &procedure :
           dumper->get_routines(schema.name, "PROCEDURE")) {
        procedures.PushBack({procedure.c_str(), a}, a);
      }

      doc.AddMember(StringRef("procedures"), std::move(procedures), a);
    }
  }

  {
    // map of basenames
    Value basenames{Type::kObjectType};

    for (const auto &table : schema.tables) {
      basenames.AddMember(ref(table.name), ref(table.basename), a);
    }

    for (const auto &view : schema.views) {
      basenames.AddMember(ref(view.name), ref(view.basename), a);
    }

    doc.AddMember(StringRef("basenames"), std::move(basenames), a);
  }

  write_json(make_file(get_schema_filename(schema.basename, "json")), &doc);
}

void Dumper::write_table_metadata(
    const Table_task &table,
    const std::shared_ptr<mysqlshdk::db::ISession> &session) const {
  if (m_options.is_export_only()) {
    return;
  }

  using rapidjson::Document;
  using rapidjson::StringRef;
  using rapidjson::Type;
  using rapidjson::Value;

  Document doc{Type::kObjectType};
  auto &a = doc.GetAllocator();

  {
    // options - to be used by importer
    Value options{Type::kObjectType};

    options.AddMember(StringRef("schema"), ref(table.schema), a);
    options.AddMember(StringRef("table"), ref(table.name), a);

    Value cols{Type::kArrayType};
    Value decode{Type::kObjectType};

    for (const auto &c : table.columns) {
      cols.PushBack(ref(c.name), a);

      if (c.csv_unsafe) {
        decode.AddMember(
            ref(c.name),
            StringRef(m_options.use_base64() ? "FROM_BASE64" : "UNHEX"), a);
      }
    }

    options.AddMember(StringRef("columns"), std::move(cols), a);

    if (!decode.ObjectEmpty()) {
      options.AddMember(StringRef("decodeColumns"), std::move(decode), a);
    }

    options.AddMember(
        StringRef("primaryIndex"),
        table.index.primary ? ref(table.index.name) : StringRef(""), a);

    options.AddMember(
        StringRef("compression"),
        {mysqlshdk::storage::to_string(m_options.compression()).c_str(), a}, a);

    options.AddMember(StringRef("defaultCharacterSet"),
                      ref(m_options.character_set()), a);

    options.AddMember(StringRef("fieldsTerminatedBy"),
                      ref(m_options.dialect().fields_terminated_by), a);
    options.AddMember(StringRef("fieldsEnclosedBy"),
                      ref(m_options.dialect().fields_enclosed_by), a);
    options.AddMember(StringRef("fieldsOptionallyEnclosed"),
                      m_options.dialect().fields_optionally_enclosed, a);
    options.AddMember(StringRef("fieldsEscapedBy"),
                      ref(m_options.dialect().fields_escaped_by), a);
    options.AddMember(StringRef("linesTerminatedBy"),
                      ref(m_options.dialect().lines_terminated_by), a);

    doc.AddMember(StringRef("options"), std::move(options), a);
  }

  const auto dumper = schema_dumper(session);

  if (m_options.dump_triggers() && m_options.dump_ddl()) {
    // list of triggers
    Value triggers{Type::kArrayType};

    for (const auto &trigger : dumper->get_triggers(table.schema, table.name)) {
      triggers.PushBack({trigger.c_str(), a}, a);
    }

    doc.AddMember(StringRef("triggers"), std::move(triggers), a);
  }

  const auto all_histograms = dumper->get_histograms(table.schema, table.name);

  if (!all_histograms.empty()) {
    // list of histograms
    Value histograms{Type::kArrayType};

    for (const auto &histogram : all_histograms) {
      Value h{Type::kObjectType};

      h.AddMember(StringRef("column"), ref(histogram.column), a);
      h.AddMember(StringRef("buckets"),
                  static_cast<uint64_t>(histogram.buckets), a);

      histograms.PushBack(std::move(h), a);
    }

    doc.AddMember(StringRef("histograms"), std::move(histograms), a);
  }

  doc.AddMember(StringRef("includesData"), m_options.dump_data(), a);
  doc.AddMember(StringRef("includesDdl"), m_options.dump_ddl(), a);

  doc.AddMember(StringRef("extension"), {get_table_data_ext().c_str(), a}, a);
  doc.AddMember(StringRef("chunking"), is_chunked(table), a);

  write_json(make_file(dump::get_table_data_filename(table.basename, "json")),
             &doc);
}

void Dumper::summarize() const {
  const auto console = current_console();

  console->print_status("Duration: " + m_dump_info->duration());

  if (!m_options.is_export_only()) {
    console->print_status("Schemas dumped: " + std::to_string(m_total_schemas));
    console->print_status("Tables dumped: " + std::to_string(m_total_tables));
  }

  console->print_status(
      std::string{compressed() ? "Uncompressed d" : "D"} +
      "ata size: " + mysqlshdk::utils::format_bytes(m_data_bytes));

  if (compressed()) {
    console->print_status("Compressed data size: " +
                          mysqlshdk::utils::format_bytes(m_bytes_written));
    console->print_status(shcore::str_format(
        "Compression ratio: %.1f",
        m_data_bytes / std::max(static_cast<double>(m_bytes_written), 1.0)));
  }

  console->print_status("Rows written: " + std::to_string(m_rows_written));
  console->print_status("Bytes written: " +
                        mysqlshdk::utils::format_bytes(m_bytes_written));
  console->print_status(std::string{"Average "} +
                        (compressed() ? "uncompressed " : "") + "throughput: " +
                        mysqlshdk::utils::format_throughput_bytes(
                            m_data_bytes, m_dump_info->seconds()));

  if (compressed()) {
    console->print_status("Average compressed throughput: " +
                          mysqlshdk::utils::format_throughput_bytes(
                              m_bytes_written, m_dump_info->seconds()));
  }

  summary();
}

void Dumper::rethrow() const {
  for (const auto &exc : m_worker_exceptions) {
    if (exc) {
      throw std::runtime_error("Fatal error during dump");
    }
  }
}

void Dumper::emergency_shutdown() {
  m_worker_interrupt = true;

  const auto workers = m_workers.size();

  if (workers > 0) {
    m_worker_tasks.shutdown(workers);
  }
}

void Dumper::kill_workers() {
  emergency_shutdown();
  wait_for_all_tasks();
}

std::string Dumper::get_table_data_filename(const std::string &basename) const {
  return dump::get_table_data_filename(basename, get_table_data_ext());
}

std::string Dumper::get_table_data_filename(const std::string &basename,
                                            const std::size_t idx,
                                            const bool last_chunk) const {
  return dump::get_table_data_filename(basename, get_table_data_ext(), idx,
                                       last_chunk);
}

std::string Dumper::get_table_data_ext() const {
  using import_table::Dialect;

  const auto dialect = m_options.dialect();
  auto extension = "txt";

  if (dialect == Dialect::default_() || dialect == Dialect::tsv()) {
    extension = "tsv";
  } else if (dialect == Dialect::csv() || dialect == Dialect::csv_unix()) {
    extension = "csv";
  } else if (dialect == Dialect::json()) {
    extension = "json";
  }

  return extension + mysqlshdk::storage::get_extension(m_options.compression());
}

void Dumper::initialize_progress() {
  m_rows_written = 0;
  m_bytes_written = 0;
  m_data_bytes = 0;
  m_table_data_bytes.clear();

  m_data_throughput = std::make_unique<mysqlshdk::textui::Throughput>();
  m_bytes_throughput = std::make_unique<mysqlshdk::textui::Throughput>();

  m_num_threads_chunking = 0;
  m_num_threads_dumping = 0;

  m_use_json = "off" != mysqlsh::current_shell_options()->get().wrap_json;

  if (m_options.show_progress()) {
    if (m_use_json) {
      m_progress = std::make_unique<mysqlshdk::textui::Json_progress>(
          "rows", "rows", "row", "rows");
    } else {
      m_progress = std::make_unique<mysqlshdk::textui::Text_progress>(
          "rows", "rows", "row", "rows", true, true);
    }
  } else {
    m_progress = std::make_unique<mysqlshdk::textui::IProgress>();
  }

  m_progress->total(m_total_rows);
  m_dump_info = std::make_unique<Dump_info>(session());
}

void Dumper::update_progress(uint64_t new_rows,
                             const Dump_write_result &new_bytes) {
  m_rows_written += new_rows;
  m_bytes_written += new_bytes.bytes_written();
  m_data_bytes += new_bytes.data_bytes();

  {
    std::lock_guard<std::mutex> lock(m_table_data_bytes_mutex);
    m_table_data_bytes[new_bytes.schema()][new_bytes.table()] +=
        new_bytes.data_bytes();
  }

  {
    std::unique_lock<std::mutex> lock(m_progress_mutex, std::try_to_lock);
    if (lock.owns_lock()) {
      m_data_throughput->push(m_data_bytes);
      m_bytes_throughput->push(m_bytes_written);
      m_progress->current(m_rows_written);

      if (!m_options.is_export_only()) {
        const uint64_t chunking = m_num_threads_chunking;
        const uint64_t dumping = m_num_threads_dumping;

        if (0 == chunking) {
          m_progress->set_left_label(std::to_string(dumping) +
                                     " thds dumping - ");
        } else {
          m_progress->set_left_label(std::to_string(chunking) +
                                     " thds chunking, " +
                                     std::to_string(dumping) + " dumping - ");
        }
      }

      m_progress->show_status(false, ", " + throughput());
    }
  }
}

void Dumper::shutdown_progress() {
  if (m_dump_info) {
    m_dump_info->finish();
  }

  m_progress->current(m_rows_written);
  m_progress->show_status(true, ", " + throughput());
  m_progress->shutdown();
}

std::string Dumper::throughput() const {
  return mysqlshdk::utils::format_throughput_bytes(m_data_throughput->rate(),
                                                   1.0) +
         (compressed() ? " uncompressed, " +
                             mysqlshdk::utils::format_throughput_bytes(
                                 m_bytes_throughput->rate(), 1.0) +
                             " compressed"
                       : "");
}

std::string Dumper::quote(const Schema_task &schema) {
  return shcore::quote_identifier(schema.name);
}

std::string Dumper::quote(const Schema_task &schema, const Table_info &table) {
  return quote(schema, table.name);
}

std::string Dumper::quote(const Schema_task &schema, const std::string &view) {
  return quote(schema.name, view);
}

std::string Dumper::quote(const Table_task &table) {
  return quote(table.schema, table.name);
}

std::string Dumper::quote(const std::string &schema, const std::string &table) {
  return shcore::quote_identifier(schema) + "." +
         shcore::quote_identifier(table);
}

mysqlshdk::storage::IDirectory *Dumper::directory() const {
  return m_output_dir.get();
}

std::unique_ptr<mysqlshdk::storage::IFile> Dumper::make_file(
    const std::string &filename) const {
  return directory()->file(filename);
}

uint64_t Dumper::get_row_count(const Schema_task &schema,
                               const Table_info &table) const {
  // this is only an estimate, COUNT(*) could be too slow
  const auto result = session()->queryf(
      "SELECT TABLE_ROWS FROM information_schema.tables WHERE "
      "TABLE_SCHEMA = ? AND TABLE_NAME = ?",
      schema.name, table.name);
  if (const auto row = result->fetch_one()) {
    return row->get_uint(0);
  } else {
    return UINT64_C(0);
  }
}

std::string Dumper::get_basename(const std::string &basename) {
  // 255 characters total:
  // - 225 - base name
  // -   5 - base name ordinal number
  // -   2 - '@@'
  // -   5 - chunk ordinal number
  // -  10 - extension
  // -   8 - '.dumping' extension
  static const std::size_t max_length = 225;
  const auto wbasename = shcore::utf8_to_wide(basename);
  const auto wtruncated = shcore::truncate(wbasename, max_length);

  if (wbasename.length() != wtruncated.length()) {
    const auto truncated = shcore::wide_to_utf8(wtruncated);
    const auto ordinal = m_truncated_basenames[truncated]++;
    return truncated + std::to_string(ordinal);
  } else {
    return basename;
  }
}

bool Dumper::compressed() const {
  return mysqlshdk::storage::Compression::NONE != m_options.compression();
}

void Dumper::kill_query() const {
  const auto &s = session();

  if (s) {
    try {
      // establish_session() cannot be used here, as it's going to create
      // interrupt handler of its own
      const auto &co = s->get_connection_options();
      std::shared_ptr<mysqlshdk::db::ISession> kill_session;

      switch (co.get_session_type()) {
        case mysqlsh::SessionType::X:
          kill_session = mysqlshdk::db::mysqlx::Session::create();
          break;

        case mysqlsh::SessionType::Classic:
          kill_session = mysqlshdk::db::mysql::Session::create();
          break;

        default:
          throw std::runtime_error("Unsupported session type.");
      }

      kill_session->connect(co);
      kill_session->executef("KILL QUERY ?", s->get_connection_id());
      kill_session->close();
    } catch (const std::exception &e) {
      log_warning("Error canceling SQL query: %s", e.what());
    }
  }
}

std::string Dumper::get_query_comment(const std::string &schema,
                                      const std::string &table,
                                      const std::string &id,
                                      const char *context) const {
  return "/* mysqlsh " +
         shcore::get_member_name(name(), shcore::current_naming_style()) +
         ", " + context + " table " + quote(schema, table) +
         ", chunk ID: " + id + " */";
}

std::string Dumper::get_query_comment(const Table_data_task &task,
                                      const char *context) const {
  return get_query_comment(task.schema, task.name, task.id, context);
}

bool Dumper::exists(const std::string &schema) const {
  const auto result = session()->queryf(
      "SELECT SCHEMA_NAME FROM information_schema.schemata WHERE SCHEMA_NAME=?",
      schema);
  return nullptr != result->fetch_one();
}

bool Dumper::exists(const std::string &schema, const std::string &table) const {
  const auto result = session()->queryf(
      "SELECT TABLE_NAME FROM information_schema.tables WHERE TABLE_SCHEMA = ? "
      "AND TABLE_NAME = ?;",
      schema, table);
  return nullptr != result->fetch_one();
}

bool Dumper::is_chunked(const Table_task &task) const {
  return m_options.split() && !task.index.name.empty();
}

std::vector<Dumper::Column_info> Dumper::get_columns(
    const Schema_task &schema, const Table_info &table) const {
  const auto result = session()->queryf(
      "SELECT COLUMN_NAME, DATA_TYPE FROM information_schema.columns WHERE "
      "EXTRA <> 'VIRTUAL GENERATED' AND EXTRA <> 'STORED GENERATED' AND "
      "TABLE_SCHEMA = ? AND TABLE_NAME = ? ORDER BY ORDINAL_POSITION",
      schema.name, table.name);

  std::vector<Column_info> columns;

  while (auto row = result->fetch_one()) {
    Column_info column;
    column.name = row->get_string(0);
    const auto type = row->get_string(1);
    column.csv_unsafe = shcore::str_iendswith(type, "binary") ||
                        shcore::str_iendswith(type, "bit") ||
                        shcore::str_iendswith(type, "blob") ||
                        shcore::str_iendswith(type, "geometry") ||
                        shcore::str_iendswith(type, "geometrycollection") ||
                        shcore::str_iendswith(type, "linestring") ||
                        shcore::str_iendswith(type, "point") ||
                        shcore::str_iendswith(type, "polygon");
    columns.emplace_back(std::move(column));
  }

  return columns;
}

bool Dumper::should_dump_data(const Table_task &table) {
  if (table.schema == "mysql" &&
      (table.name == "apply_status" || table.name == "general_log" ||
       table.name == "schema" || table.name == "slow_log")) {
    return false;
  } else {
    return true;
  }
}

}  // namespace dump
}  // namespace mysqlsh
