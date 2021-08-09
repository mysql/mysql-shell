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

#include "modules/util/load/dump_loader.h"

#include <mysqld_error.h>

#include <algorithm>
#include <cinttypes>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "modules/mod_utils.h"
#include "modules/util/dump/capability.h"
#include "modules/util/dump/compatibility.h"
#include "modules/util/dump/schema_dumper.h"
#include "modules/util/import_table/load_data.h"
#include "modules/util/load/load_progress_log.h"
#include "mysqlshdk/include/scripting/naming_style.h"
#include "mysqlshdk/include/shellcore/console.h"
#include "mysqlshdk/include/shellcore/shell_init.h"
#include "mysqlshdk/include/shellcore/shell_options.h"
#include "mysqlshdk/libs/mysql/instance.h"
#include "mysqlshdk/libs/mysql/script.h"
#include "mysqlshdk/libs/mysql/utils.h"
#include "mysqlshdk/libs/storage/compressed_file.h"
#include "mysqlshdk/libs/utils/fault_injection.h"
#include "mysqlshdk/libs/utils/strformat.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_lexing.h"
#include "mysqlshdk/libs/utils/utils_mysql_parsing.h"
#include "mysqlshdk/libs/utils/utils_net.h"
#include "mysqlshdk/libs/utils/version.h"

namespace mysqlsh {

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

class dump_wait_timeout : public std::runtime_error {
 public:
  explicit dump_wait_timeout(const char *w) : std::runtime_error(w) {}
};

namespace {

bool histograms_supported(const Version &version) {
  return version > Version(8, 0, 0);
}

bool has_pke(mysqlshdk::db::ISession *session, const std::string &schema,
             const std::string &table) {
  // Return true if the table has a PK or equivalent (UNIQUE NOT NULL)
  auto res = session->queryf("SHOW INDEX IN !.!", schema, table);
  while (auto row = res->fetch_one_named()) {
    if (row.get_int("Non_unique") == 0 && row.get_string("Null").empty())
      return true;
  }
  return false;
}

std::vector<std::string> preprocess_table_script_for_indexes(
    std::string *script, const std::string &key, bool fulltext_only) {
  std::vector<std::string> indexes;
  auto script_length = script->length();
  std::istringstream stream(*script);
  script->clear();
  mysqlshdk::utils::iterate_sql_stream(
      &stream, script_length,
      [&](const char *s, size_t len, const std::string &delim, size_t) {
        auto sql = std::string(s, len) + delim + "\n";
        mysqlshdk::utils::SQL_iterator sit(sql);
        if (shcore::str_caseeq(sit.next_token(), "CREATE") &&
            shcore::str_caseeq(sit.next_token(), "TABLE")) {
          assert(indexes.empty());
          indexes = compatibility::check_create_table_for_indexes(
              sql, fulltext_only, &sql);
        }
        script->append(sql);
        return true;
      },
      [&key](const std::string &err) {
        throw std::runtime_error("Error splitting DDL script for table " + key +
                                 ": " + err);
      });
  return indexes;
}

void add_invisible_pk(std::string *script, const std::string &key) {
  const auto script_length = script->length();
  std::istringstream stream(*script);

  script->clear();

  mysqlshdk::utils::iterate_sql_stream(
      &stream, script_length,
      [&](const char *s, size_t len, const std::string &delim, size_t) {
        auto sql = std::string(s, len) + delim + "\n";
        mysqlshdk::utils::SQL_iterator sit(sql);

        if (shcore::str_caseeq(sit.next_token(), "CREATE") &&
            shcore::str_caseeq(sit.next_token(), "TABLE")) {
          compatibility::add_pk_to_create_table(sql, &sql);
        }

        script->append(sql);
        return true;
      },
      [&key](const std::string &err) {
        throw std::runtime_error("Error splitting DDL script for table " + key +
                                 ": " + err);
      });
}

void execute_statement(const std::shared_ptr<mysqlshdk::db::ISession> &session,
                       const std::string_view &stmt,
                       const std::string &error_prefix) {
  assert(!error_prefix.empty());

  constexpr uint32_t k_max_retry_time = 5 * 60 * 1000;  // 5 minutes
  uint32_t sleep_time = 200;  // start with 200 ms, double it with each sleep
  uint32_t total_sleep_time = 0;

  while (true) {
    try {
      session->executes(stmt.data(), stmt.length());
      return;
    } catch (const mysqlshdk::db::Error &e) {
      log_info("Error executing SQL: %s:\n%.*s", e.format().c_str(),
               static_cast<int>(stmt.length()), stmt.data());

      if (ER_LOCK_DEADLOCK == e.code() && total_sleep_time < k_max_retry_time) {
        current_console()->print_note(
            error_prefix + ", will retry after delay: " + e.format());

        if (total_sleep_time + sleep_time > k_max_retry_time) {
          sleep_time = k_max_retry_time - total_sleep_time;
        }

        shcore::sleep_ms(sleep_time);

        total_sleep_time += sleep_time;
        sleep_time *= 2;
      } else {
        current_console()->print_error(shcore::str_format(
            "%s: %s: %.*s", error_prefix.c_str(), e.format().c_str(),
            static_cast<int>(stmt.length()), stmt.data()));
        throw;
      }
    }
  }
}

void execute_script(const std::shared_ptr<mysqlshdk::db::ISession> &session,
                    const std::string &script, const std::string &error_prefix,
                    const std::function<bool(const char *, size_t,
                                             std::string *)> &process_stmt) {
  std::stringstream stream(script);

  mysqlshdk::utils::iterate_sql_stream(
      &stream, 1024 * 64,
      [&error_prefix, &session, &process_stmt](const char *s, size_t len,
                                               const std::string &, size_t) {
        std::string new_stmt;

        if (process_stmt && process_stmt(s, len, &new_stmt)) {
          s = new_stmt.data();
          len = new_stmt.length();
        }

        if (len > 0) {
          execute_statement(session, {s, len}, error_prefix);
        }

        return true;
      },
      [](const std::string &err) { current_console()->print_error(err); });
}

class Index_file {
 public:
  explicit Index_file(mysqlshdk::storage::IFile *data_file) {
    m_idx_file = data_file->parent()->file(data_file->filename() + ".idx");
  }

  uint64_t data_size() {
    load_metadata();
    return m_data_size;
  }

  const std::vector<uint64_t> &offsets() {
    load_offsets();
    return m_offsets;
  }

 private:
  void load_metadata() {
    if (m_metadata_loaded) {
      return;
    }

    m_file_size = m_idx_file->file_size();
    if ((m_file_size % k_entry_size) != 0 || m_file_size < k_entry_size) {
      log_warning(
          "idx file %s has unexpected size %s, which is not a multiple of %s",
          m_idx_file->filename().c_str(), std::to_string(m_file_size).c_str(),
          std::to_string(k_entry_size).c_str());
      return;
    }
    m_entries = m_file_size / k_entry_size;

    m_idx_file->open(mysqlshdk::storage::Mode::READ);
    m_idx_file->seek(m_file_size - k_entry_size);
    m_idx_file->read(&m_data_size, k_entry_size);
    m_idx_file->seek(0);
    m_idx_file->close();

    m_data_size = mysqlshdk::utils::network_to_host(m_data_size);

    m_metadata_loaded = true;
  }

  void load_offsets() {
    if (m_offsets_loaded) {
      return;
    }

    load_metadata();

    if (m_entries > 0) {
      m_offsets.resize(m_entries);

      m_idx_file->open(mysqlshdk::storage::Mode::READ);
      m_idx_file->read(&m_offsets[0], m_file_size);
      m_idx_file->close();

      uint64_t prev = 0;
      bool invalid = false;

      std::transform(m_offsets.begin(), m_offsets.end(), m_offsets.begin(),
                     [&prev, &invalid](uint64_t offset) {
                       const auto next =
                           mysqlshdk::utils::network_to_host(offset);
                       invalid |= prev > next;
                       prev = next;
                       return next;
                     });

      if (invalid) {
        m_offsets.clear();
      }
    }
    m_offsets_loaded = true;
  }

  static constexpr uint64_t k_entry_size = sizeof(uint64_t);

  std::unique_ptr<mysqlshdk::storage::IFile> m_idx_file;

  std::size_t m_file_size = 0;

  uint64_t m_entries = 0;

  uint64_t m_data_size = 0;

  std::vector<uint64_t> m_offsets;

  bool m_metadata_loaded = false;

