/*
 * Copyright (c) 2018, 2021, Oracle and/or its affiliates.
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
#include "modules/adminapi/common/errors.h"
#include "modules/adminapi/common/metadata_storage.h"
#include "modules/adminapi/common/validations.h"
#include "mysqlshdk/include/shellcore/console.h"
#include "mysqlshdk/libs/config/config_server_handler.h"
#include "mysqlshdk/libs/db/mysql/session.h"
#include "mysqlshdk/libs/mysql/replication.h"
#include "mysqlshdk/libs/utils/utils_general.h"

namespace mysqlsh {
namespace dba {
namespace cluster {

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

Set_instance_option::~Set_instance_option() {}

void Set_instance_option::ensure_option_valid() {
  /* - Validate if the option is valid, being the accepted values:
   *     - label
   *     - exitStateAction
   *     - memberWeight
   */
  if (m_option == "label") {
    // The value must be a string
    if (!m_value_int.is_null()) {
      throw shcore::Exception::type_error(
          "Invalid value for 'label': Argument #3 is expected to be a "
          "string.");
    } else {
      mysqlsh::dba::validate_label(*m_value_str);
    }

    // Check if there's already an instance with the label we want to set
    if (!m_cluster.get_metadata_storage()->is_instance_label_unique(
            m_cluster.get_id(), *m_value_str)) {
      throw shcore::Exception::argument_error(
          "An instance with label '" + *m_value_str +
          "' is already part of this InnoDB cluster");
    }
  } else {
    if (k_instance_supported_options.count(m_option) == 0) {
      throw shcore::Exception::argument_error("Option '" + m_option +
                                              "' not supported.");
    }
  }
}

void Set_instance_option::ensure_instance_belong_to_cluster() {
  auto console = mysqlsh::current_console();

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
  auto console = mysqlsh::current_console();

  log_debug("Checking if member '%s' of the cluster supports the option '%s'",
            m_target_instance->descr().c_str(), m_option.c_str());

  // Verify if the instance version is supported
  bool is_supported = is_option_supported(
      m_target_instance->get_version(), m_option, k_instance_supported_options);

  if (!is_supported) {
    console->print_error(
        "The instance '" + m_target_instance_address + "' has the version " +
        m_target_instance->get_version().get_full() +
        " which does not support the option '" + m_option + "'.");

    throw shcore::Exception::runtime_error("The instance '" +
                                           m_target_instance_address +
                                           "' does not support "
                                           "this operation.");
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

  if (m_option == kAutoRejoinTries && !m_value_int.is_null() &&
      *m_value_int != 0) {
    auto console = mysqlsh::current_console();
    std::string warn_msg =
        "The member will only proceed according to its exitStateAction if "
        "auto-rejoin fails (i.e. all retry attempts are exhausted).";
    console->print_warning(warn_msg);
    console->println();
  }
}

shcore::Value Set_instance_option::execute() {
  auto console = mysqlsh::current_console();

  std::string target_instance_label =
      m_cluster.get_metadata_storage()
          ->get_instance_by_address(m_address_in_metadata)
          .label;

  console->print_info(
      "Setting the value of '" + m_option + "' to '" +
      (m_value_str.is_null() ? std::to_string(*m_value_int) : *m_value_str) +
      "' in the instance: '" + m_target_instance_address + "' ...");
  console->println();

  if (m_option == "label") {
    m_cluster.get_metadata_storage()->set_instance_label(
        m_cluster.get_id(), target_instance_label, *m_value_str);
  } else {
    // Update the option value in the target instance:
    std::string option_gr_variable =
        k_instance_supported_options.at(m_option).option_variable;

    if (!m_value_str.is_null()) {
      m_cfg->set(option_gr_variable, m_value_str);
    } else {
      m_cfg->set(option_gr_variable, m_value_int);
    }

    m_cfg->apply();
  }

  console->print_info(
      "Successfully set the value of '" + m_option + "' to '" +
      ((m_value_str.is_null() ? std::to_string(*m_value_int) : *m_value_str)) +
      "' in the cluster member: '" + m_target_instance_address + "'.");

  return shcore::Value();
}

void Set_instance_option::rollback() {}

void Set_instance_option::finish() {}

}  // namespace cluster
}  // namespace dba
}  // namespace mysqlsh
