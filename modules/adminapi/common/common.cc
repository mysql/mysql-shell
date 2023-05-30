/*
 * Copyright (c) 2016, 2023, Oracle and/or its affiliates.
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

#include "modules/adminapi/common/common.h"

#include <algorithm>
#include <cstdlib>
#include <iterator>
#include <map>
#include <set>
#include <string>
#include <utility>

#include "modules/adminapi/common/cluster_types.h"
#include "modules/adminapi/common/dba_errors.h"
#include "modules/adminapi/common/metadata_storage.h"
#include "modules/adminapi/common/preconditions.h"
#include "modules/adminapi/common/server_features.h"
#include "modules/adminapi/common/sql.h"
#include "modules/adminapi/common/validations.h"
#include "modules/adminapi/mod_dba.h"
#include "mysqlshdk/include/shellcore/console.h"
#include "mysqlshdk/include/shellcore/interrupt_handler.h"
#include "mysqlshdk/include/shellcore/shell_options.h"
#include "mysqlshdk/libs/config/config_file_handler.h"
#include "mysqlshdk/libs/config/config_server_handler.h"
#include "mysqlshdk/libs/db/connection_options.h"
#include "mysqlshdk/libs/db/mysql/session.h"
#include "mysqlshdk/libs/db/utils_error.h"
#include "mysqlshdk/libs/mysql/clone.h"
#include "mysqlshdk/libs/mysql/group_replication.h"
#include "mysqlshdk/libs/mysql/gtid_utils.h"
#include "mysqlshdk/libs/mysql/replication.h"
#include "mysqlshdk/libs/mysql/script.h"
#include "mysqlshdk/libs/mysql/user_privileges.h"
#include "mysqlshdk/libs/textui/progress.h"
#include "mysqlshdk/libs/textui/textui.h"
#include "mysqlshdk/libs/utils/debug.h"
#include "mysqlshdk/libs/utils/utils_file.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_lexing.h"
#include "mysqlshdk/libs/utils/utils_net.h"
#include "mysqlshdk/libs/utils/utils_sqlstring.h"
#include "mysqlshdk/libs/utils/utils_string.h"
#include "mysqlshdk/libs/utils/version.h"
#include "mysqlshdk/shellcore/shell_console.h"

namespace mysqlsh {
namespace dba {

namespace {
constexpr const char kSandboxDatadir[] = "sandboxdata";
constexpr uint16_t k_max_port = 65535;

/**
 * Validates the session used to manage the group.
 *
 * Checks if the given session is valid for use with AdminAPI.
 *
 * @param instance A group session to validate.
 *
 * @throws shcore::Exception::runtime_error if session is not valid.
 */
void validate_gr_session(
    const std::shared_ptr<mysqlshdk::db::ISession> &group_session) {
  // TODO(alfredo) - this check seems too extreme, this only matters in the
  // target instance (and only for some operations) and this function can get
  // called on other members
  if (mysqlshdk::gr::is_group_replication_delayed_starting(
          mysqlsh::dba::Instance(group_session)))
    throw shcore::Exception::runtime_error(
        "Cannot perform operation while group replication is "
        "starting up");
}

}  // namespace

namespace TargetType {
std::string to_string(Type type) {
  switch (type) {
    case Type::AsyncReplicaSet:
      return "InnoDB ReplicaSet";
    case Type::GroupReplication:
      return "unmanaged Group Replication";
    case Type::InnoDBCluster:
      return "InnoDB Cluster";
    case Type::InnoDBClusterSet:
      return "InnoDB ClusterSet";
    case Type::InnoDBClusterSetOffline:
      return "InnoDB ClusterSet offline";
    case Type::AsyncReplication:
      return "unmanaged Asynchronous Replication";
    case Type::Standalone:
      return "standalone";
    case Type::StandaloneInMetadata:
      return "standalone in Metadata";
    case Type::StandaloneWithMetadata:
      return "standalone with Metadata";
    case Type::Unknown:
      return "unknown";
    default:
      throw std::logic_error("Unexpected TargetType");
  }
}
}  // namespace TargetType

std::string to_string(Cluster_ssl_mode ssl_mode) {
  switch (ssl_mode) {
    case Cluster_ssl_mode::AUTO:
      return kClusterSSLModeAuto;
    case Cluster_ssl_mode::DISABLED:
      return kClusterSSLModeDisabled;
    case Cluster_ssl_mode::REQUIRED:
      return kClusterSSLModeRequired;
    case Cluster_ssl_mode::VERIFY_CA:
      return kClusterSSLModeVerifyCA;
    case Cluster_ssl_mode::VERIFY_IDENTITY:
      return kClusterSSLModeVerifyIdentity;
    case Cluster_ssl_mode::NONE:
      return "NONE";
  }
  return {};
}

Cluster_ssl_mode to_cluster_ssl_mode(std::string_view mode) {
  if (shcore::str_caseeq(kClusterSSLModeAuto, mode))
    return Cluster_ssl_mode::AUTO;
  if (shcore::str_caseeq(kClusterSSLModeDisabled, mode))
    return Cluster_ssl_mode::DISABLED;
  if (shcore::str_caseeq(kClusterSSLModeRequired, mode))
    return Cluster_ssl_mode::REQUIRED;
  if (shcore::str_caseeq(kClusterSSLModeVerifyCA, mode))
    return Cluster_ssl_mode::VERIFY_CA;
  if (shcore::str_caseeq(kClusterSSLModeVerifyIdentity, mode))
    return Cluster_ssl_mode::VERIFY_IDENTITY;

  throw std::runtime_error(
      shcore::str_format("Unsupported Cluster SSL-mode: %.*s",
                         static_cast<int>(mode.size()), mode.data()));
}

std::string to_string(Replication_auth_type auth) {
  switch (auth) {
    case Replication_auth_type::PASSWORD:
      return kReplicationMemberAuthPassword;
    case Replication_auth_type::CERT_ISSUER:
      return kReplicationMemberAuthCertIssuer;
    case Replication_auth_type::CERT_SUBJECT:
      return kReplicationMemberAuthCertSubject;
    case Replication_auth_type::CERT_ISSUER_PASSWORD:
      return kReplicationMemberAuthCertIssuerPassword;
    case Replication_auth_type::CERT_SUBJECT_PASSWORD:
      return kReplicationMemberAuthCertSubjectPassword;
  }
  return {};
}

Replication_auth_type to_replication_auth_type(std::string_view auth) {
  if (shcore::str_caseeq(kReplicationMemberAuthPassword, auth))
    return Replication_auth_type::PASSWORD;
  if (shcore::str_caseeq(kReplicationMemberAuthCertIssuer, auth))
    return Replication_auth_type::CERT_ISSUER;
  if (shcore::str_caseeq(kReplicationMemberAuthCertSubject, auth))
    return Replication_auth_type::CERT_SUBJECT;
  if (shcore::str_caseeq(kReplicationMemberAuthCertIssuerPassword, auth))
    return Replication_auth_type::CERT_ISSUER_PASSWORD;
  if (shcore::str_caseeq(kReplicationMemberAuthCertSubjectPassword, auth))
    return Replication_auth_type::CERT_SUBJECT_PASSWORD;

  throw std::runtime_error(
      shcore::str_format("Unsupported replication member authentication: %.*s",
                         static_cast<int>(auth.size()), auth.data()));
}

std::string get_mysqlprovision_error_string(
    const shcore::Value::Array_type_ref &errors) {
  if (!errors) return "mysqlprovision error";
  std::vector<std::string> str_errors;

  if (errors) {
    for (auto error : *errors) {
      auto data = error.as_map();
      auto error_type = data->get_string("type");
      auto error_text = data->get_string("msg");

      str_errors.push_back(error_type + ": " + error_text);
    }
  } else {
    str_errors.push_back(
        "Unexpected error calling mysqlprovision, "
        "see the shell log for more details");
  }

  return shcore::str_join(str_errors, "\n");
}

bool is_option_supported(
    const mysqlshdk::utils::Version &version, const std::string &option,
    const std::map<std::string, Option_availability> &options_map) {
  assert(options_map.find(option) != options_map.end());
  auto &opt_avail = options_map.at(option);

  // if no version was supplied, options can be used regardless of version
  if (opt_avail.support_in_80.get_major() == 0 &&
      opt_avail.support_in_57.get_major() == 0)
    return true;

  if (version.get_major() == 8) {
    // 8.0 server
    // if only the 5.7 version was provided to the Option_availability struct,
    // then assume variable will also be supported in 8.0. This check is enough
    // since the default Version constructor sets the major version to 0.
    return version >= opt_avail.support_in_80;
  }

  if (version.get_major() == 5 && version.get_minor() == 7) {
    // 5.7 server
    if (opt_avail.support_in_57.get_major() == 0) {
      // if the default constructor for version was used, no 5.7 version was
      // used on the Option_availability struct, meaning 5.7 is not supported
      return false;
    }

    return version >= opt_avail.support_in_57;
  }

  throw std::runtime_error(
      "Unexpected version found for option support check: '" +
      version.get_full() + "'.");
}

void validate_replication_filters(const mysqlshdk::mysql::IInstance &instance,
                                  Cluster_type cluster_type) {
  // SHOW MASTER STATUS includes info about binlog filters, which filter
  // events from being written to the binlog, at the master
  auto result = instance.query("SHOW MASTER STATUS");

  while (auto row = result->fetch_one()) {
    if (!row->get_string(2, "").empty() || !row->get_string(3, "").empty())
      throw shcore::Exception(
          instance.descr() +
              ": instance has binlog filters configured, but they are "
              "not supported in " +
              to_display_string(cluster_type, Display_form::THINGS_FULL) + ".",
          SHERR_DBA_INVALID_SERVER_CONFIGURATION);
  }

  // replication_applier_filters and replication_applier_global_filters
  // have info about replication filters, which filters what's applied from
  // the relay log at a slave
  if (instance.get_version() >= mysqlshdk::utils::Version(8, 0, 11)) {
    bool has_global_filter = false;
    bool has_filter = false;
    try {
      if (instance.queryf_one_int(
              0, 0,
              "SELECT count(*) "
              "FROM performance_schema.replication_applier_global_filters "
              "WHERE filter_rule <> ''") > 0) {
        has_global_filter = true;
      }

      if (instance.queryf_one_int(0, 0,
                                  "SELECT count(*) FROM "
                                  "performance_schema.replication_applier_"
                                  "filters WHERE filter_rule <> ''") > 0) {
        has_filter = true;
      }
    } catch (std::exception &e) {
      log_warning("Error querying for replication filters at %s: %s",
                  instance.descr().c_str(), e.what());
    }

    if (has_global_filter) {
      throw shcore::Exception(
          instance.descr() +
              ": instance has global replication filters "
              "configured, but they are not supported in " +
              to_display_string(cluster_type, Display_form::THINGS_FULL) + ".",
          SHERR_DBA_INVALID_SERVER_CONFIGURATION);
    }

    if (has_filter) {
      throw shcore::Exception(
          instance.descr() +
              ": instance has replication filters configured, but they are "
              "not supported in " +
              to_display_string(cluster_type, Display_form::THINGS_FULL) + ".",
          SHERR_DBA_INVALID_SERVER_CONFIGURATION);
    }
  }
}

