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

#ifndef MODULES_UTIL_LOAD_DUMP_LOADER_H_
#define MODULES_UTIL_LOAD_DUMP_LOADER_H_

#include <atomic>
#include <chrono>
#include <functional>
#include <limits>
#include <list>
#include <memory>
#include <queue>
#include <regex>
#include <set>
#include <string>
#include <string_view>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "modules/util/dump/compatibility.h"
#include "modules/util/dump/progress_thread.h"

#include "modules/util/load/dump_reader.h"
#include "modules/util/load/load_dump_options.h"
#include "modules/util/load/load_progress_log.h"

#include "modules/util/import_table/import_stats.h"

#include "mysqlshdk/libs/db/mysql/session.h"
#include "mysqlshdk/libs/storage/ifile.h"
#include "mysqlshdk/libs/textui/text_progress.h"
#include "mysqlshdk/libs/utils/synchronized_queue.h"

namespace mysqlsh {

class Dump_loader {
 public:
  using Reconnect = std::function<void()>;

  Dump_loader() = delete;
  explicit Dump_loader(const Load_dump_options &options);

  Dump_loader(const Dump_loader &) = delete;
  Dump_loader(Dump_loader &&) = delete;

  Dump_loader &operator=(const Dump_loader &) = delete;
  Dump_loader &operator=(Dump_loader &&) = delete;

  ~Dump_loader();

  void interrupt();

  void hard_interrupt();

  void abort();

  void run();

  void show_metadata(bool force = false) const;

  dump::Progress_thread *progress() { return &m_progress_thread; }

 private:
  class Worker {
   public:
    class Task {
     public:
      Task(std::string_view schema, std::string_view table)
          : m_schema(schema),
            m_table(table),
            m_key(schema_object_key(schema, table)) {}
      virtual ~Task() {}

      virtual bool execute(Worker *, Dump_loader *) = 0;

      size_t id() const { return m_id; }
      const char *log_id() const { return m_log_id.c_str(); }
      const std::string &schema() const { return m_schema; }
      const std::string &table() const { return m_table; }
      const std::string &key() const { return m_key; }
      uint64_t weight() const { return m_weight; }
      void set_weight(uint64_t weight) { m_weight = weight; }
      void done() { m_weight = 0; }

     protected:
      static void handle_current_exception(Worker *worker, Dump_loader *loader,
                                           const std::string &error);

     protected:
      size_t m_id = std::numeric_limits<size_t>::max();
      std::string m_log_id;
      std::string m_schema;
      std::string m_table;
      std::string m_key;

     private:
      friend class Worker;

      void set_id(size_t id);

      uint64_t m_weight = 1;
    };

    class Schema_ddl_task : public Task {
     public:
      Schema_ddl_task(std::string_view schema, std::string &&script,
                      bool resuming)
          : Task(schema, ""),
            m_script(std::move(script)),
            m_resuming(resuming) {}

      bool execute(Worker *, Dump_loader *) override;

     private:
      std::string m_script;
      bool m_resuming;
    };

    class Table_ddl_task : public Task {
     public:
      Table_ddl_task(std::string_view schema, std::string_view table,
                     std::string &&script, bool placeholder,
                     Load_progress_log::Status status, bool exists)
          : Task(schema, table),
            m_script(std::move(script)),
            m_placeholder(placeholder),
            m_status(status),
            m_exists(exists) {}

      bool execute(Worker *, Dump_loader *) override;

      std::unique_ptr<compatibility::Deferred_statements>
      steal_deferred_statements() {
        return std::move(m_deferred_statements);
      }

      bool placeholder() const { return m_placeholder; }

     private:
      void pre_process(Dump_loader *loader);

      void load_ddl(Worker *worker, Dump_loader *loader);

      void post_process(Worker *worker, Dump_loader *loader);

      void extract_deferred_statements(Dump_loader *loader);

