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
#include "mysqlshdk/include/shellcore/console.h"
#include "mysqlshdk/libs/db/mysql/session.h"
#include "mysqlshdk/libs/innodbcluster/cluster_metadata.h"
#include "mysqlshdk/libs/mysql/script.h"
#include "mysqlshdk/libs/utils/logger.h"

namespace mysqlsh {
namespace dba {
namespace metadata {

using mysqlshdk::utils::Version;

namespace {
struct Metadata_upgrade_script {
  Version source_version;
  Version target_version;
  const char *script;
};

#include "modules/adminapi/common/metadata-model_definitions.h"
}  // namespace

Version current_version() {
  return Version(k_metadata_schema_internal_version);
}

Version current_public_version() { return Version(k_metadata_schema_version); }

Version installed_version(std::shared_ptr<mysqlshdk::db::ISession> session) {
  try {
    auto result = session->query(
        "SELECT `major`, `minor`, `patch` FROM "
        "       mysql_innodb_cluster_metadata.schema_version");
    auto row = result->fetch_one_or_throw();

    return Version(row->get_int(0), row->get_int(1), row->get_int(2));
  } catch (mysqlshdk::db::Error &error) {
    if (error.code() != ER_NO_SUCH_TABLE && error.code() != ER_BAD_DB_ERROR)
      throw;
    return Version();
  }
}

void install(std::shared_ptr<mysqlshdk::db::ISession> session) {
  auto console = mysqlsh::current_console();
  bool first_error = true;

  mysqlshdk::mysql::execute_sql_script(
      session, k_metadata_schema_script,
      [console, &first_error](const std::string &err) {
        if (first_error) {
          console->print_error("While installing metadata schema:");
          first_error = false;
        }
        console->print_error(err);
      });
}

void uninstall(std::shared_ptr<mysqlshdk::db::ISession> session) {
  session->execute("DROP SCHEMA IF EXISTS mysql_innodb_cluster_metadata");
}

std::vector<const Metadata_upgrade_script *> get_upgrade_path(
    const Version &start_version, const Version &final_version,
    const Metadata_upgrade_script *upgrade_scripts) {
  std::vector<const Metadata_upgrade_script *> path;

  Version v = start_version;
  while (v != final_version) {
    bool found = false;
    for (const auto *script = upgrade_scripts; script->script; ++script) {
      if (script->source_version == v) {
        path.push_back(script);
        v = script->target_version;
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

std::vector<const Metadata_upgrade_script *> get_upgrade_path(
    const Version &start_version) {
  return get_upgrade_path(start_version, current_version(),
                          k_metadata_upgrade_scripts);
}

void backup_metadata(std::shared_ptr<mysqlshdk::db::ISession>) {}

void upgrade(std::shared_ptr<mysqlshdk::db::ISession> session) {
  Version version = installed_version(session);
  Version current = current_version();
  auto path = get_upgrade_path(version);
  auto console = mysqlsh::current_console();

  if (version >= current) {
    log_debug(
        "Installed metadata version at %s is equal or newer than latest known "
        "(installed %s, shell %s)",
        session->get_connection_options().as_uri().c_str(),
        version.get_full().c_str(), current.get_full().c_str());
    return;
  }

  log_info(
      "Upgrading InnoDB cluster metadata schema from version %s to %s at %s",
      version.get_full().c_str(), current.get_full().c_str(),
      session->get_connection_options().as_uri().c_str());
  log_info("Upgrade will require %zu steps", path.size());

  // log_info("Backing up current version of metadata schema to
  // mysql_innodb_cluster_md_old...");
  // backup_metadata(session);

  for (const Metadata_upgrade_script *upgrade : get_upgrade_path(version)) {
    console->print_info(
        shcore::str_format("Upgrading from %s to %s...",
                           upgrade->source_version.get_full().c_str(),
                           upgrade->target_version.get_full().c_str()));
    try {
      bool first_error = true;

      mysqlshdk::mysql::execute_sql_script(
          session, upgrade->script,
          [console, &first_error](const std::string &err) {
            if (first_error) {
              console->print_error("While upgrading metadata schema:");
              first_error = false;
            }
            console->print_error(err);
          });

      log_info("Succeeded");
    } catch (std::exception &e) {
      log_error("Error executing metadata schema upgrade script: %s", e.what());
      log_info("Attempting to restore original schema version from backup...");
    }
  }

  // Get the actually installed version after upgrade
  version = installed_version(session);
  console->print_info(
      shcore::str_format("Upgrade process successfully finished, cluster "
                         "metadata schema is now "
                         "on version %s",
                         version.get_full().c_str()));
}

}  // namespace metadata
}  // namespace dba
}  // namespace mysqlsh
