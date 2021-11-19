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

#ifndef MODULES_ADMINAPI_COMMON_METADATA_MANAGEMENT_MYSQL_H_
#define MODULES_ADMINAPI_COMMON_METADATA_MANAGEMENT_MYSQL_H_

#include <memory>
#include <string>

#include "modules/adminapi/common/instance_pool.h"
#include "mysqlshdk/libs/utils/enumset.h"
#include "mysqlshdk/libs/utils/utils_string.h"
#include "mysqlshdk/libs/utils/version.h"

namespace mysqlsh {
namespace dba {

void prepare_metadata_schema(const std::shared_ptr<Instance> &target_instance,
                             bool dry_run);

namespace metadata {

namespace scripts {
std::string get_metadata_script(const mysqlshdk::utils::Version &version);

std::string get_metadata_upgrade_script();
}  // namespace scripts

namespace upgrade {
enum class Stage;
}
const mysqlshdk::utils::Version kUpgradingVersion =
    mysqlshdk::utils::Version(0, 0, 0);
const mysqlshdk::utils::Version kNotInstalled =
    mysqlshdk::utils::Version(-1, -1, -1);

constexpr char kMetadataSchemaName[] = "mysql_innodb_cluster_metadata";
constexpr char kMetadataSchemaBackupName[] =
    "mysql_innodb_cluster_metadata_bkp";

static constexpr const char *kClusterSetupIndicatorTag =
    "__mysql_innodb_cluster_creating_cluster__";

constexpr char kFailedUpgradeError[] =
    "An unfinished metadata upgrade was detected, which may have left it in an "
    "invalid state. Execute dba.<<<upgradeMetadata>>> again to repair it.";

enum State {
  EQUAL,
  MAJOR_HIGHER,
  MAJOR_LOWER,
  MINOR_HIGHER,
  MINOR_LOWER,
  PATCH_HIGHER,
  PATCH_LOWER,
  UPGRADING,
  FAILED_UPGRADE,
  FAILED_SETUP,
  NONEXISTING
};

using States = mysqlshdk::utils::Enum_set<State, State::NONEXISTING>;

const States kCompatible = States(State::EQUAL)
                               .set(State::MINOR_HIGHER)
                               .set(State::MINOR_LOWER)
                               .set(State::PATCH_HIGHER)
                               .set(State::PATCH_LOWER);
const States kCompatibleLower =
    States(State::MINOR_LOWER).set(State::PATCH_LOWER);
const States kIncompatible =
    States(State::MAJOR_HIGHER).set(State::MAJOR_LOWER);
const States kUpgradeStates =
    States(State::UPGRADING).set(State::FAILED_UPGRADE);
const States kUpgradeInProgress = States(State::UPGRADING);
const States kIncompatibleOrUpgrading = States(State::UPGRADING)
                                            .set(State::FAILED_UPGRADE)
                                            .set(State::MAJOR_HIGHER)
                                            .set(State::MAJOR_LOWER);

metadata::State check_installed_schema_version(
    const std::shared_ptr<Instance> &group_server,
    mysqlshdk::utils::Version *out_installed,
    mysqlshdk::utils::Version *out_real_md_version,
    std::string *out_version_schema);

// Metadata schema version supported by "this" version of the shell.
mysqlshdk::utils::Version current_version();

// Metadata schema version currently installed in target
mysqlshdk::utils::Version installed_version(
    const std::shared_ptr<Instance> &group_server,
    const std::string &schema_name = kMetadataSchemaName);

void install(const std::shared_ptr<Instance> &group_server);

void upgrade_or_restore_schema(const std::shared_ptr<Instance> &group_server,
                               bool dry_run);

void backup_schema(const std::shared_ptr<Instance> &group_server,
                   const mysqlshdk::utils::Version &version,
                   const char *name = nullptr);
void uninstall(const std::shared_ptr<Instance> &group_server);

bool is_valid_version(const mysqlshdk::utils::Version &version);
}  // namespace metadata
}  // namespace dba
}  // namespace mysqlsh

#endif  // MODULES_ADMINAPI_COMMON_METADATA_MANAGEMENT_MYSQL_H_
