/*
 * Copyright (c) 2017, 2019, Oracle and/or its affiliates. All rights reserved.
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

#include <memory>
#include <string>
#include <utility>
#include <vector>
#include "modules/adminapi/common/instance_pool.h"
#include "modules/adminapi/common/metadata-model_definitions.h"
#include "modules/adminapi/common/metadata_storage.h"
#include "mysqlshdk/include/shellcore/console.h"
#include "mysqlshdk/libs/db/mysql/session.h"
#include "mysqlshdk/libs/mysql/script.h"
#include "mysqlshdk/libs/mysql/utils.h"
#include "mysqlshdk/libs/utils/debug.h"
#include "mysqlshdk/libs/utils/logger.h"

#if defined(__APPLE__)
#define FALLTHROUGH
#elif __has_cpp_attribute(fallthrough)
#define FALLTHROUGH [[fallthrough]]
#elif __has_cpp_attribute(clang::fallthrough)
#define FALLTHROUGH [[clang::fallthrough]]
#elif __has_cpp_attribute(gnu::fallthrough)
#define FALLTHROUGH [[gnu::fallthrough]]
#else
#define FALLTHROUGH
#endif

using mysqlshdk::utils::Version;

namespace mysqlsh {
namespace dba {
namespace metadata {
namespace {
constexpr char kMetadataSchemaBackupName[] =
    "mysql_innodb_cluster_metadata_bkp";
constexpr char kMetadataUpgradeLock[] =
    "mysql_innodb_cluster_metadata.upgrade_in_progress";

Version get_version(const std::shared_ptr<Instance> &group_server,
                    const std::string &schema = kMetadataSchemaName) {
  try {
    auto result = group_server->queryf(
        "SELECT `major`, `minor`, `patch` FROM !.schema_version",
        schema.c_str());
    auto row = result->fetch_one_or_throw();

    return Version(row->get_int(0), row->get_int(1), row->get_int(2));
  } catch (const mysqlshdk::db::Error &error) {
    if (error.code() != ER_NO_SUCH_TABLE && error.code() != ER_BAD_DB_ERROR)
      throw;
  }

  return kNotInstalled;
}

using Instance_list = std::list<Scoped_instance>;

void set_schema_version(const std::shared_ptr<Instance> &group_server,
                        Version version) {
  try {
    group_server->executef(
        "ALTER VIEW !.schema_version (major, minor, patch) AS SELECT ?, ?, ?",
        kMetadataSchemaName, version.get_major(), version.get_minor(),
        version.get_patch());
  } catch (const mysqlshdk::db::Error &error) {
    if (error.code() != ER_NO_SUCH_TABLE && error.code() != ER_BAD_DB_ERROR)
      throw;

    group_server->executef(
        "CREATE VIEW !.schema_version (major, minor, patch) AS SELECT ?, ?, ?",
        kMetadataSchemaName, version.get_major(), version.get_minor(),
        version.get_patch());
  }
}

bool schema_exists(const std::shared_ptr<Instance> &group_server,
                   const std::string &name) {
  auto result = group_server->queryf("SHOW DATABASES LIKE ?", name.c_str());

  auto row = result->fetch_one();

  // If the schema exists but not the table we are on version 1.0.0
  return row ? true : false;
}

}  // namespace

namespace upgrade {
namespace steps {
/**
 * This function is for testing purposes, it would upgrade a metadata from
 * version 1.0.0 (which does not exist) to version 1.0.1 (which is the very
 * first version available). The only thing it does is create the
 * schema_version view with 1.0.1
 */
void upgrade_100_101(const std::shared_ptr<Instance> &group_server) {
  DBUG_EXECUTE_IF("dba_FAIL_metadata_upgrade_at_UPGRADING", {
    throw std::logic_error(
        "Debug emulation of failed metadata upgrade "
        "UPGRADING.");
  });
  mysqlsh::dba::metadata::set_schema_version(group_server, Version(1, 0, 1));
}

}  // namespace steps

enum class State {
  INITIAL,
  BACKING_UP_METADATA,
  GETTING_LOCKS,
  SETTING_UPGRADE_VERSION,
  UPGRADING,
  DONE,
  FAILED
};

struct Step {
  Version source_version;
  Version target_version;
  std::function<void(const std::shared_ptr<Instance> &)> function;
};

