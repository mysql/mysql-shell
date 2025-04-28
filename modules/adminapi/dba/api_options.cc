/*
 * Copyright (c) 2021, 2025, Oracle and/or its affiliates.
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
#include "modules/adminapi/common/common_cmd_options.h"
#include "modules/adminapi/common/server_features.h"
#include "mysqlshdk/libs/db/utils_connection.h"
#include "mysqlshdk/libs/utils/utils_file.h"
#include "shellcore/shell_options.h"

namespace mysqlsh::dba {

namespace {
constexpr shcore::Option_data<std::optional<std::string>> kOptionSandboxDir{
    "sandboxDir"};
constexpr shcore::Option_data<std::optional<int>> kOptionXPort{"portx"};
constexpr shcore::Option_data<std::string> kOptionAllowRootFrom{
    "allowRootFrom"};
constexpr shcore::Option_data<bool> kOptionIgnoreSSLError{"ignoreSslError"};
constexpr shcore::Option_data<shcore::Array_t> kOptionMysqldOptions{
    "mysqldOptions"};
constexpr shcore::Option_data<std::string> kOptionMysqldPath{"mysqldPath"};
}  // namespace

const shcore::Option_pack_def<Common_sandbox_options>
    &Common_sandbox_options::options() {
  static const auto opts = std::invoke([]() {
    shcore::Option_pack_builder<Common_sandbox_options> b;

    b.optional(
        kOptionSandboxDir, &Common_sandbox_options::sandbox_dir,
        [](std::optional<std::string> &value) {
          if (value.has_value() && shcore::is_folder(*value)) return;

          throw shcore::Exception::argument_error(shcore::str_format(
              "The sandbox dir path '%s' is not valid: it %s.",
              value.value_or("").c_str(),
              shcore::path_exists(value.value_or("")) ? "is not a directory"
                                                      : "does not exist"));
        });

    return b.build();
  });

  return opts;
}

std::string Common_sandbox_options::get_sandbox_dir() const {
  return sandbox_dir.has_value()
             ? *sandbox_dir
             : mysqlsh::current_shell_options()->get().sandbox_directory;
}

const shcore::Option_pack_def<Stop_sandbox_options>
    &Stop_sandbox_options::options() {
  static const auto opts = std::invoke([]() {
    shcore::Option_pack_builder<Stop_sandbox_options> b;

    b.include<Common_sandbox_options>();
    b.optional(kOptionPassword, &Stop_sandbox_options::password);
    return b.build();
  });

  return opts;
}

const shcore::Option_pack_def<Deploy_sandbox_options>
    &Deploy_sandbox_options::options() {
  static const auto opts = std::invoke([]() {
    shcore::Option_pack_builder<Deploy_sandbox_options> b;

    b.include<Common_sandbox_options>();
    b.optional(kOptionPassword, &Deploy_sandbox_options::password);
    b.optional(kOptionXPort, &Deploy_sandbox_options::xport);
    b.optional(kOptionAllowRootFrom, &Deploy_sandbox_options::allow_root_from);
    b.optional(kOptionIgnoreSSLError,
               &Deploy_sandbox_options::ignore_ssl_error);
    b.optional(kOptionMysqldPath, &Deploy_sandbox_options::mysqld_path);
    b.optional(kOptionMysqldOptions, &Deploy_sandbox_options::mysqld_options);

    return b.build();
  });

  return opts;
}

namespace {

constexpr shcore::Option_data<std::string> kOptionMycnfPath{"mycnfPath"};
constexpr shcore::Option_data<std::string> kOptionOutputMycnfPath{
    "outputMycnfPath"};
constexpr shcore::Option_data<std::string> kOptionVerifyMyCnf{"verifyMyCnf"};

constexpr shcore::Option_data<std::string> kOptionClusterAdmin{"clusterAdmin"};
constexpr shcore::Option_data<std::optional<std::string>>
    kOptionClusterAdminPassword{"clusterAdminPassword"};
constexpr shcore::Option_data<std::optional<std::string>>
    kOptionClusterAdminCertIssuer{"clusterAdminCertIssuer"};
constexpr shcore::Option_data<std::optional<std::string>>
    kOptionClusterAdminCertSubject{"clusterAdminCertSubject"};
constexpr shcore::Option_data<std::optional<int64_t>>
    kOptionClusterAdminPasswordExpiration{"clusterAdminPasswordExpiration"};
constexpr shcore::Option_data<std::optional<bool>> kOptionRestart{"restart"};
constexpr shcore::Option_data<std::optional<int64_t>>
    kOptionReplicaParallelWorkers{"applierWorkerThreads"};

std::optional<int64_t> convert_admin_password_expiration_option(
    const shcore::Value &value) {
  switch (value.get_type()) {
    case shcore::Null:
      return std::nullopt;

    case shcore::String: {
      const auto &s = value.get_string();
      if (shcore::str_caseeq(s, "NEVER")) return -1;
      if (shcore::str_caseeq(s, "DEFAULT") || s.empty()) return std::nullopt;

      [[fallthrough]];
    }

    case shcore::Integer:
      [[fallthrough]];
    case shcore::UInteger:
      try {
        if (auto ui_value = value.as_uint(); ui_value > 0) return ui_value;
      } catch (...) {
      }
      throw shcore::Exception::value_error(shcore::str_format(
          "Option '%.*s' UInteger, 'NEVER' or 'DEFAULT' expected, but value is "
          "'%s'",
          static_cast<int>(kOptionClusterAdminPasswordExpiration.name.length()),
          kOptionClusterAdminPasswordExpiration.name.data(),
          value.as_string().c_str()));
      break;

    default:
      throw shcore::Exception::type_error(shcore::str_format(
          "Option '%.*s' UInteger, 'NEVER' or 'DEFAULT' expected, but type is "
          "%s",
          static_cast<int>(kOptionClusterAdminPasswordExpiration.name.length()),
          kOptionClusterAdminPasswordExpiration.name.data(),
          type_name(value.get_type()).c_str()));
  }
}

void check_applier_work_threads_options(std::string_view name,
                                        std::optional<int64_t> &value) {
  if (value.value_or(0) >= 0) return;

  throw shcore::Exception::argument_error(shcore::str_format(
      "Invalid value for '%.*s' option: it only accepts positive integers.",
      static_cast<int>(name.length()), name.data()));
}

}  // namespace

const shcore::Option_pack_def<Check_instance_configuration_options>
    &Check_instance_configuration_options::options() {
  static const auto opts = std::invoke([]() {
    shcore::Option_pack_builder<Check_instance_configuration_options> b;

    b.optional(kOptionMycnfPath,
               &Check_instance_configuration_options::mycnf_path);
    b.optional(kOptionVerifyMyCnf,
               &Check_instance_configuration_options::mycnf_path);

    return b.build();
  });

  return opts;
}

const shcore::Option_pack_def<Configure_cluster_instance_options>
    &Configure_cluster_instance_options::options() {
  static const auto opts = std::invoke([]() {
    shcore::Option_pack_builder<Configure_cluster_instance_options> b;

    b.optional(kOptionClusterAdmin,
               &Configure_cluster_instance_options::cluster_admin);
    b.optional(kOptionClusterAdminPassword,
               &Configure_cluster_instance_options::cluster_admin_password);
    b.optional(kOptionClusterAdminCertIssuer,
               &Configure_cluster_instance_options::cluster_admin_cert_issuer);
    b.optional(kOptionClusterAdminCertSubject,
               &Configure_cluster_instance_options::cluster_admin_cert_subject);
    b.optional_as<shcore::Value>(
        kOptionClusterAdminPasswordExpiration,
        &Configure_cluster_instance_options::cluster_admin_password_expiration,
        &convert_admin_password_expiration_option);

    b.optional(kOptionRestart, &Configure_cluster_instance_options::restart);

    b.optional(kOptionMycnfPath,
               &Configure_cluster_instance_options::mycnf_path);
    b.optional(kOptionOutputMycnfPath,
               &Configure_cluster_instance_options::output_mycnf_path);

    b.optional(kOptionReplicaParallelWorkers,
               &Configure_cluster_instance_options::replica_parallel_workers,
               &check_applier_work_threads_options);

    return b.build();
  });

  return opts;
}

const shcore::Option_pack_def<Configure_replicaset_instance_options>
    &Configure_replicaset_instance_options::options() {
  static const auto opts = std::invoke([]() {
    shcore::Option_pack_builder<Configure_replicaset_instance_options> b;

    b.optional(kOptionClusterAdmin,
               &Configure_replicaset_instance_options::cluster_admin);
    b.optional(kOptionClusterAdminPassword,
               &Configure_replicaset_instance_options::cluster_admin_password);
    b.optional(
        kOptionClusterAdminCertIssuer,
        &Configure_replicaset_instance_options::cluster_admin_cert_issuer);
    b.optional(
        kOptionClusterAdminCertSubject,
        &Configure_replicaset_instance_options::cluster_admin_cert_subject);
    b.optional_as<shcore::Value>(kOptionClusterAdminPasswordExpiration,
                                 &Configure_replicaset_instance_options::
                                     cluster_admin_password_expiration,
                                 &convert_admin_password_expiration_option);

    b.optional(kOptionRestart, &Configure_replicaset_instance_options::restart);

    b.optional(kOptionReplicaParallelWorkers,
               &Configure_replicaset_instance_options::replica_parallel_workers,
               &check_applier_work_threads_options);

    return b.build();
  });

  return opts;
}

const shcore::Option_pack_def<Replication_auth_options>
    &Replication_auth_options::options() {
  static const auto opts = std::invoke([]() {
    shcore::Option_pack_builder<Replication_auth_options> b;

    b.optional_as<std::string>(
        kOptionMemberAuthType, &Replication_auth_options::member_auth_type,
        [](const std::string &value) -> Replication_auth_type {
          constexpr std::array<std::string_view, 5>
              kReplicationMemberAuthValues = {
                  kReplicationMemberAuthPassword,
                  kReplicationMemberAuthCertIssuer,
                  kReplicationMemberAuthCertSubject,
                  kReplicationMemberAuthCertIssuerPassword,
                  kReplicationMemberAuthCertSubjectPassword};

          if (std::find(kReplicationMemberAuthValues.begin(),
                        kReplicationMemberAuthValues.end(),
                        shcore::str_upper(value)) ==
              kReplicationMemberAuthValues.end()) {
            auto valid_values =
                shcore::str_join(kReplicationMemberAuthValues, ", ");
            throw shcore::Exception::argument_error(shcore::str_format(
                "Invalid value for '%.*s' option. Supported values: %s.",
                static_cast<int>(kOptionMemberAuthType.name.length()),
                kOptionMemberAuthType.name.data(), valid_values.c_str()));
          }

          return to_replication_auth_type(value);
        });

    b.optional(kOptionCertIssuer, &Replication_auth_options::cert_issuer);
    b.optional(kOptionCertSubject, &Replication_auth_options::cert_subject);

    return b.build();
  });

  return opts;
}

namespace {
constexpr shcore::Option_data<std::optional<bool>> kOptionMultiPrimary{
    "multiPrimary"};
constexpr shcore::Option_data<std::optional<bool>> kOptionAdoptFromGR{
    "adoptFromGR"};

}  // namespace

const shcore::Option_pack_def<Create_cluster_options>
    &Create_cluster_options::options() {
  static const auto opts = std::invoke([]() {
    shcore::Option_pack_builder<Create_cluster_options> b;

    b.include(&Create_cluster_options::gr_options);
    b.include(&Create_cluster_options::clone_options);
    b.include(&Create_cluster_options::member_auth_options);

    b.optional(kOptionMultiPrimary, &Create_cluster_options::multi_primary);
    b.optional(kOptionAdoptFromGR, &Create_cluster_options::adopt_from_gr);
    b.optional(kOptionReplicationAllowedHost,
               &Create_cluster_options::replication_allowed_host);

    b.optional(kOptionForce, &Create_cluster_options::force);

    return b.build();
  });

  return opts;
}

namespace {
constexpr shcore::Option_data<bool> kOptionAdoptFromAR{"adoptFromAR"};
constexpr shcore::Option_data<std::string> kOptionInstanceLabel{
    "instanceLabel"};
constexpr shcore::Option_data<Cluster_ssl_mode> kOptionReplicationSslMode{
    "replicationSslMode"};

}  // namespace

const shcore::Option_pack_def<Create_replicaset_options>
    &Create_replicaset_options::options() {
  static const auto opts = std::invoke([]() {
    shcore::Option_pack_builder<Create_replicaset_options> b;

    b.optional(kOptionAdoptFromAR, &Create_replicaset_options::adopt);
    b.optional(kOptionDryRun, &Create_replicaset_options::dry_run);

    b.optional(kOptionInstanceLabel, &Create_replicaset_options::instance_label,
               [](const Create_replicaset_options &options,
                  std::string_view name, std::string &value) {
                 if (!options.adopt || value.empty()) return;

                 throw shcore::Exception::argument_error(shcore::str_format(
                     "%.*s option not allowed when %.*s:true",
                     static_cast<int>(name.length()), name.data(),
                     static_cast<int>(kOptionAdoptFromAR.name.length()),
                     kOptionAdoptFromAR.name.data()));
               });

    b.optional_as<std::string>(
        kOptionReplicationSslMode, &Create_replicaset_options::ssl_mode,
        [](const auto &value) -> Cluster_ssl_mode {
          if (std::find(
                  kClusterSSLModeValues.begin(), kClusterSSLModeValues.end(),
                  shcore::str_upper(value)) == kClusterSSLModeValues.end()) {
            auto valid_values = shcore::str_join(kClusterSSLModeValues, ", ");
            throw shcore::Exception::argument_error(shcore::str_format(
                "Invalid value for '%.*s' option. Supported values: %s.",
                static_cast<int>(kOptionReplicationSslMode.name.length()),
                kOptionReplicationSslMode.name.data(), valid_values.c_str()));
          }

          return to_cluster_ssl_mode(value);
        });

    b.optional(kOptionGtidSetIsComplete,
               &Create_replicaset_options::gtid_set_is_complete);
    b.optional(kOptionReplicationAllowedHost,
               &Create_replicaset_options::replication_allowed_host);
    b.include(&Create_replicaset_options::member_auth_options);

    return b.build();
  });

  return opts;
}

const shcore::Option_pack_def<Drop_metadata_schema_options>
    &Drop_metadata_schema_options::options() {
  static const auto opts = std::invoke([]() {
    shcore::Option_pack_builder<Drop_metadata_schema_options> b;

    b.optional(kOptionForce, &Drop_metadata_schema_options::force);
    return b.build();
  });

  return opts;
}

namespace {
constexpr shcore::Option_data<std::optional<std::string>> kOptionPrimary{
    "primary"};
constexpr shcore::Option_data<std::optional<std::string>>
    kOptionSwitch_communication_stack{"switchCommunicationStack"};
}  // namespace

const shcore::Option_pack_def<Reboot_cluster_options>
    &Reboot_cluster_options::options() {
  static const auto opts = std::invoke([]() {
    shcore::Option_pack_builder<Reboot_cluster_options> b;

    b.optional(kOptionForce, &Reboot_cluster_options::force);
    b.optional(kOptionDryRun.as<std::optional<bool>>(),
               &Reboot_cluster_options::dry_run);

    b.optional_as<std::string>(
        kOptionPrimary, &Reboot_cluster_options::primary,
        [](const auto &value) -> decltype(kOptionPrimary)::Type {
          try {
            auto cnx_opt =
                mysqlsh::get_connection_options(shcore::Value{value});

            if (cnx_opt.get_host().empty())
              throw shcore::Exception::argument_error("host cannot be empty.");
            if (!cnx_opt.has_port())
              throw shcore::Exception::argument_error("port is missing.");

          } catch (const std::exception &err) {
            throw shcore::Exception::argument_error(shcore::str_format(
                "Invalid value '%s' for 'primary' option: %s", value.c_str(),
                err.what()));
          }

          return value;
        });

    b.optional(
        kOptionSwitch_communication_stack,
        &Reboot_cluster_options::switch_communication_stack,
        [](std::optional<std::string> &value) {
          if (!value.has_value()) return;

          value = shcore::str_upper(shcore::str_strip(*value));

          if (value->empty()) {
            throw shcore::Exception::argument_error(shcore::str_format(
                "Invalid value for '%.*s', string value cannot be empty.",
                static_cast<int>(
                    kOptionSwitch_communication_stack.name.length()),
                kOptionSwitch_communication_stack.name.data()));
          }
        });

    b.optional_as<uint32_t>(
        kOptionTimeout.as<std::optional<std::chrono::seconds>>(),
        &Reboot_cluster_options::timeout, [](auto timeout_seconds) -> auto{
          return std::chrono::seconds{timeout_seconds};
        });

    b.include(&Reboot_cluster_options::gr_options);

    return b.build();
  });

  return opts;
}

void Reboot_cluster_options::check_option_values(
    const mysqlshdk::utils::Version &version, int canonical_port,
    std::string_view comm_stack) const {
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

std::chrono::seconds Reboot_cluster_options::get_timeout() const {
  return timeout.value_or(std::chrono::seconds{
      current_shell_options()->get().dba_gtid_wait_timeout});
}

const shcore::Option_pack_def<Upgrade_metadata_options>
    &Upgrade_metadata_options::options() {
  static const auto opts = std::invoke([]() {
    shcore::Option_pack_builder<Upgrade_metadata_options> b;

    b.optional(kOptionDryRun, &Upgrade_metadata_options::dry_run);

    return b.build();
  });

  return opts;
}

}  // namespace mysqlsh::dba
