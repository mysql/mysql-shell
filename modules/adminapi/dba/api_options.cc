/*
 * Copyright (c) 2021, 2024, Oracle and/or its affiliates.
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

#include "modules/adminapi/dba/api_options.h"

#include <array>
#include <string_view>

#include "modules/adminapi/common/common.h"
#include "modules/adminapi/common/server_features.h"
#include "mysqlshdk/libs/db/utils_connection.h"
#include "mysqlshdk/libs/utils/utils_file.h"
#include "shellcore/shell_options.h"

namespace mysqlsh {
namespace dba {

namespace {
constexpr std::array<std::string_view, 5> kReplicationMemberAuthValues = {
    kReplicationMemberAuthPassword, kReplicationMemberAuthCertIssuer,
    kReplicationMemberAuthCertSubject, kReplicationMemberAuthCertIssuerPassword,
    kReplicationMemberAuthCertSubjectPassword};
}  // namespace

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

const shcore::Option_pack_def<Check_instance_configuration_options>
    &Check_instance_configuration_options::options() {
  static const auto opts =
      shcore::Option_pack_def<Check_instance_configuration_options>()
          .optional(kMyCnfPath,
                    &Check_instance_configuration_options::mycnf_path)
          .optional(kVerifyMyCnf,
                    &Check_instance_configuration_options::mycnf_path);

  return opts;
}

const shcore::Option_pack_def<Configure_instance_options>
    &Configure_instance_options::options() {
  static const auto opts =
      shcore::Option_pack_def<Configure_instance_options>()
          .optional(kClusterAdmin, &Configure_instance_options::cluster_admin)
          .optional(kClusterAdminPassword,
                    &Configure_instance_options::cluster_admin_password)
          .optional(kClusterAdminCertIssuer,
                    &Configure_instance_options::cluster_admin_cert_issuer)
          .optional(kClusterAdminCertSubject,
                    &Configure_instance_options::cluster_admin_cert_subject)
          .optional(kClusterAdminPasswordExpiration,
                    &Configure_instance_options::set_password_expiration)
          .optional(kRestart, &Configure_instance_options::restart);

  return opts;
}

void Configure_instance_options::set_password_expiration(
    const shcore::Value &value) {
  switch (value.get_type()) {
    case shcore::Null:
      cluster_admin_password_expiration.reset();
      break;

    case shcore::String: {
      const auto &s = value.get_string();
      if (shcore::str_caseeq(s, "NEVER")) {
        cluster_admin_password_expiration = -1;
        break;
      } else if (shcore::str_caseeq(s, "DEFAULT") || s.empty()) {
        cluster_admin_password_expiration.reset();
        break;
      }
      [[fallthrough]];
    }

    case shcore::Integer:
    case shcore::UInteger:
      try {
        cluster_admin_password_expiration = value.as_uint();
        if (*cluster_admin_password_expiration > 0) break;
      } catch (...) {
      }
      throw shcore::Exception::value_error(
          std::string("Option '") + kPasswordExpiration +
          "' UInteger, 'NEVER' or 'DEFAULT' expected, but value is '" +
          value.as_string() + "'");
      break;

    default:
      throw shcore::Exception::type_error(
          std::string("Option '") + kPasswordExpiration +
          "' UInteger, 'NEVER' or 'DEFAULT' expected, but value is " +
          type_name(value.get_type()));
  }
}

void Configure_cluster_instance_options::set_replica_parallel_workers(
    int64_t value) {
  if (value < 0)
    throw shcore::Exception::argument_error(shcore::str_format(
        "Invalid value for '%s' option: it only accepts positive integers.",
        kApplierWorkerThreads));

  replica_parallel_workers = value;
}

void Configure_cluster_instance_options::set_mycnf_path(
    const std::string &value) {
  mycnf_path = value;
}

void Configure_cluster_instance_options::set_output_mycnf_path(
    const std::string &value) {
  output_mycnf_path = value;
}

const shcore::Option_pack_def<Configure_cluster_instance_options>
    &Configure_cluster_instance_options::options() {
  static const auto opts =
      shcore::Option_pack_def<Configure_cluster_instance_options>()
          .include<Configure_instance_options>()
          .optional(kMyCnfPath,
                    &Configure_cluster_instance_options::set_mycnf_path)
          .optional(kOutputMycnfPath,
                    &Configure_cluster_instance_options::set_output_mycnf_path)
          .optional(kApplierWorkerThreads, &Configure_cluster_instance_options::
                                               set_replica_parallel_workers);

  return opts;
}

void Configure_replicaset_instance_options::set_replica_parallel_workers(
    int64_t value) {
  if (value < 0)
    throw shcore::Exception::argument_error(shcore::str_format(
        "Invalid value for '%s' option: it only accepts positive integers.",
        kApplierWorkerThreads));

  replica_parallel_workers = value;
}

const shcore::Option_pack_def<Configure_replicaset_instance_options>
    &Configure_replicaset_instance_options::options() {
  static const auto opts =
      shcore::Option_pack_def<Configure_replicaset_instance_options>()
          .include<Configure_instance_options>()
          .optional(kApplierWorkerThreads,
                    &Configure_replicaset_instance_options::
                        set_replica_parallel_workers);

  return opts;
}

const shcore::Option_pack_def<Replication_auth_options>
    &Replication_auth_options::options() {
  static const auto opts =
      shcore::Option_pack_def<Replication_auth_options>()
          .optional(kMemberAuthType, &Replication_auth_options::set_auth_type)
          .optional(kCertIssuer, &Replication_auth_options::set_cert_issuer)
          .optional(kCertSubject, &Replication_auth_options::set_cert_subject);

  return opts;
}

void Replication_auth_options::set_auth_type(const std::string &value) {
  if (std::find(kReplicationMemberAuthValues.begin(),
                kReplicationMemberAuthValues.end(), shcore::str_upper(value)) ==
      kReplicationMemberAuthValues.end()) {
    std::string valid_values =
        shcore::str_join(kReplicationMemberAuthValues, ", ");
    throw shcore::Exception::argument_error(shcore::str_format(
        "Invalid value for '%s' option. Supported values: %s.", kMemberAuthType,
        valid_values.c_str()));
  }

  member_auth_type = to_replication_auth_type(value);
}

void Replication_auth_options::set_cert_issuer(const std::string &value) {
  if (value.empty())
    throw shcore::Exception::argument_error(shcore::str_format(
        "Invalid value for '%s' option. Value cannot be an empty string.",
        kCertIssuer));

  cert_issuer = value;
}
void Replication_auth_options::set_cert_subject(const std::string &value) {
  if (value.empty())
    throw shcore::Exception::argument_error(shcore::str_format(
        "Invalid value for '%s' option. Value cannot be an empty string.",
        kCertSubject));

  cert_subject = value;
}

const shcore::Option_pack_def<Create_cluster_options>
    &Create_cluster_options::options() {
  static const auto opts =
      shcore::Option_pack_def<Create_cluster_options>()
          .include(&Create_cluster_options::gr_options)
          .include(&Create_cluster_options::clone_options)
          .include(&Create_cluster_options::member_auth_options)
          .optional(kMultiPrimary, &Create_cluster_options::multi_primary)
          .optional(kAdoptFromGR, &Create_cluster_options::adopt_from_gr)
          .optional(kReplicationAllowedHost,
                    &Create_cluster_options::replication_allowed_host)
          .include<Force_options>();
  ;

  return opts;
}

const shcore::Option_pack_def<Create_replicaset_options>
    &Create_replicaset_options::options() {
  static const auto opts =
      shcore::Option_pack_def<Create_replicaset_options>()
          .optional(kAdoptFromAR, &Create_replicaset_options::adopt)
          .optional(kDryRun, &Create_replicaset_options::dry_run)
          .optional(kInstanceLabel,
                    &Create_replicaset_options::set_instance_label)

          .optional(kReplicationSslMode,
                    &Create_replicaset_options::set_ssl_mode)

          .optional(kGtidSetIsComplete,
                    &Create_replicaset_options::gtid_set_is_complete)
          .optional(kReplicationAllowedHost,
                    &Create_replicaset_options::replication_allowed_host)
          .include(&Create_replicaset_options::member_auth_options);

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

void Create_replicaset_options::set_ssl_mode(const std::string &value) {
  if (std::find(kClusterSSLModeValues.begin(), kClusterSSLModeValues.end(),
                shcore::str_upper(value)) == kClusterSSLModeValues.end()) {
    auto valid_values = shcore::str_join(kClusterSSLModeValues, ", ");
    throw shcore::Exception::argument_error(shcore::str_format(
        "Invalid value for '%s' option. Supported values: %s.",
        kReplicationSslMode, valid_values.c_str()));
  }

  ssl_mode = to_cluster_ssl_mode(value);
}

const shcore::Option_pack_def<Drop_metadata_schema_options>
    &Drop_metadata_schema_options::options() {
  static const auto opts =
      shcore::Option_pack_def<Drop_metadata_schema_options>().optional(
          kForce, &Drop_metadata_schema_options::force);

  return opts;
}

const shcore::Option_pack_def<Reboot_cluster_options>
    &Reboot_cluster_options::options() {
  static const auto opts =
      shcore::Option_pack_def<Reboot_cluster_options>()
          .optional(kForce, &Reboot_cluster_options::force)
          .optional(kDryRun, &Reboot_cluster_options::dry_run)
          .optional("primary", &Reboot_cluster_options::set_primary)
          .optional(kSwitchCommunicationStack,
                    &Reboot_cluster_options::set_switch_communication_stack, "",
                    shcore::Option_extract_mode::CASE_INSENSITIVE)
          .optional(kTimeout, &Reboot_cluster_options::set_timeout, "",
                    shcore::Option_extract_mode::CASE_INSENSITIVE)
          .include(&Reboot_cluster_options::gr_options);

  return opts;
}

void Reboot_cluster_options::check_option_values(
    const mysqlshdk::utils::Version &version, int canonical_port,
    const std::string &comm_stack) {
  // The switchCommunicationStack option must be allowed only if the target
  // server version is >= 8.0.27
  if (switch_communication_stack &&
      !supports_mysql_communication_stack(version)) {
    throw shcore::Exception::runtime_error(shcore::str_format(
        "Option '%s' not supported on target server version: '%s'",
        kSwitchCommunicationStack, version.get_full().c_str()));
  }

  // Using switchCommunicationStack set to 'mysql' and ipAllowlist at the same
  // time is forbidden
  if (switch_communication_stack.value_or("") == kCommunicationStackMySQL &&
      gr_options.ip_allowlist.has_value()) {
    throw shcore::Exception::argument_error(shcore::str_format(
        "Cannot use '%s' when setting the '%s' option to '%s'", kIpAllowlist,
        kSwitchCommunicationStack, kCommunicationStackMySQL));
  }

  if (!switch_communication_stack && comm_stack == kCommunicationStackMySQL &&
      gr_options.ip_allowlist.has_value()) {
    throw shcore::Exception::argument_error(
        shcore::str_format("Cannot use 'ipAllowList' when the Cluster's "
                           "communication stack is '%s'",
                           kCommunicationStackMySQL));
  }

  // Validate the usage of the localAddress option
  if (gr_options.local_address.has_value()) {
    validate_local_address_option(gr_options.local_address.value_or(""),
                                  switch_communication_stack.value_or(""),
                                  canonical_port);
  }
}

void Reboot_cluster_options::set_primary(std::string value) {
  try {
    auto cnx_opt = mysqlsh::get_connection_options(shcore::Value{value});

    if (cnx_opt.get_host().empty())
      throw shcore::Exception::argument_error("host cannot be empty.");
    else if (!cnx_opt.has_port())
      throw shcore::Exception::argument_error("port is missing.");

  } catch (const std::exception &err) {
    throw shcore::Exception::argument_error(
        shcore::str_format("Invalid value '%s' for 'primary' option: %s",
                           value.c_str(), err.what()));
  }

  primary = std::move(value);
}

void Reboot_cluster_options::set_switch_communication_stack(
    const std::string &value) {
  switch_communication_stack = shcore::str_upper(shcore::str_strip(value));

  if (switch_communication_stack->empty()) {
    throw shcore::Exception::argument_error(shcore::str_format(
        "Invalid value for '%s', string value cannot be empty.",
        kSwitchCommunicationStack));
  }
}

void Reboot_cluster_options::set_timeout(uint32_t timeout_seconds) {
  timeout = std::chrono::seconds{timeout_seconds};
}

std::chrono::seconds Reboot_cluster_options::get_timeout() const {
  return timeout.value_or(std::chrono::seconds{
      current_shell_options()->get().dba_gtid_wait_timeout});
}

const shcore::Option_pack_def<Upgrade_metadata_options>
    &Upgrade_metadata_options::options() {
  static const auto opts =
      shcore::Option_pack_def<Upgrade_metadata_options>().optional(
          kDryRun, &Upgrade_metadata_options::dry_run);

  return opts;
}

}  // namespace dba
}  // namespace mysqlsh
