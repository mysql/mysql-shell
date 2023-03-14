/*
 * Copyright (c) 2018, 2023, Oracle and/or its affiliates.
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

#include "modules/adminapi/cluster/set_option.h"
#include "adminapi/common/common.h"
#include "adminapi/common/server_features.h"
#include "modules/adminapi/common/metadata_storage.h"
#include "modules/adminapi/common/validations.h"
#include "mysqlshdk/include/shellcore/console.h"
#include "mysqlshdk/libs/db/mysql/session.h"
#include "mysqlshdk/libs/mysql/clone.h"
#include "mysqlshdk/libs/utils/utils_general.h"

namespace mysqlsh {
namespace dba {
namespace cluster {

Set_option::Set_option(Cluster_impl *cluster, std::string option,
                       std::string value)
    : m_cluster(cluster),
      m_option(std::move(option)),
      m_value(std::move(value)) {
  assert(cluster);
}

Set_option::Set_option(Cluster_impl *cluster, std::string option, int64_t value)
    : m_cluster(cluster), m_option(std::move(option)), m_value(value) {
  assert(cluster);
}

Set_option::Set_option(Cluster_impl *cluster, std::string option, double value)
    : m_cluster(cluster), m_option(std::move(option)), m_value(value) {
  assert(cluster);
}

Set_option::Set_option(Cluster_impl *cluster, std::string option, bool value)
    : m_cluster(cluster), m_option(std::move(option)), m_value(value) {
  assert(cluster);
}

Set_option::Set_option(Cluster_impl *cluster, std::string option,
                       std::monostate value)
    : m_cluster(cluster), m_option(std::move(option)), m_value(value) {
  assert(cluster);
}

Set_option::~Set_option() = default;

void Set_option::ensure_replication_option_valid() const {
  if (!m_cluster->is_cluster_set_member()) {
    throw shcore::Exception::runtime_error(
        shcore::str_format("Cannot update replication option '%s' on a Cluster "
                           "that doesn't belong to a ClusterSet.",
                           m_option.c_str()));
  }

  auto option_name = std::string_view{m_option}.substr(
      std::string_view{"clusterSetReplication"}.length());

  if (option_name == kReplicationHeartbeatPeriod) {
    if (!std::holds_alternative<std::monostate>(m_value) &&
        !std::holds_alternative<double>(m_value) &&
        !std::holds_alternative<int64_t>(m_value))
      throw shcore::Exception::argument_error(shcore::str_format(
          "Invalid value for '%s': Argument #2 is expected to be a double",
          m_option.c_str()));

    if ((std::holds_alternative<int64_t>(m_value) &&
         (std::get<int64_t>(m_value) < 0)) ||
        (std::holds_alternative<double>(m_value) &&
         (std::get<double>(m_value) < 0.0)))
      throw shcore::Exception::argument_error(shcore::str_format(
          "Invalid value for '%s': Argument #2 cannot have a negative value",
          m_option.c_str()));

    return;
  }

  if ((option_name == kReplicationConnectRetry) ||
      (option_name == kReplicationRetryCount) ||
      (option_name == kReplicationZstdCompressionLevel)) {
    if (!std::holds_alternative<std::monostate>(m_value) &&
        !std::holds_alternative<int64_t>(m_value))
      throw shcore::Exception::argument_error(shcore::str_format(
          "Invalid value for '%s': Argument #2 is expected to be a integer",
          m_option.c_str()));

    if (std::holds_alternative<int64_t>(m_value) &&
        (std::get<int64_t>(m_value) < 0))
      throw shcore::Exception::argument_error(shcore::str_format(
          "Invalid value for '%s': Argument #2 cannot have a negative value",
          m_option.c_str()));

    return;
  }

  if ((option_name == kReplicationCompressionAlgorithms) ||
      (option_name == kReplicationBind) ||
      (option_name == kReplicationNetworkNamespace)) {
    if (!std::holds_alternative<std::monostate>(m_value) &&
        !std::holds_alternative<std::string>(m_value))
      throw shcore::Exception::argument_error(shcore::str_format(
          "Invalid value for '%s': Argument #2 is expected to be a string",
          m_option.c_str()));
    return;
  }

  throw shcore::Exception::argument_error(
      shcore::str_format("Option '%s' not supported.", m_option.c_str()));
}

void Set_option::ensure_option_valid() const {
  /* - Validate if the option is valid, being the accepted values:
   *     - clusterName
   *     - disableClone
   *     - exitStateAction
   *     - memberWeight
   *     - failoverConsistency
   *     - consistency
   *     - expelTimeout
   *     - replicationAllowedHost
   *     - transactionSizeLimit
   *     - ipAllowlist
   */
  if (k_global_cluster_supported_options.count(m_option) == 0 &&
      m_option != kClusterName && m_option != kDisableClone &&
      m_option != kReplicationAllowedHost) {
    throw shcore::Exception::argument_error(
        shcore::str_format("Option '%s' not supported.", m_option.c_str()));
  }

  // Verify the deprecation of failoverConsistency
  if (m_option == kFailoverConsistency) {
    mysqlsh::current_console()->print_warning(
        "The failoverConsistency option is deprecated. "
        "Please use the consistency option instead.");
    return;
  }

  if (m_option == kClusterName) {
    // Validate if the clusterName value is valid
    if (!std::holds_alternative<std::string>(m_value)) {
      throw shcore::Exception::argument_error(
          "Invalid value for 'clusterName': Argument #2 is expected to be a "
          "string.");
    }

    mysqlsh::dba::validate_cluster_name(std::get<std::string>(m_value),
                                        Cluster_type::GROUP_REPLICATION);
    return;
  }

  if (m_option == kDisableClone) {
    // Validate if the disableClone value is valid
    // Ensure disableClone is a boolean or integer value
    if (!std::holds_alternative<bool>(m_value) &&
        !std::holds_alternative<int64_t>(m_value)) {
      throw shcore::Exception::type_error(
          "Invalid value for 'disableClone': Argument #2 is expected to be a "
          "boolean.");
    }
    return;
  }

  if (m_option == kReplicationAllowedHost) {
    if (!std::holds_alternative<std::string>(m_value) ||
        std::get<std::string>(m_value).empty()) {
      throw shcore::Exception::argument_error(
          "Invalid value for 'replicationAllowedHost': Argument #2 is expected "
          "to be a string.");
    }
    return;
  }

  if (m_option == kTransactionSizeLimit &&
      (m_cluster->is_cluster_set_member() &&
       !m_cluster->is_primary_cluster())) {
    // transactionSizeLimit is not changeable in Replica Clusters (must be zero
    // and its automatically managed by the AdminAPI)
    throw shcore::Exception::runtime_error(
        "Option '" + m_option + "' not supported on Replica Clusters.");
    return;
  }

  if (m_option == kIpAllowlist) {
    if (!std::holds_alternative<std::string>(m_value))
      throw shcore::Exception::argument_error(
          "Invalid value for 'ipAllowlist': Argument #2 is expected "
          "to be a string.");

    return;
  }
}