void resolve_ssl_mode_option(const std::string &option,
                             const std::string &context,
                             const mysqlshdk::mysql::IInstance &instance,
                             Cluster_ssl_mode *ssl_mode) {
  bool require_secure_transport =
      instance.get_sysvar_bool("require_secure_transport", false);
  bool have_ssl = instance.is_ssl_enabled();

  // sslMode is DISABLED but instance requires SSL
  if (*ssl_mode == Cluster_ssl_mode::DISABLED && require_secure_transport) {
    throw shcore::Exception::argument_error(
        "The instance '" + instance.descr() +
        "' requires secure connections, to create the " + context +
        " either turn off require_secure_transport or use the " + option +
        " option with 'REQUIRED', 'VERIFY_CA' or "
        "'VERIFY_IDENTITY' value.");
  }

  // sslMode is REQUIRED, VERIFY_CA or VERIFY_IDENTITY
  if (!have_ssl && (*ssl_mode == Cluster_ssl_mode::REQUIRED ||
                    *ssl_mode == Cluster_ssl_mode::VERIFY_CA ||
                    *ssl_mode == Cluster_ssl_mode::VERIFY_IDENTITY)) {
    throw shcore::Exception::argument_error(
        "The instance '" + instance.descr() +
        "' does not have TLS enabled, to create the " + context +
        " either use an instance with TLS enabled, remove the " + option +
        " option or use it with any of 'AUTO' or 'DISABLED'.");
  }

  // sslMode is VERIFY_CA or VERIFY_IDENTITY, verify the certificates
  if (*ssl_mode == Cluster_ssl_mode::VERIFY_CA ||
      *ssl_mode == Cluster_ssl_mode::VERIFY_IDENTITY) {
    ensure_certificates_set(instance, *ssl_mode);
  }

  if (*ssl_mode == Cluster_ssl_mode::NONE ||
      *ssl_mode == Cluster_ssl_mode::AUTO) {
    if (have_ssl) {
      *ssl_mode = Cluster_ssl_mode::REQUIRED;
    } else {
      current_console()->print_note("TLS not available at '" +
                                    instance.descr() + "', assuming " + option +
                                    " to be DISABLED");

      *ssl_mode = Cluster_ssl_mode::DISABLED;
    }
  }
}

/**
 * Determines the value to use on the instance for the SSL mode
 * configured for the cluster, based on:
 * - The instance having SSL available
 * - The instance having require_secure_transport ON
 * - The cluster SSL configuration
 * - The memberSslMode option given by the user
 *
 * Assigned to:
 *   REQUIRED if SSL is supported and member_ssl_mode is either AUTO or
 *            REQUIRED, and the cluster uses SSL
 *   VERIFY_CA if SSL is supported and member_ssl_mode is VERIFY_CA on the
 *             cluster
 *   VERIFY_IDENTITY if SSL is supported and member_ssl_mode is VERIFY_CA on
 *                   the cluster
 *   DISABLED if SSL is not supported and member_ssl_mode is either unspecified
 *            DISABLED or AUTO and the cluster has SSL disabled.
 *
 * Error:
 *   - Cluster has SSL enabled and member_ssl_mode is DISABLED or not specified
 *   - Cluster has SSL enabled and SSL is not supported on the instance
 *   - Cluster has SSL disabled and member_ssl_mode is REQUIRED, VERIFY_CA or
 *     VERIFY_IDENTITY
 *   - Cluster has SSL disabled and the instance has require_secure_transport ON
 */
void resolve_instance_ssl_mode_option(
    const mysqlshdk::mysql::IInstance &instance,
    const mysqlshdk::mysql::IInstance &pinstance,
    Cluster_ssl_mode *member_ssl_mode) {
  assert(member_ssl_mode);

  Cluster_ssl_mode gr_ssl_mode = to_cluster_ssl_mode(
      pinstance.get_sysvar_string("group_replication_ssl_mode", ""));

  // check for conflicts between the (deprecated) per-instance memberSslMode
  // and the group setting
  switch (gr_ssl_mode) {
    case Cluster_ssl_mode::REQUIRED:
    case Cluster_ssl_mode::VERIFY_CA:
    case Cluster_ssl_mode::VERIFY_IDENTITY:
      // memberSslMode is DISABLED
      if (*member_ssl_mode == Cluster_ssl_mode::DISABLED) {
        throw shcore::Exception::runtime_error(
            "The cluster has TLS (encryption) enabled. "
            "To add the instance '" +
            instance.descr() +
            "' to the cluster either disable TLS on the cluster, remove the "
            "memberSslMode option or use it with any of 'AUTO', 'REQUIRED', "
            "'VERIFY_CA' or 'VERIFY_IDENTITY'.");
      }
      // addInstance() can have a different memberSslMode from the cluster,
      // although that's deprecated
      if (*member_ssl_mode == Cluster_ssl_mode::AUTO ||
          *member_ssl_mode == Cluster_ssl_mode::NONE)
        *member_ssl_mode = gr_ssl_mode;
      break;

    case Cluster_ssl_mode::NONE:
    case Cluster_ssl_mode::DISABLED:
      if (*member_ssl_mode == Cluster_ssl_mode::REQUIRED ||
          *member_ssl_mode == Cluster_ssl_mode::VERIFY_CA ||
          *member_ssl_mode == Cluster_ssl_mode::VERIFY_IDENTITY) {
        throw shcore::Exception::runtime_error(
            "The cluster has TLS (encryption) disabled. "
            "To add the instance '" +
            instance.descr() +
            "' to the cluster either "
            "enable TLS on the cluster, remove the memberSslMode option or "
            "use it with any of 'AUTO' or 'DISABLED'.");
      }
      *member_ssl_mode = gr_ssl_mode;
      break;

    case Cluster_ssl_mode::AUTO:
      // not possible
      assert(0);
      *member_ssl_mode = gr_ssl_mode;
      break;
  }

  validate_instance_ssl_mode(Cluster_type::GROUP_REPLICATION, instance,
                             *member_ssl_mode);
}

void validate_instance_ssl_mode(Cluster_type type,
                                const mysqlshdk::mysql::IInstance &instance,
                                Cluster_ssl_mode ssl_mode) {
  assert(ssl_mode != Cluster_ssl_mode::AUTO);

  bool have_ssl = instance.is_ssl_enabled();

  // The cluster REQUIRES SSL
  if (ssl_mode == Cluster_ssl_mode::REQUIRED ||
      ssl_mode == Cluster_ssl_mode::VERIFY_CA ||
      ssl_mode == Cluster_ssl_mode::VERIFY_IDENTITY) {
    // Now checks if SSL is actually supported on the instance

    // SSL is not supported on the instance
    if (!have_ssl) {
      throw shcore::Exception::runtime_error(
          "Instance '" + instance.descr() +
          "' does not support TLS and cannot join " +
          to_display_string(type, Display_form::A_THING) +
          " with TLS (encryption) enabled. Enable TLS support on the instance "
          "and try again, otherwise it can only be added to " +
          to_display_string(type, Display_form::A_THING) +
          " with TLS disabled.");
    } else {
      if (ssl_mode == Cluster_ssl_mode::VERIFY_CA ||
          ssl_mode == Cluster_ssl_mode::VERIFY_IDENTITY) {
        // memberSslMode is VERIFY_CA or VERIFY_IDENTITY
        // verify the certificates
        ensure_certificates_set(instance, ssl_mode);
      }
    }
    // The cluster has SSL DISABLED
  } else if (ssl_mode == Cluster_ssl_mode::DISABLED) {
    bool require_secure_transport =
        instance.get_sysvar_bool("require_secure_transport", false);

    // If the instance session requires SSL connections, then it can's
    // Join a cluster with SSL disabled
    if (require_secure_transport) {
      throw shcore::Exception::runtime_error(
          "The instance '" + instance.descr() +
          "' is configured to require a secure transport but the " +
          to_display_string(type, Display_form::THING) +
          " has TLS disabled. To add the instance to the " +
          to_display_string(type, Display_form::THING) +
          ", either turn OFF the require_secure_transport option on the "
          "instance or enable TLS on the " +
          to_display_string(type, Display_form::THING) + ".");
    }
  }
}

void validate_instance_member_auth_type(
    const mysqlshdk::mysql::IInstance &instance, bool is_replica_cluster,
    Cluster_ssl_mode ssl_mode, std::string_view ssl_mode_option,
    Replication_auth_type auth_type) {
  if (auth_type == Replication_auth_type::PASSWORD) return;

  if (ssl_mode == Cluster_ssl_mode::DISABLED) {
    if (is_replica_cluster)
      throw shcore::Exception::runtime_error(shcore::str_format(
          "Instance '%s' has the \"%.*s\" option with the value "
          "'DISABLED', which isn't compatible with the \"memberAuthType\" "
          "value of '%s' inherited from the ClusterSet.",
          instance.descr().c_str(), static_cast<int>(ssl_mode_option.length()),
          ssl_mode_option.data(), to_string(auth_type).c_str()));

    throw shcore::Exception::runtime_error(shcore::str_format(
        "Instance '%s' has the \"%.*s\" option with the value "
        "'DISABLED', which means that \"memberAuthType\" only accepts the "
        "value 'PASSWORD'.",
        instance.descr().c_str(), static_cast<int>(ssl_mode_option.length()),
        ssl_mode_option.data()));
  }

  if (!instance.is_ssl_enabled()) {
    std::string_view cluster_set_ctx{
        is_replica_cluster ? " (inherited from the ClusterSet)" : ""};

    throw shcore::Exception::runtime_error(shcore::str_format(
        "Instance '%s' does not support TLS, which means that "
        "\"memberAuthType\"%.*s with value '%s' isn't supported.",
        instance.descr().c_str(), static_cast<int>(cluster_set_ctx.length()),
        cluster_set_ctx.data(), to_string(auth_type).c_str()));
  }

  // verify the certificates
  ensure_certificates_set(instance, auth_type);
}

void validate_instance_member_auth_options(std::string_view context,
                                           bool is_replica_cluster,
                                           Replication_auth_type auth_type,
                                           std::string_view cert_subject) {
  std::string_view cluster_set_ctx{
      is_replica_cluster ? " (inherited from the ClusterSet)" : ""};

  switch (auth_type) {
    case Replication_auth_type::PASSWORD:
    case Replication_auth_type::CERT_ISSUER:
    case Replication_auth_type::CERT_ISSUER_PASSWORD:
      if (!cert_subject.empty())
        mysqlsh::current_console()->print_note(shcore::str_format(
            "The %.*s's SSL mode%.*s is set to %s, which makes the "
            "'certSubject' option not required. The option will be ignored.",
            static_cast<int>(context.length()), context.data(),
            static_cast<int>(cluster_set_ctx.length()), cluster_set_ctx.data(),
            to_string(auth_type).c_str()));
      break;

    case Replication_auth_type::CERT_SUBJECT:
    case Replication_auth_type::CERT_SUBJECT_PASSWORD:
      if (cert_subject.empty())
        throw shcore::Exception(
            shcore::str_format(
                "The %.*s's SSL mode%.*s is set to %s but the 'certSubject' "
                "option for the instance wasn't supplied.",
                static_cast<int>(context.length()), context.data(),
                static_cast<int>(cluster_set_ctx.length()),
                cluster_set_ctx.data(), to_string(auth_type).c_str()),
            SHERR_DBA_MISSING_CERT_OPTION);
      break;
    default:
      break;
  }
}

