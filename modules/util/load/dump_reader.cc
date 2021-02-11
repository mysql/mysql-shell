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

#include "modules/util/load/dump_reader.h"
#include <algorithm>
#include <numeric>
#include <utility>
#include "modules/util/dump/dump_utils.h"
#include "modules/util/dump/schema_dumper.h"
#include "mysqlshdk/libs/utils/utils_lexing.h"
#include "mysqlshdk/libs/utils/utils_path.h"
#include "mysqlshdk/libs/utils/utils_sqlstring.h"
#include "mysqlshdk/libs/utils/utils_string.h"

namespace mysqlsh {

namespace {

std::string fetch_file(mysqlshdk::storage::IDirectory *dir,
                       const std::string &fn) {
  auto file = dir->file(fn);
  file->open(mysqlshdk::storage::Mode::READ);

  std::string data = mysqlshdk::storage::read_file(file.get());

  file->close();

  return data;
}

shcore::Dictionary_t fetch_metadata(mysqlshdk::storage::IDirectory *dir,
                                    const std::string &fn) {
  std::string data = fetch_file(dir, fn);

  try {
    auto metadata = shcore::Value::parse(data);
    if (metadata.type != shcore::Map) {
      throw std::runtime_error("Invalid metadata file " + fn);
    }
    return metadata.as_map();
  } catch (const shcore::Exception &e) {
    throw shcore::Exception::runtime_error("Could not parse metadata file " +
                                           fn + ": " + e.format());
  }
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
    if (m_options.include_schema(schema.as_string()) ||
        m_options.include_table(schema.as_string(), "")) {
      auto info = std::make_shared<Schema_info>();

      info->schema = schema.get_string();
      if (basenames->has_key(info->schema)) {
        info->basename = basenames->get_string(info->schema);
      } else {
        info->basename = info->schema;
      }

      m_contents.schemas.emplace(info->schema, info);
    } else {
      log_debug("Skipping schema '%s'", schema.as_string().c_str());
    }
  }

  if (md->has_key("version"))
    m_contents.dump_version =
        mysqlshdk::utils::Version(md->get_string("version"));

  if (md->has_key("serverVersion"))
    m_contents.server_version =
        mysqlshdk::utils::Version(md->get_string("serverVersion"));

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

  if (md->has_key("tableOnly"))
    m_contents.table_only = md->get_bool("tableOnly");

  if (md->has_key("bytesPerChunk"))
    m_contents.bytes_per_chunk = md->get_uint("bytesPerChunk");

  m_contents.has_users = md->has_key("users");

