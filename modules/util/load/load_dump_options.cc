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

#include "modules/util/load/load_dump_options.h"

#include <mysqld_error.h>

#include <algorithm>

#include "modules/mod_utils.h"
#include "modules/util/dump/dump_manifest.h"
#include "mysqlshdk/include/scripting/type_info/custom.h"
#include "mysqlshdk/include/scripting/type_info/generic.h"
#include "mysqlshdk/libs/storage/backend/oci_object_storage.h"
#include "mysqlshdk/libs/storage/utils.h"
#include "mysqlshdk/libs/utils/debug.h"
#include "mysqlshdk/libs/utils/strformat.h"
#include "mysqlshdk/libs/utils/utils_string.h"

namespace mysqlsh {
namespace {

const char *k_excluded_users[] = {"mysql.infoschema", "mysql.session",
                                  "mysql.sys"};
const char *k_oci_excluded_users[] = {"administrator", "ociadmin", "ocimonitor",
                                      "ocirpl"};

constexpr auto k_minimum_max_bytes_per_transaction = "128k";

bool is_mds(const mysqlshdk::utils::Version &version) {
  return shcore::str_endswith(version.get_extra(), "cloud");
}

void parse_tables(const std::vector<std::string> &opt_tables,
                  std::unordered_set<std::string> *out_filter,
                  bool add_schema_entry) {
  for (const auto &table_def : opt_tables) {
    std::string schema, table;
    try {
      shcore::split_schema_and_table(table_def, &schema, &table);
    } catch (const std::exception &e) {
      throw std::invalid_argument(
          "Can't parse table filter '" + table_def +
          "'. The table must be in the following form: "
          "schema.table, with optional backtick quotes.");
    }

    if (schema.empty()) {
      throw std::invalid_argument(
          "Can't parse table filter '" + table_def +
          "'. The table must be in the following form: "
          "schema.table, with optional backtick quotes.");
    }

    if (schema[0] == '`') schema = shcore::unquote_identifier(schema);

    if (table[0] == '`') table = shcore::unquote_identifier(table_def);

    out_filter->insert(schema_table_key(schema, table));

    if (add_schema_entry) {
      // insert schema."", so that we can check whether we want to include
      // one or more tables for a given schema, but not the whole schema
      out_filter->insert(schema_table_key(schema, ""));
    }
  }
}

}  // namespace

using mysqlsh::dump::Dump_manifest;
using mysqlsh::dump::Manifest_mode;
Load_dump_options::Load_dump_options() : Load_dump_options("") {}
Load_dump_options::Load_dump_options(const std::string &url) : m_url(url) {}

const shcore::Option_pack_def<Load_dump_options> &Load_dump_options::options() {
  static const auto opts =
      shcore::Option_pack_def<Load_dump_options>()
          .optional("threads", &Load_dump_options::m_threads_count)
          .optional("showProgress", &Load_dump_options::m_show_progress)
          .optional("waitDumpTimeout", &Load_dump_options::set_wait_timeout)
          .optional("loadData", &Load_dump_options::m_load_data)
          .optional("loadDdl", &Load_dump_options::m_load_ddl)
          .optional("loadUsers", &Load_dump_options::m_load_users)
          .optional("dryRun", &Load_dump_options::m_dry_run)
          .optional("resetProgress", &Load_dump_options::m_reset_progress)
          .optional("progressFile", &Load_dump_options::m_progress_file)
          .optional("includeSchemas", &Load_dump_options::set_str_vector_option)
          .optional("includeTables", &Load_dump_options::set_str_vector_option)
          .optional("excludeSchemas", &Load_dump_options::set_str_vector_option)
          .optional("excludeTables", &Load_dump_options::set_str_vector_option)
          .optional("characterSet", &Load_dump_options::m_character_set)
          .optional("skipBinlog", &Load_dump_options::m_skip_binlog)
          .optional("ignoreExistingObjects",
                    &Load_dump_options::m_ignore_existing_objects)
          .optional("ignoreVersion", &Load_dump_options::m_ignore_version)
          .optional("analyzeTables", &Load_dump_options::m_analyze_tables,
                    {{"histogram", Analyze_table_mode::HISTOGRAM},
                     {"on", Analyze_table_mode::ON},
                     {"off", Analyze_table_mode::OFF}})
          .optional("deferTableIndexes",
                    &Load_dump_options::m_defer_table_indexes,
                    {{"off", Defer_index_mode::OFF},
                     {"all", Defer_index_mode::ALL},
                     {"fulltext", Defer_index_mode::FULLTEXT}})
          .optional("loadIndexes", &Load_dump_options::m_load_indexes)
          .optional("schema", &Load_dump_options::m_target_schema)
          .optional("excludeUsers",
                    &Load_dump_options::set_str_unordered_set_option)
          .optional("includeUsers",
                    &Load_dump_options::set_str_unordered_set_option)
          .optional("updateGtidSet", &Load_dump_options::m_update_gtid_set,
                    {{"append", Update_gtid_set::APPEND},
                     {"replace", Update_gtid_set::REPLACE},
                     {"off", Update_gtid_set::OFF}})
          .optional("showMetadata", &Load_dump_options::m_show_metadata)
          .optional("createInvisiblePKs",
                    &Load_dump_options::m_create_invisible_pks)
          .optional("maxBytesPerTransaction",
                    &Load_dump_options::set_max_bytes_per_transaction)
          .include(&Load_dump_options::m_oci_option_pack)
          .on_done(&Load_dump_options::on_unpacked_options)
          .on_log(&Load_dump_options::on_log_options);

  return opts;
}

void Load_dump_options::set_wait_timeout(const double &timeout_seconds) {
  m_wait_dump_timeout_ms = timeout_seconds * 1000;
}

void Load_dump_options::set_str_vector_option(
    const std::string &option, const std::vector<std::string> &data) {
  if (option == "includeSchemas") {
    for (const auto &schema : data) {
      if (!schema.empty() && schema[0] != '`')
        m_include_schemas.insert(shcore::quote_identifier(schema));
      else
        m_include_schemas.insert(schema);
    }
  } else if (option == "includeTables") {
    parse_tables(data, &m_include_tables, true);
  } else if (option == "excludeSchemas") {
    for (const auto &schema : data) {
      if (!schema.empty() && schema[0] != '`')
        m_exclude_schemas.insert(shcore::quote_identifier(schema));
      else
        m_exclude_schemas.insert(schema);
    }
  } else if (option == "excludeTables") {
    parse_tables(data, &m_exclude_tables, false);
  } else {
    // This function should only be called with the options above.
    assert(false);
  }
}

void Load_dump_options::set_str_unordered_set_option(
    const std::string &option, const std::unordered_set<std::string> &data) {
  if (option == "excludeUsers") {
    if (!m_load_users) {
      if (!data.empty()) {
        throw std::invalid_argument(
            "The 'excludeUsers' option cannot be used if the "
            "'loadUsers' option is set to false.");
      }
    } else {
      try {
        // some users are always excluded
        auto excluded_users = data;
        excluded_users.insert(std::begin(k_excluded_users),
                              std::end(k_excluded_users));

        if (is_mds()) {
          excluded_users.insert(std::begin(k_oci_excluded_users),
                                std::end(k_oci_excluded_users));
        }

        m_excluded_users = shcore::to_accounts(excluded_users);
      } catch (const std::runtime_error &e) {
        throw std::invalid_argument(e.what());
      }
    }
  } else if (option == "includeUsers") {
    if (!m_load_users) {
      if (!data.empty()) {
        throw std::invalid_argument(
            "The 'includeUsers' option cannot be used if the "
            "'loadUsers' option is set to false.");
      }
    } else {
      try {
        m_included_users = shcore::to_accounts(data);
      } catch (const std::runtime_error &e) {
        throw std::invalid_argument(e.what());
      }
    }
  } else {
    // This function should only be called with the options above.
    assert(false);
  }
}

void Load_dump_options::set_max_bytes_per_transaction(
    const std::string &value) {
  if (value.empty()) {
    throw std::invalid_argument(
        "The option 'maxBytesPerTransaction' cannot be set to an empty "
        "string.");
  }

  m_max_bytes_per_transaction = mysqlshdk::utils::expand_to_bytes(value);

  if (m_max_bytes_per_transaction &&
      *m_max_bytes_per_transaction < mysqlshdk::utils::expand_to_bytes(
                                         k_minimum_max_bytes_per_transaction)) {
    throw std::invalid_argument(
        "The value of 'maxBytesPerTransaction' option must be greater than or "
        "equal to " +
        std::string{k_minimum_max_bytes_per_transaction} + ".");
  }
}

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