      void remove_duplicate_deferred_statements(Worker *worker,
                                                Dump_loader *loader);

     private:
      std::string m_script;
      bool m_placeholder = false;
      Load_progress_log::Status m_status;

      std::unique_ptr<compatibility::Deferred_statements> m_deferred_statements;

      bool m_exists = false;
    };

    class Table_data_task : public Task {
     public:
      Table_data_task(std::string_view schema, std::string_view table,
                      std::string_view partition, ssize_t chunk_index)
          : Task(schema, table),
            m_chunk_index(chunk_index),
            m_partition(partition) {
        m_key = schema_table_object_key(schema, table, partition);
      }

      ssize_t chunk_index() const noexcept { return m_chunk_index; }

      const std::string &partition() const noexcept { return m_partition; }

     protected:
      std::string query_comment() const;

     private:
      ssize_t m_chunk_index;
      std::string m_partition;
    };

    class Load_data_task : public Table_data_task {
     public:
      Load_data_task(Dump_reader::Table_chunk chunk, bool resume)
          : Table_data_task(chunk.schema, chunk.table, chunk.partition,
                            chunk.index),
            m_chunk(std::move(chunk)),
            m_resume(resume) {}

      import_table::Stats stats;

      bool execute(Worker *, Dump_loader *) override;

      const Dump_reader::Table_chunk &chunk() const { return m_chunk; }

     protected:
      bool resume() const { return m_resume; }

      std::unique_ptr<mysqlshdk::storage::IFile> extract_file() {
        return std::move(m_chunk.file);
      }

     private:
      void load(Dump_loader *, Worker *);

      virtual void on_load_start(Worker *, Dump_loader *) = 0;

      virtual void do_load(Worker *, Dump_loader *) = 0;

      virtual void on_load_end(Worker *, Dump_loader *) = 0;

      Dump_reader::Table_chunk m_chunk;
      bool m_resume = false;
    };

    class Load_chunk_task : public Load_data_task {
     public:
      Load_chunk_task(Dump_reader::Table_chunk chunk, bool resume,
                      uint64_t bytes_to_skip)
          : Load_data_task(std::move(chunk), resume),
            m_bytes_to_skip(bytes_to_skip) {}

     private:
      void on_load_start(Worker *, Dump_loader *) override;

      void do_load(Worker *, Dump_loader *) override;

      void on_load_end(Worker *, Dump_loader *) override;

      uint64_t m_bytes_to_skip = 0;
    };

    class Bulk_load_task : public Load_data_task {
     public:
      Bulk_load_task(Dump_reader::Table_chunk chunk, bool resume,
                     uint64_t weight)
          : Load_data_task(std::move(chunk), resume),
            m_target_table(this->chunk().table) {
        set_weight(weight);
      }

     private:
      void on_load_start(Worker *, Dump_loader *) override;

      void do_load(Worker *, Dump_loader *) override;

      void on_load_end(Worker *, Dump_loader *) override;

      bool partitioned() const;

      void bulk_load(Worker *, Dump_loader *);

      std::string m_target_table;
    };

    class Analyze_table_task : public Task {
     public:
      Analyze_table_task(std::string_view schema, std::string_view table,
                         const std::vector<Dump_reader::Histogram> &histograms)
          : Task(schema, table), m_histograms(histograms) {}

      bool execute(Worker *, Dump_loader *) override;

     private:
      std::vector<Dump_reader::Histogram> m_histograms;
    };

    class Index_recreation_task : public Task {
     public:
      Index_recreation_task(
          std::string_view schema, std::string_view table,
          compatibility::Deferred_statements::Index_info *indexes,
          uint64_t weight)
          : Task(schema, table), m_indexes(indexes) {
        set_weight(weight);
      }

      bool execute(Worker *, Dump_loader *) override;

      const compatibility::Deferred_statements::Index_info *indexes() const {
        return m_indexes;
      }