  bool m_offsets_loaded = false;
};

std::string format_table(const std::string &schema, const std::string &table,
                         const std::string &partition, ssize_t chunk) {
  return "`" + schema + "`.`" + table + "`" +
         (partition.empty() ? "" : " partition `" + partition + "`") +
         " (chunk " + std::to_string(chunk) + ")";
}

}  // namespace

void Dump_loader::Worker::Task::handle_current_exception(
    Worker *worker, Dump_loader *loader, const std::string &error) {
  const auto id = worker->id();

  if (!loader->m_thread_exceptions[id]) {
    current_console()->print_error(
        shcore::str_format("[Worker%03zu] %s", id, error.c_str()));
    loader->m_thread_exceptions[id] = std::current_exception();
  }

  loader->m_num_errors += 1;
  loader->m_worker_interrupt = true;
  loader->post_worker_event(worker, Worker_event::FATAL_ERROR);
}

bool Dump_loader::Worker::Schema_ddl_task::execute(
    const std::shared_ptr<mysqlshdk::db::mysql::Session> &session,
    Worker *worker, Dump_loader *loader) {
  log_debug("worker%zu will execute DDL for schema %s", id(), schema().c_str());

  loader->post_worker_event(worker, Worker_event::SCHEMA_DDL_START);

  if (!m_script.empty()) {
    try {
      log_info("%s DDL script for schema `%s`",
               m_resuming ? "Re-executing" : "Executing", schema().c_str());

      if (!loader->m_options.dry_run()) {
        // execute sql
        execute_script(session, m_script,
                       shcore::str_format("While processing schema `%s`",
                                          schema().c_str()),
                       loader->m_default_sql_transforms);
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

  log_debug("worker%zu done", id());
  ++loader->m_ddl_executed;
  loader->post_worker_event(worker, Worker_event::SCHEMA_DDL_END);

  return true;
}

bool Dump_loader::Worker::Table_ddl_task::execute(
    const std::shared_ptr<mysqlshdk::db::mysql::Session> &session,
    Worker *worker, Dump_loader *loader) {
  log_debug("worker%zu will execute DDL file for table %s", id(),
            key().c_str());

  loader->post_worker_event(worker, Worker_event::TABLE_DDL_START);

  try {
    std::string script;
    process(session, loader);

    if (!loader->m_options.dry_run()) {
      // this is here to detect if data is loaded into a non-existing schema
      session->executef("use !", m_schema.c_str());
    }

    load_ddl(session, loader);
  } catch (const std::exception &e) {
    handle_current_exception(
        worker, loader,
        shcore::str_format("While executing DDL script for %s: %s",
                           key().c_str(), e.what()));
    return false;
  }

  log_debug("worker%zu done", id());
  ++loader->m_ddl_executed;
  loader->post_worker_event(worker, Worker_event::TABLE_DDL_END);

  return true;
}

void Dump_loader::Worker::Table_ddl_task::process(
    const std::shared_ptr<mysqlshdk::db::mysql::Session> &session,
    Dump_loader *loader) {
  if (!m_placeholder && (loader->m_options.load_ddl() ||
                         loader->m_options.load_deferred_indexes())) {
    if (loader->m_options.defer_table_indexes() !=
        Load_dump_options::Defer_index_mode::OFF) {
      extract_pending_indexes(session,
                              loader->m_options.defer_table_indexes() ==
                                  Load_dump_options::Defer_index_mode::FULLTEXT,
                              m_status == Load_progress_log::DONE, loader);
    }

    if (loader->m_options.load_ddl() && loader->should_create_pks() &&
        !loader->m_options.auto_create_pks_supported() &&
        !loader->m_dump->has_primary_key(m_schema, m_table)) {
      add_invisible_pk(&m_script, key());
    }
  }
}

void Dump_loader::Worker::Table_ddl_task::extract_pending_indexes(
    const std::shared_ptr<mysqlshdk::db::mysql::Session> &session,
    bool fulltext_only, bool check_recreated, Dump_loader *loader) {
  m_deferred_indexes = std::make_unique<std::vector<std::string>>(
      preprocess_table_script_for_indexes(&m_script, key(), fulltext_only));

  if (check_recreated && !m_deferred_indexes->empty()) {
    // this handles the case where the table was already created in a previous
    // run and some indexes may already have been re-created
    try {
      auto ct = session->query("show create table " + key())
                    ->fetch_one()
                    ->get_string(1);
      auto recreated =
          compatibility::check_create_table_for_indexes(ct, fulltext_only);
      m_deferred_indexes->erase(
          std::remove_if(m_deferred_indexes->begin(), m_deferred_indexes->end(),
                         [&recreated](const std::string &i) {
                           return std::find(recreated.begin(), recreated.end(),
                                            i) != recreated.end();
                         }),
          m_deferred_indexes->end());
    } catch (const shcore::Error &e) {
      current_console()->print_error(
          shcore::str_format("Unable to get status of table %s that "
                             "according to load status "
                             "was already created, consider resetting "
                             "progress, error message: %s",
                             key().c_str(), e.what()));
      if (!loader->m_options.force()) throw;
    }
  }

  const auto prefix = "ALTER TABLE " + key() + " ADD ";
  for (size_t i = 0; i < m_deferred_indexes->size(); ++i)
    (*m_deferred_indexes)[i] = prefix + (*m_deferred_indexes)[i] + ";";
}

void Dump_loader::Worker::Table_ddl_task::load_ddl(
    const std::shared_ptr<mysqlshdk::db::mysql::Session> &session,
    Dump_loader *loader) {
  if (m_status != Load_progress_log::DONE && loader->m_options.load_ddl()) {
    log_debug("worker%zu: Executing %stable DDL for %s", id(),
              m_placeholder ? "placeholder " : "", key().c_str());

    try {
      log_info(
          "[Worker%03zu] %s DDL script for %s%s", id(),
          (m_status == Load_progress_log::INTERRUPTED ? "Re-executing"
                                                      : "Executing"),
          key().c_str(),
          (m_placeholder ? " (placeholder for view)"
                         : (m_deferred_indexes && !m_deferred_indexes->empty()
                                ? " (indexes removed for deferred creation)"
                                : "")));
      if (!loader->m_options.dry_run()) {
        // execute sql
        execute_script(
            session, m_script,
            shcore::str_format("[Worker%03zu] Error processing table %s", id(),
                               key().c_str()),
            loader->m_default_sql_transforms);
      }
    } catch (const std::exception &e) {
      if (!loader->m_options.force()) throw;
    }
  }
}

bool Dump_loader::Worker::Load_chunk_task::execute(
    const std::shared_ptr<mysqlshdk::db::mysql::Session> &session,
    Worker *worker, Dump_loader *loader) {
  const auto masked_path = m_file->full_path().masked();

  log_debug("worker%zu will load chunk %s for table `%s`.`%s`", id(),
            masked_path.c_str(), schema().c_str(), table().c_str());

  try {
    FI_TRIGGER_TRAP(dump_loader,
                    mysqlshdk::utils::FI::Trigger_options(
                        {{"op", "BEFORE_LOAD_START"},
                         {"schema", schema()},
                         {"table", table()},
                         {"chunk", std::to_string(chunk_index())}}));

    loader->post_worker_event(worker, Worker_event::LOAD_START);

    FI_TRIGGER_TRAP(dump_loader,
                    mysqlshdk::utils::FI::Trigger_options(
                        {{"op", "AFTER_LOAD_START"},
                         {"schema", schema()},
                         {"table", table()},
                         {"chunk", std::to_string(chunk_index())}}));

    // do work
    if (!loader->m_options.dry_run()) {
      // load the data
      load(session, loader, worker);
    }

    FI_TRIGGER_TRAP(dump_loader,
                    mysqlshdk::utils::FI::Trigger_options(
                        {{"op", "BEFORE_LOAD_END"},
                         {"schema", schema()},
                         {"table", table()},
                         {"chunk", std::to_string(chunk_index())}}));
  } catch (const std::exception &e) {
    handle_current_exception(worker, loader,
                             shcore::str_format("While loading %s: %s",
                                                masked_path.c_str(), e.what()));
    return false;
  }

  log_debug("worker%zu done", id());

  // signal for more work
  loader->post_worker_event(worker, Worker_event::LOAD_END);

  return true;
}

std::string Dump_loader::Worker::Load_chunk_task::query_comment() const {
  std::string query_comment =
      "/* mysqlsh loadDump(), thread " + std::to_string(id()) + ", table " +
      shcore::str_replace(
          "`" + schema() + "`.`" + table() + "`" +
              (partition().empty() ? "" : ".`" + partition() + "`"),
          "*/", "*\\/");
  if (chunk_index() >= 0) {
    query_comment += ", chunk ID: " + std::to_string(chunk_index());
  }
  query_comment += " */ ";
  return query_comment;
}

void Dump_loader::Worker::Load_chunk_task::load(
    const std::shared_ptr<mysqlshdk::db::mysql::Session> &session,
    Dump_loader *loader, Worker *worker) {
  import_table::Import_table_options import_options;

  import_table::Import_table_options::options().unpack(m_options,
                                                       &import_options);

  // replace duplicate rows by default
  import_options.set_replace_duplicates(true);

  import_options.set_base_session(session);

  import_options.set_verbose(false);

  import_options.set_partition(partition());

  import_table::Stats stats;
  if (m_resume) {
    // Truncate the table if its not chunked, but if it's chunked leave it
    // and rely on duplicate rows being ignored.
    if (chunk_index() < 0 && !has_pke(session.get(), schema(), table())) {
      current_console()->print_note(shcore::str_format(
          "Truncating table `%s`.`%s` because of resume and it "
          "has no PK or equivalent key",
          schema().c_str(), table().c_str()));
      session->executef("TRUNCATE TABLE !.!", schema(), table());

      m_bytes_to_skip = 0;
    }
  }

  import_table::Load_data_worker op(
      import_options, id(), &loader->m_num_bytes_loaded,
      &loader->m_worker_hard_interrupt, nullptr, &loader->m_thread_exceptions,
      &stats, query_comment());

  loader->m_num_threads_loading++;

  shcore::on_leave_scope cleanup([this, loader]() {
    std::lock_guard<std::mutex> lock(loader->m_tables_being_loaded_mutex);
    auto it = loader->m_tables_being_loaded.find(key());
    while (it != loader->m_tables_being_loaded.end() && it->first == key()) {
      if (it->second == raw_bytes_loaded) {
        loader->m_tables_being_loaded.erase(it);
        break;
      }
      ++it;
    }

    loader->m_num_threads_loading--;
  });

  {
    mysqlshdk::storage::Compression compr;
    try {
      compr = mysqlshdk::storage::from_extension(
          std::get<1>(shcore::path::split_extension(m_file->filename())));
    } catch (...) {
      compr = mysqlshdk::storage::Compression::NONE;
    }

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
    const auto &max_bytes_per_transaction =
        loader->m_options.max_bytes_per_transaction();

    import_table::Transaction_options options;

    // if the maxBytesPerTransaction is not given, it defaults to bytesPerChunk
    // value used during the dump
    options.max_trx_size =
        max_bytes_per_transaction.get_safe(loader->m_dump->bytes_per_chunk());

    uint64_t max_chunk_size = options.max_trx_size;

    // if maxBytesPerTransaction is not given, only files bigger than
    // k_chunk_size_overshoot_tolerance * bytesPerChunk are affected
    if (!max_bytes_per_transaction) {
      max_chunk_size *= k_chunk_size_overshoot_tolerance;
    }

    Index_file idx_file{m_file.get()};

    if (options.max_trx_size > 0) {
      bool valid = false;
      auto chunk_file_size =
          loader->m_dump->chunk_size(m_file->filename(), &valid);

      if (!valid) {
        // @.done.json not there yet, use the idx file directly
        chunk_file_size = idx_file.data_size();
      }

      if (chunk_file_size < max_chunk_size) {
        // chunk is small enough, so don't sub-chunk
        options.max_trx_size = 0;
      } else {
        // data will not fit into a single transaction and needs to be divided
        // load the row offsets from the IDX file
        options.offsets = &idx_file.offsets();
        if (options.offsets->empty()) options.offsets = nullptr;
      }
    }

    uint64_t subchunk = 0;

    options.transaction_started = [this, &loader, &worker, &subchunk]() {
      log_debug("Transaction for '%s'.'%s'/%zi subchunk %" PRIu64
                " has started",
                schema().c_str(), table().c_str(), chunk_index(), subchunk);

      FI_TRIGGER_TRAP(dump_loader,
                      mysqlshdk::utils::FI::Trigger_options(
                          {{"op", "BEFORE_LOAD_SUBCHUNK_START"},
                           {"schema", schema()},
                           {"table", table()},
                           {"chunk", std::to_string(chunk_index())},
                           {"subchunk", std::to_string(subchunk)}}));

      loader->post_worker_event(worker, Worker_event::LOAD_SUBCHUNK_START,
                                shcore::make_dict("subchunk", subchunk));

      FI_TRIGGER_TRAP(dump_loader,
                      mysqlshdk::utils::FI::Trigger_options(
                          {{"op", "AFTER_LOAD_SUBCHUNK_START"},
                           {"schema", schema()},
                           {"table", table()},
                           {"chunk", std::to_string(chunk_index())},
                           {"subchunk", std::to_string(subchunk)}}));
    };

    options.transaction_finished = [this, &loader, &worker,
                                    &subchunk](uint64_t bytes) {
      log_debug("Transaction for '%s'.'%s'/%zi subchunk %" PRIu64
                " has finished, wrote %" PRIu64 " bytes",
                schema().c_str(), table().c_str(), chunk_index(), subchunk,
                bytes);

      FI_TRIGGER_TRAP(dump_loader,
                      mysqlshdk::utils::FI::Trigger_options(
                          {{"op", "BEFORE_LOAD_SUBCHUNK_END"},
                           {"schema", schema()},
                           {"table", table()},
                           {"chunk", std::to_string(chunk_index())},
                           {"subchunk", std::to_string(subchunk)}}));

      loader->post_worker_event(
          worker, Worker_event::LOAD_SUBCHUNK_END,
          shcore::make_dict("subchunk", subchunk, "bytes", bytes));

      FI_TRIGGER_TRAP(dump_loader,
                      mysqlshdk::utils::FI::Trigger_options(
                          {{"op", "AFTER_LOAD_SUBCHUNK_END"},
                           {"schema", schema()},
                           {"table", table()},
                           {"chunk", std::to_string(chunk_index())},
                           {"subchunk", std::to_string(subchunk)}}));

      ++subchunk;
    };

    options.skip_bytes = m_bytes_to_skip;

    op.execute(session, mysqlshdk::storage::make_file(std::move(m_file), compr),
               options);
  }

  if (loader->m_thread_exceptions[id()])
    std::rethrow_exception(loader->m_thread_exceptions[id()]);

  bytes_loaded = m_bytes_to_skip + stats.total_bytes;
  loader->m_num_raw_bytes_loaded += raw_bytes_loaded;

  loader->m_num_chunks_loaded += 1;
  loader->m_num_rows_loaded += stats.total_records;
  loader->m_num_warnings += stats.total_warnings;
}

bool Dump_loader::Worker::Analyze_table_task::execute(
    const std::shared_ptr<mysqlshdk::db::mysql::Session> &session,
    Worker *worker, Dump_loader *loader) {
  log_debug("worker%zu will analyze table `%s`.`%s`", id(), schema().c_str(),
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
      if (m_histograms.empty() ||
          !histograms_supported(loader->m_options.target_server_version())) {
        session->executef("ANALYZE TABLE !.!", schema(), table());
      } else {
        for (const auto &h : m_histograms) {
          shcore::sqlstring q(
              "ANALYZE TABLE !.! UPDATE HISTOGRAM ON ! WITH ? BUCKETS", 0);
          q << schema() << table() << h.column << h.buckets;

          std::string sql = q.str();
          log_debug("Executing %s", sql.c_str());
          session->execute(sql);
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

  log_debug("worker%zu done", id());
  ++loader->m_tables_analyzed;

  // signal for more work
  loader->post_worker_event(worker, Worker_event::ANALYZE_END);
  return true;
}

bool Dump_loader::Worker::Index_recreation_task::execute(
    const std::shared_ptr<mysqlshdk::db::mysql::Session> &session,
    Worker *worker, Dump_loader *loader) {
  log_debug("worker%zu will recreate indexes for table `%s`.`%s`", id(),
            schema().c_str(), table().c_str());

  const auto console = current_console();

  if (!m_queries.empty())
    log_info("[Worker%03zu] Recreating indexes for `%s`.`%s`", id(),
             schema().c_str(), table().c_str());

  loader->post_worker_event(worker, Worker_event::INDEX_START);

  // do work
  if (!loader->m_options.dry_run()) {
    loader->m_num_threads_recreating_indexes++;
    shcore::on_leave_scope cleanup(
        [loader]() { loader->m_num_threads_recreating_indexes--; });

    try {
      for (const auto &query : m_queries) {
        try {
          execute_statement(
              session, query,
              "While recreating indexes for table `" + m_table + "`");
        } catch (const shcore::Error &e) {
          // Deadlocks and duplicate key errors should not happen but if they
          // do they can be ignored at least for a while
          if (e.code() == ER_DUP_KEYNAME) {
            console->print_note("Index already existed for query: " + query);
          } else {
            throw;
          }
        }
      }
    } catch (const std::exception &e) {
      handle_current_exception(
          worker, loader,
          shcore::str_format("While recreating indexes for table `%s`.`%s`: %s",
                             schema().c_str(), table().c_str(), e.what()));
      return false;
    }
  }

  log_debug("worker%zu done", id());
  ++loader->m_indexes_recreated;

  // signal for more work
  loader->post_worker_event(worker, Worker_event::INDEX_END);
  return true;
}

Dump_loader::Worker::Worker(size_t id, Dump_loader *owner)
    : m_id(id), m_owner(owner), m_connection_id(0) {}

void Dump_loader::Worker::run() {
  auto console = current_console();

  try {
    connect();
  } catch (const shcore::Error &e) {
    console->print_error(shcore::str_format(
        "[Worker%03zu] Error opening connection to MySQL: %s", m_id,
        e.format().c_str()));

    m_owner->m_thread_exceptions[m_id] = std::current_exception();
    m_owner->m_num_errors += 1;
    m_owner->post_worker_event(this, Worker_event::FATAL_ERROR);
    return;
  }

  for (;;) {
    m_owner->post_worker_event(this, Worker_event::READY);

    // wait for signal that there's work to do... false means stop worker
    bool work = m_work_ready.pop();
    if (!work || m_owner->m_worker_interrupt) {
      m_owner->post_worker_event(this, Worker_event::EXIT);
      break;
    }

    assert(std::numeric_limits<size_t>::max() != m_task->id());

    if (!m_task->execute(m_session, this, m_owner)) break;
  }
}

void Dump_loader::Worker::stop() { m_work_ready.push(false); }

void Dump_loader::Worker::connect() {
  m_session = m_owner->create_session();
  m_connection_id = m_session->get_connection_id();
}

void Dump_loader::Worker::schedule(std::unique_ptr<Task> task) {
  task->set_id(m_id);
  m_task = std::move(task);
  m_work_ready.push(true);
}

void Dump_loader::Worker::load_chunk_file(
    const std::string &schema, const std::string &table,
    const std::string &partition,
    std::unique_ptr<mysqlshdk::storage::IFile> file, ssize_t chunk_index,
    size_t chunk_size, const shcore::Dictionary_t &options, bool resuming,
    uint64_t bytes_to_skip) {
  log_debug("Loading data for %s",
            format_table(schema, table, partition, chunk_index).c_str());
  assert(!schema.empty());
  assert(!table.empty());
  assert(file);

  // The reason why sending work to worker threads isn't done through a
  // regular queue is because a regular queue would create a static schedule
  // for the chunk loading order. But we need to be able to dynamically
  // schedule chunks based on the current conditions at the time each new
  // chunk needs to be scheduled.

  auto task = std::make_unique<Load_chunk_task>(
      schema, table, partition, chunk_index, std::move(file), options, resuming,
      bytes_to_skip);
  task->raw_bytes_loaded = chunk_size;

  schedule(std::move(task));
}

void Dump_loader::Worker::recreate_indexes(
    const std::string &schema, const std::string &table,
    const std::vector<std::string> &indexes) {
  log_debug("Recreating indexes for `%s`.`%s`", schema.c_str(), table.c_str());
  assert(!schema.empty());
  assert(!table.empty());

  schedule(std::make_unique<Index_recreation_task>(schema, table, indexes));
}

void Dump_loader::Worker::analyze_table(
    const std::string &schema, const std::string &table,
    const std::vector<Dump_reader::Histogram> &histograms) {
  log_debug("Analyzing table `%s`.`%s`", schema.c_str(), table.c_str());
  assert(!schema.empty());
  assert(!table.empty());

  schedule(std::make_unique<Analyze_table_task>(schema, table, histograms));
}
// ----

Dump_loader::Dump_loader(const Load_dump_options &options)
    : m_options(options),
      m_num_threads_loading(0),
      m_num_threads_recreating_indexes(0),
      m_character_set(options.character_set()),
      m_progress_thread("Load dump", options.show_progress()),
      m_num_rows_loaded(0),
      m_num_bytes_loaded(0),
      m_num_raw_bytes_loaded(0),
      m_num_chunks_loaded(0),
      m_num_warnings(0),
      m_num_errors(0) {
  if (m_options.ignore_version()) {
    m_default_sql_transforms.add_strip_removed_sql_modes();
  }
}

Dump_loader::~Dump_loader() {}

std::shared_ptr<mysqlshdk::db::mysql::Session> Dump_loader::create_session() {
  auto session = mysqlshdk::db::mysql::Session::create();

  session->connect(m_options.connection_options());

  // Set timeouts to larger values since worker threads may get stuck
  // downloading data for some time before they have a chance to get back to
  // doing MySQL work.
  session->executef("SET SESSION net_read_timeout = ?",
                    k_mysql_server_net_read_timeout);

  // This is the time until the server kicks out idle connections. Our
  // connections should last for as long as the dump lasts even if they're
  // idle.
  session->executef("SET SESSION wait_timeout = ?",
                    k_mysql_server_wait_timeout);

  // Disable binlog if requested by user
  if (m_options.skip_binlog()) {
    try {
      session->execute("SET sql_log_bin=0");
    } catch (const mysqlshdk::db::Error &e) {
      throw shcore::Exception::runtime_error(
          "'SET sql_log_bin=0' failed with error - " + e.format());
    }
  }

  session->execute("SET foreign_key_checks = 0");
  session->execute("SET unique_checks = 0");

  // Make sure we don't get affected by user customizations of sql_mode
  session->execute("SET SQL_MODE = 'NO_AUTO_VALUE_ON_ZERO'");

  if (!m_character_set.empty())
    session->executef("SET NAMES ?", m_character_set);

  if (m_dump->tz_utc()) session->execute("SET TIME_ZONE='+00:00'");

  if (m_options.load_ddl() && m_options.auto_create_pks_supported()) {
    // target server supports automatic creation of primary keys, we need to
    // explicitly set the value of session variable, so we won't end up creating
    // primary keys when user doesn't want to do that
    session->executef("SET @@SESSION.sql_generate_invisible_primary_key=?",
                      should_create_pks());
  }

  return session;
}

std::string Dump_loader::filter_user_script_for_mds(const std::string &script) {
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
  return dump::Schema_dumper::preprocess_users_script(
      script,
      [this](const std::string &account) {
        return m_options.include_user(shcore::split_account(account));
      },
      [&restrictions, &allowed_global_grants](const std::string &priv_type,
                                              const std::string &priv_level) {
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
      });
}

void Dump_loader::on_dump_begin() {
  std::string pre_script = m_dump->begin_script();

  current_console()->print_status("Executing common preamble SQL");

  if (!m_options.dry_run())
    execute_script(m_session, pre_script, "While executing preamble SQL",
                   m_default_sql_transforms);
}

void Dump_loader::on_dump_end() {
  // Users have to be loaded last, because GRANTs on specific objects require
  // the objects to exist
  if (m_options.load_users()) {
    std::string script = m_dump->users_script();

    current_console()->print_status("Executing user accounts SQL...");

    if (m_options.is_mds()) {
      script = filter_user_script_for_mds(script);
    } else {
      script = dump::Schema_dumper::preprocess_users_script(
          script,
          [this](const std::string &account) {
            return m_options.include_user(shcore::split_account(account));
          },
          {});
    }

    if (!m_options.dry_run())
      execute_script(m_session, script, "While executing user accounts SQL",
                     m_default_sql_transforms);
  }

  std::string post_script = m_dump->end_script();

  // Execute schema end scripts
  for (const std::string &schema : m_dump->schemas()) {
    on_schema_end(schema);
  }

  current_console()->print_status("Executing common postamble SQL");

  if (!m_options.dry_run())
    execute_script(m_session, post_script, "While executing postamble SQL",
                   m_default_sql_transforms);

  // Update GTID_PURGED only when requested by the user
  if (m_options.update_gtid_set() != Load_dump_options::Update_gtid_set::OFF) {
    auto status = m_load_log->gtid_update_status();
    if (status == Load_progress_log::Status::DONE) {
      current_console()->print_status("GTID_PURGED already updated");
      log_info("GTID_PURGED already updated");
    } else if (!m_dump->gtid_executed().empty()) {
      if (m_dump->gtid_executed_inconsistent()) {
        current_console()->print_warning(
            "The gtid update requested, but gtid_executed was not guaranteed "
            "to be consistent during the dump");
      }

      try {
        m_load_log->start_gtid_update();

        const auto query = m_options.is_mds() ? "CALL sys.set_gtid_purged(?)"
                                              : "SET GLOBAL GTID_PURGED=?";

        if (m_options.update_gtid_set() ==
            Load_dump_options::Update_gtid_set::REPLACE) {
          current_console()->print_status(
              "Resetting GTID_PURGED to dumped gtid set");
          log_info("Setting GTID_PURGED to %s",
                   m_dump->gtid_executed().c_str());
          m_session->executef(query, m_dump->gtid_executed());
        } else {
          current_console()->print_status(
              "Appending dumped gtid set to GTID_PURGED");
          log_info("Appending %s to GTID_PURGED",
                   m_dump->gtid_executed().c_str());
          m_session->executef(query, "+" + m_dump->gtid_executed());
        }
        m_load_log->end_gtid_update();
      } catch (const std::exception &e) {
        current_console()->print_error(
            std::string("Error while updating GTID_PURGED: ") + e.what());
        throw;
      }
    } else {
      current_console()->print_warning(
          "gtid update requested but, gtid_executed not set in dump");
    }
  }

  // check if redo log is disabled and print a reminder if so
  auto res = m_session->query(
      "SELECT VARIABLE_VALUE = 'OFF' FROM "
      "performance_schema.global_status "
      "WHERE variable_name = 'Innodb_redo_log_enabled'");
  if (auto row = res->fetch_one()) {
    if (row->get_int(0, 0)) {
      current_console()->print_note(
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
            m_session->execute(q);
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
    auto status = m_load_log->triggers_ddl_status(schema, it.first);

    log_debug("Triggers DDL for `%s`.`%s` (%s)", schema.c_str(),
              it.first.c_str(), to_string(status).c_str());

    if (m_options.load_ddl()) {
      m_load_log->start_triggers_ddl(schema, it.first);

      if (status != Load_progress_log::DONE) {
        log_info("Executing triggers SQL for `%s`.`%s`", schema.c_str(),
                 it.first.c_str());

        it.second->open(mysqlshdk::storage::Mode::READ);
        std::string script = mysqlshdk::storage::read_file(it.second.get());
        it.second->close();

        if (!m_options.dry_run()) {
          m_session->executef("USE !", schema);

          execute_script(m_session, script, "While executing triggers SQL",
                         m_default_sql_transforms);
        }
      }

      m_load_log->end_triggers_ddl(schema, it.first);
    }
  }
}

void Dump_loader::switch_schema(const std::string &schema, bool load_done) {
  if (!m_options.dry_run()) {
    try {
      m_session->executef("use !", schema.c_str());
    } catch (const std::exception &e) {
      current_console()->print_error(shcore::str_format(
          "Unable to use schema `%s`%s, error message: %s", schema.c_str(),
          load_done ? " that according to load status was already created, "
                      "consider resetting progress"
                    : "",
          e.what()));

      if (!m_options.force()) throw;

      add_skipped_schema(schema);
    }
  }
}

bool Dump_loader::should_fetch_table_ddl(bool placeholder) const {
  return m_options.load_ddl() ||
         (!placeholder && m_options.load_deferred_indexes());
}

bool Dump_loader::handle_table_data(Worker *worker) {
  std::unique_ptr<mysqlshdk::storage::IFile> data_file;

  bool scheduled = false;
  bool chunked = false;
  size_t index = 0;
  size_t total = 0;
  size_t size = 0;
  std::unordered_multimap<std::string, size_t> tables_being_loaded;
  std::string schema;
  std::string table;
  std::string partition;
  shcore::Dictionary_t options;

  // Note: job scheduling should preferably load different tables per thread,
  //       each partition is treated as a different table

  do {
    {
      std::lock_guard<std::mutex> lock(m_tables_being_loaded_mutex);
      tables_being_loaded = m_tables_being_loaded;
    }
    if (m_dump->next_table_chunk(tables_being_loaded, &schema, &table,
                                 &partition, &chunked, &index, &total,
                                 &data_file, &size, &options)) {
      options->set("showProgress", m_options.show_progress()
                                       ? shcore::Value::True()
                                       : shcore::Value::False());

      // Override characterSet if given in options
      if (!m_options.character_set().empty()) {
        options->set("characterSet", shcore::Value(m_options.character_set()));
      }

      const auto chunk = chunked ? index : -1;
      auto status =
          m_load_log->table_chunk_status(schema, table, partition, chunk);

      log_debug("Table data for %s (%s)",
                format_table(schema, table, partition, chunk).c_str(),
                to_string(status).c_str());

      uint64_t bytes_to_skip = 0;

      // if task was interrupted, check if any of the subchunks were loaded, if
      // yes then we need to skip them
      if (status == Load_progress_log::INTERRUPTED) {
        uint64_t subchunk = 0;

        while (m_load_log->table_subchunk_status(schema, table, partition,
                                                 chunk, subchunk) ==
               Load_progress_log::DONE) {
          bytes_to_skip += m_load_log->table_subchunk_size(
              schema, table, partition, chunk, subchunk);
          ++subchunk;
        }

        if (subchunk > 0) {
          log_debug(
              "Loading table data for %s was interrupted after "
              "%" PRIu64 " subchunks were loaded, skipping %" PRIu64 " bytes",
              format_table(schema, table, partition, chunk).c_str(), subchunk,
              bytes_to_skip);
        }
      }

      if (m_options.load_data()) {
        if (status != Load_progress_log::DONE) {
          scheduled = schedule_table_chunk(
              schema, table, partition, chunk, worker, std::move(data_file),
              size, options, status == Load_progress_log::INTERRUPTED,
              bytes_to_skip);
        }
      }
    } else {
      scheduled = false;
      break;
    }
  } while (!scheduled);

  return scheduled;
}

bool Dump_loader::schedule_table_chunk(
    const std::string &schema, const std::string &table,
    const std::string &partition, ssize_t chunk_index, Worker *worker,
    std::unique_ptr<mysqlshdk::storage::IFile> file, size_t size,
    shcore::Dictionary_t options, bool resuming, uint64_t bytes_to_skip) {
  {
    std::lock_guard<std::recursive_mutex> lock(m_skip_schemas_mutex);

    if (m_skip_schemas.find(schema) != m_skip_schemas.end() ||
        m_skip_tables.find(schema_table_key(schema, table)) !=
            m_skip_tables.end() ||
        !m_options.include_table(schema, table))
      return false;
  }

  {
    std::lock_guard<std::mutex> lock(m_tables_being_loaded_mutex);
    m_tables_being_loaded.emplace(partition_key(schema, table, partition),
                                  file->file_size());
  }

  log_debug("Scheduling chunk for table %s (%s) - worker%zi",
            format_table(schema, table, partition, chunk_index).c_str(),
            file->full_path().masked().c_str(), worker->id());

  worker->load_chunk_file(schema, table, partition, std::move(file),
                          chunk_index, size, options, resuming, bytes_to_skip);

  return true;
}

size_t Dump_loader::handle_worker_events(
    const std::function<bool(Worker *)> &schedule_next) {
  const auto to_string = [](Worker_event::Event event) {
    switch (event) {
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
    }
    return "";
  };

  std::list<Worker *> idle_workers;
  while (idle_workers.size() < m_workers.size()) {
    Worker_event event;

    // Wait for events from workers, but update progress and check for ^C
    // every now and then
    for (;;) {
      event = m_worker_events.try_pop(1000);
      if (event.worker) break;
    }

    log_debug2("Got event %s from worker %zi", to_string(event.event),
               event.worker->id());

    switch (event.event) {
      case Worker_event::LOAD_START: {
        auto task = static_cast<Worker::Load_chunk_task *>(
            event.worker->current_task());

        on_chunk_load_start(task->schema(), task->table(), task->partition(),
                            task->chunk_index());
        break;
      }

      case Worker_event::LOAD_END: {
        auto task = static_cast<Worker::Load_chunk_task *>(
            event.worker->current_task());

        on_chunk_load_end(task->schema(), task->table(), task->partition(),
                          task->chunk_index(), task->bytes_loaded,
                          task->raw_bytes_loaded);
        break;
      }

      case Worker_event::LOAD_SUBCHUNK_START: {
        const auto task = static_cast<Worker::Load_chunk_task *>(
            event.worker->current_task());

        on_subchunk_load_start(task->schema(), task->table(), task->partition(),
                               task->chunk_index(),
                               event.details->get_uint("subchunk"));
        break;
      }

      case Worker_event::LOAD_SUBCHUNK_END: {
        const auto task = static_cast<Worker::Load_chunk_task *>(
            event.worker->current_task());

        on_subchunk_load_end(task->schema(), task->table(), task->partition(),
                             task->chunk_index(),
                             event.details->get_uint("subchunk"),
                             event.details->get_uint("bytes"));
        break;
      }

      case Worker_event::SCHEMA_DDL_START: {
        auto task = static_cast<Worker::Schema_ddl_task *>(
            event.worker->current_task());

        on_schema_ddl_start(task->schema());
        break;
      }

      case Worker_event::SCHEMA_DDL_END: {
        auto task = static_cast<Worker::Schema_ddl_task *>(
            event.worker->current_task());

        on_schema_ddl_end(task->schema());
        break;
      }

      case Worker_event::TABLE_DDL_START: {
        auto task =
            static_cast<Worker::Table_ddl_task *>(event.worker->current_task());

        on_table_ddl_start(task->schema(), task->table(), task->placeholder());
        break;
      }

      case Worker_event::TABLE_DDL_END: {
        auto task =
            static_cast<Worker::Table_ddl_task *>(event.worker->current_task());

        on_table_ddl_end(task->schema(), task->table(), task->placeholder(),
                         task->steal_deferred_indexes());
        break;
      }

      case Worker_event::INDEX_START:
      case Worker_event::INDEX_END:
      case Worker_event::ANALYZE_START:
      case Worker_event::ANALYZE_END:
        break;

      case Worker_event::READY:
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
    }

    // schedule more work if the worker became free
    if (event.event == Worker_event::READY) {
      // no more work to do
      if (m_worker_interrupt || !schedule_next(event.worker)) {
        idle_workers.push_back(event.worker);
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

bool Dump_loader::schedule_next_task(Worker *worker) {
  if (!handle_table_data(worker)) {
    std::string schema;
    std::string table;

    if (m_options.load_deferred_indexes()) {
      setup_create_indexes_progress();

      const auto load_finished = [this](const std::vector<std::string> &keys) {
        std::lock_guard<std::mutex> lock(m_tables_being_loaded_mutex);
        for (const auto &key : keys) {
          if (m_tables_being_loaded.find(key) != m_tables_being_loaded.end()) {
            return false;
          }
        }

        return true;
      };

      std::vector<std::string> *indexes = nullptr;

      if (m_dump->next_deferred_index(&schema, &table, &indexes,
                                      load_finished)) {
        assert(indexes != nullptr);
        ++m_indexes_to_recreate;
        worker->recreate_indexes(schema, table, *indexes);
        return true;
      } else if (Dump_reader::Status::COMPLETE == m_dump->status()) {
        m_all_index_tasks_scheduled = true;
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
          worker->analyze_table(schema, table, histograms);
          return true;
        }
      } else if (Dump_reader::Status::COMPLETE == m_dump->status()) {
        m_all_analyze_tasks_scheduled = true;
      }
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
    m_worker_hard_interrupt = true;
    current_console()->print_info(
        "^C -- Aborting active transactions. This may take a while...");
  }
}

void Dump_loader::run() {
  m_progress_thread.start();

  open_dump();

  spawn_workers();
  {
    shcore::on_leave_scope cleanup([this]() {
      join_workers();

      m_progress_thread.finish();
    });

    execute_tasks();
  }

  show_summary();

  if (m_worker_interrupt && !m_abort) {
    // If interrupted by the user and not by a fatal error
    throw std::runtime_error("Aborted");
  }

  for (const auto &e : m_thread_exceptions) {
    if (e) {
      throw std::runtime_error("Error loading dump");
    }
  }
}

void Dump_loader::show_summary() {
  using namespace mysqlshdk::utils;

  const auto console = current_console();

  if (m_num_rows_loaded == 0) {
    if (m_resuming)
      console->print_info("There was no remaining data left to be loaded.");
    else
      console->print_info("No data loaded.");
  } else {
    const auto seconds = m_progress_thread.duration().seconds();
    const auto load_seconds = m_load_data_stage->duration().seconds();

    console->print_info(shcore::str_format(
        "%zi chunks (%s, %s) for %zi tables in %zi schemas were "
        "loaded in %s (avg throughput %s)",
        m_num_chunks_loaded.load(),
        format_items("rows", "rows", m_num_rows_loaded.load()).c_str(),
        format_bytes(m_num_bytes_loaded.load()).c_str(),
        m_unique_tables_loaded.size(), m_dump->schemas().size(),
        format_seconds(seconds, false).c_str(),
        format_throughput_bytes(
            m_num_bytes_loaded.load() - m_num_bytes_previously_loaded,
            load_seconds)
            .c_str()));
  }
  if (m_num_errors > 0) {
    console->print_info(shcore::str_format(
        "%zi errors and %zi warnings messages were reported during the load.",
        m_num_errors.load(), m_num_warnings.load()));
  } else {
    console->print_info(shcore::str_format(
        "%zi warnings were reported during the load.", m_num_warnings.load()));
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
    throw std::runtime_error("Unsupported dump version");
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
    throw std::runtime_error("Unsupported dump capabilities");
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
      throw std::runtime_error("Incomplete dump");
    }
  }

  m_dump->validate_options();

  m_dump->show_metadata();

  // Pick charset
  if (m_character_set.empty()) {
    m_character_set = m_dump->default_character_set();
  }
}

void Dump_loader::check_server_version() {
  const auto console = current_console();
  const auto &target_server = m_options.target_server_version();
  const auto mds = m_options.is_mds();
  mysqlshdk::mysql::Instance session(m_options.base_session());

  std::string msg = "Target is MySQL " + target_server.get_full();
  if (mds) msg += " (MySQL Database Service)";
  msg +=
      ". Dump was produced from MySQL " + m_dump->server_version().get_full();

  console->print_info(msg);

  if (target_server < Version(5, 7, 0)) {
    throw std::runtime_error(
        "Loading dumps is only supported in MySQL 5.7 or newer");
  }

  if (mds && !m_dump->mds_compatibility()) {
    msg =
        "Destination is a MySQL Database Service instance but the dump was "
        "produced without the compatibility option. ";

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

      throw std::runtime_error("Dump is not MDS compatible");
    }
  }

  if (target_server.get_major() != m_dump->server_version().get_major()) {
    if (target_server.get_major() < m_dump->server_version().get_major())
      msg =
          "Destination MySQL version is older than the one where the dump "
          "was created.";
    else
      msg =
          "Destination MySQL version is newer than the one where the dump "
          "was created.";
    msg +=
        " Loading dumps from different major MySQL versions is "
        "not fully supported and may not work.";
    if (m_options.ignore_version()) {
      msg += " The 'ignoreVersion' option is enabled, so loading anyway.";
      console->print_warning(msg);
    } else {
      msg += " Enable the 'ignoreVersion' option to load anyway.";
      console->print_error(msg);
      throw std::runtime_error("MySQL version mismatch");
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
    if (group_replication_running)
      throw std::runtime_error(
          "updateGtidSet option cannot be used on server with group "
          "replication running");

    if (target_server < Version(8, 0, 0)) {
      if (m_options.update_gtid_set() ==
          Load_dump_options::Update_gtid_set::APPEND)
        throw std::runtime_error(
            "Target MySQL server does not support updateGtidSet:'append'.");

      if (!m_options.skip_binlog())
        throw std::runtime_error(
            "updateGtidSet option on MySQL 5.7 target server can only be "
            "used if skipBinlog option is enabled.");

      if (!session.queryf_one_int(0, 0,
                                  "select @@global.gtid_executed = '' and "
                                  "@@global.gtid_purged = ''"))
        throw std::runtime_error(
            "updateGtidSet:'replace' on target server version can only be "
            "used if GTID_PURGED and GTID_EXECUTED are empty, but they’re "
            "not.");
    } else {
      const char *g = m_dump->gtid_executed().c_str();
      if (m_options.update_gtid_set() ==
          Load_dump_options::Update_gtid_set::REPLACE) {
        if (!session.queryf_one_int(
                0, 0,
                "select GTID_SUBTRACT(?, "
                "GTID_SUBTRACT(@@global.gtid_executed, "
                "@@global.gtid_purged)) = gtid_subtract(?, '')",
                g, g))
          throw std::runtime_error(
              "updateGtidSet:'replace' can only be used if "
              "gtid_subtract(gtid_executed,gtid_purged) "
              "on target server does not intersects with dumped gtid set.");
        if (!session.queryf_one_int(
                0, 0, "select GTID_SUBSET(@@global.gtid_purged, ?);", g))
          throw std::runtime_error(
              "updateGtidSet:'replace' can only be used if dumped gtid set "
              "is a superset of the current value of gtid_purged on target "
              "server");
      } else if (!session.queryf_one_int(
                     0, 0,
                     "select GTID_SUBTRACT(@@global.gtid_executed, ?) = "
                     "@@global.gtid_executed",
                     g)) {
        throw std::runtime_error(
            "updateGtidSet:'append' can only be used if gtid_executed on "
            "target server does not intersects with dumped gtid set.");
      }
    }
  }

  if (should_create_pks() && target_server < Version(8, 0, 24)) {
    throw std::runtime_error(
        "The 'createInvisiblePKs' option requires server 8.0.24 or newer.");
  }
}

void Dump_loader::check_tables_without_primary_key() {
  if (!m_options.load_ddl()) {
    return;
  }

  if (m_options.is_mds() && m_dump->has_tables_without_pk()) {
    std::string msg =
        "The dump contains tables without Primary Keys and it is loaded with "
        "the 'createInvisiblePKs' option set to ";

    if (should_create_pks()) {
      msg +=
          "true, Inbound Replication into an MySQL Database Service instance "
          "with High Availability (at the time of the release of MySQL Shell "
          "8.0.24) cannot be used with this dump.";
    } else {
      msg +=
          "false, this dump cannot be loaded into an MySQL Database Service "
          "instance with High Availability.";
    }

    current_console()->print_warning(msg);
  }

  if (m_options.target_server_version() < Version(8, 0, 13) ||
      should_create_pks()) {
    return;
  }

  if (m_session->query("show variables like 'sql_require_primary_key';")
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
    throw shcore::Exception::runtime_error(
        "sql_require_primary_key enabled at destination server");
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
    mysqlshdk::db::ISession *session, const std::string &schema,
    const std::vector<std::string> &names, const std::string &query_prefix) {
  std::string set = shcore::str_join(names, ",", [](const std::string &s) {
    return shcore::quote_sql_string(s);
  });
  set = set.empty() ? "" : "(" + set + ")";

  if (!set.empty())
    return session->queryf(query_prefix + set, schema);
  else
    return {};
}

}  // namespace

bool Dump_loader::report_duplicates(const std::string &what,
                                    const std::string &schema,
                                    mysqlshdk::db::IResult *result) {
  bool has_duplicates = false;

  while (auto row = result->fetch_one()) {
    std::string name = row->get_string(0);

    if (m_options.ignore_existing_objects())
      current_console()->print_note("Schema `" + schema +
                                    "` already contains a " + what + " named " +
                                    name);
    else
      current_console()->print_error("Schema `" + schema +
                                     "` already contains a " + what +
                                     " named " + name);
    has_duplicates = true;
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
      if (m_options.include_user(a))
        accounts.emplace(shcore::str_lower(
            shcore::str_format("'%s'@'%s'", a.user.c_str(), a.host.c_str())));
    }

    auto result = m_session->query(
        "SELECT DISTINCT grantee FROM information_schema.user_privileges");
    for (auto row = result->fetch_one(); row; row = result->fetch_one()) {
      auto grantee = row->get_string(0);
      if (accounts.count(shcore::str_lower(grantee))) {
        if (m_options.ignore_existing_objects())
          current_console()->print_note("Account " + grantee +
                                        " already exists");
        else
          current_console()->print_error("Account " + grantee +
                                         " already exists");
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

  auto result = m_session->query(
      "SELECT schema_name FROM information_schema.schemata"
      " WHERE schema_name in (" +
      set + ")");
  std::vector<std::string> dup_schemas = fetch_names(result.get());

  for (const auto &schema : dup_schemas) {
    std::vector<std::string> tables;
    std::vector<std::string> views;
    std::vector<std::string> triggers;
    std::vector<std::string> functions;
    std::vector<std::string> procedures;
    std::vector<std::string> events;

    if (!m_dump->schema_objects(schema, &tables, &views, &triggers, &functions,
                                &procedures, &events))
      continue;

    result = query_names(m_session.get(), schema, tables,
                         "SELECT table_name FROM information_schema.tables"
                         " WHERE table_schema = ? AND table_name in ");
    if (result)
      has_duplicates |= report_duplicates("table", schema, result.get());

    result = query_names(m_session.get(), schema, views,
                         "SELECT table_name FROM information_schema.views"
                         " WHERE table_schema = ? AND table_name in ");
    if (result)
      has_duplicates |= report_duplicates("view", schema, result.get());

    result = query_names(m_session.get(), schema, triggers,
                         "SELECT trigger_name FROM information_schema.triggers"
                         " WHERE trigger_schema = ? AND trigger_name in ");
    if (result)
      has_duplicates |= report_duplicates("trigger", schema, result.get());

    result =
        query_names(m_session.get(), schema, functions,
                    "SELECT routine_name FROM information_schema.routines"
                    " WHERE routine_schema = ? AND routine_type = 'FUNCTION'"
                    " AND routine_name in ");
    if (result)
      has_duplicates |= report_duplicates("function", schema, result.get());

    result =
        query_names(m_session.get(), schema, procedures,
                    "SELECT routine_name FROM information_schema.routines"
                    " WHERE routine_schema = ? AND routine_type = 'PROCEDURE'"
                    " AND routine_name in ");
    if (result)
      has_duplicates |= report_duplicates("procedure", schema, result.get());

    result = query_names(m_session.get(), schema, events,
                         "SELECT event_name FROM information_schema.events"
                         " WHERE event_schema = ? AND event_name in ");
    if (result)
      has_duplicates |= report_duplicates("event", schema, result.get());
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
      throw std::runtime_error(
          "Duplicate objects found in destination database");
    }
  }
}

void Dump_loader::setup_progress_file(bool *out_is_resuming) {
  auto console = current_console();

  m_load_log = std::make_unique<Load_progress_log>();

  if (m_options.progress_file().is_null() ||
      !m_options.progress_file()->empty()) {
    auto progress_file = m_dump->create_progress_file_handle();
    const auto path = progress_file->full_path().masked();
    bool rewrite_on_flush = m_options.oci_options() ? true : false;

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

        log_info("Resuming load, last loaded %s bytes",
                 std::to_string(progress.bytes_completed).c_str());
        // Recall the partial progress that was made before
        m_num_bytes_previously_loaded = progress.bytes_completed;
        m_num_bytes_loaded.store(progress.bytes_completed);
        m_num_raw_bytes_loaded.store(progress.raw_bytes_completed);
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

void Dump_loader::execute_threaded(
    const std::function<bool(Worker *)> &schedule_next) {
  do {
    // handle events from workers and schedule more chunks when a worker
    // becomes available
    size_t num_idle_workers = handle_worker_events(schedule_next);

    if (num_idle_workers == m_workers.size()) {
      // make sure that there's really no more work. schedule_work() is
      // supposed to just return false without doing anything. If it does
      // something (and returns true), then we have a bug.
      assert(!schedule_next(&m_workers.front()) || m_worker_interrupt);
      break;
    }
  } while (!m_worker_interrupt);
}

void Dump_loader::execute_table_ddl_tasks() {
  m_ddl_executed = 0;
  uint64_t ddl_to_execute = 0;
  bool all_tasks_scheduled = false;

  dump::Progress_thread::Progress_config config;
  config.current = [this]() -> uint64_t { return m_ddl_executed; };
  config.total = [&ddl_to_execute]() { return ddl_to_execute; };
  config.is_total_known = [&all_tasks_scheduled]() {
    return all_tasks_scheduled;
  };

  const auto stage =
      m_progress_thread.start_stage("Executing DDL", std::move(config));
  shcore::on_leave_scope finish_stage([stage]() { stage->finish(); });

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

  const auto handle_ddl_files = [this, pool, &worker_tasks, &ddl_to_execute](
                                    const std::string &s,
                                    std::list<Dump_reader::Name_and_file> *list,
                                    bool placeholder,
                                    Load_progress_log::Status schema_status) {
    if (should_fetch_table_ddl(placeholder)) {
      for (auto &item : *list) {
        if (item.second) {
          const auto status = placeholder
                                  ? schema_status
                                  : m_load_log->table_ddl_status(s, item.first);

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
              [s, table = item.first, placeholder, &worker_tasks,
               status](std::string &&data) {
                worker_tasks.push(std::make_unique<Worker::Table_ddl_task>(
                    s, table, std::move(data), placeholder, status));
              });
        }
      }
    }
  };

  log_debug("Begin loading table DDL");

  pool->start_threads();
  const auto &pool_status = pool->process_async();

  while (!m_worker_interrupt) {
    // fetch next schema and process it
    if (!m_dump->next_schema_and_tables(&schema, &tables, &view_placeholders)) {
      break;
    }

    schema_load_status = m_load_log->schema_ddl_status(schema);

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
  auto pending_tasks = ddl_to_execute;
  pool->tasks_done();

  {
    std::list<std::unique_ptr<Worker::Task>> schema_tasks;
    std::list<std::unique_ptr<Worker::Task>> table_tasks;

    execute_threaded([this, &pool_status, &worker_tasks, &pending_tasks,
                      &schema_tasks, &table_tasks](Worker *worker) {
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
          work = worker_tasks.try_pop(1);

          if (!work) {
            break;
          }

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
          worker->schedule(std::move(work));
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
  uint64_t ddl_to_execute = 0;
  bool all_tasks_scheduled = false;

  dump::Progress_thread::Progress_config config;
  config.current = [this]() -> uint64_t { return m_ddl_executed; };
  config.total = [&ddl_to_execute]() { return ddl_to_execute; };
  config.is_total_known = [&all_tasks_scheduled]() {
    return all_tasks_scheduled;
  };

  const auto stage =
      m_progress_thread.start_stage("Executing view DDL", std::move(config));
  shcore::on_leave_scope finish_stage([stage]() { stage->finish(); });

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

    schema_load_status = m_load_log->schema_ddl_status(schema);

    if (schema_load_status != Load_progress_log::DONE) {
      if (views.empty()) {
        // there are no views, mark schema as ready
        m_load_log->end_schema_ddl(schema);
      } else {
        ddl_to_execute += views.size();
        views_per_schema[schema] = views.size();

        for (auto &item : views) {
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
                  m_session->executef("use !", schema.c_str());

                  // execute sql
                  execute_script(
                      m_session, script,
                      shcore::str_format(
                          "Error executing DDL script for view `%s`.`%s`",
                          schema.c_str(), view.c_str()),
                      m_default_sql_transforms);
                }

                ++m_ddl_executed;

                if (0 == --views_per_schema[schema]) {
                  m_load_log->end_schema_ddl(schema);
                }

                if (m_worker_interrupt) {
                  pool->terminate();
                }
              });
        }
      }
    }
  }

  all_tasks_scheduled = true;
  pool->tasks_done();
  pool->process();

  log_debug("End loading view DDL");
}

void Dump_loader::execute_tasks() {
  auto console = current_console();

  if (m_options.dry_run())
    console->print_info("dryRun enabled, no changes will be made.");

  check_server_version();

  m_session = create_session();

  setup_progress_file(&m_resuming);

  // the 1st potentially slow operation, as many errors should be detected
  // before this as possible
  m_dump->rescan(&m_progress_thread);

  handle_schema_option();

  if (!m_resuming && m_options.load_ddl()) check_existing_objects();

  check_tables_without_primary_key();

  size_t num_idle_workers = 0;

  do {
    if (m_dump->status() != Dump_reader::Status::COMPLETE) {
      wait_for_more_data();
    }

    if (!m_init_done) {
      // process dump metadata first
      on_dump_begin();

      // NOTE: this assumes that all DDL files are already present

      if (m_options.load_ddl() || m_options.load_deferred_indexes()) {
        // exec DDL for all tables in parallel (fetching DDL for
        // thousands of tables from remote storage can be slow)
        if (!m_worker_interrupt) execute_table_ddl_tasks();

        // exec DDL for all views after all tables are created
        if (!m_worker_interrupt && m_options.load_ddl())
          execute_view_ddl_tasks();
      }

      m_init_done = true;

      if (!m_worker_interrupt) {
        setup_load_data_progress();
      }
    }

    m_total_tables_with_data = m_dump->tables_with_data();

    // handle events from workers and schedule more chunks when a worker
    // becomes available
    num_idle_workers = handle_worker_events(
        [this](Worker *worker) { return schedule_next_task(worker); });

    // If the whole dump is already available and there's no more data to be
    // loaded and all workers are idle (done loading), then we're done
    if (m_dump->status() == Dump_reader::Status::COMPLETE &&
        !m_dump->data_available() &&
        (m_options.analyze_tables() ==
             Load_dump_options::Analyze_table_mode::OFF ||
         !m_dump->work_available()) &&
        num_idle_workers == m_workers.size()) {
      break;
    }
  } while (!m_worker_interrupt);

  if (!m_worker_interrupt) {
    on_dump_end();
    m_load_log->cleanup();
  }

  log_debug("Import done");
}

bool Dump_loader::wait_for_more_data() {
  const auto start_time = std::chrono::steady_clock::now();
  bool waited = false;
  auto console = current_console();
  bool was_ready = m_dump->ready();

  // if there are still idle workers, check if there's more that was dumped
  while (m_dump->status() != Dump_reader::Status::COMPLETE &&
         !m_worker_interrupt) {
    m_dump->rescan();

    if (m_dump->status() == Dump_reader::Status::DUMPING) {
      if (m_dump->data_available() || (!was_ready && m_dump->ready())) {
        log_debug("Dump data available");
        return true;
      }

      if (m_options.dump_wait_timeout_ms() > 0) {
        const auto current_time = std::chrono::steady_clock::now();
        const auto time_diff = std::chrono::duration_cast<std::chrono::seconds>(
                                   current_time - start_time)
                                   .count();
        if (static_cast<uint64_t>(time_diff * 1000) >=
            m_options.dump_wait_timeout_ms()) {
          console->print_warning(
              "Timeout while waiting for dump to finish. Imported data "
              "may be incomplete.");

          throw dump_wait_timeout("Dump timeout");
        }
      } else {
        // Dump isn't complete yet, but we're not waiting for it
        break;
      }

      if (!waited) {
        console->print_status("Waiting for more data to become available...");
      }
      waited = true;
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
  }

  return false;
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
}

void Dump_loader::join_workers() {
  log_debug("Waiting on worker threads...");
  for (auto &w : m_workers) w.stop();

  for (auto &t : m_worker_threads)
    if (t.joinable()) t.join();
  log_debug("All worker threads stopped");
}

void Dump_loader::clear_worker(Worker *worker) {
  const auto wid = worker->id();
  m_worker_threads[wid].join();
  m_workers.remove_if([wid](const Worker &w) { return w.id() == wid; });
}

void Dump_loader::post_worker_event(Worker *worker, Worker_event::Event event,
                                    shcore::Dictionary_t &&details) {
  m_worker_events.push(Worker_event{event, worker, std::move(details)});
}

void Dump_loader::on_schema_ddl_start(const std::string &schema) {
  m_load_log->start_schema_ddl(schema);
}

void Dump_loader::on_schema_ddl_end(const std::string &) {}

void Dump_loader::on_table_ddl_start(const std::string &schema,
                                     const std::string &table,
                                     bool placeholder) {
  if (!placeholder) m_load_log->start_table_ddl(schema, table);
}

void Dump_loader::on_table_ddl_end(
    const std::string &schema, const std::string &table, bool placeholder,
    std::unique_ptr<std::vector<std::string>> deferred_indexes) {
  if (!placeholder) {
    m_load_log->end_table_ddl(schema, table);

    if (deferred_indexes && !deferred_indexes->empty())
      m_dump->add_deferred_indexes(schema, table, std::move(*deferred_indexes));
  }

  on_ddl_done_for_schema(schema);
}

void Dump_loader::on_chunk_load_start(const std::string &schema,
                                      const std::string &table,
                                      const std::string &partition,
                                      ssize_t index) {
  m_load_log->start_table_chunk(schema, table, partition, index);
}

void Dump_loader::on_chunk_load_end(const std::string &schema,
                                    const std::string &table,
                                    const std::string &partition, ssize_t index,
                                    size_t bytes_loaded,
                                    size_t raw_bytes_loaded) {
  m_load_log->end_table_chunk(schema, table, partition, index, bytes_loaded,
                              raw_bytes_loaded);

  m_unique_tables_loaded.insert(partition_key(schema, table, partition));

  log_debug("Ended loading chunk %s (%s, %s)",
            format_table(schema, table, partition, index).c_str(),
            std::to_string(bytes_loaded).c_str(),
            std::to_string(raw_bytes_loaded).c_str());
}

void Dump_loader::on_subchunk_load_start(const std::string &schema,
                                         const std::string &table,
                                         const std::string &partition,
                                         ssize_t index, uint64_t subchunk) {
  m_load_log->start_table_subchunk(schema, table, partition, index, subchunk);
}

void Dump_loader::on_subchunk_load_end(const std::string &schema,
                                       const std::string &table,
                                       const std::string &partition,
                                       ssize_t index, uint64_t subchunk,
                                       uint64_t bytes) {
  m_load_log->end_table_subchunk(schema, table, partition, index, subchunk,
                                 bytes);
}

void Dump_loader::Sql_transform::add_strip_removed_sql_modes() {
  // Remove NO_AUTO_CREATE_USER from sql_mode, which doesn't exist in 8.0 but
  // does in 5.7

  std::regex re(R"*((\/\*![0-9]+\s+)?(SET\s+sql_mode\s*=\s*')(.*)('.*))*",
                std::regex::icase);

  add([re](const std::string &sql, std::string *out_new_sql) {
    std::smatch m;
    if (std::regex_match(sql, m, re)) {
      auto modes = shcore::str_split(m[3].str(), ",");
      std::string new_modes;
      for (const auto &mode : modes) {
        if (mode != "NO_AUTO_CREATE_USER") new_modes += mode + ",";
      }
      if (!new_modes.empty()) new_modes.pop_back();  // strip ,

      *out_new_sql = m[1].str() + m[2].str() + new_modes + m[4].str();
    } else {
      *out_new_sql = sql;
    }
  });
}

void Dump_loader::handle_schema_option() {
  if (!m_options.target_schema().empty()) {
    m_dump->replace_target_schema(m_options.target_schema());
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
  config.initial = [this]() { return m_num_bytes_previously_loaded; };
  config.current = [this]() {
    const uint64_t current = m_num_bytes_loaded;
    const auto total = m_dump->filtered_data_size();

    if (Dump_reader::Status::COMPLETE == m_dump->status() && current >= total) {
      // this callback can be called before m_load_data_stage is assigned to
      if (m_load_data_stage) {
        // finish the stage when all data is loaded
        m_load_data_stage->finish(false);
      }
    }

    return current;
  };
  config.total = [this]() { return m_dump->filtered_data_size(); };

  config.left_label = [this]() {
    std::string label;

    if (m_dump->status() != Dump_reader::Status::COMPLETE) {
      label += "Dump still in progress, " +
               mysqlshdk::utils::format_bytes(m_dump->dump_size()) +
               " ready (compr.) - ";
    }

    const auto threads_loading = m_num_threads_loading.load();

    if (threads_loading) {
      label += std::to_string(threads_loading) + " thds loading - ";
    }

    const auto threads_indexing = m_num_threads_recreating_indexes.load();

    if (threads_indexing) {
      label += std::to_string(threads_indexing) + " thds indexing - ";
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

  config.right_label = [this]() {
    return shcore::str_format(", %zu / %zu tables done",
                              m_unique_tables_loaded.size(),
                              m_total_tables_with_data);
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
  m_indexes_to_recreate = 0;
  m_all_index_tasks_scheduled = false;

  dump::Progress_thread::Progress_config config;
  config.current = [this]() -> uint64_t { return m_indexes_recreated; };
  config.total = [this]() { return m_indexes_to_recreate; };
  config.is_total_known = [this]() { return m_all_index_tasks_scheduled; };

  m_create_indexes_stage =
      m_progress_thread.start_stage("Recreating indexes", std::move(config));
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
      m_progress_thread.start_stage("Analyzing tables", std::move(config));
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

}  // namespace mysqlsh
