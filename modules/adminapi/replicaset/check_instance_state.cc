/*
 * Copyright (c) 2018, 2019, Oracle and/or its affiliates. All rights reserved.
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

#include "modules/adminapi/replicaset/check_instance_state.h"

#include "modules/adminapi/common/instance_validations.h"
#include "modules/adminapi/common/metadata_storage.h"
#include "modules/adminapi/common/sql.h"
#include "mysqlshdk/include/shellcore/console.h"
#include "mysqlshdk/libs/mysql/replication.h"

namespace mysqlsh {
namespace dba {

Check_instance_state::Check_instance_state(
    const ReplicaSet &replicaset,
    const mysqlshdk::db::Connection_options &instance_cnx_opts)
    : m_replicaset(replicaset), m_instance_cnx_opts(instance_cnx_opts) {
  m_target_instance_address =
      m_instance_cnx_opts.as_uri(mysqlshdk::db::uri::formats::only_transport());
  m_address_in_metadata = m_target_instance_address;
}

Check_instance_state::~Check_instance_state() {}

/**
 * Ensure target instance is reachable.
 *
 * If the instance is not reachable:
 *   - Prints an error message indicating the connection error
 *   - Throws a runtime error indicating the instance is not reachable
 */
void Check_instance_state::ensure_target_instance_reachable() {
  log_debug("Connecting to instance '%s'", m_target_instance_address.c_str());
  std::shared_ptr<mysqlshdk::db::ISession> session;

  auto console = mysqlsh::current_console();

  try {
    session = mysqlshdk::db::mysql::Session::create();
    session->connect(m_instance_cnx_opts);
    m_target_instance =
        shcore::make_unique<mysqlshdk::mysql::Instance>(session);

    // Set the metadata address to use if instance is reachable.
    m_address_in_metadata =
        mysqlshdk::mysql::get_report_host(*m_target_instance) + ":" +
        std::to_string(m_instance_cnx_opts.get_port());
    log_debug("Successfully connected to instance");
  } catch (std::exception &err) {
    console->print_error("Failed to connect to instance: " +
                         std::string(err.what()));

    throw shcore::Exception::runtime_error(
        "The instance '" + m_target_instance_address + "' is not reachable.");
  }
}

/**
 * Ensure target instance has a valid GR state, being the only accepted state
 * Standalone.
 *
 * Throws an explicative runtime error if the target instance is not Standalone
 */
void Check_instance_state::ensure_instance_valid_gr_state() {
  // Get the instance GR state
  GRInstanceType instance_type =
      get_gr_instance_type(m_target_instance->get_session());

  if (instance_type != GRInstanceType::Standalone) {
    std::string error = "The instance '" + m_target_instance_address;

    // No need to verify if the state is GRInstanceType::InnoDBCluster because
    // that has been verified in ensure_instance_not_belong_to_cluster

    if (instance_type == GRInstanceType::GroupReplication) {
      error +=
          "' belongs to a Group Replication group that is not managed as an "
          "InnoDB cluster.";
    } else if (instance_type == GRInstanceType::StandaloneWithMetadata) {
      error +=
          "' is a standalone instance but is part of a different InnoDB "
          "Cluster (metadata exists, but Group Replication is not active).";
    } else {
      error += " has an unknown state.";
    }

    throw shcore::Exception::runtime_error(error);
  }
}

/**
 * Get the target instance GTID state in relation to the ReplicaSet
 *
 * This function gets the instance GTID state and builds a Dictionary with the
 * format:
 * {
 *   "reason": "currentReason",
 *   "state": "currentState",
 * }
 *
 * @return a shcore::Dictionary_t containing a dictionary object with instance
 * GTID state in relation to the replicaset
 */
shcore::Dictionary_t Check_instance_state::collect_instance_state() {
  shcore::Dictionary_t ret = shcore::make_dict();

  std::shared_ptr<mysqlshdk::db::ISession> cluster_session =
      m_replicaset.get_cluster()->get_group_session();

  // Get the target instance gtid_executed
  std::string instance_gtid_executed;
  get_server_variable(m_target_instance->get_session(), "GLOBAL.GTID_EXECUTED",
                      &instance_gtid_executed);

  // Check the gtid state in regards to the cluster_session
  SlaveReplicationState state =
      get_slave_replication_state(cluster_session, instance_gtid_executed);

  std::string reason;
  std::string status;
  switch (state) {
    case SlaveReplicationState::Diverged:
      status = "error";
      reason = "diverged";
      break;
    case SlaveReplicationState::Irrecoverable:
      status = "error";
      reason = "lost_transactions";
      break;
    case SlaveReplicationState::Recoverable:
      status = "ok";
      reason = "recoverable";
      break;
    case SlaveReplicationState::New:
      status = "ok";
      reason = "new";
      break;
  }

  (*ret)["state"] = shcore::Value(status);
  (*ret)["reason"] = shcore::Value(reason);

  return ret;
}

/**
 * Print the instance GTID state information
 *
 * This function prints human-readable information of the instance GTID state in
 * relation to the ReplicaSet based a shcore::Dictionary_t with that information
 * (collected by collect_instance_state())
 *
 * @param instance_state shcore::Dictionary_t containing a dictionary object
 * with instance GTID state in relation to the replicaset
 *
 */
void Check_instance_state::print_instance_state_info(
    shcore::Dictionary_t instance_state) {
  auto console = mysqlsh::current_console();

  if (instance_state->get_string("state") == "ok") {
    console->print_info("The instance '" + m_target_instance_address +
                        "' is valid for the cluster.");

    if (instance_state->get_string("reason") == "new") {
      console->print_info("The instance is new to Group Replication.");
    } else {
      console->print_info("The instance is fully recoverable.");
    }
  } else {
    console->print_info("The instance '" + m_target_instance_address +
                        "' is invalid for the cluster.");

    if (instance_state->get_string("reason") == "diverged") {
      console->print_info(
          "The instance contains additional transactions in relation to "
          "the cluster.");
    } else {
      console->print_info(
          "There are transactions in the cluster that can't be recovered "
          "on the instance.");
    }
  }
  console->println();
}

void Check_instance_state::prepare() {
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

  // Verify if the target instance is reachable
  ensure_target_instance_reachable();

  // Ensure the target instance does not belong to the metadata.
  mysqlsh::dba::checks::ensure_instance_not_belong_to_metadata(
      *m_target_instance, m_address_in_metadata, m_replicaset);

  // Ensure the target instance has a valid GR state
  ensure_instance_valid_gr_state();
}

shcore::Value Check_instance_state::execute() {
  auto console = mysqlsh::current_console();

  console->print_info("Analyzing the instance '" + m_target_instance_address +
                      "' replication state...");
  console->println();

  // Get the instance state
  shcore::Dictionary_t instance_state;

  instance_state = collect_instance_state();

  // Print the instance state information
  print_instance_state_info(instance_state);

  return shcore::Value(instance_state);
}

void Check_instance_state::rollback() {}

void Check_instance_state::finish() {
  // Close the session to the target instance
  if (m_target_instance) m_target_instance->close_session();

  // Reset all auxiliary (temporary) data used for the operation execution.
  m_target_instance_address.clear();
}

}  // namespace dba
}  // namespace mysqlsh
