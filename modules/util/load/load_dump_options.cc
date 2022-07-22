/*
 * Copyright (c) 2020, 2022, Oracle and/or its affiliates.
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
#include "modules/util/dump/dump_manifest_config.h"
#include "modules/util/dump/dump_utils.h"
#include "modules/util/load/load_errors.h"
#include "mysqlshdk/include/scripting/type_info/custom.h"
#include "mysqlshdk/include/scripting/type_info/generic.h"
#include "mysqlshdk/include/shellcore/console.h"
#include "mysqlshdk/libs/mysql/instance.h"
#include "mysqlshdk/libs/oci/oci_par.h"
#include "mysqlshdk/libs/storage/backend/oci_par_directory_config.h"
#include "mysqlshdk/libs/storage/utils.h"
#include "mysqlshdk/libs/utils/debug.h"
#include "mysqlshdk/libs/utils/strformat.h"
#include "mysqlshdk/libs/utils/utils_lexing.h"
#include "mysqlshdk/libs/utils/utils_string.h"

namespace mysqlsh {
namespace {

const char *k_excluded_users[] = {"mysql.infoschema", "mysql.session",
                                  "mysql.sys"};
const char *k_oci_excluded_users[] = {"administrator", "ociadmin", "ocimonitor",
                                      "ocirpl"};

constexpr auto k_minimum_max_bytes_per_transaction = 4096;

bool is_mds(const mysqlshdk::utils::Version &version) {
  return shcore::str_endswith(version.get_extra(), "cloud");
}

void parse_objects(const std::vector<std::string> &opt_objects,
                   const std::string &object_name,
                   std::unordered_set<std::string> *out_filter,
                   bool add_schema_entry = false) {
  const auto throw_invalid_argument = [&object_name](const std::string &entry) {
    throw std::invalid_argument(
        "Can't parse " + object_name + " filter '" + entry + "'. The " +
        object_name + " must be in the following form: schema." + object_name +
        ", with optional backtick quotes.");
  };

  for (const auto &entry : opt_objects) {
    std::string schema, object;

    try {
      shcore::split_schema_and_table(entry, &schema, &object);
    } catch (const std::exception &e) {
      throw_invalid_argument(entry);
    }

    if (schema.empty()) {
      throw_invalid_argument(entry);
    }

    out_filter->insert(schema_object_key(schema, object));

    if (add_schema_entry) {
      // insert schema."", so that we can check whether we want to include
      // one or more tables for a given schema, but not the whole schema
      out_filter->insert(schema_object_key(schema, ""));
    }
  }
}

void parse_triggers(const std::vector<std::string> &opt_triggers,
                    std::unordered_set<std::string> *out_filter) {
  const auto throw_invalid_argument = [](const std::string &entry) {
    throw std::invalid_argument(
        "Can't parse trigger filter '" + entry +
        "'. The filter must be in the following form: schema.table or "
        "schema.table.trigger, with optional backtick quotes.");
  };

  for (const auto &entry : opt_triggers) {
    std::string schema, table, object;

    try {
      shcore::split_schema_table_and_object(entry, &schema, &table, &object);
    } catch (const std::exception &e) {
      throw_invalid_argument(entry);
    }

    if (table.empty()) {
      throw_invalid_argument(entry);
    }

    if (schema.empty()) {
      // we got schema.table, need to move names around
      std::swap(schema, table);
      std::swap(table, object);
    }

    // if object is empty (all triggers are included/excluded), `schema`.`table`
    // will be inserted
    out_filter->insert(schema_table_object_key(schema, table, object));
  }
}

std::shared_ptr<mysqlshdk::oci::IPAR_config> par_config(
    const std::string &url) {
  mysqlshdk::oci::PAR_structure par;

  switch (mysqlshdk::oci::parse_par(url, &par)) {
    case mysqlshdk::oci::PAR_type::MANIFEST:
      return std::make_shared<dump::Dump_manifest_read_config>(par);

    case mysqlshdk::oci::PAR_type::PREFIX:
      return std::make_shared<
          mysqlshdk::storage::backend::oci::Oci_par_directory_config>(par);

    case mysqlshdk::oci::PAR_type::GENERAL:
      return std::make_shared<mysqlshdk::oci::General_par_config>(par);

    case mysqlshdk::oci::PAR_type::NONE:
      break;
  }

  return {};
}

}  // namespace

Load_dump_options::Load_dump_options() : Load_dump_options("") {}
Load_dump_options::Load_dump_options(const std::string &url) : m_url(url) {
  // some users are always excluded
  add_excluded_users(shcore::to_accounts(k_excluded_users));
}

const shcore::Option_pack_def<Load_dump_options> &Load_dump_options::options() {
  static const auto opts =
      shcore::Option_pack_def<Load_dump_options>()
          .optional("threads", &Load_dump_options::m_threads_count)
          .optional("backgroundThreads",
                    &Load_dump_options::m_background_threads_count)
          .optional("showProgress", &Load_dump_options::m_show_progress)
          .optional("waitDumpTimeout", &Load_dump_options::set_wait_timeout)
          .optional("loadData", &Load_dump_options::m_load_data)
          .optional("loadDdl", &Load_dump_options::m_load_ddl)
          .optional("loadUsers", &Load_dump_options::m_load_users)
          .optional("dryRun", &Load_dump_options::m_dry_run)
          .optional("resetProgress", &Load_dump_options::m_reset_progress)
          .optional("progressFile", &Load_dump_options::set_progress_file)
          .optional("includeEvents", &Load_dump_options::set_str_vector_option)
          .optional("includeRoutines",
                    &Load_dump_options::set_str_vector_option)
          .optional("includeSchemas", &Load_dump_options::set_str_vector_option)
          .optional("includeTables", &Load_dump_options::set_str_vector_option)
          .optional("includeTriggers",
                    &Load_dump_options::set_str_vector_option)
          .optional("excludeEvents", &Load_dump_options::set_str_vector_option)
          .optional("excludeRoutines",
                    &Load_dump_options::set_str_vector_option)
          .optional("excludeSchemas", &Load_dump_options::set_str_vector_option)
          .optional("excludeTables", &Load_dump_options::set_str_vector_option)
          .optional("excludeTriggers",
                    &Load_dump_options::set_str_vector_option)
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
          .optional("sessionInitSql", &Load_dump_options::m_session_init_sql)
          .include(&Load_dump_options::m_oci_bucket_options)
          .include(&Load_dump_options::m_s3_bucket_options)
          .on_done(&Load_dump_options::on_unpacked_options)
          .on_log(&Load_dump_options::on_log_options);

  return opts;
}

void Load_dump_options::set_wait_timeout(const double &timeout_seconds) {
  // we're using double here, so that tests can set it to millisecond values
  if (timeout_seconds > 0.0) {
    m_wait_dump_timeout_ms = timeout_seconds * 1000;
  }
}

void Load_dump_options::set_str_vector_option(
    const std::string &option, const std::vector<std::string> &data) {
  if (option == "includeEvents") {
    parse_objects(data, "event", &m_include_events);
  } else if (option == "includeRoutines") {
    parse_objects(data, "routine", &m_include_routines);
  } else if (option == "includeSchemas") {
    for (const auto &schema : data) {
      if (!schema.empty() && schema[0] != '`')
        m_include_schemas.insert(shcore::quote_identifier(schema));
      else
        m_include_schemas.insert(schema);
    }
  } else if (option == "includeTables") {
    parse_objects(data, "table", &m_include_tables, true);
  } else if (option == "includeTriggers") {
    parse_triggers(data, &m_include_triggers);
  } else if (option == "excludeEvents") {
    parse_objects(data, "event", &m_exclude_events);
  } else if (option == "excludeRoutines") {
    parse_objects(data, "routine", &m_exclude_routines);
  } else if (option == "excludeSchemas") {
    for (const auto &schema : data) {
      if (!schema.empty() && schema[0] != '`')
        m_exclude_schemas.insert(shcore::quote_identifier(schema));
      else
        m_exclude_schemas.insert(schema);
    }
  } else if (option == "excludeTables") {
    parse_objects(data, "table", &m_exclude_tables);
  } else if (option == "excludeTriggers") {
    parse_triggers(data, &m_exclude_triggers);
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
        add_excluded_users(shcore::to_accounts(data));
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
      *m_max_bytes_per_transaction < k_minimum_max_bytes_per_transaction) {
    throw std::invalid_argument(
        "The value of 'maxBytesPerTransaction' option must be greater than or "
        "equal to " +
        std::to_string(k_minimum_max_bytes_per_transaction) + " bytes.");
  }
}

void Load_dump_options::set_progress_file(const std::string &value) {
  m_progress_file = value;

  auto config = par_config(value);

  if (config && config->valid()) {
    if (mysqlshdk::oci::PAR_type::GENERAL != config->par().type()) {
      throw shcore::Exception::argument_error(
          "Invalid PAR for progress file, use a PAR to a specific file "
          "different than the manifest");
    }

    m_progress_file_config = std::move(config);
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

  const auto instance = mysqlshdk::mysql::Instance(m_base_session);

  m_target_server_version = mysqlshdk::utils::Version(
      query("SELECT @@version")->fetch_one()->get_string(0));

  m_is_mds = ::mysqlsh::is_mds(m_target_server_version);
  DBUG_EXECUTE_IF("dump_loader_force_mds", { m_is_mds = true; });

  m_sql_generate_invisible_primary_key =
      instance.get_sysvar_bool("sql_generate_invisible_primary_key");

  if (auto_create_pks_supported()) {
    // server supports sql_generate_invisible_primary_key, now let's see if
    // user can actually set it
    bool user_can_set_var = false;

    try {
      // try to set the session variable to the same value as global variable,
      // to check if user can set it
      m_base_session->executef(
          "SET @@SESSION.sql_generate_invisible_primary_key=?",
          sql_generate_invisible_primary_key());
      user_can_set_var = true;
    } catch (const std::exception &e) {
      log_warning(
          "The current user cannot set the sql_generate_invisible_primary_key "
          "session variable: %s",
          e.what());
    }

    if (!user_can_set_var) {
      // BUG#34408669 - when user doesn't have any of the privileges required to
      // set the sql_generate_invisible_primary_key session variable, fall back
      // to inserting the key manually, but only if global value is OFF; if it's
      // ON, then falling back would mean that PKs are created even if user
      // does not want this, we override this behaviour with
      // MYSQLSH_ALLOW_ALWAYS_GIPK environment variable
      if (!sql_generate_invisible_primary_key() ||
          getenv("MYSQLSH_ALLOW_ALWAYS_GIPK")) {
        m_sql_generate_invisible_primary_key.reset();
      }
    }
  }

  m_server_uuid =
      query("SELECT @@server_uuid")->fetch_one_or_throw()->get_string(0);

  if (m_load_users) {
    if (is_mds()) {
      add_excluded_users(shcore::to_accounts(k_oci_excluded_users));
    }

    shcore::Account account;

    instance.get_current_user(&account.user, &account.host);

    m_excluded_users.emplace_back(std::move(account));
  }
}

void Load_dump_options::validate() {
  if (!m_storage_config || !m_storage_config->valid()) {
    auto config = par_config(m_url);

    if (config && config->valid()) {
      if (mysqlshdk::oci::PAR_type::MANIFEST == config->par().type() ||
          mysqlshdk::oci::PAR_type::PREFIX == config->par().type()) {
        if (m_progress_file.is_null()) {
          throw shcore::Exception::argument_error(
              "When using a PAR to load a dump, the progressFile option must "
              "be defined");
        }
      } else {
        current_console()->print_warning(
            "The given URL is not a prefix PAR or a PAR to the @.manifest.json "
            "file.");
      }

      m_storage_config = std::move(config);
    }
  }

  {
    const auto result = query("SHOW GLOBAL VARIABLES LIKE 'local_infile'");
    const auto row = result->fetch_one();
    const auto local_infile_value = row->get_string(1);

    if (shcore::str_caseeq(local_infile_value, "off")) {
      mysqlsh::current_console()->print_error(
          "The 'local_infile' global system variable must be set to ON in "
          "the target server, after the server is verified to be trusted.");
      THROW_ERROR0(SHERR_LOAD_LOCAL_INFILE_DISABLED);
    }
  }

  if (m_progress_file.is_null()) {
    m_default_progress_file = "load-progress." + m_server_uuid + ".json";
  }
}

std::string Load_dump_options::target_import_info() const {
  std::string action;

  std::vector<std::string> what_to_load;

  if (load_ddl()) what_to_load.push_back("DDL");
  if (load_data()) what_to_load.push_back("Data");
  if (load_users()) what_to_load.push_back("Users");

  std::string where;

  if (m_storage_config && m_storage_config->valid()) {
    where = m_storage_config->describe(m_url);
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
  m_s3_bucket_options.throw_on_conflict(m_oci_bucket_options);

  if (m_oci_bucket_options) {
    m_storage_config = m_oci_bucket_options.config();
  }

  if (m_s3_bucket_options) {
    m_storage_config = m_s3_bucket_options.config();
  }

  if (!m_load_data && !m_load_ddl && !m_load_users &&
      m_analyze_tables == Analyze_table_mode::OFF &&
      m_update_gtid_set == Update_gtid_set::OFF) {
    throw shcore::Exception::argument_error(
        "At least one of loadData, loadDdl or loadUsers options must be "
        "enabled");
  }

  if (!m_load_indexes && m_defer_table_indexes == Defer_index_mode::OFF) {
    throw std::invalid_argument(
        "'deferTableIndexes' option needs to be enabled when "
        "'loadIndexes' option is disabled");
  }

  bool has_conflicts = false;

  has_conflicts |= error_on_object_filters_conflicts(
      m_include_schemas, m_exclude_schemas, "a schema", "Schemas");
  has_conflicts |= error_on_object_filters_conflicts(
      m_include_tables, m_exclude_tables, "a table", "Tables");
  has_conflicts |= error_on_object_filters_conflicts(
      m_include_events, m_exclude_events, "an event", "Events");
  has_conflicts |= error_on_object_filters_conflicts(
      m_include_routines, m_exclude_routines, "a routine", "Routines");
  has_conflicts |= error_on_trigger_filters_conflicts();
  has_conflicts |= ::mysqlsh::dump::error_on_user_filters_conflicts(
      m_included_users, m_excluded_users);

  if (has_conflicts) {
    throw std::invalid_argument("Conflicting filtering options");
  }
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
  return mysqlshdk::storage::make_directory(m_url, storage_config());
}

std::unique_ptr<mysqlshdk::storage::IFile>
Load_dump_options::create_progress_file_handle() const {
  if (m_progress_file.get_safe().empty())
    return create_dump_handle()->file(m_default_progress_file);
  else
    return mysqlshdk::storage::make_file(*m_progress_file,
                                         m_progress_file_config);
}

// Filtering works as:
// (includeSchemas + includeTables || *) - excludeSchemas - excludeTables
bool Load_dump_options::include_schema(const std::string &schema) const {
  const auto qschema = shcore::quote_identifier(schema);

  if (m_exclude_schemas.count(qschema) > 0) return false;

  // If includeSchemas neither includeTables are given, then all schemas
  // are included by default
  if ((m_include_schemas.empty() && m_include_tables.empty()) ||
      m_include_schemas.count(qschema) > 0 ||
      m_include_tables.count(qschema) > 0) {
    return true;
  }

  return false;
}

bool Load_dump_options::include_table(const std::string &schema,
                                      const std::string &table) const {
  return include_object(schema, table, m_include_tables, m_exclude_tables);
}

bool Load_dump_options::include_event(const std::string &schema,
                                      const std::string &event) const {
  return include_object(schema, event, m_include_events, m_exclude_events);
}

bool Load_dump_options::include_routine(const std::string &schema,
                                        const std::string &routine) const {
  return include_object(schema, routine, m_include_routines,
                        m_exclude_routines);
}

bool Load_dump_options::include_object(
    const std::string &schema, const std::string &object,
    const std::unordered_set<std::string> &included,
    const std::unordered_set<std::string> &excluded) const {
  assert(!schema.empty());
  assert(!object.empty());

  if (include_schema(schema)) {
    const auto key = schema_object_key(schema, object);

    if (excluded.count(key) > 0) {
      return false;
    }

    if (included.empty() || included.count(key) > 0) {
      return true;
    }
  }

  return false;
}

bool Load_dump_options::include_trigger(const std::string &schema,
                                        const std::string &table,
                                        const std::string &trigger) const {
  assert(!schema.empty());
  assert(!table.empty());
  assert(!trigger.empty());

  if (include_table(schema, table)) {
    // filters for triggers contain either `schema`.`table` or
    // `schema`.`table`.`trigger` entries
    const auto table_key = schema_object_key(schema, table);
    const auto trigger_key = schema_table_object_key(schema, table, trigger);

    if (m_exclude_triggers.count(table_key) > 0 ||
        m_exclude_triggers.count(trigger_key) > 0) {
      return false;
    }

    if (m_include_triggers.empty() || m_include_triggers.count(table_key) > 0 ||
        m_include_triggers.count(trigger_key) > 0) {
      return true;
    }
  }

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

void Load_dump_options::add_excluded_users(
    std::vector<shcore::Account> &&users) {
  std::move(users.begin(), users.end(), std::back_inserter(m_excluded_users));
}

bool Load_dump_options::error_on_object_filters_conflicts(
    const std::unordered_set<std::string> &included,
    const std::unordered_set<std::string> &excluded,
    const std::string &object_label, const std::string &option_suffix) const {
  const auto included_is_smaller = included.size() < excluded.size();
  const auto &needle = included_is_smaller ? included : excluded;
  const auto &haystack = !included_is_smaller ? included : excluded;
  const auto console = current_console();
  bool conflict = false;

  for (const auto &object : needle) {
    if (haystack.count(object)) {
      conflict = true;
      console->print_error("Both include" + option_suffix + " and exclude" +
                           option_suffix + " options contain " + object_label +
                           " " + object + ".");
    }
  }

  const auto cross_check_with_schema_filters = [this, &object_label,
                                                &option_suffix,
                                                &conflict](const auto &list,
                                                           bool is_included) {
    const std::string prefix = is_included ? "include" : "exclude";

    const auto schema_name = [](const std::string &o) {
      // extract the schema part from the object name
      return o.substr(0, mysqlshdk::utils::span_quoted_sql_identifier_bt(o, 0));
    };

    for (const auto &object : list) {
      const auto schema = schema_name(object);

      // some object filter lists contain a filter that's just the schema name,
      // ignore those
      if (schema == object) {
        continue;
      }

      // excluding an object from an excluded schema is redundant, but not an
      // error
      if (is_included && m_exclude_schemas.count(schema) > 0) {
        conflict = true;
        current_console()->print_error("The " + prefix + option_suffix +
                                       " option contains " + object_label +
                                       " " + object +
                                       " which refers to an excluded schema.");
      }

      if (!m_include_schemas.empty() && 0 == m_include_schemas.count(schema)) {
        conflict = true;
        current_console()->print_error(
            "The " + prefix + option_suffix + " option contains " +
            object_label + " " + object +
            " which refers to a schema which was not included in the dump.");
      }
    }
  };

  // cross checking schemas vs schemas does not make sense
  if (&included != &m_include_schemas) {
    cross_check_with_schema_filters(included, true);
    cross_check_with_schema_filters(excluded, false);
  }

  return conflict;
}

bool Load_dump_options::error_on_trigger_filters_conflicts() const {
  std::string schema;
  std::string table;
  std::string trigger;
  bool conflict = false;

  const auto console = current_console();

  for (const auto &excluded : m_exclude_triggers) {
    shcore::split_schema_table_and_object(excluded, &schema, &table, &trigger);

    // check exact matches first
    if (m_include_triggers.count(excluded)) {
      const std::string object_name = schema.empty() ? "filter" : "trigger";
      conflict = true;
      console->print_error(
          "Both includeTriggers and excludeTriggers options contain a " +
          object_name + " " + excluded + ".");
    }

    if (schema.empty()) {
      // search for schema.table.trigger includes, which are excluded using
      // schema.table filter
      const auto prefix = excluded + '.';

      for (const auto &included : m_include_triggers) {
        if (shcore::str_beginswith(included, prefix)) {
          conflict = true;
          console->print_error(
              "The includeTriggers option contains a trigger " + included +
              " which is excluded by the value of the excludeTriggers "
              "option: " +
              excluded + ".");
        }
      }
    }
  }

  const auto cross_check_with_filters =
      [&conflict](
          const auto &list, bool is_included, const std::string &filter_type,
          const std::unordered_set<std::string> &include_filter,
          const std::unordered_set<std::string> &exclude_filter,
          const std::function<std::string(const std::string &)> &get_name) {
        const std::string prefix = is_included ? "include" : "exclude";

        const auto print_error = [](const std::string &object,
                                    const std::string &format) {
          std::string s;
          shcore::split_schema_table_and_object(object, &s, nullptr, nullptr);
          current_console()->print_error(shcore::str_format(
              format.c_str(), s.empty() ? "filter" : "trigger",
              object.c_str()));
        };

        for (const auto &filter : list) {
          const auto object_name = get_name(filter);

          // excluding an object from an excluded schema is redundant, but not
          // an error
          if (is_included && exclude_filter.count(object_name) > 0) {
            conflict = true;
            print_error(filter, "The " + prefix +
                                    "Triggers option contains a %s %s which "
                                    "refers to an excluded " +
                                    filter_type + ".");
          }

          if (!include_filter.empty() &&
              0 == include_filter.count(object_name)) {
            conflict = true;
            print_error(
                filter,
                "The " + prefix +
                    "Triggers option contains a %s %s which refers to a " +
                    filter_type + " which was not included in the dump.");
          }
        }
      };

  const auto schema_name = [](const std::string &o) {
    // extract the schema part from the object name
    return o.substr(0, mysqlshdk::utils::span_quoted_sql_identifier_bt(o, 0));
  };

  cross_check_with_filters(m_include_triggers, true, "schema",
                           m_include_schemas, m_exclude_schemas, schema_name);
  cross_check_with_filters(m_exclude_triggers, false, "schema",
                           m_include_schemas, m_exclude_schemas, schema_name);

  const auto table_name = [](const std::string &o) {
    // extract the table part from the object name
    auto pos = mysqlshdk::utils::span_quoted_sql_identifier_bt(o, 0);
    // move to the next backtick
    ++pos;
    // find the end of table
    pos = mysqlshdk::utils::span_quoted_sql_identifier_bt(o, pos);

    return o.substr(0, pos);
  };

  cross_check_with_filters(m_include_triggers, true, "table", m_include_tables,
                           m_exclude_tables, table_name);
  cross_check_with_filters(m_exclude_triggers, false, "table", m_include_tables,
                           m_exclude_tables, table_name);

  return conflict;
}

bool Load_dump_options::sql_generate_invisible_primary_key() const {
  assert(auto_create_pks_supported());
  return *m_sql_generate_invisible_primary_key;
}

}  // namespace mysqlsh