  // set size of max packet (~size of 1 row) we can send to server
  if (!m_target.has(mysqlshdk::db::kMaxAllowedPacket)) {
    const auto k_one_gb = "1073741824";
    m_target.set(mysqlshdk::db::kMaxAllowedPacket, k_one_gb);
  }

  m_target_server_version = mysqlshdk::utils::Version(
      m_base_session->query("SELECT @@version")->fetch_one()->get_string(0));

  m_is_mds = ::mysqlsh::is_mds(m_target_server_version);
  DBUG_EXECUTE_IF("dump_loader_force_mds", { m_is_mds = true; });

  try {
    m_base_session->query(
        "SELECT @@SESSION.sql_generate_invisible_primary_key;");
    m_auto_create_pks_supported = true;
  } catch (const mysqlshdk::db::Error &e) {
    if (e.code() == ER_UNKNOWN_SYSTEM_VARIABLE) {
      m_auto_create_pks_supported = false;
    } else {
      throw;
    }
  }
}

void Load_dump_options::validate() {
  using mysqlshdk::storage::backend::oci::Par_structure;
  using mysqlshdk::storage::backend::oci::Par_type;
  using mysqlshdk::storage::backend::oci::parse_par;
  Par_structure url_par_data;
  m_par_type = parse_par(m_url, &url_par_data);

  if (m_par_type == Par_type::MANIFEST || m_par_type == Par_type::PREFIX) {
    if (m_progress_file.is_null()) {
      throw shcore::Exception::argument_error(
          "When using a PAR to load a dump, the progressFile option must "
          "be defined");
    } else {
      m_progress_file = shcore::str_strip(*m_progress_file);

      const auto scheme =
          mysqlshdk::storage::utils::get_scheme(*m_progress_file);

      bool is_remote_progress = false;
      Par_type progress_par_type = Par_type::NONE;

      if (!scheme.empty()) {
        if (mysqlshdk::storage::utils::scheme_matches(scheme, "http")) {
          is_remote_progress = true;
        } else if (mysqlshdk::storage::utils::scheme_matches(scheme, "https")) {
          is_remote_progress = true;
          Par_structure progress_par_data;
          progress_par_type = parse_par(*m_progress_file, &progress_par_data);
        }
      }

      if (m_par_type == Par_type::PREFIX && is_remote_progress) {
        throw shcore::Exception::argument_error(
            "When using a prefix PAR to load a dump, the progressFile option "
            "must be a local file");
      } else if (progress_par_type != Par_type::NONE &&
                 progress_par_type != Par_type::GENERAL) {
        throw shcore::Exception::argument_error(
            "Invalid PAR for progress file, use a PAR to a specific file "
            "different than the manifest");
      }

      m_use_par_progress = (progress_par_type == Par_type::GENERAL);
    }

    // TODO(rennox): this should NOT be needed!!
    if (m_par_type == Par_type::MANIFEST) {
      m_oci_options.set_par(m_url);
    }
  } else if (m_par_type == Par_type::GENERAL) {
    current_console()->print_warning(
        "The given URL is not a prefix PAR or a PAR to the @.manifest.json "
        "file.");
  }

  if (Par_type::NONE != m_par_type) {
    // always get the data, we will display it to the user, this will help to
    // figure out what's wrong in case of an error
    m_prefix = url_par_data.object_prefix;
    m_par_object = url_par_data.get_object_path();
  }

  m_oci_options.check_option_values();

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

  m_excluded_users.emplace_back(
      shcore::split_account(m_base_session->query("SELECT current_user()")
                                ->fetch_one()
                                ->get_string(0),
                            true));
}

