/*
 * Copyright (c) 2018, Oracle and/or its affiliates. All rights reserved.
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

#include "modules/adminapi/replicaset/set_instance_option.h"
#include "modules/adminapi/common/metadata_storage.h"
#include "modules/adminapi/common/validations.h"
#include "mysqlshdk/include/shellcore/console.h"
#include "mysqlshdk/libs/config/config_server_handler.h"
#include "mysqlshdk/libs/utils/utils_general.h"

namespace mysqlsh {
namespace dba {

Set_instance_option::Set_instance_option(
    const ReplicaSet &replicaset,
    const mysqlshdk::db::Connection_options &instance_cnx_opts,
    const std::string &option, const std::string &value)
    : m_replicaset(replicaset),
      m_instance_cnx_opts(instance_cnx_opts),
      m_option(option),
      m_value_str(value) {
  m_target_instance_address =
      m_instance_cnx_opts.as_uri(mysqlshdk::db::uri::formats::only_transport());
}

Set_instance_option::Set_instance_option(
    const ReplicaSet &replicaset,
    const mysqlshdk::db::Connection_options &instance_cnx_opts,
    const std::string &option, int64_t value)
    : m_replicaset(replicaset),
      m_instance_cnx_opts(instance_cnx_opts),
      m_option(option),
      m_value_int(value) {
  m_target_instance_address =
      m_instance_cnx_opts.as_uri(mysqlshdk::db::uri::formats::only_transport());
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
      throw shcore::Exception::argument_error(
          "Invalid value for 'label': Argument #3 is expected to be a "
          "string.");
    } else {
      mysqlsh::dba::validate_label(*m_value_str);
    }

    // Check if there's already an instance with the label we want to set
    if (!m_replicaset.get_cluster()
             ->get_metadata_storage()
             ->is_instance_label_unique(m_replicaset.get_id(), *m_value_str)) {
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

void Set_instance_option::ensure_instance_belong_to_replicaset() {
  auto console = mysqlsh::current_console();

  // Check if the instance exists on the ReplicaSet
  log_debug("Checking if the instance belongs to the replicaset");
  bool is_instance_on_md =
      m_replicaset.get_cluster()
          ->get_metadata_storage()
          ->is_instance_on_replicaset(m_replicaset.get_id(),
                                      m_target_instance_address);

  if (!is_instance_on_md) {
    std::string err_msg = "The instance '" + m_target_instance_address +
                          "' does not belong to the ReplicaSet: '" +
                          m_replicaset.get_member("name").get_string() + "'.";
    throw shcore::Exception::runtime_error(err_msg);
  }
}

void Set_instance_option::ensure_target_member_online() {
  log_debug("Connecting to instance '%s'", m_target_instance_address.c_str());
  std::shared_ptr<mysqlshdk::db::ISession> session;

  try {
    session = mysqlshdk::db::mysql::Session::create();
    session->connect(m_instance_cnx_opts);
    m_target_instance =
        shcore::make_unique<mysqlshdk::mysql::Instance>(session);
    log_debug("Successfully connected to instance");
  } catch (std::exception &err) {
    log_debug("Failed to connect to instance: %s", err.what());

    throw shcore::Exception::runtime_error(
        "The instance '" + m_target_instance_address + "' is not ONLINE.");
  }
}

void Set_instance_option::ensure_option_supported_target_member() {
  auto console = mysqlsh::current_console();

  log_debug("Checking if all members of the Replicaset support the option '%s'",
            m_option.c_str());

  mysqlshdk::utils::Version support_in_80 =
      k_instance_supported_options.at(m_option).support_in_80;
  mysqlshdk::utils::Version support_in_57 =
      k_instance_supported_options.at(m_option).support_in_57;

  // Verify if the instance version is supported
  mysqlshdk::utils::Version instance_version = m_target_instance->get_version();

  if ((instance_version < mysqlshdk::utils::Version("8.0.0") &&
       instance_version > mysqlshdk::utils::Version("5.7.0") &&
       instance_version < support_in_57) ||
      (instance_version > mysqlshdk::utils::Version("8.0.0") &&
       instance_version < support_in_80)) {
    console->print_error(
        "The instance '" + m_target_instance_address + "' has the version " +
        m_target_instance->get_version().get_full() +
        " which does not support the option '" + m_option + "'.");

    throw shcore::Exception::runtime_error(
        "One or more instances of the cluster have a version that does not "
        "support this operation.");
  }
}

void Set_instance_option::prepare_config_object() {
  auto console = mysqlsh::current_console();
  m_cfg = shcore::make_unique<mysqlshdk::config::Config>();

  bool is_set_persist_supported =
      !m_target_instance->is_set_persist_supported().is_null() &&
      *m_target_instance->is_set_persist_supported();

  // Create server configuration handler depending on SET PERSIST support.
  std::unique_ptr<mysqlshdk::config::IConfig_handler> config_handler =
      shcore::make_unique<mysqlshdk::config::Config_server_handler>(
          m_target_instance.get(),
          is_set_persist_supported ? mysqlshdk::mysql::Var_qualifier::PERSIST
                                   : mysqlshdk::mysql::Var_qualifier::GLOBAL);

  // Add the server configuration to the configuration object
  m_cfg->add_handler(m_target_instance_address, std::move(config_handler));
}

void Set_instance_option::prepare() {
  // Validate if the option is valid
  ensure_option_valid();

  // Validate connection options.
  log_debug("Verifying connection options");
  validate_connection_options(m_instance_cnx_opts);

  // Use default port if not provided in the connection options.
  if (!m_instance_cnx_opts.has_port()) {
    m_instance_cnx_opts.set_port(mysqlshdk::db::k_default_mysql_port);
    m_target_instance_address = m_instance_cnx_opts.as_uri(
        mysqlshdk::db::uri::formats::only_transport());
  }

  // Get instance login information from the cluster session if missing.
  if (!m_instance_cnx_opts.has_user() || !m_instance_cnx_opts.has_password()) {
    std::shared_ptr<mysqlshdk::db::ISession> cluster_session =
        m_replicaset.get_cluster()->get_group_session();
    Connection_options cluster_cnx_opt =
        cluster_session->get_connection_options();

    if (!m_instance_cnx_opts.has_user() && cluster_cnx_opt.has_user())
      m_instance_cnx_opts.set_user(cluster_cnx_opt.get_user());

    if (!m_instance_cnx_opts.has_password() && cluster_cnx_opt.has_password())
      m_instance_cnx_opts.set_password(cluster_cnx_opt.get_password());
  }

  // Verify if the target instance belongs to the replicaset
  ensure_instance_belong_to_replicaset();

  // Verify if the target cluster member is ONLINE
  ensure_target_member_online();

  // Verify user privileges to execute operation;
  ensure_user_privileges(*m_target_instance);

  // Verify if the target cluster member supports the option
  // NOTE: label does not require this validation
  if (m_option != "label") {
    ensure_option_supported_target_member();
  }

  // Set the internal configuration object.
  prepare_config_object();
}

shcore::Value Set_instance_option::execute() {
  auto console = mysqlsh::current_console();

  std::string target_instance_label =
      m_replicaset.get_cluster()
          ->get_metadata_storage()
          ->get_instance(m_target_instance_address)
          .label;

  console->print_info(
      "Setting the value of '" + m_option + "' to '" +
      (m_value_str.is_null() ? std::to_string(*m_value_int) : *m_value_str) +
      "' in the instance: '" + m_target_instance_address + "' ...");
  console->println();

  if (m_option == "label") {
    m_replicaset.get_cluster()->get_metadata_storage()->set_instance_label(
        m_replicaset.get_id(), target_instance_label, *m_value_str);
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
      "' in the '" + m_replicaset.get_name() + "' ReplicaSet member: '" +
      m_target_instance_address + "'.");

  return shcore::Value();
}

void Set_instance_option::rollback() {}

void Set_instance_option::finish() {}

}  // namespace dba
}  // namespace mysqlsh
