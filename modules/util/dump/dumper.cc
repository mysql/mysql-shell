/*
 * Copyright (c) 2020, 2021, Oracle and/or its affiliates.
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
#include <type_traits>
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
#include "mysqlshdk/libs/textui/progress.h"
#include "mysqlshdk/libs/textui/textui.h"
#include "mysqlshdk/libs/utils/debug.h"
#include "mysqlshdk/libs/utils/fault_injection.h"
#include "mysqlshdk/libs/utils/profiling.h"
#include "mysqlshdk/libs/utils/rate_limit.h"
#include "mysqlshdk/libs/utils/std.h"
#include "mysqlshdk/libs/utils/strformat.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_net.h"
#include "mysqlshdk/libs/utils/utils_sqlstring.h"
#include "mysqlshdk/libs/utils/utils_string.h"

#include "modules/mod_utils.h"
#include "modules/util/dump/compatibility_option.h"
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
using mysqlshdk::utils::Version;

using Row = std::vector<std::string>;

namespace {

static constexpr auto k_dump_in_progress_ext = ".dumping";

static constexpr const int k_mysql_server_net_write_timeout = 30 * 60;
static constexpr const int k_mysql_server_wait_timeout = 365 * 24 * 60 * 60;

FI_DEFINE(dumper, [](const mysqlshdk::utils::FI::Args &args) {
  throw std::runtime_error(args.get_string("msg"));
});

enum class Issue_status {
  NONE,
  FIXED,
  FIXED_CREATE_PKS,
  FIXED_IGNORE_PKS,
  ERROR,
  PK_ERROR,
};

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

auto refs(const std::string &s) {
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

Issue_status show_issues(const std::vector<Schema_dumper::Issue> &issues) {
  const auto console = current_console();
  Issue_status status = Issue_status::NONE;
  const auto set_status = [&status](Issue_status s) {
    status = std::max(status, s);
  };

  for (const auto &issue : issues) {
    if (issue.status <= Schema_dumper::Issue::Status::FIXED) {
      if (issue.status ==
          Schema_dumper::Issue::Status::FIXED_BY_CREATE_INVISIBLE_PKS) {
        set_status(Issue_status::FIXED_CREATE_PKS);
      } else if (issue.status ==
                 Schema_dumper::Issue::Status::FIXED_BY_IGNORE_MISSING_PKS) {
        set_status(Issue_status::FIXED_IGNORE_PKS);
      } else {
        set_status(Issue_status::FIXED);
      }

      console->print_note(issue.description);
    } else {
      set_status(Issue_status::ERROR);

      std::string hint;

      if (Schema_dumper::Issue::Status::FIX_MANUALLY == issue.status) {
        hint = "this issue needs to be fixed manually";
      } else if (Schema_dumper::Issue::Status::USE_CREATE_OR_IGNORE_PKS ==
                 issue.status) {
        set_status(Issue_status::PK_ERROR);
      } else {
        hint = "fix this with '" +
               to_string(to_compatibility_option(issue.status)) +
               "' compatibility option";
      }

      console->print_error(issue.description +
                           (hint.empty() ? "" : " (" + hint + ")"));
    }
  }

  return status;
}

std::string quote(const std::string &schema, const std::string &table) {
  return shcore::quote_identifier(schema) + "." +
         shcore::quote_identifier(table);
}

Row fetch_row(const mysqlshdk::db::IRow *row) {
  Row result;

  if (row) {
    for (uint32_t i = 0, size = row->num_fields(); i < size; ++i) {
      assert(!row->is_null(i));
      result.emplace_back(row->get_as_string(i));
    }
  }

  return result;
}

int64_t to_int64_t(const std::string &s) { return std::stoll(s); }

uint64_t to_uint64_t(const std::string &s) { return std::stoull(s); }

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
    std::string context;

    try {
#ifndef NDEBUG
      try {
        FI_TRIGGER_TRAP(dumper, mysqlshdk::utils::FI::Trigger_options(
                                    {{"op", "WORKER_START"},
                                     {"id", std::to_string(m_id)}}));
      } catch (const std::exception &e) {
        if (shcore::str_beginswith(e.what(), "sleep=")) {
          shcore::sleep_ms(shcore::lexical_cast<uint32_t>(e.what() + 6));
        } else {
          throw;
        }
      }
#endif
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
        auto work = m_dumper->m_worker_tasks.pop();

        if (m_dumper->m_worker_interrupt) {
          return;
        }

        if (!work.task) {
          break;
        }

        context = std::move(work.info);

        work.task(this);

        if (m_dumper->m_worker_interrupt) {
          return;
        }
      }

      m_dumper->assert_transaction_is_open(m_session);
    } catch (const mysqlshdk::db::Error &e) {
      handle_exception(context, e.format().c_str());
    } catch (const std::exception &e) {
      handle_exception(context, e.what());
    } catch (...) {
      handle_exception(context, "Unknown exception");
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

  std::string prepare_query(
      const Table_data_task &table,
      std::vector<Dump_writer::Encoding_type> *out_pre_encoded_columns) const {
    const auto base64 = m_dumper->m_options.use_base64();
    std::string query = "SELECT SQL_NO_CACHE ";

    for (const auto &column : table.info->columns) {
      if (column->csv_unsafe) {
        query += (base64 ? "TO_BASE64(" : "HEX(") + column->quoted_name + ")";

        out_pre_encoded_columns->push_back(
            base64 ? Dump_writer::Encoding_type::BASE64
                   : Dump_writer::Encoding_type::HEX);
      } else {
        query += column->quoted_name;

        out_pre_encoded_columns->push_back(Dump_writer::Encoding_type::NONE);
      }

      query += ",";
    }

    // remove last comma
    query.pop_back();

    query += " FROM " + table.quoted_name;

    if (table.partition) {
      query += " PARTITION (" + table.partition->quoted_name + ")";
    }

    query += where(table.where);

    if (table.info->index.valid()) {
      query += " ORDER BY " + table.info->index.columns_sql();
    }

    query += " " + m_dumper->get_query_comment(table, "dumping");

    return query;
  }

  void dump_table_data(const Table_data_task &table) {
    Dump_write_result bytes_written_per_file;
    Dump_write_result bytes_written_per_update{table.schema, table.name};
    uint64_t rows_written_per_update = 0;
    uint64_t rows_written_per_file = 0;
    uint64_t max_row_size = 0;
    const uint64_t update_every = 2000;
    uint64_t bytes_written_per_idx = 0;
    const uint64_t write_idx_every = 1024 * 1024;  // bytes
    Dump_write_result bytes_written;
    mysqlshdk::utils::Profile_timer timer;
    std::vector<Dump_writer::Encoding_type> pre_encoded_columns;

    log_debug("Dumping %s (chunk %s) using condition: %s",
              table.task_name.c_str(), table.id.c_str(), table.where.c_str());

    timer.stage_begin("dumping");

    const auto result =
        m_session->query(prepare_query(table, &pre_encoded_columns));

    shcore::on_leave_scope close_index_file([&table]() {
      try {
        if (table.index_file) {
          if (table.index_file->is_open()) {
            table.index_file->close();
          }
        }
      } catch (const std::runtime_error &error) {
        log_error("%s", error.what());
      }
    });

    table.writer->open();
    if (table.index_file) {
      table.index_file->open(Mode::WRITE);
    }
    bytes_written = table.writer->write_preamble(result->get_metadata(),
                                                 pre_encoded_columns);
    bytes_written_per_file += bytes_written;
    bytes_written_per_update += bytes_written;

    while (const auto row = result->fetch_one()) {
      if (m_dumper->m_worker_interrupt) {
        return;
      }

      bytes_written = table.writer->write_row(row);
      bytes_written_per_file += bytes_written;
      bytes_written_per_update += bytes_written;
      bytes_written_per_idx += bytes_written.data_bytes();
      ++rows_written_per_update;
      ++rows_written_per_file;

      if (table.index_file && bytes_written_per_idx >= write_idx_every) {
        // the idx file contains offsets to the data stream, not to binary one
        const auto offset = mysqlshdk::utils::host_to_network(
            bytes_written_per_file.data_bytes());
        table.index_file->write(&offset, sizeof(uint64_t));

        // make sure offsets are written when close to the write_idx_every
        bytes_written_per_idx %= write_idx_every;
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

      if (bytes_written.data_bytes() > max_row_size) {
        max_row_size = bytes_written.data_bytes();
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
      table.index_file->close();
    }

    log_info("Dump of %s into '%s' (chunk %s) took %f seconds, written %" PRIu64
             " rows (%" PRIu64 " bytes), longest row has %" PRIu64 " bytes",
             table.task_name.c_str(),
             table.writer->output()->full_path().c_str(), table.id.c_str(),
             timer.total_seconds_elapsed(), rows_written_per_file,
             bytes_written_per_file.data_bytes(), max_row_size);

    const auto file_size = m_dumper->finish_writing(
        table.writer, bytes_written_per_file.data_bytes());
    m_dumper->update_progress(rows_written_per_update,
                              bytes_written_per_update);

    // if file is compressed, the final file size may differ from the bytes
    // written so far, as i.e. bytes were not flushed until the whole block was
    // ready
    if (file_size > bytes_written_per_file.bytes_written()) {
      m_dumper->update_bytes_written(file_size -
                                     bytes_written_per_file.bytes_written());
    }
  }

  void push_table_data_task(Table_data_task &&task) {
    std::string info = "dumping " + task.task_name;
    // Table_data_task contains an unique_ptr, it's moved into shared_ptr and
    // then move-captured by lambda, so that this lambda can be stored,
    // otherwise compiler will complain about a call to implicitly-deleted
    // copy constructor
    auto t = std::make_shared<Table_data_task>(std::move(task));

    m_dumper->m_worker_tasks.push({std::move(info),
                                   [task = std::move(t)](Table_worker *worker) {
                                     ++worker->m_dumper->m_num_threads_dumping;

                                     worker->dump_table_data(*task);

                                     --worker->m_dumper->m_num_threads_dumping;
                                   }},
                                  shcore::Queue_priority::LOW);
  }

  Table_data_task create_table_data_task(const Table_task &table,
                                         const std::string &filename) {
    Table_data_task data_task;

    data_task.task_name = table.task_name;
    data_task.name = table.name;
    data_task.quoted_name = table.quoted_name;
    data_task.schema = table.schema;
    data_task.info = table.info;
    data_task.partition = table.partition;
    data_task.writer = m_dumper->get_table_data_writer(filename);

    if (!m_dumper->m_options.is_export_only()) {
      data_task.index_file = m_dumper->make_file(filename + ".idx");
    }

    return data_task;
  }

  void create_and_push_table_data_task(const Table_task &table) {
    Table_data_task data_task = create_table_data_task(
        table, m_dumper->get_table_data_filename(table.basename));

    data_task.id = "1";

    push_table_data_task(std::move(data_task));
  }

  void create_and_push_table_data_task(const Table_task &table,
                                       const std::string &where,
                                       const std::string &id, std::size_t idx,
                                       bool last_chunk) {
    Table_data_task data_task = create_table_data_task(
        table,
        m_dumper->get_table_data_filename(table.basename, idx, last_chunk));

    data_task.id = id;

    if (!where.empty()) {
      data_task.where = "(" + where + ")";
    }

    if (0 == idx) {
      for (const auto &c : table.info->index.columns()) {
        if (c->nullable) {
          data_task.where += "OR(" + c->quoted_name + " IS NULL)";
        }
      }
    }

    push_table_data_task(std::move(data_task));
  }

  void write_table_metadata(const Table_task &table) {
    m_dumper->write_table_metadata(table, m_session);
  }

  void create_table_data_tasks(const Table_task &table) {
    auto ranges = create_ranged_tasks(table);

    if (0 == ranges) {
      create_and_push_table_data_task(table);
      ++ranges;
    }

    current_console()->print_status(
        "Data dump for table " + table.task_name + " will be written to " +
        std::to_string(ranges) + " file" + (ranges > 1 ? "s" : ""));

    m_dumper->chunking_task_finished();
  }

  template <typename T, std::enable_if_t<std::is_integral<T>::value, int> = 0>
  static std::string between(const std::string &column, T begin, T end) {
    return column + " BETWEEN " + std::to_string(begin) + " AND " +
           std::to_string(end);
  }

  struct Chunking_info {
    const Table_task *table;
    uint64_t row_count;
    uint64_t rows_per_chunk;
    uint64_t accuracy;
    int explain_rows_idx;
    std::string partition;
    std::string where;
    std::string order_by;
    std::size_t index_column;
  };

  static std::string compare(const Chunking_info &info, const Row &value,
                             const std::string &op, bool eq) {
    const auto &columns = info.table->info->index.columns();
    const auto size = columns.size() - info.index_column;

    assert(size == value.size());

    auto column = columns.rbegin();
    const auto end = column + size;
    std::size_t idx = size;

    const auto get_value = [&column, &value, &idx]() {
      return quote_value(value[--idx], (*column)->type);
    };

    std::string result = (*column)->quoted_name + op;

    if (eq) {
      result += "=";
    }

    result += get_value();

    while (++column != end) {
      const auto val = get_value();
      result = (*column)->quoted_name + op + val + " OR(" +
               (*column)->quoted_name + "=" + val + " AND(" + result + "))";
    }

    return "(" + result + ")";
  }

  static std::string compare_ge(const Chunking_info &info, const Row &value) {
    return compare(info, value, ">", true);
  }

  static std::string compare_le(const Chunking_info &info, const Row &value) {
    return compare(info, value, "<", true);
  }

  static std::string ge(const Chunking_info &info, const Row &value) {
    std::string result = info.where;

    if (!result.empty()) {
      result += " AND";
    }

    result += compare_ge(info, value);

    return result;
  }

  static std::string between(const Chunking_info &info, const Row &begin,
                             const Row &end) {
    return ge(info, begin) + "AND" + compare_le(info, end);
  }

  template <typename T, std::enable_if_t<std::is_integral<T>::value, int> = 0>
  static std::string between(const Chunking_info &info, T begin, T end) {
    std::string result = info.where;

    if (!result.empty()) {
      result += " AND ";
    }

    return result +
           between(info.table->info->index.columns()[info.index_column]
                       ->quoted_name,
                   begin, end);
  }

  static std::string where(const std::string &condition) {
    std::string result;

    if (!condition.empty()) {
      result = " WHERE " + condition;
    }

    return result;
  }

  template <typename T, std::enable_if_t<std::is_integral<T>::value, int> = 0>
  static T constant_step(const T /* from */, const T step) {
    return step;
  }

  template <typename T, std::enable_if_t<std::is_integral<T>::value, int> = 0>
  T adaptive_step(const T from, const T step, const T max,
                  const Chunking_info &info, const std::string &chunk_id) {
    static constexpr int k_chunker_retries = 10;
    static constexpr int k_chunker_iterations = 20;

    const auto double_step = 2 * step;
    auto middle = from;

    auto rows = info.rows_per_chunk;
    const auto comment = this->get_query_comment(*info.table, chunk_id);

    int retry = 0;
    uint64_t delta = info.accuracy + 1;

    const auto row_count = [&info, &comment, this](const auto begin,
                                                   const auto end) {
      return m_session
          ->query("EXPLAIN SELECT COUNT(*) FROM " + info.table->quoted_name +
                  info.partition + where(between(info, begin, end)) +
                  info.order_by + comment)
          ->fetch_one_or_throw()
          ->get_uint(info.explain_rows_idx);
    };

    while (delta > info.accuracy && retry < k_chunker_retries) {
      if (max - retry * double_step <= from) {
        // if left boundary is greater than max, stop here
        middle = max;
        break;
      }

      // each time search in a different range, we didn't find the answer in the
      // previous one
      auto left = from + retry * double_step;
      // if right boundary is greater than max, use max instead
      auto right = std::numeric_limits<T>::max() - double_step <= left
                       ? std::numeric_limits<T>::max()
                       : left + double_step;

      assert(left < right);

      for (int i = 0; i < k_chunker_iterations; ++i) {
        middle = left + (right - left) / 2;

        if (middle >= right || middle <= left) {
          break;
        }

        rows = row_count(from, middle);

        if (0 == i && rows < info.rows_per_chunk) {
          // if in the first iteration there's not enough rows, check the whole
          // range, if there's still not enough rows we can skip this range
          const auto total_rows = row_count(from, right);

          if (total_rows < info.rows_per_chunk) {
            middle = right;
            delta = info.rows_per_chunk - total_rows;
            break;
          }
        }

        if (rows > info.rows_per_chunk) {
          right = middle;
          delta = rows - info.rows_per_chunk;
        } else {
          left = middle;
          delta = info.rows_per_chunk - rows;
        }

        if (delta <= info.accuracy) {
          // we're close enough
          break;
        }
      }

      if (delta > info.accuracy) {
        if (rows >= info.rows_per_chunk) {
          // we have too many rows, but that's OK...
          retry = k_chunker_retries;
        } else {
          if (middle >= max) {
            // we've reached the upper boundary, stop here
            retry = k_chunker_retries;
          } else {
            // we didn't find enough rows here, move farther to
            // the right
            ++retry;
          }
        }
      }
    }

    return middle - from;
  }

  template <typename T, std::enable_if_t<std::is_integral<T>::value, int> = 0>
  std::size_t chunk_integer_column(const Chunking_info &info, const T min,
                                   const T max) {
    static constexpr int k_chunker_step_accuracy = 1;

    std::size_t ranges_count = 0;

    // if rows_per_chunk <= 1 it may mean that the rows are bigger than chunk
    // size, which means we # chunks ~= # rows
    const auto estimated_chunks =
        info.rows_per_chunk > 0
            ? std::max(info.row_count / info.rows_per_chunk, UINT64_C(1))
            : info.row_count;
    const auto index_range = static_cast<uint64_t>(max - min);
    const auto row_count_accuracy = std::max(info.row_count / 10, UINT64_C(10));
    // it should be (max - min + 1), but this can potentially overflow and `+ 1`
    // is not significant, as the result is divided anyway
    const auto estimated_step =
        std::max(index_range / estimated_chunks, UINT64_C(2));

    std::string chunk_id;
    using step_t = decltype(min);

    // use constant step if number of chunks is small or index range is close to
    // the number of rows
    const bool use_constant_step =
        estimated_chunks < 2 || std::max(index_range, info.row_count) -
                                        std::min(index_range, info.row_count) <=
                                    row_count_accuracy;

    const auto next_step =
        use_constant_step
            ? std::function<step_t(const step_t, const step_t)>(
                  constant_step<T>)
            // using the default capture [&] below results in problems with
            // GCC 5.4.0 (https://gcc.gnu.org/bugzilla/show_bug.cgi?id=80543)
            : [&info, &max, &chunk_id, this](const auto from, const auto step) {
                return this->adaptive_step(from, step, max, info, chunk_id);
              };

    auto current = min;
    // if estimated_step is greater than maximum possible value, casting will
    // overflow, use max instead
    const auto step = estimated_step < static_cast<decltype(estimated_step)>(
                                           std::numeric_limits<step_t>::max())
                          ? static_cast<step_t>(estimated_step)
                          : std::numeric_limits<step_t>::max();

    log_info("Chunking %s using integer algorithm with %s step",
             info.table->task_name.c_str(),
             use_constant_step ? "constant" : "adaptive");

    while (true) {
      if (m_dumper->m_worker_interrupt) {
        return ranges_count;
      }

      chunk_id = std::to_string(ranges_count);
      const auto begin = current;

      const auto new_step =
          std::max(next_step(current, step), static_cast<step_t>(2));

      // ensure that there's no integer overflow
      current = (current > max - new_step + 1 ? max : current + new_step - 1);

      // step is always > 0, if max is > 0 => max - step will not overflow
      // current is <= max, if max is < 0 => max - current will not overflow
      if (max > 0 ? max - k_chunker_step_accuracy <= current
                  : max - current <= k_chunker_step_accuracy) {
        current = max;
      }

      const auto end = current;

      const auto last_chunk = (current >= max);

      create_and_push_table_data_task(*info.table, between(info, begin, end),
                                      chunk_id, ranges_count++, last_chunk);

      if (last_chunk) {
        // exit here in case ++current overflows
        break;
      }

      ++current;
    }

    return ranges_count;
  }

  std::size_t chunk_integer_column(const Chunking_info &info, const Row &begin,
                                   const Row &end) {
    log_info("Chunking %s using integer algorithm",
             info.table->task_name.c_str());

    const auto type =
        info.table->info->index.columns()[info.index_column]->type;

    if (mysqlshdk::db::Type::Integer == type) {
      return chunk_integer_column(info, to_int64_t(begin[info.index_column]),
                                  to_int64_t(end[info.index_column]));
    } else if (mysqlshdk::db::Type::UInteger == type) {
      return chunk_integer_column(info, to_uint64_t(begin[info.index_column]),
                                  to_uint64_t(end[info.index_column]));
    }

    throw std::logic_error(
        "Unsupported column type for the integer algorithm: " +
        mysqlshdk::db::to_string(type));
  }

  std::size_t chunk_non_integer_column(const Chunking_info &info,
                                       const Row &begin, const Row &end) {
    log_info("Chunking %s using non-integer algorithm",
             info.table->task_name.c_str());

    std::size_t ranges_count = 0;

    Row range_begin = begin;
    Row range_end;

    const auto &columns = info.table->info->index.columns();
    const auto index =
        shcore::str_join(columns.begin() + info.index_column, columns.end(),
                         ",", [](const auto &c) { return c->quoted_name; });

    const auto select = "SELECT SQL_NO_CACHE " + index + " FROM " +
                        info.table->quoted_name + info.partition + " ";
    const auto order_by_and_limit = info.order_by + " LIMIT " +
                                    std::to_string(info.rows_per_chunk - 1) +
                                    ",2 ";

    const auto fetch =
        [&end](const std::shared_ptr<mysqlshdk::db::IResult> &res) {
          const auto row = res->fetch_one();
          return row ? fetch_row(row) : end;
        };

    std::shared_ptr<mysqlshdk::db::IResult> result;

    do {
      const auto condition = where(ge(info, range_begin));
      const auto chunk_id = std::to_string(ranges_count);
      const auto comment = get_query_comment(*info.table, chunk_id);

      result =
          m_session->query(select + condition + order_by_and_limit + comment);

      if (m_dumper->m_worker_interrupt) {
        return 0;
      }

      range_end = fetch(result);

      create_and_push_table_data_task(
          *info.table, between(info, range_begin, range_end), chunk_id,
          ranges_count++, range_end == end);

      range_begin = fetch(result);
    } while (range_end != end);

    return ranges_count;
  }

  std::size_t chunk_column(const Chunking_info &info) {
    const auto query =
        "SELECT SQL_NO_CACHE " + info.table->info->index.columns_sql() +
        " FROM " + info.table->quoted_name + info.partition + where(info.where);

    auto result = m_session->query(query + info.order_by + " LIMIT 1");
    auto row = result->fetch_one();

    const auto handle_empty_table = [&info, this]() {
      create_and_push_table_data_task(*info.table, info.where, "0", 0, true);
      return 1;
    };

    if (!row) {
      return handle_empty_table();
    }

    const Row begin = fetch_row(row);

    const auto desc_order_by =
        shcore::str_join(shcore::str_split(info.order_by, ","), ",",
                         [](const auto &s) { return s + " DESC"; });

    result = m_session->query(query + desc_order_by + " LIMIT 1");
    row = result->fetch_one();

    if (!row) {
      return handle_empty_table();
    }

    const Row end = fetch_row(row);

    const auto type =
        info.table->info->index.columns()[info.index_column]->type;

    log_info("Chunking %s, using columns: %s, min: [%s], max: [%s]",
             info.table->task_name.c_str(),
             shcore::str_join(info.table->info->index.columns(), ", ",
                              [](const auto &c) {
                                return c->quoted_name + " (" +
                                       mysqlshdk::db::to_string(c->type) + ")";
                              })
                 .c_str(),
             shcore::str_join(begin, ", ").c_str(),
             shcore::str_join(end, ", ").c_str());

    if (mysqlshdk::db::Type::Integer == type ||
        mysqlshdk::db::Type::UInteger == type) {
      return chunk_integer_column(info, begin, end);
    } else {
      return chunk_non_integer_column(info, begin, end);
    }
  }

  std::size_t create_ranged_tasks(const Table_task &table) {
    if (!m_dumper->is_chunked(table)) {
      return 0;
    }

    mysqlshdk::utils::Profile_timer timer;
    timer.stage_begin("chunking");

    const auto &task_name = table.task_name;

    // default row size to use when there's no known row size
    constexpr const uint64_t k_default_row_size = 256;

    auto average_row_length = table.partition
                                  ? table.partition->average_row_length
                                  : table.info->average_row_length;

    if (0 == average_row_length) {
      average_row_length = k_default_row_size;

      auto msg = "Table statistics not available for " + task_name +
                 ", chunking operation may be not optimal. "
                 "Please consider running 'ANALYZE TABLE " +
                 table.quoted_name + ";'";

      if (table.partition) {
        msg += " or 'ALTER TABLE " + table.quoted_name + " ANALYZE PARTITION " +
               table.partition->quoted_name + ";'";
      }

      msg += " first.";

      current_console()->print_note(msg);
    }

    Chunking_info info;

    info.table = &table;
    info.row_count =
        table.partition ? table.partition->row_count : table.info->row_count;
    info.rows_per_chunk =
        m_dumper->m_options.bytes_per_chunk() / average_row_length;
    info.accuracy = std::max(info.rows_per_chunk / 10, UINT64_C(10));
    info.explain_rows_idx = m_dumper->m_cache.server_is_5_6 ? 8 : 9;

    info.partition = table.partition
                         ? " PARTITION (" + table.partition->quoted_name + ")"
                         : "";

    for (const auto &c : table.info->index.columns()) {
      if (c->nullable) {
        if (!info.where.empty()) {
          info.where += "AND";
        }

        info.where += "(" + c->quoted_name + " IS NOT NULL)";
      }
    }

    info.order_by = " ORDER BY " + table.info->index.columns_sql();
    info.index_column = 0;

    log_info("Chunking %s, rows: %" PRIu64 ", average row length: %" PRIu64
             ", rows per chunk: %" PRIu64,
             task_name.c_str(), info.row_count, average_row_length,
             info.rows_per_chunk);

    const auto ranges_count = chunk_column(info);

    timer.stage_end();
    log_debug("Chunking of %s took %f seconds", task_name.c_str(),
              timer.total_seconds_elapsed());

    return ranges_count;
  }

  void handle_exception(const std::string &context, const char *msg) {
    m_dumper->m_worker_exceptions[m_id] = std::current_exception();
    current_console()->print_error(
        shcore::str_format("[Worker%03zu]: ", m_id) +
        (context.empty() ? "" : "Error while " + context + ": ") + msg);

    if (Exception_strategy::ABORT == m_strategy) {
      m_dumper->emergency_shutdown();
    }
  }

  void dump_schema_ddl(const Schema_info &schema) const {
    current_console()->print_status("Writing DDL for schema " +
                                    schema.quoted_name);

    const auto dumper = m_dumper->schema_dumper(m_session);

    m_dumper->write_ddl(*m_dumper->dump_schema(dumper.get(), schema.name),
                        get_schema_filename(schema.basename));
  }

  void dump_table_ddl(const Schema_info &schema,
                      const Table_info &table) const {
    current_console()->print_status("Writing DDL for table " +
                                    table.quoted_name);

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

  void dump_view_ddl(const Schema_info &schema, const View_info &view) const {
    current_console()->print_status("Writing DDL for view " + view.quoted_name);

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
    return m_dumper->get_query_comment(table.task_name, id, "chunking");
  }

  const std::size_t m_id;
  Dumper *m_dumper;
  Exception_strategy m_strategy;
  mysqlshdk::utils::Rate_limit m_rate_limit;
  std::shared_ptr<mysqlshdk::db::ISession> m_session;
};