void validate_instance_member_auth_options(std::string_view context,
                                           Replication_auth_type auth_type,
                                           std::string_view cert_issuer,
                                           std::string_view cert_subject) {
  if (auth_type == Replication_auth_type::PASSWORD) {
    if (cert_issuer.empty() && cert_subject.empty()) return;

    mysqlsh::current_console()->print_note(shcore::str_format(
        "The %.*s's SSL mode is set to %s, which makes both the 'certIssuer' "
        "and 'certSubject' options not required. Both options will be ignored.",
        static_cast<int>(context.length()), context.data(),
        to_string(auth_type).c_str()));
    return;
  }

  switch (auth_type) {
    case Replication_auth_type::CERT_ISSUER:
    case Replication_auth_type::CERT_ISSUER_PASSWORD:
    case Replication_auth_type::CERT_SUBJECT:
    case Replication_auth_type::CERT_SUBJECT_PASSWORD:
      if (cert_issuer.empty())
        throw shcore::Exception(
            shcore::str_format("The %.*s's SSL mode is set to %s but the "
                               "'certIssuer' option wasn't supplied.",
                               static_cast<int>(context.length()),
                               context.data(), to_string(auth_type).c_str()),
            SHERR_DBA_MISSING_CERT_OPTION);
      break;
    default:
      break;
  }

  switch (auth_type) {
    case Replication_auth_type::CERT_ISSUER:
    case Replication_auth_type::CERT_ISSUER_PASSWORD:
      if (!cert_subject.empty())
        mysqlsh::current_console()->print_note(shcore::str_format(
            "The %.*s's SSL mode is set to %s, which makes the 'certSubject' "
            "option not required. The option will be ignored.",
            static_cast<int>(context.length()), context.data(),
            to_string(auth_type).c_str()));
      break;

    case Replication_auth_type::CERT_SUBJECT:
    case Replication_auth_type::CERT_SUBJECT_PASSWORD:
      if (cert_subject.empty())
        throw shcore::Exception(
            shcore::str_format(
                "The %.*s's SSL mode is set to %s but the 'certSubject' option "
                "for the instance wasn't supplied.",
                static_cast<int>(context.length()), context.data(),
                to_string(auth_type).c_str()),
            SHERR_DBA_MISSING_CERT_OPTION);
      break;
    default:
      break;
  }
}

/**
 * Gets the list of instances belonging to the GR group which are not
 * registered on the metadata for a specific ReplicaSet.
 *
 * @param metadata metadata object which represents the session to the metadata
 *                 storage
 * @param cluster_id the Cluster id
 * @return a vector of NewInstanceInfo with the list of instances found
 * as belonging to the GR group but not registered on the Metadata
 */
std::vector<NewInstanceInfo> get_newly_discovered_instances(
    const mysqlshdk::mysql::IInstance &group_server,
    const std::shared_ptr<MetadataStorage> &metadata, Cluster_id cluster_id) {
  std::vector<mysqlshdk::gr::Member> instances_gr(
      mysqlshdk::gr::get_members(group_server));

  std::vector<Instance_metadata> instances_md_array =
      metadata->get_all_instances(cluster_id);

  // Check if the instances_gr list has more members than the instances_md lists
  // Meaning that an instance was added to the GR group outside of the AdminAPI
  std::vector<NewInstanceInfo> ret;
  for (const auto &i : instances_gr) {
    if (std::find_if(instances_md_array.begin(), instances_md_array.end(),
                     [&i](const Instance_metadata &md) {
                       return md.uuid == i.uuid;
                     }) == instances_md_array.end()) {
      NewInstanceInfo info;
      info.member_id = i.uuid;
      info.host = i.host;
      info.port = i.port;
      info.version = i.version;

      ret.push_back(info);
    }
  }

  return ret;
}

/**
 * Gets the list of instances registered on the Metadata, for a specific
 * ReplicaSet, which do not belong in the GR group.
 *
 * @param metadata metadata object which represents the session to the metadata
 *                 storage
 * @param cluster_id the Cluster id
 * @return a vector of MissingInstanceInfo with the list of instances found
 * as registered on the Metadata but not present in the GR group.
 */
std::vector<MissingInstanceInfo> get_unavailable_instances(
    const mysqlshdk::mysql::IInstance &group_server,
    const std::shared_ptr<MetadataStorage> &metadata, Cluster_id cluster_id) {
  std::vector<std::string> instances_gr_array, instances_md_array;

  for (const mysqlshdk::gr::Member &m :
       mysqlshdk::gr::get_members(group_server))
    instances_gr_array.push_back(m.uuid);

  std::vector<Instance_metadata> all_instances(
      metadata->get_all_instances(cluster_id));
  for (const auto &i : all_instances) instances_md_array.push_back(i.uuid);

  // Check the differences between the two lists
  std::vector<std::string> removed_members;

  // Sort the arrays in order to be able to use
  // std::set_difference()
  std::sort(instances_gr_array.begin(), instances_gr_array.end());
  std::sort(instances_md_array.begin(), instances_md_array.end());

  // Check if the instances_md list has more members than the instances_gr
  // lists. Meaning that an instance was removed from the GR group outside
  // of the AdminAPI
  std::set_difference(instances_md_array.begin(), instances_md_array.end(),
                      instances_gr_array.begin(), instances_gr_array.end(),
                      std::inserter(removed_members, removed_members.begin()));

  std::vector<MissingInstanceInfo> ret;
  for (auto &i : removed_members) {
    for (const auto &m : all_instances) {
      if (m.uuid == i) {
        MissingInstanceInfo info;
        info.id = m.uuid;
        info.label = m.label;
        info.endpoint = m.endpoint;
        ret.push_back(info);
        break;
      }
    }
  }

  return ret;
}

void parse_fully_qualified_cluster_name(const std::string &name,
                                        std::string *out_domain,
                                        std::string *out_partition,
                                        std::string *out_cluster_group) {
  // TODO(pjesus): Replace current code by the commented one, when support
  //               for names with a domain part is really supported.
  *out_domain = "";
  *out_cluster_group = name;
  // size_t p = name.find("::");
  // if (p != std::string::npos) {
  //   *out_domain = name.substr(0, p);
  //   *out_cluster_group = name.substr(p + 2);
  // } else {
  //   *out_domain = "";
  //   *out_cluster_group = name;
  // }
  if (out_partition) *out_partition = "";

  // not supported yet
  // p = out_domain->find("/");
  // if (p != std::string::npos) {
  //   *out_partition = out_domain->substr(p + 1);
  //   *out_domain = out_domain->substr(0, p);
  // }
}

/**
 * Validate the given Cluster name.
 *
 * A fully qualified cluster name has the following form:
 * [domain[/partition]::]clusterGroup
 *
 * Each component must be:
 * non-empty and no greater than 63 characters long and start with an alphabetic
 * or '_' character and just contain alphanumeric or the characters '_', '-' or
 * '.'.
 *
 * @param name of the cluster
 * @param cluster_type indicates what the name is intended for
 * @throws shcore::Exception if the name is not valid.
 */
void SHCORE_PUBLIC validate_cluster_name(const std::string &name,
                                         Cluster_type cluster_type) {
  std::string what;

  switch (cluster_type) {
    case Cluster_type::GROUP_REPLICATION:
      what = "Cluster";
      break;
    case Cluster_type::ASYNC_REPLICATION:
      what = "ReplicaSet";
      break;
    case Cluster_type::REPLICATED_CLUSTER:
      what = "ClusterSet";
      break;
    case Cluster_type::NONE:
      throw shcore::Exception::logic_error("Unknown Cluster_type.");
  }

  if (!name.empty()) {
    std::string domain;
    std::string partition;
    std::string group;

    parse_fully_qualified_cluster_name(name, &domain, &partition, &group);

    if (domain.length() > k_cluster_name_max_length) {
      throw shcore::Exception::argument_error(
          domain + ": The " + what +
          " domain name can not be greater than 63 characters.");
    }
    if (partition.length() > k_cluster_name_max_length) {
      throw shcore::Exception::argument_error(
          partition + ": The " + what +
          " domain partition name can not be greater than 63 "
          "characters.");
    }
    if (group.length() > k_cluster_name_max_length) {
      throw shcore::Exception::argument_error(
          group + ": The " + what +
          " name can not be greater than 63 characters.");
    }

    if (!domain.empty()) {
      if (mysqlshdk::utils::span_keyword(
              domain, 0, k_cluster_name_allowed_chars) != domain.length()) {
        throw shcore::Exception::argument_error(
            "" + what +
            " name may only contain alphanumeric characters, '_', '-', or '.' "
            "and may not start with a number (" +
            domain + ")");
      }
    }
    if (!partition.empty()) {
      if (mysqlshdk::utils::span_keyword(partition, 0,
                                         k_cluster_name_allowed_chars) !=
          partition.length()) {
        throw shcore::Exception::argument_error(
            "" + what +
            " name may only contain alphanumeric characters, '_', '-', or '.' "
            "and may not start with a number (" +
            partition + ")");
      }
    }
    if (!group.empty()) {
      if (mysqlshdk::utils::span_keyword(
              group, 0, k_cluster_name_allowed_chars) != group.length()) {
        throw shcore::Exception::argument_error(
            "" + what +
            " name may only contain alphanumeric characters, '_', '-', or '.' "
            "and may not start with a number (" +
            group + ")");
      }
    } else {
      throw shcore::Exception::argument_error("The " + what +
                                              " name cannot be empty.");
    }
  } else {
    throw shcore::Exception::argument_error("The " + what +
                                            " name cannot be empty.");
  }
}

/**
 * Validate the given label name.
 * Cluster name must be non-empty and no greater than 256 characters long and
 * start with an alphanumeric or '_' character and can only contain
 * alphanumerics, Period ".", Underscore "_", Hyphen "-" or Colon ":"
 * characters.
 * @param name of the cluster
 * @throws shcore::Exception if the name is not valid.
 */
void SHCORE_PUBLIC validate_label(const std::string &label) {
  if (!label.empty()) {
    bool valid = false;

    if (label.length() > 256) {
      throw shcore::Exception::argument_error(
          "The label can not be greater than 256 characters.");
    }

    std::locale locale;

    valid = std::isalnum(label[0], locale) || label[0] == '_';
    if (!valid) {
      throw shcore::Exception::argument_error(
          "The label can only start with an alphanumeric or the '_' "
          "character.");
    }

    size_t index = 1;
    while (valid && index < label.size()) {
      valid = std::isalnum(label[index], locale) || label[index] == '_' ||
              label[index] == '.' || label[index] == '-' || label[index] == ':';
      if (!valid) {
        std::string msg =
            "The label can only contain alphanumerics or the '_', '.', '-', "
            "':' characters. Invalid character '";
        msg.append(&(label.at(index)), 1);
        msg.append("' found.");
        throw shcore::Exception::argument_error(msg);
      }
      index++;
    }
  } else {
    throw shcore::Exception::argument_error("The label can not be empty.");
  }

  return;
}

namespace {
/*
 * Gets Group Replication Group Name variable: group_replication_group_name
 *
 * @param session object which represents the session to the instance
 * @return string with the value of @@group_replication_group_name
 */
std::string get_gr_replicaset_group_name(
    const mysqlshdk::mysql::IInstance &instance) {
  std::string group_name;

  std::string query("SELECT @@group_replication_group_name");

  // Any error will bubble up right away
  auto result = instance.query(query);
  auto row = result->fetch_one();
  if (row) {
    group_name = row->get_as_string(0);
  }
  return group_name;
}
}  // namespace

/**
 * Validates if the current session instance 'group_replication_group_name'
 * differs from the one registered in the corresponding Cluster table of the
 * Metadata
 *
 * @param session session to instance that is supposed to belong to a group
 * @param md_group_name name of the group the member belongs to according to
 *        metadata
 * @return a boolean value indicating whether the replicaSet
 * 'group_replication_group_name' is the same as the one registered in the
 * corresponding replicaset in the Metadata
 */
bool validate_cluster_group_name(const mysqlshdk::mysql::IInstance &instance,
                                 const std::string &md_group_name) {
  std::string gr_group_name = get_gr_replicaset_group_name(instance);

  log_info("Group Replication 'group_name' value: %s", gr_group_name.c_str());
  log_info("Metadata 'group_name' value: %s", md_group_name.c_str());

  // If the value is NULL it means the instance does not belong to any GR group
  // so it can be considered valid
  if (gr_group_name == "NULL") return true;

  return (gr_group_name == md_group_name);
}

/*
 * Check for the value of super-read-only and any open sessions
 * to the instance and raise an exception if super_read_only is turned on.
 *
 * @param instance
 * @param clear_read_only if true, will clear SRO if false, throws error
 * @param interactive if true and if clear_read_only is null, will prompt
 * whether to clear SRO
 *
 * @returns true if super_read_only was disabled
 */
