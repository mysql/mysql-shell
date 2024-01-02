/*
 * Copyright (c) 2018, 2024, Oracle and/or its affiliates.
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

#include "modules/adminapi/cluster/options.h"

#include <string_view>

#include "modules/adminapi/common/common.h"
#include "modules/adminapi/common/parallel_applier_options.h"
#include "modules/adminapi/common/server_features.h"
#include "mysqlshdk/libs/mysql/group_replication.h"
#include "mysqlshdk/libs/mysql/replication.h"

namespace mysqlsh {
namespace dba {
namespace cluster {

namespace {
/**
 * Map of the global Cluster configuration options of the AdminAPI
 * <sysvar, name>
 */
const std::map<std::string_view, std::string_view> k_global_cluster_options{
    {kGroupName, kGrGroupName},
    {kMemberSslMode, kGrMemberSslMode},
    {kTransactionSizeLimit, kGrTransactionSizeLimit}};

/**
 * Map of the instance configuration options of the AdminAPI
 * <sysvar, name>
 */
const std::map<std::string_view, std::string_view> k_instance_options{
    {kExitStateAction, kGrExitStateAction},
    {kGroupSeeds, kGrGroupSeeds},
    {kIpAllowlist, kGrIpAllowlist},
    {kLocalAddress, kGrLocalAddress},
    {kMemberWeight, kGrMemberWeight},
    {kExpelTimeout, kGrExpelTimeout},
    {kConsistency, kGrConsistency},
    {kAutoRejoinTries, kGrAutoRejoinTries}};

}  // namespace

Options::Options(const Cluster_impl &cluster, bool all)
    : m_cluster(cluster), m_all(all) {}

Options::~Options() = default;

/**
 * Connects to all members of the Cluster
 *
 * This function tries to connect to all registered members of the Cluster
 * and:
 *  - If the connection is established successfully add the session object to
 * m_member_sessions
 *  - If the connection cannot be established, add the connection error to
 * m_member_connect_errors
 */
void Options::connect_to_members() {
  auto group_server = m_cluster.get_cluster_server();

  mysqlshdk::db::Connection_options group_session_copts =
      group_server->get_connection_options();

  for (const auto &inst : m_instances) {
    mysqlshdk::db::Connection_options opts(inst.endpoint);
    if (opts.uri_endpoint() == group_session_copts.uri_endpoint()) {
      m_member_sessions[inst.endpoint] = group_server;
      continue;
    }

    opts.set_login_options_from(group_session_copts);

    try {
      m_member_sessions[inst.endpoint] = Instance::connect(opts);
    } catch (const shcore::Error &e) {
      m_member_connect_errors[inst.endpoint] = e.format();
    }
  }
}

/**
 * Get the global Cluster configuration options configuration options of
 * the Cluster
 *
 * This function gets the global Cluster configuration options and builds
 * an Array containing a Dictionary with the format:
 * {
 *   "option": "option_xxx",
 *   "value": "xxx-xxx",
 *   "variable": "group_replication_xxx"
 * }
 *
 * @return a shcore::Array_t containing a dictionary object with the global
 * Cluster configuration options information
 */