class Dumper::Dump_info final {
 public:
  Dump_info() {
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

  const std::string &begin() const { return m_begin; }

  const std::string &end() const { return m_end; }

  const std::string &duration() const { return m_duration; }

  double seconds() const { return m_timer.total_seconds_elapsed(); }

 private:
  mysqlshdk::utils::Profile_timer m_timer;
  std::string m_begin;
  std::string m_end;
  std::string m_duration;
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
      m_output_dir = std::make_unique<Dump_manifest>(
          Manifest_mode::WRITE, m_options.oci_options(), nullptr,
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
  if (m_options.is_dry_run()) {
    current_console()->print_info(
        "dryRun enabled, no locks will be acquired and no files will be "
        "created.");
  }

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

  fetch_user_privileges();

  {
    shcore::on_leave_scope read_locks([this]() { release_read_locks(); });

    acquire_read_locks();

    if (m_worker_interrupt) {
      return;
    }

    create_worker_threads();

    // initialize cache while threads are starting up
    initialize_instance_cache();

    wait_for_workers();

    if (m_options.consistent_dump() && !m_worker_interrupt) {
      current_console()->print_info("All transactions have been started");
      lock_instance();
    }

    if (!m_worker_interrupt && !m_options.is_export_only() &&
        is_gtid_executed_inconsistent()) {
      current_console()->print_warning(
          "The dumped value of gtid_executed is not guaranteed to be "
          "consistent");
    }
  }

  create_schema_tasks();

  validate_privileges();
  validate_mds();
  initialize_counters();
  initialize_progress();

  initialize_dump();

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

#ifndef NDEBUG
  const auto server_version = Version(m_cache.server_version);

  if (server_version < Version(8, 0, 21) ||
      server_version > Version(8, 0, 23) || !m_options.dump_users()) {
    // SHOW CREATE USER auto-commits transaction in some 8.0 versions, we don't
    // check if transaction is still open in such case if users were dumped
    assert_transaction_is_open(session());
  }
#endif  // !NDEBUG
}

const std::shared_ptr<mysqlshdk::db::ISession> &Dumper::session() const {
  return m_session;
}

std::unique_ptr<Schema_dumper> Dumper::schema_dumper(
    const std::shared_ptr<mysqlshdk::db::ISession> &session) const {
  auto dumper = std::make_unique<Schema_dumper>(session);

  dumper->use_cache(&m_cache);

  dumper->opt_comments = true;
  dumper->opt_drop_database = false;
  dumper->opt_drop_table = false;
  dumper->opt_drop_view = true;
  dumper->opt_drop_event = true;
  dumper->opt_drop_routine = true;
  dumper->opt_drop_trigger = true;
  dumper->opt_reexecutable = true;
  dumper->opt_tz_utc = m_options.use_timezone_utc();
  dumper->opt_mysqlaas = static_cast<bool>(m_options.mds_compatibility());
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
}

void Dumper::fetch_user_privileges() {
  using mysqlshdk::mysql::Instance;
  using mysqlshdk::mysql::User_privileges;

  const auto instance = Instance(session());
  std::string user;
  std::string host;

  instance.get_current_user(&user, &host);

  m_user_privileges = std::make_unique<User_privileges>(instance, user, host);
  m_user_account = shcore::make_account(user, host);
}

void Dumper::lock_all_tables() {
  lock_instance();

  // find out the max query size we can send
  uint64_t max_packet_size = 0;

  {
    const auto r = session()->query("select @@max_allowed_packet");
    // 1024 is the minimum allowed value
    max_packet_size = r->fetch_one_or_throw()->get_uint(0, 1024);
  }

  const auto console = current_console();
  const std::string k_lock_tables = "LOCK TABLES ";

  using mysqlshdk::mysql::User_privileges;
  const std::set<std::string> privileges = {"LOCK TABLES", "SELECT"};

  const auto validate_privileges = [this, &privileges](
                                       const std::string &schema,
                                       const std::string &table =
                                           User_privileges::k_wildcard) {
    const auto result = m_user_privileges->validate(privileges, schema, table);

    if (result.has_missing_privileges()) {
      const std::string object =
          User_privileges::k_wildcard == table
              ? "schema " + shcore::quote_identifier(schema)
              : "table " + quote(schema, table);

      throw std::runtime_error(
          "User " + m_user_account +
          " is missing the following privilege(s) for " + object + ": " +
          shcore::str_join(result.missing_privileges(), ", ") + ".");
    }
  };

  // lock relevant tables in mysql so that grants, views, routines etc. are
  // consistent too, use the main thread to lock these tables, as it is going to
  // read from them
  try {
    std::string stmt = k_lock_tables;
    const std::string mysql = "mysql";

    // if user doesn't have the SELECT privilege on mysql.*, it's not going to
    // be possible to list tables
    validate_privileges(mysql);

    const auto result = session()->query(
        "SHOW TABLES IN mysql WHERE Tables_in_mysql IN"
        "('columns_priv', 'db', 'default_roles', 'func', 'global_grants', "
        "'proc', 'procs_priv', 'proxies_priv', 'role_edges', 'tables_priv', "
        "'user')");

    while (auto row = result->fetch_one()) {
      const auto table = row->get_string(0);

      validate_privileges(mysql, table);

      stmt.append("mysql." + shcore::quote_identifier(table) + " READ,");
    }

    // remove last comma
    stmt.pop_back();

    log_info("Locking tables: %s", stmt.c_str());

    if (!m_options.is_dry_run()) {
      session()->execute(stmt);
    }
  } catch (const mysqlshdk::db::Error &e) {
    console->print_error("Could not lock mysql system tables: " + e.format());
    throw;
  } catch (const std::runtime_error &e) {
    console->print_warning("Could not lock mysql system tables: " +
                           std::string{e.what()});
    console->print_warning(
        "The dump will continue, but the dump may not be completely consistent "
        "if changes to accounts or routines are made during it.");
  }

  initialize_instance_cache_minimal();

  // get tables to be locked
  std::vector<std::string> tables;

  for (const auto &schema : m_cache.schemas) {
    for (const auto &table : schema.second.tables) {
      validate_privileges(schema.first, table.first);

      tables.emplace_back(quote(schema.first, table.first) + " READ,");
    }
  }

  // iterate all tables that are going to be dumped and LOCK TABLES them, use
  // separate sessions to run the queries as executing LOCK TABLES implicitly
  // unlocks tables locked before
  try {
    std::string stmt = k_lock_tables;

    const auto lock_tables = [this, &stmt]() {
      // remove last comma
      stmt.pop_back();

      log_info("Locking tables: %s", stmt.c_str());

      if (!m_options.is_dry_run()) {
        // initialize session
        auto s = establish_session(session()->get_connection_options(), false);
        on_init_thread_session(s);
        s->execute(stmt);
        m_lock_sessions.emplace_back(std::move(s));
      }
    };

    const auto k_lock_tables_size = k_lock_tables.size();
    auto size = std::string::npos;

    for (const auto &table : tables) {
      size = stmt.size();

      // check if we're overflowing the SQL packet (256B slack is probably
      // enough)
      if (size + table.size() >= max_packet_size - 256 &&
          size > k_lock_tables_size) {
        lock_tables();

        stmt = k_lock_tables;
      }

      stmt.append(table);
    }

    if (stmt.size() > k_lock_tables_size) {
      // flush the rest
      lock_tables();
    }
  } catch (const mysqlshdk::db::Error &e) {
    console->print_error("Error locking tables: " + e.format());
    throw;
  }
}

void Dumper::acquire_read_locks() {
  if (m_options.consistent_dump()) {
    // This will block until lock_wait_timeout if there are any
    // sessions with open transactions/locks.
    current_console()->print_info("Acquiring global read lock");

    // user can execute FLUSH statements if has RELOAD privilege or, starting
    // with 8.0.23, FLUSH_TABLES privilege
    if (!m_user_privileges->validate({"RELOAD"}).has_missing_privileges() ||
        (session()->get_server_version() >= Version(8, 0, 23) &&
         !m_user_privileges->validate({"FLUSH_TABLES"})
              .has_missing_privileges())) {
      if (!m_options.is_dry_run()) {
        try {
          // We do first a FLUSH TABLES. If a long update is running, the FLUSH
          // TABLES will wait but will not stall the whole mysqld, and when the
          // long update is done the FLUSH TABLES WITH READ LOCK will start and
          // succeed quickly. So, FLUSH TABLES is to lower the probability of a
          // stage where both shell and most client connections are stalled. Of
          // course, if a second long update starts between the two FLUSHes, we
          // have that bad stall.
          session()->execute("FLUSH NO_WRITE_TO_BINLOG TABLES;");
          session()->execute("FLUSH TABLES WITH READ LOCK;");
        } catch (const mysqlshdk::db::Error &e) {
          current_console()->print_error(
              "Failed to acquire global read lock: " + e.format());
          throw std::runtime_error("Unable to acquire global read lock");
        }
      }

      current_console()->print_info("Global read lock acquired");

      // FTWRL was used to lock the tables, it is safe to start a transaction,
      // as it will not release the global read lock
      start_transaction(session());
    } else {
      m_ftwrl_failed = true;

      current_console()->print_warning(
          "The current user lacks privileges to acquire a global read lock "
          "using 'FLUSH TABLES WITH READ LOCK'. Falling back to LOCK "
          "TABLES...");

      const auto handle_error = [](const std::string &msg) {
        current_console()->print_error(
            "Unable to acquire global read lock neither table read locks.");

        throw std::runtime_error("Unable to lock tables: " + msg);
      };

      // if FTWRL isn't executable by the user, try LOCK TABLES instead
      try {
        lock_all_tables();

        current_console()->print_info("Table locks acquired");
        // a transaction would release the table locks, it cannot be started
        // until tables are unlocked
      } catch (const mysqlshdk::db::Error &e) {
        handle_error(e.format());
      } catch (const std::runtime_error &e) {
        handle_error(e.what());
      }
    }
  }
}

void Dumper::release_read_locks() const {
  if (m_options.consistent_dump()) {
    if (m_ftwrl_failed) {
      // FTWRL has failed, LOCK TABLES was used instead, transaction is not yet
      // active, we start it here - existing table locks will be implicitly
      // released
      start_transaction(session());

      log_info("Releasing the table locks");

      // close the locking sessions, this will release the table locks
      for (const auto &session : m_lock_sessions) {
        session->close();
      }
    } else {
      // FTWRL has succeeded, transaction is active, UNLOCK TABLES will not
      // automatically commit the transaction
      session()->execute("UNLOCK TABLES;");
    }

    if (!m_worker_interrupt) {
      // we've been interrupted, we still need to release locks, but don't
      // inform the user
      current_console()->print_info("Global read lock has been released");
    }
  }
}

void Dumper::start_transaction(
    const std::shared_ptr<mysqlshdk::db::ISession> &session) const {
  if (m_options.consistent_dump() && !m_options.is_dry_run()) {
    session->execute(
        "SET SESSION TRANSACTION ISOLATION LEVEL REPEATABLE READ;");
    session->execute("START TRANSACTION WITH CONSISTENT SNAPSHOT;");
  }
}

void Dumper::assert_transaction_is_open(
    const std::shared_ptr<mysqlshdk::db::ISession> &session) const {
#ifdef NDEBUG
  (void)session;
#else  // !NDEBUG
  if (m_options.consistent_dump() && !m_options.is_dry_run()) {
    try {
      // if there's an active transaction, this will throw
      session->execute("SET TRANSACTION ISOLATION LEVEL REPEATABLE READ");
      // no exception -> transaction is not active
      assert(false);
    } catch (const mysqlshdk::db::Error &e) {
      // make sure correct error is reported
      assert(e.code() == ER_CANT_CHANGE_TX_CHARACTERISTICS);
    } catch (...) {
      // any other exception means that something else went wrong
      assert(false);
    }
  }
#endif
}

void Dumper::lock_instance() {
  if (m_options.consistent_dump() && !m_instance_locked) {
    const auto console = current_console();

    console->print_info("Locking instance for backup");

    // cache might not be available here, query the session for version
    const auto version = session()->get_server_version();

    if (version >= Version(8, 0, 0)) {
      if (m_user_privileges->validate({"BACKUP_ADMIN"})
              .has_missing_privileges()) {
        console->print_error("User " + m_user_account +
                             " is missing the BACKUP_ADMIN privilege and "
                             "cannot execute 'LOCK INSTANCE FOR BACKUP'.");
        throw std::runtime_error("Could not acquire the backup lock");
      }

      if (!m_options.is_dry_run()) {
        try {
          session()->execute("LOCK INSTANCE FOR BACKUP");
        } catch (const shcore::Error &e) {
          console->print_error("Could not acquire the backup lock: " +
                               e.format());

          throw;
        }
      }
    } else {
      console->print_note("Backup lock is not supported in MySQL " +
                          version.get_short() +
                          " and DDL changes will not be blocked. The dump may "
                          "fail with an error or not be completely consistent "
                          "if schema changes are made while dumping.");
    }

    m_instance_locked = true;
  }
}

void Dumper::initialize_instance_cache_minimal() {
  m_cache = Instance_cache_builder(session(), m_options.included_schemas(),
                                   m_options.included_tables(),
                                   m_options.excluded_schemas(),
                                   m_options.excluded_tables(), false)
                .build();
}

void Dumper::initialize_instance_cache() {
  const std::string status_msg = "Gathering information";
  mysqlshdk::textui::Threaded_spinny_stick spinner{status_msg, "- done"};

  if (m_options.show_progress()) {
    spinner.start();
  } else {
    current_console()->print_status(status_msg + "...");
  }

  auto builder =
      m_cache.schemas.empty()
          ? Instance_cache_builder(session(), m_options.included_schemas(),
                                   m_options.included_tables(),
                                   m_options.excluded_schemas(),
                                   m_options.excluded_tables())
          : Instance_cache_builder(session(), std::move(m_cache));

  if (m_options.dump_users()) {
    builder.users(m_options.included_users(), m_options.excluded_users());
  }

  if (m_options.dump_ddl()) {
    if (m_options.dump_events()) {
      builder.events();
    }

    if (m_options.dump_routines()) {
      builder.routines();
    }

    if (m_options.dump_triggers()) {
      builder.triggers();
    }
  }

  if (m_options.dump_binlog_info()) {
    builder.binlog_info();
  }

  m_cache = builder.build();
}

void Dumper::create_schema_tasks() {
  for (const auto &s : m_cache.schemas) {
    Schema_info schema;
    schema.name = s.first;
    schema.quoted_name = shcore::quote_identifier(schema.name);
    schema.basename = get_basename(encode_schema_basename(schema.name));

    for (const auto &t : s.second.tables) {
      Table_info table;
      table.name = t.first;
      table.quoted_name = quote(schema.name, table.name);
      table.basename =
          get_basename(encode_table_basename(schema.name, table.name));
      table.info = &t.second;

      for (const auto &p : t.second.partitions) {
        Partition_info partition;
        partition.info = &p;
        partition.basename = get_basename(
            encode_partition_basename(schema.name, table.name, p.name));

        table.partitions.emplace_back(std::move(partition));
      }

      schema.tables.emplace_back(std::move(table));
    }

    for (const auto &v : s.second.views) {
      View_info view;
      view.name = v.first;
      view.quoted_name = quote(schema.name, view.name);
      view.basename =
          get_basename(encode_table_basename(schema.name, view.name));

      schema.views.emplace_back(std::move(view));
    }

    m_schema_infos.emplace_back(std::move(schema));
  }
}

void Dumper::validate_mds() const {
  if (!m_options.mds_compatibility() ||
      (!m_options.dump_ddl() && !m_options.dump_users())) {
    return;
  }

  const auto console = current_console();
  const auto version = m_options.mds_compatibility()->get_base();

  console->print_info(
      "Checking for compatibility with MySQL Database Service " + version);

  if (!m_cache.server_is_8_0) {
    console->print_note("MySQL Server " + m_cache.server_version.get_short() +
                        " detected, please consider upgrading to 8.0 first.");

    if (m_cache.server_is_5_7) {
      console->print_note(
          "You can check for potential upgrade issues using util." +
          shcore::get_member_name("checkForServerUpgrade",
                                  shcore::current_naming_style()) +
          "().");
    }
  }

  Issue_status status = Issue_status::NONE;

  const auto issues = [&status](const auto &memory) {
    status = std::max(status, show_issues(memory->issues()));
  };

  const auto dumper = schema_dumper(session());

  if (m_options.dump_users()) {
    issues(dump_users(dumper.get()));
  }

  if (m_options.dump_ddl()) {
    for (const auto &schema : m_schema_infos) {
      issues(dump_schema(dumper.get(), schema.name));
    }

    for (const auto &schema : m_schema_infos) {
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
  }

  switch (status) {
    case Issue_status::PK_ERROR:
      console->print_info();
      console->print_error(
          "One or more tables without Primary Keys were found.");
      console->print_info(R"(
       MySQL Database Service High Availability (MDS HA) requires Primary Keys to be present in all tables.
       To continue with the dump you must do one of the following:

       * Create PRIMARY keys in all tables before dumping them.
         MySQL 8.0.23 supports the creation of invisible columns to allow creating Primary Key columns with no impact to applications. For more details, see https://dev.mysql.com/doc/refman/en/invisible-columns.html.
         This is considered a best practice for both performance and usability and will work seamlessly with MDS.

       * Add the "create_invisible_pks" to the "compatibility" option.
         The dump will proceed and loader will automatically add Primary Keys to tables that don't have them when loading into MDS.
         This will make it possible to enable HA in MDS without application impact.
         However, Inbound Replication into an MDS HA instance (at the time of the release of MySQL Shell 8.0.24) will still not be possible.

       * Add the "ignore_missing_pks" to the "compatibility" option.
         This will disable this check and the dump will be produced normally, Primary Keys will not be added automatically.
         It will not be possible to load the dump in an HA enabled MDS instance.
)");
      // fallthrough

    case Issue_status::ERROR:
      console->print_info(
          "Compatibility issues with MySQL Database Service " + version +
          " were found. Please use the 'compatibility' option to apply "
          "compatibility adaptations to the dumped DDL.");
      throw std::runtime_error("Compatibility issues were found");

    case Issue_status::FIXED:
    case Issue_status::FIXED_CREATE_PKS:
    case Issue_status::FIXED_IGNORE_PKS:
      if (Issue_status::FIXED != status) {
        console->print_info();
        console->print_note(
            "One or more tables without Primary Keys were found.");
      }

      if (Issue_status::FIXED_CREATE_PKS == status) {
        console->print_info(R"(
      Missing Primary Keys will be created automatically when this dump is loaded.
      This will make it possible to enable High Availability in MySQL Database Service instance without application impact.
      However, Inbound Replication into an MDS HA instance (at the time of the release of MySQL Shell 8.0.24) will still not be possible.
)");
      }

      if (Issue_status::FIXED_IGNORE_PKS == status) {
        console->print_info(R"(
      This issue is ignored.
      This dump cannot be loaded into an MySQL Database Service instance with High Availability.
)");
      }

      console->print_info("Compatibility issues with MySQL Database Service " +
                          version +
                          " were found and repaired. Please review the changes "
                          "made before loading them.");
      break;

    case Issue_status::NONE:
      console->print_info("Compatibility checks finished.");
      break;
  }
}

void Dumper::initialize_counters() {
  m_total_rows = 0;
  m_total_tables = 0;
  m_total_views = 0;
  m_total_schemas = m_schema_infos.size();

  for (auto &schema : m_schema_infos) {
    m_total_tables += schema.tables.size();
    m_total_views += schema.views.size();

    for (auto &table : schema.tables) {
      m_total_rows += table.info->row_count;
    }
  }
}

void Dumper::initialize_dump() {
  if (m_options.is_dry_run()) {
    return;
  }

  create_output_directory();
  write_metadata();
}

void Dumper::create_output_directory() {
  const auto dir = directory();

  if (!dir->exists()) {
    dir->create();
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
    if (show_issues(in_memory.issues()) >= Issue_status::ERROR) {
      throw std::runtime_error(
          "Could not apply some of the compatibility options");
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

  for (const auto &schema : m_schema_infos) {
    m_worker_tasks.push(
        {"writing DDL of " + schema.quoted_name,
         [&schema](Table_worker *worker) { worker->dump_schema_ddl(schema); }},
        shcore::Queue_priority::HIGH);

    for (const auto &view : schema.views) {
      m_worker_tasks.push({"writing DDL of " + view.quoted_name,
                           [&schema, &view](Table_worker *worker) {
                             worker->dump_view_ddl(schema, view);
                           }},
                          shcore::Queue_priority::HIGH);
    }

    for (const auto &table : schema.tables) {
      m_worker_tasks.push({"writing DDL of " + table.quoted_name,
                           [&schema, &table](Table_worker *worker) {
                             worker->dump_table_ddl(schema, table);
                           }},
                          shcore::Queue_priority::HIGH);
    }
  }
}

void Dumper::create_table_tasks() {
  m_chunking_tasks = 0;

  m_main_thread_finished_producing_chunking_tasks = false;

  for (const auto &schema : m_schema_infos) {
    for (const auto &table : schema.tables) {
      auto task = create_table_task(schema, table);

      if (!m_options.is_dry_run() && should_dump_data(task)) {
        m_worker_tasks.push({"writing metadata of " + table.quoted_name,
                             [task](Table_worker *worker) {
                               worker->write_table_metadata(task);
                             }},
                            shcore::Queue_priority::HIGH);
      }

      if (m_options.dump_data()) {
        push_table_task(std::move(task));
      }
    }
  }

  m_main_thread_finished_producing_chunking_tasks = true;
}

Dumper::Table_task Dumper::create_table_task(const Schema_info &schema,
                                             const Table_info &table) {
  Table_task task;
  task.task_name = table.quoted_name;
  task.name = table.name;
  task.quoted_name = table.quoted_name;
  task.schema = schema.name;
  task.basename = table.basename;
  task.info = table.info;
  task.partitions = table.partitions;

  on_create_table_task(task.schema, task.name, task.info);

  return task;
}

void Dumper::push_table_task(Table_task &&task) {
  const auto &task_name = task.task_name;

  if (!should_dump_data(task)) {
    current_console()->print_warning("Skipping data dump for table " +
                                     task_name);
    return;
  }

  current_console()->print_status("Preparing data dump for table " + task_name);

  const auto &index = task.info->index;

  const auto get_index = [&index]() {
    return std::string{"column"} + (index.columns().size() > 1 ? "s" : "") +
           " " + index.columns_sql();
  };

  if (m_options.split()) {
    if (!index.valid()) {
      current_console()->print_note(
          "Could not select columns to be used as an index for table " +
          task_name +
          ". Chunking has been disabled for this table, data will be dumped to "
          "a single file.");
    } else {
      current_console()->print_status("Data dump for table " + task_name +
                                      " will be chunked using " + get_index());
    }
  } else {
    current_console()->print_status(
        "Data dump for table " + task_name +
        (!index.valid() ? " will not use an index"
                        : " will use " + get_index() + " as an index"));
  }

  if (m_options.is_dry_run()) {
    return;
  }

  if (m_options.is_export_only() || task.partitions.empty()) {
    push_table_chunking_task(std::move(task));
  } else {
    for (const auto &partition : task.partitions) {
      auto copy = task;

      copy.task_name =
          task.task_name + " partition " + partition.info->quoted_name;
      copy.basename = partition.basename;
      copy.partition = partition.info;

      push_table_chunking_task(std::move(copy));
    }
  }
}

void Dumper::push_table_chunking_task(Table_task &&task) {
  ++m_chunking_tasks;

  std::string info = "chunking " + task.task_name;
  m_worker_tasks.push({std::move(info),
                       [task = std::move(task)](Table_worker *worker) {
                         ++worker->m_dumper->m_num_threads_chunking;

                         worker->create_table_data_tasks(task);

                         --worker->m_dumper->m_num_threads_chunking;
                       }},
                      shcore::Queue_priority::MEDIUM);
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
                    : make_file(filename + k_dump_in_progress_ext, true);
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

std::size_t Dumper::finish_writing(Dump_writer *writer, uint64_t total_bytes) {
  std::size_t file_size = 0;

  // close the file if we're writing to multiple files, otherwise the single
  // file is going to be closed when all tasks are finished
  if (!m_options.use_single_file()) {
    std::string final_filename = close_file(*writer);

    {
      std::lock_guard<std::mutex> lock(m_table_data_bytes_mutex);
      m_chunk_file_bytes[final_filename] = total_bytes;
    }

    file_size = writer->output()->file_size();

    {
      std::lock_guard<std::mutex> lock(m_worker_writers_mutex);

      m_worker_writers.erase(
          std::remove_if(m_worker_writers.begin(), m_worker_writers.end(),
                         [writer](const auto &w) { return w.get() == writer; }),
          m_worker_writers.end());
    }
  }

  return file_size;
}

std::string Dumper::close_file(const Dump_writer &writer) const {
  const auto output = writer.output();

  if (output->is_open()) {
    output->close();
  }

  const auto filename = output->filename();
  const auto trimmed = trim_in_progress_extension(filename);

  if (trimmed != filename) {
    output->rename(trimmed);
  }
  return trimmed;
}

void Dumper::write_metadata() const {
  if (m_options.is_export_only()) {
    return;
  }

  write_dump_started_metadata();

  for (const auto &schema : m_schema_infos) {
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
  doc.AddMember(StringRef("dumper"), refs(mysqlsh), a);
  doc.AddMember(StringRef("version"), StringRef(Schema_dumper::version()), a);
  doc.AddMember(StringRef("origin"), StringRef(name()), a);

  {
    // list of schemas
    Value schemas{Type::kArrayType};

    for (const auto &schema : m_schema_infos) {
      schemas.PushBack(refs(schema.name), a);
    }

    doc.AddMember(StringRef("schemas"), std::move(schemas), a);
  }

  {
    // map of basenames
    Value basenames{Type::kObjectType};

    for (const auto &schema : m_schema_infos) {
      basenames.AddMember(refs(schema.name), refs(schema.basename), a);
    }

    doc.AddMember(StringRef("basenames"), std::move(basenames), a);
  }

  if (m_options.dump_users()) {
    const auto dumper = schema_dumper(session());

    // list of users
    Value users{Type::kArrayType};

    for (const auto &user : dumper->get_users(m_options.included_users(),
                                              m_options.excluded_users())) {
      users.PushBack({shcore::make_account(user).c_str(), a}, a);
    }

    doc.AddMember(StringRef("users"), std::move(users), a);
  }

  doc.AddMember(StringRef("defaultCharacterSet"),
                refs(m_options.character_set()), a);
  doc.AddMember(StringRef("tzUtc"), m_options.use_timezone_utc(), a);
  doc.AddMember(StringRef("bytesPerChunk"), m_options.bytes_per_chunk(), a);

  doc.AddMember(StringRef("user"), refs(m_cache.user), a);
  doc.AddMember(StringRef("hostname"), refs(m_cache.hostname), a);
  doc.AddMember(StringRef("server"), refs(m_cache.server), a);
  doc.AddMember(StringRef("serverVersion"),
                {m_cache.server_version.get_full().c_str(), a}, a);

  if (m_options.dump_binlog_info()) {
    doc.AddMember(StringRef("binlogFile"), refs(m_cache.binlog_file), a);
    doc.AddMember(StringRef("binlogPosition"), m_cache.binlog_position, a);
  }

  doc.AddMember(StringRef("gtidExecuted"), refs(m_cache.gtid_executed), a);
  doc.AddMember(StringRef("gtidExecutedInconsistent"),
                is_gtid_executed_inconsistent(), a);
  doc.AddMember(StringRef("consistent"), m_options.consistent_dump(), a);

  if (m_options.mds_compatibility()) {
    bool compat = static_cast<bool>(m_options.mds_compatibility());
    doc.AddMember(StringRef("mdsCompatibility"), compat, a);
  }

  {
    // list of compatibility options
    Value compatibility{Type::kArrayType};

    for (const auto &option : m_options.compatibility_options().values()) {
      compatibility.PushBack({to_string(option).c_str(), a}, a);
    }

    doc.AddMember(StringRef("compatibilityOptions"), std::move(compatibility),
                  a);
  }

  doc.AddMember(StringRef("begin"), refs(m_dump_info->begin()), a);

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

  doc.AddMember(StringRef("end"), refs(m_dump_info->end()), a);
  doc.AddMember(StringRef("dataBytes"), m_data_bytes.load(), a);

  {
    Value schemas{Type::kObjectType};

    for (const auto &schema : m_table_data_bytes) {
      Value tables{Type::kObjectType};

      for (const auto &table : schema.second) {
        tables.AddMember(refs(table.first), table.second, a);
      }

      schemas.AddMember(refs(schema.first), std::move(tables), a);
    }

    doc.AddMember(StringRef("tableDataBytes"), std::move(schemas), a);
  }

  {
    Value files{Type::kObjectType};

    for (const auto &file : m_chunk_file_bytes) {
      Value tables{Type::kObjectType};

      files.AddMember(refs(file.first), file.second, a);
    }
    doc.AddMember(StringRef("chunkFileBytes"), std::move(files), a);
  }

  write_json(make_file("@.done.json"), &doc);
}

void Dumper::write_schema_metadata(const Schema_info &schema) const {
  if (m_options.is_export_only()) {
    return;
  }

  using rapidjson::Document;
  using rapidjson::StringRef;
  using rapidjson::Type;
  using rapidjson::Value;

  Document doc{Type::kObjectType};
  auto &a = doc.GetAllocator();

  doc.AddMember(StringRef("schema"), refs(schema.name), a);
  doc.AddMember(StringRef("includesDdl"), m_options.dump_ddl(), a);
  doc.AddMember(StringRef("includesViewsDdl"), m_options.dump_ddl(), a);
  doc.AddMember(StringRef("includesData"), m_options.dump_data(), a);

  {
    // list of tables
    Value tables{Type::kArrayType};

    for (const auto &table : schema.tables) {
      tables.PushBack(refs(table.name), a);
    }

    doc.AddMember(StringRef("tables"), std::move(tables), a);
  }

  if (m_options.dump_ddl()) {
    // list of views
    Value views{Type::kArrayType};

    for (const auto &view : schema.views) {
      views.PushBack(refs(view.name), a);
    }

    doc.AddMember(StringRef("views"), std::move(views), a);
  }

  if (m_options.dump_ddl()) {
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
      basenames.AddMember(refs(table.name), refs(table.basename), a);
    }

    for (const auto &view : schema.views) {
      basenames.AddMember(refs(view.name), refs(view.basename), a);
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

    options.AddMember(StringRef("schema"), refs(table.schema), a);
    options.AddMember(StringRef("table"), refs(table.name), a);

    Value cols{Type::kArrayType};
    Value decode{Type::kObjectType};

    for (const auto &c : table.info->columns) {
      cols.PushBack(refs(c->name), a);

      if (c->csv_unsafe) {
        decode.AddMember(
            refs(c->name),
            StringRef(m_options.use_base64() ? "FROM_BASE64" : "UNHEX"), a);
      }
    }

    options.AddMember(StringRef("columns"), std::move(cols), a);

    if (!decode.ObjectEmpty()) {
      options.AddMember(StringRef("decodeColumns"), std::move(decode), a);
    }

    options.AddMember(StringRef("defaultCharacterSet"),
                      refs(m_options.character_set()), a);

    options.AddMember(StringRef("fieldsTerminatedBy"),
                      refs(m_options.dialect().fields_terminated_by), a);
    options.AddMember(StringRef("fieldsEnclosedBy"),
                      refs(m_options.dialect().fields_enclosed_by), a);
    options.AddMember(StringRef("fieldsOptionallyEnclosed"),
                      m_options.dialect().fields_optionally_enclosed, a);
    options.AddMember(StringRef("fieldsEscapedBy"),
                      refs(m_options.dialect().fields_escaped_by), a);
    options.AddMember(StringRef("linesTerminatedBy"),
                      refs(m_options.dialect().lines_terminated_by), a);

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

      h.AddMember(StringRef("column"), refs(histogram.column), a);
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
  doc.AddMember(
      StringRef("compression"),
      {mysqlshdk::storage::to_string(m_options.compression()).c_str(), a}, a);

  {
    Value primary{Type::kArrayType};

    if (table.info->index.primary()) {
      for (const auto &column : table.info->index.columns()) {
        primary.PushBack(refs(column->name), a);
      }
    }

    doc.AddMember(StringRef("primaryIndex"), std::move(primary), a);
  }

  {
    // list of partitions
    Value partitions{Type::kArrayType};

    for (const auto &partition : table.partitions) {
      partitions.PushBack(refs(partition.info->name), a);
    }

    doc.AddMember(StringRef("partitions"), std::move(partitions), a);
  }

  {
    // map of partition basenames
    Value basenames{Type::kObjectType};

    for (const auto &partition : table.partitions) {
      basenames.AddMember(refs(partition.info->name), refs(partition.basename),
                          a);
    }

    doc.AddMember(StringRef("basenames"), std::move(basenames), a);
  }

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
  m_dump_info = std::make_unique<Dump_info>();
}

void Dumper::update_bytes_written(uint64_t new_bytes) {
  m_bytes_written += new_bytes;
}

void Dumper::update_progress(uint64_t new_rows,
                             const Dump_write_result &new_bytes) {
  m_rows_written += new_rows;
  update_bytes_written(new_bytes.bytes_written());
  m_data_bytes += new_bytes.data_bytes();

  {
    std::lock_guard<std::mutex> lock(m_table_data_bytes_mutex);
    m_table_data_bytes[new_bytes.schema()][new_bytes.table()] +=
        new_bytes.data_bytes();
  }

  {
    std::unique_lock<std::recursive_mutex> lock(m_progress_mutex,
                                                std::try_to_lock);
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

      m_progress->set_right_label(", " + throughput());
      m_progress->show_status(false);
    }
  }
}

void Dumper::shutdown_progress() {
  if (m_dump_info) {
    m_dump_info->finish();
  }

  m_progress->current(m_rows_written);
  m_progress->set_right_label(", " + throughput());
  m_progress->show_status(true);
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

mysqlshdk::storage::IDirectory *Dumper::directory() const {
  return m_output_dir.get();
}

std::unique_ptr<mysqlshdk::storage::IFile> Dumper::make_file(
    const std::string &filename, bool use_mmap) const {
  static const char *s_mmap_mode = nullptr;
  if (s_mmap_mode == nullptr) {
    s_mmap_mode = getenv("MYSQLSH_MMAP");
    if (!s_mmap_mode) s_mmap_mode = "on";
  }

  mysqlshdk::storage::File_options options;
  if (use_mmap) options["file.mmap"] = s_mmap_mode;
  return directory()->file(filename, options);
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

std::string Dumper::get_query_comment(const std::string &quoted_name,
                                      const std::string &id,
                                      const char *context) const {
  return "/* mysqlsh " +
         shcore::get_member_name(name(), shcore::current_naming_style()) +
         ", " + context + " table " +
         // sanitize schema/table names in case they contain a '*/'
         // *\/ isn't really a valid escape, but it doesn't matter because we
         // just want the lexer to not see */
         shcore::str_replace(quoted_name, "*/", "*\\/") + ", chunk ID: " + id +
         " */";
}

std::string Dumper::get_query_comment(const Table_data_task &task,
                                      const char *context) const {
  return get_query_comment(task.task_name, task.id, context);
}

bool Dumper::is_chunked(const Table_task &task) const {
  return m_options.split() && task.info->index.valid();
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

void Dumper::validate_privileges() const {
  std::set<std::string> all_required;
  std::set<std::string> global_required;
  std::set<std::string> schema_required;
  std::set<std::string> table_required;

  if (m_options.dump_events()) {
    std::string event{"EVENT"};
    all_required.emplace(event);
    schema_required.emplace(std::move(event));
  }

  if (m_options.dump_triggers()) {
    std::string trigger{"TRIGGER"};
    all_required.emplace(trigger);
    table_required.emplace(std::move(trigger));
  }

  if (m_cache.server_is_5_6 && m_options.dump_users()) {
    // on 5.6, SUPER is required to get the real hash value from SHOW GRANTS
    std::string super{"SUPER"};
    all_required.emplace(super);
    global_required.emplace(std::move(super));
  }

  if (!all_required.empty()) {
    using mysqlshdk::mysql::User_privileges_result;

    const auto get_missing = [](const User_privileges_result &result,
                                const std::set<std::string> &required) {
      std::set<std::string> missing;

      std::set_intersection(result.missing_privileges().begin(),
                            result.missing_privileges().end(), required.begin(),
                            required.end(),
                            std::inserter(missing, missing.begin()));

      return missing;
    };

    const auto global_result = m_user_privileges->validate(all_required);
    const auto global_missing = get_missing(global_result, global_required);

    if (!global_missing.empty()) {
      throw std::runtime_error(
          "User " + m_user_account +
          " is missing the following global privilege(s): " +
          shcore::str_join(global_missing, ", ") + ".");
    }

    if (global_result.has_missing_privileges()) {
      {
        // global privileges can be safely removed from the all_required set
        std::set<std::string> temporary;

        std::set_difference(all_required.begin(), all_required.end(),
                            global_required.begin(), global_required.end(),
                            std::inserter(temporary, temporary.begin()));

        all_required = std::move(temporary);
      }

      // user has all required global privileges
      // user doesn't have *.* schema/table-level privileges, check schemas
      for (const auto &schema : m_schema_infos) {
        const auto schema_result =
            m_user_privileges->validate(all_required, schema.name);
        const auto schema_missing = get_missing(schema_result, schema_required);

        if (!schema_missing.empty()) {
          throw std::runtime_error(
              "User " + m_user_account +
              " is missing the following privilege(s) for schema " +
              schema.quoted_name + ": " +
              shcore::str_join(schema_missing, ", ") + ".");
        }

        if (schema_result.has_missing_privileges()) {
          // user has all required schema-level privileges for this schema
          // user doesn't have schema.* table-level privileges, check tables
          for (const auto &table : schema.tables) {
            const auto table_result = m_user_privileges->validate(
                all_required, schema.name, table.name);

            // if at this stage there are any missing privileges, they are all
            // table-level ones
            if (table_result.has_missing_privileges()) {
              throw std::runtime_error(
                  "User " + m_user_account +
                  " is missing the following privilege(s) for table " +
                  table.quoted_name + ": " +
                  shcore::str_join(table_result.missing_privileges(), ", ") +
                  ".");
            }
          }
        }
      }
    }
  }
}

bool Dumper::is_gtid_executed_inconsistent() const {
  return !m_options.consistent_dump() || m_ftwrl_failed;
}

}  // namespace dump
}  // namespace mysqlsh
