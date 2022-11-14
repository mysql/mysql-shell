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
#include "modules/util/common/dump/utils.h"
#include "modules/util/dump/dump_manifest_config.h"
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

using Version = mysqlshdk::utils::Version;

const char *k_excluded_users[] = {"mysql.infoschema", "mysql.session",
                                  "mysql.sys"};
const char *k_oci_excluded_users[] = {"administrator", "ociadmin", "ocimonitor",
                                      "ocirpl"};

constexpr auto k_minimum_max_bytes_per_transaction = 4096;

bool is_mds(const Version &version) {
  return shcore::str_endswith(version.get_extra(), "cloud");
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
Load_dump_options::Load_dump_options(const std::string &url)
    : m_url(url),
      m_blob_storage_options{
          mysqlshdk::azure::Blob_storage_options::Operation::READ} {}

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
          .include(&Load_dump_options::m_filtering_options,
                   &dump::common::Filtering_options::events)
          .include(&Load_dump_options::m_filtering_options,
                   &dump::common::Filtering_options::routines)
          .include(&Load_dump_options::m_filtering_options,
                   &dump::common::Filtering_options::schemas)
          .include(&Load_dump_options::m_filtering_options,
                   &dump::common::Filtering_options::tables)
          .include(&Load_dump_options::m_filtering_options,
                   &dump::common::Filtering_options::triggers)
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
          .include(&Load_dump_options::m_filtering_options,
                   &dump::common::Filtering_options::users)
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
          .include(&Load_dump_options::m_blob_storage_options)
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

void Load_dump_options::set_max_bytes_per_transaction(
    const std::string &value) {
  if (value.empty()) {
    throw std::invalid_argument(
        "The option 'maxBytesPerTransaction' cannot be set to an empty "
        "string.");
  }

  m_max_bytes_per_transaction = mysqlshdk::utils::expand_to_bytes(value);

  if (m_max_bytes_per_transaction.has_value() &&
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

  m_target_server_version =
      Version(query("SELECT @@version")->fetch_one()->get_string(0));

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
        m_sql_generate_invisible_primary_key = std::nullopt;
      }
    }
  }

  m_server_uuid =
      query("SELECT @@server_uuid")->fetch_one_or_throw()->get_string(0);

  if (m_load_users) {
    // some users are always excluded
    filters().users().exclude(k_excluded_users);

    if (is_mds()) {
      filters().users().exclude(k_oci_excluded_users);
    }

    shcore::Account account;

    instance.get_current_user(&account.user, &account.host);

    filters().users().exclude(std::move(account));
  }

  if (m_target_server_version >= Version(8, 0, 27)) {
    // adding indexes in parallel was added in 8.0.27,
    // innodb_parallel_read_threads threads are used during the first stage,
    // innodb_ddl_threads threads are used during second and third stages, in
    // most cases first stage is executed before the rest, so we're using
    // maximum of these two values
    m_threads_per_add_index =
        query(
            "SELECT GREATEST(@@innodb_parallel_read_threads, "
            "@@innodb_ddl_threads)")
            ->fetch_one_or_throw()
            ->get_uint(0);
  }
}

void Load_dump_options::validate() {
  if (!m_storage_config || !m_storage_config->valid()) {
    auto config = par_config(m_url);

    if (config && config->valid()) {
      if (mysqlshdk::oci::PAR_type::MANIFEST == config->par().type() ||
          mysqlshdk::oci::PAR_type::PREFIX == config->par().type()) {
        if (!m_progress_file.has_value()) {
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

  if (!m_progress_file.has_value()) {
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
  m_s3_bucket_options.throw_on_conflict(m_blob_storage_options);
  m_blob_storage_options.throw_on_conflict(m_oci_bucket_options);

  if (m_oci_bucket_options) {
    m_storage_config = m_oci_bucket_options.config();
  }

  if (m_s3_bucket_options) {
    m_storage_config = m_s3_bucket_options.config();
  }

  if (m_blob_storage_options) {
    m_storage_config = m_blob_storage_options.config();
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

  if (!m_load_users) {
    if (!filters().users().excluded().empty()) {
      throw std::invalid_argument(
          "The 'excludeUsers' option cannot be used if the 'loadUsers' option "
          "is set to false.");
    }

    if (!filters().users().included().empty()) {
      throw std::invalid_argument(
          "The 'includeUsers' option cannot be used if the 'loadUsers' option "
          "is set to false.");
    }
  }

  bool has_conflicts = false;

  has_conflicts |= filters().schemas().error_on_conflicts();
  has_conflicts |= filters().tables().error_on_conflicts();
  has_conflicts |= filters().tables().error_on_cross_filters_conflicts();
  has_conflicts |= filters().events().error_on_conflicts();
  has_conflicts |= filters().events().error_on_cross_filters_conflicts();
  has_conflicts |= filters().routines().error_on_conflicts();
  has_conflicts |= filters().routines().error_on_cross_filters_conflicts();
  has_conflicts |= filters().triggers().error_on_conflicts();
  has_conflicts |= filters().triggers().error_on_cross_filters_conflicts();
  has_conflicts |= filters().users().error_on_conflicts();

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
  if (m_progress_file.value_or(std::string{}).empty())
    return create_dump_handle()->file(m_default_progress_file);
  else
    return mysqlshdk::storage::make_file(*m_progress_file,
                                         m_progress_file_config);
}

bool Load_dump_options::include_object(
    std::string_view schema, std::string_view object,
    const std::unordered_set<std::string> &included,
    const std::unordered_set<std::string> &excluded) const {
  assert(!schema.empty());
  assert(!object.empty());

  if (!filters().schemas().is_included(std::string{schema})) return false;

  const auto key = schema_object_key(schema, object);

  if (excluded.count(key) > 0) return false;

  return (included.empty() || included.count(key) > 0);
}

bool Load_dump_options::sql_generate_invisible_primary_key() const {
  assert(auto_create_pks_supported());
  return *m_sql_generate_invisible_primary_key;
}

}  // namespace mysqlsh