shcore::Array_t Options::collect_global_options() {
  shcore::Array_t array = shcore::make_array();

  auto group_instance = m_cluster.get_cluster_server();

  for (const auto &cfg : k_global_cluster_options) {
    shcore::Dictionary_t option = shcore::make_dict();
    std::string value = *group_instance->get_sysvar_string(
        cfg.second, mysqlshdk::mysql::Var_qualifier::GLOBAL);

    (*option)["option"] = shcore::Value(cfg.first);
    (*option)["variable"] = shcore::Value(cfg.second);
    (*option)["value"] = shcore::Value(std::move(value));

    array->push_back(shcore::Value(option));
  }

  // Get the cluster's disableClone option
  {
    shcore::Dictionary_t option = shcore::make_dict();
    (*option)["option"] = shcore::Value(kDisableClone);
    (*option)["value"] = shcore::Value(m_cluster.get_disable_clone_option());
    array->push_back(shcore::Value(std::move(option)));
  }

  // read cluster attributes
  {
    std::array<std::tuple<std::string_view, std::string_view>, 3> attribs{
        std::make_tuple(k_cluster_attribute_replication_allowed_host,
                        kReplicationAllowedHost),
        std::make_tuple(k_cluster_attribute_member_auth_type, kMemberAuthType),
        std::make_tuple(k_cluster_attribute_cert_issuer, kCertIssuer)};

    for (const auto &[attrib_name, attrib_desc] : attribs) {
      shcore::Value attrib_value;
      if (!m_cluster.get_metadata_storage()->query_cluster_attribute(
              m_cluster.get_id(), attrib_name, &attrib_value))
        attrib_value = shcore::Value::Null();

      shcore::Dictionary_t option = shcore::make_dict();
      (*option)["option"] = shcore::Value(attrib_desc);
      (*option)["value"] = shcore::Value(std::move(attrib_value));
      array->push_back(shcore::Value(std::move(option)));
    }
  }

  // Get the communicationStack
  //
  // If the Cluster doesn't support communication stack don't add it to the
  // globalOptions, otherwise, we might pass a misleading message that the
  // communicationStack is an option when in fact it's not in that case
  if (supports_mysql_communication_stack(group_instance->get_version())) {
    shcore::Dictionary_t option = shcore::make_dict();
    (*option)["option"] = shcore::Value(kCommunicationStack);
    (*option)["variable"] =
        shcore::Value("group_replication_communication_stack");
    (*option)["value"] =
        shcore::Value(get_communication_stack(*m_cluster.get_cluster_server()));
    array->push_back(shcore::Value(option));
  }

  // Get paxosSingleLeader
  //
  // If the Cluster doesn't support paxosSingleLeader don't add it to the
  // globalOptions, otherwise, we might pass a misleading message that the
  // paxosSingleLeader is an option when in fact it's not in that case
  if (supports_paxos_single_leader(group_instance->get_version())) {
    shcore::Dictionary_t option = shcore::make_dict();
    (*option)["option"] = shcore::Value(kPaxosSingleLeader);
    // Do not add the variable 'group_replication_paxos_single_leader' because
    // that's not enough to know whether the Cluster has it enabled and
    // effective or not. That information is available on
    // performance_schema.replication_group_communication_information.
    // WRITE_CONSENSUS_SINGLE_LEADER_CAPABLE
    std::string_view paxos_single_leader =
        get_paxos_single_leader_enabled(*m_cluster.get_cluster_server()) == true
            ? "ON"
            : "OFF";
    (*option)["value"] = shcore::Value(paxos_single_leader);
    array->push_back(shcore::Value(option));
  }

  // replication options
  if (m_cluster.is_cluster_set_member()) {
    std::array<std::tuple<std::string_view, std::string_view>, 7> options{
        std::make_tuple(k_cluster_attribute_repl_connect_retry,
                        kReplicationConnectRetry),
        std::make_tuple(k_cluster_attribute_repl_retry_count,
                        kReplicationRetryCount),
        std::make_tuple(k_cluster_attribute_repl_heartbeat_period,
                        kReplicationHeartbeatPeriod),
        std::make_tuple(k_cluster_attribute_repl_compression_algorithms,
                        kReplicationCompressionAlgorithms),
        std::make_tuple(k_cluster_attribute_repl_zstd_compression_level,
                        kReplicationZstdCompressionLevel),
        std::make_tuple(k_cluster_attribute_repl_bind, kReplicationBind),
        std::make_tuple(k_cluster_attribute_repl_network_namespace,
                        kReplicationNetworkNamespace)};

    for (const auto &[option_attrib, option_name] : options) {
      shcore::Value option_value;
      if (!m_cluster.get_metadata_storage()->query_cluster_attribute(
              m_cluster.get_id(), option_attrib, &option_value))
        option_value = shcore::Value::Null();

      shcore::Dictionary_t option = shcore::make_dict();
      (*option)["option"] = shcore::Value(shcore::str_format(
          "clusterSetReplication%.*s", static_cast<int>(option_name.size()),
          option_name.data()));
      (*option)["value"] = shcore::Value(std::move(option_value));
      array->push_back(shcore::Value(std::move(option)));
    }
  }

  return array;
}

