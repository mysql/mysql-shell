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

#include "modules/util/load/dump_reader.h"

#include <algorithm>
#include <atomic>
#include <numeric>
#include <utility>

#include "modules/util/common/dump/utils.h"
#include "modules/util/dump/schema_dumper.h"
#include "modules/util/load/load_errors.h"
#include "mysqlshdk/libs/db/mysql/result.h"
#include "mysqlshdk/libs/utils/utils_lexing.h"
#include "mysqlshdk/libs/utils/utils_path.h"
#include "mysqlshdk/libs/utils/utils_sqlstring.h"
#include "mysqlshdk/libs/utils/utils_string.h"

namespace mysqlsh {

namespace {

using mysqlshdk::utils::Version;

std::string fetch_file(mysqlshdk::storage::IDirectory *dir,
                       const std::string &fn) {
  auto file = dir->file(fn);
  file->open(mysqlshdk::storage::Mode::READ);

  std::string data = mysqlshdk::storage::read_file(file.get());

  file->close();

  return data;
}

shcore::Dictionary_t parse_metadata(const std::string &data,
                                    const std::string &fn) {
  try {
    auto metadata = shcore::Value::parse(data);
    if (metadata.get_type() != shcore::Map) {
      THROW_ERROR(SHERR_LOAD_INVALID_METADATA_FILE, fn.c_str());
    }
    return metadata.as_map();
  } catch (const shcore::Exception &e) {
    THROW_ERROR(SHERR_LOAD_PARSING_METADATA_FILE_FAILED, fn.c_str(),
                e.format().c_str());
  }
}

shcore::Dictionary_t fetch_metadata(mysqlshdk::storage::IDirectory *dir,
                                    const std::string &fn) {
  return parse_metadata(fetch_file(dir, fn), fn);
}

auto to_vector_of_strings(const shcore::Array_t &arr) {
  std::vector<std::string> res;
  res.reserve(arr->size());

  for (const auto &s : *arr) {
    res.emplace_back(s.as_string());
  }

  return res;
}

}  // namespace

Dump_reader::Dump_reader(
    std::unique_ptr<mysqlshdk::storage::IDirectory> dump_dir,
    const Load_dump_options &options)
    : m_dir(std::move(dump_dir)), m_options(options) {}

Dump_reader::Status Dump_reader::open() {
  shcore::Dictionary_t md(fetch_metadata(m_dir.get(), "@.json"));

  shcore::Dictionary_t basenames(md->get_map("basenames"));

  for (const auto &schema : *md->get_array("schemas")) {
    if (include_schema(schema.as_string())) {
      auto info = std::make_shared<Schema_info>();

      info->name = schema.get_string();
      if (basenames->has_key(info->name)) {
        info->basename = basenames->get_string(info->name);
      } else {
        info->basename = info->name;
      }

      m_contents.schemas.emplace(info->name, std::move(info));
    } else {
      log_debug("Skipping schema '%s'", schema.as_string().c_str());
    }
  }

  if (md->has_key("version"))
    m_contents.dump_version = Version(md->get_string("version"));

  if (md->has_key("serverVersion"))
    m_contents.server_version = Version(md->get_string("serverVersion"));

  if (md->has_key("targetVersion"))
    m_contents.target_version = Version(md->get_string("targetVersion"));

  if (md->has_key("origin")) m_contents.origin = md->get_string("origin");

  if (md->has_key("defaultCharacterSet"))
    m_contents.default_charset = md->get_string("defaultCharacterSet");

  if (md->has_key("binlogFile"))
    m_contents.binlog_file = md->get_string("binlogFile");

  if (md->has_key("binlogPosition"))
    m_contents.binlog_position = md->get_uint("binlogPosition");

  if (md->has_key("gtidExecuted"))
    m_contents.gtid_executed = md->get_string("gtidExecuted");

  if (md->has_key("gtidExecutedInconsistent")) {
    m_contents.gtid_executed_inconsistent =
        md->get_bool("gtidExecutedInconsistent");
  }

  if (md->has_key("tzUtc")) m_contents.tz_utc = md->get_bool("tzUtc");

  if (md->has_key("mdsCompatibility"))
    m_contents.mds_compatibility = md->get_bool("mdsCompatibility");

  if (md->has_key("partialRevokes"))
    m_contents.partial_revokes = md->get_bool("partialRevokes");

  if (md->has_key("compatibilityOptions")) {
    const auto options = md->at("compatibilityOptions")
                             .to_string_container<std::vector<std::string>>();

    m_contents.create_invisible_pks =
        std::find(options.begin(), options.end(), "create_invisible_pks") !=
        options.end();
  }

  if (md->has_key("tableOnly"))
    m_contents.table_only = md->get_bool("tableOnly");

  if (md->has_key("bytesPerChunk"))
    m_contents.bytes_per_chunk = md->get_uint("bytesPerChunk");

  m_contents.has_users = md->has_key("users");

  if (md->has_key("capabilities")) {
    const auto capabilities = md->at("capabilities").as_array();

    for (const auto &entry : *capabilities) {
      const auto capability = entry.as_map();

      m_contents.capabilities.emplace_back(Capability_info{
          capability->at("id").as_string(),
          capability->at("description").as_string(),
          Version(capability->at("versionRequired").as_string())});
    }
  }

  if (md->has_key("checksum"))
    m_contents.has_checksum = md->get_bool("checksum");

  if (m_dir->file("@.done.json")->exists()) {
    m_contents.parse_done_metadata(m_dir.get(), m_options.checksum(),
                                   m_options.base_session());
    m_dump_status = Status::COMPLETE;
  } else {
    log_info("@.done.json: not found");
    m_dump_status = Status::DUMPING;
  }

  return m_dump_status;
}

std::string Dump_reader::begin_script() const {
  return m_contents.sql ? *m_contents.sql : "";
}

std::string Dump_reader::end_script() const {
  return m_contents.post_sql ? *m_contents.post_sql : "";
}

std::string Dump_reader::users_script() const {
  return m_contents.users_sql ? *m_contents.users_sql : "";
}

bool Dump_reader::next_schema_and_tables(
    std::string *out_schema, std::list<Name_and_file> *out_tables,
    std::list<Name_and_file> *out_view_placeholders) {
  out_tables->clear();
  out_view_placeholders->clear();

  // find the first schema that is ready
  for (auto &it : m_contents.schemas) {
    auto &s = it.second;
    if (!s->table_sql_done && s->ready()) {
      *out_schema = s->name;

      for (const auto &t : s->tables) {
        out_tables->emplace_back(
            t.first,
            t.second->has_sql ? m_dir->file(t.second->script_name()) : nullptr);
      }

      if (s->has_view_sql) {
        for (const auto &v : s->views) {
          out_view_placeholders->emplace_back(v.name,
                                              m_dir->file(v.pre_script_name()));
        }
      }

      s->table_sql_done = true;

      return true;
    }
  }

  return false;
}

bool Dump_reader::next_schema_and_views(std::string *out_schema,
                                        std::list<Name_and_file> *out_views) {
  out_views->clear();

  // find the first schema that is ready
  for (auto &it : m_contents.schemas) {
    auto &s = it.second;

    // always return true for every schema, even if there are no views
    if (!s->view_sql_done && s->ready()) {
      *out_schema = s->name;

      if (s->has_view_sql) {
        for (const auto &v : s->views) {
          out_views->emplace_back(v.name, m_dir->file(v.script_name()));
        }
      }

      s->view_sql_done = true;

      return true;
    }
  }

  return false;
}

std::vector<shcore::Account> Dump_reader::accounts() const {
  std::vector<shcore::Account> account_list;
  std::string script = users_script();

  // parse the script to extract user list
  dump::Schema_dumper::preprocess_users_script(
      script, [&account_list](const std::string &account) {
        account_list.emplace_back(shcore::split_account(account));
        return true;
      });

  return account_list;
}

std::vector<std::string> Dump_reader::schemas() const {
  std::vector<std::string> slist;

  for (const auto &s : m_contents.schemas) {
    slist.push_back(s.second->name);
  }

  return slist;
}

bool Dump_reader::schema_objects(const std::string &schema,
                                 std::list<Object_info *> *out_tables,
                                 std::list<Object_info *> *out_views,
                                 std::list<Object_info *> *out_triggers,
                                 std::list<Object_info *> *out_functions,
                                 std::list<Object_info *> *out_procedures,
                                 std::list<Object_info *> *out_events) {
  auto schema_it = m_contents.schemas.find(schema);
  if (schema_it == m_contents.schemas.end()) return false;
  const auto &schema_info = schema_it->second;

  out_tables->clear();
  out_views->clear();
  out_triggers->clear();
  out_functions->clear();
  out_procedures->clear();
  out_events->clear();

  const auto add_objects = [](auto *src, std::list<Object_info *> *tgt) {
    for (auto &s : *src) {
      tgt->emplace_back(&s);
    }
  };

  for (const auto &t : schema_info->tables) {
    out_tables->push_back(t.second.get());

    add_objects(&t.second->triggers, out_triggers);
  }

  add_objects(&schema_info->views, out_views);
  add_objects(&schema_info->functions, out_functions);
  add_objects(&schema_info->procedures, out_procedures);
  add_objects(&schema_info->events, out_events);

  return true;
}

void Dump_reader::schema_table_triggers(
    const std::string &schema, std::list<Name_and_file> *out_table_triggers) {
  out_table_triggers->clear();

  const auto &s = m_contents.schemas.at(schema);
  for (const auto &t : s->tables) {
    if (t.second->has_triggers)
      out_table_triggers->emplace_back(
          t.first, m_dir->file(t.second->triggers_script_name()));
  }
}

const std::vector<std::string> &Dump_reader::deferred_schema_fks(
    const std::string &schema) const {
  const auto &s = m_contents.schemas.at(schema);
  return s->foreign_key_queries;
}

const std::vector<std::string> &Dump_reader::queries_on_schema_end(
    const std::string &schema) const {
  const auto &s = m_contents.schemas.at(schema);
  return s->queries_on_schema_end;
}

const std::map<std::string, std::vector<std::string>>
Dump_reader::tables_without_pk() const {
  std::map<std::string, std::vector<std::string>> res;
  for (const auto &s : m_contents.schemas) {
    std::vector<std::string> tables;
    for (const auto &t : s.second->tables)
      if (t.second->primary_index.empty())
        tables.emplace_back(shcore::quote_identifier(t.first));
    if (!tables.empty()) {
      std::sort(tables.begin(), tables.end());
      res.emplace(s.first, tables);
    }
  }
  return res;
}

bool Dump_reader::has_tables_without_pk() const {
  for (const auto &s : m_contents.schemas) {
    for (const auto &t : s.second->tables) {
      if (t.second->primary_index.empty()) {
        return true;
      }
    }
  }

  return false;
}

bool Dump_reader::has_primary_key(const std::string &schema,
                                  const std::string &table) const {
  return !m_contents.schemas.at(schema)
              ->tables.at(table)
              ->primary_index.empty();
}

std::string Dump_reader::fetch_schema_script(const std::string &schema) const {
  std::string script;

  const auto &s = m_contents.schemas.at(schema);

  if (s->has_sql) {
    // Get the base script for the schema
    script = fetch_file(m_dir.get(), s->script_name());
  }

  return script;
}

// Proportional chunk scheduling
//
// Multiple sessions writing to the same table mean they will be competing for
// locks to be able to update indexes.
// So to optimize performance, we try to have as many different tables loaded
// at the same time as possible.
// If there are more tables than threads, that means there will be a single
// session per table.
// But if there are more threads than tables, we need to distribute them
// among tables. If we distribute the threads equally, then smaller tables
// will finish sooner and bigger tables will take extra longer because
// it will have more threads working. So our optimization goal must be to keep
// as many different tables loading concurrently for as long as possible, while
// ensuring that tables that are currently being loaded have preference over
// others, which could otherwise trigger tables to get flushed out from the
// buffer pool.
// Thus, smaller tables must get fewer threads allocated so they take longer
// to load, while bigger threads get more, with the hope that the total time
// to load all tables is minimized.
Dump_reader::Candidate Dump_reader::schedule_chunk_proportionally(
    const std::unordered_multimap<std::string, size_t> &tables_being_loaded,
    std::unordered_set<Dump_reader::Table_data_info *> *tables_with_data,
    uint64_t max_concurrent_tables) {
  if (tables_with_data->empty()) return tables_with_data->end();

  std::vector<Candidate> tables_in_progress;

  // first check if there's any table that's not being loaded
  {
    const auto end = tables_with_data->end();
    auto best = end;

    for (auto it = tables_with_data->begin(); it != end; ++it) {
      if ((*it)->chunks_consumed) {
        tables_in_progress.emplace_back(it);
      }

      if (tables_being_loaded.find((*it)->key()) == tables_being_loaded.end()) {
        // table is better if it's bigger and in the same state as the current
        // best, or if it was previously scheduled and current best was not
        if (best == end ||
            ((*it)->bytes_available() > (*best)->bytes_available() &&
             !(*it)->chunks_consumed == !(*best)->chunks_consumed) ||
            ((*it)->chunks_consumed && !(*best)->chunks_consumed))
          best = it;
      }
    }

    // schedule a new table only if we're not exceeding the maximum number of
    // concurrent tables that can be loaded at the same time
    if (best != end && (tables_in_progress.size() < max_concurrent_tables ||
                        (*best)->chunks_consumed)) {
      return best;
    }
  }

  // if all available tables are already loaded, then schedule proportionally
  std::unordered_map<std::string, double> worker_weights;

  // calc ratio of data being loaded per table / total data being loaded
  double total_bytes_loading = std::accumulate(
      tables_being_loaded.begin(), tables_being_loaded.end(),
      static_cast<size_t>(0),
      [&worker_weights](size_t size,
                        const std::pair<std::string, size_t> &table_size) {
        worker_weights[table_size.first] += table_size.second;
        return size + table_size.second;
      });

  if (total_bytes_loading > 0) {
    for (auto &it : worker_weights) {
      it.second = it.second / total_bytes_loading;
    }
  }

  std::vector<std::pair<Candidate, double>> candidate_weights;

  // calc ratio of data available per table / total data available
  double total_bytes_available = std::accumulate(
      tables_in_progress.begin(), tables_in_progress.end(),
      static_cast<size_t>(0),
      [](size_t size, auto it) { return size + (*it)->bytes_available(); });
  if (total_bytes_available > 0) {
    for (auto it = tables_in_progress.begin(); it != tables_in_progress.end();
         ++it) {
      candidate_weights.emplace_back(
          *it, static_cast<double>((**it)->bytes_available()) /
                   total_bytes_available);
    }
  } else {
    // it's possible that all files loaded so far are empty, return any table
    return tables_in_progress.front();
  }

  // pick a chunk from the table that has the biggest difference between both
  double best_diff = 0;
  Candidate best = tables_in_progress.front();

  for (const auto &cand : candidate_weights) {
    const auto it = worker_weights.find((*cand.first)->key());
    const auto weight = it == worker_weights.end() ? 0.0 : it->second;
    const auto d = cand.second - weight;

    if (d > best_diff) {
      best_diff = d;
      best = cand.first;
    }
  }

  return best;
}

bool Dump_reader::next_table_chunk(
    const std::unordered_multimap<std::string, size_t> &tables_being_loaded,
    std::string *out_schema, std::string *out_table, std::string *out_partition,
    bool *out_chunked, size_t *out_chunk_index, size_t *out_chunks_total,
    std::unique_ptr<mysqlshdk::storage::IFile> *out_file,
    size_t *out_chunk_size, shcore::Dictionary_t *out_options) {
  auto iter = schedule_chunk_proportionally(
      tables_being_loaded, &m_tables_with_data, m_options.threads_count());

  if (iter != m_tables_with_data.end()) {
    *out_schema = (*iter)->owner->schema;
    *out_table = (*iter)->owner->name;
    *out_partition = (*iter)->partition;
    *out_chunked = (*iter)->chunked;
    *out_chunk_index = (*iter)->chunks_consumed;

    if ((*iter)->last_chunk_seen) {
      *out_chunks_total = (*iter)->available_chunks.size();
    } else {
      *out_chunks_total = 0;
    }

    const auto &info = (*iter)->available_chunks[*out_chunk_index];

    if (!info.has_value()) {
      throw std::logic_error(
          "Trying to use chunk " + std::to_string(*out_chunk_index) + " of " +
          schema_table_object_key(*out_schema, *out_table, *out_partition) +
          " which is not yet available");
    }

    *out_file = m_dir->file(info->name());
    *out_chunk_size = info->size();
    *out_options = (*iter)->owner->options;

    (*iter)->consume_chunk();
    if (!(*iter)->has_data_available()) m_tables_with_data.erase(iter);

    return true;
  }

  return false;
}

bool Dump_reader::next_deferred_index(
    std::string *out_schema, std::string *out_table,
    compatibility::Deferred_statements::Index_info **out_indexes) {
  for (auto &schema : m_contents.schemas) {
    for (auto &table : schema.second->tables) {
      if ((!m_options.load_data() || table.second->all_data_loaded()) &&
          !table.second->indexes_scheduled) {
        table.second->indexes_scheduled = true;
        *out_schema = schema.second->name;
        *out_table = table.second->name;
        *out_indexes = &table.second->indexes;
        return true;
      }
    }
  }
  return false;
}

bool Dump_reader::next_table_analyze(std::string *out_schema,
                                     std::string *out_table,
                                     std::vector<Histogram> *out_histograms) {
  for (auto &schema : m_contents.schemas) {
    for (auto &table : schema.second->tables) {
      if ((!m_options.load_data() || table.second->all_data_loaded()) &&
          table.second->indexes_created && !table.second->analyze_scheduled) {
        table.second->analyze_scheduled = true;
        *out_schema = schema.second->name;
        *out_table = table.second->name;
        *out_histograms = table.second->histograms;
        return true;
      }
    }
  }
  return false;
}

bool Dump_reader::next_table_checksum(
    const dump::common::Checksums::Checksum_data **out_checksum) {
  assert(out_checksum);

  for (auto &schema : m_contents.schemas) {
    for (auto &table : schema.second->tables) {
      if (table.second->indexes_created && table.second->analyze_finished) {
        for (auto &partition : table.second->data_info) {
          if (!partition.checksums.empty() &&
              (!m_options.load_data() || partition.data_loaded())) {
            *out_checksum = partition.checksums.front();
            partition.checksums.pop_front();
            return true;
          }
        }
      }
    }
  }

  return false;
}

bool Dump_reader::data_available() const { return !m_tables_with_data.empty(); }

bool Dump_reader::data_pending() const {
  if (!m_options.load_data()) {
    return false;
  }

  for (const auto &schema : m_contents.schemas) {
    for (const auto &table : schema.second->tables) {
      if (!table.second->all_data_scheduled()) {
        return true;
      }
    }
  }

  return false;
}

bool Dump_reader::work_available() const {
  for (const auto &schema : m_contents.schemas) {
    for (const auto &table : schema.second->tables) {
      if ((m_options.load_data() && !table.second->all_data_scheduled()) ||
          !table.second->indexes_scheduled ||
          !table.second->analyze_scheduled ||
          !table.second->all_data_verified()) {
        return true;
      }
    }
  }
  return false;
}

bool Dump_reader::all_data_verification_scheduled() const {
  if (!m_options.checksum()) {
    return true;
  }

  if (Status::COMPLETE != m_dump_status) {
    return false;
  }

  for (const auto &schema : m_contents.schemas) {
    for (const auto &table : schema.second->tables) {
      if (!table.second->all_data_verification_scheduled()) {
        return false;
      }
    }
  }

  return true;
}

size_t Dump_reader::filtered_data_size() const {
  if (m_dump_status == Status::COMPLETE) {
    return m_filtered_data_size;
  } else {
    return total_data_size();
  }
}

size_t Dump_reader::table_data_size(const std::string &schema,
                                    const std::string &table) const {
  if (const auto s = m_contents.table_data_size.find(schema);
      s != m_contents.table_data_size.end()) {
    if (const auto t = s->second.find(table); t != s->second.end()) {
      return t->second;
    }
  }

  return 0;
}

void Dump_reader::compute_filtered_data_size() {
  m_filtered_data_size = 0;

  for (const auto &schema : m_contents.schemas) {
    const auto s = m_contents.table_data_size.find(schema.first);

    if (s != m_contents.table_data_size.end()) {
      for (const auto &table : schema.second->tables) {
        const auto t = s->second.find(table.second->name);

        if (t != s->second.end()) {
          m_filtered_data_size += t->second;
        }
      }
    }
  }
}

// Scan directory for new files and adds them to the pending file list
void Dump_reader::rescan(dump::Progress_thread *progress_thread) {
  Files files;

  {
    dump::Progress_thread::Stage *stage = nullptr;
    shcore::on_leave_scope finish_stage([&stage]() {
      if (stage) {
        stage->finish();
      }
    });

    if (!m_dir->is_local()) {
      current_console()->print_status(
          "Fetching dump data from remote location...");

      if (progress_thread) {
        stage = progress_thread->start_stage("Listing files");
      }
    }

    files = m_dir->list_files();
  }

  log_debug("Finished listing files, starting rescan");

  m_contents.rescan(m_dir.get(), files, this, progress_thread);

  log_debug("Rescan done");

  if (m_dump_status != Status::COMPLETE &&
      files.find({"@.done.json"}) != files.end()) {
    m_contents.parse_done_metadata(m_dir.get(), m_options.checksum(),
                                   m_options.base_session());
    m_dump_status = Status::COMPLETE;
  }

  compute_filtered_data_size();
}

uint64_t Dump_reader::add_deferred_statements(
    const std::string &schema, const std::string &table,
    compatibility::Deferred_statements &&stmts) {
  const auto s = m_contents.schemas.find(schema);

  if (s == m_contents.schemas.end()) {
    throw std::logic_error("Unable to find schema " + schema +
                           " for adding index");
  }

  const auto t = s->second->tables.find(table);

  if (t == s->second->tables.end()) {
    throw std::logic_error("Unable to find table " + table + " in schema " +
                           schema + " for adding index");
  }

  // if indexes are not going to be recreated, we're marking them as already
  // created
  t->second->indexes_scheduled = t->second->indexes_created =
      !m_options.load_deferred_indexes() || stmts.index_info.empty();
  t->second->indexes = std::move(stmts.index_info);

  const auto table_name = schema_object_key(schema, table);

  for (const auto &fk : stmts.foreign_keys) {
    s->second->foreign_key_queries.emplace_back("ALTER TABLE " + table_name +
                                                " ADD " + fk);
  }

  if (!stmts.secondary_engine.empty()) {
    s->second->queries_on_schema_end.emplace_back(
        std::move(stmts.secondary_engine));
  }

  return t->second->indexes.size();
}

void Dump_reader::replace_target_schema(const std::string &schema) {
  if (1 != m_contents.schemas.size()) {
    current_console()->print_error(
        "The 'schema' option can only be used when loading a single schema, "
        "but " +
        std::to_string(m_contents.schemas.size()) + " will be loaded.");

    throw std::invalid_argument("Invalid option: schema.");
  }

  const auto info = m_contents.schemas.begin()->second;
  m_contents.schemas.clear();
  m_contents.schemas.emplace(schema, info);

  m_schema_override = {schema, info->name};
  info->name = schema;

  for (const auto &table : info->tables) {
    table.second->schema = schema;
    table.second->options->set("schema", shcore::Value(schema));
  }

  for (auto &view : info->views) {
    view.schema = schema;
  }
}

void Dump_reader::validate_options() {
  if (m_options.load_users() && !m_contents.has_users) {
    current_console()->print_warning(
        "The 'loadUsers' option is set to true, but the dump does not contain "
        "the user data.");
  }

  if (table_only()) {
    // old version of dumpTables() - no schema SQL is available
    if (m_options.target_schema().empty()) {
      // user didn't provide the new schema name, we cannot proceed
      current_console()->print_error(
          "The dump was created by an older version of the util." +
          shcore::get_member_name("dumpTables",
                                  shcore::current_naming_style()) +
          "() function and needs to be loaded into an existing schema. "
          "Please set the target schema using the 'schema' option.");

      throw std::invalid_argument("The target schema was not specified.");
    } else {
      const auto result = m_options.base_session()->queryf(
          "SELECT SCHEMA_NAME FROM information_schema.schemata WHERE "
          "SCHEMA_NAME=?",
          m_options.target_schema());

      if (nullptr == result->fetch_one()) {
        throw std::invalid_argument("The specified schema does not exist.");
      }
    }
  }

  if (should_create_pks() && !m_options.load_ddl()) {
    current_console()->print_warning(
        "The 'createInvisiblePKs' option is set to true, but the 'loadDdl' "
        "option is false, Primary Keys are not going to be created.");
  }

  if (m_options.checksum()) {
    if (!m_contents.has_checksum) {
      throw std::invalid_argument(
          "The 'checksum' option cannot be used when the dump does not contain "
          "checksum information.");
    }

    if (m_options.dry_run()) {
      current_console()->print_warning(
          "Checksum information is not going to be verified, dryRun enabled.");
    }
  }
}

std::string Dump_reader::Table_info::script_name() const {
  return dump::common::get_table_filename(basename);
}

std::string Dump_reader::Table_info::triggers_script_name() const {
  return dump::common::get_table_data_filename(basename, "triggers.sql");
}

bool Dump_reader::Table_info::ready() const {
  return md_done && (!has_sql || sql_seen);
}

std::string Dump_reader::Table_info::metadata_name() const {
  // schema@table.json
  return dump::common::get_table_data_filename(basename, "json");
}

bool Dump_reader::Table_info::should_fetch_metadata_file(
    const Files &files) const {
  if (!md_done && files.find(metadata_name()) != files.end()) {
    return true;
  }

  return false;
}

void Dump_reader::Table_info::update_metadata(const std::string &data,
                                              Dump_reader *reader) {
  auto md = parse_metadata(data, metadata_name());

  Table_data_info di;
  di.owner = this;

  has_sql = md->get_bool("includesDdl", true);
  di.has_data = md->get_bool("includesData", true);

  options = md->get_map("options");

  if (options) {
    // "compression" and "primaryIndex" are not used by the chunk importer
    // and need to be removed
    // these were misplaced in the options dictionary, code is kept for
    // backward compatibility
    options->erase("compression");

    if (options->has_key("primaryIndex")) {
      auto index = options->get_string("primaryIndex");

      if (!index.empty()) {
        primary_index.emplace_back(std::move(index));
      }

      options->erase("primaryIndex");
    }

    // chunk importer uses characterSet instead of defaultCharacterSet
    if (options->has_key("defaultCharacterSet")) {
      options->set("characterSet", options->at("defaultCharacterSet"));
      options->erase("defaultCharacterSet");
    } else {
      // By default, we use the character set from the source DB
      options->set("characterSet",
                   shcore::Value(reader->default_character_set()));
    }
  }

  options->set("showProgress",
               shcore::Value(reader->m_options.show_progress()));

  // Override characterSet if given in options
  if (!reader->m_options.character_set().empty()) {
    options->set("characterSet",
                 shcore::Value(reader->m_options.character_set()));
  }

  di.extension = md->get_string("extension", "tsv");
  di.chunked = md->get_bool("chunking", false);

  if (md->has_key("primaryIndex")) {
    primary_index = to_vector_of_strings(md->get_array("primaryIndex"));
  }

  if (const auto trigger_list = md->get_array("triggers")) {
    for (const auto &t : *trigger_list) {
      auto trigger_name = t.as_string();

      if (reader->include_trigger(schema, name, trigger_name)) {
        triggers.emplace_back(Object_info{std::move(trigger_name)});
      }
    }

    has_triggers = !triggers.empty();

    log_debug("%s.%s has %zi triggers", schema.c_str(), name.c_str(),
              triggers.size());
  } else {
    has_triggers = false;
  }

  auto histogram_list = md->get_array("histograms");

  if (histogram_list) {
    for (const auto &h : *histogram_list) {
      auto histogram = h.as_map();
      if (histogram) {
        histograms.emplace_back(
            Histogram{histogram->get_string("column"),
                      static_cast<size_t>(histogram->get_int("buckets"))});
      }
    }
  }

  {
    // maps partition names to basenames
    const auto basenames = md->get_map("basenames");

    if (basenames && !basenames->empty()) {
      for (const auto &p : *basenames) {
        auto copy = di;

        copy.partition = p.first;
        copy.basename = p.second.as_string();
        copy.initialize_checksums(reader->m_contents.checksum.get());

        data_info.emplace_back(std::move(copy));
      }
    } else {
      di.basename = basename;
      di.initialize_checksums(reader->m_contents.checksum.get());

      data_info.emplace_back(std::move(di));
    }
  }

  reader->on_table_metadata_parsed(*this);
  md_done = true;
}

void Dump_reader::Table_info::rescan(const Files &files) {
  // MD not included for tables if data is not dumped

  // check for the sql file for the schema
  if (!sql_seen) {
    if (files.find(script_name()) != files.end()) {
      sql_seen = true;
    }
  }

  if (!has_triggers) {
    if (files.find(triggers_script_name()) != files.end()) {
      has_triggers = true;
    }
  }
}

bool Dump_reader::Table_info::all_data_scheduled() const {
  for (const auto &di : data_info) {
    if (!di.data_scheduled()) return false;
  }

  return true;
}

bool Dump_reader::Table_info::all_data_loaded() const {
  for (const auto &di : data_info) {
    if (!di.data_loaded()) return false;
  }

  return true;
}

bool Dump_reader::Table_info::all_data_verified() const {
  for (const auto &di : data_info) {
    if (!di.data_verified()) return false;
  }

  return true;
}

bool Dump_reader::Table_info::all_data_verification_scheduled() const {
  for (const auto &di : data_info) {
    if (!di.data_verification_scheduled()) return false;
  }

  return true;
}

void Dump_reader::Table_data_info::rescan_data(const Files &files,
                                               Dump_reader *reader) {
  bool found_data = false;

  const auto try_to_add_chunk = [&files, reader, &found_data,
                                 this](auto &&... params) {
    static_assert(sizeof...(params) == 0 || sizeof...(params) == 2);

    // default values for non-chunked case
    size_t idx = 0;
    bool last_chunk = true;

    if constexpr (sizeof...(params) == 2) {
      std::tie(idx, last_chunk) = std::forward_as_tuple(params...);
    }

    const auto it = files.find(
        dump::common::get_table_data_filename(basename, extension, params...));

    if (it == files.end()) {
      return false;
    }

    if (idx >= available_chunks.size()) {
      available_chunks.resize(idx + 1, {});
    }

    available_chunks[idx] = *it;
    reader->m_contents.dump_size += it->size();
    ++chunks_seen;
    found_data = true;

    if (last_chunk) {
      last_chunk_seen = true;
    }

    return true;
  };

  // in older versions of Shell, empty tables would create a single data file
  // using the non-chunked name format even if file was supposed to be chunked
  try_to_add_chunk();

  if (chunked) {
    if (chunks_seen < available_chunks.size()) {
      // search for chunks that we're still missing
      for (size_t i = 0, s = available_chunks.size(); i < s; ++i) {
        // If we have found the last chunk, then the last element in the array
        // will already be set. If we didn't find the last chunk, then the last
        // element in the array is going to be a regular chunk.
        if (!available_chunks[i].has_value()) {
          try_to_add_chunk(i, false);
        }
      }
    }

    if (!last_chunk_seen) {
      // We don't know how many chunks there are, try to find a consecutive
      // sequence of chunk IDs. If sequence stops, check if the final chunk
      // follows, then always break as we don't want to scan too many files.
      for (size_t i = available_chunks.size(); i < files.size(); ++i) {
        if (!try_to_add_chunk(i, false)) {
          try_to_add_chunk(i, true);
          break;
        }
      }
    }
  }

  if (found_data) reader->m_tables_with_data.insert(this);
}

void Dump_reader::Table_data_info::initialize_checksums(
    const dump::common::Checksums *info) {
  if (!info) {
    return;
  }

  const auto list = info->find_checksums(owner->schema, owner->name, partition);

  if (list.empty()) {
    log_warning(
        "Could not find checksum information of: %s",
        schema_table_object_key(owner->schema, owner->name, partition).c_str());
  } else {
    checksums = std::list(list.begin(), list.end());
    checksums_total = checksums.size();
  }
}

std::string Dump_reader::View_info::script_name() const {
  return dump::common::get_table_filename(basename);
}

std::string Dump_reader::View_info::pre_script_name() const {
  return dump::common::get_table_data_filename(basename, "pre.sql");
}

void Dump_reader::View_info::rescan(const Files &files) {
  // check for the sql file for the schema
  if (!sql_seen) {
    if (files.find(script_name()) != files.end()) {
      sql_seen = true;
    }
  }

  if (!sql_pre_seen) {
    if (files.find(pre_script_name()) != files.end()) {
      sql_pre_seen = true;
    }
  }
}

bool Dump_reader::Schema_info::ready() const {
  if (md_done && (!has_sql || sql_seen)) {
    return true;
  }

  return false;
}

std::string Dump_reader::Schema_info::script_name() const {
  return dump::common::get_schema_filename(basename);
}

std::string Dump_reader::Schema_info::metadata_name() const {
  // schema.json
  return dump::common::get_schema_filename(basename, "json");
}

bool Dump_reader::Schema_info::should_fetch_metadata_file(
    const Files &files) const {
  if (!md_loaded && files.find(metadata_name()) != files.end()) {
    return true;
  }

  return false;
}

void Dump_reader::Schema_info::update_metadata(const std::string &data,
                                               Dump_reader *reader) {
  auto md = parse_metadata(data, metadata_name());

  has_sql = md->get_bool("includesDdl", true);
  has_view_sql = md->get_bool("includesViewsDdl", has_sql);
  has_data = md->get_bool("includesData", true);

  shcore::Dictionary_t basenames = md->get_map("basenames");

  if (const auto table_list = md->get_array("tables")) {
    for (const auto &t : *table_list) {
      auto table_name = t.as_string();

      if (reader->include_table(name, table_name)) {
        auto info = std::make_shared<Table_info>();

        info->schema = name;
        info->name = std::move(table_name);

        if (basenames->has_key(info->name))
          info->basename = basenames->get_string(info->name);
        else
          info->basename = basename + "@" + info->name;

        // if tables are not going to be analysed, we're marking them as already
        // analysed
        info->analyze_scheduled = info->analyze_finished =
            reader->m_options.analyze_tables() ==
            Load_dump_options::Analyze_table_mode::OFF;

        tables.emplace(info->name, std::move(info));
      }
    }

    log_debug("%s has %zi tables", name.c_str(), tables.size());
  }

  if (const auto view_list = md->get_array("views")) {
    for (const auto &v : *view_list) {
      auto view_name = v.as_string();

      if (reader->include_table(name, view_name)) {
        View_info info;

        info.schema = name;
        info.name = std::move(view_name);
        if (basenames->has_key(info.name))
          info.basename = basenames->get_string(info.name);
        else
          info.basename = basename + "@" + info.name;

        views.emplace_back(std::move(info));
      }
    }

    log_debug("%s has %zi views", name.c_str(), views.size());
  }

  if (const auto function_list = md->get_array("functions")) {
    for (const auto &f : *function_list) {
      auto function_name = f.as_string();

      if (reader->include_routine(name, function_name)) {
        functions.emplace_back(Object_info{std::move(function_name)});
      }
    }

    log_debug("%s has %zi functions", name.c_str(), functions.size());
  }

  if (const auto procedure_list = md->get_array("procedures")) {
    for (const auto &p : *procedure_list) {
      auto procedure_name = p.as_string();

      if (reader->include_routine(name, procedure_name)) {
        procedures.emplace_back(Object_info{std::move(procedure_name)});
      }
    }

    log_debug("%s has %zi procedures", name.c_str(), procedures.size());
  }

  if (const auto event_list = md->get_array("events")) {
    for (const auto &e : *event_list) {
      auto event_name = e.as_string();

      if (reader->include_event(name, event_name)) {
        events.emplace_back(Object_info{std::move(event_name)});
      }
    }

    log_debug("%s has %zi events", name.c_str(), events.size());
  }

  md_loaded = true;
  reader->on_metadata_parsed();
}

void Dump_reader::Schema_info::rescan(mysqlshdk::storage::IDirectory *dir,
                                      const Files &files, Dump_reader *reader,
                                      shcore::Thread_pool *pool) {
  log_debug("Scanning contents of schema '%s'", name.c_str());

  if (md_loaded && !md_done) {
    // we have the list of tables, so check for their metadata and data files
    std::size_t files_to_fetch = 0;
    for (auto &t : tables) {
      if (!t.second->ready()) {
        if (t.second->should_fetch_metadata_file(files)) {
          // metadata available, fetch it, parse it, then rescan asynchronously
          reader->on_metadata_available();
          ++files_to_fetch;

          pool->add_task(
              [dir, mdpath = t.second->metadata_name()]() {
                return fetch_file(dir, mdpath);
              },
              [table = t.second.get(), &files, reader](std::string &&data) {
                table->update_metadata(data, reader);
                table->rescan(files);
              });
        } else {
          // metadata already parsed, scan synchronously
          t.second->rescan(files);
        }
      }
    }

    if (files_to_fetch > 0 && !dir->is_local()) {
      log_info("Fetching %zu table metadata files for schema `%s`...",
               files_to_fetch, name.c_str());
    }

    for (auto &v : views) {
      if (!v.ready()) {
        v.rescan(files);
      }
    }
  }

  // check for the sql file for the schema
  if (!sql_seen) {
    if (files.find(script_name()) != files.end()) {
      sql_seen = true;
    }
  }
}

void Dump_reader::Schema_info::check_if_ready() {
  if (md_loaded && !md_done) {
    bool children_done = true;

    for (const auto &t : tables) {
      if (!t.second->ready()) {
        children_done = false;
        break;
      }
    }

    if (children_done) {
      for (const auto &v : views) {
        if (!v.ready()) {
          children_done = false;
          break;
        }
      }
    }

    md_done = children_done;

    if (md_done) {
      log_debug("All metadata for schema `%s` was scanned", name.c_str());
    }
  }
}

void Dump_reader::Schema_info::rescan_data(const Files &files,
                                           Dump_reader *reader) {
  for (auto &t : tables) {
    for (auto &di : t.second->data_info) {
      if (!di.data_dumped()) {
        di.rescan_data(files, reader);
      }
    }
  }
}

bool Dump_reader::Dump_info::ready() const {
  // we're not checking for the @.sql, @.post.sql and @.users.sql here, because
  // presence of these files depends on the dataOnly dump option (which is not
  // recorded in the metadata), but these files are written at the beginning of
  // the dump, so if md_done is true, then for sure these were already written
  // by the dumper
  return md_done;
}

void Dump_reader::Dump_info::rescan(mysqlshdk::storage::IDirectory *dir,
                                    const Files &files, Dump_reader *reader,
                                    dump::Progress_thread *progress_thread) {
  if (!sql && files.find({"@.sql"}) != files.end()) {
    sql = std::make_unique<std::string>(fetch_file(dir, "@.sql"));
  }
  if (!post_sql && files.find({"@.post.sql"}) != files.end()) {
    post_sql = std::make_unique<std::string>(fetch_file(dir, "@.post.sql"));
  }
  if (has_users && !users_sql && files.find({"@.users.sql"}) != files.end()) {
    users_sql = std::make_unique<std::string>(fetch_file(dir, "@.users.sql"));
  }

  if (!md_done) {
    rescan_metadata(dir, files, reader, progress_thread);
  }

  rescan_data(files, reader);
}

void Dump_reader::Dump_info::rescan_metadata(
    mysqlshdk::storage::IDirectory *dir, const Files &files,
    Dump_reader *reader, dump::Progress_thread *progress_thread) {
  const auto thread_pool_ptr = reader->create_thread_pool();
  const auto pool = thread_pool_ptr.get();

  std::atomic<uint64_t> task_producers{0};

  const auto maybe_shutdown = [&task_producers, pool]() {
    if (0 == --task_producers) {
      pool->tasks_done();
    }
  };

  ++task_producers;

  dump::Progress_thread::Stage *stage = nullptr;
  shcore::on_leave_scope finish_stage([&stage]() {
    if (stage) {
      stage->finish();
    }
  });

  if (progress_thread) {
    dump::Progress_thread::Progress_config config;
    config.current = [reader]() { return reader->metadata_parsed(); };
    config.total = [reader]() { return reader->metadata_available(); };
    config.is_total_known = [&task_producers]() { return 0 == task_producers; };

    stage =
        progress_thread->start_stage("Scanning metadata", std::move(config));
  }

  pool->start_threads();

  for (const auto &s : schemas) {
    if (!s.second->ready()) {
      if (s.second->should_fetch_metadata_file(files)) {
        // metadata available, fetch it, parse it, then rescan asynchronously
        reader->on_metadata_available();
        ++task_producers;

        pool->add_task(
            [dir, mdpath = s.second->metadata_name()]() {
              return fetch_file(dir, mdpath);
            },
            [&maybe_shutdown, schema = s.second.get(), dir, &files, reader,
             pool](std::string &&data) {
              shcore::on_leave_scope cleanup(
                  [&maybe_shutdown]() { maybe_shutdown(); });

              schema->update_metadata(data, reader);
              schema->rescan(dir, files, reader, pool);
            });
      } else {
        // metadata already parsed, scan synchronously
        s.second->rescan(dir, files, reader, pool);
      }
    }
  }

  maybe_shutdown();
  pool->process();

  check_if_ready();
}

void Dump_reader::Dump_info::check_if_ready() {
  if (!md_done) {
    bool children_done = true;

    for (const auto &s : schemas) {
      s.second->check_if_ready();

      if (!s.second->ready()) {
        children_done = false;
      }
    }

    md_done = children_done;

    if (md_done) log_debug("All metadata for dump was scanned");
  }
}

void Dump_reader::Dump_info::rescan_data(const Files &files,
                                         Dump_reader *reader) {
  for (const auto &s : schemas) {
    if (s.second->ready()) {
      log_debug("Scanning data of schema '%s'", s.second->name.c_str());

      s.second->rescan_data(files, reader);
    }
  }
}

void Dump_reader::Dump_info::parse_done_metadata(
    mysqlshdk::storage::IDirectory *dir, bool get_checksum,
    const std::shared_ptr<mysqlshdk::db::ISession> &session) {
  log_info("Dump %s is complete", dir->full_path().masked().c_str());

  if (const auto metadata = fetch_metadata(dir, "@.done.json")) {
    if (metadata->has_key("dataBytes")) {
      data_size = metadata->get_uint("dataBytes");
    } else {
      log_warning(
          "Dump metadata file @.done.json does not contain dataBytes "
          "information");
    }

    if (metadata->has_key("tableDataBytes")) {
      for (const auto &schema : *metadata->get_map("tableDataBytes")) {
        for (const auto &table : *schema.second.as_map()) {
          table_data_size[schema.first][table.first] = table.second.as_uint();
        }
      }
    } else {
      log_warning(
          "Dump metadata file @.done.json does not contain tableDataBytes "
          "information");
    }

    // only exists in 1.0.1+
    if (metadata->has_key("chunkFileBytes")) {
      for (const auto &file : *metadata->get_map("chunkFileBytes")) {
        chunk_sizes[file.first] = file.second.as_uint();
      }
    }
  } else {
    log_warning("Dump metadata file @.done.json is invalid");
  }

  if (get_checksum && has_checksum) {
    initialize_checksums(dir, session);
  }
}

void Dump_reader::Dump_info::initialize_checksums(
    mysqlshdk::storage::IDirectory *dir,
    const std::shared_ptr<mysqlshdk::db::ISession> &session) {
  checksum = std::make_unique<dump::common::Checksums>();
  checksum->deserialize(dir->file("@.checksums.json"));
  checksum->configure(session);

  // initialize tables which were parsed before dump was complete
  for (auto &schema : schemas) {
    for (auto &table : schema.second->tables) {
      for (auto &di : table.second->data_info) {
        if (di.checksums.empty()) {
          di.initialize_checksums(checksum.get());
        }
      }
    }
  }
}

std::unique_ptr<mysqlshdk::storage::IFile>
Dump_reader::create_progress_file_handle() const {
  return m_options.create_progress_file_handle();
}

void Dump_reader::show_metadata() const {
  const auto metadata = shcore::make_dict(
      "Dump_metadata", shcore::make_dict("Binlog_file", binlog_file(),
                                         "Binlog_position", binlog_position(),
                                         "Executed_GTID_set", gtid_executed()));

  const auto yaml = shcore::Value(metadata).yaml();
  const auto console = current_console();

  console->println();
  console->println(yaml);

  log_info("%s", yaml.c_str());
}

bool Dump_reader::should_create_pks() const {
  return m_options.should_create_pks(m_contents.create_invisible_pks);
}

std::unique_ptr<shcore::Thread_pool> Dump_reader::create_thread_pool() const {
  auto threads = m_options.threads_count();

  if (!m_dir->is_local()) {
    // in case of remote dumps we're using more threads, in order to compensate
    // for download times
    threads *= 4;
  }

  return std::make_unique<shcore::Thread_pool>(
      m_options.background_threads_count(threads));
}

void Dump_reader::on_table_metadata_parsed(const Table_info &info) {
  const auto partitions = info.data_info.size();
  assert(partitions > 0);

  if (info.data_info[0].has_data) {
    ++m_tables_to_load;
    m_tables_and_partitions_to_load += partitions;

    if (partitions > 1) {
      m_dump_has_partitions = true;
    }
  }

  on_metadata_parsed();
}

bool Dump_reader::include_schema(const std::string &schema) const {
  return m_options.filters().schemas().is_included(override_schema(schema));
}

bool Dump_reader::include_table(const std::string &schema,
                                const std::string &table) const {
  return m_options.filters().tables().is_included(override_schema(schema),
                                                  table);
}

bool Dump_reader::include_event(const std::string &schema,
                                const std::string &event) const {
  return m_options.filters().events().is_included(override_schema(schema),
                                                  event);
}

bool Dump_reader::include_routine(const std::string &schema,
                                  const std::string &routine) const {
  return m_options.filters().routines().is_included(override_schema(schema),
                                                    routine);
}

bool Dump_reader::include_trigger(const std::string &schema,
                                  const std::string &table,
                                  const std::string &trigger) const {
  return m_options.filters().triggers().is_included(override_schema(schema),
                                                    table, trigger);
}

const std::string &Dump_reader::override_schema(const std::string &s) const {
  if (!m_schema_override.has_value()) return s;

  const auto &value = *m_schema_override;
  return value.first == s ? value.second : s;
}

void Dump_reader::on_chunk_loaded(const std::string &schema,
                                  const std::string &table,
                                  const std::string &partition) {
  ++find_partition(schema, table, partition, "chunk was loaded")->chunks_loaded;
}

void Dump_reader::on_index_end(const std::string &schema,
                               const std::string &table) {
  find_table(schema, table, "indexes were created")->indexes_created = true;
}

void Dump_reader::on_analyze_end(const std::string &schema,
                                 const std::string &table) {
  find_table(schema, table, "analysis was finished")->analyze_finished = true;
}

void Dump_reader::on_checksum_end(std::string_view schema,
                                  std::string_view table,
                                  std::string_view partition) {
  ++find_partition(schema, table, partition, "checksum was verified")
        ->checksums_verified;
}

const Dump_reader::Table_info *Dump_reader::find_table(
    std::string_view schema, std::string_view table,
    const char *context) const {
  const auto s = m_contents.schemas.find(
#if __cpp_lib_generic_unordered_lookup
      schema
#else
      std::string(schema)
#endif
  );

  if (s == m_contents.schemas.end()) {
    throw std::logic_error(
        shcore::str_format("Unable to find schema %s whose %s",
                           std::string{schema}.c_str(), context));
  }

  const auto t = s->second->tables.find(
#if __cpp_lib_generic_unordered_lookup
      table
#else
      std::string(table)
#endif
  );

  if (t == s->second->tables.end()) {
    throw std::logic_error(shcore::str_format(
        "Unable to find table %s in schema %s whose %s",
        std::string{table}.c_str(), std::string{schema}.c_str(), context));
  }

  return t->second.get();
}

Dump_reader::Table_info *Dump_reader::find_table(std::string_view schema,
                                                 std::string_view table,
                                                 const char *context) {
  return const_cast<Table_info *>(
      const_cast<const Dump_reader *>(this)->find_table(schema, table,
                                                        context));
}

const Dump_reader::Table_data_info *Dump_reader::find_partition(
    std::string_view schema, std::string_view table, std::string_view partition,
    const char *context) const {
  const auto t = find_table(schema, table, context);

  for (auto &tdi : t->data_info) {
    if (tdi.partition == partition) {
      return &tdi;
    }
  }

  throw std::logic_error(shcore::str_format(
      "Unable to find partition %s of table %s in "
      "schema %s whose %s",
      std::string{partition}.c_str(), std::string{table}.c_str(),
      std::string{schema}.c_str(), context));
}

Dump_reader::Table_data_info *Dump_reader::find_partition(
    std::string_view schema, std::string_view table, std::string_view partition,
    const char *context) {
  return const_cast<Table_data_info *>(
      const_cast<const Dump_reader *>(this)->find_partition(
          schema, table, partition, context));
}

const Dump_reader::View_info *Dump_reader::find_view(
    std::string_view schema, std::string_view view, const char *context) const {
  const auto s = m_contents.schemas.find(
#if __cpp_lib_generic_unordered_lookup
      schema
#else
      std::string(schema)
#endif
  );

  if (s == m_contents.schemas.end()) {
    throw std::logic_error(
        shcore::str_format("Unable to find schema %s whose %s",
                           std::string{schema}.c_str(), context));
  }

  for (const auto &v : s->second->views) {
    if (v.name == view) {
      return &v;
    }
  }

  throw std::logic_error(shcore::str_format(
      "Unable to find view %s in schema %s whose %s", std::string{view}.c_str(),
      std::string{schema}.c_str(), context));
}

Dump_reader::View_info *Dump_reader::find_view(std::string_view schema,
                                               std::string_view view,
                                               const char *context) {
  return const_cast<View_info *>(
      const_cast<const Dump_reader *>(this)->find_view(schema, view, context));
}

bool Dump_reader::table_exists(std::string_view schema,
                               std::string_view table) {
  auto t = find_table(schema, table, "existence is being checked");

  if (!t->exists.has_value()) {
    const auto result = m_options.base_session()->queryf(
        "SELECT table_name FROM information_schema.tables WHERE table_schema = "
        "? AND table_name = ?",
        schema, table);

    t->exists = result->fetch_one();
  }

  return *t->exists;
}

bool Dump_reader::view_exists(std::string_view schema, std::string_view view) {
  auto v = find_view(schema, view, "existence is being checked");

  if (!v->exists.has_value()) {
    const auto result = m_options.base_session()->queryf(
        "SELECT table_name FROM information_schema.views WHERE table_schema = "
        "? AND table_name = ?",
        schema, view);

    v->exists = result->fetch_one();
  }

  return *v->exists;
}

void Dump_reader::set_table_exists(std::string_view schema,
                                   std::string_view table) {
  find_table(schema, table, "existence is being set")->exists = true;
}

void Dump_reader::set_view_exists(std::string_view schema,
                                  std::string_view view) {
  find_view(schema, view, "existence is being set")->exists = true;
}

}  // namespace mysqlsh
