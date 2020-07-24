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

#include "modules/util/load/dump_loader.h"
#include <mysqld_error.h>
#include <algorithm>
#include <sstream>
#include <string>
#include <utility>
#include <vector>
#include "modules/mod_utils.h"
#include "modules/util/dump/compatibility.h"
#include "modules/util/dump/console_with_progress.h"
#include "modules/util/dump/schema_dumper.h"
#include "modules/util/import_table/load_data.h"
#include "modules/util/load/load_progress_log.h"
#include "mysqlshdk/include/scripting/naming_style.h"
#include "mysqlshdk/include/shellcore/console.h"
#include "mysqlshdk/include/shellcore/shell_init.h"
#include "mysqlshdk/include/shellcore/shell_options.h"
#include "mysqlshdk/libs/mysql/script.h"
#include "mysqlshdk/libs/utils/strformat.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_lexing.h"
#include "mysqlshdk/libs/utils/utils_mysql_parsing.h"
#include "mysqlshdk/libs/utils/version.h"

namespace mysqlsh {

// how many seconds the server should wait to finish reading data from client
// basically how long it may take for a block of data to be read from its source
// (download + decompression)
static constexpr const int k_mysql_server_net_read_timeout = 30 * 60;

// the version of the dump we support in this code
static constexpr const int k_supported_dump_version_major = 1;
static constexpr const int k_supported_dump_version_minor = 0;

class dump_wait_timeout : public std::runtime_error {
 public:
  explicit dump_wait_timeout(const char *w) : std::runtime_error(w) {}
};

namespace {

bool histograms_supported(const mysqlshdk::utils::Version &version) {
  return version > mysqlshdk::utils::Version(8, 0, 0);
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
}  // namespace

bool Dump_loader::Worker::Load_chunk_task::execute(
    const std::shared_ptr<mysqlshdk::db::mysql::Session> &session,
    Worker *worker, Dump_loader *loader) {
  std::string path = m_file->full_path();

  log_debug("worker%zu will load chunk %s for table `%s`.`%s`", id(),
            path.c_str(), schema().c_str(), table().c_str());

  loader->post_worker_event(worker, Worker_event::LOAD_START);

  // do work

  try {
    if (!loader->m_options.dry_run()) {
      // load the data
      load(session, loader);
    }
  } catch (const std::exception &e) {
    if (!loader->m_thread_exceptions[id()]) {
      current_console()->print_error(shcore::str_format(
          "[Worker%03zu] While loading %s: %s", id(), path.c_str(), e.what()));
      loader->m_thread_exceptions[id()] = std::current_exception();
    }
    loader->post_worker_event(worker, Worker_event::FATAL_ERROR);
    return false;
  }

  log_debug("worker%zu done", id());

  // signal for more work
  loader->post_worker_event(worker, Worker_event::LOAD_END);
  return true;
}

void Dump_loader::Worker::Load_chunk_task::load(
    const std::shared_ptr<mysqlshdk::db::mysql::Session> &session,
    Dump_loader *loader) {
  import_table::Import_table_options import_options(m_options);

  // replace duplicate rows by default
  import_options.set_replace_duplicates(true);

  import_options.base_session(session);

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
    }
  }

  import_table::Load_data_worker op(
      import_options, id(), loader->m_progress.get(), &loader->m_output_mutex,
      &loader->m_num_bytes_loaded, &loader->m_worker_hard_interrupt, nullptr,
      &loader->m_thread_exceptions, &stats);

  loader->m_num_threads_loading++;
  loader->update_progress();

  shcore::on_leave_scope cleanup([this, loader]() {
    std::lock_guard<std::mutex> lock(loader->m_tables_being_loaded_mutex);
    auto key = schema_table_key(schema(), table());
    auto it = loader->m_tables_being_loaded.find(key);
    while (it != loader->m_tables_being_loaded.end() && it->first == key) {
      if (it->second == raw_bytes_loaded) {
        loader->m_tables_being_loaded.erase(it);
        break;
      }
      ++it;
    }

    loader->m_num_threads_loading--;
  });

  try {
    op.execute(session, std::move(m_file));

    if (loader->m_thread_exceptions[id()])
      std::rethrow_exception(loader->m_thread_exceptions[id()]);
  } catch (...) {
    loader->m_num_errors += 1;
    throw;
  }

  bytes_loaded = stats.total_bytes;
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
    console->print_status(shcore::str_format(
        "Analyzing table `%s`.`%s`", schema().c_str(), table().c_str()));
  else
    console->print_status(
        shcore::str_format("Updating histogram for table `%s`.`%s`",
                           schema().c_str(), table().c_str()));

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
    console->print_error(
        shcore::str_format("[Worker%03zu] While analyzing table `%s`.`%s`: %s",
                           id(), schema().c_str(), table().c_str(), e.what()));
    loader->m_thread_exceptions[id()] = std::current_exception();