     private:
      compatibility::Deferred_statements::Index_info *m_indexes;
    };

    class Checksum_task : public Table_data_task {
     public:
      using Checksum_result =
          std::pair<bool, dump::common::Checksums::Checksum_result>;
      explicit Checksum_task(
          const dump::common::Checksums::Checksum_data *checksum)
          : Table_data_task(checksum->schema(), checksum->table(),
                            checksum->partition(), checksum->chunk()),
            m_checksum(checksum) {}

      bool execute(Worker *, Dump_loader *) override;

      const dump::common::Checksums::Checksum_data *checksum_info()
          const noexcept {
        return m_checksum;
      }

      const Checksum_result &result() const noexcept { return m_result; }

     private:
      const dump::common::Checksums::Checksum_data *m_checksum;
      Checksum_result m_result = {true, {}};
    };

    Worker(size_t id, Dump_loader *owner);

    size_t id() const { return m_id; }

    void schedule(std::unique_ptr<Task> task);

    Task *current_task() const { return m_task.get(); }

    void run();
    void stop();

    uint64_t connection_id() const noexcept { return m_connection_id; }

    uint64_t thread_id() const noexcept { return m_thread_id; }

    const std::shared_ptr<mysqlshdk::db::mysql::Session> &session()
        const noexcept {
      return m_session;
    }

    const Reconnect &reconnect_callback() const noexcept {
      return m_reconnect_callback;
    }

   private:
    void handle_current_exception(Dump_loader *loader,
                                  const std::string &error);

    inline void handle_current_exception(const std::string &error) {
      handle_current_exception(m_owner, error);
    }

    void do_run();

    void connect();

    uint64_t current_thread_id() const;

    size_t m_id = 0;
    Dump_loader *m_owner;

    std::shared_ptr<mysqlshdk::db::mysql::Session> m_session;
    uint64_t m_connection_id = 0;
    uint64_t m_thread_id = 0;
    Reconnect m_reconnect_callback;

    std::unique_ptr<Task> m_task;

    shcore::Synchronized_queue<bool> m_work_ready;
  };

  using Task_ptr = std::unique_ptr<Worker::Task>;

  struct Worker_event {
    enum Event {
      CONNECTED,
      READY,
      FATAL_ERROR,
      SCHEMA_DDL_START,
      SCHEMA_DDL_END,
      TABLE_DDL_START,
      TABLE_DDL_END,
      LOAD_START,
      LOAD_END,
      INDEX_START,
      INDEX_END,
      ANALYZE_START,
      ANALYZE_END,
      EXIT,
      LOAD_SUBCHUNK_START,
      LOAD_SUBCHUNK_END,
      CHECKSUM_START,
      CHECKSUM_END,
      BULK_LOAD_START,
      BULK_LOAD_PROGRESS,
      BULK_LOAD_END,
    };
    Event event;
    Worker *worker = nullptr;
    shcore::Dictionary_t details;
  };

  using Name_and_file =
      std::pair<std::string, std::unique_ptr<mysqlshdk::storage::IFile>>;

  void execute_tasks(bool testing = false);
  void execute_table_ddl_tasks();
  void execute_view_ddl_tasks();

  void wait_for_metadata();
  bool scan_for_more_data(bool wait = true);
  void wait_for_dump(std::chrono::steady_clock::time_point start_time);

  std::shared_ptr<mysqlshdk::db::mysql::Session> create_session();

  void show_summary();

  void on_dump_begin();
  void on_dump_end();

  bool handle_table_data();
  void handle_schema_post_scripts();

  bool should_fetch_table_ddl(bool placeholder) const;

  bool schedule_table_chunk(Dump_reader::Table_chunk chunk);

  bool schedule_next_task();
  size_t handle_worker_events(const std::function<bool()> &schedule_next);

  void execute_threaded(const std::function<bool()> &schedule_next);

