/*
 * Copyright (c) 2021, Oracle and/or its affiliates.
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
#include "modules/adminapi/mod_dba_options.h"
#include "modules/adminapi/common/common.h"
#include "mysqlshdk/include/scripting/type_info/custom.h"
#include "mysqlshdk/include/scripting/type_info/generic.h"
#include "mysqlshdk/include/shellcore/console.h"
#include "mysqlshdk/libs/db/utils_connection.h"
#include "mysqlshdk/libs/utils/utils_file.h"
#include "shellcore/shell_options.h"

namespace mysqlsh {
namespace dba {

Common_sandbox_options::Common_sandbox_options() {
  sandbox_dir = mysqlsh::current_shell_options()->get().sandbox_directory;
}

const shcore::Option_pack_def<Common_sandbox_options>
    &Common_sandbox_options::options() {
  static const auto opts =
      shcore::Option_pack_def<Common_sandbox_options>().optional(
          kSandboxDir, &Common_sandbox_options::set_sandbox_dir);

  return opts;
}

void Common_sandbox_options::set_sandbox_dir(const std::string &value) {
  if (!shcore::is_folder(value))
    throw shcore::Exception::argument_error(
        "The sandbox dir path '" + value + "' is not valid: it " +
        (shcore::path_exists(value) ? "is not a directory" : "does not exist") +
        ".");

  sandbox_dir = value;
}

const shcore::Option_pack_def<Stop_sandbox_options>
    &Stop_sandbox_options::options() {
  static const auto opts =
      shcore::Option_pack_def<Stop_sandbox_options>()
          .include<Common_sandbox_options>()
          .optional(mysqlshdk::db::kPassword, &Stop_sandbox_options::password);

  return opts;
}

const shcore::Option_pack_def<Deploy_sandbox_options>
    &Deploy_sandbox_options::options() {
  static const auto opts =
      shcore::Option_pack_def<Deploy_sandbox_options>()
          .include<Stop_sandbox_options>()
          .optional(kPortX, &Deploy_sandbox_options::xport)
          .optional(kAllowRootFrom, &Deploy_sandbox_options::allow_root_from)
          .optional(kIgnoreSslError, &Deploy_sandbox_options::ignore_ssl_error)
          .optional(kMysqldOptions, &Deploy_sandbox_options::mysqld_options);

  return opts;
}

Check_instance_configuration_options::Check_instance_configuration_options() {
  interactive = current_shell_options()->get().wizards;
}

const shcore::Option_pack_def<Check_instance_configuration_options>
    &Check_instance_configuration_options::options() {
  static const auto opts =
      shcore::Option_pack_def<Check_instance_configuration_options>()
          .optional(kMyCnfPath,
                    &Check_instance_configuration_options::mycnf_path)
          .optional(kVerifyMyCnf,
                    &Check_instance_configuration_options::mycnf_path)
          .optional(kInteractive,
                    &Check_instance_configuration_options::interactive)
          .optional(mysqlshdk::db::kPassword,
                    &Check_instance_configuration_options::password, "",
                    shcore::Option_extract_mode::CASE_INSENSITIVE,
                    shcore::Option_scope::CLI_DISABLED);

  return opts;
}

const shcore::Option_pack_def<Configure_instance_options>
    &Configure_instance_options::options() {
  static const auto opts =
      shcore::Option_pack_def<Configure_instance_options>()
          .optional(kClusterAdmin, &Configure_instance_options::cluster_admin)
          .optional(kClusterAdminPassword,
                    &Configure_instance_options::cluster_admin_password)
          .optional(kRestart, &Configure_instance_options::restart)
          .optional(kInteractive, &Configure_instance_options::interactive)
          .optional(mysqlshdk::db::kPassword,
                    &Configure_instance_options::password, "",
                    shcore::Option_extract_mode::CASE_INSENSITIVE,
                    shcore::Option_scope::CLI_DISABLED);

  return opts;
}

void Configure_cluster_local_instance_options::set_mycnf_path(
    const std::string &value) {
  mycnf_path = value;
}
void Configure_cluster_local_instance_options::set_output_mycnf_path(
    const std::string &value) {
  output_mycnf_path = value;
}
void Configure_cluster_local_instance_options::set_clear_read_only(bool value) {
  clear_read_only = value;
}

Configure_cluster_local_instance_options::
    Configure_cluster_local_instance_options() {
  local = true;
  cluster_type = Cluster_type::GROUP_REPLICATION;
}

const shcore::Option_pack_def<Configure_cluster_local_instance_options>
    &Configure_cluster_local_instance_options::options() {
  static const auto opts =
      shcore::Option_pack_def<Configure_cluster_local_instance_options>()
          .include<Configure_instance_options>()
          .optional(kMyCnfPath,
                    &Configure_cluster_local_instance_options::set_mycnf_path)
          .optional(
              kOutputMycnfPath,
              &Configure_cluster_local_instance_options::set_output_mycnf_path)
          .optional(
              kClearReadOnly,
              &Configure_cluster_local_instance_options::set_clear_read_only);

  return opts;
}

Configure_cluster_instance_options::Configure_cluster_instance_options()
    : Configure_cluster_local_instance_options() {
  local = false;
}

void Configure_cluster_instance_options::set_slave_parallel_workers(
    int64_t value) {
  slave_parallel_workers = value;
}

const shcore::Option_pack_def<Configure_cluster_instance_options>
    &Configure_cluster_instance_options::options() {
  static const auto opts =
      shcore::Option_pack_def<Configure_cluster_instance_options>()
          .include<Configure_cluster_local_instance_options>()
          .optional(
              kApplierWorkerThreads,
              &Configure_cluster_instance_options::set_slave_parallel_workers);

  return opts;
}

Configure_replicaset_instance_options::Configure_replicaset_instance_options()
    : Configure_instance_options() {
  cluster_type = Cluster_type::ASYNC_REPLICATION;
  clear_read_only = true;
}

void Configure_replicaset_instance_options::set_slave_parallel_workers(
    int64_t value) {
  slave_parallel_workers = value;
}

const shcore::Option_pack_def<Configure_replicaset_instance_options>
    &Configure_replicaset_instance_options::options() {
  static const auto opts =
      shcore::Option_pack_def<Configure_replicaset_instance_options>()
          .include<Configure_instance_options>()
          .optional(kApplierWorkerThreads,
                    &Configure_replicaset_instance_options::
                        set_slave_parallel_workers);

  return opts;
}

const shcore::Option_pack_def<Create_cluster_options>
    &Create_cluster_options::options() {
  static const auto opts =
      shcore::Option_pack_def<Create_cluster_options>()
          .include(&Create_cluster_options::gr_options)
          .include(&Create_cluster_options::clone_options)
          .optional(kMultiPrimary, &Create_cluster_options::set_multi_primary)
          .optional(kMultiMaster, &Create_cluster_options::set_multi_primary)
          .optional(kForce, &Create_cluster_options::force)
          .optional(kAdoptFromGR, &Create_cluster_options::adopt_from_gr)
          .optional(kClearReadOnly,
                    &Create_cluster_options::set_clear_read_only)
          .optional(kInteractive, &Create_cluster_options::interactive);

  return opts;
}

void Create_cluster_options::set_multi_primary(const std::string &option,
                                               bool value) {
  if (option == kMultiMaster) {
    handle_deprecated_option(kMultiMaster, kMultiPrimary,
                             !multi_primary.is_null(), false);
  }

  multi_primary = value;
}

void Create_cluster_options::set_clear_read_only(bool value) {
  auto console = current_console();
  console->print_warning(
      shcore::str_format("The %s option is deprecated. The super_read_only "
                         "mode is now automatically cleared.",
                         kClearReadOnly));
  console->print_info();

  clear_read_only = value;
}

const shcore::Option_pack_def<Create_replicaset_options>
    &Create_replicaset_options::options() {
  static const auto opts =
      shcore::Option_pack_def<Create_replicaset_options>()
          .optional(kAdoptFromAR, &Create_replicaset_options::adopt)
          .optional(kDryRun, &Create_replicaset_options::dry_run)
          .optional(kInstanceLabel,
                    &Create_replicaset_options::set_instance_label)
          .optional(kGtidSetIsComplete,
                    &Create_replicaset_options::gtid_set_is_complete);

  return opts;
}

void Create_replicaset_options::set_instance_label(const std::string &value) {
  if (adopt && !value.empty()) {
    throw shcore::Exception::argument_error(shcore::str_format(
        "%s option not allowed when %s:true", kInstanceLabel, kAdoptFromAR));
  } else {
    instance_label = value;
  }
}

const shcore::Option_pack_def<Drop_metadata_schema_options>
    &Drop_metadata_schema_options::options() {
  static const auto opts =
      shcore::Option_pack_def<Drop_metadata_schema_options>()
          .optional(kForce, &Drop_metadata_schema_options::force)
          .optional(kClearReadOnly,
                    &Drop_metadata_schema_options::clear_read_only);

  return opts;
}

const shcore::Option_pack_def<Reboot_cluster_options>
    &Reboot_cluster_options::options() {
  static const auto opts =
      shcore::Option_pack_def<Reboot_cluster_options>()
          .optional(kRemoveInstances, &Reboot_cluster_options::remove_instances)
          .optional(kRejoinInstances, &Reboot_cluster_options::rejoin_instances)
          .optional(kClearReadOnly,
                    &Reboot_cluster_options::set_clear_read_only)
          .optional(mysqlshdk::db::kUser, &Reboot_cluster_options::set_user)
          .optional(mysqlshdk::db::kDbUser, &Reboot_cluster_options::set_user,
                    "", shcore::Option_extract_mode::CASE_INSENSITIVE,
                    shcore::Option_scope::CLI_DISABLED)
          .optional(mysqlshdk::db::kPassword,
                    &Reboot_cluster_options::set_password)
          .optional(mysqlshdk::db::kDbPassword,
                    &Reboot_cluster_options::set_password, "",
                    shcore::Option_extract_mode::CASE_INSENSITIVE,
                    shcore::Option_scope::CLI_DISABLED);

  return opts;
}

void Reboot_cluster_options::set_user(const std::string &option,
                                      const std::string &value) {
  if (option == mysqlshdk::db::kDbUser) {
    handle_deprecated_option(mysqlshdk::db::kDbUser, mysqlshdk::db::kUser,
                             !user.is_null(), true);
  }

  user = value;
}

void Reboot_cluster_options::set_password(const std::string &option,
                                          const std::string &value) {
  if (option == mysqlshdk::db::kDbPassword) {
    handle_deprecated_option(mysqlshdk::db::kDbPassword,
                             mysqlshdk::db::kPassword, !user.is_null(), true);
  }

  password = value;
}

void Reboot_cluster_options::set_clear_read_only(bool value) {
  auto console = current_console();
  console->print_warning(
      shcore::str_format("The %s option is deprecated. The super_read_only "
                         "mode is now automatically cleared.",
                         kClearReadOnly));
  console->print_info();

  clear_read_only = value;
}

Upgrade_metadata_options::Upgrade_metadata_options() {
  interactive = current_shell_options()->get().wizards;
}

const shcore::Option_pack_def<Upgrade_metadata_options>
    &Upgrade_metadata_options::options() {
  static const auto opts =
      shcore::Option_pack_def<Upgrade_metadata_options>()
          .optional(kInteractive, &Upgrade_metadata_options::interactive)
          .optional(kDryRun, &Upgrade_metadata_options::dry_run);

  return opts;
}

}  // namespace dba
}  // namespace mysqlsh