    loader->post_worker_event(worker, Worker_event::FATAL_ERROR);
    return false;
  }

  log_debug("worker%zu done", id());

  // signal for more work
  loader->post_worker_event(worker, Worker_event::ANALYZE_END);
  return true;
}

bool Dump_loader::Worker::Index_recreation_task::execute(
    const std::shared_ptr<mysqlshdk::db::mysql::Session> &session,
    Worker *worker, Dump_loader *loader) {
  log_debug("worker%zu will recreate indexes for table `%s`.`%s`", id(),
            schema().c_str(), table().c_str());

  auto console = current_console();

  if (!m_queries.empty())
    console->print_status(shcore::str_format(
        "Recreating indexes for `%s`.`%s`", schema().c_str(), table().c_str()));

  loader->post_worker_event(worker, Worker_event::INDEX_START);

  // do work
  if (!loader->m_options.dry_run()) {
    loader->m_num_threads_recreating_indexes++;
    loader->update_progress();
    shcore::on_leave_scope cleanup(
        [loader]() { loader->m_num_threads_recreating_indexes--; });

    try {
      int retries = 0;
      session->execute("SET unique_checks = 0");
      for (size_t i = 0; i < m_queries.size(); i++) {
        try {
          session->execute(m_queries[i]);
        } catch (const shcore::Error &e) {
          // Deadlocks and duplicate key errors should not happen but if they do
          // they can be ignored at least for a while
          if (e.code() == ER_LOCK_DEADLOCK && retries < 20) {
            console->print_note(
                "Deadlock detected when recreating indexes for table `" +
                m_table + "`, will retry after " + std::to_string(++retries) +
                " seconds");
            shcore::sleep_ms(retries * 1000);
            --i;
          } else if (e.code() == ER_DUP_KEYNAME) {
            console->print_note("Index already existed for query: " +
                                m_queries[i]);
          } else {
            throw;
          }
        }
      }
    } catch (const std::exception &e) {
      console->print_error(shcore::str_format(
          "[Worker%03zu] While recreating indexes for table `%s`.`%s`: %s",
          id(), schema().c_str(), table().c_str(), e.what()));
      loader->m_thread_exceptions[id()] = std::current_exception();

      loader->post_worker_event(worker, Worker_event::FATAL_ERROR);
      return false;
    }
  }

  log_debug("worker%zu done", id());

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
    m_owner->post_worker_event(this, Worker_event::FATAL_ERROR);
    return;
  }

  m_owner->post_worker_event(this, Worker_event::READY);

  for (;;) {
    // wait for signal that there's work to do... false means stop worker
    bool work = m_work_ready.pop();
    if (!work || m_owner->m_worker_interrupt) {
      m_owner->post_worker_event(this, Worker_event::EXIT);
      break;
    }

    if (!m_task->execute(m_session, this, m_owner)) break;
  }
}

void Dump_loader::Worker::stop() { m_work_ready.push(false); }

void Dump_loader::Worker::connect() {
  m_session = m_owner->create_session(true);
  m_connection_id = m_session->get_connection_id();
}

void Dump_loader::Worker::load_chunk_file(
    const std::string &schema, const std::string &table,
    std::unique_ptr<mysqlshdk::storage::IFile> file, ssize_t chunk_index,
    size_t chunk_size, const shcore::Dictionary_t &options, bool resuming) {
  log_debug("Loading data for `%s`.`%s` (chunk %zi)", schema.c_str(),
            table.c_str(), chunk_index);
  assert(!schema.empty());
  assert(!table.empty());

  m_task = std::make_unique<Load_chunk_task>(
      m_id, schema, table, chunk_index, std::move(file), options, resuming);
  static_cast<Load_chunk_task *>(m_task.get())->raw_bytes_loaded = chunk_size;

  m_work_ready.push(true);
}

void Dump_loader::Worker::recreate_indexes(
    const std::string &schema, const std::string &table,
    const std::vector<std::string> &indexes) {
  log_debug("Recreating indexes for `%s`.`%s`", schema.c_str(), table.c_str());
  assert(!schema.empty());
  assert(!table.empty());

  m_task =
      std::make_unique<Index_recreation_task>(m_id, schema, table, indexes);
  m_work_ready.push(true);
}

void Dump_loader::Worker::analyze_table(
    const std::string &schema, const std::string &table,
    const std::vector<Dump_reader::Histogram> &histograms) {
  log_debug("Analyzing table `%s`.`%s`", schema.c_str(), table.c_str());
  assert(!schema.empty());
  assert(!table.empty());

  m_task =
      std::make_unique<Analyze_table_task>(m_id, schema, table, histograms);

  m_work_ready.push(true);
}
// ----