  void check_existing_objects();
  bool report_duplicates(const std::string &what, const std::string &schema,
                         std::list<Dump_reader::Object_info *> *objects,
                         mysqlshdk::db::IResult *result);

  void open_dump();
  void open_dump(std::unique_ptr<mysqlshdk::storage::IDirectory> dumpdir);
  void setup_progress_file(bool *out_is_resuming);
  void spawn_workers();
  void join_workers();

  void clear_worker(Worker *worker);

  void post_worker_event(Worker *worker, Worker_event::Event event,
                         shcore::Dictionary_t &&details = {});

  void on_schema_end(const std::string &schema);

  void on_schema_ddl_start(std::size_t worker_id,
                           const Worker::Schema_ddl_task *task);
  void on_schema_ddl_end(std::size_t worker_id,
                         const Worker::Schema_ddl_task *task);

  void on_table_ddl_start(std::size_t worker_id,
                          const Worker::Table_ddl_task *task);
  void on_table_ddl_end(std::size_t worker_id, Worker::Table_ddl_task *task);

  void on_index_start(std::size_t worker_id,
                      const Worker::Index_recreation_task *task);
  void on_index_end(std::size_t worker_id,
                    const Worker::Index_recreation_task *task);

  void on_analyze_start(std::size_t worker_id,
                        const Worker::Analyze_table_task *task);
  void on_analyze_end(std::size_t worker_id,
                      const Worker::Analyze_table_task *task);

  void on_chunk_load_start(std::size_t worker_id,
                           const Worker::Load_chunk_task *task);
  void on_chunk_load_end(std::size_t worker_id,
                         const Worker::Load_chunk_task *task);

  void on_subchunk_load_start(std::size_t worker_id,
                              const shcore::Dictionary_t &event,
                              const Worker::Load_chunk_task *task);
  void on_subchunk_load_end(std::size_t worker_id,
                            const shcore::Dictionary_t &event,
                            const Worker::Load_chunk_task *task);

  void on_bulk_load_start(std::size_t worker_id,
                          const Worker::Bulk_load_task *task);
  void on_bulk_load_progress(const shcore::Dictionary_t &event);
  void on_bulk_load_end(std::size_t worker_id,
                        const Worker::Bulk_load_task *task);

  void on_checksum_start(
      const dump::common::Checksums::Checksum_data *checksum);
  void on_checksum_end(const dump::common::Checksums::Checksum_data *checksum,
                       const Worker::Checksum_task::Checksum_result &result);
  void report_checksum_error(const std::string &msg);
  bool maybe_push_checksum_task(
      const dump::common::Checksums::Checksum_data *checksum);

  friend class Worker;
  friend class Worker::Load_chunk_task;

  void check_server_version();
  void check_tables_without_primary_key();

  void handle_schema_option();

  std::function<bool(const std::string &, const std::string &)>
  filter_user_script_for_mds() const;

  bool should_create_pks() const;

  void setup_load_data_progress();

  void setup_create_indexes_progress();

  void setup_analyze_tables_progress();

  void setup_checksum_tables_progress();

  void add_skipped_schema(const std::string &schema);

  void on_ddl_done_for_schema(const std::string &schema);

  bool is_data_load_complete() const;

  void log_server_version() const;

  void push_pending_task(Task_ptr task);

  Task_ptr load_chunk_file(Dump_reader::Table_chunk chunk, bool resuming,
                           uint64_t bytes_to_skip) const;

  Task_ptr bulk_load_table(Dump_reader::Table_chunk chunk, bool resuming) const;

  Task_ptr recreate_indexes(
      const std::string &schema, const std::string &table,
      compatibility::Deferred_statements::Index_info *indexes) const;

  Task_ptr analyze_table(
      const std::string &schema, const std::string &table,
      const std::vector<Dump_reader::Histogram> &histograms) const;

  Task_ptr checksum(const dump::common::Checksums::Checksum_data *data) const;

  void load_users();

