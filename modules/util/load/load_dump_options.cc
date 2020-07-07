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

#include "modules/util/load/load_dump_options.h"
#include "modules/util/dump/dump_manifest.h"

#include "modules/mod_utils.h"

namespace mysqlsh {
using mysqlsh::dump::Dump_manifest;

Load_dump_options::Load_dump_options(const std::string &url)
    : m_url(url),
      m_oci_options(mysqlshdk::oci::Oci_options::Unpack_target::
                        OBJECT_STORAGE_NO_PAR_OPTIONS) {}

void Load_dump_options::set_session(
    const std::shared_ptr<mysqlshdk::db::ISession> &session,
    const std::string &current_schema) {
  m_base_session = session;
  m_current_schema = current_schema;

  m_target = get_classic_connection_options(m_base_session);

  if (m_target.has(mysqlshdk::db::kLocalInfile)) {
    m_target.remove(mysqlshdk::db::kLocalInfile);
  }
  m_target.set(mysqlshdk::db::kLocalInfile, "true");

  // Set long timeouts by default
  std::string timeout = "86400000";  // 1 day in milliseconds
  if (!m_target.has(mysqlshdk::db::kNetReadTimeout)) {
    m_target.set(mysqlshdk::db::kNetReadTimeout, timeout);
  }
  if (!m_target.has(mysqlshdk::db::kNetWriteTimeout)) {
    m_target.set(mysqlshdk::db::kNetWriteTimeout, timeout);
  }
}

void Load_dump_options::validate() {
  if (!m_load_data && !m_load_ddl && !m_load_users &&
      m_analyze_tables == Analyze_table_mode::OFF)
    throw shcore::Exception::runtime_error(
        "At least one of loadData, loadDdl or loadUsers options must be "
        "enabled");

  if (m_use_par) {
    if (m_progress_file.is_null()) {
      throw shcore::Exception::runtime_error(
          "When using a PAR to a dump manifest, the progressFile option must "
          "be defined.");
    } else {
      dump::Par_structure progress_par_data;
      m_use_par_progress =
          dump::parse_full_object_par(*m_progress_file, &progress_par_data);
    }
  }

  {
    auto result =
        m_base_session->query("SHOW GLOBAL VARIABLES LIKE 'local_infile'");
    auto row = result->fetch_one();
    auto local_infile_value = row->get_string(1);

    if (shcore::str_caseeq(local_infile_value, "off")) {
      mysqlsh::current_console()->print_error(
          "The 'local_infile' global system variable must be set to ON in "
          "the target server, after the server is verified to be trusted.");
      throw shcore::Exception::runtime_error("local_infile disabled in server");
    }
  }

  if (m_progress_file.is_null()) {
    std::string uuid = m_base_session->query("SELECT @@server_uuid")
                           ->fetch_one_or_throw()
                           ->get_string(0);
    m_default_progress_file = "load-progress." + uuid + ".json";
  }
}

std::string Load_dump_options::target_import_info() const {
  std::string action;

  std::vector<std::string> what_to_load;

  if (load_ddl()) what_to_load.push_back("DDL");
  if (load_data()) what_to_load.push_back("Data");
  if (load_users()) what_to_load.push_back("Users");

  std::string where;
  if (m_oci_options) {
    if (m_use_par) {
      where = "OCI PAR=" + m_url + ", prefix='" + m_prefix + "'";
    } else {
      where = "OCI ObjectStorage bucket=" + *m_oci_options.os_bucket_name +
              ", prefix='" + m_url + "'";
    }
  } else {
    where = "'" + m_url + "'";
  }

  // this is validated earlier on
  assert(!what_to_load.empty() || m_analyze_tables != Analyze_table_mode::OFF);

  if (what_to_load.size() == 3) {
    action = shcore::str_format(
        "Loading %s, %s and %s from %s", what_to_load[0].c_str(),
        what_to_load[1].c_str(), what_to_load[2].c_str(), where.c_str());
  } else if (what_to_load.size() == 2) {
    action =
        shcore::str_format("Loading %s and %s from %s", what_to_load[0].c_str(),
                           what_to_load[1].c_str(), where.c_str());
  } else if (!what_to_load.empty()) {
    action = shcore::str_format("Loading %s only from %s",
                                what_to_load[0].c_str(), where.c_str());
  } else {
    assert(m_analyze_tables != Analyze_table_mode::OFF);
    if (m_analyze_tables == Analyze_table_mode::HISTOGRAM)
      action = "Updating table histograms";
    else
      action = "Updating table histograms and key distribution statistics";
  }

  std::string detail;

  if (threads_count() == 1)
    detail = " using 1 thread.";
  else
    detail = shcore::str_format(" using %s threads.",
                                std::to_string(threads_count()).c_str());

  return action + detail;
}

void Load_dump_options::set_options(const shcore::Dictionary_t &options) {
  std::vector<std::string> tables;
  std::vector<std::string> schemas;
  std::vector<std::string> exclude_tables;
  std::vector<std::string> exclude_schemas;
  std::string analyze_tables = "off";
  std::string defer_table_indexes = "fulltext";

  Unpack_options unpacker(options);

  unpacker.optional("threads", &m_threads_count)
      .optional("showProgress", &m_show_progress)
      .optional("waitDumpTimeout", &m_wait_dump_timeout)
      .optional("loadData", &m_load_data)
      .optional("loadDdl", &m_load_ddl)
      .optional("loadUsers", &m_load_users)
      .optional("dryRun", &m_dry_run)
      .optional("resetProgress", &m_reset_progress)
      .optional("progressFile", &m_progress_file)
      .optional("includeSchemas", &schemas)
      .optional("includeTables", &tables)
      .optional("excludeSchemas", &exclude_schemas)
      .optional("excludeTables", &exclude_tables)
      .optional("characterSet", &m_character_set)
      .optional("skipBinlog", &m_skip_binlog)
      .optional("ignoreExistingObjects", &m_ignore_existing_objects)
      .optional("ignoreVersion", &m_ignore_version)
      .optional("analyzeTables", &analyze_tables)
      .optional("deferTableIndexes", &defer_table_indexes)
      .optional("loadIndexes", &m_load_indexes)
      .optional("schema", &m_target_schema);

  unpacker.unpack(&m_oci_options);
  unpacker.end();

  dump::Par_structure url_par_data;
  m_use_par = dump::parse_full_object_par(m_url, &url_par_data);

  if (m_use_par) {
    m_oci_options.os_par = m_url;
    m_prefix = url_par_data.object_prefix;
  }

  m_oci_options.check_option_values();

  for (const auto &schema : schemas) {
    if (!schema.empty() && schema[0] != '`')
      m_include_schemas.insert(shcore::quote_identifier(schema));
    else
      m_include_schemas.insert(schema);
  }

  for (const auto &schema : exclude_schemas) {
    if (!schema.empty() && schema[0] != '`')
      m_exclude_schemas.insert(shcore::quote_identifier(schema));
    else
      m_exclude_schemas.insert(schema);
  }

  auto parse_tables = [](const std::vector<std::string> &opt_tables,
                         std::unordered_set<std::string> *out_filter,
                         bool add_schema_entry) {
    for (const auto &table : opt_tables) {
      std::string schema_, table_;
      try {
        shcore::split_schema_and_table(table, &schema_, &table_);
      } catch (const std::exception &e) {
        throw std::runtime_error(
            "Can't parse table filter '" + table +
            "'. The table must be in the following form: "
            "schema.table, with optional backtick quotes.");
      }

      if (schema_.empty()) {
        throw std::runtime_error(
            "Can't parse table filter '" + table +
            "'. The table must be in the following form: "
            "schema.table, with optional backtick quotes.");
      }

      if (schema_[0] == '`') schema_ = shcore::unquote_identifier(schema_);

      if (table_[0] == '`') table_ = shcore::unquote_identifier(table);

      out_filter->insert(schema_table_key(schema_, table_));

      if (add_schema_entry) {
        // insert schema."", so that we can check whether we want to include
        // one or more tables for a given schema, but not the whole schema
        out_filter->insert(schema_table_key(schema_, ""));
      }
    }
  };

  parse_tables(tables, &m_include_tables, true);
  parse_tables(exclude_tables, &m_exclude_tables, false);

  if (defer_table_indexes == "off") {
    m_defer_table_indexes = Defer_index_mode::OFF;
  } else if (defer_table_indexes == "all") {
    m_defer_table_indexes = Defer_index_mode::ALL;
  } else if (defer_table_indexes == "fulltext") {
    m_defer_table_indexes = Defer_index_mode::FULLTEXT;
  } else {
    throw std::runtime_error("Invalid value '" + defer_table_indexes +
                             "' for deferTableIndexes option, allowed values: "
                             "'all', 'fulltext', and 'off'.");
  }

  if (!m_load_indexes && m_defer_table_indexes == Defer_index_mode::OFF)
    throw std::invalid_argument(
        "'deferTableIndexes' option needs to be enabled when "
        "'loadIndexes' option is disabled");

  if (analyze_tables == "histogram") {
    m_analyze_tables = Analyze_table_mode::HISTOGRAM;
  } else if (analyze_tables == "on") {
    m_analyze_tables = Analyze_table_mode::ON;
  } else if (analyze_tables == "off") {
    m_analyze_tables = Analyze_table_mode::OFF;
  } else {
    throw std::runtime_error("Invalid value '" + analyze_tables +
                             "' for analyzeTables option, allowed values: "
                             "'off', 'on', and 'histogram'.");
  }
}

std::unique_ptr<mysqlshdk::storage::IDirectory>
Load_dump_options::create_dump_handle() const {
  if (m_use_par) {
    return std::make_unique<Dump_manifest>(Dump_manifest::Mode::READ,
                                           m_oci_options, m_url);
  } else if (!m_oci_options.os_bucket_name.get_safe().empty()) {
    return mysqlshdk::storage::make_directory(m_url, m_oci_options);
  } else {
    return mysqlshdk::storage::make_directory(m_url);
  }
}

std::unique_ptr<mysqlshdk::storage::IFile>
Load_dump_options::create_progress_file_handle() const {
  if (m_progress_file.get_safe().empty())
    return create_dump_handle()->file(m_default_progress_file);
  else
    return mysqlshdk::storage::make_file(*m_progress_file);
}

// Filtering works as:
// (includeSchemas + includeTables || *) - excludeSchemas - excludeTables
bool Load_dump_options::include_schema(const std::string &schema) const {
  std::string qschema = shcore::quote_identifier(schema);

  if (m_exclude_schemas.count(qschema) > 0) return false;

  // If includeSchemas neither includeTables are given, then all schemas
  // are included by default
  if ((m_include_schemas.empty() && m_include_tables.empty()) ||
      m_include_schemas.count(qschema) > 0)
    return true;

  return false;
}

bool Load_dump_options::include_table(const std::string &schema,
                                      const std::string &table) const {
  std::string key = schema_table_key(schema, table);

  if (m_exclude_tables.count(key) > 0 ||
      m_exclude_schemas.count(shcore::quote_identifier(schema)) > 0)
    return false;

  if ((include_schema(schema) && m_include_tables.empty()) ||
      m_include_tables.count(key) > 0)
    return true;

  return false;
}

std::vector<std::string> Load_dump_options::get_excluded_users(bool is_mds) {
  std::vector<std::string> ret = {"mysql.sys", "mysql.session",
                                  "mysql.infoschema"};
  if (is_mds)
    ret.insert(ret.end(),
               {"ociadmin", "ocirpl", "ocimonitor", "administrator"});
  return ret;
}

}  // namespace mysqlsh