Dump_loader::Dump_loader(const Load_dump_options &options)
    : m_options(options),
      m_num_threads_loading(0),
      m_num_threads_recreating_indexes(0),
      m_console(std::make_shared<dump::Console_with_progress>(m_progress,
                                                              &m_output_mutex)),
      m_num_rows_loaded(0),
      m_num_bytes_loaded(0),
      m_num_raw_bytes_loaded(0),
      m_num_chunks_loaded(0),
      m_num_warnings(0),
      m_num_errors(0) {
  bool use_json = (mysqlsh::current_shell_options()->get().wrap_json != "off");

  if (m_options.show_progress()) {
    if (use_json) {
      m_progress = std::make_unique<mysqlshdk::textui::Json_progress>();
    } else {
      m_progress = std::make_unique<mysqlshdk::textui::Text_progress>();
    }
  } else {
    m_progress = std::make_unique<mysqlshdk::textui::IProgress>();
  }

  m_progress->hide(true);

  if (m_options.ignore_version()) {
    m_default_sql_transforms.add_strip_removed_sql_modes();
  }
}

Dump_loader::~Dump_loader() {}

std::shared_ptr<mysqlshdk::db::mysql::Session> Dump_loader::create_session(
    bool data_loader) {
  auto session = mysqlshdk::db::mysql::Session::create();

  session->connect(m_options.connection_options());

  // Set timeouts to larger values since worker threads may get stuck
  // downloading data for some time before they have a chance to get back to
  // doing MySQL work.
  session->executef("SET SESSION net_read_timeout = ?",
                    k_mysql_server_net_read_timeout);

  // Disable binlog if requested by user or if target is -cloud
  if (m_options.skip_binlog() && !m_options.is_mds()) {
    try {
      session->execute("SET sql_log_bin=0");
    } catch (const mysqlshdk::db::Error &e) {
      throw shcore::Exception::runtime_error(
          "'SET sql_log_bin=0' failed with error - " + e.format());
    }
  }

  if (!data_loader) {
    session->execute("SET foreign_key_checks = 0");
    session->execute("SET unique_checks = 0");
  }

  // Make sure we don't get affected by user customizations of sql_mode
  session->execute("SET SQL_MODE = 'NO_AUTO_VALUE_ON_ZERO'");

  if (!m_character_set.empty())
    session->executef("SET NAMES ?", m_character_set);

  if (m_dump->tz_utc()) session->execute("SET TIME_ZONE='+00:00'");

  return session;
}

void Dump_loader::on_dump_begin() {
  if (m_options.load_users()) {
    std::string script = m_dump->users_script();

    current_console()->print_status("Executing user accounts SQL...");

    script = Schema_dumper::preprocess_users_script(
        script, [this](const std::string &account) {
          return m_options.include_user(shcore::split_account(account));
        });

    if (!m_options.dry_run())
      execute_script(m_session, script, "While executing user accounts SQL");
  }

  std::string pre_script = m_dump->begin_script();

  current_console()->print_status("Executing common preamble SQL");

  if (!m_options.dry_run())
    execute_script(m_session, pre_script, "While executing preamble SQL");
}

void Dump_loader::on_dump_end() {
  std::string post_script = m_dump->end_script();

  // Execute schema end scripts
  for (const std::string &schema : m_dump->schemas()) {
    on_schema_end(schema);
  }

  current_console()->print_status("Executing common postamble SQL");

  if (!m_options.dry_run())
    execute_script(m_session, post_script, "While executing postamble SQL");

  // Don't do anything gtid related yet
  // if (!m_dump->gtid_executed().empty()) {
  //   print_status("Updating GTID_PURGED");
  //   log_info("Setting GTID_PURGED to %s", m_dump->gtid_executed().c_str());
  //   m_session->executef("SET GLOBAL GTID_PURGED=/*!80000 '+'*/ ?",
  //                       m_dump->gtid_executed());
  // } else {
  //   log_info("gtid_executed not set in dump, skipping handling of
  //   GTID_PURGED");
  // }
}

void Dump_loader::on_schema_end(const std::string &schema) {
  if (m_options.load_indexes() &&
      m_options.defer_table_indexes() !=
          Load_dump_options::Defer_index_mode::OFF) {
    const auto &fks = m_dump->deferred_schema_fks(schema);
    if (!fks.empty()) {
      current_console()->print_status(
          "Recreating FOREIGN KEY constraints for schema " +
          shcore::quote_identifier(schema));
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
        current_console()->print_status("Executing triggers SQL for `" +
                                        schema + "`.`" + it.first + "`");

        it.second->open(mysqlshdk::storage::Mode::READ);
        std::string script = mysqlshdk::storage::read_file(it.second.get());
        it.second->close();

        if (!m_options.dry_run()) {
          m_session->executef("USE !", schema);

          execute_script(m_session, script, "While executing triggers SQL");
        }
      }

      m_load_log->end_triggers_ddl(schema, it.first);
    }
  }
}