  bool bulk_load_supported(const Dump_reader::Table_chunk &chunk) const;

  bool schedule_bulk_load(Dump_reader::Table_chunk chunk);

  bool is_table_included(const std::string &schema, const std::string &table);

  void mark_table_as_in_progress(const Dump_reader::Table_chunk &chunk);

  void setup_temp_tables();

  std::string temp_table_name();

  void update_rows_throughput(uint64_t rows);

  uint64_t rows_throughput_rate();

  void on_add_index_start(uint64_t count);

  void on_add_index_result(bool success, uint64_t count);

 private:
#ifdef FRIEND_TEST
  FRIEND_TEST(Load_dump, sql_transforms_strip_sql_mode);
  FRIEND_TEST(Load_dump, add_execute_conditionally);
  friend class Load_dump_mocked;
  FRIEND_TEST(Load_dump_mocked, filter_user_script_for_mds);
#endif

  class Sql_transform {
   public:
    bool operator()(std::string_view sql, std::string *out_new_sql) const {
      if (m_ops.empty()) return false;

      std::string orig_sql(sql);
      std::string new_sql;

      for (const auto &f : m_ops) {
        f(orig_sql, &new_sql);
        orig_sql = std::move(new_sql);
      }
      *out_new_sql = std::move(orig_sql);

      return true;
    }

    void add_strip_removed_sql_modes();

    /**
     * Adds a callback which is going to be called whenever a CREATE|ALTER|DROP
     * statement for an EVENT|FUNCTION|PROCEDURE|TRIGGER object is about to
     * be executed. Callback is called with two arguments:
     *  - type - EVENT|FUNCTION|PROCEDURE|TRIGGER
     *  - name - name of the object
     * Callback should return true if this statement should be executed.
     */
    void add_execute_conditionally(
        std::function<bool(std::string_view, const std::string &)> f);

    /**
     * Whenever a USE `schema` or CREATE DATABASE ... `schema` statement is
     * executed, `schema` is going to be replaced with the new name.
     */
    void add_rename_schema(std::string_view new_name);

   private:
    void add(std::function<void(std::string_view, std::string *)> f) {
      m_ops.emplace_back(std::move(f));
    }

    std::list<std::function<void(std::string_view, std::string *)>> m_ops;
  };

  /**
   * Stable priority queue.
   */
  class Priority_queue final {
   public:
    inline void resize(std::size_t new_size) { m_tasks.resize(new_size); }

    [[nodiscard]] inline bool empty() const noexcept { return 0 == m_size; }

    [[nodiscard]] inline const Worker::Task *top() const noexcept {
      assert(!empty());
      return m_tasks[m_top_pos].front().get();
    }

    void emplace(Task_ptr task);

    [[nodiscard]] Task_ptr pop_top() noexcept;

   private:
#ifdef _WIN32
    // workaround for MSVC issue 37592
    struct Container : public std::list<Task_ptr> {
     public:
      using Base = std::list<Task_ptr>;
      using Base::Base;

      Container() = default;
      Container(Container &&) = default;

      Container &operator=(Container &&) = default;
    };
#else   // !_WIN32
    using Container = std::list<Task_ptr>;
#endif  // !_WIN32

    std::size_t m_size = 0;
    uint64_t m_top_pos = 0;
    std::vector<Container> m_tasks;
  };

  class Bulk_load_support;

  class Monitoring;

  const Load_dump_options &m_options;

  std::unique_ptr<Dump_reader> m_dump;
  std::unique_ptr<Load_progress_log> m_load_log;
  bool m_resuming = false;

  std::shared_ptr<mysqlshdk::db::mysql::Session> m_session;

  std::vector<std::thread> m_worker_threads;
  std::list<Worker> m_workers;
  Priority_queue m_pending_tasks;
  uint64_t m_current_weight = 0;