bool validate_super_read_only(const mysqlshdk::mysql::IInstance &instance,
                              mysqlshdk::null_bool clear_read_only,
                              bool interactive) {
  if (auto super_read_only = instance.get_sysvar_bool("super_read_only", false);
      !super_read_only)
    return false;

  auto console = mysqlsh::current_console();

  const std::string error_message =
      "The MySQL instance at '" + instance.descr() +
      "' currently has the super_read_only system variable set to "
      "protect it from inadvertent updates from applications.\nYou must "
      "first unset it to be able to perform any changes to this "
      "instance.\n"
      "For more information see: https://dev.mysql.com/doc/refman/en/"
      "server-system-variables.html#sysvar_super_read_only.";

  if (clear_read_only.is_null() && interactive) {
    console->print_error(error_message);

    if (console->confirm("Do you want to disable super_read_only and continue?",
                         mysqlsh::Prompt_answer::NO) ==
        mysqlsh::Prompt_answer::NO) {
      console->print_info();
      throw shcore::Exception::runtime_error("Server in SUPER_READ_ONLY mode");
    } else {
      console->print_info();
      clear_read_only = true;
    }
  }

  bool super_read_only_changed = false;
  if (clear_read_only.get_safe()) {
    log_info("Disabling super_read_only on the instance '%s'",
             instance.descr().c_str());
    instance.set_sysvar("super_read_only", false);
    super_read_only_changed = true;
  } else {
    console->print_error(error_message);

    throw shcore::Exception::runtime_error("Server in SUPER_READ_ONLY mode");
  }

  return super_read_only_changed;
}

/*
 * Checks if an instance can be rejoined to a given Cluster.
 *
 * @param instance
 * @param metadata Metadata object which represents the session to the metadata
 * storage
 * @param cluster_id The Cluster id
 * @param out_uuid_mistmatch Boolean value that will be set to TRUE if the UUID
 * changed for the given instance
 *
 * @return Indication whether instance can be rejoined and reason if not.
 *
 * It is considered that the UUID changed for an instance if the instance is NOT
 * found using the current UUID but it is found using it's canonical address.
 */
Instance_rejoinability validate_instance_rejoinable(
    const mysqlshdk::mysql::IInstance &instance,
    const std::shared_ptr<MetadataStorage> &metadata, Cluster_id cluster_id,
    bool *out_uuid_mistmatch) {
  std::string instance_uuid = instance.get_uuid();

  // Ensure that:
  // 1 - instance is part of the given cluster
  // 2 - instance is not ONLINE or RECOVERING
  Instance_metadata imd;
  bool uuid_in_md = false;
  try {
    imd = metadata->get_instance_by_uuid(instance.get_uuid());
    uuid_in_md = true;
  } catch (const shcore::Exception &e) {
    if (e.code() == SHERR_DBA_MEMBER_METADATA_MISSING) {
      try {
        imd =
            metadata->get_instance_by_address(instance.get_canonical_address());
      } catch (const shcore::Exception &ex) {
        if (e.code() == SHERR_DBA_MEMBER_METADATA_MISSING) {
          return Instance_rejoinability::NOT_MEMBER;
        }
        throw;
      }
    } else {
      throw;
    }
  }

  // If the instance was not found by UUID then it means the UUID changed
  if (out_uuid_mistmatch) *out_uuid_mistmatch = !uuid_in_md;

  if (imd.cluster_id != cluster_id) return Instance_rejoinability::NOT_MEMBER;

  if (imd.instance_type == Instance_type::READ_REPLICA) {
    return Instance_rejoinability::READ_REPLICA;
  }

  auto state = mysqlshdk::gr::get_member_state(instance);
  if (state == mysqlshdk::gr::Member_state::ONLINE)
    return Instance_rejoinability::ONLINE;
  else if (state == mysqlshdk::gr::Member_state::RECOVERING)
    return Instance_rejoinability::RECOVERING;
  else
    return Instance_rejoinability::REJOINABLE;
}

/**
 * Checks whether the target instance is standalone or not
 *
 * @param  instance    Instance to check
 * @param  target_type Variable containing the TargetType of the instance
 *
 * @return             True if the instance is standalone, False otherwise
 */
bool validate_instance_standalone(const Instance &instance,
                                  TargetType::Type *target_type) {
  const auto md_storage =
      std::make_shared<mysqlsh::dba::MetadataStorage>(instance);

  auto type = mysqlsh::dba::get_instance_type(*md_storage, instance);

  if (type == TargetType::Unknown) {
    assert(!"Unexpected state for instance");
    throw std::logic_error("Unexpected state in " + instance.descr());
  }

  if (target_type) {
    *target_type = type;
  }

  if (type == TargetType::Standalone ||
      type == TargetType::StandaloneWithMetadata ||
      type == TargetType::AsyncReplication) {
    return true;
  }

  return false;
}

/**
 * Checks if the target instance is a sandbox
 *
 * @param instance Object which represents the target instance
 * @param optional string to store the value of the cnfPath in case the instance
 * is a sandbox
 *
 * @return a boolean value which is true if the target instance is a sandbox or
 * false if is not.
 */
bool is_sandbox(const mysqlshdk::mysql::IInstance &instance,
                std::string *cnfPath) {
  int port = 0;
  std::string datadir;

  mysqlsh::dba::get_port_and_datadir(instance, &port, &datadir);
  std::string path_separator = datadir.substr(datadir.size() - 1);
  auto path_elements = shcore::split_string(datadir, path_separator);

  // Removes the empty element at the end
  if (!datadir.empty() && (datadir[datadir.size() - 1] == '/' ||
                           datadir[datadir.size() - 1] == '\\')) {
    datadir.pop_back();
  }

  // Sandbox deployment structure is: <root_path>/<port>/sandboxdata
  // So we validate such structure to determine it is a sandbox
  if (datadir.length() < strlen(kSandboxDatadir) ||
      datadir.compare(datadir.length() - strlen(kSandboxDatadir),
                      strlen(kSandboxDatadir), kSandboxDatadir) != 0) {
    // Not a sandbox, we can immediately return false
    return false;
  } else {
    // Check if the my.cnf file is present
    if (path_elements[path_elements.size() - 3] == std::to_string(port)) {
      path_elements[path_elements.size() - 2] = "my.cnf";
      std::string tmpPath = shcore::str_join(path_elements, path_separator);

      // Remove the trailing path_separator
      if (tmpPath.back() == path_separator[0]) tmpPath.pop_back();

      if (shcore::is_file(tmpPath)) {
        if (cnfPath) {
          *cnfPath = tmpPath;
        }
        return true;
      } else {
        return false;
      }
    } else {
      return false;
    }
  }
}

// AdminAPI interactive handling specific methods

/*
 * Resolves the .cnf file path
 *
 * @param instance Instance object which represents the target instance
 *
 * @return the target instance resolved .cnf file path
 */
std::string prompt_cnf_path(const mysqlshdk::mysql::IInstance &instance) {
  // Path is not given, let's try to autodetect it
  std::string cnfPath;
  // Try to locate the .cnf file path
  // based on the OS and the default my.cnf path as configured
  // by our official MySQL packages

  // Detect the OS
  auto console = mysqlsh::current_console();

  console->print_info();
  console->print_info("Detecting the configuration file...");
  std::vector<std::string> config_paths, default_paths;

  if (instance.get_version() >= mysqlshdk::utils::Version(8, 0, 0)) {
    // use the performance schema table to try and find the existing
    // configuration files
    auto result = instance.query(
        "select DISTINCT VARIABLE_PATH from performance_schema.variables_info "
        "WHERE VARIABLE_PATH <> ''");
    auto row = result->fetch_one();
    while (row) {
      config_paths.emplace_back(row->get_string(0));
      row = result->fetch_one();
    }
  }
  // if no config files were found, try to look in the default paths
  if (config_paths.empty()) {
    shcore::OperatingSystem os = shcore::get_os_type();
    default_paths = mysqlshdk::config::get_default_config_paths(os);
  }
  // Iterate the config_paths found in the instance checking if the user wants
  // to modify any of them
  for (const auto &value : config_paths) {
    // Prompt the user to validate if shall use it or not
    console->print_info("Found configuration file being used by instance '" +
                        instance.get_connection_options().uri_endpoint() +
                        "' at location: " + value);

    if (console->confirm("Do you want to modify this file?") ==
        Prompt_answer::YES) {
      cnfPath = value;
      break;
    }
  }
  if (cnfPath.empty()) {
    // Iterate the default_paths to check if the files exist and if so,
    // set cnfPath
    for (const auto &value : default_paths) {
      if (shcore::is_file(value)) {
        // Prompt the user to validate if shall use it or not
        console->print_info("Found configuration file at standard location: " +
                            value);

        if (console->confirm("Do you want to modify this file?") ==
            Prompt_answer::YES) {
          cnfPath = value;
          break;
        }
      }
    }
  }

  if (cnfPath.empty()) {
    console->print_info("Default file not found at the standard locations.");

    bool done = false;
    std::string tmpPath;

    while (!done && console->prompt("Please specify the path to the MySQL "
                                    "configuration file: ",
                                    &tmpPath) == shcore::Prompt_result::Ok) {
      if (tmpPath.empty()) {
        done = true;
      } else {
        if (shcore::is_file(tmpPath)) {
          cnfPath = tmpPath;
          done = true;
        } else {
          console->print_info(
              "The given path to the MySQL configuration file is invalid.");
          console->print_info();
        }
      }
    }
  }

  return cnfPath;
}

/*
 * Generates a prompt with several options
 *
 * @param options a vector of strings containing the optional selections
 * @param defopt the default option
 *
 * @return an integer representing the chosen option
 */
int prompt_menu(const std::vector<std::string> &options, int defopt) {
  int i = 0;
  auto console = mysqlsh::current_console();
  for (const auto &opt : options) {
    console->print_info(std::to_string(++i) + ") " + opt);
  }
  for (;;) {
    std::string result;
    if (defopt > 0) {
      console->print_info();
      if (console->prompt(
              "Please select an option [" + std::to_string(defopt) + "]: ",
              &result) != shcore::Prompt_result::Ok)
        return 0;
    } else {
      console->print_info();
      if (console->prompt("Please select an option: ", &result) !=
          shcore::Prompt_result::Ok)
        return 0;
    }
    // Note that menu options start at 1, not 0 since that's what users will
    // input
    if (result.empty() && defopt > 0) return defopt;
    std::stringstream ss(result);
    ss >> i;
    if (i <= 0 || i > static_cast<int>(options.size())) continue;
    break;
  }
  return i;
}

/*
 * Check if super_read_only is enable and prompts the user to act on it:
 * disable it or not.
 *
 * @param instance Instance object which represents the target instance
 * @param throw_on_error boolean value to indicate if an exception shall be
 * thrown or nor in case of an error
 *
 * @return a boolean value indicating the result of the operation: canceled or
 * not
 */
