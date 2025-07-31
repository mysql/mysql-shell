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

#include "modules/util/load/dump_reader.h"

#include <mysqld_error.h>

#include <algorithm>
#include <atomic>
#include <functional>
#include <memory>
#include <numeric>
#include <string>
#include <unordered_map>
#include <utility>

#include "modules/util/common/dump/constants.h"
#include "modules/util/common/dump/server_info.h"
#include "modules/util/common/dump/utils.h"
#include "modules/util/common/utils.h"
#include "modules/util/dump/capability.h"
#include "modules/util/dump/compatibility.h"
#include "modules/util/dump/schema_dumper.h"
#include "modules/util/load/load_errors.h"
#include "mysqlshdk/libs/db/mysql/result.h"
#include "mysqlshdk/libs/mysql/utils.h"
#include "mysqlshdk/libs/oci/oci_par.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_json.h"
#include "mysqlshdk/libs/utils/utils_lexing.h"
#include "mysqlshdk/libs/utils/utils_net.h"
#include "mysqlshdk/libs/utils/utils_path.h"
#include "mysqlshdk/libs/utils/utils_sqlstring.h"
#include "mysqlshdk/libs/utils/utils_string.h"

namespace mysqlsh {

namespace {

using dump::common::fetch_file;
using dump::common::fetch_json;
using mysqlshdk::utils::Version;

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
  return parse_metadata(fetch_file(dir->file(fn)), fn);
}

auto to_vector_of_strings(const shcore::Array_t &arr) {
  std::vector<std::string> res;
  res.reserve(arr->size());

  for (const auto &s : *arr) {
    res.emplace_back(s.as_string());
  }

  return res;
}

class Index_file {
 public:
  explicit Index_file(mysqlshdk::storage::IDirectory *dir,
                      const std::string &data_filename) {
    m_idx_file = dir->file(data_filename + ".idx");
  }

  uint64_t data_size() {
    load_metadata();
    return m_data_size;
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
    m_idx_file->close();

    m_data_size = mysqlshdk::utils::network_to_host(m_data_size);

    m_metadata_loaded = true;
  }

  static constexpr uint64_t k_entry_size = sizeof(uint64_t);

  std::unique_ptr<mysqlshdk::storage::IFile> m_idx_file;

  std::size_t m_file_size = 0;

  uint64_t m_entries = 0;

  uint64_t m_data_size = 0;

  bool m_metadata_loaded = false;
};

}  // namespace

Dump_reader::Dump_reader(
    std::unique_ptr<mysqlshdk::storage::IDirectory> dump_dir,
    const Load_dump_options &options)
    : m_dir(std::move(dump_dir)), m_options(options) {}