  std::mutex m_tables_being_loaded_mutex;
  std::unordered_multimap<std::string, size_t> m_tables_being_loaded;
  std::atomic<size_t> m_num_threads_loading;
  std::atomic<size_t> m_num_threads_recreating_indexes;
  std::atomic<size_t> m_num_threads_checksumming{0};
  std::atomic<size_t> m_num_index_retries{0};

  Sql_transform m_default_sql_transforms;

  shcore::Synchronized_queue<Worker_event> m_worker_events;
  std::recursive_mutex m_skip_schemas_mutex;
  std::unordered_set<std::string> m_skip_schemas;
  std::unordered_set<std::string> m_skip_tables;
  std::vector<std::exception_ptr> m_thread_exceptions;
  volatile bool m_worker_hard_interrupt = false;
  bool m_worker_interrupt = false;
  bool m_abort = false;

  std::string m_character_set;

  bool m_init_done = false;
  bool m_workers_killed = false;

  // tables and partitions loaded
  std::atomic<std::size_t> m_unique_tables_loaded = 0;
  size_t m_total_tables_with_data = 0;

  size_t m_data_bytes_previously_loaded = 0;
  size_t m_rows_previously_loaded = 0;
  import_table::Stats m_stats;
  std::atomic<size_t> m_num_errors;
  mysqlshdk::textui::Throughput m_rows_throughput;
  std::mutex m_rows_throughput_mutex;

  std::atomic<uint64_t> m_ddl_executed;

  uint64_t m_total_ddl_executed = 0;
  double m_total_ddl_execution_seconds = 0;

  // these variables are used to display the progress
  std::mutex m_indexes_display_mutex;
  double m_indexes_progress = 0.0;
  uint64_t m_indexes_recreated;
  // (this variable does not change once DDL finishes loading)
  uint64_t m_indexes_to_recreate = 0;
  // these variables are used to track the progress
  std::mutex m_indexes_progress_mutex;
  uint64_t m_indexes_completed = 0;
  uint64_t m_indexes_in_progress = 0;
  uint64_t m_index_statements_in_progress = 0;

  std::atomic<uint64_t> m_tables_analyzed;
  uint64_t m_tables_to_analyze = 0;
  bool m_all_analyze_tasks_scheduled = false;

  uint64_t m_checksum_tasks_completed = 0;
  uint64_t m_checksum_tasks_to_complete = 0;
  bool m_all_checksum_tasks_scheduled = false;

  uint64_t m_data_load_tasks_completed = 0;
  uint64_t m_data_load_tasks_scheduled = 0;
  bool m_all_data_load_tasks_scheduled = false;

  std::unordered_map<std::string, bool> m_schema_ddl_ready;
  std::unordered_map<std::string, uint64_t> m_ddl_in_progress_per_schema;

  // progress thread needs to be placed after any of the fields it uses, in
  // order to ensure that it is destroyed (and stopped) before any of those
  // fields
  dump::Progress_thread m_progress_thread;
  dump::Progress_thread::Stage *m_load_data_stage = nullptr;
  dump::Progress_thread::Stage *m_create_indexes_stage = nullptr;
  dump::Progress_thread::Stage *m_analyze_tables_stage = nullptr;
  dump::Progress_thread::Stage *m_checksum_tables_stage = nullptr;

  std::size_t m_loaded_accounts = 0;
  std::size_t m_dropped_accounts = 0;
  std::size_t m_ignored_grant_errors = 0;
  std::size_t m_ignored_plugin_errors = 0;

  std::size_t m_checksum_errors = 0;

  std::string m_temp_table_prefix;
  std::atomic<uint64_t> m_temp_table_suffix{0};

  std::unique_ptr<Monitoring> m_monitoring;

  Reconnect m_reconnect_callback;

  // BULK LOAD
  std::unique_ptr<Bulk_load_support> m_bulk_load;
};

}  // namespace mysqlsh

#endif  // MODULES_UTIL_LOAD_DUMP_LOADER_H_