bool prompt_super_read_only(const mysqlshdk::mysql::IInstance &instance,
                            bool throw_on_error) {
  // Get the status of super_read_only in order to verify if we need to
  // prompt the user to disable it
  if (auto super_read_only = instance.get_sysvar_bool("super_read_only", false);
      !super_read_only) {
    return true;
  }

  auto options_session = instance.get_connection_options();
  auto active_session_address =
      options_session.as_uri(mysqlshdk::db::uri::formats::only_transport());

  auto console = mysqlsh::current_console();
  console->print_para(
      "The MySQL instance at '" + active_session_address +
      "' currently has the super_read_only system variable set to "
      "protect it from inadvertent updates from applications.\n"
      "You must first unset it to be able to perform any changes "
      "to this instance.\n"
      "For more information see: https://dev.mysql.com/doc/refman/"
      "en/server-system-variables.html#sysvar_super_read_only.");

  // Get the list of open session to the instance
  std::vector<std::pair<std::string, int>> open_sessions;
  open_sessions = mysqlsh::dba::get_open_sessions(instance);

  if (!open_sessions.empty()) {
    console->print_note("There are open sessions to '" +
                        active_session_address + "'.");
    console->print_info(
        "You may want to kill these sessions to prevent them from "
        "performing unexpected updates: \n");

    for (const auto &[account, num_open_sessions] : open_sessions) {
      console->print_info(shcore::str_format("%d open session(s) of '%s'. \n",
                                             num_open_sessions,
                                             account.c_str()));
    }
  }

  if (console->confirm("Do you want to disable super_read_only and continue?",
                       Prompt_answer::NO) != Prompt_answer::NO) {
    return true;
  }

  console->print_info();

  if (throw_on_error) throw shcore::cancelled("Cancelled");
  console->print_info("Cancelled");

  return false;
}

/*
 * Prints a table with configurable contents
 *
 * @param column_names the names of the table columns
 * @param column_labels the labels for the columns
 * @param documents a Value::Array with the table contents
 */
void dump_table(const std::vector<std::string> &column_names,
                const std::vector<std::string> &column_labels,
                shcore::Value::Array_type_ref documents) {
  size_t field_count = column_names.size();

  std::vector<size_t> max_lengths;
  max_lengths.reserve(field_count);

  // Updates the max_length array with the maximum length between column name,
  // min column length and column max length
  for (size_t field_index = 0; field_index < field_count; field_index++) {
    max_lengths.push_back(0);
    max_lengths[field_index] = std::max<size_t>(
        max_lengths[field_index], column_labels[field_index].size());
  }

  // Now updates the length with the real column data lengths
  if (documents) {
    for (auto map : *documents) {
      auto document = map.as_map();
      for (size_t field_index = 0; field_index < field_count; field_index++)
        max_lengths[field_index] = std::max<size_t>(
            max_lengths[field_index],
            document->get_string(column_names[field_index]).size());
    }
  }

  //-----------

  std::vector<std::string> formats(field_count, "%-");

  // Calculates the max column widths and constructs the separator line.
  std::string separator("+");
  for (size_t index = 0; index < field_count; index++) {
    // Creates the format string to print each field
    formats[index].append(std::to_string(max_lengths[index]));
    if (index == field_count - 1)
      formats[index].append("s |");
    else
      formats[index].append("s | ");

    separator.append(max_lengths[index] + 2, '-');
    separator.append(1, '+');
  }
  separator.append("\n");

  // Prints the initial separator line and the column headers
  auto console = mysqlsh::current_console();
  console->print(separator);
  console->print("| ");

  for (size_t index = 0; index < field_count; index++) {
    std::string data = shcore::str_format(formats[index].c_str(),
                                          column_labels[index].c_str());
    console->print(data.c_str());

    // Once the header is printed, updates the numeric fields formats
    // so they are right aligned
    // if (numerics[index])
    //  formats[index] = formats[index].replace(1, 1, "");
  }
  console->print_info();
  console->print(separator);

  if (documents) {
    // Now prints the records
    for (auto map : *documents) {
      auto document = map.as_map();

      console->print("| ");

      for (size_t field_index = 0; field_index < field_count; field_index++) {
        std::string raw_value = document->get_string(column_names[field_index]);
        std::string data =
            shcore::str_format(formats[field_index].c_str(), raw_value.c_str());

        console->print(data.c_str());
      }
      console->print_info();
    }
  }

  console->print_info(separator);
}

/*
 * Gets the configure instance action returned from MP
 *
 * @param opt_map map of the "config_errors" JSON dictionary retured by MP:
 * result->get_array("config_errors");
 *
 * @return an enum object of ConfigureInstanceAction with the action to be taken
 */
ConfigureInstanceAction get_configure_instance_action(
    const shcore::Value::Map_type &opt_map) {
  std::string action = opt_map.get_string("action");
  ConfigureInstanceAction ret_val;

  if (action == "server_update+config_update")
    ret_val = ConfigureInstanceAction::UPDATE_SERVER_AND_CONFIG_DYNAMIC;
  else if (action == "config_update+restart")
    ret_val = ConfigureInstanceAction::UPDATE_SERVER_AND_CONFIG_STATIC;
  else if (action == "config_update")
    ret_val = ConfigureInstanceAction::UPDATE_CONFIG;
  else if (action == "server_update")
    ret_val = ConfigureInstanceAction::UPDATE_SERVER_DYNAMIC;
  else if (action == "server_update+restart")
    ret_val = ConfigureInstanceAction::UPDATE_SERVER_STATIC;
  else if (action == "remove_opt+restart")
    ret_val = ConfigureInstanceAction::REMOVE_OPTION_RESTART;
  else if (action == "restart")
    ret_val = ConfigureInstanceAction::RESTART;
  else
    ret_val = ConfigureInstanceAction::UNDEFINED;

  return ret_val;
}

/*
 * Presents the required configuration changes and prompts the user for
 * confirmation
 *
 * @param result the return value of the MP operation
 * @param print_note a boolean value indicating if the column 'note' should be
 * presented or not
 */
void print_validation_results(const shcore::Value::Map_type_ref &result,
                              bool print_note) {
  if (!result->has_key("config_errors")) return;

  auto console = mysqlsh::current_console();
  console->print_note("Some configuration options need to be fixed:");

  auto config_errors = result->get_array("config_errors");

  if (print_note) {
    for (auto option : *config_errors) {
      auto opt_map = option.as_map();
      ConfigureInstanceAction action = get_configure_instance_action(*opt_map);
      std::string note;
      switch (action) {
        case ConfigureInstanceAction::UPDATE_SERVER_AND_CONFIG_DYNAMIC: {
          note = "Update the server variable and the config file";
          break;
        }
        case ConfigureInstanceAction::UPDATE_SERVER_AND_CONFIG_STATIC: {
          note = "Update the config file and restart the server";
          break;
        }
        case ConfigureInstanceAction::UPDATE_CONFIG: {
          note = "Update the config file";
          break;
        }
        case ConfigureInstanceAction::UPDATE_SERVER_DYNAMIC: {
          note = "Update the server variable";
          break;
        }
        case ConfigureInstanceAction::UPDATE_SERVER_STATIC: {
          note = "Update read-only variable and restart the server";
          break;
        }
        case ConfigureInstanceAction::REMOVE_OPTION_RESTART: {
          note = "Remove the option and restart the server";
          break;
        }
        case ConfigureInstanceAction::RESTART: {
          note = "Restart the server";
          break;
        }
        default: {
          note = "Undefined action";
          break;
        }
      }
      (*opt_map)["note"] = shcore::Value(std::move(note));
    }

    dump_table({"option", "current", "required", "note"},
               {"Variable", "Current Value", "Required Value", "Note"},
               config_errors);
  } else {
    dump_table({"option", "current", "required"},
               {"Variable", "Current Value", "Required Value"}, config_errors);
  }

  for (auto option : *config_errors) {
    auto opt_map = option.as_map();
    opt_map->erase("note");
  }
}

std::string get_report_host_address(
    const mysqlshdk::db::Connection_options &cnx_opts,
    const mysqlshdk::db::Connection_options &group_cnx_opts) {
  // Set login credentials to connect to instance if needed.
  // NOTE: It is assumed that the same login credentials can be used to
  //       connect to all cluster instances.
  auto target_cnx_opts = cnx_opts;
  if (!target_cnx_opts.has_user()) {
    target_cnx_opts.set_login_options_from(group_cnx_opts);
  }

  std::string instance_address =
      target_cnx_opts.as_uri(mysqlshdk::db::uri::formats::only_transport());

  // Connect to instance to try to get report_host value.
  std::string md_address;
  log_debug("Connecting to instance '%s' to determine report host.",
            instance_address.c_str());
  try {
    auto target_instance = mysqlsh::dba::Instance::connect_raw(target_cnx_opts);
    log_debug("Successfully connected to instance");

    // Get the instance report host value.
    md_address = target_instance->get_canonical_address();

    log_debug("Closed connection to the instance '%s'.",
              instance_address.c_str());
  } catch (const std::exception &err) {
    log_debug("Failed to connect to instance '%s': %s",
              instance_address.c_str(), err.what());
  }

  return (!md_address.empty()) ? md_address : instance_address;
}

std::unique_ptr<mysqlshdk::config::Config> create_server_config(
    mysqlshdk::mysql::IInstance *instance,
    const std::string &srv_cfg_handler_name, bool silent) {
  auto cfg = std::make_unique<mysqlshdk::config::Config>();

  // Get the capabilities to use set persist by the server.
  std::optional<bool> can_set_persist = instance->is_set_persist_supported();

  // Add server configuration handler depending on SET PERSIST support.
  cfg->add_handler(
      srv_cfg_handler_name,
      std::unique_ptr<mysqlshdk::config::IConfig_handler>(
          std::make_unique<mysqlshdk::config::Config_server_handler>(
              instance, can_set_persist.value_or(false)
                            ? mysqlshdk::mysql::Var_qualifier::PERSIST
                            : mysqlshdk::mysql::Var_qualifier::GLOBAL)));

  if (!silent) {
    if (!can_set_persist.has_value()) {
      auto console = mysqlsh::current_console();

      std::string warn_msg =
          "Instance '" + instance->descr() +
          "' cannot persist Group Replication configuration since MySQL "
          "version " +
          instance->get_version().get_base() +
          " does not support the SET PERSIST command (MySQL version >= 8.0.11 "
          "required). Please use the dba.<<<configureLocalInstance>>>"
          "() command locally to persist the changes.";
      console->print_warning(warn_msg);
    } else if (*can_set_persist == false) {
      auto console = mysqlsh::current_console();

      std::string warn_msg =
          "Instance '" + instance->descr() +
          "' will not load the persisted cluster configuration upon reboot "
          "since "
          "'persisted-globals-load' is set to 'OFF'. Please use the dba."
          "<<<configureLocalInstance>>>() command locally to persist the "
          "changes "
          "or set 'persisted-globals-load' to 'ON' on the configuration file.";
      console->print_warning(warn_msg);
    }
  }

  return cfg;
}

void add_config_file_handler(mysqlshdk::config::Config *cfg,
                             const std::string &handler_name,
                             const std::string &server_uuid,
                             const std::string &mycnf_path,
                             const std::string &output_mycnf_path) {
  // Add configuration handle to update option file (if provided).
  if (mycnf_path.empty() && output_mycnf_path.empty()) {
    throw std::logic_error("No option file path was provided.");
  }
  if (output_mycnf_path.empty()) {
    // Read and update mycnf.
    cfg->add_handler(handler_name,
                     std::unique_ptr<mysqlshdk::config::IConfig_handler>(
                         new mysqlshdk::config::Config_file_handler(
                             server_uuid, mycnf_path, mycnf_path)));
  } else if (mycnf_path.empty()) {
    // Update output_mycnf (creating it if needed).
    cfg->add_handler(handler_name,
                     std::unique_ptr<mysqlshdk::config::IConfig_handler>(
                         new mysqlshdk::config::Config_file_handler(
                             server_uuid, output_mycnf_path)));
  } else {
    // Read from mycnf but update output_mycnf (creating it if needed).
    cfg->add_handler(handler_name,
                     std::unique_ptr<mysqlshdk::config::IConfig_handler>(
                         new mysqlshdk::config::Config_file_handler(
                             server_uuid, mycnf_path, output_mycnf_path)));
  }
}

