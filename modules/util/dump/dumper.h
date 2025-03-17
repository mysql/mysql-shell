/*
 * Copyright (c) 2020, 2025, Oracle and/or its affiliates.
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

#ifndef MODULES_UTIL_DUMP_DUMPER_H_
#define MODULES_UTIL_DUMP_DUMPER_H_

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <string_view>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "mysqlshdk/libs/db/column.h"
#include "mysqlshdk/libs/mysql/user_privileges.h"
#include "mysqlshdk/libs/storage/backend/memory_file.h"
#include "mysqlshdk/libs/storage/idirectory.h"
#include "mysqlshdk/libs/storage/ifile.h"
#include "mysqlshdk/libs/textui/text_progress.h"
#include "mysqlshdk/libs/utils/atomic_flag.h"
#include "mysqlshdk/libs/utils/enumset.h"
#include "mysqlshdk/libs/utils/synchronized_queue.h"
#include "mysqlshdk/libs/utils/version.h"

#include "modules/util/common/dump/basenames.h"
#include "modules/util/common/dump/checksums.h"
#include "modules/util/common/dump/server_info.h"
#include "modules/util/common/dump/vector_store_info.h"
#include "modules/util/dump/capability.h"
#include "modules/util/dump/dump_options.h"
#include "modules/util/dump/dump_writer.h"
#include "modules/util/dump/instance_cache.h"
#include "modules/util/dump/progress_thread.h"

namespace mysqlsh {
namespace dump {

namespace issues {

enum class Status {
  FIXED,
  FIXED_CREATE_PKS,
  FIXED_IGNORE_PKS,
  WARNING,
  WARNING_DEPRECATED_DEFINERS,
  WARNING_ESCAPED_WILDCARDS,
  WARNING_HAS_INVALID_VIEW_REFERENCES,
  ERROR,
  ERROR_MISSING_PKS,
  ERROR_HAS_INVALID_GRANTS,
  ERROR_HAS_WILDCARD_GRANTS,
  ERROR_HAS_NON_STANDARD_FKS,
};

using Status_set =
    mysqlshdk::utils::Enum_set<Status, Status::ERROR_HAS_NON_STANDARD_FKS>;

}  // namespace issues

class Schema_dumper;

class Dumper {
 public:
  Dumper() = delete;
  explicit Dumper(const Dump_options &options);

  Dumper(const Dumper &) = delete;
  Dumper(Dumper &&) = delete;

  Dumper &operator=(const Dumper &) = delete;
  Dumper &operator=(Dumper &&) = delete;

  virtual ~Dumper() = default;

  void run();

  void interrupt();

  void interruption_notification();

  void abort();

  Progress_thread *progress() { return &m_progress_thread; }

 protected:
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
      Args &&...args) {
    session->executef(sql, std::forward<Args>(args)...);
  }

  inline std::shared_ptr<mysqlshdk::db::IResult> query(
      std::string_view sql) const {
    return query(session(), sql);
  }

  inline void execute(std::string_view sql) const {
    return execute(session(), sql);
  }

  virtual std::unique_ptr<Schema_dumper> schema_dumper(
      const std::shared_ptr<mysqlshdk::db::ISession> &session) const;

  static std::string format_object_stats(uint64_t value, uint64_t total,
                                         std::string_view objects);

  bool dump_users() const;

  std::string decode_column(const Instance_cache::Table *table,
                            const Instance_cache::Column *column) const;

 private:
  class Dump_writer_controller;
  class Single_file_writer_controller;
  class Default_writer_controller;
  class Multi_file_writer_controller;

  struct Output_config {
    void init(
        const import_table::Dialect &dialect,
        mysqlshdk::storage::Compression compression,
        const mysqlshdk::storage::Compression_options &compression_options);

    void create_directory();

    void fini();

    std::unique_ptr<mysqlshdk::storage::IDirectory> dir;
    std::string data_file_extension;

    import_table::Dialect dialect;

    mysqlshdk::storage::Compression compression =
        mysqlshdk::storage::Compression::NONE;
    mysqlshdk::storage::Compression_options compression_options;

    std::function<std::unique_ptr<Dump_writer>()> writer_creator;
  };

  struct Object_info {
    std::string name;
    std::string quoted_name;
    std::string basename;
  };

  struct Index_info {
    const Instance_cache::Index *info = nullptr;
    bool is_pke = false;
  };

  struct Partition_info {
    std::string basename;
    const Instance_cache::Partition *info = nullptr;
  };

  struct Table_info : public Object_info {
    const Instance_cache::Table *info = nullptr;
    std::vector<Partition_info> partitions;
  };

  struct View_info : public Object_info {
    const Instance_cache::View *info = nullptr;
  };

  struct Schema_info : public Object_info {
    std::vector<Table_info> tables;
    std::vector<View_info> views;
  };

  struct Table_task : Table_info {
    std::string task_name;
    std::string schema;
    std::string extra_filter;
    Index_info index;
    const Output_config *output_config = nullptr;
  };

  struct Table_data_task : Table_task {
    std::unique_ptr<Dump_writer_controller> controller;
    std::string id;
    int64_t chunk;
    std::string boundary;
  };

  struct Checksum_task {
    std::string name;
    std::string id;
    common::Checksums::Checksum_data *checksum = nullptr;
  };

  class Table_worker;

  struct Task_info {
    std::string info;
    std::function<void(Table_worker *)> task;
  };

  class Dump_info;

  class Memory_dumper;

  virtual const char *name() const = 0;

  virtual void summary() const = 0;

  virtual std::vector<std::string> object_stats(
      const Instance_cache::Stats &filtered,
      const Instance_cache::Stats &total) const = 0;

  virtual void on_create_table_task(const std::string &schema,
                                    const std::string &table,
                                    const Instance_cache::Table *cache) = 0;

  const std::shared_ptr<mysqlshdk::db::ISession> &session() const noexcept;

  void do_run();

  void on_init_thread_session(
      const std::shared_ptr<mysqlshdk::db::ISession> &session) const;

  void open_session();

  void close_session();

  void fetch_user_privileges();

  void warn_about_backup_lock() const;

  std::string why_backup_lock_is_missing() const;

  void acquire_read_locks();

  void release_read_locks() const;

  void lock_all_tables();

  void start_transaction(
      const std::shared_ptr<mysqlshdk::db::ISession> &session) const;

  void assert_transaction_is_open(
      const std::shared_ptr<mysqlshdk::db::ISession> &session) const;

  void lock_instance();

  void initialize_instance_cache_minimal();

  void initialize_instance_cache();

  void create_schema_tasks();

  void validate_mds() const;

  void initialize_counters();

  void initialize_dump();

  void finalize_dump();

  void create_output_directory();

  void close_output_directory();

  void setup_vector_store_directory();

  void close_vector_store_directory();

  void create_worker_sessions();

  void create_worker_threads();

  void maybe_push_shutdown_tasks();

  void chunking_task_finished();

  void data_task_finished();

  void checksum_task_started();

  void checksum_task_finished();

  void wait_for_all_tasks();

  void dump_ddl() const;

  void dump_global_ddl() const;

  void dump_users_ddl() const;

  void write_ddl(const Memory_dumper &in_memory, const std::string &file) const;

  std::unique_ptr<Memory_dumper> dump_ddl(
      Schema_dumper *dumper,
      const std::function<void(Memory_dumper *)> &func) const;

  /**
   * Dumps schema, events, routines and libraries to a single file.
   */
  std::unique_ptr<Memory_dumper> dump_complete_schema(
      Schema_dumper *dumper, const std::string &schema) const;

  /**
   * Dumps just the schema DDL.
   */
  std::unique_ptr<Memory_dumper> dump_schema(Schema_dumper *dumper,
                                             const std::string &schema) const;

  std::unique_ptr<Memory_dumper> dump_events(Schema_dumper *dumper,
                                             const std::string &schema) const;

  std::unique_ptr<Memory_dumper> dump_libraries(
      Schema_dumper *dumper, const std::string &schema) const;

  std::unique_ptr<Memory_dumper> dump_routines(Schema_dumper *dumper,
                                               const std::string &schema) const;

  std::unique_ptr<Memory_dumper> dump_table(Schema_dumper *dumper,
                                            const std::string &schema,
                                            const std::string &table) const;

  std::unique_ptr<Memory_dumper> dump_triggers(Schema_dumper *dumper,
                                               const std::string &schema,
                                               const std::string &table) const;

  std::unique_ptr<Memory_dumper> dump_temporary_view(
      Schema_dumper *dumper, const std::string &schema,
      const std::string &view) const;

  std::unique_ptr<Memory_dumper> dump_view(Schema_dumper *dumper,
                                           const std::string &schema,
                                           const std::string &view) const;

  std::unique_ptr<Memory_dumper> dump_users(Schema_dumper *dumper) const;

  void create_schema_metadata_tasks();

  void create_schema_ddl_tasks();

  void create_table_tasks();

  Table_task create_table_task(const Schema_info &schema,
                               const Table_info &table);

  Table_task create_table_task(const Schema_info &schema,
                               const View_info &view);

  void push_table_task(Table_task &&task);

  void push_table_chunking_task(Table_task &&task);

  void push_table_data_task(Table_data_task &&task);

  Checksum_task create_checksum_task(const Table_data_task &table);

  Checksum_task create_checksum_task(const Table_task &table);

  void push_checksum_task(Checksum_task &&task);

  bool should_dump_data(const Table_task &table) const;

  std::unique_ptr<Dump_writer_controller> table_dump_controller(
      const Output_config *config, const std::string &filename) const;

  std::unique_ptr<Dump_writer_controller> table_dump_multi_file_controller(
      const Output_config *config, const std::string &basename) const;

  void finish_writing(const std::string &schema, const std::string &table,
                      const Dump_writer_controller *controller);

  void write_dump_redirected_metadata() const;

  void write_dump_started_metadata() const;

  void write_dump_finished_metadata() const;

  void write_checksum_metadata() const;

  void write_schema_metadata(
      const Schema_info &schema,
      const std::shared_ptr<mysqlshdk::db::ISession> &session) const;

  void write_table_metadata(
      const Table_task &table,
      const std::shared_ptr<mysqlshdk::db::ISession> &session) const;

  void summarize() const;

  void rethrow() const;

  void emergency_shutdown();

  void kill_workers();

  void initialize_throughput_progress();

  void initialize_checksum_progress();

  void update_progress(const Dump_write_result &progress);

  void shutdown_progress();

  std::string throughput() const;

  inline mysqlshdk::storage::IDirectory *dump_dir() const noexcept {
    return m_dump_config.dir.get();
  }

  inline mysqlshdk::storage::IDirectory *vector_store_dir() const noexcept {
    return m_vector_store_config.dir.get();
  }

  std::unique_ptr<mysqlshdk::storage::IFile> make_dump_file(
      const std::string &filename) const;

  std::unique_ptr<mysqlshdk::storage::IFile> make_data_file(
      const Output_config *config, const std::string &filename,
      bool use_mmap) const;

  std::string get_basename(const std::string &basename);

  bool compressed() const;

  std::string get_query_comment(const std::string &quoted_name,
                                const std::string &id,
                                const char *context) const;

  std::string get_query_comment(const Table_data_task &task,
                                const char *context) const;

  void validate_privileges() const;

  bool is_gtid_executed_inconsistent() const;

  void log_server_version() const;

  void print_object_stats() const;

  void validate_schemas_list() const;

  void validate_dump_consistency(
      const std::shared_ptr<mysqlshdk::db::ISession> &session) const;

  void fetch_server_information();

  // returns true in case of errors
  issues::Status_set check_for_upgrade_errors() const;

  void throw_if_cannot_dump_users() const;

  shcore::Synchronized_queue<std::shared_ptr<mysqlshdk::db::ISession>>
      &session_pool() {
    return m_session_pool;
  }

  bool all_tasks_produced() const;

  void handle_invalid_view_references(issues::Status_set status,
                                      bool as_error) const;

  std::string gtid_executed(
      const std::shared_ptr<mysqlshdk::db::ISession> &session) const;

  common::Binlog binlog(
      const std::shared_ptr<mysqlshdk::db::ISession> &session) const;

  Dump_writer::Encoding_type encoding_type(
      const Instance_cache::Table *table,
      const Instance_cache::Column *column) const;

  std::string optimizer_hints(const Instance_cache::Table *info) const;

  bool uses_multifile_schema_ddl() const noexcept {
    return m_capability_set.is_set(Capability::MULTIFILE_SCHEMA_DDL);
  }

  bool uses_dump_dir_redirection() const noexcept {
    return !m_redirected_dump_dir.empty();
  }

  bool uses_innodb_vector_store() const noexcept {
    return m_capability_set.is_set(Capability::INNODB_VECTOR_STORE);
  }

  // session
  std::shared_ptr<mysqlshdk::db::ISession> m_session;