Dump_reader::Status Dump_reader::open() {
  const auto md = fetch_json(m_dir->file(dump::common::k_root_metadata_file));

  {
    const auto basenames =
        shcore::json::required_unordered_map(md, "basenames");

    for (auto &schema : shcore::json::required_string_array(md, "schemas")) {
      if (include_schema(schema)) {
        auto info = std::make_shared<Schema_info>();

        info->parent = nullptr;
        info->name = std::move(schema);

        if (const auto it = basenames.find(info->name); basenames.end() != it) {
          info->basename = it->second;
        } else {
          info->basename = info->name;
        }

        m_contents.schemas.emplace(info->name, std::move(info));
      } else {
        log_debug("Skipping schema '%s'", schema.c_str());
      }
    }
  }

  m_contents.dump = dump::common::dump_info(md);

  {
    const auto &options = m_contents.dump.compatibility_options;

    m_contents.create_invisible_pks =
        std::find(options.begin(), options.end(), "create_invisible_pks") !=
        options.end();

    m_contents.force_non_standard_fks =
        std::find(options.begin(), options.end(), "force_non_standard_fks") !=
        options.end();
  }

  {
    const auto &capabilities = m_contents.dump.capabilities;

    m_contents.redirect_dump_dir =
        std::find_if(capabilities.begin(), capabilities.end(),
                     [](const auto &c) {
                       return c.capability.has_value() &&
                              mysqlsh::dump::Capability::DUMP_DIR_REDIRECTION ==
                                  *c.capability;
                     }) != capabilities.end();

    m_contents.has_innodb_vector_store =
        std::find_if(capabilities.begin(), capabilities.end(),
                     [](const auto &c) {
                       return c.capability.has_value() &&
                              mysqlsh::dump::Capability::INNODB_VECTOR_STORE ==
                                  *c.capability;
                     }) != capabilities.end();
  }

  if (m_contents.redirect_dump_dir) {
    m_contents.redirected_dump_dir =
        shcore::json::required(md, "redirectedDumpDir", false);
    // dump was redirected to another directory, this is not a valid dump,
    // loader has to load it from that directory
    return Status::INVALID;
  }

  if (m_contents.has_innodb_vector_store) {
    m_contents.innodb_vector_store = dump::common::vector_store_info(
        shcore::json::required_object(md, "vectorStore"));
  }

  // deprecated entry
  m_contents.table_only =
      shcore::json::optional_bool(md, "tableOnly").value_or(false);

  if (m_dir->file(dump::common::k_done_metadata_file)->exists()) {
    m_contents.parse_done_metadata(m_dir.get(), m_options.checksum(),
                                   m_options.session());
    m_dump_status = Status::COMPLETE;
  } else {
    log_info("%s: not found", dump::common::k_done_metadata_file);
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

const Dump_reader::Schema_info *Dump_reader::next_schema() {
  // find the first schema that is ready
  for (auto &it : m_contents.schemas) {
    auto &s = it.second;

    if (!s->schema_sql_done && s->ready()) {
      s->schema_sql_done = true;

      return s.get();
    }
  }

  return nullptr;
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

std::list<Dump_reader::Object_info *> Dump_reader::schemas() {
  std::list<Dump_reader::Object_info *> slist;

  for (const auto &s : m_contents.schemas) {
    slist.push_back(s.second.get());
  }

  return slist;
}

bool Dump_reader::schema_objects(std::string_view schema,
                                 std::list<Object_info *> *out_tables,
                                 std::list<Object_info *> *out_views,
                                 std::list<Object_info *> *out_triggers,
                                 std::list<Object_info *> *out_functions,
                                 std::list<Object_info *> *out_procedures,
                                 std::list<Object_info *> *out_libraries,
                                 std::list<Object_info *> *out_events) {
  const auto schema_info = find_schema(schema, nullptr);

  if (!schema_info) return false;

  {
    const auto clear_out = [](auto *out) {
      if (!out) return;
      out->clear();
    };

    clear_out(out_tables);
    clear_out(out_views);
    clear_out(out_triggers);
    clear_out(out_functions);
    clear_out(out_procedures);
    clear_out(out_libraries);
    clear_out(out_events);
  }

  const auto add_objects = [](auto *src, std::list<Object_info *> *tgt) {
    if (!tgt) return;

    for (auto &s : *src) {
      tgt->emplace_back(&s);
    }
  };

  for (const auto &t : schema_info->tables) {
    if (out_tables) {
      out_tables->push_back(t.second.get());
    }

    add_objects(&t.second->triggers, out_triggers);
  }

  add_objects(&schema_info->views, out_views);
  add_objects(&schema_info->functions, out_functions);
  add_objects(&schema_info->procedures, out_procedures);
  add_objects(&schema_info->libraries, out_libraries);
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

std::string Dump_reader::fetch_schema_script(
    const std::string &schema,
    std::string (Schema_info::*method)() const) const {
  std::string script;

  const auto &s = m_contents.schemas.at(schema);

  if (s->has_sql) {
    // Get the base script for the schema
    script = fetch_file(m_dir->file(std::invoke(method, *s)));
  }

  return script;
}

std::string Dump_reader::fetch_schema_script(const std::string &schema) const {
  return fetch_schema_script(schema, &Schema_info::script_name);
}

std::string Dump_reader::fetch_events_script(const std::string &schema) const {
  return fetch_schema_script(schema, &Schema_info::events_script_name);
}

std::string Dump_reader::fetch_libraries_script(
    const std::string &schema) const {
  return fetch_schema_script(schema, &Schema_info::libraries_script_name);
}

std::string Dump_reader::fetch_routines_script(
    const std::string &schema) const {
  return fetch_schema_script(schema, &Schema_info::routines_script_name);
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
    Table_chunk *out_chunk) {
  auto iter = schedule_chunk_proportionally(
      tables_being_loaded, &m_tables_with_data, m_options.threads_count());

  if (iter != m_tables_with_data.end()) {
    out_chunk->schema = (*iter)->table->parent->name;
    out_chunk->table = (*iter)->table->name;
    out_chunk->partition = (*iter)->partition;
    out_chunk->chunked = (*iter)->chunked;
    out_chunk->index = (*iter)->chunks_consumed;

    if ((*iter)->last_chunk_seen) {
      out_chunk->chunks_total = (*iter)->available_chunks.size();
    } else {
      out_chunk->chunks_total = 0;
    }

    const auto &info = (*iter)->available_chunks[out_chunk->index];

    if (!info.has_value()) {
      throw std::logic_error(
          "Trying to use chunk " + std::to_string(out_chunk->index) + " of " +
          schema_table_object_key(out_chunk->schema, out_chunk->table,
                                  out_chunk->partition) +
          " which is not yet available");
    }

    out_chunk->compression = (*iter)->table->compression;

    {
      auto dir = m_dir.get();

      if (m_vector_store_dir && (*iter)->table->is_innodb_vector_store) {
        // if m_vector_store_dir is set and we're here, we're loading a vector
        // store dump with 'convertInnoDbVectorStore' set to 'keep'
        dir = m_vector_store_dir.get();
      }

      out_chunk->file = mysqlshdk::storage::make_file(dir->file(info->name()),
                                                      out_chunk->compression);
    }

    out_chunk->file_size = info->size();
    out_chunk->data_size = data_size_in_file(info->name());
    out_chunk->options = (*iter)->table->options;
    out_chunk->dump_complete = (*iter)->data_dumped();
    out_chunk->basename = (*iter)->basename;
    out_chunk->extension = (*iter)->extension;

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
          (!m_options.load_data() ||
           table.second->is_secondary_load_completed()) &&
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
      if ((!m_options.load_data() ||
           (table.second->all_data_loaded() &&
            table.second->is_secondary_load_completed())) &&
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
              (!m_options.load_data() ||
               (partition.data_loaded() &&
                table.second->is_secondary_load_completed()))) {
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

bool Dump_reader::next_secondary_load(std::string *out_schema,
                                      std::string *out_table) {
  assert(out_schema);
  assert(out_table);

  if (!has_innodb_vector_store()) {
    return false;
  }

  for (auto &schema : m_contents.schemas) {
    for (auto &table : schema.second->tables) {
      if (!table.second->is_secondary_load_completed() &&
          !table.second->secondary_load_scheduled) {
        table.second->secondary_load_scheduled = true;
        *out_schema = schema.second->name;
        *out_table = table.second->name;
        return true;
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
      if ((m_options.load_data() &&
           (!table.second->all_data_scheduled() ||
            !table.second->secondary_load_scheduled)) ||
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
        if (!table.second->data_info.empty() &&
            table.second->data_info[0].has_data) {
          const auto t = s->second.find(table.second->name);

          if (t != s->second.end()) {
            m_filtered_data_size += t->second;
          }
        }
      }
    }
  }
}

// Scan directory for new files and adds them to the pending file list
void Dump_reader::rescan(dump::Progress_thread *progress_thread) {
  Files files;
  Files vector_store_files;

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

    files = std::move(m_dir->list().files);

    if (m_vector_store_dir) {
      vector_store_files = std::move(m_vector_store_dir->list().files);
    }
  }

  log_debug("Finished listing files, starting rescan");
  m_contents.rescan(m_dir.get(), files, this, progress_thread);
  log_debug("Rescan done");

  if (!vector_store_files.empty()) {
    log_debug("Scanning for vector store data");
    m_contents.rescan_data(vector_store_files, this);
    log_debug("Vector store data scan done");
  }

  if (m_dump_status != Status::COMPLETE &&
      files.find({dump::common::k_done_metadata_file}) != files.end()) {
    m_contents.parse_done_metadata(m_dir.get(), m_options.checksum(),
                                   m_options.session());
    m_dump_status = Status::COMPLETE;
  }

  compute_filtered_data_size();
}

uint64_t Dump_reader::add_deferred_statements(
    std::string_view schema, std::string_view table,
    compatibility::Deferred_statements &&stmts) {
  const auto t = find_table(schema, table, "index will be added");

  // if indexes are not going to be recreated, we're marking them as already
  // created
  t->indexes_scheduled = t->indexes_created =
      !m_options.load_deferred_indexes() || stmts.index_info.empty();
  t->indexes = std::move(stmts.index_info);

  const auto table_name = schema_object_key(schema, table);
  const auto s = find_schema(schema, "index will be added");

  for (const auto &fk : stmts.foreign_keys) {
    s->foreign_key_queries.emplace_back("ALTER TABLE " + table_name + " ADD " +
                                        fk);
  }

  if (!stmts.secondary_engine.empty()) {
    s->queries_on_schema_end.emplace_back(std::move(stmts.secondary_engine));
  }

  return t->indexes.size();
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

  for (auto routines : {&info->functions, &info->procedures}) {
    for (auto &r : *routines) {
      for (auto &dep : r.dependencies) {
        if (dep.schema == info->name) {
          dep.schema = schema;
        }
      }
    }
  }

  m_schema_override = {schema, info->name};
  info->name = schema;

  for (const auto &table : info->tables) {
    table.second->options->set("schema", shcore::Value(schema));
  }
}

void Dump_reader::validate_options() {
  if (m_options.load_users() && !m_contents.dump.has_users) {
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
          common::member_name("dumpTables") +
          "() function and needs to be loaded into an existing schema. "
          "Please set the target schema using the 'schema' option.");

      throw std::invalid_argument("The target schema was not specified.");
    } else {
      const auto result = m_options.session()->queryf(
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
    if (!m_contents.dump.has_checksum) {
      throw std::invalid_argument(
          "The 'checksum' option cannot be used when the dump does not contain "
          "checksum information.");
    }

    if (m_options.dry_run()) {
      current_console()->print_warning(
          "Checksum information is not going to be verified, dryRun enabled.");
    }
  }

  validate_vector_store_options();
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
  di.table = this;

  has_sql = md->get_bool("includesDdl", true);
  di.has_data = md->get_bool("includesData", true);

  options = md->get_map("options");

  std::string compression_type;

  if (options) {
    // "compression" and "primaryIndex" are not used by the chunk importer
    // and need to be removed
    // these were misplaced in the options dictionary, code is kept for
    // backward compatibility

    if (options->has_key("compression")) {
      compression_type = options->get_string("compression");
      options->erase("compression");
    }

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

  if (md->has_key("compression")) {
    compression_type = md->get_string("compression");
  }

  if (compression_type.empty()) {
    // extension is either like 'tsv', where split_extension() will return an
    // empty string, or 'tsv.zst' where it will return '.zst'
    compression = mysqlshdk::storage::from_file_path(di.extension);
  } else {
    compression = mysqlshdk::storage::to_compression(compression_type);
  }

  if (md->has_key("primaryIndex")) {
    primary_index = to_vector_of_strings(md->get_array("primaryIndex"));
  }

  if (const auto trigger_list = md->get_array("triggers")) {
    for (const auto &t : *trigger_list) {
      auto trigger_name = t.as_string();

      if (reader->include_trigger(parent->name, name, trigger_name)) {
        triggers.emplace_back(Object_info{this, std::move(trigger_name)});
      }
    }

    has_triggers = !triggers.empty();

    log_debug("%s.%s has %zi triggers", parent->name.c_str(), name.c_str(),
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

  is_innodb_vector_store = md->get_bool("isVectorStoreTable", false);

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
                                 this](auto &&...params) {
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
    reader->m_contents.total_file_size += it->size();
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

  const auto list =
      info->find_checksums(table->parent->name, table->name, partition);

  if (list.empty()) {
    log_warning(
        "Could not find checksum information of: %s",
        schema_table_object_key(table->parent->name, table->name, partition)
            .c_str());
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

std::string Dump_reader::Schema_info::events_script_name() const {
  assert(multifile_sql);
  return dump::common::get_schema_filename(basename, "events.sql");
}

std::string Dump_reader::Schema_info::libraries_script_name() const {
  assert(multifile_sql);
  return dump::common::get_schema_filename(basename, "libraries.sql");
}

std::string Dump_reader::Schema_info::routines_script_name() const {
  assert(multifile_sql);
  return dump::common::get_schema_filename(basename, "routines.sql");
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

  multifile_sql = md->get_bool("multifileDdl", false);

  shcore::Dictionary_t basenames = md->get_map("basenames");

  if (const auto table_list = md->get_array("tables")) {
    for (const auto &t : *table_list) {
      auto table_name = t.as_string();

      if (reader->include_table(name, table_name)) {
        auto info = std::make_shared<Table_info>();

        info->parent = this;
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

        info.parent = this;
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

  for (const std::string routine_type : {"function", "procedure"}) {
    if (const auto routine_list = md->get_array(routine_type + "s")) {
      const auto routine_deps = md->get_map(routine_type + "Dependencies");
      auto &target = "function" == routine_type ? functions : procedures;

      for (const auto &r : *routine_list) {
        auto routine_name = r.as_string();

        if (reader->include_routine(name, routine_name)) {
          Routine_info info;
          info.parent = this;
          info.name = std::move(routine_name);

          if (const auto deps = routine_deps
                                    ? routine_deps->get_array(info.name)
                                    : shcore::Array_t{}) {
            for (const auto &dep : *deps) {
              const auto lib = dep.as_map();

              info.dependencies.emplace_back(Routine_info::Dependency{
                  lib->get_string("schema"), lib->get_string("library")});
            }
          }

          target.emplace_back(std::move(info));
        }
      }

      log_debug("%s has %zi %ss", name.c_str(), target.size(),
                routine_type.c_str());
    }
  }

  if (const auto library_list = md->get_array("libraries")) {
    for (const auto &l : *library_list) {
      auto library_name = l.as_string();

      if (reader->include_library(name, library_name)) {
        libraries.emplace_back(Object_info{this, std::move(library_name)});
      }
    }

    log_debug("%s has %zi libraries", name.c_str(), libraries.size());
  }

  if (const auto event_list = md->get_array("events")) {
    for (const auto &e : *event_list) {
      auto event_name = e.as_string();

      if (reader->include_event(name, event_name)) {
        events.emplace_back(Object_info{this, std::move(event_name)});
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
                return fetch_file(dir->file(mdpath));
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

  // check for the sql file(s) for the schema
  if (!sql_seen) {
    if (files.contains(script_name())) {
      if (!multifile_sql ||
          ((events.empty() || files.contains(events_script_name())) &&
           (libraries.empty() || files.contains(libraries_script_name())) &&
           ((functions.empty() && procedures.empty()) ||
            files.contains(routines_script_name())))) {
        sql_seen = true;
      }
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
  if (!sql && files.find({dump::common::k_preamble_sql_file}) != files.end()) {
    sql = std::make_unique<std::string>(
        fetch_file(dir->file(dump::common::k_preamble_sql_file)));
  }
  if (!post_sql &&
      files.find({dump::common::k_postamble_sql_file}) != files.end()) {
    post_sql = std::make_unique<std::string>(
        fetch_file(dir->file(dump::common::k_postamble_sql_file)));
  }
  if (dump.has_users && !users_sql &&
      files.find({dump::common::k_users_sql_file}) != files.end()) {
    users_sql = std::make_unique<std::string>(
        fetch_file(dir->file(dump::common::k_users_sql_file)));
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
              return fetch_file(dir->file(mdpath));
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

  if (const auto metadata =
          fetch_metadata(dir, dump::common::k_done_metadata_file)) {
    if (metadata->has_key("dataBytes")) {
      total_data_size = metadata->get_uint("dataBytes");
    } else {
      log_warning(
          "Dump metadata file %s does not contain dataBytes information",
          dump::common::k_done_metadata_file);
    }

    if (metadata->has_key("tableDataBytes")) {
      for (const auto &schema : *metadata->get_map("tableDataBytes")) {
        for (const auto &table : *schema.second.as_map()) {
          table_data_size[schema.first][table.first] = table.second.as_uint();
        }
      }
    } else {
      log_warning(
          "Dump metadata file %s does not contain tableDataBytes information",
          dump::common::k_done_metadata_file);
    }

    // only exists in 1.0.1+
    if (metadata->has_key("chunkFileBytes")) {
      for (const auto &file : *metadata->get_map("chunkFileBytes")) {
        chunk_data_sizes[file.first] = file.second.as_uint();
      }
    }

    if (metadata->has_key("issues")) {
      const auto issues = metadata->get_map("issues");

      if (const auto issue = issues->find("hasInvalidViewReferences");
          issues->end() != issue) {
        has_invalid_view_references = issue->second.as_bool();
      }
    }
  } else {
    log_warning("Dump metadata file %s is invalid",
                dump::common::k_done_metadata_file);
  }

  if (get_checksum && dump.has_checksum) {
    initialize_checksums(dir, session);
  }
}

void Dump_reader::Dump_info::initialize_checksums(
    mysqlshdk::storage::IDirectory *dir,
    const std::shared_ptr<mysqlshdk::db::ISession> &session) {
  checksum = std::make_unique<dump::common::Checksums>();
  checksum->deserialize(dir->file(dump::common::k_checksums_metadata_file));
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

  if (info.is_innodb_vector_store) {
    ++m_vector_store_tables_to_load;
  }

  // we update these stats only if we're going to actually load the data into
  // this table; if table is a vector store table, we're going to do that only
  // when it's not going to be converted to Lakehouse (or skipped)
  if (!info.is_innodb_vector_store ||
      load::Convert_vector_store::KEEP == m_options.convert_vector_store()) {
    if (info.data_info[0].has_data) {
      ++m_tables_to_load;
      m_tables_and_partitions_to_load += partitions;

      if (partitions > 1) {
        m_dump_has_partitions = true;
      }
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

bool Dump_reader::include_library(const std::string &schema,
                                  const std::string &library) const {
  return m_options.filters().libraries().is_included(override_schema(schema),
                                                     library);
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

bool Dump_reader::on_chunk_loaded(const Table_chunk &chunk) {
  const auto p = find_partition(chunk.schema, chunk.table, chunk.partition,
                                "chunk was loaded");

  ++p->chunks_loaded;
  return p->data_loaded();
}

void Dump_reader::on_table_loaded(const Table_chunk &chunk) {
  const auto tdi = find_partition(chunk.schema, chunk.table, chunk.partition,
                                  "table data was loaded");
  assert(tdi->data_dumped());
  tdi->chunks_loaded = tdi->available_chunks.size();
}

void Dump_reader::on_table_ddl_end(const std::string &schema,
                                   const std::string &table) {
  find_table(schema, table, "DDL was loaded")->sql_loaded = true;
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

void Dump_reader::on_secondary_load_end(std::string_view schema,
                                        std::string_view table) {
  find_table(schema, table, "secondary load was finished")
      ->secondary_load_completed = true;
}

const Dump_reader::Schema_info *Dump_reader::find_schema(
    std::string_view schema, const char *context) const {
  const auto s = m_contents.schemas.find(
#if __cpp_lib_generic_unordered_lookup
      schema
#else
      std::string(schema)
#endif
  );

  if (s == m_contents.schemas.end()) {
    if (context) {
      throw std::logic_error(
          shcore::str_format("Unable to find schema %s whose %s",
                             std::string{schema}.c_str(), context));
    } else {
      return nullptr;
    }
  }

  return s->second.get();
}

Dump_reader::Schema_info *Dump_reader::find_schema(std::string_view schema,
                                                   const char *context) {
  return const_cast<Schema_info *>(
      const_cast<const Dump_reader *>(this)->find_schema(schema, context));
}

const Dump_reader::Table_info *Dump_reader::find_table(
    std::string_view schema, std::string_view table,
    const char *context) const {
  const auto s = find_schema(schema, context);

  if (!s) {
    return nullptr;
  }

  const auto t = s->tables.find(
#if __cpp_lib_generic_unordered_lookup
      table
#else
      std::string(table)
#endif
  );

  if (t == s->tables.end()) {
    if (context) {
      throw std::logic_error(shcore::str_format(
          "Unable to find table %s in schema %s whose %s",
          std::string{table}.c_str(), std::string{schema}.c_str(), context));
    } else {
      return nullptr;
    }
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

  if (!t) {
    return nullptr;
  }

  for (auto &tdi : t->data_info) {
    if (tdi.partition == partition) {
      return &tdi;
    }
  }

  if (context) {
    throw std::logic_error(shcore::str_format(
        "Unable to find partition %s of table %s in schema %s whose %s",
        std::string{partition}.c_str(), std::string{table}.c_str(),
        std::string{schema}.c_str(), context));
  } else {
    return nullptr;
  }
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
  return find_schema_object(schema, view, &Schema_info::views, "view", context);
}

Dump_reader::View_info *Dump_reader::find_view(std::string_view schema,
                                               std::string_view view,
                                               const char *context) {
  return const_cast<View_info *>(
      const_cast<const Dump_reader *>(this)->find_view(schema, view, context));
}

template <typename T>
const T *Dump_reader::find_schema_object(std::string_view schema,
                                         std::string_view object,
                                         std::vector<T> Schema_info::*member,
                                         const char *type,
                                         const char *context) const {
  const auto s = find_schema(schema, context);

  if (!s) {
    return nullptr;
  }

  for (const auto &m : (*s).*member) {
    if (m.name == object) {
      return &m;
    }
  }

  if (context) {
    throw std::logic_error(shcore::str_format(
        "Unable to find %s %s in schema %s whose %s", type,
        std::string{object}.c_str(), std::string{schema}.c_str(), context));
  } else {
    return nullptr;
  }
}

template <typename T>
T *Dump_reader::find_schema_object(std::string_view schema,
                                   std::string_view object,
                                   std::vector<T> Schema_info::*member,
                                   const char *type, const char *context) {
  return const_cast<T *>(
      const_cast<const Dump_reader *>(this)->find_schema_object(
          schema, object, member, type, context));
}

bool Dump_reader::schema_exists(
    std::string_view schema,
    const std::shared_ptr<mysqlshdk::db::ISession> &session) {
  auto s = find_schema(schema, "existence is being checked");

  if (!s->exists.has_value()) {
    try {
      session->executef("USE !", schema);
      s->exists = true;
    } catch (const mysqlshdk::db::Error &e) {
      if (ER_BAD_DB_ERROR == e.code()) {
        s->exists = false;
      } else {
        throw;
      }
    }
  }

  return *s->exists;
}

bool Dump_reader::table_exists(
    std::string_view schema, std::string_view table,
    const std::shared_ptr<mysqlshdk::db::ISession> &session) {
  auto t = find_table(schema, table, "existence is being checked");

  if (!t->exists.has_value()) {
    try {
      session->queryf("SELECT 1 FROM !.! LIMIT 1", schema, table);
      t->exists = true;
    } catch (const mysqlshdk::db::Error &e) {
      if (ER_BAD_DB_ERROR == e.code() || ER_NO_SUCH_TABLE == e.code()) {
        t->exists = false;
      } else {
        throw;
      }
    }
  }

  return *t->exists;
}

bool Dump_reader::view_exists(
    std::string_view schema, std::string_view view,
    const std::shared_ptr<mysqlshdk::db::ISession> &session) {
  auto v = find_view(schema, view, "existence is being checked");

  if (!v->exists.has_value()) {
    try {
      session->queryf("SELECT 1 FROM !.! LIMIT 1", schema, view);
      v->exists = true;
    } catch (const mysqlshdk::db::Error &e) {
      if (ER_BAD_DB_ERROR == e.code() || ER_NO_SUCH_TABLE == e.code()) {
        v->exists = false;
      } else {
        throw;
      }
    }
  }

  return *v->exists;
}

void Dump_reader::set_schema_exists(std::string_view schema) {
  find_schema(schema, "existence is being set")->exists = true;
}

void Dump_reader::set_table_exists(std::string_view schema,
                                   std::string_view table) {
  find_table(schema, table, "existence is being set")->exists = true;
}

void Dump_reader::set_view_exists(std::string_view schema,
                                  std::string_view view) {
  find_view(schema, view, "existence is being set")->exists = true;
}

void Dump_reader::consume_table(const Table_chunk &chunk) {
  const auto tdi = find_partition(chunk.schema, chunk.table, chunk.partition,
                                  "table data will be loaded");
  assert(tdi->data_dumped());
  tdi->consume_table();
  m_tables_with_data.erase(tdi);
}

size_t Dump_reader::partition_file_size(const std::string &schema,
                                        const std::string &table,
                                        const std::string &partition) const {
  const auto tdi = find_partition(schema, table, partition,
                                  "total file size will be calculated");
  assert(tdi->data_dumped());
  size_t size = 0;

  for (const auto &file : tdi->available_chunks) {
    if (!file.has_value()) {
      throw std::logic_error("Trying to use chunk of " +
                             schema_table_object_key(schema, table, partition) +
                             " which is not yet available");
    }

    size += file->size();
  }

  return size;
}

size_t Dump_reader::partition_data_size(const std::string &schema,
                                        const std::string &table,
                                        const std::string &partition) const {
  const auto tdi = find_partition(schema, table, partition,
                                  "total data size will be calculated");
  assert(tdi->data_dumped());
  size_t size = 0;

  for (const auto &file : tdi->available_chunks) {
    if (!file.has_value()) {
      throw std::logic_error("Trying to use chunk of " +
                             schema_table_object_key(schema, table, partition) +
                             " which is not yet available");
    }

    size += data_size_in_file(file->name());
  }

  return size;
}

uint64_t Dump_reader::data_size_in_file(const std::string &filename) const {
  if (const auto it = m_contents.chunk_data_sizes.find(filename);
      m_contents.chunk_data_sizes.end() != it) {
    return it->second;
  }

  try {
    // @.done.json not there yet, use the idx file
    return Index_file{m_dir.get(), filename}.data_size();
  } catch (const std::exception &) {
    // index file is not present when doing a copy
    return 0;
  }
}

bool Dump_reader::has_library_ddl() const noexcept {
  return m_contents.dump.has_library_ddl;
}

void Dump_reader::exclude_routines_with_missing_dependencies(
    const std::shared_ptr<mysqlshdk::db::ISession> &session) {
  if (!m_contents.dump.has_library_ddl) {
    return;
  }

  // schema -> library -> exists
  std::unordered_map<std::string, std::unordered_map<std::string, bool>>
      libraries;
  const auto library_exists = [this, &libraries,
                               &session](const Routine_info::Dependency &dep) {
    // check in cache first
    if (const auto schema = libraries.find(dep.schema);
        libraries.end() != schema) {
      if (const auto library = schema->second.find(dep.library);
          schema->second.end() != library) {
        return library->second;
      }
    }

    bool exists = false;

    if (find_schema_object(dep.schema, dep.library, &Schema_info::libraries,
                           "library", nullptr)) {
      // library is included in the dump and will be created at some point
      exists = true;
    } else {
      // run a query to check if library exists
      exists = session
                   ->queryf(
                       "SELECT 1 FROM information_schema.libraries WHERE "
                       "library_schema=? AND library_name=?",
                       dep.schema, dep.library)
                   ->fetch_one();
    }

    // cache the value
    libraries[dep.schema][dep.library] = exists;

    return exists;
  };
  const auto exclude_routine = [this](const std::string &schema,
                                      const std::string &routine) {
    const_cast<Load_dump_options &>(m_options).filters().routines().exclude(
        schema, routine);
  };
  const auto console = current_console();

  for (auto &schema : m_contents.schemas) {
    for (auto routines :
         {&schema.second->functions, &schema.second->procedures}) {
      const auto type =
          routines == &schema.second->functions ? "function" : "procedure";

      for (auto routine = routines->begin(); routines->end() != routine;) {
        bool include = true;

        for (const auto &dependency : routine->dependencies) {
          if (!library_exists(dependency)) {
            include = false;
            console->print_note(shcore::str_format(
                "The %s %s uses library %s which does not exist.", type,
                schema_object_key(schema.first, routine->name).c_str(),
                schema_object_key(dependency.schema, dependency.library)
                    .c_str()));
          }
        }

        if (include) {
          ++routine;
        } else {
          console->print_note(shcore::str_format(
              "Skipping the %s %s due to missing dependencies.", type,
              schema_object_key(schema.first, routine->name).c_str()));
          exclude_routine(schema.first, routine->name);
          routine = routines->erase(routine);
        }
      }
    }
  }
}

void Dump_reader::validate_vector_store_options() {
  const auto console = current_console();

  if (!has_innodb_vector_store()) {
    // WL16802-FR2.1.3: print a note if option is set, but dump does not have
    // vector store tables
    if (m_options.has_convert_vector_store_option()) {
      console->print_note(
          "The 'convertInnoDbVectorStore' option is set, but dump does not "
          "contain InnoDB based vector store tables.");
    }

    // WL16802-FR2.2.2: print a note if option is set, but dump does not have
    // vector store tables
    if (m_options.has_lakehouse_source_option()) {
      console->print_note(
          "The 'lakehouseSource' option is set, but dump does not contain "
          "InnoDB based vector store tables.");
    }

    if (m_options.has_heatwave_load_option()) {
      console->print_note(
          "The 'heatwaveLoad' option is set, but dump does not contain InnoDB "
          "based vector store tables.");
    }

    // other checks are not needed
    return;
  }

  // WL16802-FR2.1.4: if set to AUTO, choose the appropriate mode
  if (load::Convert_vector_store::AUTO == m_options.convert_vector_store()) {
    load::Convert_vector_store mode = load::Convert_vector_store::KEEP;

    // WL16802-FR2.1.4.1-4: if instance is MHS, has supported version, and the
    // location of vector store data in OCI is known, convert tables to
    // lakehouse
    if (m_options.is_mds() &&
        compatibility::supports_vector_store_conversion(
            m_options.target_server_version()) &&
        (m_options.has_lakehouse_source_option() ||
         m_contents.innodb_vector_store.resource_principals.valid())) {
      mode = load::Convert_vector_store::CONVERT;
    }

    log_info(
        "The 'convertInnoDbVectorStore' option has been automatically set to: "
        "'%s'",
        load::to_string(mode).c_str());

    const_cast<Load_dump_options &>(m_options).set_convert_vector_store(mode);
  }

  if (load::Convert_vector_store::KEEP == m_options.convert_vector_store()) {
    // we can only load the vector store data if metadata contains its location
    if (!m_contents.innodb_vector_store.relative_path.has_value()) {
      if (m_options.load_data()) {
        console->print_error(
            "The dump contains InnoDB based vector store tables, but the "
            "location of their data is not known, cannot load them as regular "
            "tables. Set the 'convertInnoDbVectorStore' to 'skip' to ignore "
            "them and continue with the load operation.");
        THROW_ERROR(SHERR_LOAD_INNODB_VECTOR_STORE_UNKNOWN_LOCATION);
      }
    } else {
      // WL16802-FR2.1.6: keep vector store tables as is
      if (!m_contents.innodb_vector_store.relative_path->empty()) {
        m_vector_store_dir =
            m_dir->directory(*m_contents.innodb_vector_store.relative_path);
      } else {
        // relative path is empty, vector store data is in the same location as
        // the main dump
      }
    }
  } else if (load::Convert_vector_store::CONVERT ==
             m_options.convert_vector_store()) {
    // WL16802-FR2.1.7.1: throw when user wants to covert to Lakehouse, but
    // target instance is not MHS
    if (!m_options.is_mds()) {
      THROW_ERROR(SHERR_LOAD_INNODB_VECTOR_STORE_NOT_MHS);
    }

    // WL16802-FR2.1.7.2: throw when MHS version is not supported
    if (!compatibility::supports_vector_store_conversion(
            m_options.target_server_version())) {
      THROW_ERROR(SHERR_LOAD_INNODB_VECTOR_STORE_UNSUPPORTED_MHS);
    }

    // WL16802-FR2.1.7.3: throw when user wants to covert to Lakehouse, but
    // vector store data is not in OS
    if (!(m_options.has_lakehouse_source_option() ||
          m_contents.innodb_vector_store.resource_principals.valid())) {
      console->print_error(
          "The dump contains InnoDB based vector store tables, their data is "
          "not available in OCI's Object Storage, cannot convert them to "
          "Lakehouse tables. Set the 'lakehouseSource' to a valid value to "
          "continue with the load operation.");
      THROW_ERROR(SHERR_LOAD_INNODB_VECTOR_STORE_NON_OCI_LOCATION);
    }

    // WL16802-FR2.3.5.3: Lakehouse is not configured, don't load the data
    if (load::Heatwave_load::VECTOR_STORE == m_options.heatwave_load() &&
        !m_options.is_lakehouse_enabled()) {
      console->print_warning(
          "The dump contains InnoDB based vector store tables, but target "
          "instance doesn't have the Lakehouse storage configured. Data will "
          "not be loaded into these tables.");

      const_cast<Load_dump_options &>(m_options).set_heatwave_load(
          load::Heatwave_load::NONE);
    }
  }

  if (load::Convert_vector_store::CONVERT != m_options.convert_vector_store() &&
      m_options.has_heatwave_load_option() &&
      load::Heatwave_load::VECTOR_STORE == m_options.heatwave_load()) {
    console->print_note(
        "The dump contains InnoDB based vector store tables which are not "
        "going to be converted to Lakehouse. The 'heatwaveLoad' option is "
        "ignored.");
  }
}

void Dump_reader::handle_vector_store_dump() {
  if (!has_innodb_vector_store()) {
    return;
  }

  if (load::Convert_vector_store::SKIP == m_options.convert_vector_store()) {
    // WL16802-FR2.1.5: don't load vector store tables
    const auto exclude_table = [this](const std::string &schema,
                                      const std::string &table) {
      const_cast<Load_dump_options &>(m_options).filters().tables().exclude(
          schema, table);
    };

    std::vector<std::reference_wrapper<std::string>> table_names;

    // iterate through tables, remove the ones which are using vector store
    for (auto &schema : m_contents.schemas) {
      table_names.clear();

      for (const auto &table : schema.second->tables) {
        if (table.second->is_innodb_vector_store) {
          table_names.emplace_back(table.second->name);
        }
      }

      for (const auto &name : table_names) {
        // make sure that this table is excluded from further processing
        exclude_table(schema.first, name);
        // remove the table metadata
        schema.second->tables.erase(name);
      }
    }

    // update the size estimates
    compute_filtered_data_size();
  } else if (load::Convert_vector_store::CONVERT ==
             m_options.convert_vector_store()) {
    // WL16802-FR2.1.7: convert vector store tables
    // WL16802-FR2.3.5: tables are being converted, load them into HeatWave if
    // heatwaveLoad is set to 'vector_store'
    const auto secondary_load =
        load::Heatwave_load::VECTOR_STORE == m_options.heatwave_load();

    // iterate through tables, mark the ones which are using vector store as not
    // having any data
    for (auto &schema : m_contents.schemas) {
      for (auto &table : schema.second->tables) {
        auto &t = table.second;

        if (t->is_innodb_vector_store) {
          for (auto &data_info : t->data_info) {
            data_info.has_data = false;
          }

          t->secondary_load = secondary_load;
          t->secondary_load_scheduled = !secondary_load;
          t->secondary_load_completed = !secondary_load;
        }
      }
    }

    // update the size estimates
    compute_filtered_data_size();
  }
}

namespace {

std::string escape_regex(std::string_view s) {
  static constexpr std::string_view k_regex_metachars = R"(\^|.$?*+()[]{})";

  std::string result;
  result.reserve(s.length());

  for (const auto c : s) {
    if (std::string_view::npos != k_regex_metachars.find(c)) {
      result += '\\';
    }

    result += c;
  }

  return result;
}

}  // namespace

std::string Dump_reader::get_vector_store_engine_attribute(
    std::string_view schema, std::string_view table) const {
  if (!has_innodb_vector_store() ||
      load::Convert_vector_store::CONVERT != m_options.convert_vector_store()) {
    return {};
  }

  const auto t = find_table(schema, table, "engine attributes are checked");

  if (!t->is_innodb_vector_store) {
    return {};
  }

  if (t->options->get_bool("fieldsOptionallyEnclosed")) {
    throw std::runtime_error(shcore::str_format(
        "Cannot convert table `%s`.`%s` to Lakehouse, data files are using "
        "dialect where fields are optionally enclosed with '%s', this is not "
        "supported.",
        std::string{schema}.c_str(), std::string{table}.c_str(),
        t->options->get_string("fieldsEnclosedBy").c_str()));
  }

  // WL16802-FR2.1.7.4: construct a Lakehouse file definition entry pointing to
  // vector store data of this table

  shcore::JSON_dumper json;
  json.start_object();  // global

  json.append("file");
  json.start_array();   // file
  json.start_object();  // file

  std::string prefix;

  if (!m_options.lakehouse_source().par().empty()) {
    mysqlshdk::oci::PAR_structure par;
    mysqlshdk::oci::parse_par(m_options.lakehouse_source().par(), &par);

    assert(mysqlshdk::oci::PAR_type::PREFIX == par.type());

    json.append("par", par.par_url());
    prefix = par.object_prefix();
  } else {
    // WL16802-FR2.2.4: if lakehouseSource option is not given, location is read
    // from metadata
    const auto &rp = m_options.lakehouse_source().resource_principals().valid()
                         ? m_options.lakehouse_source().resource_principals()
                         : m_contents.innodb_vector_store.resource_principals;

    assert(rp.valid());

    json.append("bucket", rp.bucket);
    json.append("namespace", rp.namespace_);
    json.append("region", rp.region);

    prefix = rp.prefix();
  }

  json.append("pattern", '^' + escape_regex(prefix) +
                             escape_regex(t->basename) + "@[@]?\\d+\\." +
                             escape_regex(t->data_info[0].extension) + '$');
  json.append("allow_missing_files", false);
  json.append("is_strict_mode", true);

  json.end_object();  // file
  json.end_array();   // file

  json.append("dialect");
  json.start_object();  // dialect

  json.append("field_delimiter", t->options->get_string("fieldsTerminatedBy"));
  json.append("record_delimiter", t->options->get_string("linesTerminatedBy"));
  json.append("escape_character", t->options->get_string("fieldsEscapedBy"));
  json.append("quotation_marks", t->options->get_string("fieldsEnclosedBy"));

  json.append("encoding", t->options->get_string("characterSet"));

  switch (t->compression) {
    case mysqlshdk::storage::Compression::GZIP:
      json.append("compression", "gzip");
      break;

    case mysqlshdk::storage::Compression::NONE:
      break;

    default:
      throw std::runtime_error(shcore::str_format(
          "Cannot convert table `%s`.`%s` to Lakehouse, data files are "
          "compressed with unsupported compression '%s'.",
          std::string{schema}.c_str(), std::string{table}.c_str(),
          mysqlshdk::storage::to_string(t->compression).c_str()));
  }

  json.end_object();  // dialect

  json.end_object();  // global

  return json.str();
}

size_t Dump_reader::tables_to_secondary_load() const {
  uint64_t result = 0;

  if (has_innodb_vector_store()) {
    result += vector_store_tables_to_load();
  }

  return result;
}

uint64_t Dump_reader::vector_store_tables_loaded() const {
  if (!has_innodb_vector_store()) {
    return 0;
  }

  uint64_t result = 0;

  for (const auto &schema : m_contents.schemas) {
    for (const auto &table : schema.second->tables) {
      const auto &t = table.second;

      if (t->is_innodb_vector_store && t->sql_loaded) {
        ++result;
      }
    }
  }

  return result;
}

bool Dump_reader::all_secondary_loads_scheduled() const {
  if (!has_innodb_vector_store()) {
    return true;
  }

  if (Status::COMPLETE != m_dump_status) {
    return false;
  }

  for (const auto &schema : m_contents.schemas) {
    for (const auto &table : schema.second->tables) {
      if (!table.second->secondary_load_scheduled) {
        return false;
      }
    }
  }

  return true;
}

}  // namespace mysqlsh