std::string resolve_gr_local_address(
    const std::optional<std::string> &local_address,
    const std::optional<std::string> &communication_stack,
    const std::string &raw_report_host, int port, bool check_if_busy,
    bool quiet) {
  assert(!raw_report_host.empty());  // First we need to get the report host.
  bool generated = false;
  bool comm_stack_mysql = shcore::str_caseeq(communication_stack.value_or(""),
                                             kCommunicationStackMySQL);

  // if report host is an IPv6 address, surround it with []
  // The [] are necessary for IPv6 because we later have to append a colon and
  // the port and we need to distinguish between colon in the Ip address and the
  // colon that separates the IP and the port.
  std::string report_host = raw_report_host;
  if (mysqlshdk::utils::Net::is_ipv6(raw_report_host)) {
    report_host = "[" + raw_report_host + "]";
  }

  std::string local_host;
  int local_port;

  if (!local_address.has_value() || local_address->empty()) {
    // If communicationStack is set to 'mysql' local_address must default to the
    // network address on which MySQL is listening to: report_host:port
    if (comm_stack_mysql) {
      local_host = report_host;
      local_port = port;
    } else {
      // No local address specified, use instance host and port * 10 + 1.
      local_host = report_host;
      local_port = port * 10 + 1;
      generated = true;
    }

    if (!quiet) {
      auto console = mysqlsh::current_console();

      console->print_note(
          "Group Replication will communicate with other members using '" +
          local_host + ":" + std::to_string(local_port) +
          "'. Use the localAddress option to override.");
      console->print_info();
    }
  } else if (local_address->front() == '[' && local_address->back() == ']') {
    // It is an IPV6 address (no port specified), since it is surrounded by [].
    // NOTE: Must handle this situation first to avoid splitting IPv6 addresses
    //      in the middle next.
    local_host = *local_address;
    local_port = port * 10 + 1;
  } else {
    // Parse the given address host:port (both parts are optional).
    // Note: Get the last ':' in case a IPv6 address is used, e.g. [::1]:123.
    size_t pos = local_address->find_last_of(":");
    if (pos != std::string::npos) {
      // Local address with ':' separating host and port.
      local_host = local_address->substr(0, pos);
      std::string str_local_port =
          local_address->substr(pos + 1, local_address->length());

      // No host part, use instance report host.
      if (local_host.empty()) {
        local_host = report_host;
      }

      // No port part, then use instance port * 10 +1.
      if (str_local_port.empty()) {
        local_port = port * 10 + 1;
        generated = true;
      } else {
        // Convert port string value to int
        try {
          local_port = std::stoi(str_local_port);
        } catch (const std::exception &) {
          // Error if the port cannot be converted to an integer (not valid).
          throw shcore::Exception::argument_error(
              "Invalid port '" + str_local_port +
              "' for localAddress option. The port must be an integer between "
              "1 and 65535.");
        }
      }
    } else {
      // No separator found ':'.
      // First, if the value only has digits assume it is a port.
      if (std::all_of(local_address->cbegin(), local_address->cend(),
                      ::isdigit)) {
        local_port = std::stoi(*local_address);
        local_host = report_host;
      } else {
        // Otherwise, assume only the host part was provided.
        local_host = *local_address;
        local_port = port * 10 + 1;
        generated = true;
      }
    }
  }

  // Check the port value, if out of range then issue an error.
  if (local_port <= 0 || local_port > k_max_port) {
    std::string msg;

    if (generated) {
      msg =
          "Automatically generated port for localAddress falls out of valid "
          "range.";
    } else {
      msg = "Invalid port '" + std::to_string(local_port) +
            "' for localAddress option.";
    }

    msg += " The port must be an integer between 1 and 65535.";

    if (generated) {
      msg +=
          " Please use the localAddress option to manually set a valid value.";
    }

    throw shcore::Exception::argument_error(msg);
  }

  // Verify if the port is already in use, skip if the communication protocol is
  // "MySQL"
  if (check_if_busy && !comm_stack_mysql) {
    bool port_busy = false;
    try {
      port_busy =
          mysqlshdk::utils::Net::is_port_listening(local_host, local_port);
    } catch (...) {
      // Ignore any error while checking if the port is busy, let GR try later
      // and possibly fail (e.g., if a wrong host is used).
    }
    if (port_busy) {
      throw shcore::Exception::runtime_error(
          "The port '" + std::to_string(local_port) +
          "' for localAddress option is already in use. Specify an "
          "available port to be used with localAddress option or free port '" +
          std::to_string(local_port) + "'.");
    }
  }

  // Return the final local address value to use for GR.
  return local_host + ":" + std::to_string(local_port);
}

std::vector<Instance_gtid_info> filter_primary_candidates(
    const mysqlshdk::mysql::IInstance &server,
    const std::vector<Instance_gtid_info> &gtid_info,
    const std::function<bool(const Instance_gtid_info &,
                             const Instance_gtid_info &)> &on_conflit) {
  using mysqlshdk::mysql::Gtid_set_relation;

  if (gtid_info.empty()) return {};

  std::vector<Instance_gtid_info> candidates;

  const Instance_gtid_info *freshest_instance = &gtid_info[0];
  candidates.push_back(gtid_info[0]);

  for (const auto &inst : gtid_info) {
    if (freshest_instance == &inst) continue;  // ignore the first

    auto rel = mysqlshdk::mysql::compare_gtid_sets(
        server, freshest_instance->gtid_executed, inst.gtid_executed);

    switch (rel) {
      // Conflicting GTID sets
      case Gtid_set_relation::DISJOINT:
      case Gtid_set_relation::INTERSECTS:
        if (!on_conflit)
          throw shcore::Exception("Conflicting transaction sets between " +
                                      freshest_instance->server + " and " +
                                      inst.server,
                                  SHERR_DBA_DATA_ERRANT_TRANSACTIONS);

        if (!on_conflit(*freshest_instance, inst)) return candidates;
        break;

      case Gtid_set_relation::CONTAINED:
        // this has more transactions than the best candidate
        candidates.clear();
        candidates.push_back(inst);
        freshest_instance = &inst;
        break;

      case Gtid_set_relation::EQUAL:
        // this has the same tx set as the best candidate(s)
        candidates.push_back(inst);
        break;

      case Gtid_set_relation::CONTAINS:
        // this has fewer transactions than the best candidate
        break;
    }
  }

  return candidates;
}

void validate_replication_channel_startup_status(
    const mysqlshdk::mysql::Replication_channel &channel, bool *out_io_on,
    bool *out_applier_on) {
  using mysqlshdk::mysql::Replication_channel;

  if (out_io_on) *out_io_on = false;
  if (out_applier_on) *out_applier_on = false;

  // 1st check connection thread
  if (channel.receiver.state == Replication_channel::Receiver::ON) {
    // channel is ok, now go check other stuff
    if (out_io_on) *out_io_on = true;
  } else {
    // channel is stopped, not good
    if (channel.receiver.last_error.code != 0) {
      if (channel.receiver.last_error.code == ER_ACCESS_DENIED_ERROR) {
        current_console()->print_error(
            "Authentication error in replication channel '" +
            channel.channel_name +
            "': " + mysqlshdk::mysql::to_string(channel.receiver.last_error));

        throw shcore::Exception("Authentication error in replication channel",
                                SHERR_DBA_REPLICATION_AUTH_ERROR);
      } else {
        current_console()->print_error(
            "Receiver error in replication channel '" + channel.channel_name +
            "': " + mysqlshdk::mysql::to_string(channel.receiver.last_error));

        throw shcore::Exception("Error found in replication receiver thread",
                                SHERR_DBA_REPLICATION_CONNECT_ERROR);
      }
    }
  }

  // then check coordinator thread, if there's one
  if (channel.coordinator.state != Replication_channel::Coordinator::NONE) {
    if (channel.coordinator.state == Replication_channel::Coordinator::ON) {
      // ok, check next
    } else {
      if (channel.coordinator.last_error.code != 0) {
        current_console()->print_error(
            "Coordinator error in replication channel '" +
            channel.channel_name + "': " +
            mysqlshdk::mysql::to_string(channel.coordinator.last_error));

        throw shcore::Exception("Error found in replication coordinator thread",
                                SHERR_DBA_REPLICATION_COORDINATOR_ERROR);
      }
    }
  }

  // finally, check applier threads
  for (const auto &applier : channel.appliers) {
    if (applier.state == Replication_channel::Applier::ON) {
      // ok, check next
      if (out_applier_on) *out_applier_on = true;
    } else {
      if (applier.last_error.code != 0) {
        current_console()->print_error(
            "Applier error in replication channel '" + channel.channel_name +
            "': " + mysqlshdk::mysql::to_string(applier.last_error));

        throw shcore::Exception("Error found in replication applier thread",
                                SHERR_DBA_REPLICATION_APPLIER_ERROR);
      }
    }
  }
}

// TODO(alfredo): check GR recovery channel after instance join with this
void check_replication_startup(const mysqlshdk::mysql::IInstance &instance,
                               const std::string &channel_name) {
  using mysqlshdk::mysql::Replication_channel;

  Replication_channel channel;

  log_debug("%s: waiting for replication i/o thread for channel %s",
            instance.descr().c_str(), channel_name.c_str());

  // This checks whether the slave starts correctly
  try {
    channel = mysqlshdk::mysql::wait_replication_done_connecting(instance,
                                                                 channel_name);
  } catch (shcore::Error &e) {
    throw shcore::Exception::mysql_error_with_code(e.what(), e.code());
  }
  log_debug("%s", mysqlshdk::mysql::format_status(channel, true).c_str());

  bool io_on = false;
  bool sql_on = false;

  switch (channel.status()) {
    case Replication_channel::CONNECTING:
      throw shcore::Exception("Timeout waiting for replication to start",
                              SHERR_DBA_REPLICATION_START_TIMEOUT);

    case Replication_channel::ON:
      break;

    default:
      try {
        validate_replication_channel_startup_status(channel, &io_on, &sql_on);
      } catch (const shcore::Exception &e) {
        throw shcore::Exception(instance.descr() + ": " + e.what(), e.code());
      }

      if (!io_on || !sql_on) {
        throw shcore::Exception("Replication thread not in expected state",
                                SHERR_DBA_REPLICATION_START_ERROR);
      }
      break;
  }
}

