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

#include "modules/adminapi/cluster/set_instance_option.h"

#include "modules/adminapi/cluster/cluster_impl.h"
#include "modules/adminapi/common/async_topology.h"
#include "modules/adminapi/common/common.h"
#include "modules/adminapi/common/errors.h"
#include "modules/adminapi/common/preconditions.h"
#include "modules/adminapi/common/validations.h"
#include "mysqlshdk/include/shellcore/console.h"
#include "scripting/types.h"

namespace mysqlsh::dba::cluster {

namespace {

/**
 * Map of the supported instance configuration options in the AdminAPI
 * <sysvar, name>
 */
const std::map<std::string, Option_availability> k_instance_supported_options{
    {kExitStateAction,
     {kGrExitStateAction, mysqlshdk::utils::Version("8.0.12")}},
    {kMemberWeight, {kGrMemberWeight, mysqlshdk::utils::Version("8.0.11")}},
    {kAutoRejoinTries,
     {kGrAutoRejoinTries, mysqlshdk::utils::Version("8.0.16")}},
    {kIpAllowlist, {kGrIpAllowlist, mysqlshdk::utils::Version("8.0.24")}},
    {kReplicationSources, {"", Precondition_checker::k_min_rr_version}}};

constexpr std::array<std::string_view, 2> k_read_replica_supported_options = {
    kLabel, kReplicationSources};

}  // namespace

Set_instance_option::Set_instance_option(
    const Cluster_impl &cluster,
    const mysqlshdk::db::Connection_options &instance_cnx_opts,
    const std::string &option, const std::string &value)
    : m_cluster(cluster),
      m_instance_cnx_opts(instance_cnx_opts),
      m_option(option),
      m_value_str(value) {
  m_target_instance_address =
      m_instance_cnx_opts.as_uri(mysqlshdk::db::uri::formats::only_transport());
  m_address_in_metadata = m_target_instance_address;
}

Set_instance_option::Set_instance_option(
    const Cluster_impl &cluster,
    const mysqlshdk::db::Connection_options &instance_cnx_opts,
    const std::string &option, int64_t value)
    : m_cluster(cluster),
      m_instance_cnx_opts(instance_cnx_opts),
      m_option(option),
      m_value_int(value) {
  m_target_instance_address =
      m_instance_cnx_opts.as_uri(mysqlshdk::db::uri::formats::only_transport());
  m_address_in_metadata = m_target_instance_address;
}

Set_instance_option::Set_instance_option(
    const Cluster_impl &cluster,
    const mysqlshdk::db::Connection_options &instance_cnx_opts,
    const std::string &option, shcore::Value value)
    : m_cluster(cluster),
      m_instance_cnx_opts(instance_cnx_opts),
      m_option(option),
      m_value_value(value) {
  m_target_instance_address =
      m_instance_cnx_opts.as_uri(mysqlshdk::db::uri::formats::only_transport());
  m_address_in_metadata = m_target_instance_address;
}

Set_instance_option::~Set_instance_option() = default;

void Set_instance_option::ensure_option_valid() {
  /* - Validate if the option is valid, being the accepted values:
   *     - label
   *     - exitStateAction
   *     - memberWeight
   */
  if (m_option == "label") {
    // The value must be a string
    if (m_value_int.has_value())
      throw shcore::Exception::type_error(
          "Invalid value for 'label': Argument #3 is expected to be a string.");

    mysqlsh::dba::validate_label(*m_value_str);

    // Check if there's already an instance with the label we want to set
    if (!m_cluster.get_metadata_storage()->is_instance_label_unique(
            m_cluster.get_id(), *m_value_str)) {
      auto instance_md =
          m_cluster.get_metadata_storage()->get_instance_by_label(*m_value_str);
      throw shcore::Exception::argument_error(shcore::str_format(
          "Instance '%s' is already using label '%s'.",
          instance_md.address.c_str(), m_value_str->c_str()));
    }
  } else {
    if (k_instance_supported_options.count(m_option) == 0) {
      throw shcore::Exception::argument_error("Option '" + m_option +
                                              "' not supported.");
    }

    if (m_option == kIpAllowlist && !m_value_str.has_value()) {
      throw shcore::Exception::argument_error(
          "Invalid value for 'ipAllowlist': Argument #3 is expected "
          "to be a string.");
    }

    if (m_option == kReplicationSources) {
      validate_replication_sources_option(*m_value_value);
    }
  }
}

void Set_instance_option::ensure_instance_belong_to_cluster() {
  // Check if the instance exists on the cluster
  log_debug("Checking if the instance belongs to the cluster");
  bool is_instance_on_md =
      m_cluster.contains_instance_with_address(m_address_in_metadata);

  if (!is_instance_on_md) {
    std::string err_msg = "The instance '" + m_target_instance_address +
                          "' does not belong to the cluster.";
    throw shcore::Exception::runtime_error(err_msg);
  }
}

void Set_instance_option::ensure_target_member_reachable() {
  log_debug("Connecting to instance '%s'", m_target_instance_address.c_str());

  try {
    m_target_instance = Instance::connect(m_instance_cnx_opts);

    // Set the metadata address to use if instance is reachable.
    m_address_in_metadata = m_target_instance->get_canonical_address();
    log_debug("Successfully connected to instance");
  }
  CATCH_REPORT_AND_THROW_CONNECTION_ERROR(m_target_instance_address)
}

void Set_instance_option::ensure_option_supported_target_member() {
  log_debug("Checking if member '%s' of the cluster supports the option '%s'",
            m_target_instance->descr().c_str(), m_option.c_str());

  // If the member is a Read-Replica, validate first if the option is supportes
  if (m_cluster.is_read_replica(*m_target_instance)) {
    if (std::find(k_read_replica_supported_options.begin(),
                  k_read_replica_supported_options.end(),
                  m_option) != k_read_replica_supported_options.end()) {
      return;
    }

    mysqlsh::current_console()->print_error(
        "The option '" + m_option + "' is not supported for Read-Replicas.");
  } else {
    // Verify if the instance version is supported
    bool is_supported =
        is_option_supported(m_target_instance->get_version(), m_option,
                            k_instance_supported_options);

    if (is_supported) return;

    mysqlsh::current_console()->print_error(
        "The instance '" + m_target_instance_address + "' has the version " +
        m_target_instance->get_version().get_full() +
        " which does not support the option '" + m_option + "'.");
  }

  throw shcore::Exception::runtime_error("The instance '" +
                                         m_target_instance_address +
                                         "' does not support this operation.");
}

void Set_instance_option::ensure_instance_is_read_replica() {
  log_debug("Checking if the instance is a read-replica");

  if (!m_cluster.is_read_replica(*m_target_instance)) {
    std::string err_msg = "The instance '" + m_target_instance_address +
                          "' is not a Read-Replica.";
    throw shcore::Exception::runtime_error(err_msg);
  }
}

void Set_instance_option::prepare() {
  // Validate if the option is valid
  ensure_option_valid();

  // Use default port if not provided in the connection options.
  if (!m_instance_cnx_opts.has_port() && !m_instance_cnx_opts.has_socket()) {
    m_instance_cnx_opts.set_port(mysqlshdk::db::k_default_mysql_port);
    m_target_instance_address = m_instance_cnx_opts.as_uri(
        mysqlshdk::db::uri::formats::only_transport());
  }
  // Get instance login information from the cluster session if missing.
  if (!m_instance_cnx_opts.has_user() || !m_instance_cnx_opts.has_password()) {
    std::shared_ptr<Instance> cluster_instance = m_cluster.get_cluster_server();
    Connection_options cluster_cnx_opt =
        cluster_instance->get_connection_options();

    if (!m_instance_cnx_opts.has_user() && cluster_cnx_opt.has_user()) {
      m_instance_cnx_opts.set_user(cluster_cnx_opt.get_user());
    }

    if (!m_instance_cnx_opts.has_password() && cluster_cnx_opt.has_password()) {
      m_instance_cnx_opts.set_password(cluster_cnx_opt.get_password());
    }
  }

  // Verify if the target cluster member is ONLINE
  ensure_target_member_reachable();

  // Verify if the target instance belongs to the cluster
  ensure_instance_belong_to_cluster();

  // Verify if the target instance is a Read-Replica
  // and validate the replicationSources:
  //  - All sources must be reachable
  //  - All sources must be ONLINE Cluster members
  if (m_option == kReplicationSources) {
    ensure_instance_is_read_replica();

    // If replicationSources is a list, validate the sources
    if (m_value_value->get_type() == shcore::Array) {
      auto list =
          m_value_value->to_string_container<std::vector<std::string>>();

      std::set<Managed_async_channel_source,
               std::greater<Managed_async_channel_source>>
          managed_src_list;

      // The source list is ordered by weight
      int64_t source_weight = k_read_replica_max_weight;

      for (const auto &src : list) {
        Managed_async_channel_source managed_src_address(src);

        // Add it if not in the list already. The one kept in the list is the
        // one with the highest priority/weight
        if (managed_src_list.find(managed_src_address) ==
            managed_src_list.end()) {
          Managed_async_channel_source managed_src(src, source_weight--);
          managed_src_list.emplace(std::move(managed_src));
        }
      }

      std::vector<std::string> updated_list;

      m_cluster.validate_replication_sources(
          managed_src_list, m_target_instance_address, &updated_list);

      m_value_value = shcore::Value::new_array();
      auto array = m_value_value->as_array();

      for (const auto &source : updated_list) {
        array->emplace_back(source);
      }
    }
  }

  // Verify user privileges to execute operation;
  ensure_user_privileges(*m_target_instance);

  // Verify if the target cluster member supports the option
  // NOTE: label does not require this validation
  if (m_option != "label") {
    ensure_option_supported_target_member();
  }

  // Create the internal configuration object.
  m_cfg = mysqlsh::dba::create_server_config(m_target_instance.get(),
                                             m_target_instance_address);

  if (m_option != kAutoRejoinTries || !m_value_int.has_value() ||
      (*m_value_int == 0))
    return;

  auto console = mysqlsh::current_console();
  console->print_warning(
      "The member will only proceed according to its exitStateAction if "
      "auto-rejoin fails (i.e. all retry attempts are exhausted).");
  console->print_info();
}

shcore::Value Set_instance_option::execute() {
  std::string value_to_print;

  if (m_value_int.has_value()) {
    value_to_print = std::to_string(*m_value_int);
  } else if (m_value_str.has_value()) {
    value_to_print = *m_value_str;
  } else if (m_value_value.has_value()) {
    if (m_value_value->get_type() == shcore::String) {
      value_to_print = m_value_value->as_string();
    } else if (m_value_value->get_type() == shcore::Array) {
      auto list =
          m_value_value->to_string_container<std::vector<std::string>>();

      value_to_print = "[";

      for (const auto &element : list) {
        value_to_print += element;
        value_to_print += ", ";
      }

      if (!list.empty()) {
        value_to_print.erase(value_to_print.size() - 2);
      }

      value_to_print += "]";
    }
  }

  auto console = mysqlsh::current_console();
  console->print_info("Setting the value of '" + m_option + "' to '" +
                      value_to_print + "' in the instance: '" +
                      m_target_instance_address + "' ...");
  console->print_info();

  if (m_option == "label") {
    std::string target_instance_label =
        m_cluster.get_metadata_storage()
            ->get_instance_by_address(m_address_in_metadata)
            .label;

    m_cluster.get_metadata_storage()->set_instance_label(
        m_cluster.get_id(), target_instance_label, *m_value_str);
  } else if (m_option == kReplicationSources) {
    m_cluster.get_metadata_storage()->update_instance_attribute(
        m_target_instance->get_uuid(),
        k_instance_attribute_read_replica_replication_sources, *m_value_value);

    console->print_warning(
        "To update the replication channel with the changes the Read-Replica "
        "must be reconfigured using Cluster.<<<rejoinInstance>>>().");
  } else {
    // Update the option value in the target instance:
    const std::string &option_gr_variable =
        k_instance_supported_options.at(m_option).option;

    if (m_value_str.has_value()) {
      m_cfg->set(option_gr_variable, m_value_str);
    } else {
      m_cfg->set(option_gr_variable, m_value_int);
    }

    m_cfg->apply();
  }

  console->print_info(shcore::str_format(
      "Successfully set the value of '%s' to '%s' in the cluster member: '%s'.",
      m_option.c_str(), value_to_print.c_str(),
      m_target_instance_address.c_str()));

  return shcore::Value();
}

}  // namespace mysqlsh::dba::cluster