void Dump_loader::handle_schema_scripts() {
  std::string schema;
  std::list<Dump_reader::Name_and_file> tables;
  std::list<Dump_reader::Name_and_file> views;

  // create all available schemas
  while (m_dump->next_schema(&schema, &tables, &views)) {
    auto status = m_load_log->schema_ddl_status(schema);

    log_debug("Schema DDL for '%s' (%s)", schema.c_str(),
              to_string(status).c_str());

    if (m_options.load_ddl() || m_options.defer_table_indexes() !=
                                    Load_dump_options::Defer_index_mode::OFF) {
      m_load_log->start_schema_ddl(schema);

      if (status != Load_progress_log::DONE && m_options.load_ddl()) {
        handle_schema(schema,
                      status == mysqlsh::Load_progress_log::INTERRUPTED);
      }

      // make sure the correct schema is used
      switch_schema(schema, Load_progress_log::DONE == status);

      // Process tables of the schema
      handle_tables(schema, tables, false);

      // Process views after all tables have been created
      handle_tables(schema, views, true);

      m_load_log->end_schema_ddl(schema);
    }
  }
}

void Dump_loader::handle_schema(const std::string &schema, bool resuming) {
  log_debug("Fetching schema DDL for %s", schema.c_str());
  const auto script = m_dump->fetch_schema_script(schema);

  if (script.empty()) {
    // no schema SQL or view pre SQL
    return;
  }

  log_debug("Executing schema DDL for %s", schema.c_str());

  try {
    current_console()->print_info((resuming
                                       ? "Re-executing DDL script for schema `"
                                       : "Executing DDL script for schema `") +
                                  schema + "`");
    if (!m_options.dry_run()) {
      // execute sql
      execute_script(
          m_session, script,
          shcore::str_format("While processing schema `%s`", schema.c_str()));
    }
  } catch (const std::exception &e) {
    current_console()->print_error(shcore::str_format(
        "Error processing schema `%s`: %s", schema.c_str(), e.what()));

    if (!m_options.force()) throw;

    m_skip_schemas.insert(schema);
  }
}

void Dump_loader::switch_schema(const std::string &schema, bool load_done) {
  if (!m_options.dry_run()) {
    try {
      m_session->executef("use !", schema.c_str());
    } catch (const std::exception &e) {
      current_console()->print_error(shcore::str_format(
          "Unable to use schema `%s`%s, error message: %s", schema.c_str(),
          load_done
              ? " that according to load status was already created, consider "
                "resetting progress"
              : "",
          e.what()));
      if (!m_options.force()) throw;
      m_skip_schemas.insert(schema);
    }
  }
}

void Dump_loader::handle_tables(const std::string &schema,
                                const std::list<Name_and_file> &tables,
                                bool is_view) {
  for (const auto &it : tables) {
    const std::string &table = it.first;

    const auto status = m_load_log->table_ddl_status(schema, table);

    log_debug("%s DDL for '%s'.'%s' (%s)", is_view ? "View" : "Table",
              schema.c_str(), table.c_str(), to_string(status).c_str());

    if ((m_options.load_ddl() ||
         m_options.defer_table_indexes() !=
             Load_dump_options::Defer_index_mode::OFF) &&
        m_dump->has_ddl(schema, table)) {
      it.second->open(mysqlshdk::storage::Mode::READ);
      auto script = mysqlshdk::storage::read_file(it.second.get());
      it.second->close();

      bool indexes_deferred = false;
      if (!is_view && m_options.defer_table_indexes() !=
                          Load_dump_options::Defer_index_mode::OFF) {
        indexes_deferred =
            handle_indexes(schema, table, &script,
                           m_options.defer_table_indexes() ==
                               Load_dump_options::Defer_index_mode::FULLTEXT,
                           status == Load_progress_log::DONE);
      }

      if (status != Load_progress_log::DONE && m_options.load_ddl()) {
        m_load_log->start_table_ddl(schema, table);

        handle_table(schema, table, script,
                     status == Load_progress_log::INTERRUPTED,
                     indexes_deferred);
        m_load_log->end_table_ddl(schema, table);
      }
    }
  }
}