bool wait_for_gtid_set_safe(const mysqlshdk::mysql::IInstance &target_instance,
                            const std::string &gtid_set,
                            const std::string &channel_name, int timeout,
                            bool cancelable) {
  size_t total_transactions_primary, missing_transactions;
  // The GTID wait will not abort if an error occurs, so calling it without a
  // timeout will just freeze forever. To prevent that, we call the wait with
  // smaller incremental timeouts and wait for errors during that.

  // wait for at most 10s at a time
  int incremental_timeout = std::min(timeout, 10);
  if (incremental_timeout == 0) incremental_timeout = 2;

  int time_elapsed = 0;

  bool stop = false;

  shcore::Interrupt_handler intr(
      [&stop]() {
        stop = true;
        return true;
      },
      !cancelable);

  bool sync_res;

  // Get the Progress_reporting style option in use
  auto progress_reporting = current_shell_options()->get().progress_reporting;
  std::unique_ptr<mysqlshdk::textui::Progress_vt100> progress_bar;

  const auto update_progress = [](mysqlshdk::textui::Progress_vt100 *bar,
                                  uint64_t total, size_t missing) {
    bar->set_current(total - missing);
    bar->update();
  };

  // Get the total count of GTID_EXECUTED from the primary
  auto gtid_set_primary =
      mysqlshdk::mysql::Gtid_set::from_normalized_string(gtid_set);
  total_transactions_primary = gtid_set_primary.count();

  using Progress_reporting = Shell_options::Storage::Progress_reporting;

  if (progress_reporting == Progress_reporting::PROGRESSBAR) {
    // Init the progress bar
    progress_bar = std::make_unique<mysqlshdk::textui::Progress_vt100>(0);
    progress_bar->set_label("** Transactions replicated");
    progress_bar->set_total(total_transactions_primary);
    progress_bar->start();

    // Always terminate the progress bar when going out of the scope of the
    // function. That must happen regardless if the function ended successfully
    // or not, or because of an exception, to ensure the output is cleared out.
    shcore::Scoped_callback end_progress_bar(
        [&progress_bar]() { progress_bar->end(); });

    // Update the progress bar to show the current state of the transactions
    // missing
    auto gtid_set_target =
        mysqlshdk::mysql::Gtid_set::from_gtid_executed(target_instance);
    missing_transactions = mysqlshdk::mysql::estimate_gtid_set_size(
        gtid_set_primary.subtract(gtid_set_target, target_instance));

    update_progress(progress_bar.get(), total_transactions_primary,
                    missing_transactions);
  }

  do {
    // Every minute, get the executed gtid_set at the target instance to check
    // how many transactions are still missing to provide that info to the user
    if (time_elapsed != 0 && time_elapsed % 60 == 0) {
      auto gtid_set_target =
          mysqlshdk::mysql::Gtid_set::from_gtid_executed(target_instance);

      missing_transactions = mysqlshdk::mysql::estimate_gtid_set_size(
          gtid_set_primary.subtract(gtid_set_target, target_instance));

      switch (progress_reporting) {
        case Progress_reporting::PROGRESSBAR: {
          update_progress(progress_bar.get(), total_transactions_primary,
                          missing_transactions);
          break;
        }
        case Progress_reporting::SIMPLE: {
          // Simple progress reporting, just print info about the transactions
          // left
          mysqlsh::current_console()->print_info(
              "** " + std::to_string(missing_transactions) + " of " +
              std::to_string(total_transactions_primary) +
              " transactions left");
          break;
        }
        default:
          // Nothing to do
          break;
      }
    }

    sync_res = mysqlshdk::mysql::wait_for_gtid_set(target_instance, gtid_set,
                                                   incremental_timeout);
    if (!sync_res) {
      time_elapsed += incremental_timeout;

      // check for replication errors if the sync timed out
      // the check function will throw an exception if so.
      // both i/o and applier errors are checked
      check_replication_startup(target_instance, channel_name);

      // continue waiting if there were no errors
    } else {
      // wait succeeded, get out of here
      if (progress_reporting == Progress_reporting::PROGRESSBAR) {
        update_progress(progress_bar.get(), total_transactions_primary, 0);
      }
      break;
    }
  } while ((timeout == 0 || time_elapsed < timeout) && !stop);

  if (stop) throw cancel_sync();

  return sync_res;
}

void wait_for_apply_retrieved_trx(
    const mysqlshdk::mysql::IInstance &target_instance,
    const std::string &channel_name, std::chrono::seconds timeout,
    bool silent) {
  log_info("Waiting %d seconds for received transactions to be applied at '%s'",
           static_cast<int>(timeout.count()), target_instance.descr().c_str());

  DBUG_EXECUTE_IF("dba_wait_for_apply_retrieved_trx_timeout", {
    throw shcore::Exception(
        shcore::str_format(
            "Timeout waiting for received transactions to be applied on "
            "instance '%s' (debug)",
            target_instance.descr().c_str()),
        SHERR_DBA_GTID_SYNC_TIMEOUT);
  });

  std::string gtid_set =
      mysqlshdk::mysql::get_received_gtid_set(target_instance, channel_name);

  // optimize (common?) case where the GTIDs have already been applied
  if (mysqlshdk::mysql::test_for_gtid_set(
          target_instance, mysqlshdk::mysql::Gtid_set::from_string(gtid_set)))
    return;

  if (!silent) {
    auto console = mysqlsh::current_console();
    console->print_info(shcore::str_format(
        "** Waiting for received transactions to be applied at '%s'...",
        target_instance.descr().c_str()));
  }

  try {
    if (!mysqlshdk::mysql::wait_for_gtid_set(target_instance, gtid_set,
                                             timeout.count())) {
      throw shcore::Exception(
          shcore::str_format(
              "Timeout waiting for received transactions to be applied on "
              "instance '%s'",
              target_instance.descr().c_str()),
          SHERR_DBA_GTID_SYNC_TIMEOUT);
    }
  } catch (const shcore::Error &e) {
    throw shcore::Exception::mysql_error_with_code(e.what(), e.code());
  }
}

void execute_script(const std::shared_ptr<Instance> &group_server,
                    const std::string &script, const std::string &context) {
  auto console = mysqlsh::current_console();
  bool first_error = true;

  mysqlshdk::mysql::execute_sql_script(
      *group_server, script,
      [&console, &first_error, &context](std::string_view err) {
        if (first_error) {
          console->print_error(context);
          first_error = false;
        }
        console->print_error(std::string{err});
      });
}

void handle_deprecated_option(const std::string &deprecated_name,
                              const std::string &new_name, bool already_set,
                              bool fall_back_to_new_option,
                              const std::string &additional_info) {
  assert(!deprecated_name.empty());

  std::string message;

  if (!new_name.empty()) {
    if (already_set) {
      // If the value is already set (the new name was already read) then errors
      // out
      throw shcore::Exception::argument_error(
          "Cannot use the " + deprecated_name + " and " + new_name +
          " options simultaneously. The " + deprecated_name +
          " option is deprecated, please use the " + new_name +
          " option instead.");
    } else {
      // Otherwise just prints a warning
      if (fall_back_to_new_option) {
        message = "The " + deprecated_name +
                  " option is deprecated in favor of " + new_name + ". " +
                  new_name + " will be set instead.";
      } else {
        message = "The " + deprecated_name +
                  " option is deprecated. Please use the " + new_name +
                  " option instead.";
      }
    }
  } else {
    message = "The " + deprecated_name +
              " option is deprecated and will be removed in a future release.";
  }

  if (!additional_info.empty()) {
    message += " " + additional_info;
  }

  auto console = current_console();
  console->print_warning(message);
  console->print_info();
}

TargetType::Type get_instance_type(
    const MetadataStorage &metadata,
    const mysqlshdk::mysql::IInstance &target_instance) {
  Cluster_type cluster_type = Cluster_type::NONE;
  Replica_type replica_type = Replica_type::NONE;
  bool has_metadata = false;
  bool gr_active;

  try {
    mysqlshdk::utils::Version md_version;
    has_metadata =
        metadata.check_metadata(&md_version, &cluster_type, &replica_type);

    gr_active = mysqlshdk::gr::is_active_member(target_instance);
  } catch (const shcore::Error &error) {
    auto e =
        shcore::Exception::mysql_error_with_code(error.what(), error.code());

    log_warning("Error querying GR member state: %s: %i %s",
                target_instance.descr().c_str(), error.code(), error.what());

    if (error.code() == ER_NO_SUCH_TABLE) {
      gr_active = false;
    } else if (error.code() == ER_TABLEACCESS_DENIED_ERROR) {
      throw std::runtime_error(shcore::str_format(
          "Unable to detect state for instance '%s'. Please check account "
          "privileges.",
          target_instance.get_canonical_address().c_str()));
    } else {
      throw shcore::Exception::mysql_error_with_code(error.what(),
                                                     error.code());
    }
  }

  if (has_metadata) {
    // Check if it is a Read-Replica
    if (replica_type == Replica_type::READ_REPLICA) {
      return TargetType::InnoDBCluster;
    }

    if (cluster_type == Cluster_type::GROUP_REPLICATION && gr_active)
      return TargetType::InnoDBCluster;

    if (cluster_type == Cluster_type::GROUP_REPLICATION && !gr_active) {
      // InnoDB cluster but with GR stopped
      if (metadata.check_cluster_set(&target_instance)) {
        return TargetType::InnoDBClusterSetOffline;
      }

      return TargetType::StandaloneInMetadata;
    }

    if (gr_active) {
      // GR running but instance is not in the metadata, could be:
      // - member was added to the cluster by hand
      // - UUID of member changed
      // - MD in the group belongs to a different cluster
      // - others
      if (cluster_type != Cluster_type::NONE) {
        log_warning(
            "Instance %s is running Group Replication, but does not belong to "
            "a InnoDB cluster",
            target_instance.descr().c_str());
      }

      return TargetType::GroupReplication;
    }

    if (cluster_type == Cluster_type::ASYNC_REPLICATION)
      return TargetType::AsyncReplicaSet;

    return TargetType::StandaloneWithMetadata;
  }

  if (gr_active) return TargetType::GroupReplication;

  try {
    auto channels = mysqlshdk::mysql::get_incoming_channels(target_instance);

    // filter out GR channels, since they don't indicate AR is configured
    channels.erase(
        std::remove_if(
            channels.begin(), channels.end(),
            [](const mysqlshdk::mysql::Replication_channel &ch) {
              return ch.channel_name == mysqlshdk::gr::k_gr_applier_channel ||
                     ch.channel_name == mysqlshdk::gr::k_gr_recovery_channel;
            }),
        channels.end());

    if (!channels.empty()) return TargetType::AsyncReplication;

    auto slaves = mysqlshdk::mysql::get_slaves(target_instance);
    if (!slaves.empty()) return TargetType::AsyncReplication;
  } catch (const shcore::Error &error) {
    switch (error.code()) {
      case ER_TABLEACCESS_DENIED_ERROR:
      case ER_SPECIFIC_ACCESS_DENIED_ERROR:
        throw std::runtime_error(shcore::str_format(
            "Unable to detect state for instance '%s'. Please check account "
            "privileges.",
            target_instance.get_canonical_address().c_str()));
      default:
        throw shcore::Exception::mysql_error_with_code(error.what(),
                                                       error.code());
    }
  }

  return TargetType::Standalone;
}

Cluster_check_info get_cluster_check_info(const MetadataStorage &metadata,
                                          Instance *group_server,
                                          bool skip_version_check) {
  Cluster_check_info state;

  auto group_instance =
      group_server ? group_server : metadata.get_md_server().get();

  // Retrieves the instance configuration type from the perspective of the
  // active session
  try {
    state.source_type = get_instance_type(metadata, *group_instance);
  } catch (const shcore::Exception &e) {
    if (mysqlshdk::db::is_server_connection_error(e.code())) {
      throw shcore::Exception::mysql_error_with_code(
          group_instance->descr() + ": " + e.what(), e.code());
    } else {
      log_warning("Error detecting GR instance: %s", e.what());
      state.source_type = TargetType::Unknown;
    }
  }

  // If it is a GR instance, validates the instance state
  if (state.source_type == TargetType::GroupReplication ||
      state.source_type == TargetType::InnoDBCluster) {
    validate_gr_session(group_instance->get_session());

    // Retrieves the instance cluster statues from the perspective of the
    // active session
    state = get_replication_group_state(*group_instance, state.source_type);

    // On IDC we want to also determine whether the quorum is just Normal or if
    // All the instances are ONLINE
    if (state.source_type == TargetType::InnoDBCluster) {
      if (state.quorum == ReplicationQuorum::States::Normal) {
        try {
          if (metadata.check_all_members_online()) {
            state.quorum |= ReplicationQuorum::States::All_online;
          }
        } catch (const shcore::Error &e) {
          log_error(
              "Error while verifying all members in InnoDB Cluster are ONLINE: "
              "%s",
              e.what());
          throw;
        }
      }

      if (metadata.check_cluster_set(group_instance)) {
        state.source_type = TargetType::InnoDBClusterSet;
      }
    }
  } else if (state.source_type == TargetType::AsyncReplicaSet) {
    auto instance = metadata.get_instance_by_uuid(group_instance->get_uuid());
    if (instance.primary_master) {
      state.source_state = ManagedInstance::OnlineRW;
    } else {
      state.source_state = ManagedInstance::OnlineRO;
    }

    state.quorum = ReplicationQuorum::States::Normal;
  } else {
    state.quorum = ReplicationQuorum::States::Normal;
    state.source_state = ManagedInstance::Offline;
  }

  if (!skip_version_check) {
    state.source_version = group_instance->get_version();
  }

  return state;
}