std::string Load_dump_options::target_import_info() const {
  std::string action;

  std::vector<std::string> what_to_load;

  if (load_ddl()) what_to_load.push_back("DDL");
  if (load_data()) what_to_load.push_back("Data");
  if (load_users()) what_to_load.push_back("Users");

  using mysqlshdk::storage::backend::oci::Par_type;
  std::string where;

  // Par_type::GENERAL is not a valid type for loader, but we check it here to
  // potentially hide the secret information
  if (m_par_type == Par_type::MANIFEST || m_par_type == Par_type::PREFIX ||
      m_par_type == Par_type::GENERAL) {
    where = "OCI PAR=" + mysqlshdk::oci::anonymize_par(m_par_object).masked() +
            ", prefix='" + m_prefix + "'";
  } else if (m_oci_options) {
    where = "OCI ObjectStorage bucket=" + *m_oci_options.os_bucket_name +
            ", prefix='" + m_url + "'";
  } else {
    where = "'" + m_url + "'";
  }

  // this is validated earlier on
  assert(!what_to_load.empty() || m_analyze_tables != Analyze_table_mode::OFF ||
         m_update_gtid_set != Update_gtid_set::OFF);

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
    if (m_analyze_tables == Analyze_table_mode::HISTOGRAM)
      action = "Updating table histograms";
    else if (m_analyze_tables == Analyze_table_mode::ON)
      action = "Updating table histograms and key distribution statistics";
    if (m_update_gtid_set != Update_gtid_set::OFF) {
      if (!action.empty())
        action += ", and updating GTID_PURGED";
      else
        action = "Updating GTID_PURGED";
    }
  }

  std::string detail;

  if (threads_count() == 1)
    detail = " using 1 thread.";
  else
    detail = shcore::str_format(" using %s threads.",
                                std::to_string(threads_count()).c_str());

  return action + detail;
}