void Set_option::store_replication_option() {
  auto option_name = std::string_view{m_option}.substr(
      std::string_view{"clusterSetReplication"}.length());

  auto metadata = m_cluster->get_metadata_storage();

  auto [value, desc] = std::visit(
      [](auto &&arg) {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, std::string>) {
          return std::make_tuple(shcore::Value(arg),
                                 shcore::str_format("'%s'", arg.c_str()));
        } else if constexpr (std::is_same_v<T, int64_t>) {
          return std::make_tuple(shcore::Value(arg),
                                 shcore::str_format("'%" PRId64 "'", arg));
        } else if constexpr (std::is_same_v<T, double>) {
          return std::make_tuple(shcore::Value(arg),
                                 shcore::str_format("'%.2f'", arg));
        } else {
          return std::make_tuple(shcore::Value::Null(), std::string{"NULL"});
        }
      },
      m_value);

  // small optimization: if the cluster is the primary and the value is NULL, we
  // can delete the attribute instead of storing NULL (because there's no
  // channel to reset)
  {
    auto attrib_name = std::invoke(
        [](std::string_view name) -> std::string_view {
          if (name == kReplicationConnectRetry)
            return k_cluster_attribute_repl_connect_retry;
          if (name == kReplicationRetryCount)
            return k_cluster_attribute_repl_retry_count;
          if (name == kReplicationHeartbeatPeriod)
            return k_cluster_attribute_repl_heartbeat_period;
          if (name == kReplicationCompressionAlgorithms)
            return k_cluster_attribute_repl_compression_algorithms;
          if (name == kReplicationZstdCompressionLevel)
            return k_cluster_attribute_repl_zstd_compression_level;
          if (name == kReplicationBind) return k_cluster_attribute_repl_bind;
          if (name == kReplicationNetworkNamespace)
            return k_cluster_attribute_repl_network_namespace;

          assert(!"Unknown replication option.");
          throw shcore::Exception::argument_error(
              shcore::str_format("Unknown replication option '%.*s'.",
                                 static_cast<int>(name.size()), name.data()));
        },
        option_name);

    if (m_cluster->is_primary_cluster() && !value)
      metadata->remove_cluster_attribute(m_cluster->get_id(), attrib_name);
    else
      metadata->update_cluster_attribute(m_cluster->get_id(), attrib_name,
                                         value, true);
  }

  auto console = current_console();
  console->print_info(shcore::str_format(
      "Successfully set the value of '%s' to %s for the '%s' cluster.",
      m_option.c_str(), desc.c_str(), m_cluster->get_name().c_str()));

  if (m_cluster->is_primary_cluster()) {
    console->print_warning(
        "The new value won't take effect while the cluster remains the "
        "primary Cluster of the ClusterSet.");
  } else {
    console->print_warning(
        "The new value won't take effect until "
        "ClusterSet.<<<rejoinCluster>>>() is called on the cluster.");
  }
}