mysqlshdk::db::Ssl_options prepare_replica_ssl_options(
    const mysqlshdk::mysql::IInstance &instance, Cluster_ssl_mode ssl_mode,
    Replication_auth_type auth_type) {
  mysqlshdk::db::Ssl_options ssl_options;

  if (ssl_mode == Cluster_ssl_mode::VERIFY_CA ||
      ssl_mode == Cluster_ssl_mode::VERIFY_IDENTITY ||
      auth_type != Replication_auth_type::PASSWORD) {
    ssl_options = read_ssl_client_options(
        instance, ssl_mode != Cluster_ssl_mode::REQUIRED ||
                      auth_type != Replication_auth_type::PASSWORD);
  }
  switch (ssl_mode) {
    case Cluster_ssl_mode::NONE:
    case Cluster_ssl_mode::AUTO:
      assert(0);  // these are supposed to be resolved before reaching here
      [[fallthrough]];
    case Cluster_ssl_mode::DISABLED:
      ssl_options.set_mode(mysqlshdk::db::Ssl_mode::Disabled);
      break;
    case Cluster_ssl_mode::REQUIRED:
      ssl_options.set_mode(mysqlshdk::db::Ssl_mode::Required);
      break;
    case Cluster_ssl_mode::VERIFY_CA:
      ssl_options.set_mode(mysqlshdk::db::Ssl_mode::VerifyCa);
      break;
    case Cluster_ssl_mode::VERIFY_IDENTITY:
      ssl_options.set_mode(mysqlshdk::db::Ssl_mode::VerifyIdentity);
      break;
  }
  return ssl_options;
}

void validate_replication_sources_option(const shcore::Value &value) {
  if (value.type == shcore::Value_type::Array) {
    auto array = value.as_array();
    if (array->empty()) {
      throw shcore::Exception::argument_error(shcore::str_format(
          "The list for '%s' option cannot be empty.", kReplicationSources));
    }
  } else if (value.type == shcore::Value_type::String) {
    auto string = value.as_string();

    if (!shcore::str_caseeq(string, kReplicationSourcesAutoPrimary) &&
        !shcore::str_caseeq(string, kReplicationSourcesAutoSecondary)) {
      throw shcore::Exception::argument_error(shcore::str_format(
          "Option '%s' only accepts 'primary' or 'secondary' as a valid "
          "string value, otherwise a list of instances is expected.",
          kReplicationSources));
    }
  } else {
    throw shcore::Exception::argument_error(shcore::str_format(
        "The '%s' option must be a string or a list of strings.",
        kReplicationSources));
  }
}

namespace cluster_topology_executor_ops {
void validate_add_rejoin_options(Group_replication_options options,
                                 const std::string &communication_stack) {
  // ipAllowList cannot be used by addInstance/rejoinInstance when the
  // communication stack in use by the Cluster is 'MySQL'
  if (communication_stack == kCommunicationStackMySQL &&
      options.ip_allowlist.has_value()) {
    throw shcore::Exception::argument_error(shcore::str_format(
        "Cannot use '%s' when the Cluster's communication stack is "
        "'%s'",
        options.ip_allowlist_option_name.c_str(), kCommunicationStackMySQL));
  }
}

bool is_member_auto_rejoining(
    const std::shared_ptr<mysqlsh::dba::Instance> &target_instance) {
  // Check if instance was doing auto-rejoin and let the user know that the
  // rejoin operation will override the auto-rejoin
  if (mysqlshdk::gr::is_running_gr_auto_rejoin(*target_instance) ||
      mysqlshdk::gr::is_group_replication_delayed_starting(*target_instance)) {
    auto console = current_console();
    console->print_note(
        "The instance '" +
        target_instance->get_connection_options().uri_endpoint() +
        "' is running auto-rejoin process, which will be cancelled.");
    console->print_info();

    return true;
  }

  return false;
}

void ensure_not_auto_rejoining(
    const std::shared_ptr<mysqlsh::dba::Instance> &target_instance) {
  current_console()->print_note("Cancelling active GR auto-initialization at " +
                                target_instance->descr());
  mysqlshdk::gr::stop_group_replication(*target_instance);
}

void check_comm_stack_support(
    const std::shared_ptr<mysqlsh::dba::Instance> &target_instance,
    Group_replication_options *gr_options,
    const std::string &communication_stack) {
  if (communication_stack == kCommunicationStackMySQL &&
      !supports_mysql_communication_stack(target_instance->get_version())) {
    auto console = mysqlsh::current_console();
    console->print_error("Cannot join instance '" + target_instance->descr() +
                         "' to Cluster: The Group Replication protocol stack "
                         "in used by the Cluster ('" +
                         kCommunicationStackMySQL +
                         "') is not supported in the instance");

    throw shcore::Exception("Unsupported Group Replication protocol stack '" +
                                std::string(kCommunicationStackMySQL) + "'",
                            SHERR_DBA_UNSUPPORTED_COMMUNICATION_PROTOCOL);
  }

  // Set the communication stack protocol to be used only if the target
  // instance supports it
  if (supports_mysql_communication_stack(target_instance->get_version())) {
    gr_options->communication_stack = communication_stack;
  }
}

void ensure_instance_check_installed_schema_version(
    const std::shared_ptr<mysqlsh::dba::Instance> &target_instance,
    mysqlshdk::utils::Version lowest_cluster_version) {
  try {
    // Check instance version compatibility according to Group Replication.
    mysqlshdk::gr::check_instance_check_installed_schema_version(
        *target_instance, lowest_cluster_version);
  } catch (const std::runtime_error &) {
    auto console = mysqlsh::current_console();
    console->print_error(
        "Cannot join instance '" + target_instance->descr() +
        "' to cluster: instance version is incompatible with the cluster.");
    throw;
  }

  // Print a warning if the instance is only read compatible.
  if (mysqlshdk::gr::is_instance_only_read_compatible(*target_instance,
                                                      lowest_cluster_version)) {
    auto console = mysqlsh::current_console();
    console->print_warning("The instance '" + target_instance->descr() +
                           "' is only read compatible with the cluster, thus "
                           "it will join the cluster in R/O mode.");
  }
}

void log_used_gr_options(const Group_replication_options &gr_options) {
  if (gr_options.local_address.has_value() &&
      !gr_options.local_address->empty()) {
    log_info("Using Group Replication local address: %s",
             gr_options.local_address->c_str());
  }

  if (gr_options.group_seeds.has_value() && !gr_options.group_seeds->empty()) {
    log_info("Using Group Replication group seeds: %s",
             gr_options.group_seeds->c_str());
  }

  if (gr_options.exit_state_action.has_value() &&
      !gr_options.exit_state_action->empty()) {
    log_info("Using Group Replication exit state action: %s",
             gr_options.exit_state_action->c_str());
  }

  if (gr_options.member_weight.has_value()) {
    log_info("Using Group Replication member weight: %s",
             std::to_string(*gr_options.member_weight).c_str());
  }

  if (gr_options.consistency.has_value() && !gr_options.consistency->empty()) {
    log_info("Using Group Replication failover consistency: %s",
             gr_options.consistency->c_str());
  }

  if (gr_options.expel_timeout.has_value()) {
    log_info("Using Group Replication expel timeout: %s",
             std::to_string(*gr_options.expel_timeout).c_str());
  }

  if (gr_options.auto_rejoin_tries.has_value()) {
    log_info("Using Group Replication auto-rejoin tries: %s",
             std::to_string(*gr_options.auto_rejoin_tries).c_str());
  }
}

void validate_local_address_ip_compatibility(
    const std::shared_ptr<mysqlsh::dba::Instance> &target_instance,
    const std::string &local_address, const std::string &group_seeds,
    mysqlshdk::utils::Version lowest_cluster_version) {
  // local_address must have some value
  assert(!local_address.empty());

  // Validate that the group_replication_local_address is valid for the version
  // of the target instance.
  if (!mysqlshdk::gr::is_endpoint_supported_by_gr(
          local_address, target_instance->get_version())) {
    auto console = mysqlsh::current_console();
    console->print_error("Cannot join instance '" + target_instance->descr() +
                         "' to cluster: unsupported localAddress value.");
    throw shcore::Exception::argument_error(shcore::str_format(
        "Cannot use value '%s' for option localAddress because it has "
        "an IPv6 address which is only supported by Group Replication "
        "from MySQL version >= 8.0.14 and the target instance version is %s.",
        local_address.c_str(),
        target_instance->get_version().get_base().c_str()));
  }

  // Validate that the localAddress values of the cluster members are
  // compatible with the target instance we are adding
  auto local_address_list = shcore::str_split(group_seeds, ",");
  std::vector<std::string> unsupported_addresses;
  for (const auto &raw_local_addr : local_address_list) {
    std::string local_addr = shcore::str_strip(raw_local_addr);
    if (!mysqlshdk::gr::is_endpoint_supported_by_gr(
            local_addr, target_instance->get_version())) {
      unsupported_addresses.push_back(local_addr);
    }
  }
  if (!unsupported_addresses.empty()) {
    std::string value_str =
        (unsupported_addresses.size() == 1) ? "value" : "values";

    auto console = mysqlsh::current_console();
    console->print_error(
        "Cannot join instance '" + target_instance->descr() +
        "' to cluster: unsupported localAddress value on the cluster.");
    throw shcore::Exception::runtime_error(shcore::str_format(
        "Instance does not support the following localAddress %s of the "
        "cluster: '%s'. IPv6 "
        "addresses/hostnames are only supported by Group "
        "Replication from MySQL version >= 8.0.14 and the target instance "
        "version is %s.",
        value_str.c_str(),
        shcore::str_join(unsupported_addresses.cbegin(),
                         unsupported_addresses.cend(), ", ")
            .c_str(),
        target_instance->get_version().get_base().c_str()));
  }

  // validate that the value of the localAddress is supported by all
  // existing cluster members
  if (!mysqlshdk::gr::is_endpoint_supported_by_gr(local_address,
                                                  lowest_cluster_version)) {
    auto console = mysqlsh::current_console();
    console->print_error(
        "Cannot join instance '" + target_instance->descr() +
        "' to cluster: localAddress value not supported by the cluster.");
    throw shcore::Exception::argument_error(shcore::str_format(
        "Cannot use value '%s' for option localAddress because it has "
        "an IPv6 address which is only supported by Group Replication "
        "from MySQL version >= 8.0.14 but the cluster "
        "contains at least one member with version %s.",
        local_address.c_str(), lowest_cluster_version.get_base().c_str()));
  }
}

void validate_read_replica_version(
    mysqlshdk::utils::Version target_instance_version,
    mysqlshdk::utils::Version lowest_cluster_version) {
  if (target_instance_version < Precondition_checker::k_min_rr_version) {
    mysqlsh::current_console()->print_info(
        "The target instance is running MySQL version " +
        target_instance_version.get_full() + ", but " +
        Precondition_checker::k_min_rr_version.get_full() +
        " is the minimum required for InnoDB Cluster Read Replicas.");
    throw shcore::Exception("Unsupported MySQL version",
                            SHERR_DBA_BADARG_VERSION_NOT_SUPPORTED);
  }

  // Check if the Cluster's minimum version is >= 8.0.23
  if (lowest_cluster_version < Precondition_checker::k_min_rr_version) {
    mysqlsh::current_console()->print_info(
        "Detected a member in the target Cluster running MySQL server "
        "version '" +
        lowest_cluster_version.get_full() +
        "' which is older than the minimum required for InnoDB Cluster Read "
        "Replicas: '" +
        Precondition_checker::k_min_rr_version.get_full() + "'");

    throw shcore::Exception("Unsupported MySQL version",
                            SHERR_DBA_BADARG_VERSION_NOT_SUPPORTED);
  }
}

}  // namespace cluster_topology_executor_ops

}  // namespace dba
}  // namespace mysqlsh