void Load_dump_options::on_unpacked_options() {
  m_oci_options = m_oci_option_pack;

  if (!m_load_data && !m_load_ddl && !m_load_users &&
      m_analyze_tables == Analyze_table_mode::OFF &&
      m_update_gtid_set == Update_gtid_set::OFF)
    throw shcore::Exception::argument_error(
        "At least one of loadData, loadDdl or loadUsers options must be "
        "enabled");

  if (!m_load_indexes && m_defer_table_indexes == Defer_index_mode::OFF)
    throw std::invalid_argument(
        "'deferTableIndexes' option needs to be enabled when "
        "'loadIndexes' option is disabled");
}

void Load_dump_options::on_log_options(const char *msg) const {
  std::string s = msg;
  const auto pos = s.find("\"progressFile\":\"https://objectstorage.");

  if (std::string::npos != pos) {
    s = mysqlshdk::oci::hide_par_secret(s, pos);
  }

  log_info("Load options: %s", s.c_str());
}

std::unique_ptr<mysqlshdk::storage::IDirectory>
Load_dump_options::create_dump_handle() const {
  using mysqlshdk::storage::backend::oci::Par_type;
  if (m_par_type == Par_type::MANIFEST) {
    return std::make_unique<Dump_manifest>(Manifest_mode::READ, m_oci_options,
                                           nullptr, m_url);
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

bool Load_dump_options::include_user(const shcore::Account &account) const {
  const auto predicate = [&account](const shcore::Account &a) {
    return a.user == account.user &&
           (a.host.empty() ? true : a.host == account.host);
  };

  if (m_excluded_users.end() != std::find_if(m_excluded_users.begin(),
                                             m_excluded_users.end(),
                                             predicate)) {
    return false;
  }

  if (m_included_users.empty()) {
    return true;
  }

  return m_included_users.end() != std::find_if(m_included_users.begin(),
                                                m_included_users.end(),
                                                predicate);
}

}  // namespace mysqlsh
