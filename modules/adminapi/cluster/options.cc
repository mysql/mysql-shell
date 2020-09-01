/*
 * Copyright (c) 2018, 2020, Oracle and/or its affiliates.
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
#include "modules/adminapi/common/common.h"
#include "mysqlshdk/libs/mysql/group_replication.h"

namespace mysqlsh {
namespace dba {
namespace cluster {

Options::Options(const Cluster_impl &cluster, bool all)
    : m_cluster(cluster), m_all(all) {}

Options::~Options() {}

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
  auto group_server = m_cluster.get_target_server();

  mysqlshdk::db::Connection_options group_session_copts(
      group_server->get_connection_options());

  for (const auto &inst : m_instances) {
    mysqlshdk::db::Connection_options opts(inst.endpoint);
    if (opts.uri_endpoint() == group_session_copts.uri_endpoint()) {
      m_member_sessions[inst.endpoint] = group_server;
    } else {
      opts.set_login_options_from(group_session_copts);

      try {
        m_member_sessions[inst.endpoint] = Instance::connect(opts);
      } catch (const shcore::Error &e) {
        m_member_connect_errors[inst.endpoint] = e.format();
      }
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

  auto group_instance = m_cluster.get_target_server();

  for (const auto &cfg : k_global_options) {
    shcore::Dictionary_t option = shcore::make_dict();
    std::string value = *group_instance->get_sysvar_string(
        cfg.second, mysqlshdk::mysql::Var_qualifier::GLOBAL);

    (*option)["option"] = shcore::Value(cfg.first);
    (*option)["variable"] = shcore::Value(cfg.second);
    (*option)["value"] = shcore::Value(value);

    array->push_back(shcore::Value(option));
  }

  // Get the cluster's disableClone option
  shcore::Dictionary_t option = shcore::make_dict();
  (*option)["option"] = shcore::Value(kDisableClone);
  (*option)["value"] = shcore::Value(m_cluster.get_disable_clone_option());
  array->push_back(shcore::Value(option));

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

  // Get all the options supported by the AdminAPI
  // If the target instance does not support a particular option, it will be
  // listed with the null value, as expected.
  for (const auto &cfg : k_instance_options) {
    shcore::Dictionary_t option = shcore::make_dict();
    mysqlshdk::utils::nullable<std::string> value = instance.get_sysvar_string(
        cfg.second, mysqlshdk::mysql::Var_qualifier::GLOBAL);

    (*option)["option"] = shcore::Value(cfg.first);
    (*option)["variable"] = shcore::Value(cfg.second);

    // Check if the option exists in the target server
    if (!value.is_null()) {
      (*option)["value"] = shcore::Value(*value);
    } else {
      (*option)["value"] = shcore::Value::Null();
    }

    array->push_back(shcore::Value(option));
  }

  // If 'all' is enabled, get all GR configuration options to add to the result
  // array.
  if (m_all) {
    auto option_supported_by_adminapi = [](const std::string &cfg) {
      for (const auto &it : k_instance_options) {
        if (it.second == cfg) {
          return true;
        }
      }
      return false;
    };

    log_debug("Get all group replication configurations.");
    std::map<std::string, mysqlshdk::utils::nullable<std::string>> gr_cfgs =
        mysqlshdk::gr::get_all_configurations(instance);

    for (const auto &cfg : gr_cfgs) {
      shcore::Dictionary_t option = shcore::make_dict();

      // If the option is part of the list of supported options by the AdminAPI,
      // skip it as it was already retrieved before
      if (option_supported_by_adminapi(cfg.first)) {
        continue;
      }

      (*option)["variable"] = shcore::Value(cfg.first);

      if (!cfg.second.is_null()) {
        (*option)["value"] = shcore::Value(*cfg.second);
      }

      array->push_back(shcore::Value(option));
    }
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
    shcore::Dictionary_t option = shcore::make_dict();

    auto instance(m_member_sessions[inst.endpoint]);

    if (!instance) {
      (*option)["shellConnectError"] =
          shcore::Value(m_member_connect_errors[inst.endpoint]);
      (*tmp)[inst.label] = shcore::Value(option);
    } else {
      (*tmp)[inst.label] = shcore::Value(get_instance_options(*instance));
    }
  }

  (*ret)["topology"] = shcore::Value(tmp);

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

  return shcore::Value(dict);
}

void Options::rollback() {
  // Do nothing right now, but it might be used in the future when
  // transactional command execution feature will be available.
}

void Options::finish() {}

}  // namespace cluster
}  // namespace dba
}  // namespace mysqlsh