void Set_option::check_disable_clone_support() {
  log_debug("Checking of disableClone is not supported by all cluster members");

  // Get all cluster instances
  auto members = mysqlshdk::gr::get_members(*m_cluster->get_cluster_server());

  size_t bad_count = 0;

  // Check if all instances have a version that supports clone
  for (const auto &member : members) {
    if (member.version.empty() ||
        !is_option_supported(
            mysqlshdk::utils::Version(member.version), m_option,
            {{kDisableClone,
              {"", k_mysql_clone_plugin_initial_version, {}}}})) {
      bad_count++;
    }
  }

  if (bad_count > 0) {
    throw shcore::Exception::runtime_error(
        "Option 'disableClone' not supported on Cluster.");
  }
}

void Set_option::update_disable_clone_option(bool disable_clone) {
  size_t count, cluster_size;

  // Get the cluster size (instances)
  cluster_size =
      m_cluster->get_metadata_storage()->get_cluster_size(m_cluster->get_id());

  // Enable/Disable the clone plugin on the target cluster
  count = m_cluster->setup_clone_plugin(!disable_clone);

  // If we failed to update the value in all cluster members, thrown an
  // exception
  if (count == cluster_size)
    throw shcore::Exception::runtime_error(
        "Failed to set the value of '" + m_option +
        "' in the Cluster: Unable to update the clone plugin status in all "
        "cluster members.");

  // Update disableClone value in the metadata only if we succeeded to
  // update the plugin status in at least one of the members
  m_cluster->get_metadata_storage()->update_cluster_attribute(
      m_cluster->get_id(), k_cluster_attribute_disable_clone,
      disable_clone ? shcore::Value::True() : shcore::Value::False());
}

