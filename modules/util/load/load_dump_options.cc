/*
 * Copyright (c) 2020, 2024, Oracle and/or its affiliates.
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

#include "modules/util/load/load_dump_options.h"

#include <mysqld_error.h>

#include <algorithm>
#include <array>
#include <iterator>
#include <regex>
#include <utility>

#include "modules/mod_utils.h"
#include "modules/util/common/dump/constants.h"
#include "modules/util/common/dump/utils.h"
#include "modules/util/load/load_errors.h"
#include "mysqlshdk/include/scripting/type_info/custom.h"
#include "mysqlshdk/include/scripting/type_info/generic.h"
#include "mysqlshdk/include/shellcore/console.h"
#include "mysqlshdk/libs/aws/s3_bucket_config.h"
#include "mysqlshdk/libs/mysql/instance.h"
#include "mysqlshdk/libs/oci/oci_par.h"
#include "mysqlshdk/libs/storage/backend/oci_par_directory_config.h"
#include "mysqlshdk/libs/storage/utils.h"
#include "mysqlshdk/libs/utils/debug.h"
#include "mysqlshdk/libs/utils/strformat.h"
#include "mysqlshdk/libs/utils/utils_lexing.h"
#include "mysqlshdk/libs/utils/utils_net.h"
#include "mysqlshdk/libs/utils/utils_string.h"

namespace mysqlsh {
namespace {

using Version = mysqlshdk::utils::Version;

constexpr auto k_minimum_max_bytes_per_transaction = 4096;

std::string bulk_load_s3_prefix(
    const std::shared_ptr<mysqlshdk::aws::S3_bucket_config> &config) {
  // 's3-region://bucket-name/file-name-or-prefix'
  std::string prefix = "s3-";

  prefix += config->region();
  prefix += "://";
  prefix += config->bucket_name();
  prefix += '/';

  return prefix;
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
                   &Filtering_options::events)
          .include(&Load_dump_options::m_filtering_options,
                   &Filtering_options::routines)
          .include(&Load_dump_options::m_filtering_options,
                   &Filtering_options::schemas)
          .include(&Load_dump_options::m_filtering_options,
                   &Filtering_options::tables)
          .include(&Load_dump_options::m_filtering_options,
                   &Filtering_options::triggers)
          .optional("characterSet", &Load_dump_options::m_character_set)
          .optional("skipBinlog", &Load_dump_options::m_skip_binlog)
          .optional("ignoreExistingObjects",
                    &Load_dump_options::m_ignore_existing_objects)
          .optional("dropExistingObjects",
                    &Load_dump_options::m_drop_existing_objects)
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
                   &Filtering_options::users)
          .optional("updateGtidSet", &Load_dump_options::m_update_gtid_set,
                    {{"append", Update_gtid_set::APPEND},
                     {"replace", Update_gtid_set::REPLACE},
                     {"off", Update_gtid_set::OFF}})
          .optional("showMetadata", &Load_dump_options::set_show_metadata)
          .optional("createInvisiblePKs",
                    &Load_dump_options::m_create_invisible_pks)
          .optional("maxBytesPerTransaction",
                    &Load_dump_options::set_max_bytes_per_transaction)
          .optional("sessionInitSql", &Load_dump_options::m_session_init_sql)
          .optional("handleGrantErrors",
                    &Load_dump_options::set_handle_grant_errors)
          .optional("checksum", &Load_dump_options::m_checksum)
          .optional("disableBulkLoad", &Load_dump_options::m_disable_bulk_load)
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

  auto config = dump::common::get_par_config(value);

  if (config && config->valid()) {
    if (mysqlshdk::oci::PAR_type::GENERAL != config->par().type()) {
      throw shcore::Exception::argument_error(
          "Invalid PAR for progress file, use a PAR to a specific file");
    }

    m_progress_file_config = std::move(config);
  }
}

void Load_dump_options::set_session(
    const std::shared_ptr<mysqlshdk::db::ISession> &session) {
  m_base_session = session;

  m_target = get_classic_connection_options(m_base_session);

  if (m_target.has(mysqlshdk::db::kLocalInfile)) {
    m_target.remove(mysqlshdk::db::kLocalInfile);
  }
  m_target.set(mysqlshdk::db::kLocalInfile, "true");

  // Set long timeouts by default
  constexpr auto timeout = 86400000;  // 1 day in milliseconds
  if (!m_target.has_net_read_timeout()) {
    m_target.set_net_read_timeout(timeout);
  }
  if (!m_target.has_net_write_timeout()) {
    m_target.set_net_write_timeout(timeout);
  }

  // set size of max packet (~size of 1 row) we can send to server
  if (!m_target.has(mysqlshdk::db::kMaxAllowedPacket)) {
    const auto k_one_gb = "1073741824";
    m_target.set(mysqlshdk::db::kMaxAllowedPacket, k_one_gb);
  }

  const auto instance = mysqlshdk::mysql::Instance(m_base_session);

  m_target_server_version =
      Version(query("SELECT @@version")->fetch_one()->get_string(0));
  DBUG_EXECUTE_IF("dump_loader_bulk_unsupported_version",
                  { m_target_server_version = Version(8, 3, 0); });

  m_is_mds = m_target_server_version.is_mds();
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
    filters().users().exclude(dump::common::k_excluded_users);

    if (is_mds()) {
      filters().users().exclude(dump::common::k_mhs_excluded_users);
    }

    shcore::Account account;

    instance.get_current_user(&account.user, &account.host);

    filters().users().exclude(std::move(account));
  }

  if (is_mds()) {
    filters().schemas().exclude(dump::common::k_mhs_excluded_schemas);
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

  if (m_target_server_version >= Version(8, 0, 16)) {
    m_partial_revokes =
        instance.get_sysvar_bool("partial_revokes").value_or(false);
  }

  m_lower_case_table_names =
      instance.get_sysvar_int("lower_case_table_names", 0);
}

void Load_dump_options::validate() {
  if (!m_storage_config || !m_storage_config->valid()) {
    auto config = dump::common::get_par_config(m_url);

    if (config && config->valid()) {
      if (mysqlshdk::oci::PAR_type::PREFIX == config->par().type()) {
        if (!m_progress_file.has_value()) {
          throw shcore::Exception::argument_error(
              "When using a PAR to load a dump, the progressFile option must "
              "be defined");
        }
      } else {
        current_console()->print_warning("The given URL is not a prefix PAR.");
      }

      if (mysqlshdk::oci::PAR_type::PREFIX == config->par().type()) {
        // prefix PAR is supported by the bulk loader
        m_bulk_load_info.fs = Bulk_load_fs::URL;
      }

      set_storage_config(std::move(config));
    } else {
      // no configuration, we're loading from the local FS
      m_bulk_load_info.fs = Bulk_load_fs::INFILE;
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
      THROW_ERROR(SHERR_LOAD_LOCAL_INFILE_DISABLED);
    }
  }

  if (!m_progress_file.has_value()) {
    m_default_progress_file = "load-progress." + m_server_uuid + ".json";
  }

  configure_bulk_load();
}

std::string Load_dump_options::target_import_info(
    const char *operation, const std::string &extra) const {
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
         m_update_gtid_set != Update_gtid_set::OFF || m_checksum);

  const auto concat = [&what_to_load]() {
    if (1 == what_to_load.size()) {
      return what_to_load[0] + " only";
    }

    auto msg = shcore::str_join(what_to_load.begin(),
                                std::prev(what_to_load.end()), ", ");
    msg += " and ";
    msg += what_to_load.back();

    return msg;
  };

  if (!what_to_load.empty()) {
    action = shcore::str_format("%s %s from %s", operation, concat().c_str(),
                                where.c_str());
  } else {
    if (m_analyze_tables == Analyze_table_mode::HISTOGRAM) {
      what_to_load.emplace_back("Updating table histograms");
    } else if (m_analyze_tables == Analyze_table_mode::ON) {
      what_to_load.emplace_back(
          "Updating table histograms and key distribution statistics");
    }

    if (m_update_gtid_set != Update_gtid_set::OFF) {
      what_to_load.emplace_back("Updating GTID_PURGED");
    }

    if (m_checksum) {
      what_to_load.emplace_back("Verifying checksum information");
    }

    action = concat();
  }

  std::string detail = extra;

  if (detail.empty()) {
    detail = " using " + std::to_string(threads_count()) + " thread";

    if (threads_count() != 1) {
      detail += 's';
    }
  }

  return action + detail + '.';
}

void Load_dump_options::on_unpacked_options() {
  mysqlshdk::storage::backend::object_storage::throw_on_conflict(
      std::array<const mysqlshdk::storage::backend::object_storage::
                     Object_storage_options *,
                 3>{&m_s3_bucket_options, &m_blob_storage_options,
                    &m_oci_bucket_options});

  if (m_oci_bucket_options) {
    set_storage_config(m_oci_bucket_options.config());
  }

  if (m_s3_bucket_options) {
    auto config = m_s3_bucket_options.s3_config();

    if (config->valid() && m_s3_bucket_options.endpoint_override().empty()) {
      // S3 is supported by the bulk loader
      m_bulk_load_info.fs = Bulk_load_fs::S3;
      m_bulk_load_info.file_prefix = bulk_load_s3_prefix(config);
    }

    set_storage_config(std::move(config));
  }

  if (m_blob_storage_options) {
    set_storage_config(m_blob_storage_options.config());
  }

  if (!m_load_data && !m_load_ddl && !m_load_users &&
      m_analyze_tables == Analyze_table_mode::OFF &&
      m_update_gtid_set == Update_gtid_set::OFF && !m_checksum) {
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

  if (m_drop_existing_objects) {
    if (m_ignore_existing_objects) {
      throw std::invalid_argument(
          "The 'dropExistingObjects' and 'ignoreExistingObjects' options "
          "cannot be both set to true.");
    }

    if (!m_load_ddl) {
      throw std::invalid_argument(
          "The 'dropExistingObjects' option cannot be set to true when the "
          "'loadDdl' option is set to false.");
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

  static const std::regex k_par_progress_file(
      R"("progressFile":"https://(?:[^\.]+\.)?objectstorage\.)");
  std::smatch match;

  if (std::regex_search(s, match, k_par_progress_file)) {
    s = mysqlshdk::oci::hide_par_secret(s, match.position());
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

void Load_dump_options::set_handle_grant_errors(const std::string &action) {
  if ("abort" == action) {
    m_handle_grant_errors = Handle_grant_errors::ABORT;
  } else if ("drop_account" == action) {
    m_handle_grant_errors = Handle_grant_errors::DROP_ACCOUNT;
  } else if ("ignore" == action) {
    m_handle_grant_errors = Handle_grant_errors::IGNORE;
  } else {
    throw std::invalid_argument(
        "The value of the 'handleGrantErrors' option must be set to one of: "
        "'abort', 'drop_account', 'ignore'.");
  }
}

void Load_dump_options::set_storage_config(
    std::shared_ptr<mysqlshdk::storage::Config> storage_config) {
  m_storage_config = std::move(storage_config);
}

void Load_dump_options::configure_bulk_load() {
  if (m_disable_bulk_load) {
    log_info("BULK LOAD is explicitly disabled");
    return;
  }

  if (Bulk_load_fs::UNSUPPORTED == m_bulk_load_info.fs) {
    log_info("BULK LOAD: unsupported FS");
    return;
  }

  if (m_target_server_version < Version(8, 4, 0)) {
    // minimum version which supports required syntax is 8.4.0
    log_info("BULK LOAD: unsupported version");
    return;
  }

  const auto instance = mysqlshdk::mysql::Instance(m_base_session);

  // this is actually an unsigned integer, but allowed values are within int64_t
  // range
  m_bulk_load_info.threads =
      instance.get_sysvar_int("bulk_loader.concurrency").value_or(0);

  if (!m_bulk_load_info.threads) {
    log_info("BULK LOAD: component is not loaded");
    return;
  }

  if (!m_skip_binlog && instance.get_sysvar_bool("log_bin").value_or(false)) {
    // BULK LOAD statement is not replicated
    log_info("BULK LOAD: binlog is enabled and 'skipBinlog' option is false");
    return;
  }

  const auto has_privilege = [&instance](const char *privilege) {
    return !instance.get_current_user_privileges()
                ->validate({privilege})
                .has_missing_privileges();
  };

  switch (m_bulk_load_info.fs) {
    case Bulk_load_fs::UNSUPPORTED:
      log_info("BULK LOAD: unsupported FS");
      return;

    case Bulk_load_fs::INFILE:
      // NOTE: this is for testing purposes only, validation is superficial
      {
        const auto &co = m_base_session->get_connection_options();

        if (mysqlshdk::db::Transport_type::Tcp == co.get_transport_type() &&
            !mysqlshdk::utils::Net::is_local_address(co.get_host())) {
          // in order to use INFILE we need to be connected to the local host
          log_info("BULK LOAD: local FS and a remote host");
          return;
        }  // else we're connected via socket, pipe or to a local TCP address
      }

      if (const auto path = instance.get_sysvar_string("secure_file_priv");
          path.has_value()) {
        if (!path->empty()) {
          // path is not empty, only files in that directory can be loaded
          if (!shcore::str_beginswith(
                  mysqlshdk::storage::make_directory(m_url, storage_config())
                      ->full_path()
                      .real(),
                  mysqlshdk::storage::make_directory(*path, storage_config())
                      ->full_path()
                      .real())) {
            log_info(
                "BULK LOAD: local FS and dump is not a subdirectory of "
                "'secure_file_priv'");
            return;
          }
        }  // else path is empty, files can be loaded from anywhere
      } else {
        // variable is set to NULL, LOAD DATA INFILE is disabled
        log_info("BULK LOAD: local FS and 'secure_file_priv' is NULL");
        return;
      }

      break;

    case Bulk_load_fs::URL:
      if (!has_privilege("LOAD_FROM_URL")) {
        log_info(
            "BULK LOAD: OCI prefix PAR and 'LOAD_FROM_URL' privilege is "
            "missing");
        return;
      }

      break;

    case Bulk_load_fs::S3:
      if (!has_privilege("LOAD_FROM_S3")) {
        log_info("BULK LOAD: AWS S3 and 'LOAD_FROM_S3' privilege is missing");
        return;
      }

      break;
  }

  // BULK LOAD is supported
  log_info("BULK LOAD is supported");
  m_bulk_load_info.enabled = true;

  // check if monitoring is enabled
  auto result = instance.query(
      "SELECT 1 FROM performance_schema.setup_consumers WHERE ENABLED='YES' "
      "AND NAME='events_stages_current'");

  if (!result->fetch_one()) {
    log_info("BULK LOAD monitoring: consumer is disabled");
    return;
  }

  result = instance.query(
      "SELECT 1 FROM performance_schema.setup_instruments WHERE ENABLED='YES' "
      "AND NAME='stage/bulk_load_sorted/loading'");

  if (!result->fetch_one()) {
    log_info("BULK LOAD monitoring: instrumentation is disabled");
    return;
  }

  log_info("BULK LOAD monitoring is enabled");
  m_bulk_load_info.monitoring = true;
}

}  // namespace mysqlsh