#ifndef NDEBUG
  std::thread::id m_main_thread;
#endif  // !NDEBUG
  std::vector<std::shared_ptr<mysqlshdk::db::ISession>> m_lock_sessions;
  common::Server_version m_server_version;
  bool m_binlog_enabled = false;
  bool m_gtid_enabled = false;

  // user privileges
  std::unique_ptr<mysqlshdk::mysql::User_privileges> m_user_privileges;
  std::string m_user_account;
  bool m_skip_grant_tables_active = false;
  // whether user has the BACKUP_ADMIN privilege
  bool m_user_has_backup_admin = false;

  // input data
  const Dump_options &m_options;

  mysqlshdk::storage::File_options m_mmap_options;
  Output_config m_dump_config;

  Output_config m_vector_store_config;
  std::string m_vector_store_output_description;

  std::string m_redirected_dump_dir;
  std::string m_redirected_output_url;

  std::unique_ptr<mysqlshdk::storage::IFile> m_output_file;
  Instance_cache m_cache;
  std::vector<Schema_info> m_schema_infos;
  common::Basenames m_basenames;

  // status variables
  bool m_instance_locked = false;
  // whether FLUSH TABLES WITH READ LOCK was used
  bool m_ftwrl_used = false;
  std::unordered_set<Capability> m_used_capabilities;
  Capability_set m_capability_set;
  bool m_has_invalid_view_references = false;

  // counters
  uint64_t m_total_rows = 0;
  uint64_t m_total_schemas = 0;
  uint64_t m_total_tables = 0;
  uint64_t m_total_views = 0;
  uint64_t m_total_objects = 0;

  // progress information
  std::atomic<uint64_t> m_data_bytes;
  std::atomic<uint64_t> m_bytes_written;
  std::atomic<uint64_t> m_rows_written;
  std::atomic<uint64_t> m_num_threads_chunking;
  std::atomic<uint64_t> m_num_threads_checksumming;
  std::atomic<uint64_t> m_num_threads_dumping;
  std::atomic<uint64_t> m_ddl_written;

  std::atomic<uint64_t> m_schema_metadata_written;

  std::atomic<uint64_t> m_table_metadata_to_write;
  std::atomic<uint64_t> m_table_metadata_written;
  bool m_all_table_metadata_tasks_scheduled = false;

  mutable std::recursive_mutex m_throughput_mutex;
  std::unique_ptr<mysqlshdk::textui::Throughput> m_data_throughput;
  std::unique_ptr<mysqlshdk::textui::Throughput> m_bytes_throughput;

  std::mutex m_table_data_stats_mutex;
  // schema -> table -> data stats
  std::unordered_map<std::string,
                     std::unordered_map<std::string, Dump_write_result>>
      m_table_data_stats;

  // path -> uncompressed bytes
  std::unordered_map<std::string, uint64_t> m_chunk_file_bytes;

  // threads
  std::vector<std::thread> m_workers;
  std::vector<std::exception_ptr> m_worker_exceptions;
  std::atomic<bool> m_worker_exception_thrown = false;
  shcore::Synchronized_queue<Task_info> m_worker_tasks;
  std::atomic<uint64_t> m_chunking_tasks_total;
  std::atomic<uint64_t> m_chunking_tasks_completed;
  std::atomic<uint64_t> m_data_tasks_total;
  std::atomic<uint64_t> m_data_tasks_completed;
  std::atomic<bool> m_checksum_started;
  std::atomic<uint64_t> m_checksum_tasks_total;
  std::atomic<uint64_t> m_checksum_tasks_completed;
  Progress_thread::Duration m_checksum_duration;
  std::atomic<bool> m_main_thread_finished_producing_chunking_tasks;
  shcore::atomic_flag m_worker_interrupt;

  // progress thread needs to be placed after any of the fields it uses, in
  // order to ensure that it is destroyed (and stopped) before any of those
  // fields
  mutable Progress_thread m_progress_thread;
  Progress_thread::Stage *m_data_dump_stage = nullptr;

  shcore::Synchronized_queue<std::shared_ptr<mysqlshdk::db::ISession>>
      m_session_pool;

  std::mutex m_checksums_mutex;
  std::unique_ptr<common::Checksums> m_checksum;

  common::Vector_store_info m_vector_store_info;
};

}  // namespace dump
}  // namespace mysqlsh

#endif  // MODULES_UTIL_DUMP_DUMPER_H_