void Set_option::connect_all_members() {
  // Get cluster session to use the same authentication credentials for all
  // cluster instances.
  Connection_options cluster_cnx_opt =
      m_cluster->get_cluster_server()->get_connection_options();

  // Check if all instances have the ONLINE state
  for (const auto &instance_def : m_cluster->get_instances_with_state()) {
    // Instance is ONLINE, initialize and populate the internal cluster
    // instances list

    // Establish a session to the instance
    // Set login credentials to connect to instance.
    // NOTE: It is assumed that the same login credentials can be used to
    // connect to all cluster instances.
    Connection_options instance_cnx_opts =
        shcore::get_connection_options(instance_def.first.endpoint, false);
    instance_cnx_opts.set_login_options_from(cluster_cnx_opt);

    log_debug("Connecting to instance '%s'.",
              instance_def.first.endpoint.c_str());

    // Establish a session to the instance
    try {
      // Add the instance to instances internal list
      m_cluster_instances.emplace_back(Instance::connect(instance_cnx_opts));
    } catch (const std::exception &err) {
      log_debug("Failed to connect to instance: %s", err.what());

      mysqlsh::current_console()->print_error(
          "Unable to connect to instance '" + instance_def.first.endpoint +
          "'. Please, verify connection credentials and make sure the "
          "instance is available.");

      throw shcore::Exception::runtime_error(err.what());
    }
  }
}

void Set_option::ensure_option_supported_all_members_cluster() const {
  log_debug("Checking if all members of the cluster support the option '%s'",
            m_option.c_str());

  for (const auto &instance : m_cluster_instances) {
    // Verify if the instance version is supported
    if (is_option_supported(instance->get_version(), m_option,
                            k_global_cluster_supported_options))
      continue;

    mysqlsh::current_console()->print_error(
        "The instance '" + instance->descr() + "' has the version " +
        instance->get_version().get_full() +
        " which does not support the option '" + m_option + "'.");

    throw shcore::Exception::runtime_error(
        "One or more instances of the cluster have a version that does not "
        "support this operation.");
  }
}

void Set_option::prepare() {
  if (shcore::str_beginswith(m_option, "clusterSetReplication")) {
    ensure_replication_option_valid();
    return;
  }

  // Verify user privileges to execute operation;
  ensure_user_privileges(*m_cluster->get_cluster_server());

  // Validate if the option is valid
  ensure_option_valid();

  // Must be able to connect to all members
  connect_all_members();

  // Verify if all cluster members support the option
  // NOTE: clusterName and disableClone do not require this validation
  if (m_option != kClusterName && m_option != kDisableClone &&
      m_option != kReplicationAllowedHost) {
    ensure_option_supported_all_members_cluster();

    // Get the Cluster Config Object
    m_cfg = m_cluster->create_config_object();

    if (m_option == kAutoRejoinTries &&
        std::holds_alternative<int64_t>(m_value) &&
        (std::get<int64_t>(m_value) != 0)) {
      auto console = mysqlsh::current_console();
      console->print_warning(
          "Each cluster member will only proceed according to its "
          "exitStateAction if auto-rejoin fails (i.e. all retry attempts are "
          "exhausted).");
      console->print_info();
    }
  }

  // If disableClone check if the target cluster supports it
  if (m_option == kDisableClone) {
    check_disable_clone_support();
  }
}