namespace {
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
        if (shcore::str_caseeq(sit.get_next_token(), "CREATE") &&
            shcore::str_caseeq(sit.get_next_token(), "TABLE")) {
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
}  // namespace

bool Dump_loader::handle_indexes(const std::string &schema,
                                 const std::string &table, std::string *script,
                                 bool fulltext_only, bool check_recreated) {
  auto key =
      shcore::quote_identifier(schema) + "." + shcore::quote_identifier(table);
  auto indexes =
      preprocess_table_script_for_indexes(script, key, fulltext_only);

  if (check_recreated && !indexes.empty()) {
    try {
      auto ct = m_session->query("show create table " + key)
                    ->fetch_one()
                    ->get_string(1);
      auto recreated =
          compatibility::check_create_table_for_indexes(ct, fulltext_only);
      indexes.erase(std::remove_if(indexes.begin(), indexes.end(),
                                   [&recreated](const std::string &i) {
                                     return std::find(recreated.begin(),
                                                      recreated.end(),
                                                      i) != recreated.end();
                                   }),
                    indexes.end());
    } catch (const std::exception &e) {
      current_console()->print_error(
          shcore::str_format("Unable to get status of table %s that "
                             "according to load status "
                             "was already created, consider reseting "
                             "progress, error message: %s",
                             key.c_str(), e.what()));
      if (!m_options.force()) throw;
    }
  }

  if (indexes.empty()) return false;
  const auto prefix = "ALTER TABLE " + key + " ADD ";
  for (size_t i = 0; i < indexes.size(); ++i)
    indexes[i] = prefix + indexes[i] + ";";
  m_dump->add_deferred_indexes(schema, table, std::move(indexes));
  return true;
}

void Dump_loader::handle_table(const std::string &schema,
                               const std::string &table,
                               const std::string &script, bool resuming,
                               bool indexes_deferred) {
  std::string key = schema_table_key(schema, table);

  log_debug("Executing table DDL for %s", key.c_str());

  try {
    current_console()->print_info(
        (resuming ? "Re-executing DDL script for "
                  : "Executing DDL script for ") +
        key +
        (indexes_deferred ? " (indexes removed for deferred creation)" : ""));
    if (!m_options.dry_run()) {
      // execute sql
      execute_script(m_session, script,
                     shcore::str_format("Error processing table `%s`.`%s`",
                                        schema.c_str(), table.c_str()));
    }
  } catch (const std::exception &e) {
    current_console()->print_error(
        shcore::str_format("Error processing table `%s`.`%s`: %s",
                           schema.c_str(), table.c_str(), e.what()));

    if (!m_options.force()) throw;
  }
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
  shcore::Dictionary_t options;

  // Note: job scheduling should preferrably load different tables per thread

  do {
    {
      std::lock_guard<std::mutex> lock(m_tables_being_loaded_mutex);
      tables_being_loaded = m_tables_being_loaded;
    }
    if (m_dump->next_table_chunk(tables_being_loaded, &schema, &table, &chunked,
                                 &index, &total, &data_file, &size, &options)) {
      options->set("showProgress", m_options.show_progress()
                                       ? shcore::Value::True()
                                       : shcore::Value::False());

      // Override characterSet if given in options
      if (!m_options.character_set().empty()) {
        options->set("characterSet", shcore::Value(m_options.character_set()));
      }

      auto status =
          m_load_log->table_chunk_status(schema, table, chunked ? index : -1);

      log_debug("Table data for '%s'.'%s'/%zi (%s)", schema.c_str(),
                table.c_str(), chunked ? index : -1, to_string(status).c_str());

      if (m_options.load_data()) {
        if (status != Load_progress_log::DONE) {
          scheduled = schedule_table_chunk(
              schema, table, chunked ? index : -1, worker, std::move(data_file),
              size, options, status == Load_progress_log::INTERRUPTED);
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
    const std::string &schema, const std::string &table, ssize_t chunk_index,
    Worker *worker, std::unique_ptr<mysqlshdk::storage::IFile> file,
    size_t size, shcore::Dictionary_t options, bool resuming) {
  if (m_skip_schemas.find(schema) != m_skip_schemas.end() ||
      m_skip_tables.find(schema_table_key(schema, table)) !=
          m_skip_tables.end() ||
      !m_options.include_table(schema, table))
    return false;

  {
    std::lock_guard<std::mutex> lock(m_tables_being_loaded_mutex);
    m_tables_being_loaded.emplace(schema_table_key(schema, table),
                                  file->file_size());
  }

  log_debug("Scheduling chunk for table %s.%s (%s) - worker%zi", schema.c_str(),
            table.c_str(), file->full_path().c_str(), worker->id());

  worker->load_chunk_file(schema, table, std::move(file), chunk_index, size,
                          options, resuming);

  return true;
}

size_t Dump_loader::handle_worker_events() {
  auto console = current_console();

  auto to_string = [](Worker_event::Event event) {
    switch (event) {
      case Worker_event::Event::READY:
        return "READY";
      case Worker_event::Event::FATAL_ERROR:
        return "FATAL_ERROR";
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
    }
    return "";
  };

  bool done = false;
  std::list<Worker *> idle_workers;

  while (!done && idle_workers.size() < m_workers.size()) {
    Worker_event event;

    // Wait for events from workers, but update progress and check for ^C
    // every now and then
    for (;;) {
      update_progress();

      event = m_worker_events.try_pop(1000);
      if (event.worker) break;
    }

    log_debug2("Got event %s from worker %zi", to_string(event.event),
               event.worker->id());

    switch (event.event) {
      case Worker_event::LOAD_START: {
        auto task = static_cast<Worker::Load_chunk_task *>(
            event.worker->current_task());

        on_chunk_load_start(task->schema(), task->table(), task->chunk_index());
        break;
      }

      case Worker_event::LOAD_END: {
        auto task = static_cast<Worker::Load_chunk_task *>(
            event.worker->current_task());

        on_chunk_load_end(task->schema(), task->table(), task->chunk_index(),
                          task->bytes_loaded, task->raw_bytes_loaded);

        event.event = Worker_event::READY;
        break;
      }

      case Worker_event::INDEX_START:
      case Worker_event::ANALYZE_START:
        break;

      case Worker_event::INDEX_END:
      case Worker_event::ANALYZE_END:
        event.event = Worker_event::READY;
        break;

      case Worker_event::READY:
        break;

      case Worker_event::FATAL_ERROR:
        if (!m_worker_interrupt) {
          console->print_error("Aborting load...");
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
      if (m_worker_interrupt || !schedule_next_task(event.worker)) {
        idle_workers.push_back(event.worker);
      }
    }
  }

  size_t num_idle_workers = idle_workers.size();
  // put all idle workers back into the queue, so that they can get assigned
  // new tasks if more becomes available later
  for (auto *worker : idle_workers) {
    m_worker_events.push({Worker_event::READY, worker});
  }
  return num_idle_workers;
}

bool Dump_loader::schedule_next_task(Worker *worker) {
  if (!handle_table_data(worker)) {
    std::string schema;
    std::string table;
    if (m_options.load_indexes() &&
        m_options.defer_table_indexes() !=
            Load_dump_options::Defer_index_mode::OFF) {
      const auto load_finished = [this](const std::string &key) {
        std::lock_guard<std::mutex> lock(m_tables_being_loaded_mutex);
        return m_tables_being_loaded.find(key) == m_tables_being_loaded.end();
      };
      std::vector<std::string> *indexes = nullptr;
      if (m_dump->next_deferred_index(&schema, &table, &indexes,
                                      load_finished)) {
        assert(indexes != nullptr);
        worker->recreate_indexes(schema, table, *indexes);
        return true;
      }
    }

    std::vector<Dump_reader::Histogram> histograms;

    switch (m_options.analyze_tables()) {
      case Load_dump_options::Analyze_table_mode::OFF:
        break;

      case Load_dump_options::Analyze_table_mode::ON:
        if (!m_dump->next_table_analyze(&schema, &table, &histograms)) break;

        worker->analyze_table(schema, table, histograms);
        return true;

      case Load_dump_options::Analyze_table_mode::HISTOGRAM:
        if (!m_dump->next_table_analyze(&schema, &table, &histograms)) break;

        // Only analyze tables with histogram info in the dump
        if (!histograms.empty()) {
          worker->analyze_table(schema, table, histograms);
          return true;
        }
        break;
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
  m_begin_time = std::chrono::system_clock::now();

  open_dump();

  spawn_workers();
  {
    shcore::on_leave_scope cleanup([this]() {
      join_workers();

      m_progress->hide(true);
      m_progress->shutdown();
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

  auto console = current_console();
  auto end_time = std::chrono::system_clock::now();

  const auto seconds = std::max(
      1L, static_cast<long>(std::chrono::duration_cast<std::chrono::seconds>(
                                end_time - m_begin_time)
                                .count()));
  if (m_num_rows_loaded == 0) {
    if (m_resuming)
      console->print_info("There was no remaining data left to be loaded.");
    else
      console->print_info("No data loaded.");
  } else {
    console->print_info(shcore::str_format(
        "%zi chunks (%s, %s) for %zi tables in %zi schemas were "
        "loaded in %s (avg throughput %s)",
        m_num_chunks_loaded.load(),
        format_items("rows", "rows", m_num_rows_loaded.load()).c_str(),
        format_bytes(m_num_bytes_loaded.load()).c_str(),
        m_unique_tables_loaded.size(), m_dump->schemas().size(),
        format_seconds(seconds, false).c_str(),
        format_throughput_bytes(
            m_num_bytes_loaded.load() - m_num_bytes_previously_loaded, seconds)
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

void Dump_loader::update_progress() {
  static const char k_progress_spin[] = "-\\|/";

  std::unique_lock<std::mutex> lock(m_output_mutex, std::try_to_lock);
  if (lock.owns_lock()) {
    if (m_dump->status() == Dump_reader::Status::COMPLETE &&
        m_num_threads_loading.load() == 0 &&
        m_num_threads_recreating_indexes.load() > 0 &&
        m_options.show_progress() &&
        (mysqlsh::current_shell_options()->get().wrap_json == "off")) {
      printf("\r%s thds indexing %c",
             std::to_string(m_num_threads_recreating_indexes.load()).c_str(),
             k_progress_spin[m_progress_spin]);
      fflush(stdout);
      m_progress->show_status(false);
    } else {
      std::string label;
      if (m_dump->status() != Dump_reader::Status::COMPLETE)
        label += "Dump still in progress, " +
                 mysqlshdk::utils::format_bytes(m_dump->dump_size()) +
                 " ready - ";
      if (m_num_threads_loading.load())
        label +=
            std::to_string(m_num_threads_loading.load()) + " thds loading - ";
      if (m_num_threads_recreating_indexes.load())
        label += std::to_string(m_num_threads_recreating_indexes.load()) +
                 " thds indexing - ";

      if (label.length() > 2)
        label[label.length() - 2] = k_progress_spin[m_progress_spin];

      m_progress->set_left_label(label);
      m_progress->show_status(true);
    }

    m_progress_spin++;
    if (m_progress_spin >= static_cast<int>(sizeof(k_progress_spin)) - 1)
      m_progress_spin = 0;
  }
}

void Dump_loader::open_dump() { open_dump(m_options.create_dump_handle()); }

void Dump_loader::open_dump(
    std::unique_ptr<mysqlshdk::storage::IDirectory> dumpdir) {
  auto console = current_console();
  m_dump = std::make_unique<Dump_reader>(std::move(dumpdir), m_options);

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

  if (status != Dump_reader::Status::COMPLETE) {
    if (m_options.dump_wait_timeout() > 0) {
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

  handle_table_only_dump();
}

void Dump_loader::check_server_version() {
  const auto console = current_console();
  const auto &target_server = m_options.target_server_version();
  const auto mds = m_options.is_mds();

  std::string msg = "Target is MySQL " + target_server.get_full();
  if (mds) msg += " (MySQL Database Service)";
  msg +=
      ". Dump was produced from MySQL " + m_dump->server_version().get_full();

  console->print_info(msg);

  if (mds && !m_dump->mds_compatibility()) {
    console->print_error(
        "Destination is a MySQL Database Service instance but the dump was "
        "produced without the compatibility option. Please enable the "
        "'ocimds' option when dumping your database.");
    throw std::runtime_error("Dump is not MDS compatible");
  } else if (target_server.get_major() !=
             m_dump->server_version().get_major()) {
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
      msg += " 'ignoreVersion' option is enabled, so loading anyway.";
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
}

void Dump_loader::check_tables_without_primary_key() {
  if (!m_options.load_ddl() ||
      m_options.target_server_version() < mysqlshdk::utils::Version(8, 0, 13))
    return;
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

  bool has_duplicates = false;

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

void Dump_loader::setup_progress(bool *out_is_resuming) {
  auto console = current_console();

  m_load_log = std::make_unique<Load_progress_log>();
  uint64_t initial = 0;

  if (m_options.progress_file().is_null() ||
      !m_options.progress_file()->empty()) {
    auto progress_file = m_dump->create_progress_file_handle();
    std::string path = progress_file->full_path();
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

        initial = m_num_bytes_loaded.load();
      } else {
        console->print_note(
            "Load progress file detected for the instance but 'resetProgress' "
            "option was enabled. Load progress will be discarded and the whole "
            "dump will be reloaded.");

        m_load_log->reset_progress();
      }
    } else {
      log_info("Logging load progress to %s", path.c_str());
    }
  }

  // Progress mechanics:
  // - if the dump is complete when it's opened, we show progress and
  // throughput relative to total uncompressed size
  //      pct% (current GB / total GB), thrp MB/s

  // - if the dump is incomplete when it's opened, we show # of uncompressed
  // bytes loaded so far and the throughput
  //      current GB compressed ready; current GB loaded, thrp MB/s

  // - when the dump completes during load, we switch to displaying progress
  // relative to the total size

  m_progress->total(m_dump->filtered_data_size(), initial);

  update_progress();
}

void Dump_loader::execute_tasks() {
  auto console = current_console();

  if (m_options.dry_run())
    console->print_info("dryRun enabled, no changes will be made.");

  // Pick charset
  m_character_set = m_options.character_set();
  if (m_character_set.empty()) {
    m_character_set = m_dump->default_character_set();
  }

  check_server_version();

  m_session = create_session(false);

  setup_progress(&m_resuming);

  if (!m_resuming && m_options.load_ddl()) check_existing_objects();

  check_tables_without_primary_key();

  size_t num_idle_workers = 0;

  do {
    if (m_dump->status() != Dump_reader::Status::COMPLETE) {
      wait_for_more_data();

      if (m_dump->status() == Dump_reader::Status::COMPLETE) {
        m_progress->total(m_dump->filtered_data_size());
      }
      update_progress();
    }

    if (!m_init_done) {
      // process dump metadata first
      on_dump_begin();

      handle_schema_scripts();
      m_init_done = true;

      m_progress->hide(false);
    }

    // handle events from workers and schedule more chunks when a worker
    // becomes available
    num_idle_workers = handle_worker_events();

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

  m_progress->current(m_num_bytes_loaded);

  update_progress();

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

      if (m_options.dump_wait_timeout() > 0) {
        const auto current_time = std::chrono::steady_clock::now();
        const auto time_diff = std::chrono::duration_cast<std::chrono::seconds>(
                                   current_time - start_time)
                                   .count();
        if (static_cast<uint64_t>(time_diff) >= m_options.dump_wait_timeout()) {
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
        update_progress();
      }
      waited = true;
      // wait for at most 5s and try again
      for (uint64_t j = 0; j < std::min(static_cast<uint64_t>(5),
                                        m_options.dump_wait_timeout()) &&
                           !m_worker_interrupt;
           j++) {
        shcore::sleep_ms(1000);
      }
    }
  }

  return false;
}

void Dump_loader::spawn_workers() {
  m_thread_exceptions.resize(m_options.threads_count());

  for (int64_t i = 0; i < m_options.threads_count(); i++) {
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
  m_worker_threads[worker->id()].join();
  m_workers.remove_if(
      [worker](const Worker &w) { return w.id() == worker->id(); });
}

void Dump_loader::post_worker_event(Worker *worker, Worker_event::Event event) {
  m_worker_events.push(Worker_event{event, worker});
}

void Dump_loader::on_chunk_load_start(const std::string &schema,
                                      const std::string &table, ssize_t index) {
  m_load_log->start_table_chunk(schema, table, index);
}

void Dump_loader::on_chunk_load_end(const std::string &schema,
                                    const std::string &table, ssize_t index,
                                    size_t bytes_loaded,
                                    size_t raw_bytes_loaded) {
  m_load_log->end_table_chunk(schema, table, index, bytes_loaded,
                              raw_bytes_loaded);

  m_unique_tables_loaded.insert(schema_table_key(schema, table));

  log_debug("Ended loading chunk `%s`.`%s`/%s (%s, %s)", schema.c_str(),
            table.c_str(), std::to_string(index).c_str(),
            std::to_string(bytes_loaded).c_str(),
            std::to_string(raw_bytes_loaded).c_str());
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

void Dump_loader::execute_script(
    const std::shared_ptr<mysqlshdk::db::ISession> &session,
    const std::string &script, const std::string &error_prefix,
    const std::function<bool(const char *, size_t, std::string *)>
        &process_stmt) {
  auto console = mysqlsh::current_console();

  std::stringstream stream(script);
  size_t count = 0;
  mysqlshdk::utils::iterate_sql_stream(
      &stream, 1024 * 64,
      [this, error_prefix, &session, &count, process_stmt](
          const char *s, size_t len, const std::string &, size_t) {
        try {
          std::string new_stmt;
          if (process_stmt && process_stmt(s, len, &new_stmt)) {
            if (!new_stmt.empty()) session->execute(new_stmt);
          } else if (!process_stmt &&
                     m_default_sql_transforms(s, len, &new_stmt)) {
            if (!new_stmt.empty()) session->execute(new_stmt);
          } else {
            session->executes(s, len);
          }
        } catch (const mysqlshdk::db::Error &e) {
          if (error_prefix.empty())
            log_info("Error executing SQL: %s:\t\n%.*s", e.format().c_str(),
                     static_cast<int>(len), s);
          else
            current_console()->print_error(error_prefix + ": " + e.format() +
                                           ": " + std::string(s, len));
          throw;
        }
        ++count;
        return true;
      },
      [console](const std::string &err) { console->print_error(err); });
}

void Dump_loader::handle_table_only_dump() {
  if (m_dump->table_only()) {
    m_dump->replace_target_schema(m_options.target_schema().empty()
                                      ? m_options.current_schema()
                                      : m_options.target_schema());
  }
}

}  // namespace mysqlsh