/**
 * Get the configuration options configuration options of
 * a Cluster member
 *
 * This function gets the instance configuration options and builds
 * an Array containing a Dictionary with the format:
 * {
 *   "option": "option_xxx",
 *   "value": "xxx-xxx",
 *   "variable": "group_replication_xxx"
 * }
 *
 * NOTE: If the option 'all' was enabled, the function will query ALL Group
 * Replication options
 *
 * @param instance Target instance to query for the configuration options
 *
 * @return a shcore::Array_t containing a dictionary object with the instance
 * Cluster configuration options information
 */
shcore::Array_t Options::get_instance_options(
    const mysqlsh::dba::Instance &instance) {
  shcore::Array_t array = shcore::make_array();
  Parallel_applier_options parallel_applier_options(instance);
  auto target_instance_version = instance.get_version();

  // Get all the options supported by the AdminAPI
  // If the target instance does not support a particular option, it will be
  // listed with the null value, as expected.
  for (const auto &cfg : k_instance_options) {
    shcore::Dictionary_t option = shcore::make_dict();
    auto value = instance.get_sysvar_string(
        cfg.second, mysqlshdk::mysql::Var_qualifier::GLOBAL);

    (*option)["option"] = shcore::Value(cfg.first);
    (*option)["variable"] = shcore::Value(cfg.second);

    // Check if the option exists in the target server
    (*option)["value"] = value.has_value() ? shcore::Value(std::move(*value))
                                           : shcore::Value::Null();

    array->push_back(shcore::Value(std::move(option)));
  }

  // cert_subject
  {
    auto cert_subject =
        m_cluster.query_cluster_instance_auth_cert_subject(instance);

    shcore::Dictionary_t option = shcore::make_dict();
    (*option)["option"] = shcore::Value(kCertSubject);
    (*option)["value"] = shcore::Value(std::move(cert_subject));

    array->push_back(shcore::Value(std::move(option)));
  }

  // If 'all' is enabled, get all GR configuration options to add to the result
  // array.
  if (m_all) {
    auto option_supported_by_adminapi = [](std::string_view cfg) {
      for (const auto &it : k_instance_options) {
        if (it.second == cfg) return true;
      }
      return false;
    };

    log_debug("Get all group replication configurations.");
    std::map<std::string, std::optional<std::string>> gr_cfgs =
        mysqlshdk::gr::get_all_configurations(instance);

    for (const auto &[name, value] : gr_cfgs) {
      // If the option is part of the list of supported options by the AdminAPI,
      // skip it as it was already retrieved before
      if (option_supported_by_adminapi(name)) continue;

      shcore::Dictionary_t option = shcore::make_dict();

      (*option)["variable"] = shcore::Value(name);
      if (value.has_value()) (*option)["value"] = shcore::Value(*value);

      array->push_back(shcore::Value(std::move(option)));
    }
  }

  // Get the parallel-appliers set of options
  auto parallel_applier_options_values =
      parallel_applier_options.get_current_settings(instance.get_version());

  for (const auto &[name, value] : parallel_applier_options_values) {
    shcore::Dictionary_t option = shcore::make_dict();
    (*option)["variable"] = shcore::Value(name);
    // Check if the option exists in the target server
    (*option)["value"] =
        value.has_value() ? shcore::Value(*value) : shcore::Value::Null();

    array->push_back(shcore::Value(std::move(option)));
  }

  // replication options
  if (m_cluster.is_cluster_set_member() && !m_cluster.is_primary_cluster()) {
    mysqlshdk::mysql::Replication_channel_master_info channel_info;
    mysqlshdk::mysql::Replication_channel_relay_log_info relay_log_info;
    mysqlshdk::mysql::get_channel_info(instance,
                                       k_clusterset_async_channel_name,
                                       &channel_info, &relay_log_info);

    auto add_option = [&array](std::string_view option_name,
                               shcore::Value option_value) {
      shcore::Dictionary_t option = shcore::make_dict();
      (*option)["option"] = shcore::Value(shcore::str_format(
          "clusterSetReplication%.*s", static_cast<int>(option_name.size()),
          option_name.data()));
      (*option)["value"] = std::move(option_value);

      array->push_back(shcore::Value(std::move(option)));
    };

    add_option(kReplicationConnectRetry,
               shcore::Value(channel_info.connect_retry));
    add_option(kReplicationRetryCount, shcore::Value(channel_info.retry_count));
    add_option(kReplicationHeartbeatPeriod,
               shcore::Value(channel_info.heartbeat_period));
    add_option(kReplicationCompressionAlgorithms,
               channel_info.compression_algorithm.has_value()
                   ? shcore::Value(*channel_info.compression_algorithm)
                   : shcore::Value::Null());
    add_option(kReplicationZstdCompressionLevel,
               channel_info.zstd_compression_level.has_value()
                   ? shcore::Value(*channel_info.zstd_compression_level)
                   : shcore::Value::Null());
    add_option(kReplicationBind, shcore::Value(channel_info.bind));
    add_option(kReplicationNetworkNamespace,
               channel_info.network_namespace.has_value()
                   ? shcore::Value(*channel_info.network_namespace)
                   : shcore::Value::Null());
  }

  return array;
}