shcore::Value Set_option::execute() {
  if (shcore::str_beginswith(m_option, "clusterSetReplication")) {
    store_replication_option();
    return shcore::Value();
  }

  auto console = mysqlsh::current_console();

  if (m_option != kClusterName && m_option != kDisableClone &&
      m_option != kReplicationAllowedHost) {
    // Update the option values in all Cluster members:
    const std::string &option_gr_variable =
        k_global_cluster_supported_options.at(m_option).option_variable;

    if (std::holds_alternative<std::string>(m_value))
      console->print_info(shcore::str_format(
          "Setting the value of '%s' to '%s' in all cluster members ...",
          m_option.c_str(), std::get<std::string>(m_value).c_str()));
    else
      console->print_info(
          shcore::str_format("Setting the value of '%s' to '%" PRId64
                             "' in all cluster members ...",
                             m_option.c_str(), std::get<int64_t>(m_value)));

    console->print_info();

    if (std::holds_alternative<std::string>(m_value)) {
      m_cfg->set(option_gr_variable, std::get<std::string>(m_value));
    } else {
      m_cfg->set(option_gr_variable,
                 std::optional<int64_t>{std::get<int64_t>(m_value)});
    }

    m_cfg->apply();

    if (m_option == kTransactionSizeLimit) {
      // If changing the option in a ClusterSet, update all Clusters
      if (m_cluster->is_cluster_set_member()) {
        m_cluster->get_metadata_storage()->update_clusters_attribute(
            k_cluster_attribute_transaction_size_limit,
            shcore::Value(std::get<int64_t>(m_value)));
      } else {
        m_cluster->get_metadata_storage()->update_cluster_attribute(
            m_cluster->get_id(), k_cluster_attribute_transaction_size_limit,
            shcore::Value(std::get<int64_t>(m_value)));
      }
    }

    if (std::holds_alternative<std::string>(m_value))
      console->print_info(shcore::str_format(
          "Successfully set the value of '%s' to '%s' in the '%s' cluster.",
          m_option.c_str(), std::get<std::string>(m_value).c_str(),
          m_cluster->get_name().c_str()));
    else
      console->print_info(
          shcore::str_format("Successfully set the value of '%s' to '%" PRId64
                             "' in the '%s' cluster.",
                             m_option.c_str(), std::get<int64_t>(m_value),
                             m_cluster->get_name().c_str()));

    return shcore::Value();
  }

  if (m_option == kClusterName) {
    // copy the current names
    auto current_full_cluster_name = m_cluster->get_name();
    auto current_cluster_name = m_cluster->cluster_name();

    auto md = m_cluster->get_metadata_storage();

    console->print_info(shcore::str_format(
        "Setting the value of '%s' to '%s' in the Cluster ...",
        m_option.c_str(), std::get<std::string>(m_value).c_str()));
    console->print_info();

    std::string domain_name;
    std::string cluster_name;
    parse_fully_qualified_cluster_name(std::get<std::string>(m_value),
                                       &domain_name, nullptr, &cluster_name);

    m_cluster->set_cluster_name(cluster_name);

    try {
      md->update_cluster_name(m_cluster->get_id(), m_cluster->cluster_name());
    } catch (...) {
      // revert changes
      m_cluster->set_cluster_name(current_cluster_name);
      throw;
    }

    console->print_info(shcore::str_format(
        "Successfully set the value of '%s' to '%s' in the Cluster: '%s'.",
        m_option.c_str(), std::get<std::string>(m_value).c_str(),
        current_full_cluster_name.c_str()));

    return shcore::Value();
  }

  if (m_option == kDisableClone) {
    // Ensure forceClone is a boolean or integer value
    auto option_value = std::holds_alternative<bool>(m_value)
                            ? std::get<bool>(m_value)
                            : (std::get<int64_t>(m_value) != 0);

    console->print_info(shcore::str_format(
        "Setting the value of '%s' to '%s' in the Cluster ...",
        m_option.c_str(), option_value ? "true" : "false"));
    console->print_info();

    update_disable_clone_option(option_value);

    console->print_info(shcore::str_format(
        "Successfully set the value of '%s' to '%s' in the Cluster: '%s'.",
        m_option.c_str(), option_value ? "true" : "false",
        m_cluster->cluster_name().c_str()));

    return shcore::Value();
  }

  if (m_option == kReplicationAllowedHost) {
    m_cluster->update_replication_allowed_host(std::get<std::string>(m_value));

    m_cluster->get_metadata_storage()->update_cluster_attribute(
        m_cluster->get_id(), k_cluster_attribute_replication_allowed_host,
        shcore::Value(std::get<std::string>(m_value)));

    current_console()->print_info(shcore::str_format(
        "Internally managed GR recovery accounts updated for Cluster '%s'",
        m_cluster->get_name().c_str()));

    return shcore::Value();
  }

  return shcore::Value();
}

void Set_option::finish() {
  // Close all sessions to cluster instances.
  for (const auto &instance : m_cluster_instances) {
    instance->close_session();
  }

  // Reset all auxiliary (temporary) data used for the operation execution.
  m_option.clear();
}

}  // namespace cluster
}  // namespace dba
}  // namespace mysqlsh