const Step k_steps[] = {
    {Version(1, 0, 0), Version(1, 0, 1), &steps::upgrade_100_101},
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
 * @param relaxed boolean to enable/disable failing if the instances table does
 *                not exist.
 *
 * The relaxed parameter is required to force the instances to be retrieved
 * from the metadata or not, i.e. when doing an upgrade it is required that they
 * are pulled from there. Doing a restore might not be the case as we don't know
 * the state of the instances table.
 */
Instance_list get_locks(const std::shared_ptr<Instance> &group_server,
                        const std::string &schema) {
  auto ipool = current_ipool();

  Instance_list locked_instances;

  MetadataStorage metadata(group_server);

  auto instances_md = metadata.get_all_instances(0, schema.c_str());
  std::shared_ptr<mysqlsh::dba::Instance> instance;
  for (const auto &instance_md : instances_md) {
    try {
      instance = ipool->connect_unchecked_endpoint(instance_md.endpoint);
    } catch (const shcore::Exception &error) {
      log_warning("Unable to reach instance '%s' for Metadata Upgrade: %s",
                  instance_md.endpoint.c_str(), error.what());
      continue;
    }

    if (get_lock(instance)) {
      locked_instances.emplace_back(instance);
    } else {
      instance->release();
      throw shcore::Exception::runtime_error(shcore::str_format(
          "Unable to get lock '%s' on instance '%s'. Either a metadata "
          "upgrade "
          "or restore operation may be in progress.",
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

void cleanup(const std::shared_ptr<Instance> &group_server,
             const Instance_list &locked_instances,
             const Version &original_version, upgrade::State state) {
  auto console = current_console();

  // Fallthrough is used on this switch because the cleanup operations get
  // incrementing as the upgrade process progresses, depending on the final
  // state is the required cleanup actions
  switch (state) {
    // Restoring the metadata is required whenever a failure occurred. UPGRADING
    // state indicates a fatal error ocurred during the upgrade
    case State::FAILED:
    case State::UPGRADING:
      console->print_info("Restoring metadata to version " +
                          original_version.get_base() + "...");

      mysqlshdk::mysql::copy_schema(*group_server, kMetadataSchemaBackupName,
                                    kMetadataSchemaName, true, false);
      FALLTHROUGH;
      // Removing the metadata backup is whenever the backup schema created
    case State::SETTING_UPGRADE_VERSION:
    case State::DONE:
    case State::BACKING_UP_METADATA:
      console->print_info("Removing metadata backup...");
      group_server->executef("DROP SCHEMA IF EXISTS !",
                             kMetadataSchemaBackupName);
      FALLTHROUGH;
      // Locks need to be released to tell other shell instances that the
      // upgrade/restore process is done
    case State::GETTING_LOCKS:
      release_locks(locked_instances);
      break;
    case State::INITIAL:
      break;
  }

  // Finally enables foreign key checks
  group_server->execute("SET FOREIGN_KEY_CHECKS=1");
}
}  // namespace upgrade

Version current_version() { return Version(k_metadata_schema_version); }

Version installed_version(const std::shared_ptr<Instance> &group_server,
                          const std::string &schema_name) {
  Version ret_val = get_version(group_server, schema_name);

  // If not available we need to determine if it is because the MD schema does
  // not exist at all or it's just the schema_version view that does not exist
  if (ret_val == kNotInstalled && schema_exists(group_server, schema_name))
    ret_val = Version(1, 0, 0);

  return ret_val;
}

State version_compatibility(const std::shared_ptr<Instance> &group_server,
                            mysqlshdk::utils::Version *out_installed) {
  State state = State::NONEXISTING;

  try {
    auto installed = installed_version(group_server);

    // Returns the installed version if requested
    if (out_installed) *out_installed = installed;

    if (installed != kNotInstalled) {
      auto current = current_version();

      if (installed == kUpgradingVersion) {
        state = State::FAILED_UPGRADE;

        // Verifies if the target server is locked to determine if an upgrade is
        // in progress
        if (upgrade::get_lock(group_server)) {
          upgrade::release_lock(group_server);
        } else {
          state = State::UPGRADING;
        }
      } else if (installed == current) {
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
    }
  } catch (const mysqlshdk::db::Error &error) {
    throw std::runtime_error("Error verifying metadata version: " +
                             error.format());
  }

  return state;
}

void install(const std::shared_ptr<Instance> &group_server) {
  auto console = mysqlsh::current_console();
  bool first_error = true;

  mysqlshdk::mysql::execute_sql_script(
      *group_server, k_metadata_schema_script,
      [console, &first_error](const std::string &err) {
        if (first_error) {
          console->print_error("While installing metadata schema:");
          first_error = false;
        }
        console->print_error(err);
      });
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

void upgrade_schema(const std::shared_ptr<Instance> &group_server,
                    bool dry_run) {
  Version installed = installed_version(group_server, kMetadataSchemaName);

  // Calling the function while upgrade is in progress is forbidden by the
  // metadata preconditions, however, they bypass the case of a failed upgrade
  // as the function could be called to do a restore.
  // This scenario covers the case where the precondition was bypassed but not
  // for a restore operation
  if (installed == kUpgradingVersion) {
    throw std::runtime_error(shcore::str_subvars(
        kFailedUpgradeError,
        [](const std::string &var) {
          return shcore::get_member_name(var, shcore::current_naming_style());
        },
        "<<<", ">>>"));
  }

  Version current = current_version();
  auto console = mysqlsh::current_console();

  std::string instance_data = group_server->descr();
  if (installed == current) {
    console->print_note("Installed metadata at '" + instance_data +
                        "' is up to date (version " + installed.get_base() +
                        ").");
  } else if (installed > current) {
    // This validation is just for safety, the metadata precondition checks
    // already handle this.
    throw std::runtime_error("Installed metadata at '" + instance_data +
                             "' is newer than the version version supported by "
                             "this Shell (installed: " +
                             installed.get_base() +
                             ", shell: " + current.get_base() + ").");
  } else {
    auto path = get_upgrade_path(installed);
    if (dry_run) {
      console->print_info("Metadata at '" + instance_data + "' is at version " +
                          installed.get_base() +
                          ", for this reason some AdminAPI functionality may "
                          "be restricted. To fully enable the AdminAPI the "
                          "metadata is required to be upgraded to version " +
                          current.get_base() + ".");
    } else {
      console->print_info("Upgrading metadata at '" + instance_data +
                          "' from version " + installed.get_base() +
                          " to version " + current.get_base() + ".");
    }

    size_t total = path.size();
    if (path.size() > 1) {
      console->print_info(shcore::str_format(
          "Upgrade %s require %zu steps", (dry_run ? "would" : "will"), total));
    }

    if (!dry_run) {
      upgrade::State state = upgrade::State::INITIAL;
      std::vector<std::string> tables;
      Instance_list locked_instances;
      try {
        // Acquires the upgrade locks
        state = upgrade::State::GETTING_LOCKS;
        locked_instances =
            upgrade::get_locks(group_server, kMetadataSchemaName);

        console->print_info("Creating backup of the metadata schema...");

        state = upgrade::State::BACKING_UP_METADATA;
        if (!backup_schema(group_server, kMetadataSchemaBackupName)) {
          // Handles this case so the cleanup does not remove the metadata
          // schema backup if it already existed
          state = upgrade::State::GETTING_LOCKS;
          throw std::runtime_error(
              "Can't create database 'mysql_innodb_cluster_metadata_bkp'; "
              "database exists.");
        }

        DBUG_EXECUTE_IF("dba_FAIL_metadata_upgrade_at_BACKING_UP_METADATA", {
          throw std::logic_error(
              "Debug emulation of failed metadata upgrade "
              "BACKING_UP_METADATA.");
        });

        // Sets the upgrading version
        DBUG_EXECUTE_IF("dba_limit_lock_wait_timeout",
                        { group_server->execute("SET lock_wait_timeout=1"); });

        state = upgrade::State::SETTING_UPGRADE_VERSION;
        set_schema_version(group_server, kUpgradingVersion);

        state = upgrade::State::UPGRADING;

        DBUG_EXECUTE_IF("dba_CRASH_metadata_upgrade_at_UPGRADING", {
          // On a CRASH the cleanup logic would not be executed letting the
          // metadata in UPGRADING state, however the locks would be released
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

          step->function(group_server);
        }

        state = upgrade::State::DONE;

        upgrade::cleanup(group_server, locked_instances, installed, state);

        installed = installed_version(group_server);
        console->print_info(shcore::str_format(
            "Upgrade process successfully finished, metadata "
            "schema is now on version %s",
            installed.get_base().c_str()));
      } catch (const std::exception &e) {
        console->print_error(e.what());

        // Only if the error happened after the upgrade started the data is
        // restored and the backup deleted
        upgrade::cleanup(group_server, locked_instances, installed, state);

        // Get the version after the restore to notify the final version
        installed = installed_version(group_server);

        if (state == upgrade::State::UPGRADING) {
          throw shcore::Exception::runtime_error(
              "Upgrade process failed, metadata has been restored to version " +
              installed.get_base() + ".");
        } else {
          throw shcore::Exception::runtime_error(
              "Upgrade process failed, metadata was not modified.");
        }
      }
    }
  }
}

/**
 * Backups the metadata schema, returns FALSE if it failed creating
 * the backup schema
 */
bool backup_schema(const std::shared_ptr<Instance> &group_server,
                   const std::string &target_schema) {
  try {
    mysqlshdk::mysql::copy_schema(*group_server, kMetadataSchemaName,
                                  target_schema, false, false);

  } catch (const mysqlshdk::db::Error &err) {
    if (err.code() == ER_DB_CREATE_EXISTS)
      return false;
    else
      throw;
  }

  return true;
}

void restore_schema(const std::shared_ptr<Instance> &group_server,
                    bool dry_run) {
  auto console = mysqlsh::current_console();

  Version version = installed_version(group_server, kMetadataSchemaBackupName);

  if (version != kNotInstalled) {
    if (dry_run) {
      console->print_info("The metadata can be restored to version " +
                          version.get_base());
    } else {
      Instance_list locked_instances =
          upgrade::get_locks(group_server, kMetadataSchemaBackupName);

      upgrade::cleanup(group_server, locked_instances, version,
                       upgrade::State::FAILED);
    }
  } else {
    throw std::runtime_error(
        "The metadata can not be restored, no backup us available.");
  }
}

}  // namespace metadata
}  // namespace dba
}  // namespace mysqlsh