/**
 * Get the full Cluster Options information
 *
 * This function gets the Cluster configuration options and builds
 * an Dictionary containing the following fields:
 *   - "globalOptions": content of collect_global_options()
 *   - "topology": an array with every member of the cluster identified by its
 * label. Each member of the array contains the result from
 * get_instance_options(instance)
 *
 *
 * @return a shcore::Dictionary_t containing a dictionary object with the full
 * Cluster configuration options information
 */
shcore::Dictionary_t Options::collect_default_replicaset_options() {
  shcore::Dictionary_t tmp = shcore::make_dict();
  shcore::Dictionary_t ret = shcore::make_dict();

  // Get global options
  (*ret)["globalOptions"] = shcore::Value(collect_global_options());

  connect_to_members();

  for (const auto &inst : m_instances) {
    const auto &instance = m_member_sessions[inst.endpoint];

    if (!instance) {
      auto option = shcore::make_dict();
      (*option)["shellConnectError"] =
          shcore::Value(m_member_connect_errors[inst.endpoint]);
      (*tmp)[inst.label] = shcore::Value(std::move(option));
    } else {
      (*tmp)[inst.label] = shcore::Value(get_instance_options(*instance));
    }
  }

  (*ret)["topology"] = shcore::Value(std::move(tmp));

  return ret;
}

void Options::prepare() {
  // NOTE: Metadata session validation is done by the MetadataStorage class
  // itself

  // Get the current members list
  m_instances = m_cluster.get_instances();

  // Connect to every Cluster member and populate:
  //   - m_member_sessions
  //   - m_member_connect_errors
  connect_to_members();
}

shcore::Value Options::execute() {
  shcore::Dictionary_t dict = shcore::make_dict();

  (*dict)["clusterName"] = shcore::Value(m_cluster.get_name());

  // Get the default replicaSet options
  (*dict)["defaultReplicaSet"] =
      shcore::Value(collect_default_replicaset_options());

  // Get and insert the tags
  dict->get_map("defaultReplicaSet")
      ->emplace(kTags, m_cluster.get_cluster_tags());

  return shcore::Value(std::move(dict));
}

}  // namespace cluster
}  // namespace dba
}  // namespace mysqlsh
