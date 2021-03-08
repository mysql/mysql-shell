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

#ifndef MODULES_UTIL_LOAD_DUMP_LOADER_H_
#define MODULES_UTIL_LOAD_DUMP_LOADER_H_

#include <atomic>
#include <list>
#include <memory>
#include <regex>
#include <set>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>
#include "modules/util/load/dump_reader.h"
#include "modules/util/load/load_dump_options.h"
#include "modules/util/load/load_progress_log.h"
#include "mysqlshdk/include/shellcore/scoped_contexts.h"
#include "mysqlshdk/libs/db/mysql/session.h"
#include "mysqlshdk/libs/storage/ifile.h"
#include "mysqlshdk/libs/textui/text_progress.h"
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

  void run();

 private:
  class Worker {
   public:
    class Task {
     public:
      Task(size_t aid, const std::string &schema, const std::string &table)
          : m_id(aid), m_schema(schema), m_table(table) {}
      virtual ~Task() {}

      virtual bool execute(
          const std::shared_ptr<mysqlshdk::db::mysql::Session> &, Worker *,
          Dump_loader *) = 0;

      size_t id() const { return m_id; }
      const std::string &schema() const { return m_schema; }
      const std::string &table() const { return m_table; }

     protected:
      static void handle_current_exception(Worker *worker, Dump_loader *loader,
                                           const std::string &error);

     protected:
      size_t m_id;
      std::string m_schema;
      std::string m_table;
    };

    class Table_ddl_task : public Task {
     public:
      Table_ddl_task(size_t id, const std::string &schema,
                     const std::string &table,
                     std::unique_ptr<mysqlshdk::storage::IFile> file,
                     bool placeholder, Load_progress_log::Status status)
          : Task(id, schema, table),
            m_file(std::move(file)),
            m_placeholder(placeholder),
            m_status(status) {}

      bool execute(const std::shared_ptr<mysqlshdk::db::mysql::Session> &,
                   Worker *, Dump_loader *) override;

      std::unique_ptr<std::vector<std::string>> steal_deferred_indexes() {
        return std::move(m_deferred_indexes);
      }

      bool placeholder() const { return m_placeholder; }

     private:
      void process(
          const std::shared_ptr<mysqlshdk::db::mysql::Session> &session,
          std::string *out_script, Dump_loader *loader);

      void load_ddl(
          const std::shared_ptr<mysqlshdk::db::mysql::Session> &session,
          const std::string &script, Dump_loader *loader);

      void extract_pending_indexes(
          const std::shared_ptr<mysqlshdk::db::mysql::Session> &session,
          std::string *script, bool fulltext_only, bool check_recreated,
          Dump_loader *loader);

     private:
      std::unique_ptr<mysqlshdk::storage::IFile> m_file;
      bool m_placeholder = false;
      Load_progress_log::Status m_status;

      std::unique_ptr<std::vector<std::string>> m_deferred_indexes;
    };

    class Ddl_fetch_task : public Task {
     public:
      Ddl_fetch_task(size_t id, const std::string &schema,
                     const std::string &object,
                     std::unique_ptr<mysqlshdk::storage::IFile> file,
                     const std::function<void(std::string &&)> &fn)
          : Task(id, schema, object),
            m_file(std::move(file)),
            m_process_fn(fn) {}

      bool execute(const std::shared_ptr<mysqlshdk::db::mysql::Session> &,
                   Worker *, Dump_loader *) override;

      std::unique_ptr<std::string> steal_script() {
        return std::move(m_script);
      }

      const std::function<void(std::string &&)> &process_fn() const {
        return m_process_fn;
      }

     private:
      std::unique_ptr<mysqlshdk::storage::IFile> m_file;
      std::function<void(std::string &&)> m_process_fn;
      std::unique_ptr<std::string> m_script;
    };

    class Load_chunk_task : public Task {
     public:
      Load_chunk_task(size_t id, const std::string &schema,
                      const std::string &table, ssize_t chunk_index,
                      std::unique_ptr<mysqlshdk::storage::IFile> file,
                      shcore::Dictionary_t options, bool resume,
                      uint64_t bytes_to_skip)
          : Task(id, schema, table),
            m_chunk_index(chunk_index),
            m_file(std::move(file)),
            m_options(options),
            m_resume(resume),
            m_bytes_to_skip(bytes_to_skip) {}

      size_t bytes_loaded = 0;
      size_t raw_bytes_loaded = 0;

      bool execute(const std::shared_ptr<mysqlshdk::db::mysql::Session> &,
                   Worker *, Dump_loader *) override;

      void load(const std::shared_ptr<mysqlshdk::db::mysql::Session> &,
                Dump_loader *, Worker *);

      ssize_t chunk_index() const { return m_chunk_index; }

     private:
      std::string query_comment() const;

      ssize_t m_chunk_index;
      std::unique_ptr<mysqlshdk::storage::IFile> m_file;
      shcore::Dictionary_t m_options;
      bool m_resume = false;
      uint64_t m_bytes_to_skip = 0;
    };

    class Analyze_table_task : public Task {
     public:
      Analyze_table_task(size_t id, const std::string &schema,
                         const std::string &table,
                         const std::vector<Dump_reader::Histogram> &histograms)
          : Task(id, schema, table), m_histograms(histograms) {}

      bool execute(const std::shared_ptr<mysqlshdk::db::mysql::Session> &,
                   Worker *, Dump_loader *) override;

     private:
      std::vector<Dump_reader::Histogram> m_histograms;
    };

    class Index_recreation_task : public Task {
     public:
      Index_recreation_task(size_t id, const std::string &schema,
                            const std::string &table,
                            const std::vector<std::string> &queries)
          : Task(id, schema, table), m_queries(queries) {}

      bool execute(const std::shared_ptr<mysqlshdk::db::mysql::Session> &,
                   Worker *, Dump_loader *) override;

     private:
      const std::vector<std::string> &m_queries;
    };

    Worker(size_t id, Dump_loader *owner);
    size_t id() const { return m_id; }

    void fetch_object_ddl(const std::string &schema, const std::string &object,
                          std::unique_ptr<mysqlshdk::storage::IFile> file,
                          const std::function<void(std::string &&)> &fn);

    void process_table_ddl(const std::string &schema, const std::string &table,
                           std::unique_ptr<mysqlshdk::storage::IFile> file,
                           bool is_placeholder,
                           Load_progress_log::Status status);

    void load_chunk_file(const std::string &schema, const std::string &table,
                         std::unique_ptr<mysqlshdk::storage::IFile> file,
                         ssize_t chunk_index, size_t chunk_size,
                         const shcore::Dictionary_t &options, bool resuming,
                         uint64_t bytes_to_skip);

    void recreate_indexes(const std::string &schema, const std::string &table,
                          const std::vector<std::string> &indexes);
    void analyze_table(const std::string &schema, const std::string &table,
                       const std::vector<Dump_reader::Histogram> &histograms);

    Task *current_task() const { return m_task.get(); }

    void run();
    void stop();

    uint64_t get_connection_id() const { return m_connection_id; }

   private:
    void connect();

    size_t m_id = 0;
    Dump_loader *m_owner;

    std::shared_ptr<mysqlshdk::db::mysql::Session> m_session;
    uint64_t m_connection_id;

    std::unique_ptr<Task> m_task;

    shcore::Synchronized_queue<bool> m_work_ready;
  };

  struct Worker_event {
    enum Event {
      READY,
      FATAL_ERROR,
      TABLE_DDL_START,
      TABLE_DDL_END,
      FETCH_DDL_START,
      FETCH_DDL_END,
      LOAD_START,
      LOAD_END,
      INDEX_START,
      INDEX_END,
      ANALYZE_START,
      ANALYZE_END,
      EXIT,
      LOAD_SUBCHUNK_START,
      LOAD_SUBCHUNK_END,
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

  bool wait_for_more_data();

  std::shared_ptr<mysqlshdk::db::mysql::Session> create_session();

  void show_summary();

  void on_dump_begin();
  void on_dump_end();

  bool handle_table_data(Worker *worker);
  void handle_schema_post_scripts();

  void handle_schema(const std::string &schema, bool resuming);
  void switch_schema(const std::string &schema, bool load_done);

  bool schedule_table_chunk(const std::string &schema, const std::string &table,
                            ssize_t chunk_index, Worker *worker,
                            std::unique_ptr<mysqlshdk::storage::IFile> file,
                            size_t size, shcore::Dictionary_t options,
                            bool resuming, uint64_t bytes_to_skip);

  bool schedule_next_task(Worker *worker);
  size_t handle_worker_events(
      const std::function<bool(Worker *)> &schedule_next);

  void execute_threaded(const std::function<bool(Worker *)> &schedule_next);

  void check_existing_objects();
  bool report_duplicates(const std::string &what, const std::string &schema,
                         mysqlshdk::db::IResult *result);

  void open_dump();
  void open_dump(std::unique_ptr<mysqlshdk::storage::IDirectory> dumpdir);
  void setup_progress(bool *out_is_resuming);
  void spawn_workers();
  void join_workers();

  void clear_worker(Worker *worker);

  void post_worker_event(Worker *worker, Worker_event::Event event,
                         shcore::Dictionary_t &&details = {});

  void on_schema_end(const std::string &schema);

  void on_table_ddl_start(const std::string &schema, const std::string &table,
                          bool placeholder);
  void on_table_ddl_end(
      const std::string &schema, const std::string &table,
      std::unique_ptr<std::vector<std::string>> deferred_indexes,
      bool placeholder);

  void on_fetch_ddl_start(const std::string &schema, const std::string &object);
  void on_fetch_ddl_end(const std::string &schema, const std::string &object,
                        std::unique_ptr<std::string> script,
                        const std::function<void(std::string &&)> &fn);

  void on_chunk_load_start(const std::string &schema, const std::string &table,
                           ssize_t index);
  void on_chunk_load_end(const std::string &schema, const std::string &table,
                         ssize_t index, size_t bytes_loaded,
                         size_t raw_bytes_loaded);

  void on_subchunk_load_start(const std::string &schema,
                              const std::string &table, ssize_t index,
                              uint64_t subchunk);
  void on_subchunk_load_end(const std::string &schema, const std::string &table,
                            ssize_t index, uint64_t subchunk, uint64_t bytes);

  friend class Worker;
  friend class Worker::Table_ddl_task;
  friend class Worker::Load_chunk_task;

  const std::string &pre_data_script() const;
  const std::string &post_data_script() const;

  void update_progress(bool force = false);

  void check_server_version();
  void check_tables_without_primary_key();

  void handle_schema_option();

  std::string filter_user_script_for_mds(const std::string &script);

  bool should_create_pks() const;

 private:
#ifdef FRIEND_TEST
  FRIEND_TEST(Load_dump, sql_transforms_strip_sql_mode);
  FRIEND_TEST(Load_dump_mocked, chunk_scheduling_more_threads);
  FRIEND_TEST(Load_dump_mocked, chunk_scheduling_more_tables);
  FRIEND_TEST(Load_dump_mocked, filter_user_script_for_mds);
  FRIEND_TEST(Load_dump_mocked, sql_generate_invisible_primary_key);
#endif

  class Sql_transform {
   public:
    bool operator()(const char *sql, size_t length, std::string *out_new_sql) {
      if (m_ops.empty()) return false;

      std::string orig_sql(sql, length);
      std::string new_sql;

      for (const auto &f : m_ops) {
        f(orig_sql, &new_sql);
        orig_sql = new_sql;
      }
      *out_new_sql = new_sql;

      return true;
    }

    void add_strip_removed_sql_modes();

   private:
    void add(std::function<void(const std::string &, std::string *)> &&f) {
      m_ops.emplace_back(std::move(f));
    }

    std::list<std::function<void(const std::string &, std::string *)>> m_ops;
  };

  const Load_dump_options &m_options;

  std::unique_ptr<Dump_reader> m_dump;
  std::unique_ptr<Load_progress_log> m_load_log;
  bool m_resuming = false;

  std::shared_ptr<mysqlshdk::db::ISession> m_session;

  std::vector<std::thread> m_worker_threads;
  std::list<Worker> m_workers;

  std::mutex m_tables_being_loaded_mutex;
  std::unordered_multimap<std::string, size_t> m_tables_being_loaded;
  std::atomic<size_t> m_num_threads_loading;
  std::atomic<size_t> m_num_threads_recreating_indexes;

  Sql_transform m_default_sql_transforms;

  shcore::Synchronized_queue<Worker_event> m_worker_events;
  std::unordered_set<std::string> m_skip_schemas;
  std::unordered_set<std::string> m_skip_tables;
  std::vector<std::exception_ptr> m_thread_exceptions;
  volatile bool m_worker_hard_interrupt = false;
  bool m_worker_interrupt = false;
  bool m_abort = false;

  std::string m_character_set;

  bool m_init_done = false;
  bool m_workers_killed = false;

  std::unique_ptr<mysqlshdk::textui::IProgress> m_progress;
  std::recursive_mutex m_output_mutex;
  Scoped_console m_console;

  std::unordered_set<std::string> m_unique_tables_loaded;
  size_t m_total_tables_with_data = 0;

  std::chrono::system_clock::time_point m_begin_time;
  size_t m_num_bytes_previously_loaded = 0;
  std::atomic<size_t> m_num_rows_loaded;
  std::atomic<size_t> m_num_bytes_loaded;
  std::atomic<size_t> m_num_raw_bytes_loaded;
  std::atomic<size_t> m_num_chunks_loaded;
  std::atomic<size_t> m_num_warnings;
  std::atomic<size_t> m_num_errors;

  int m_progress_spin = 0;
};

}  // namespace mysqlsh

#endif  // MODULES_UTIL_LOAD_DUMP_LOADER_H_
