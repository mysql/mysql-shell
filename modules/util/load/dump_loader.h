/*
 * Copyright (c) 2020, 2023, Oracle and/or its affiliates.
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

#ifndef MODULES_UTIL_LOAD_DUMP_LOADER_H_
#define MODULES_UTIL_LOAD_DUMP_LOADER_H_

#include <atomic>
#include <chrono>
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

#include "mysqlshdk/libs/db/mysql/session.h"
#include "mysqlshdk/libs/storage/ifile.h"
#include "mysqlshdk/libs/utils/priority_queue.h"
#include "mysqlshdk/libs/utils/synchronized_queue.h"

namespace mysqlsh {

class Dump_loader {
 public:
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

      virtual bool execute(
          const std::shared_ptr<mysqlshdk::db::mysql::Session> &, Worker *,
          Dump_loader *) = 0;

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

      bool execute(const std::shared_ptr<mysqlshdk::db::mysql::Session> &,
                   Worker *, Dump_loader *) override;

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

      bool execute(const std::shared_ptr<mysqlshdk::db::mysql::Session> &,
                   Worker *, Dump_loader *) override;

      std::unique_ptr<compatibility::Deferred_statements>
      steal_deferred_statements() {
        return std::move(m_deferred_statements);
      }

      bool placeholder() const { return m_placeholder; }

     private:
      void pre_process(Dump_loader *loader);

      void load_ddl(
          const std::shared_ptr<mysqlshdk::db::mysql::Session> &session,
          Dump_loader *loader);

      void post_process(
          const std::shared_ptr<mysqlshdk::db::mysql::Session> &session,
          Dump_loader *loader);

      void extract_deferred_statements(Dump_loader *loader);

      void remove_duplicate_deferred_statements(
          const std::shared_ptr<mysqlshdk::db::mysql::Session> &session,
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

    class Load_chunk_task : public Table_data_task {
     public:
      Load_chunk_task(std::string_view schema, std::string_view table,
                      std::string_view partition, ssize_t chunk_index,
                      std::unique_ptr<mysqlshdk::storage::IFile> file,
                      shcore::Dictionary_t options, bool resume,
                      uint64_t bytes_to_skip)
          : Table_data_task(schema, table, partition, chunk_index),
            m_file(std::move(file)),
            m_options(options),
            m_resume(resume),
            m_bytes_to_skip(bytes_to_skip) {}

      size_t bytes_loaded = 0;
      size_t raw_bytes_loaded = 0;
      size_t rows_loaded = 0;

      bool execute(const std::shared_ptr<mysqlshdk::db::mysql::Session> &,
                   Worker *, Dump_loader *) override;

      void load(const std::shared_ptr<mysqlshdk::db::mysql::Session> &,
                Dump_loader *, Worker *);

     private:
      std::unique_ptr<mysqlshdk::storage::IFile> m_file;
      shcore::Dictionary_t m_options;
      bool m_resume = false;
      uint64_t m_bytes_to_skip = 0;
    };

    class Analyze_table_task : public Task {
     public:
      Analyze_table_task(std::string_view schema, std::string_view table,
                         const std::vector<Dump_reader::Histogram> &histograms)
          : Task(schema, table), m_histograms(histograms) {}

      bool execute(const std::shared_ptr<mysqlshdk::db::mysql::Session> &,
                   Worker *, Dump_loader *) override;

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

      bool execute(const std::shared_ptr<mysqlshdk::db::mysql::Session> &,
                   Worker *, Dump_loader *) override;

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

      bool execute(const std::shared_ptr<mysqlshdk::db::mysql::Session> &,
                   Worker *, Dump_loader *) override;

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

    uint64_t get_connection_id() const { return m_connection_id; }

   private:
    void handle_current_exception(Dump_loader *loader,
                                  const std::string &error);

    inline void handle_current_exception(const std::string &error) {
      handle_current_exception(m_owner, error);
    }

    void do_run();

    void connect();

    size_t m_id = 0;
    Dump_loader *m_owner;

    std::shared_ptr<mysqlshdk::db::mysql::Session> m_session;
    uint64_t m_connection_id;

    std::unique_ptr<Task> m_task;

    shcore::Synchronized_queue<bool> m_work_ready;
  };

  using Task_ptr = std::unique_ptr<Worker::Task>;

  struct Task_comparator {
    bool operator()(const Worker::Task &l, const Worker::Task &r) const {
      return l.weight() < r.weight();
    }

    bool operator()(const Task_ptr &l, const Task_ptr &r) const {
      return (*this)(*l, *r);
    }
  };

  using Queue =
      shcore::Priority_queue<Task_ptr, std::vector<Task_ptr>, Task_comparator>;

  struct Worker_event {
    enum Event {
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
    };
    Event event;
    Worker *worker = nullptr;
    shcore::Dictionary_t details;
  };

  using Name_and_file =
      std::pair<std::string, std::unique_ptr<mysqlshdk::storage::IFile>>;

  void execute_tasks();
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

  void switch_schema(const std::string &schema, bool load_done);

  bool should_fetch_table_ddl(bool placeholder) const;

  bool schedule_table_chunk(const std::string &schema, const std::string &table,
                            const std::string &partition, ssize_t chunk_index,
                            std::unique_ptr<mysqlshdk::storage::IFile> file,
                            size_t size, shcore::Dictionary_t options,
                            bool resuming, uint64_t bytes_to_skip);

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

  void on_schema_ddl_start(const std::string &schema);
  void on_schema_ddl_end(const std::string &schema);

  void on_table_ddl_start(const std::string &schema, const std::string &table,
                          bool placeholder);
  void on_table_ddl_end(
      const std::string &schema, const std::string &table, bool placeholder,
      std::unique_ptr<compatibility::Deferred_statements> deferred_indexes);

  void on_chunk_load_start(const std::string &schema, const std::string &table,
                           const std::string &partition, ssize_t index);
  void on_chunk_load_end(const std::string &schema, const std::string &table,
                         const std::string &partition, ssize_t index,
                         size_t bytes_loaded, size_t raw_bytes_loaded,
                         size_t rows_loaded);

  void on_subchunk_load_start(const std::string &schema,
                              const std::string &table,
                              const std::string &partition, ssize_t index,
                              uint64_t subchunk);
  void on_subchunk_load_end(const std::string &schema, const std::string &table,
                            const std::string &partition, ssize_t index,
                            uint64_t subchunk, uint64_t bytes);

  void on_index_start(const std::string &schema, const std::string &table);
  void on_index_end(const std::string &schema, const std::string &table);

  void on_analyze_start(const std::string &schema, const std::string &table);
  void on_analyze_end(const std::string &schema, const std::string &table);

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

  static inline std::shared_ptr<mysqlshdk::db::IResult> query(
      const std::shared_ptr<mysqlshdk::db::ISession> &session,
      std::string_view sql) {
    return session->query(sql);
  }

  static inline void execute(
      const std::shared_ptr<mysqlshdk::db::ISession> &session,
      std::string_view sql) {
    session->execute(sql);
  }

  template <typename... Args>
  static inline void executef(
      const std::shared_ptr<mysqlshdk::db::ISession> &session, const char *sql,
      Args &&... args) {
    session->executef(sql, std::forward<Args>(args)...);
  }

  inline std::shared_ptr<mysqlshdk::db::IResult> query(
      std::string_view sql) const {
    return query(m_session, sql);
  }

  inline void execute(std::string_view sql) const {
    return execute(m_session, sql);
  }

  template <typename... Args>
  inline void executef(const char *sql, Args &&... args) {
    executef(m_session, sql, std::forward<Args>(args)...);
  }

  void log_server_version() const;

  void push_pending_task(Task_ptr task);

  Task_ptr load_chunk_file(const std::string &schema, const std::string &table,
                           const std::string &partition,
                           std::unique_ptr<mysqlshdk::storage::IFile> file,
                           ssize_t chunk_index, size_t chunk_size,
                           const shcore::Dictionary_t &options, bool resuming,
                           uint64_t bytes_to_skip) const;

  Task_ptr recreate_indexes(
      const std::string &schema, const std::string &table,
      compatibility::Deferred_statements::Index_info *indexes) const;

  Task_ptr analyze_table(
      const std::string &schema, const std::string &table,
      const std::vector<Dump_reader::Histogram> &histograms) const;

  Task_ptr checksum(const dump::common::Checksums::Checksum_data *data) const;

  void load_users();

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

  const Load_dump_options &m_options;

  std::unique_ptr<Dump_reader> m_dump;
  std::unique_ptr<Load_progress_log> m_load_log;
  bool m_resuming = false;

  std::shared_ptr<mysqlshdk::db::ISession> m_session;

  std::vector<std::thread> m_worker_threads;
  std::list<Worker> m_workers;
  Queue m_pending_tasks;
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
  std::unordered_set<std::string> m_unique_tables_loaded;
  size_t m_total_tables_with_data = 0;

  size_t m_num_bytes_previously_loaded = 0;
  std::atomic<size_t> m_num_rows_loaded;
  std::atomic<size_t> m_num_rows_deleted = 0;
  std::atomic<size_t> m_num_bytes_loaded;
  std::atomic<size_t> m_num_raw_bytes_loaded;
  std::atomic<size_t> m_num_chunks_loaded;
  std::atomic<size_t> m_num_warnings;
  std::atomic<size_t> m_num_errors;

  std::atomic<uint64_t> m_ddl_executed;

  std::atomic<uint64_t> m_indexes_recreated;
  uint64_t m_indexes_to_recreate = 0;
  bool m_index_count_is_known = false;

  std::atomic<uint64_t> m_tables_analyzed;
  uint64_t m_tables_to_analyze = 0;
  bool m_all_analyze_tasks_scheduled = false;

  uint64_t m_checksum_tasks_completed = 0;
  uint64_t m_checksum_tasks_to_complete = 0;
  bool m_all_checksum_tasks_scheduled = false;

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

  std::size_t m_checksum_errors = 0;
};

}  // namespace mysqlsh

#endif  // MODULES_UTIL_LOAD_DUMP_LOADER_H_