  try {
    m_contents.parse_done_metadata(m_dir.get());

    m_dump_status = Status::COMPLETE;
  } catch (const std::exception &e) {
    log_info("@.done.json: %s", e.what());
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
      *out_schema = s->schema;

      for (const auto &t : s->tables) {
        out_tables->emplace_back(
            t.first,
            t.second->has_sql ? m_dir->file(t.second->script_name()) : nullptr);
      }

      if (s->has_view_sql) {
        for (const auto &v : s->views) {
          out_view_placeholders->emplace_back(v.table,
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
      *out_schema = s->schema;

      for (const auto &v : s->views) {
        out_views->emplace_back(v.table, m_dir->file(v.script_name()));
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
      script,
      [&account_list](const std::string &account) {
        account_list.emplace_back(shcore::split_account(account));
        return true;
      },
      {});

  return account_list;
}

std::vector<std::string> Dump_reader::schemas() const {
  std::vector<std::string> slist;

  for (const auto &s : m_contents.schemas) {
    slist.push_back(s.second->schema);
  }

  return slist;
}

bool Dump_reader::schema_objects(const std::string &schema,
                                 std::vector<std::string> *out_tables,
                                 std::vector<std::string> *out_views,
                                 std::vector<std::string> *out_triggers,
                                 std::vector<std::string> *out_functions,
                                 std::vector<std::string> *out_procedures,
                                 std::vector<std::string> *out_events) {
  auto schema_it = m_contents.schemas.find(schema);
  if (schema_it == m_contents.schemas.end()) return false;
  auto schema_info = schema_it->second;

  out_tables->clear();
  for (const auto &t : schema_info->tables) {
    out_tables->push_back(t.first);
  }
  out_views->clear();
  for (const auto &v : schema_info->views) {
    out_views->push_back(v.table);
  }
  *out_triggers = schema_info->trigger_names;
  *out_functions = schema_info->function_names;
  *out_procedures = schema_info->procedure_names;
  *out_events = schema_info->event_names;

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
  return s->fk_queries;
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
// as many different tables loading concurrently for as long as possible.
// Thus, smaller tables must get fewer threads allocated so they take longer
// to load, while bigger threads get more, with the hope that the total time
// to load all tables is minimized.
std::unordered_set<Dump_reader::Table_info *>::iterator
Dump_reader::schedule_chunk_proportionally(
    const std::unordered_multimap<std::string, size_t> &tables_being_loaded,
    std::unordered_set<Dump_reader::Table_info *> *tables_with_data) {
  if (tables_with_data->empty()) return tables_with_data->end();

  // first check if there's any table that's not being loaded
  {
    auto best = tables_with_data->end();

    for (auto it = tables_with_data->begin(); it != tables_with_data->end();
         ++it) {
      std::string key = schema_table_key((*it)->schema, (*it)->table);
      if (tables_being_loaded.find(key) == tables_being_loaded.end()) {
        if (best == tables_with_data->end() ||
            (*it)->bytes_available() > (*best)->bytes_available())
          best = it;
      }
    }
    if (best != tables_with_data->end()) {
      return best;
    }
  }

  // if all available tables are already loaded, then schedule proportionally
  std::unordered_map<std::string, double> worker_weights;
  std::vector<std::pair<std::unordered_set<Dump_reader::Table_info *>::iterator,
                        double>>
      candidate_weights;
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

  // calc ratio of data available per table / total data available
  double total_bytes_available = std::accumulate(
      tables_with_data->begin(), tables_with_data->end(),
      static_cast<size_t>(0), [](size_t size, Dump_reader::Table_info *table) {
        return size + table->bytes_available();
      });
  if (total_bytes_available > 0) {
    for (auto it = tables_with_data->begin(); it != tables_with_data->end();
         ++it) {
      candidate_weights.emplace_back(
          it, static_cast<double>((*it)->bytes_available()) /
                  total_bytes_available);
    }
  } else {
    assert(0);
    return tables_with_data->begin();
  }

  // pick a chunk from the table that has the biggest difference between both
  double best_diff = 0;
  std::unordered_set<Dump_reader::Table_info *>::iterator best =
      tables_with_data->begin();

  for (const auto &cand : candidate_weights) {
    std::string key =
        schema_table_key((*cand.first)->schema, (*cand.first)->table);
    auto it = worker_weights.find(key);
    if (it != worker_weights.end()) {
      double d = cand.second - it->second;
      if (d > best_diff) {
        best_diff = d;
        best = cand.first;
      }
    } else {
      if (cand.second > best_diff) {
        best_diff = cand.second;
        best = cand.first;
      }
    }
  }

  return best;
}

bool Dump_reader::next_table_chunk(
    const std::unordered_multimap<std::string, size_t> &tables_being_loaded,
    std::string *out_schema, std::string *out_table, bool *out_chunked,
    size_t *out_chunk_index, size_t *out_chunks_total,
    std::unique_ptr<mysqlshdk::storage::IFile> *out_file,
    size_t *out_chunk_size, shcore::Dictionary_t *out_options) {
  auto iter =
      schedule_chunk_proportionally(tables_being_loaded, &m_tables_with_data);

  if (iter != m_tables_with_data.end()) {
    *out_chunked = (*iter)->chunked;
    *out_schema = (*iter)->schema;
    *out_table = (*iter)->table;
    *out_options = (*iter)->options;

    if ((*iter)->chunked) {
      *out_chunk_index = (*iter)->chunks_consumed;
      if ((*iter)->last_chunk_seen)
        *out_chunks_total = (*iter)->num_chunks;
      else
        *out_chunks_total = 0;
      (*iter)->chunks_consumed++;

      *out_file = m_dir->file(dump::get_table_data_filename(
          (*iter)->basename, (*iter)->extension, *out_chunk_index,
          *out_chunk_index + 1 == *out_chunks_total));

      *out_chunk_size = (*iter)->available_chunk_sizes[*out_chunk_index];
    } else {
      *out_chunk_index = 0;
      *out_chunks_total = 0;
      (*iter)->chunks_consumed++;

      *out_chunk_size = (*iter)->available_chunk_sizes[0];

      *out_file = m_dir->file(
          dump::get_table_data_filename((*iter)->basename, (*iter)->extension));
    }
    if (!(*iter)->has_data_available()) m_tables_with_data.erase(iter);
    return true;
  }

  return false;
}

bool Dump_reader::next_deferred_index(
    std::string *out_schema, std::string *out_table,
    std::vector<std::string> **out_indexes,
    const std::function<bool(const std::string &)> &load_finished) {
  for (auto &schema : m_contents.schemas) {
    for (auto &table : schema.second->tables) {
      if (!table.second->indexes_done && table.second->data_done() &&
          load_finished(schema_table_key(schema.first, table.first))) {
        table.second->indexes_done = true;
        *out_schema = schema.second->schema;
        *out_table = table.second->table;
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
      if (table.second->data_done() && table.second->indexes_done &&
          !table.second->analyze_done) {
        table.second->analyze_done = true;
        *out_schema = schema.second->schema;
        *out_table = table.second->table;
        *out_histograms = table.second->histograms;
        return true;
      }
    }
  }
  return false;
}

bool Dump_reader::data_available() const { return !m_tables_with_data.empty(); }

bool Dump_reader::work_available() const {
  for (auto &schema : m_contents.schemas) {
    for (auto &table : schema.second->tables) {
      if (table.second->data_done() && !table.second->analyze_done) {
        return true;
      }
    }
  }
  return false;
}

size_t Dump_reader::filtered_data_size() const {
  if (m_dump_status == Status::COMPLETE) {
    size_t size = 0;

    for (const auto &schema : m_contents.schemas) {
      for (const auto &table : schema.second->tables) {
        const auto s = m_contents.table_data_size.find(table.second->schema);

        if (s != m_contents.table_data_size.end()) {
          const auto t = s->second.find(table.second->table);

          if (t != s->second.end()) {
            size += t->second;
          }
        }
      }
    }

    return size;
  } else {
    return total_data_size();
  }
}

// Scan directory for new files and adds them to the pending file list
void Dump_reader::rescan() {
  using mysqlshdk::storage::IDirectory;
  auto console = mysqlsh::current_console();

  if (!m_dir->is_local()) {
    console->print_status("Fetching dump data from remote location...");
  }

  std::vector<IDirectory::File_info> files = m_dir->list_files();
  std::unordered_map<std::string, size_t> file_by_name;
  for (const auto &f : files) {
    file_by_name[f.name] = f.size;
  }

  m_contents.rescan(m_dir.get(), file_by_name, this);

  if (file_by_name.find("@.done.json") != file_by_name.end() &&
      m_dump_status != Status::COMPLETE) {
    m_dump_status = Status::COMPLETE;
    m_contents.parse_done_metadata(m_dir.get());
  }
}

void Dump_reader::add_deferred_indexes(const std::string &schema,
                                       const std::string &table,
                                       std::vector<std::string> &&indexes) {
  auto s = m_contents.schemas.find(schema);
  if (s == m_contents.schemas.end())
    throw std::runtime_error("Unable to find schema " + schema +
                             " for adding index");
  auto t = s->second->tables.find(table);
  if (t == s->second->tables.end())
    throw std::runtime_error("Unable to find table " + table + " in schema " +
                             schema + " for adding index");
  t->second->indexes_done = false;
  t->second->indexes = std::move(indexes);
  auto &idx = t->second->indexes;
  idx.erase(
      std::remove_if(
          idx.begin(), idx.end(),
          [&s](const std::string &q) {
            mysqlshdk::utils::SQL_iterator it(q);
            while (it.valid() &&
                   !shcore::str_caseeq(it.get_next_token(), "FOREIGN")) {
            }
            if (it.valid() && shcore::str_caseeq(it.get_next_token(), "KEY")) {
              s->second->fk_queries.emplace_back(q);
              return true;
            }
            return false;
          }),
      idx.end());
}

void Dump_reader::replace_target_schema(const std::string &schema) {
  if (!is_dump_tables()) {
    throw std::logic_error("Dump was not created using util.dumpTables().");
  }

  assert(1 == m_contents.schemas.size());

  const auto info = m_contents.schemas.begin()->second;
  m_contents.schemas.clear();
  m_contents.schemas.emplace(schema, info);

  info->schema = schema;
  info->has_sql = false;

  for (const auto &table : info->tables) {
    table.second->schema = schema;
  }

  for (auto &view : info->views) {
    view.schema = schema;
  }

  for (const auto &table : m_tables_with_data) {
    table->schema = schema;
    table->options->set("schema", shcore::Value(schema));
  }
}

void Dump_reader::validate_options() {
  if (m_options.load_users() && !m_contents.has_users) {
    current_console()->print_warning(
        "The 'loadUsers' option is set to true, but the dump does not contain "
        "the user data.");
  }

  if (is_dump_tables()) {
    if (m_options.target_schema().empty()) {
      // user didn't provide an option, we cannot proceed if the dump was
      // created by an old version of dumpTables()
      if (table_only()) {
        current_console()->print_error(
            "The dump was created by an older version of the util." +
            shcore::get_member_name("dumpTables",
                                    shcore::current_naming_style()) +
            "() function and needs to be loaded into an existing schema. "
            "Please set the target schema using the 'schema' option.");
        throw std::invalid_argument("The target schema was not specified.");
      }
    } else {
      const auto result = m_options.base_session()->queryf(
          "SELECT SCHEMA_NAME FROM information_schema.schemata WHERE "
          "SCHEMA_NAME = ?",
          m_options.target_schema());

      if (nullptr == result->fetch_one()) {
        throw std::invalid_argument("The specified schema does not exist.");
      }
    }
  } else {
    if (!m_options.target_schema().empty()) {
      current_console()->print_error(
          "The dump was not created by the util." +
          shcore::get_member_name("dumpTables",
                                  shcore::current_naming_style()) +
          "() function, the 'schema' option cannot be used.");
      throw std::invalid_argument("Invalid option: schema.");
    }
  }
}

std::string Dump_reader::Table_info::script_name() const {
  return dump::get_table_filename(basename);
}

std::string Dump_reader::Table_info::triggers_script_name() const {
  return dump::get_table_data_filename(basename, "triggers.sql");
}

bool Dump_reader::Table_info::ready() const {
  return md_seen && (!has_sql || sql_seen);
}

void Dump_reader::Table_info::rescan(
    mysqlshdk::storage::IDirectory *dir,
    const std::unordered_map<std::string, size_t> &files, Dump_reader *reader) {
  // MD not included for tables if data is not dumped
  if (!md_seen) {
    // schema@table.json
    std::string mdpath = dump::get_table_data_filename(basename, "json");
    if (files.find(mdpath) != files.end()) {
      md_seen = true;
      auto md = fetch_metadata(dir, mdpath);

      has_sql = md->get_bool("includesDdl", true);
      has_data = md->get_bool("includesData", true);

      options = md->get_map("options");

      if (options) {
        // Not used by chunk importer
        options->erase("compression");
        if (options->has_key("primaryIndex")) {
          primary_index = options->get_string("primaryIndex");
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

      extension = md->get_string("extension", "tsv");
      chunked = md->get_bool("chunking", false);
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
    }
  }

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

void Dump_reader::Table_info::rescan_data(
    mysqlshdk::storage::IDirectory *,
    const std::unordered_map<std::string, size_t> &files, Dump_reader *reader) {
  // check for data files
  if (!last_chunk_seen && has_data) {
    bool found_data = false;

    if (!chunked) {
      auto it = files.find(dump::get_table_data_filename(basename, extension));
      if (it != files.end()) {
        num_chunks = 1;
        available_chunk_sizes.resize(1, -1);
        available_chunk_sizes[0] = it->second;
        last_chunk_seen = true;
        reader->m_contents.dump_size += it->second;

        found_data = true;
      }
    } else {
      for (size_t i = num_chunks; i < files.size(); i++) {
        auto it = files.find(
            dump::get_table_data_filename(basename, extension, i, false));
        if (it != files.end()) {
          num_chunks = i + 1;
          available_chunk_sizes.resize(num_chunks, -1);
          available_chunk_sizes[i] = it->second;
          reader->m_contents.dump_size += it->second;

          found_data = true;
        } else {
          it = files.find(
              dump::get_table_data_filename(basename, extension, i, true));
          if (it != files.end()) {
            num_chunks = i + 1;
            last_chunk_seen = true;
            available_chunk_sizes.resize(num_chunks, -1);
            available_chunk_sizes[i] = it->second;
            reader->m_contents.dump_size += it->second;

            found_data = true;
          }
          break;
        }
      }
    }
    if (found_data) reader->m_tables_with_data.insert(this);
  }
}

std::string Dump_reader::View_info::script_name() const {
  return dump::get_table_filename(basename);
}

std::string Dump_reader::View_info::pre_script_name() const {
  return dump::get_table_data_filename(basename, "pre.sql");
}

void Dump_reader::View_info::rescan(
    mysqlshdk::storage::IDirectory *,
    const std::unordered_map<std::string, size_t> &files, Dump_reader *) {
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
  if (md_loaded && (!has_sql || sql_seen)) {
    for (const auto &t : tables) {
      if (!t.second->ready()) return false;
    }

    for (const auto &v : views) {
      if (!v.ready()) return false;
    }
    return true;
  }
  return false;
}

std::string Dump_reader::Schema_info::script_name() const {
  return dump::get_schema_filename(basename);
}

void Dump_reader::Schema_info::rescan(
    mysqlshdk::storage::IDirectory *dir,
    const std::unordered_map<std::string, size_t> &files, Dump_reader *reader) {
  // look for list of tables and views for this schema in the md file
  if (!md_loaded) {
    // schema.json
    std::string mdpath = dump::get_schema_filename(basename, "json");
    if (files.find(mdpath) != files.end()) {
      md_loaded = true;
      auto md = fetch_metadata(dir, mdpath);

      has_sql = md->get_bool("includesDdl", true);
      has_view_sql = md->get_bool("includesViewsDdl", has_sql);
      has_data = md->get_bool("includesData", true);

      shcore::Dictionary_t basenames = md->get_map("basenames");

      if (md->has_key("tables")) {
        for (const auto &t : *md->get_array("tables")) {
          if (reader->m_options.include_table(schema, t.as_string())) {
            auto info = std::make_shared<Table_info>();

            info->schema = schema;
            info->table = t.as_string();

            // inherit defaults from schema in case the table has no MD file
            info->has_sql = has_sql;
            info->has_data = has_data;

            if (basenames->has_key(info->table))
              info->basename = basenames->get_string(info->table);
            else
              info->basename = basename + "@" + info->table;

            tables.emplace(info->table, info);
          }
        }
        log_debug("%s has %zi tables", schema.c_str(), tables.size());
      }

      if (md->has_key("views")) {
        for (const auto &v : *md->get_array("views")) {
          if (reader->m_options.include_table(schema, v.as_string())) {
            View_info info;

            info.schema = schema;
            info.table = v.as_string();
            if (basenames->has_key(info.table))
              info.basename = basenames->get_string(info.table);
            else
              info.basename = basename + "@" + info.table;

            views.push_back(info);
          }
        }
        log_debug("%s has %zi views", schema.c_str(), views.size());
      }

      const auto to_vector_of_strings = [](const shcore::Array_t &arr) {
        std::vector<std::string> res;
        for (const auto &s : *arr) res.emplace_back(s.as_string());
        return res;
      };

      if (md->has_key("functions"))
        function_names = to_vector_of_strings(md->get_array("functions"));

      if (md->has_key("procedures"))
        procedure_names = to_vector_of_strings(md->get_array("procedures"));

      if (md->has_key("events"))
        event_names = to_vector_of_strings(md->get_array("events"));

      if (!dir->is_local()) {
        current_console()->print_status(shcore::str_format(
            "Fetching %zu table metadata files for schema `%s`...",
            tables.size(), schema.c_str()));
      }
    }
  }

  // we have the list of tables, so check for their metadata and data files
  if (md_loaded && !md_done) {
    md_done = true;
    for (auto &t : tables) {
      if (!t.second->ready()) {
        t.second->rescan(dir, files, reader);
      }

      if (!t.second->ready()) md_done = false;
    }

    for (auto &v : views) {
      if (!v.ready()) v.rescan(dir, files, reader);

      if (!v.ready()) md_done = false;
    }

    if (md_done)
      log_debug("All metadata for schema `%s` was scanned", schema.c_str());
  }

  // check for the sql file for the schema
  if (!sql_seen) {
    if (files.find(script_name()) != files.end()) {
      sql_seen = true;
    }
  }

  assert(reader->m_dump_status != Status::COMPLETE || ready());
}

void Dump_reader::Schema_info::rescan_data(
    mysqlshdk::storage::IDirectory *dir,
    const std::unordered_map<std::string, size_t> &files, Dump_reader *reader) {
  for (auto &t : tables) {
    if (!t.second->data_done()) {
      t.second->rescan_data(dir, files, reader);
    }
  }
}

bool Dump_reader::Schema_info::data_done() const {
  for (const auto &t : tables) {
    if (!t.second->data_done()) return false;
  }
  return true;
}

bool Dump_reader::Dump_info::ready() const {
  return sql && post_sql && has_users == !!users_sql && md_done;
}

void Dump_reader::Dump_info::rescan(
    mysqlshdk::storage::IDirectory *dir,
    const std::unordered_map<std::string, size_t> &files, Dump_reader *reader) {
  if (!sql && files.find("@.sql") != files.end()) {
    sql = std::make_unique<std::string>(fetch_file(dir, "@.sql"));
  }
  if (!post_sql && files.find("@.post.sql") != files.end()) {
    post_sql = std::make_unique<std::string>(fetch_file(dir, "@.post.sql"));
  }
  if (has_users && !users_sql && files.find("@.users.sql") != files.end()) {
    users_sql = std::make_unique<std::string>(fetch_file(dir, "@.users.sql"));
  }

  if (!md_done) {
    md_done = true;
    for (const auto &s : schemas) {
      log_debug("Scanning contents of schema '%s'", s.second->schema.c_str());

      if (!s.second->ready()) {
        s.second->rescan(dir, files, reader);
        if (s.second->ready()) s.second->rescan_data(dir, files, reader);
      }
      if (!s.second->ready()) md_done = false;
    }
    if (md_done) log_debug("All metadata for dump was scanned");
  } else {
    for (const auto &s : schemas) {
      log_debug("Scanning data of schema '%s'", s.second->schema.c_str());

      s.second->rescan_data(dir, files, reader);
    }
  }
}

void Dump_reader::Dump_info::parse_done_metadata(
    mysqlshdk::storage::IDirectory *dir) {
  shcore::Dictionary_t metadata = fetch_metadata(dir, "@.done.json");
  log_info("Dump %s is complete", dir->full_path().c_str());

  if (metadata) {
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
}

std::unique_ptr<mysqlshdk::storage::IFile>
Dump_reader::create_progress_file_handle() const {
  if (m_options.use_par() && m_options.use_par_progress()) {
    return m_dir->file(*m_options.progress_file());
  } else {
    return m_options.create_progress_file_handle();
  }
}

void Dump_reader::show_metadata() const {
  if (m_options.show_metadata()) {
    const auto metadata = shcore::make_dict(
        "Dump_metadata",
        shcore::make_dict("Binlog_file", binlog_file(), "Binlog_position",
                          binlog_position(), "Executed_GTID_set",
                          gtid_executed()));

    const auto console = current_console();

    console->println();
    console->println(shcore::Value(metadata).yaml());
  }
}

}  // namespace mysqlsh
