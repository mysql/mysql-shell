/*
 * Copyright (c) 2019, 2021, Oracle and/or its affiliates.
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

/*
  General notes about metadata management:

  - Both private and public metadata schema must follow semantic versioning.
  - Major version of public metadata interface must be backwards compatible.
    v1 should always be kept around as a fallback public interface.
 */

#include "modules/adminapi/common/metadata_management_mysql.h"

#include <algorithm>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "modules/adminapi/common/common.h"
#include "modules/adminapi/common/instance_pool.h"
#include "modules/adminapi/common/metadata_backup_handler.h"
#include "modules/adminapi/common/metadata_storage.h"
#include "mysqlshdk/include/shellcore/console.h"
#include "mysqlshdk/libs/db/mysql/session.h"
#include "mysqlshdk/libs/mysql/utils.h"
#include "mysqlshdk/libs/utils/debug.h"
#include "mysqlshdk/libs/utils/logger.h"
#include "mysqlshdk/libs/utils/utils_file.h"
#include "mysqlshdk/libs/utils/utils_path.h"
#include "mysqlshdk/libs/utils/utils_string.h"

using mysqlshdk::utils::Version;

namespace mysqlsh {
namespace dba {
namespace metadata {

// Must be kept up-to-date with the latest version of the metadata schema
constexpr const char k_metadata_schema_version[] = "2.1.0";
// Must be kept up-to-date with the list of all schema versions ever released
constexpr const char *k_metadata_schema_version_history[] = {"1.0.1", "2.0.0",
                                                             "2.1.0"};

namespace {
constexpr char kMetadataSchemaPreviousName[] =
    "mysql_innodb_cluster_metadata_previous";

constexpr char kMetadataUpgradeLock[] =
    "mysql_innodb_cluster_metadata.upgrade_in_progress";

constexpr char kMetadataSchemaVersion[] = "schema_version";

Version get_version(const std::shared_ptr<Instance> &group_server,
                    const std::string &schema = kMetadataSchemaName,
                    const std::string &view = kMetadataSchemaVersion) {
  try {
    auto result =
        group_server->queryf("SELECT `major`, `minor`, `patch` FROM !.!",
                             schema.c_str(), view.c_str());
    auto row = result->fetch_one_or_throw();

    return Version(row->get_int(0), row->get_int(1), row->get_int(2));
  } catch (const shcore::Error &error) {
    if (error.code() == ER_TABLEACCESS_DENIED_ERROR) {
      throw std::runtime_error(
          "Unable to detect Metadata version. Please check account "
          "privileges.");
    } else if (error.code() != ER_NO_SUCH_TABLE &&
               error.code() != ER_BAD_DB_ERROR) {
      throw;
    }
  }

  return kNotInstalled;
}

using Instance_list = std::list<Scoped_instance>;

Version set_schema_version(const std::shared_ptr<Instance> &group_server,
                           const Version &version,
                           const std::string &view = kMetadataSchemaVersion) {
  log_debug("Updating metadata version to %s.", version.get_base().c_str());

  group_server->executef(
      "CREATE OR REPLACE SQL SECURITY INVOKER VIEW !.! (major, "
      "minor, patch) AS SELECT ?, ?, ?",
      kMetadataSchemaName, view.c_str(), version.get_major(),
      version.get_minor(), version.get_patch());

  return version;
}

bool schema_exists(const std::shared_ptr<Instance> &group_server,
                   const std::string &name) {
  auto result = group_server->queryf("SHOW DATABASES LIKE ?", name.c_str());

  auto row = result->fetch_one();

  return row ? true : false;
}
}  // namespace

namespace scripts {

namespace {
std::string get_script_path(const std::string &file) {
  std::string path;
  path = shcore::path::join_path(shcore::get_share_folder(),
                                 "adminapi-metadata", file);

  if (!shcore::path::exists(path))
    throw std::runtime_error(path +
                             ": not found, shell installation likely invalid");

  return path;
}

std::string strip_comments(const std::string &text) {
  return shcore::str_subvars(
      text, [](const std::string &) { return ""; }, "/*", "*/");
}
}  // namespace

std::string get_metadata_script(const mysqlshdk::utils::Version &version) {
  return strip_comments(
      shcore::get_text_file(get_script_path(shcore::str_format(
          "metadata-model-%s.sql", version.get_base().c_str()))));
}

std::string get_metadata_upgrade_script(
    const mysqlshdk::utils::Version &version) {
  auto full_script = get_metadata_script(version);

  auto script =
      strip_comments(shcore::get_text_file(get_script_path(shcore::str_format(
          "metadata-upgrade-%s.sql", version.get_base().c_str()))));

  return shcore::str_replace(
      script,
      "-- Deploy: METADATA_MODEL_" +
          shcore::str_replace(version.get_base(), ".", "_"),
      full_script);
}

}  // namespace scripts

namespace upgrade {
enum class Stage {
  OK,                       // implicit
  NONE,                     // implicit
  SETTING_UPGRADE_VERSION,  // persisted
  UPGRADING,                // persisted
  DONE,                     // persisted
  CLEANUP                   // derived from stage + schema_version
};

std::string to_string(Stage stage) {
  switch (stage) {
    case Stage::OK:
      return "OK";
    case Stage::NONE:
      return "NONE";
    case Stage::SETTING_UPGRADE_VERSION:
      return "SETTING_UPGRADE_VERSION";
    case Stage::UPGRADING:
      return "UPGRADING";
    case Stage::DONE:
      return "DONE";
    case Stage::CLEANUP:
      return "CLEANUP";
  }
  throw std::logic_error("internal error");
}

Stage to_stage(const std::string &stage) {
  if (stage == "OK")
    return Stage::OK;
  else if (stage == "NONE")
    return Stage::NONE;
  else if (stage == "SETTING_UPGRADE_VERSION")
    return Stage::SETTING_UPGRADE_VERSION;
  else if (stage == "UPGRADING")
    return Stage::UPGRADING;
  else if (stage == "DONE")
    return Stage::DONE;
  else if (stage == "CLEANUP")
    return Stage::CLEANUP;
  else
    throw std::logic_error("internal error");
}

Stage save_stage(const std::shared_ptr<Instance> &group_server, Stage stage) {
  log_debug("Updating metadata upgrade stage to: %s",
            upgrade::to_string(stage).c_str());

  group_server->executef(
      "CREATE OR REPLACE SQL SECURITY INVOKER VIEW !.! (stage) AS SELECT ?",
      kMetadataSchemaBackupName, "backup_stage", to_string(stage));

  return stage;
}

Stage compute_failed_upgrade_stage(
    const mysqlshdk::utils::nullable<Stage> &saved_stage,
    const Version &schema_version, bool backup_exists) {
  bool at_target_version = schema_version == current_version();
  bool at_upgrading_version = schema_version == kUpgradingVersion;
  bool at_original_version = !at_upgrading_version &&
                             schema_version != kNotInstalled &&
                             !at_target_version;

  Stage actual_stage;

  if (!backup_exists && at_upgrading_version)
    actual_stage = upgrade::Stage::CLEANUP;
  else if (saved_stage == Stage::DONE && at_upgrading_version)
    actual_stage = upgrade::Stage::DONE;
  else if (saved_stage == Stage::UPGRADING)
    // all possible version values handled the same way, including original,
    // 0.0.0 and NULL/missing. latest version is impossible, but allowed
    actual_stage = Stage::UPGRADING;
  else if (saved_stage == Stage::SETTING_UPGRADE_VERSION &&
           at_upgrading_version)
    actual_stage = Stage::SETTING_UPGRADE_VERSION;
  else if (saved_stage == Stage::SETTING_UPGRADE_VERSION && at_original_version)
    actual_stage = Stage::NONE;
  else if (backup_exists && saved_stage.is_null() && at_original_version)
    actual_stage = Stage::NONE;
  else if (!backup_exists && at_target_version)
    actual_stage = upgrade::Stage::OK;
  else if (!backup_exists && at_original_version)
    actual_stage = upgrade::Stage::OK;
  else  // not supposed to reach here
    throw std::logic_error(
        "Metadata schema upgrade was left in an inconsistent state");

  std::string state_info = shcore::str_format(
      "Detected state of MD schema as %s... schema_version=%s, "
      "target_version=%s, backup_exists=%i, saved_stage=%s",
      to_string(actual_stage).c_str(), schema_version.get_base().c_str(),
      current_version().get_base().c_str(), backup_exists,
      saved_stage.is_null() ? "null" : to_string(*saved_stage).c_str());

  if (actual_stage != upgrade::Stage::OK) {
    log_info("%s", state_info.c_str());
  } else {
    log_debug("%s", state_info.c_str());
  }

  return actual_stage;
}

/**
 * This function is called knowing that there's a metadata backup schema
 */
Stage detect_failed_stage(const std::shared_ptr<Instance> &group_server) {
  mysqlshdk::utils::nullable<Stage> saved_stage;
  bool backup_exists = schema_exists(group_server, kMetadataSchemaBackupName);
  auto schema_version = get_version(group_server);

  if (backup_exists) {
    try {
      auto result = group_server->queryf(
          "SELECT `stage` FROM !.!", kMetadataSchemaBackupName, "backup_stage");
      auto row = result->fetch_one_or_throw();

      saved_stage = to_stage(row->get_string(0));
      backup_exists = true;
    } catch (const shcore::Error &error) {
      if (error.code() == ER_TABLEACCESS_DENIED_ERROR) {
        throw std::runtime_error(
            "Unable to detect Metadata upgrade state. Please check account "
            "privileges.");
      } else if (error.code() != ER_NO_SUCH_TABLE &&
                 error.code() != ER_BAD_DB_ERROR) {
        throw;
      }
    }
  }

  return compute_failed_upgrade_stage(saved_stage, schema_version,
                                      backup_exists);
}

namespace steps {

void upgrade_101_200(const std::shared_ptr<Instance> &group_server) {
  DBUG_EXECUTE_IF("dba_FAIL_metadata_upgrade_at_UPGRADING", {
    throw std::logic_error(
        "Debug emulation of failed metadata upgrade "
        "UPGRADING.");
  });
  auto result = group_server->queryf("SELECT COUNT(*) FROM !.!",
                                     kMetadataSchemaName, "clusters");

  auto row = result->fetch_one();

  int count = row->get_int(0);
  if (count > 1) {
    throw std::runtime_error(
        shcore::str_format("Inconsistent Metadata: unexpected number of "
                           "registered clusters: %d",
                           count)
            .c_str());
  }

  execute_script(
      group_server,
      scripts::get_metadata_upgrade_script(mysqlshdk::utils::Version("2.0.0")),
      "While upgrading Metadata schema");
}

void upgrade_200_210(const std::shared_ptr<Instance> &group_server) {
  DBUG_EXECUTE_IF("dba_FAIL_metadata_upgrade_at_UPGRADING", {
    throw std::logic_error(
        "Debug emulation of failed metadata upgrade "
        "UPGRADING.");
  });
  auto result = group_server->queryf("SELECT COUNT(*) FROM !.!",
                                     kMetadataSchemaName, "clusters");

  auto row = result->fetch_one();

  int count = row->get_int(0);
  if (count > 1) {
    throw std::runtime_error(
        shcore::str_format("Inconsistent Metadata: unexpected number of "
                           "registered clusters: %d",
                           count)
            .c_str());
  }

  execute_script(
      group_server,
      scripts::get_metadata_upgrade_script(mysqlshdk::utils::Version("2.1.0")),
      "While upgrading Metadata schema");
}

}  // namespace steps

struct Step {
  Version source_version;
  Version target_version;
  std::function<void(const std::shared_ptr<Instance> &)> function;
};

const Step k_steps[] = {
    {Version(1, 0, 1), Version(2, 0, 0), &steps::upgrade_101_200},
    {Version(2, 0, 0), Version(2, 1, 0), &steps::upgrade_200_210},
    {kNotInstalled, kNotInstalled, nullptr}};

bool get_lock(const std::shared_ptr<Instance> &instance) {
  auto result = instance->queryf("SELECT GET_LOCK(?,1)", kMetadataUpgradeLock);
  auto row = result->fetch_one();
  if (row) {
    if (!row->is_null(0)) {
      return row->get_int(0) == 1;
    }
  }
  return false;
}

/**
 * Gets a lock on all the instances of the cluster.
 *
 * @param group_server a session to the master instance of the cluster.
 */
Instance_list get_locks(const std::shared_ptr<Instance> &group_server) {
  auto ipool = current_ipool();

  Instance_list locked_instances;

  MetadataStorage metadata(group_server);

  auto instances_md = metadata.get_all_instances("");
  std::shared_ptr<mysqlsh::dba::Instance> instance;
  for (const auto &instance_md : instances_md) {
    try {
      instance = ipool->connect_unchecked_endpoint(instance_md.endpoint);
    } catch (const shcore::Exception &error) {
      log_warning("Unable to reach instance '%s' for metadata upgrade: %s",
                  instance_md.endpoint.c_str(), error.what());
      continue;
    }

    if (get_lock(instance)) {
      locked_instances.emplace_back(instance);
    } else {
      instance->release();
      throw shcore::Exception::runtime_error(shcore::str_format(
          "Unable to get lock '%s' on instance '%s'. Either a metadata "
          "upgrade or restore operation may be in progress.",
          kMetadataUpgradeLock, instance_md.endpoint.c_str()));
    }
  }

  return locked_instances;
}

void release_lock(const std::shared_ptr<Instance> &instance) {
  instance->executef("DO RELEASE_LOCK(?)", kMetadataUpgradeLock);
}

void release_locks(const Instance_list &instances) {
  for (const auto &instance : instances) {
    release_lock(instance);
  }
}

void drop_metadata_backups(const std::shared_ptr<Instance> &group_server,
                           bool both) {
  auto console = current_console();
  console->print_info("Removing metadata backup...");
  if (both) {
    log_debug("Dropping metadata backup: %s.", kMetadataSchemaPreviousName);
    group_server->executef("DROP SCHEMA IF EXISTS !",
                           kMetadataSchemaPreviousName);
  }
  log_debug("Dropping metadata backup: %s.", kMetadataSchemaBackupName);
  group_server->executef("DROP SCHEMA IF EXISTS !", kMetadataSchemaBackupName);
}
/**
 * Restore Process Steps
 * =====================
 * 1) Drop MD Schema
 * 2) Deploy MD Schema in upgrading mode (schema_version is 0.0.0)
 * 2.1) Set MD to Upgrading Version
 * 3) Restore Data From Backup
 * 4) Drop Step Backup
 * 5) Drop Backup
 * 5.1) Set MD to Original Version
 * 5.2) Remove temporary view with original version
 */

/**
 * The following table shows the combinations of persisted stage and
 * schema_version and to which recovery action they lead to.
 *
 * The stage name in the step column shows the value of the Stage enum that
 * is detected by detect_failed_stage(), while the stage column shows the stage
 * that's persisted in the backup schema.
 *
 * Every combination of stage and schema_version that can happen during
 * an upgrade or restore should be accounted for here, to ensure that we can
 * recover from a crash at any arbitrary point.
 *
 * Because a stage+version combination is listed for every step executed
 * for both upgrade and recovery and every stage+version combination has a
 * matching recovery procedure, we can be sure that we can recover to a sane
 * state (either rollback or roll-forward) for any crashes at any point.
 *
 * stage   | ver   | step
 * --------|-------|----------------------------
 * NULL    | 0.0.0 |  CLEANUP: (finish)
 * NULL    | 0.0.0 |      set_schema_version(latest)
 * NULL    | 2.0.0 |      break
 *
 * DONE    | 0.0.0 |  DONE: (finish)
 * DONE    | 0.0.0 |      drop backup
 * NULL    | 0.0.0 |      set_schema_version(latest)
 * NULL    | 2.0.0 |      break
 *
 * UPGRAD  | 0.0.0 |  UPGRADING: (rollback)
 * UPGRAD  | 0.0.0 |      drop metadata
 * UPGRAD  | NULL  |      restore backup
 * UPGRAD  | NULL  |      set_schema_version(original)
 * UPGRAD  | 1.0.1 |      drop backup
 * NULL    | 1.0.1 |      break
 *
 * S_U_VER | 0.0.0 |  SETTING_UPGRADE_VERSION: (rollback)
 * S_U_VER | 0.0.0 |      set_schema_version(original)
 * S_U_VER | 1.0.1 |      drop backup
 * NULL    | 1.0.1 |      break
 *
 * NULL'   | 1.0.1 |  NONE:
 * NULL'   | 1.0.1 |      drop backup
 * NULL    | 1.0.1 |      break
 *
 * NULL    | 2.0.0 |  OK: upgrade succeeded
 * NULL    | 1.0.1 |  OK: upgrade failed, but no changes were made
 *
 * Additional states for failures during recovery:
 *
 * UPGRAD  | NULL  | same as UPGRADING + 0.0.0
 *
 * UPGRAD  | 1.0.1 | same as UPGRADING + 0.0.0
 *
 * S_U_VER | 1.0.1 | same as NONE
 */
void cleanup(const std::shared_ptr<Instance> &group_server,
             upgrade::Stage stage) {
  auto console = current_console();

  switch (stage) {
    // Schema is either at the original or the latest version
    case Stage::OK:
      break;

    // Initial stages, where the upgrade didn't start yet.
    case Stage::NONE:
      drop_metadata_backups(group_server, true);
      break;

    case Stage::SETTING_UPGRADE_VERSION:
      set_schema_version(group_server,
                         get_version(group_server, kMetadataSchemaBackupName));
      drop_metadata_backups(group_server, true);
      break;

    // During upgrade
    case Stage::UPGRADING: {
      Version orig_version =
          get_version(group_server, kMetadataSchemaBackupName);

      console->print_info("Restoring metadata to version " +
                          orig_version.get_base() + "...");

      log_debug("Restoring metadata from backup: %s",
                kMetadataSchemaBackupName);

      backup(orig_version, kMetadataSchemaBackupName, kMetadataSchemaName,
             group_server, "restoring backup of the metadata", true);

      set_schema_version(group_server, orig_version);
      drop_metadata_backups(group_server, true);
      break;
    }

    // Clean up stages, after the upgrade itself completed
    case Stage::DONE:
      drop_metadata_backups(group_server, true);
      set_schema_version(group_server, Version(k_metadata_schema_version));
      break;

    case Stage::CLEANUP:
      set_schema_version(group_server, Version(k_metadata_schema_version));
      break;
  }
}
}  // namespace upgrade

Version current_version() {
  // To emulate upgrades to 1.0.1
  DBUG_EXECUTE_IF("dba_EMULATE_CURRENT_MD_1_0_1", { return Version(1, 0, 1); });

  return Version(k_metadata_schema_version);
}

Version installed_version(const std::shared_ptr<Instance> &group_server,
                          const std::string &schema_name) {
  Version ret_val = get_version(group_server, schema_name);

  // If not available we need to determine if it is because the MD schema does
  // not exist at all or it's just the schema_version view that does not exist
  if (ret_val == kNotInstalled && schema_exists(group_server, schema_name)) {
    throw std::runtime_error(shcore::str_format(
        "Error verifying metadata version, the metadata "
        "schema %s is inconsistent: the schema_version view is missing.",
        schema_name.c_str()));
  }

  return ret_val;
}

State check_installed_schema_version(
    const std::shared_ptr<Instance> &group_server,
    mysqlshdk::utils::Version *out_installed,
    mysqlshdk::utils::Version *out_real_md_version,
    std::string *out_version_schema) {
  mysqlshdk::utils::Version installed = kNotInstalled;
  mysqlshdk::utils::Version real_md_version = kNotInstalled;
  std::string version_schema = kMetadataSchemaName;
  State state = State::NONEXISTING;

  if (schema_exists(group_server, kMetadataSchemaName) ||
      schema_exists(group_server, kMetadataSchemaBackupName)) {
    upgrade::Stage stage;

    if (mysqlshdk::mysql::check_indicator_tag(*group_server,
                                              kClusterSetupIndicatorTag))
      state = State::FAILED_SETUP;
    else
      stage = upgrade::detect_failed_stage(group_server);

    if (state != State::FAILED_SETUP) {
      switch (stage) {
        case upgrade::Stage::OK:
        case upgrade::Stage::NONE: {
          auto current = current_version();
          installed = installed_version(group_server, version_schema);
          real_md_version = installed;

          if (installed == current) {
            state = State::EQUAL;
          } else {
            if (current.get_major() == installed.get_major()) {
              if (current.get_minor() == installed.get_minor()) {
                if (current.get_patch() < installed.get_patch()) {
                  state = State::PATCH_HIGHER;
                } else {
                  state = State::PATCH_LOWER;
                }
              } else {
                if (current.get_minor() < installed.get_minor()) {
                  state = State::MINOR_HIGHER;
                } else {
                  state = State::MINOR_LOWER;
                }
              }
            } else {
              if (current.get_major() < installed.get_major()) {
                state = State::MAJOR_HIGHER;
              } else {
                state = State::MAJOR_LOWER;
              }
            }
          }
          break;
        }
        case upgrade::Stage::SETTING_UPGRADE_VERSION:
        case upgrade::Stage::UPGRADING:
          version_schema = kMetadataSchemaBackupName;
          real_md_version = installed_version(group_server, version_schema);
          installed = kUpgradingVersion;
          if (upgrade::get_lock(group_server)) {
            upgrade::release_lock(group_server);
            state = State::FAILED_UPGRADE;
          } else {
            state = State::UPGRADING;
          }
          break;
        case upgrade::Stage::DONE:
        case upgrade::Stage::CLEANUP:
          installed = installed_version(group_server, version_schema);
          real_md_version = Version(k_metadata_schema_version);

          // Since the upgrade was not completed this is handled as a failed
          // upgrade
          state = State::FAILED_UPGRADE;
          break;
      }
    }
  }

  // Returns the installed version if requested
  if (out_installed) *out_installed = installed;
  if (out_real_md_version) *out_real_md_version = real_md_version;
  if (out_version_schema) *out_version_schema = version_schema;

  return state;
}

void install(const std::shared_ptr<Instance> &group_server) {
  try {
    execute_script(group_server,
                   scripts::get_metadata_script(
                       mysqlshdk::utils::Version(k_metadata_schema_version)),
                   "While installing metadata schema:");

    // The MD script sets the default DB to the MD schema
    // People who filter out the MD schema for replication purposes will filter
    // out everything that executes while the MD schema is the default.
    // Since we don't want certain operations that happen *after* this to also
    // be filtered out, we reset the default DB to something else.
    // Bug#30609075 is something caused by that.
    group_server->execute("USE mysql");
  } catch (...) {
    try {
      group_server->execute("USE mysql");
    } catch (...) {
    }
    throw;
  }
}

void uninstall(const std::shared_ptr<Instance> &group_server) {
  group_server->executef("DROP SCHEMA IF EXISTS !", kMetadataSchemaName);
}

std::vector<const upgrade::Step *> get_upgrade_path(
    const Version &start_version, const Version &final_version,
    const upgrade::Step *upgrade_functions) {
  std::vector<const upgrade::Step *> path;

  Version v = start_version;
  while (v != final_version) {
    bool found = false;
    for (const auto *function = upgrade_functions; function->function;
         ++function) {
      if (function->source_version == v) {
        path.push_back(function);
        v = function->target_version;
        found = true;
        break;
      }
    }
    if (!found) {
      log_info("Could not find upgrade path from version %s to %s",
               v.get_full().c_str(), final_version.get_full().c_str());
      return {};
    }
  }
  return path;
}

std::vector<const upgrade::Step *> get_upgrade_path(
    const Version &start_version) {
  return get_upgrade_path(start_version, current_version(), upgrade::k_steps);
}

/*
 * The following table shows the list of steps executed for the upgrade.
 * The stage and ver(sion) columns show the value of current stage and
 * schema_version. After a step is executed, the stage and version will change
 * and the new values are shown in the next row. If a crash occurs in any of
 * the steps, a recovery attempt can know the exact step that was being
 * executed from the combination of the persisted stage and version.
 *
 * stage   | ver   | step
 * --------|-------|----------------------------
 * NULL    | 1.0.1 | get_lock
 * NULL    | 1.0.1 | backup
 * NULL'   | 1.0.1 | stage SETTING_UPGRADE_VERSION
 * S_U_VER | 1.0.1 | set_schema_version(0.0.0)
 *
 * S_U_VER | 0.0.0 | stage UPGRADING
 * UPGRAD  | 0.0.0 | backup_prev
 * UPGRAD  | 0.0.0 | upgrade
 * UPGRAD  | 0.0.0 | drop backup_prev
 *
 * UPGRAD  | 0.0.0 | stage DONE
 * DONE    | 0.0.0 | drop backup
 * NULL    | 0.0.0 | set_schema_version(latest)
 *
 * NULL    | 2.0.0 | release_lock
 *
 * stage=NULL means the backup schema doesn't exist
 * stage=NULL' means the backup schema exists, but no stage is set
 *
 * The Stage enum value for NULL persisted stages is detected implicitly by
 * combining with schema_version.
 */
void do_upgrade_schema(const std::shared_ptr<Instance> &group_server,
                       bool dry_run) {
  Version installed_ver = installed_version(group_server, kMetadataSchemaName);
  Version last_ver = current_version();

  assert(installed_ver < last_ver);

  auto console = mysqlsh::current_console();

  std::string instance_data = group_server->descr();
  auto path = get_upgrade_path(installed_ver);
  if (!dry_run) {
    console->print_info("Upgrading metadata at '" + instance_data +
                        "' from version " + installed_ver.get_base() +
                        " to version " + last_ver.get_base() + ".");
  }

  // These checks for the existence of backup schemas are now
  // pre-conditions, they are verified before anything starts, this
  // guarantees that if a backup of the metadata exists, it is result of the
  // upgrade process and can be deleted without problems in case of failures
  // or during metadata restore
  if (schema_exists(group_server, kMetadataSchemaBackupName)) {
    throw std::runtime_error(
        shcore::str_format("Unable to create the general backup because a "
                           "schema named '%s' already exists.",
                           kMetadataSchemaBackupName));
  }

  if (schema_exists(group_server, kMetadataSchemaPreviousName)) {
    throw std::runtime_error(
        shcore::str_format("Unable to create the step backup because a "
                           "schema named '%s' already exists.",
                           kMetadataSchemaPreviousName));
  }

  size_t total = path.size();
  if (path.size() > 1) {
    console->print_info(shcore::str_format(
        "Upgrade %s require %zu steps", (dry_run ? "would" : "will"), total));
  }

  if (!dry_run) {
    mysqlshdk::utils::nullable<upgrade::Stage> stage;
    Version actual_version = installed_ver;

    std::vector<std::string> tables;
    Instance_list locked_instances;
    try {
      // Acquires the upgrade locks
      locked_instances = upgrade::get_locks(group_server);

      console->print_info("Creating backup of the metadata schema...");

      backup(installed_ver, kMetadataSchemaName, kMetadataSchemaBackupName,
             group_server, "creating backup of the metadata");

      DBUG_EXECUTE_IF("dba_FAIL_metadata_upgrade_at_BACKING_UP_METADATA", {
        throw std::logic_error(
            "Debug emulation of failed metadata upgrade "
            "BACKING_UP_METADATA.");
      });

      // Sets the upgrading version
      DBUG_EXECUTE_IF("dba_limit_lock_wait_timeout",
                      { group_server->execute("SET lock_wait_timeout=1"); });

      stage = upgrade::save_stage(group_server,
                                  upgrade::Stage::SETTING_UPGRADE_VERSION);
      actual_version = set_schema_version(group_server, kUpgradingVersion);

      stage = upgrade::save_stage(group_server, upgrade::Stage::UPGRADING);

      DBUG_EXECUTE_IF("dba_CRASH_metadata_upgrade_at_UPGRADING", {
        // On a CRASH the cleanup logic would not be executed letting the
        // metadata in UPGRADING stage, however the locks would be released
        upgrade::release_locks(locked_instances);
        return;
      });

      group_server->execute("SET FOREIGN_KEY_CHECKS=0");

      size_t count = 1;
      for (const auto &step : path) {
        console->print_info(shcore::str_format(
            "Step %zu of %zu: upgrading from %s to %s...", count++, total,
            step->source_version.get_base().c_str(),
            step->target_version.get_base().c_str()));

        // Inter step backup so upgrade can perform the required upgrade
        // operations
        backup(step->source_version, kMetadataSchemaName,
               kMetadataSchemaPreviousName, group_server,
               "creating step backup of the metadata");

        step->function(group_server);

        // Removing temporary step backup
        group_server->execute(std::string("DROP SCHEMA ") +
                              kMetadataSchemaPreviousName);
      }

      stage = upgrade::save_stage(group_server, upgrade::Stage::DONE);

      upgrade::drop_metadata_backups(group_server, false);

      actual_version = set_schema_version(group_server, last_ver);

      group_server->execute("SET FOREIGN_KEY_CHECKS=1");

      upgrade::release_locks(locked_instances);

      installed_ver = installed_version(group_server);
      console->print_info(
          shcore::str_format("Upgrade process successfully finished, metadata "
                             "schema is now on version %s",
                             installed_ver.get_base().c_str()));
    } catch (const std::exception &e) {
      console->print_error(e.what());

      // Only if the error happened after the upgrade started the data is
      // restored and the backup deleted
      upgrade::cleanup(
          group_server,
          upgrade::compute_failed_upgrade_stage(
              stage, actual_version,
              schema_exists(group_server, kMetadataSchemaBackupName)));

      // Finally enables foreign key checks
      group_server->execute("SET FOREIGN_KEY_CHECKS=1");

      upgrade::release_locks(locked_instances);

      // Get the version after the restore to notify the final version
      installed_ver = installed_version(group_server);

      if (stage == upgrade::Stage::UPGRADING) {
        throw shcore::Exception::runtime_error(
            "Upgrade process failed, metadata has been restored to version " +
            installed_ver.get_base() + ".");
      } else {
        throw shcore::Exception::runtime_error(
            "Upgrade process failed, metadata was not modified.");
      }
    }
  }
}

void upgrade_schema(const std::shared_ptr<Instance> &group_server,
                    bool dry_run) {
  try {
    do_upgrade_schema(group_server, dry_run);

    // See explanation for this in install()
    group_server->execute("USE mysql");
  } catch (...) {
    try {
      group_server->execute("USE mysql");
    } catch (...) {
    }
    throw;
  }
}

void upgrade_or_restore_schema(const std::shared_ptr<Instance> &group_server,
                               bool dry_run) {
  auto console = mysqlsh::current_console();

  upgrade::Stage stage = upgrade::detect_failed_stage(group_server);

  if (stage == upgrade::Stage::OK) {
    // If the MD is consistent, then we check if we need to upgrade it.
    if (installed_version(group_server) < current_version()) {
      upgrade_schema(group_server, dry_run);
    } else {
      console->print_info(
          "Metadata state is consistent and a restore is not necessary.");
    }
  } else {
    console->print_warning(
        "An unfinished metadata upgrade was detected, which may have left it "
        "in an invalid state.");

    {  // We need to what will be the final version in order print the right
       // message
      Version final_version;
      check_installed_schema_version(group_server, nullptr, &final_version,
                                     nullptr);

      std::string action = dry_run ? "can" : "will";
      if (stage == upgrade::Stage::CLEANUP || stage == upgrade::Stage::DONE) {
        console->print_info("The metadata upgrade to version " +
                            final_version.get_base() + " " + action +
                            " be completed.");
      } else {
        console->print_info("The metadata " + action +
                            " be restored to version " +
                            final_version.get_base() + ".");
      }
    }

    if (!dry_run) {
      Instance_list locked_instances = upgrade::get_locks(group_server);

      group_server->execute("SET FOREIGN_KEY_CHECKS=0");

      upgrade::cleanup(group_server, stage);

      group_server->execute("SET FOREIGN_KEY_CHECKS=1");

      upgrade::release_locks(locked_instances);
    }
  }
}

bool is_valid_version(const mysqlshdk::utils::Version &version) {
  DBUG_EXECUTE_IF("dba_EMULATE_UNEXISTING_MD", { return true; });

  return std::find_if(std::begin(k_metadata_schema_version_history),
                      std::end(k_metadata_schema_version_history),
                      [version](const char *v) -> bool {
                        return version.get_base().compare(v) == 0;
                      }) != std::end(k_metadata_schema_version_history) ||
         version == kUpgradingVersion;
}

}  // namespace metadata

void prepare_metadata_schema(const std::shared_ptr<Instance> &target_instance,
                             bool force_overwrite, bool dry_run) {
  // Ensure that the metadata schema is ready for creating a new cluster in it
  // If the metadata schema does not exist, we create it
  // If the metadata schema already exists:
  // - clear it of old data
  // - ensure it has the right version
  // We ensure both by always dropping the old schema and re-creating it from
  // scratch.

  mysqlshdk::utils::Version version;

  if (force_overwrite)
    version = metadata::kNotInstalled;
  else
    version = metadata::installed_version(target_instance);

  if (version != metadata::kNotInstalled) {
    current_console()->print_note("Metadata schema found in target instance");
  } else {
    if (!dry_run) {
      metadata::uninstall(target_instance);
    }

    log_info("Deploying metadata schema in %s%s...",
             target_instance->descr().c_str(), dry_run ? " (dryRun)" : "");
    if (!dry_run) {
      metadata::install(target_instance);
    }
  }
}

}  // namespace dba
}  // namespace mysqlsh
