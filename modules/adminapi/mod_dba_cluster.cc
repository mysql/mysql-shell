/*
 * Copyright (c) 2016, 2017, Oracle and/or its affiliates. All rights reserved.
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

#include "modules/adminapi/mod_dba_cluster.h"
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include "modules/adminapi/mod_dba_common.h"
#include "modules/mysqlxtest_utils.h"
#include "modules/adminapi/mod_dba_metadata_storage.h"
#include "modules/adminapi/mod_dba_replicaset.h"
#include "modules/adminapi/mod_dba_sql.h"
#include "shellcore/utils_help.h"
#include "utils/utils_general.h"

using namespace std::placeholders;

namespace mysqlsh {
namespace dba {

// Documentation of the Cluster Class
REGISTER_HELP(CLUSTER_BRIEF, "Represents an instance of MySQL InnoDB Cluster.");
REGISTER_HELP(CLUSTER_DETAIL,
              "The cluster object is the entrance point to manage the MySQL "
              "InnoDB Cluster system.");
REGISTER_HELP(
    CLUSTER_DETAIL1,
    "A cluster is a set of MySQLd Instances which holds the user's data.");
REGISTER_HELP(
    CLUSTER_DETAIL2,
    "It provides high-availability and scalability for the user's data.");

REGISTER_HELP(
    CLUSTER_CLOSING,
    "For more help on a specific function use: cluster.help('<functionName>')");
REGISTER_HELP(CLUSTER_CLOSING1, "e.g. cluster.help('addInstance')");

REGISTER_HELP(CLUSTER_NAME_BRIEF, "Cluster name.");

Cluster::Cluster(const std::string &name,
                 std::shared_ptr<MetadataStorage> metadata_storage)
    : _name(name), _dissolved(false), _metadata_storage(metadata_storage) {
  _session = _metadata_storage->get_session();
  init();
}

Cluster::~Cluster() {}

std::string &Cluster::append_descr(std::string &s_out, int UNUSED(indent),
                                   int UNUSED(quote_strings)) const {
  s_out.append("<" + class_name() + ":" + _name + ">");
  return s_out;
}

bool Cluster::operator==(const Object_bridge &other) const {
  return class_name() == other.class_name() && this == &other;
}

void Cluster::init() {
  add_property("name", "getName");
  add_method("addInstance", std::bind(&Cluster::add_instance, this, _1),
             "data");
  add_method("rejoinInstance", std::bind(&Cluster::rejoin_instance, this, _1),
             "data");
  add_method("removeInstance", std::bind(&Cluster::remove_instance, this, _1),
             "data");
  add_method("describe", std::bind(&Cluster::describe, this, _1), NULL);
  add_method("status", std::bind(&Cluster::status, this, _1), NULL);
  add_varargs_method("dissolve", std::bind(&Cluster::dissolve, this, _1));
  add_varargs_method("checkInstanceState",
                     std::bind(&Cluster::check_instance_state, this, _1));
  add_varargs_method("rescan", std::bind(&Cluster::rescan, this, _1));
  add_varargs_method(
      "forceQuorumUsingPartitionOf",
      std::bind(&Cluster::force_quorum_using_partition_of, this, _1));
}

// Documentation of the getName function
REGISTER_HELP(CLUSTER_GETNAME_BRIEF, "Retrieves the name of the cluster.");
REGISTER_HELP(CLUSTER_GETNAME_RETURNS, "@returns The name of the cluster.");

/**
 * $(CLUSTER_GETNAME_BRIEF)
 *
 * $(CLUSTER_GETNAME_RETURNS)
 *
 */
#if DOXYGEN_JS
String Cluster::getName() {}
#elif DOXYGEN_PY
str Cluster::get_name() {}
#endif

shcore::Value Cluster::call(const std::string &name,
                            const shcore::Argument_list &args) {
  // Throw an error if the cluster has already been dissolved
  assert_not_dissolved(name);
  return Cpp_object_bridge::call(name, args);
}

shcore::Value Cluster::get_member(const std::string &prop) const {
  shcore::Value ret_val;
  // Throw an error if the cluster has already been dissolved
  assert_not_dissolved(prop);

  if (prop == "name")
    ret_val = shcore::Value(_name);
  else
    ret_val = shcore::Cpp_object_bridge::get_member(prop);
  return ret_val;
}

void Cluster::assert_not_dissolved(const std::string &option_name) const {
  std::string name;
  if (has_member(option_name) && _dissolved) {
    if (has_method(option_name)) {
      name = get_function_name(option_name, false);
      throw shcore::Exception::runtime_error(class_name() + "." + name + ": " +
                                     "Can't call function '" + name +
                                     "' on a dissolved cluster");
    } else {
      name = get_member_name(option_name, naming_style);
      throw shcore::Exception::runtime_error(class_name() + "." + name + ": " +
                                     "Can't access object member '" + name +
                                     "' on a dissolved cluster");
    }
  }
}

#if 0
#if DOXYGEN_CPP
/**
 * Use this function to add a Seed Instance to the Cluster object
 * \param args : A list of values to be used to add a Seed Instance to the Cluster.
 *
 * This function creates the Default ReplicaSet implicitly and adds the Instance to it
 * This function returns an empty Value.
 */
#else
/**
* Adds a Seed Instance to the Cluster
* \param conn The Connection String or URI of the Instance to be added
*/
#if DOXYGEN_JS
Undefined addSeedInstance(
    String conn, String root_password, String topology_type) {}
#elif DOXYGEN_PY
None add_seed_instance(str conn, str root_password, str topology_type) {}
#endif
/**
* Adds a Seed Instance to the Cluster
* \param doc The Document representing the Instance to be added
*/
#if DOXYGEN_JS
Undefined addSeedInstance(Document doc) {}
#elif DOXYGEN_PY
None add_seed_instance(Document doc) {}
#endif
#endif
#endif
shcore::Value Cluster::add_seed_instance(
    const mysqlshdk::db::Connection_options &connection_options,
    const shcore::Argument_list &args, bool multi_master, bool is_adopted,
    const std::string &replication_user, const std::string &replication_pwd,
    const std::string &group_name) {
  shcore::Value ret_val;

  MetadataStorage::Transaction tx(_metadata_storage);
  std::string default_replication_user =
      "rpl_user";  // Default for V1.0 is rpl_user
  std::shared_ptr<ReplicaSet> default_rs = get_default_replicaset();

  // Check if we have a Default ReplicaSet, if so it means we already added the
  // Seed Instance
  if (default_rs != NULL) {
    uint64_t rs_id = default_rs->get_id();
    if (!_metadata_storage->is_replicaset_empty(rs_id))
      throw shcore::Exception::logic_error(
          "Default ReplicaSet already initialized. Please use: addInstance() "
          "to add more Instances to the ReplicaSet.");
  } else {
    std::string topology_type = ReplicaSet::kTopologyPrimaryMaster;
    if (multi_master) {
      topology_type = ReplicaSet::kTopologyMultiMaster;
    }
    // Create the Default ReplicaSet and assign it to the Cluster's
    // default_replica_set var
    _default_replica_set.reset(
        new ReplicaSet("default", topology_type, _metadata_storage));
    _default_replica_set->set_cluster(shared_from_this());

    // If we reached here without errors we can update the Metadata

    // Update the Cluster table with the Default ReplicaSet on the Metadata
    _metadata_storage->insert_replica_set(_default_replica_set, true,
                                          is_adopted);
  }
  // Add the Instance to the Default ReplicaSet passing already created
  // replication user and the group_name (if provided)
  ret_val = _default_replica_set->add_instance(
      connection_options, args, replication_user, replication_pwd, true,
      group_name);

  std::string group_replication_group_name =
      get_gr_replicaset_group_name(_metadata_storage->get_session());
  _metadata_storage->set_replicaset_group_name(_default_replica_set,
                                               group_replication_group_name);

  tx.commit();

  return ret_val;
}

REGISTER_HELP(CLUSTER_ADDINSTANCE_BRIEF, "Adds an Instance to the cluster.");
REGISTER_HELP(CLUSTER_ADDINSTANCE_PARAM,
              "@param instance An instance "
              "definition.");
REGISTER_HELP(CLUSTER_ADDINSTANCE_PARAM1,
              "@param options Optional dictionary "
              "with options for the operation.");

REGISTER_HELP(CLUSTER_ADDINSTANCE_THROWS,
              "@throws MetadataError if the "
              "Metadata is inaccessible.");
REGISTER_HELP(CLUSTER_ADDINSTANCE_THROWS1,
              "@throws MetadataError if the "
              "Metadata update operation failed.");
REGISTER_HELP(CLUSTER_ADDINSTANCE_THROWS2,
              "@throws ArgumentError if the "
              "instance parameter is empty.");
REGISTER_HELP(CLUSTER_ADDINSTANCE_THROWS3,
              "@throws ArgumentError if the "
              "instance definition is invalid.");
REGISTER_HELP(CLUSTER_ADDINSTANCE_THROWS4,
              "@throws ArgumentError if the "
              "instance definition is a "
              "connection dictionary but empty.");
REGISTER_HELP(CLUSTER_ADDINSTANCE_THROWS5,
              "@throws RuntimeError if the "
              "instance accounts are invalid.");
REGISTER_HELP(CLUSTER_ADDINSTANCE_THROWS6,
              "@throws RuntimeError if the "
              "instance is not in bootstrapped "
              "state.");
REGISTER_HELP(CLUSTER_ADDINSTANCE_THROWS7,
              "@throws ArgumentError if the "
              "value for the memberSslMode "
              "option is not one of the "
              "allowed: \"AUTO\", \"DISABLED\", "
              "\"REQUIRED\".");
REGISTER_HELP(CLUSTER_ADDINSTANCE_THROWS8,
              "@throws RuntimeError if the SSL "
              "mode specified is not compatible "
              "with the one used in the cluster.");
REGISTER_HELP(CLUSTER_ADDINSTANCE_THROWS9,
              "@throws ArgumentError if the value for the ipWhitelist, "\
              "localAddress, or groupSeeds options is empty.");
REGISTER_HELP(CLUSTER_ADDINSTANCE_THROWS10,
              "@throws RuntimeError if the value for the localAddress or "\
              "groupSeeds options is not valid for Group Replication.");

REGISTER_HELP(CLUSTER_ADDINSTANCE_RETURNS, "@returns nothing");

REGISTER_HELP(CLUSTER_ADDINSTANCE_DETAIL,
              "This function adds an Instance to "
              "the default replica set of the "
              "cluster.");

REGISTER_HELP(CLUSTER_ADDINSTANCE_DETAIL1,
              "The instance definition is the connection data for the instance.");

REGISTER_HELP(CLUSTER_ADDINSTANCE_DETAIL2, "TOPIC_CONNECTION_DATA");

REGISTER_HELP(CLUSTER_ADDINSTANCE_DETAIL3,
              "The options dictionary may contain "
              "the following attributes:");
REGISTER_HELP(CLUSTER_ADDINSTANCE_DETAIL4,
              "@li label: an identifier for the "
              "instance being added");
REGISTER_HELP(CLUSTER_ADDINSTANCE_DETAIL5,
              "@li password: the instance "
              "connection password");
REGISTER_HELP(CLUSTER_ADDINSTANCE_DETAIL6,
              "@li memberSslMode: SSL mode used "
              "on the instance");
REGISTER_HELP(CLUSTER_ADDINSTANCE_DETAIL7,
              "@li ipWhitelist: The list of "
              "hosts allowed to connect to the "
              "instance for group replication");
REGISTER_HELP(CLUSTER_ADDINSTANCE_DETAIL8,
              "@li localAddress: string value with the Group Replication "\
              "local address to be used instead of the automatically "\
              "generated one.");
REGISTER_HELP(CLUSTER_ADDINSTANCE_DETAIL9,
              "@li groupSeeds: string value with a comma-separated list of "\
              "the Group Replication peer addresses to be used instead of the "\
              "automatically generated one.");

REGISTER_HELP(CLUSTER_ADDINSTANCE_DETAIL10,
              "The password may be contained on "
              "the instance definition, "
              "however, it can be overwritten "
              "if it is specified on the options.");

REGISTER_HELP(CLUSTER_ADDINSTANCE_DETAIL11,
              "The memberSslMode option supports "
              "these values:");
REGISTER_HELP(CLUSTER_ADDINSTANCE_DETAIL12,
              "@li REQUIRED: if used, SSL "
              "(encryption) will be enabled for "
              "the instance to communicate with "
              "other members of the cluster");
REGISTER_HELP(CLUSTER_ADDINSTANCE_DETAIL13,
              "@li DISABLED: if used, SSL "
              "(encryption) will be disabled");
REGISTER_HELP(CLUSTER_ADDINSTANCE_DETAIL14,
              "@li AUTO: if used, SSL (encryption)"
              " will be automatically "
              "enabled or disabled based on the "
              "cluster configuration");
REGISTER_HELP(CLUSTER_ADDINSTANCE_DETAIL15,
              "If memberSslMode is not specified "
              "AUTO will be used by default.");

REGISTER_HELP(CLUSTER_ADDINSTANCE_DETAIL16,
              "The ipWhitelist format is a comma "
              "separated list of IP "
              "addresses or subnet CIDR "
              "notation, for example: "
              "192.168.1.0/24,10.0.0.1. "
              "By default the "
              "value is set to AUTOMATIC, "
              "allowing addresses from the "
              "instance private network to be "
              "automatically set for "
              "the whitelist.");

REGISTER_HELP(CLUSTER_ADDINSTANCE_DETAIL17,
              "The localAddress and groupSeeds are advanced options and "\
              "their usage is discouraged since incorrect values can lead to "\
              "Group Replication errors.");

REGISTER_HELP(CLUSTER_ADDINSTANCE_DETAIL18,
              "The value for localAddress is used to set the Group "\
              "Replication system variable 'group_replication_local_address'. "\
              "The localAddress option accepts values in the format: "\
              "'host:port' or 'host:' or ':port'. If the specified "\
              "value does not include a colon (:) and it is numeric, then it "\
              "is assumed to be the port, otherwise it is considered to be "\
              "the host. When the host is not specified, the default value is "\
              "the host of the target instance specified as argument. When "\
              "the port is not specified, the default value is the port of "\
              "the target instance + 10000. In case the automatically "\
              "determined default port value is invalid (> 65535) then a "\
              "random value in the range [1000, 65535] is used.");

REGISTER_HELP(CLUSTER_ADDINSTANCE_DETAIL19,
              "The value for groupSeeds is used to set the Group Replication "\
              "system variable 'group_replication_group_seeds'. The "\
              "groupSeeds option accepts a comma-separated list of addresses "
              "in the format: 'host1:port1,...,hostN:portN'.");

/**
 * $(CLUSTER_ADDINSTANCE_BRIEF)
 *
 * $(CLUSTER_ADDINSTANCE_PARAM)
 * $(CLUSTER_ADDINSTANCE_PARAM1)
 *
 * $(CLUSTER_ADDINSTANCE_THROWS)
 * $(CLUSTER_ADDINSTANCE_THROWS1)
 * $(CLUSTER_ADDINSTANCE_THROWS2)
 * $(CLUSTER_ADDINSTANCE_THROWS3)
 * $(CLUSTER_ADDINSTANCE_THROWS4)
 * $(CLUSTER_ADDINSTANCE_THROWS5)
 * $(CLUSTER_ADDINSTANCE_THROWS6)
 * $(CLUSTER_ADDINSTANCE_THROWS7)
 * $(CLUSTER_ADDINSTANCE_THROWS8)
 * $(CLUSTER_ADDINSTANCE_THROWS9)
 * $(CLUSTER_ADDINSTANCE_THROWS10)
 *
 * $(CLUSTER_ADDINSTANCE_RETURNS)
 *
 * $(CLUSTER_ADDINSTANCE_DETAIL)
 *
 * $(CLUSTER_ADDINSTANCE_DETAIL1)
 *
 * \copydoc connection_options
 *
 * Detailed description of the connection data format is available at \ref connection_data
 *
 * $(CLUSTER_ADDINSTANCE_DETAIL3)
 * $(CLUSTER_ADDINSTANCE_DETAIL4)
 * $(CLUSTER_ADDINSTANCE_DETAIL5)
 * $(CLUSTER_ADDINSTANCE_DETAIL6)
 * $(CLUSTER_ADDINSTANCE_DETAIL7)
 * $(CLUSTER_ADDINSTANCE_DETAIL8)
 * $(CLUSTER_ADDINSTANCE_DETAIL9)
 *
 * $(CLUSTER_ADDINSTANCE_DETAIL10)
 *
 * $(CLUSTER_ADDINSTANCE_DETAIL11)
 * $(CLUSTER_ADDINSTANCE_DETAIL12)
 * $(CLUSTER_ADDINSTANCE_DETAIL13)
 * $(CLUSTER_ADDINSTANCE_DETAIL14)
 * $(CLUSTER_ADDINSTANCE_DETAIL15)
 *
 * $(CLUSTER_ADDINSTANCE_DETAIL16)
 *
 * $(CLUSTER_ADDINSTANCE_DETAIL17)
 *
 * $(CLUSTER_ADDINSTANCE_DETAIL18)
 *
 * $(CLUSTER_ADDINSTANCE_DETAIL19)
 */
#if DOXYGEN_JS
Undefined Cluster::addInstance(InstanceDef instance, Dictionary options) {}
#elif DOXYGEN_PY
None Cluster::add_instance(InstanceDef instance, dict options) {}
#endif
shcore::Value Cluster::add_instance(const shcore::Argument_list &args) {
  // Throw an error if the cluster has already been dissolved
  assert_not_dissolved("addInstance");

  args.ensure_count(1, 2, get_function_name("addInstance").c_str());

  _metadata_storage->set_session(_session);

  check_preconditions("addInstance");

  // Add the Instance to the Default ReplicaSet
  shcore::Value ret_val;
  try {
    // Check if we have a Default ReplicaSet
    if (!_default_replica_set)
      throw shcore::Exception::logic_error("ReplicaSet not initialized.");

    auto connection_options =
        mysqlsh::get_connection_options(args, PasswordFormat::OPTIONS);

    shcore::Argument_list rest;
    if (args.size() == 2)
      rest.push_back(args.at(1));

    ret_val = _default_replica_set->add_instance(connection_options, rest);
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name("addInstance"));

  return ret_val;
}

REGISTER_HELP(CLUSTER_REJOININSTANCE_BRIEF,
              "Rejoins an Instance to the cluster.");
REGISTER_HELP(CLUSTER_REJOININSTANCE_PARAM,
              "@param instance An instance "
              "definition.");
REGISTER_HELP(CLUSTER_REJOININSTANCE_PARAM1,
              "@param options Optional dictionary "
              "with options for the operation.");

REGISTER_HELP(CLUSTER_REJOININSTANCE_THROWS,
              "@throws MetadataError if the "
              "Metadata is inaccessible.");
REGISTER_HELP(CLUSTER_REJOININSTANCE_THROWS1,
              "@throws MetadataError if the "
              "Metadata update operation failed.");
REGISTER_HELP(CLUSTER_REJOININSTANCE_THROWS2,
              "@throws RuntimeError if the "
              "instance does not exist.");
REGISTER_HELP(CLUSTER_REJOININSTANCE_THROWS3,
              "@throws RuntimeError if the "
              "instance accounts are invalid.");
REGISTER_HELP(CLUSTER_REJOININSTANCE_THROWS4,
              "@throws RuntimeError if the "
              "instance is not in bootstrapped "
              "state.");
REGISTER_HELP(CLUSTER_REJOININSTANCE_THROWS5,
              "@throws ArgumentError if the "
              "value for the memberSslMode "
              "option is not one of the allowed: "
              "\"AUTO\", \"DISABLED\", \"REQUIRED\".");
REGISTER_HELP(CLUSTER_REJOININSTANCE_THROWS6,
              "@throws RuntimeError if the SSL "
              "mode specified is not compatible "
              "with the one used in the cluster.");
REGISTER_HELP(CLUSTER_REJOININSTANCE_THROWS7,
              "@throws RuntimeError if the "
              "instance is an active member "
              "of the ReplicaSet.");

REGISTER_HELP(CLUSTER_REJOININSTANCE_RETURNS, "@returns nothing.");

REGISTER_HELP(CLUSTER_REJOININSTANCE_DETAIL,
              "This function rejoins an Instance "
              "to the cluster.");

REGISTER_HELP(CLUSTER_REJOININSTANCE_DETAIL1,
              "The instance definition is the connection data for the instance.");

REGISTER_HELP(CLUSTER_REJOININSTANCE_DETAIL2, "TOPIC_CONNECTION_DATA");

REGISTER_HELP(CLUSTER_REJOININSTANCE_DETAIL3,
              "The options dictionary may "
              "contain the following attributes:");
REGISTER_HELP(CLUSTER_REJOININSTANCE_DETAIL4,
              "@li label: an identifier for "
              "the instance being added");
REGISTER_HELP(CLUSTER_REJOININSTANCE_DETAIL5,
              "@li password: the instance "
              "connection password");
REGISTER_HELP(CLUSTER_REJOININSTANCE_DETAIL6,
              "@li memberSslMode: SSL mode "
              "used to be used on the instance");
REGISTER_HELP(CLUSTER_REJOININSTANCE_DETAIL7,
              "@li ipWhitelist: The list of "
              "hosts allowed to connect to the "
              "instance for group replication");

REGISTER_HELP(CLUSTER_REJOININSTANCE_DETAIL8,
              "The password may be contained "
              "on the instance definition, "
              "however, it can be overwritten "
              "if it is specified on the options.");

REGISTER_HELP(CLUSTER_REJOININSTANCE_DETAIL9,
              "The memberSslMode option supports "
              "these values:");
REGISTER_HELP(CLUSTER_REJOININSTANCE_DETAIL10,
              "@li REQUIRED: if used, SSL "
              "(encryption) will be enabled "
              "for the instance to communicate "
              "with other members of the cluster");
REGISTER_HELP(CLUSTER_REJOININSTANCE_DETAIL11,
              "@li DISABLED: if used, SSL "
              "(encryption) will be disabled");
REGISTER_HELP(CLUSTER_REJOININSTANCE_DETAIL12,
              "@li AUTO: if used, SSL "
              "(encryption) will be automatically "
              "enabled or disabled based on the cluster "
              "configuration");
REGISTER_HELP(CLUSTER_REJOININSTANCE_DETAIL13,
              "If memberSslMode is not specified "
              "AUTO will be used by default.");

REGISTER_HELP(CLUSTER_REJOININSTANCE_DETAIL14,
              "The ipWhitelist format is a "
              "comma separated list of IP "
              "addresses or subnet CIDR notation, "
              "for example: 192.168.1.0/24,10.0.0.1. "
              "By default the value is set to "
              "AUTOMATIC, allowing addresses "
              "from the instance private network "
              "to be automatically set for the whitelist.");

/**
* $(CLUSTER_REJOININSTANCE_BRIEF)
*
* $(CLUSTER_REJOININSTANCE_PARAM)
* $(CLUSTER_REJOININSTANCE_PARAM1)
*
* $(CLUSTER_REJOININSTANCE_THROWS)
* $(CLUSTER_REJOININSTANCE_THROWS1)
* $(CLUSTER_REJOININSTANCE_THROWS2)
* $(CLUSTER_REJOININSTANCE_THROWS3)
* $(CLUSTER_REJOININSTANCE_THROWS4)
* $(CLUSTER_REJOININSTANCE_THROWS5)
* $(CLUSTER_REJOININSTANCE_THROWS6)
* $(CLUSTER_REJOININSTANCE_THROWS7)
*
* $(CLUSTER_REJOININSTANCE_RETURNS)
*
* $(CLUSTER_REJOININSTANCE_DETAIL)
*
* $(CLUSTER_REJOININSTANCE_DETAIL1)
*
* \copydoc connection_options
*
* Detailed description of the connection data format is available at \ref connection_data
*
* $(CLUSTER_REJOININSTANCE_DETAIL3)
* $(CLUSTER_REJOININSTANCE_DETAIL4)
* $(CLUSTER_REJOININSTANCE_DETAIL5)
* $(CLUSTER_REJOININSTANCE_DETAIL6)
* $(CLUSTER_REJOININSTANCE_DETAIL7)
*
* $(CLUSTER_REJOININSTANCE_DETAIL8)
*
* $(CLUSTER_REJOININSTANCE_DETAIL9)
* $(CLUSTER_REJOININSTANCE_DETAIL10)
* $(CLUSTER_REJOININSTANCE_DETAIL11)
* $(CLUSTER_REJOININSTANCE_DETAIL12)
* $(CLUSTER_REJOININSTANCE_DETAIL13)
*
* $(CLUSTER_REJOININSTANCE_DETAIL14)
*/
#if DOXYGEN_JS
Undefined Cluster::rejoinInstance(InstanceDef instance, Dictionary options) {}
#elif DOXYGEN_PY
None Cluster::rejoin_instance(InstanceDef instance, dict options) {}
#endif

shcore::Value Cluster::rejoin_instance(const shcore::Argument_list &args) {
  // Throw an error if the cluster has already been dissolved
  assert_not_dissolved("rejoinInstance");

  args.ensure_count(1, 2, get_function_name("rejoinInstance").c_str());

  // Point the metadata session to the cluster session
  _metadata_storage->set_session(_session);

  check_preconditions("rejoinInstance");

  // rejoin the Instance to the Default ReplicaSet
  shcore::Value ret_val;
  try {
    // Check if we have a Default ReplicaSet
    if (!_default_replica_set)
      throw shcore::Exception::logic_error("ReplicaSet not initialized.");

    auto instance_def =
        mysqlsh::get_connection_options(args, mysqlsh::PasswordFormat::OPTIONS);

    shcore::Value::Map_type_ref options;

    if (args.size() == 2)
      options = args.map_at(1);

    // if not, call mysqlprovision to join the instance to its own group
    ret_val = _default_replica_set->rejoin_instance(&instance_def, options);
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name("rejoinInstance"));

  return ret_val;
}

REGISTER_HELP(CLUSTER_REMOVEINSTANCE_BRIEF,
              "Removes an Instance from the cluster.");
REGISTER_HELP(CLUSTER_REMOVEINSTANCE_PARAM,
              "@param instance An instance definition.");
REGISTER_HELP(
    CLUSTER_REMOVEINSTANCE_PARAM1,
    "@param options Optional dictionary with options for the operation.");

REGISTER_HELP(CLUSTER_REMOVEINSTANCE_THROWS,
              "@throws MetadataError if the "
              "Metadata is inaccessible.");
REGISTER_HELP(CLUSTER_REMOVEINSTANCE_THROWS1,
              "@throws MetadataError if the "
              "Metadata update operation failed.");
REGISTER_HELP(CLUSTER_REMOVEINSTANCE_THROWS2,
              "@throws ArgumentError if the instance parameter is empty.");
REGISTER_HELP(CLUSTER_REMOVEINSTANCE_THROWS3,
              "@throws ArgumentError if the instance definition is invalid.");
REGISTER_HELP(CLUSTER_REMOVEINSTANCE_THROWS4,
              "@throws ArgumentError if the instance definition is a "
              "connection dictionary but empty.");
REGISTER_HELP(CLUSTER_REMOVEINSTANCE_THROWS5,
              "@throws RuntimeError if the instance accounts are invalid.");
REGISTER_HELP(CLUSTER_REMOVEINSTANCE_THROWS6,
              "@throws RuntimeError if an error occurs when trying to remove "
              "the instance "
              "(e.g., instance is not reachable).");

REGISTER_HELP(CLUSTER_REMOVEINSTANCE_RETURNS, "@returns nothing.");

REGISTER_HELP(CLUSTER_REMOVEINSTANCE_DETAIL,
              "This function removes an "
              "Instance from the default "
              "replicaSet of the cluster.");

REGISTER_HELP(CLUSTER_REMOVEINSTANCE_DETAIL1,
              "The instance definition is the connection data for the instance.");

REGISTER_HELP(CLUSTER_REMOVEINSTANCE_DETAIL2, "TOPIC_CONNECTION_DATA");

REGISTER_HELP(CLUSTER_REMOVEINSTANCE_DETAIL3,
              "The options dictionary may contain the following attributes:");
REGISTER_HELP(CLUSTER_REMOVEINSTANCE_DETAIL4,
              "@li password/dbPassword: the instance connection password");
REGISTER_HELP(
    CLUSTER_REMOVEINSTANCE_DETAIL5,
    "@li force: boolean, indicating if the instance must be removed (even if "
    "only from metadata) in case it cannot be reached. By default, set to "
    "false.");
REGISTER_HELP(
    CLUSTER_REMOVEINSTANCE_DETAIL6,
    "The password may be contained in the instance definition, however, it can "
    "be overwritten if it is specified on the options.");
REGISTER_HELP(
    CLUSTER_REMOVEINSTANCE_DETAIL7,
    "The force option (set to true) must only be used to remove instances that "
    "are permanently not available (no longer reachable) or never to be reused "
    "again in a cluster. This allows to remove from the metadata an instance "
    "than can no longer be recovered. Otherwise, the instance must be brought "
    "back ONLINE and removed without the force option to avoid errors trying "
    "to add it back to a cluster.");

/**
 * $(CLUSTER_REMOVEINSTANCE_BRIEF)
 *
 * $(CLUSTER_REMOVEINSTANCE_PARAM)
 * $(CLUSTER_REMOVEINSTANCE_PARAM1)
 *
 * $(CLUSTER_REMOVEINSTANCE_THROWS)
 * $(CLUSTER_REMOVEINSTANCE_THROWS1)
 * $(CLUSTER_REMOVEINSTANCE_THROWS2)
 * $(CLUSTER_REMOVEINSTANCE_THROWS3)
 * $(CLUSTER_REMOVEINSTANCE_THROWS4)
 * $(CLUSTER_REMOVEINSTANCE_THROWS5)
 * $(CLUSTER_REMOVEINSTANCE_THROWS6)
 *
 * $(CLUSTER_REMOVEINSTANCE_RETURNS)
 *
 * $(CLUSTER_REMOVEINSTANCE_DETAIL)
 *
 * $(CLUSTER_REMOVEINSTANCE_DETAIL1)
 *
 * \copydoc connection_options
 *
 * Detailed description of the connection data format is available at \ref connection_data
 *
 * $(CLUSTER_REMOVEINSTANCE_DETAIL3)
 * $(CLUSTER_REMOVEINSTANCE_DETAIL4)
 * $(CLUSTER_REMOVEINSTANCE_DETAIL5)
 *
 * $(CLUSTER_REMOVEINSTANCE_DETAIL6)
 *
 * $(CLUSTER_REMOVEINSTANCE_DETAIL7)
 */
#if DOXYGEN_JS
Undefined Cluster::removeInstance(InstanceDef instance, Dictionary options) {}
#elif DOXYGEN_PY
None Cluster::remove_instance(InstanceDef instance, dict options) {}
#endif

shcore::Value Cluster::remove_instance(const shcore::Argument_list &args) {
  // Throw an error if the cluster has already been dissolved
  assert_not_dissolved("removeInstance");

  args.ensure_count(1, 2, get_function_name("removeInstance").c_str());

  // Point the metadata session to the cluster session
  _metadata_storage->set_session(_session);

  check_preconditions("removeInstance");

  // Remove the Instance from the Default ReplicaSet
  try {
        // Check if we have a Default ReplicaSet
    if (!_default_replica_set)
      throw shcore::Exception::logic_error("ReplicaSet not initialized.");

    _default_replica_set->remove_instance(args);
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name("removeInstance"));

  return shcore::Value();
}

#if 0  // Hidden for now
/**
* Returns the ReplicaSet of the given name.
* \sa ReplicaSet
* \param name the name of the ReplicaSet to look for.
* \return the ReplicaSet object matching the name.
*
* Verifies if the requested Collection exist on the metadata schema, if exists, returns the corresponding ReplicaSet object.
*/
#if DOXYGEN_JS
ReplicaSet Cluster::getReplicaSet(String name) {}
#elif DOXYGEN_PY
ReplicaSet Cluster::get_replica_set(str name) {}
#endif
#endif
shcore::Value Cluster::get_replicaset(const shcore::Argument_list &args) {
  // Throw an error if the cluster has already been dissolved
  assert_not_dissolved("getReplicaSet");

  shcore::Value ret_val;

  if (args.size() == 0) {
    ret_val = shcore::Value(
        std::dynamic_pointer_cast<shcore::Object_bridge>(_default_replica_set));
  } else {
    args.ensure_count(1, get_function_name("getReplicaSet").c_str());
    std::string name = args.string_at(0);

    if (name == "default") {
      ret_val = shcore::Value(std::dynamic_pointer_cast<shcore::Object_bridge>(
          _default_replica_set));
    } else {
      /*
       * Retrieve the ReplicaSet from the ReplicaSets array
       */
      // if (found)

      // else
      //  throw shcore::Exception::runtime_error("The ReplicaSet " + _name + "."
      //  + name + " does not exist");
    }
  }

  return ret_val;
}

void Cluster::set_default_replicaset(std::shared_ptr<ReplicaSet> default_rs) {
  _default_replica_set = default_rs;

  if (_default_replica_set)
    _default_replica_set->set_cluster(shared_from_this());
}

REGISTER_HELP(CLUSTER_DESCRIBE_BRIEF, "Describe the structure of the cluster.");
REGISTER_HELP(CLUSTER_DESCRIBE_THROWS,
              "@throws MetadataError if the Metadata is inaccessible.");
REGISTER_HELP(CLUSTER_DESCRIBE_THROWS1,
              "@throws MetadataError if the Metadata update operation failed.");
REGISTER_HELP(
    CLUSTER_DESCRIBE_RETURNS,
    "@returns A JSON object describing the structure of the cluster.");
REGISTER_HELP(CLUSTER_DESCRIBE_DETAIL,
              "This function describes the structure of the cluster including "
              "all its information, ReplicaSets and Instances.");
REGISTER_HELP(CLUSTER_DESCRIBE_DETAIL1,
              "The returned JSON object contains the following attributes:");
REGISTER_HELP(CLUSTER_DESCRIBE_DETAIL2, "@li clusterName: the cluster name");
REGISTER_HELP(CLUSTER_DESCRIBE_DETAIL3,
              "@li defaultReplicaSet: the default replicaSet object");
REGISTER_HELP(
    CLUSTER_DESCRIBE_DETAIL4,
    "The defaultReplicaSet JSON object contains the following attributes:");
REGISTER_HELP(CLUSTER_DESCRIBE_DETAIL5,
              "@li name: the default replicaSet name");
REGISTER_HELP(CLUSTER_DESCRIBE_DETAIL6,
              "@li instances: a List of dictionaries "
              "describing each instance belonging to "
              "the Default ReplicaSet.");
REGISTER_HELP(CLUSTER_DESCRIBE_DETAIL7,
              "Each instance dictionary contains the following attributes:");
REGISTER_HELP(CLUSTER_DESCRIBE_DETAIL8,
              "@li label: the instance name identifier");
REGISTER_HELP(
    CLUSTER_DESCRIBE_DETAIL9,
    "@li host: the instance hostname and IP address in the form of host:port");
REGISTER_HELP(CLUSTER_DESCRIBE_DETAIL10, "@li role: the instance role");

/**
 * $(CLUSTER_DESCRIBE_BRIEF)
 *
 * $(CLUSTER_DESCRIBE_THROWS)
 * $(CLUSTER_DESCRIBE_THROWS1)
 *
 * $(CLUSTER_DESCRIBE_RETURNS)
 *
 * $(CLUSTER_DESCRIBE_DETAIL)
 * $(CLUSTER_DESCRIBE_DETAIL1)
 * $(CLUSTER_DESCRIBE_DETAIL2)
 * $(CLUSTER_DESCRIBE_DETAIL3)
 * $(CLUSTER_DESCRIBE_DETAIL4)
 * $(CLUSTER_DESCRIBE_DETAIL5)
 * $(CLUSTER_DESCRIBE_DETAIL6)
 * $(CLUSTER_DESCRIBE_DETAIL7)
 * $(CLUSTER_DESCRIBE_DETAIL8)
 * $(CLUSTER_DESCRIBE_DETAIL9)
 * $(CLUSTER_DESCRIBE_DETAIL10)
 */
#if DOXYGEN_JS
String Cluster::describe() {}
#elif DOXYGEN_PY
str Cluster::describe() {}
#endif

shcore::Value Cluster::describe(const shcore::Argument_list &args) {
  // Throw an error if the cluster has already been dissolved
  assert_not_dissolved("describe");

  args.ensure_count(0, get_function_name("describe").c_str());

  // Point the metadata session to the cluster session
  _metadata_storage->set_session(_session);

  auto state = check_preconditions("describe");

  bool warning = (state.source_state != ManagedInstance::OnlineRW &&
                  state.source_state != ManagedInstance::OnlineRO);

  shcore::Value ret_val;
  try {
    if (!_metadata_storage->cluster_exists(_name))
      throw shcore::Exception::argument_error("The cluster '" + _name +
                                      "' no longer exists.");

    ret_val = shcore::Value::new_map();

    auto description = ret_val.as_map();

    (*description)["clusterName"] = shcore::Value(_name);

    if (!_default_replica_set)
      (*description)["defaultReplicaSet"] = shcore::Value::Null();
    else
      (*description)["defaultReplicaSet"] =
          _default_replica_set->get_description();

    if (warning) {
      std::string warning =
          "The instance description may be outdated since was generated from "
          "an instance in ";
      warning.append(ManagedInstance::describe(
          static_cast<ManagedInstance::State>(state.source_state)));
      warning.append(" state");
      (*description)["warning"] = shcore::Value(warning);
    }
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name("describe"));

  return ret_val;
}

REGISTER_HELP(CLUSTER_STATUS_BRIEF, "Describe the status of the cluster.");

REGISTER_HELP(CLUSTER_STATUS_THROWS,
              "@throws MetadataError if the Metadata is inaccessible.");
REGISTER_HELP(CLUSTER_STATUS_THROWS1,
              "@throws MetadataError if the Metadata update operation failed.");

REGISTER_HELP(CLUSTER_STATUS_RETURNS,
              "@returns A JSON object describing the status of the cluster.");

REGISTER_HELP(CLUSTER_STATUS_DETAIL,
              "This function describes the status of "
              "the cluster including its ReplicaSets "
              "and Instances.");
REGISTER_HELP(CLUSTER_STATUS_DETAIL1,
              "The returned JSON object contains the following attributes:");
REGISTER_HELP(CLUSTER_STATUS_DETAIL2, "@li clusterName: the cluster name");
REGISTER_HELP(CLUSTER_STATUS_DETAIL3,
              "@li defaultReplicaSet: the default replicaSet object");
REGISTER_HELP(
    CLUSTER_STATUS_DETAIL4,
    "The defaultReplicaSet JSON object contains the following attributes:");
REGISTER_HELP(CLUSTER_STATUS_DETAIL5, "@li name: the default replicaSet name");
REGISTER_HELP(
    CLUSTER_STATUS_DETAIL6,
    "@li primary: the Default ReplicaSet single-master primary instance");
REGISTER_HELP(CLUSTER_STATUS_DETAIL7,
              "@li status: the Default ReplicaSet status");
REGISTER_HELP(CLUSTER_STATUS_DETAIL8,
              "@li statusText: the Default ReplicaSet status descriptive text");
REGISTER_HELP(
    CLUSTER_STATUS_DETAIL9,
    "@li topology: a List of instances belonging to the Default ReplicaSet.");
REGISTER_HELP(
    CLUSTER_STATUS_DETAIL10,
    "Each instance is dictionary containing the following attributes:");
REGISTER_HELP(CLUSTER_STATUS_DETAIL11,
              "@li label: the instance name identifier");
REGISTER_HELP(CLUSTER_STATUS_DETAIL12,
              "@li address: the instance hostname and "
              "IP address in the form of host:port");
REGISTER_HELP(CLUSTER_STATUS_DETAIL14, "@li status: the instance status");
REGISTER_HELP(CLUSTER_STATUS_DETAIL15, "@li role: the instance role");
REGISTER_HELP(CLUSTER_STATUS_DETAIL16, "@li mode: the instance mode");
REGISTER_HELP(
    CLUSTER_STATUS_DETAIL17,
    "@li readReplicas: a List of read replica Instances of the instance.");

/**
 * $(CLUSTER_STATUS_BRIEF)
 *
 * $(CLUSTER_STATUS_THROWS)
 * $(CLUSTER_STATUS_THROWS1)
 *
 * $(CLUSTER_STATUS_RETURNS)
 *
 * $(CLUSTER_STATUS_DETAIL)
 * $(CLUSTER_STATUS_DETAIL1)
 * $(CLUSTER_STATUS_DETAIL2)
 * $(CLUSTER_STATUS_DETAIL3)
 * $(CLUSTER_STATUS_DETAIL4)
 * $(CLUSTER_STATUS_DETAIL5)
 * $(CLUSTER_STATUS_DETAIL6)
 * $(CLUSTER_STATUS_DETAIL7)
 * $(CLUSTER_STATUS_DETAIL8)
 * $(CLUSTER_STATUS_DETAIL10)
 * $(CLUSTER_STATUS_DETAIL11)
 * $(CLUSTER_STATUS_DETAIL12)
 * $(CLUSTER_STATUS_DETAIL13)
 * $(CLUSTER_STATUS_DETAIL14)
 * $(CLUSTER_STATUS_DETAIL15)
 * $(CLUSTER_STATUS_DETAIL16)
 * $(CLUSTER_STATUS_DETAIL17)
 */
#if DOXYGEN_JS
String Cluster::status() {}
#elif DOXYGEN_PY
str Cluster::status() {}
#endif

shcore::Value Cluster::status(const shcore::Argument_list &args) {
  // Throw an error if the cluster has already been dissolved
  assert_not_dissolved("status");

  args.ensure_count(0, get_function_name("status").c_str());

  // Point the metadata session to the cluster session
  _metadata_storage->set_session(_session);

  auto state = check_preconditions("status");

  bool warning = (state.source_state != ManagedInstance::OnlineRW &&
                  state.source_state != ManagedInstance::OnlineRO);

  shcore::Value ret_val;
  try {
    ret_val = shcore::Value::new_map();

    auto status = ret_val.as_map();

    (*status)["clusterName"] = shcore::Value(_name);

    if (!_default_replica_set)
      (*status)["defaultReplicaSet"] = shcore::Value::Null();
    else
      (*status)["defaultReplicaSet"] = _default_replica_set->get_status(state);

    if (warning) {
      std::string warning =
          "The instance status may be inaccurate as it was generated from an "
          "instance in ";
      warning.append(ManagedInstance::describe(
          static_cast<ManagedInstance::State>(state.source_state)));
      warning.append(" state");
      (*status)["warning"] = shcore::Value(warning);
    }
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name("status"));

  return ret_val;
}

REGISTER_HELP(CLUSTER_DISSOLVE_BRIEF, "Dissolves the cluster.");

REGISTER_HELP(CLUSTER_DISSOLVE_THROWS,
              "@throws MetadataError if the Metadata is inaccessible.");
REGISTER_HELP(CLUSTER_DISSOLVE_THROWS1,
              "@throws MetadataError if the Metadata update operation failed.");

REGISTER_HELP(CLUSTER_DISSOLVE_RETURNS, "@returns nothing.");

REGISTER_HELP(CLUSTER_DISSOLVE_PARAM,
              "@param options Optional parameter to "
              "specify if it should deactivate "
              "replication and unregister the "
              "ReplicaSets from the cluster.");
REGISTER_HELP(CLUSTER_DISSOLVE_DETAIL,
              "This function disables replication on "
              "the ReplicaSets, unregisters them and "
              "the the cluster from the metadata.");
REGISTER_HELP(CLUSTER_DISSOLVE_DETAIL1, "It keeps all the user's data intact.");
REGISTER_HELP(CLUSTER_DISSOLVE_DETAIL2,
              "The following is the only option supported:");
REGISTER_HELP(CLUSTER_DISSOLVE_DETAIL3,
              "@li force: boolean, confirms that the "
              "dissolve operation must be executed.");

/**
 * $(CLUSTER_DISSOLVE_BRIEF)
 *
 * $(CLUSTER_DISSOLVE_THROWS)
 * $(CLUSTER_DISSOLVE_THROWS1)
 *
 * $(CLUSTER_DISSOLVE_RETURNS)
 *
 * $(CLUSTER_DISSOLVE_PARAM)
 *
 * $(CLUSTER_DISSOLVE_DETAIL)
 * $(CLUSTER_DISSOLVE_DETAIL1)
 * $(CLUSTER_DISSOLVE_DETAIL2)
 * $(CLUSTER_DISSOLVE_DETAIL3)
 */
#if DOXYGEN_JS
Undefined Cluster::dissolve(Dictionary options) {}
#elif DOXYGEN_PY
None Cluster::dissolve(Dictionary options) {}
#endif

shcore::Value Cluster::dissolve(const shcore::Argument_list &args) {
  // Throw an error if the cluster has already been dissolved
  assert_not_dissolved("dissolve");

  args.ensure_count(0, 1, get_function_name("dissolve").c_str());

  // Point the metadata session to the cluster session
  _metadata_storage->set_session(_session);

  // We need to check if the group has quorum and if not we must abort the
  // operation otherwise GR blocks the writes to preserve the consistency
  // of the group and we end up with a hang.
  // This check is done at check_preconditions()
  check_preconditions("dissolve");

  try {
    shcore::Value::Map_type_ref options;

    bool force = false;
    if (args.size() == 1)
      options = args.map_at(0);

    if (options) {
      // Verification of invalid attributes on the instance creation options
      shcore::Argument_map opt_map(*options);

      opt_map.ensure_keys({}, {"force"}, "dissolve options");

      if (opt_map.has_key("force"))
        force = opt_map.bool_at("force");
    }

    MetadataStorage::Transaction tx(_metadata_storage);
    std::string cluster_name = get_name();

    // check if the Cluster is empty
    if (_metadata_storage->is_cluster_empty(get_id())) {
      _metadata_storage->drop_cluster(cluster_name);
      tx.commit();
      // Set the flag, marking this cluster instance as invalid.
      _dissolved = true;
    } else {
      if (force) {
        // We must stop GR on the online instances only, otherwise we'll
        // get connection failures to the (MISSING) instances
        // BUG#26001653.
        // Get the online instances on the only available replica set
        auto online_instances =
          _metadata_storage->get_replicaset_online_instances(
            _default_replica_set->get_id());

        _metadata_storage->drop_replicaset(_default_replica_set->get_id());

        // TODO(miguel): we only have the Default ReplicaSet
        // but will have more in the future
        _metadata_storage->drop_cluster(cluster_name);

        tx.commit();

        // once the data changes are done, we proceed doing the remove from GR
        _default_replica_set->remove_instances_from_gr(online_instances);

        // Set the flag, marking this cluster instance as invalid.
        _dissolved = true;
      } else {
        throw shcore::Exception::logic_error(
          "Cannot drop cluster: The cluster is not empty.");
      }
    }
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name("dissolve"))

  return shcore::Value();
}

REGISTER_HELP(CLUSTER_RESCAN_BRIEF, "Rescans the cluster.");

REGISTER_HELP(CLUSTER_RESCAN_THROWS,
              "@throws MetadataError if the Metadata is inaccessible.");
REGISTER_HELP(CLUSTER_RESCAN_THROWS1,
              "@throws MetadataError if the Metadata update operation failed.");
REGISTER_HELP(CLUSTER_RESCAN_THROWS2,
              "@throws LogicError if the cluster does not exist.");
REGISTER_HELP(CLUSTER_RESCAN_THROWS3,
              "@throws RuntimeError if all the "
              "ReplicaSet instances of any ReplicaSet "
              "are offline.");

REGISTER_HELP(CLUSTER_RESCAN_RETURNS, "@returns nothing.");

REGISTER_HELP(CLUSTER_RESCAN_DETAIL,
              "This function rescans the cluster for "
              "new Group Replication "
              "members/instances.");

/**
 * $(CLUSTER_RESCAN_BRIEF)
 *
 * $(CLUSTER_RESCAN_THROWS)
 * $(CLUSTER_RESCAN_THROWS1)
 * $(CLUSTER_RESCAN_THROWS2)
 * $(CLUSTER_RESCAN_THROWS3)
 *
 * $(CLUSTER_RESCAN_RETURNS)
 *
 * $(CLUSTER_RESCAN_DETAIL)
 */
#if DOXYGEN_JS
Undefined Cluster::rescan() {}
#elif DOXYGEN_PY
None Cluster::rescan() {}
#endif

shcore::Value Cluster::rescan(const shcore::Argument_list &args) {
  // Throw an error if the cluster has already been dissolved
  assert_not_dissolved("rescan");

  args.ensure_count(0, get_function_name("rescan").c_str());

  // Point the metadata session to the cluster session
  _metadata_storage->set_session(_session);

  check_preconditions("rescan");

  shcore::Value ret_val;
  try {
    ret_val = shcore::Value(_rescan(args));
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name("rescan"));

  return ret_val;
}

shcore::Value::Map_type_ref Cluster::_rescan(
    const shcore::Argument_list &args) {
  shcore::Value::Map_type_ref ret_val(new shcore::Value::Map_type());

  // Check if we have a Default ReplicaSet
  if (!_default_replica_set)
    throw shcore::Exception::logic_error("ReplicaSet not initialized.");

  // Rescan the Default ReplicaSet
  (*ret_val)["defaultReplicaSet"] = _default_replica_set->rescan(args);

  return ret_val;
}

REGISTER_HELP(CLUSTER_FORCEQUORUMUSINGPARTITIONOF_BRIEF,
              "Restores the cluster from quorum loss.");
REGISTER_HELP(
    CLUSTER_FORCEQUORUMUSINGPARTITIONOF_PARAM,
    "@param instance An instance definition to derive the forced group from.");
REGISTER_HELP(
    CLUSTER_FORCEQUORUMUSINGPARTITIONOF_PARAM1,
    "@param password Optional string with the password for the connection.");

REGISTER_HELP(CLUSTER_FORCEQUORUMUSINGPARTITIONOF_THROWS,
              "@throws MetadataError if the instance parameter is empty.");
REGISTER_HELP(CLUSTER_FORCEQUORUMUSINGPARTITIONOF_THROWS1,
              "@throws ArgumentError if the instance parameter is empty.");
REGISTER_HELP(
    CLUSTER_FORCEQUORUMUSINGPARTITIONOF_THROWS2,
    "@throws RuntimeError if the instance does not exist on the Metadata.");
REGISTER_HELP(
    CLUSTER_FORCEQUORUMUSINGPARTITIONOF_THROWS3,
    "@throws RuntimeError if the instance is not on the ONLINE state.");
REGISTER_HELP(CLUSTER_FORCEQUORUMUSINGPARTITIONOF_THROWS4,
              "@throws RuntimeError if the instance does is not an active "
              "member of a replication group.");
REGISTER_HELP(CLUSTER_FORCEQUORUMUSINGPARTITIONOF_THROWS5,
              "@throws RuntimeError if there are no ONLINE instances visible "
              "from the given one.");
REGISTER_HELP(CLUSTER_FORCEQUORUMUSINGPARTITIONOF_THROWS6,
              "@throws LogicError if the cluster does not exist.");

REGISTER_HELP(CLUSTER_FORCEQUORUMUSINGPARTITIONOF_RETURNS, "@returns nothing.");

REGISTER_HELP(CLUSTER_FORCEQUORUMUSINGPARTITIONOF_DETAIL,
              "This function restores the cluster's default replicaset back "
              "into operational status from a loss of quorum scenario. "
              "Such a scenario can occur if a group is partitioned or more "
              "crashes than tolerable occur.");

REGISTER_HELP(CLUSTER_FORCEQUORUMUSINGPARTITIONOF_DETAIL1,
              "The instance definition is the connection data for the instance.");

REGISTER_HELP(CLUSTER_FORCEQUORUMUSINGPARTITIONOF_DETAIL2, "TOPIC_CONNECTION_DATA");

REGISTER_HELP(CLUSTER_FORCEQUORUMUSINGPARTITIONOF_DETAIL3,
              "The options dictionary may contain the following options:");
REGISTER_HELP(CLUSTER_FORCEQUORUMUSINGPARTITIONOF_DETAIL4,
              "@li mycnfPath: The path of the MySQL configuration file for the "
              "instance.");
REGISTER_HELP(CLUSTER_FORCEQUORUMUSINGPARTITIONOF_DETAIL5,
              "@li password: The password to get connected to the instance.");
REGISTER_HELP(
    CLUSTER_FORCEQUORUMUSINGPARTITIONOF_DETAIL6,
    "@li clusterAdmin: The name of the InnoDB cluster administrator user.");
REGISTER_HELP(CLUSTER_FORCEQUORUMUSINGPARTITIONOF_DETAIL7,
              "@li clusterAdminPassword: The password for the InnoDB cluster "
              "administrator account.");

REGISTER_HELP(CLUSTER_FORCEQUORUMUSINGPARTITIONOF_DETAIL8,
              "The password may be contained on the instance definition, "
              "however, it can be overwritten "
              "if it is specified on the options.");

REGISTER_HELP(
    CLUSTER_FORCEQUORUMUSINGPARTITIONOF_DETAIL9,
    "Note that this operation is DANGEROUS as it can create a "
    "split-brain if incorrectly used and should be considered a last "
    "resort. Make absolutely sure that there are no partitions of this group "
    "that are still operating somewhere in the network, but not "
    "accessible from your location.");

REGISTER_HELP(CLUSTER_FORCEQUORUMUSINGPARTITIONOF_DETAIL10,
              "When this function is used, all the members that are ONLINE "
              "from the point of view "
              "of the given instance definition will be added to the group.");

/**
 * $(CLUSTER_FORCEQUORUMUSINGPARTITIONOF_BRIEF)
 *
 * $(CLUSTER_FORCEQUORUMUSINGPARTITIONOF_PARAM)
 * $(CLUSTER_FORCEQUORUMUSINGPARTITIONOF_PARAM1)
 *
 * $(CLUSTER_FORCEQUORUMUSINGPARTITIONOF_THROWS)
 * $(CLUSTER_FORCEQUORUMUSINGPARTITIONOF_THROWS1)
 * $(CLUSTER_FORCEQUORUMUSINGPARTITIONOF_THROWS2)
 * $(CLUSTER_FORCEQUORUMUSINGPARTITIONOF_THROWS3)
 * $(CLUSTER_FORCEQUORUMUSINGPARTITIONOF_THROWS4)
 * $(CLUSTER_FORCEQUORUMUSINGPARTITIONOF_THROWS5)
 * $(CLUSTER_FORCEQUORUMUSINGPARTITIONOF_THROWS6)
 *
 ** $(CLUSTER_FORCEQUORUMUSINGPARTITIONOF_RETURNS)
 *
 * $(CLUSTER_FORCEQUORUMUSINGPARTITIONOF_DETAIL)
 *
 * $(CLUSTER_FORCEQUORUMUSINGPARTITIONOF_DETAIL1)
 *
 * \copydoc connection_options
 *
 * Detailed description of the connection data format is available at \ref connection_data
 *
 * $(CLUSTER_FORCEQUORUMUSINGPARTITIONOF_DETAIL3)
 * $(CLUSTER_FORCEQUORUMUSINGPARTITIONOF_DETAIL4)
 * $(CLUSTER_FORCEQUORUMUSINGPARTITIONOF_DETAIL5)
 * $(CLUSTER_FORCEQUORUMUSINGPARTITIONOF_DETAIL6)
 * $(CLUSTER_FORCEQUORUMUSINGPARTITIONOF_DETAIL7)
 *
 * $(CLUSTER_FORCEQUORUMUSINGPARTITIONOF_DETAIL8)
 *
 * $(CLUSTER_FORCEQUORUMUSINGPARTITIONOF_DETAIL9)
 *
 * $(CLUSTER_FORCEQUORUMUSINGPARTITIONOF_DETAIL10)
 */
#if DOXYGEN_JS
Undefined Cluster::forceQuorumUsingPartitionOf(InstanceDef instance,
                                               String password) {}
#elif DOXYGEN_PY
None Cluster::force_quorum_using_partition_of(InstanceDef instance,
                                              str password) {}
#endif

shcore::Value Cluster::force_quorum_using_partition_of(
    const shcore::Argument_list &args) {
  // Throw an error if the cluster has already been dissolved
  assert_not_dissolved("forceQuorumUsingPartitionOf");

  args.ensure_count(1, 2,
                    get_function_name("forceQuorumUsingPartitionOf").c_str());

  // Point the metadata session to the cluster session
  _metadata_storage->set_session(_session);

  check_preconditions("forceQuorumUsingPartitionOf");

  shcore::Value ret_val;
  try {
    // Check if we have a Default ReplicaSet
    if (!_default_replica_set)
      throw shcore::Exception::logic_error("ReplicaSet not initialized.");

    ret_val = _default_replica_set->force_quorum_using_partition_of(args);
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(
      get_function_name("forceQuorumUsingPartitionOf"));

  return ret_val;
}

void Cluster::set_option(const std::string &option,
                         const shcore::Value &value) {
  if (!_options)
    _options.reset(new shcore::Value::Map_type());

  (*_options)[option] = value;
}

void Cluster::set_attribute(const std::string &attribute,
                            const shcore::Value &value) {
  if (!_attributes)
    _attributes.reset(new shcore::Value::Map_type());

  (*_attributes)[attribute] = value;
}

REGISTER_HELP(CLUSTER_CHECKINSTANCESTATE_BRIEF,
              "Verifies the instance gtid state in relation with the cluster.");
REGISTER_HELP(CLUSTER_CHECKINSTANCESTATE_PARAM,
              "@param instance An instance definition.");
REGISTER_HELP(
    CLUSTER_CHECKINSTANCESTATE_PARAM1,
    "@param password Optional string with the password for the connection.");

REGISTER_HELP(CLUSTER_CHECKINSTANCESTATE_THROWS,
              "@throws ArgumentError if the instance parameter is empty.");
REGISTER_HELP(CLUSTER_CHECKINSTANCESTATE_THROWS1,
              "@throws ArgumentError if the instance definition is invalid.");
REGISTER_HELP(CLUSTER_CHECKINSTANCESTATE_THROWS2,
              "@throws ArgumentError if the instance definition is a "
              "connection dictionary but empty.");
REGISTER_HELP(CLUSTER_CHECKINSTANCESTATE_THROWS3,
              "@throws RuntimeError if the instance accounts are invalid.");
REGISTER_HELP(CLUSTER_CHECKINSTANCESTATE_THROWS4,
              "@throws RuntimeError if the instance is offline.");

REGISTER_HELP(LUSTER_CHECKINSTANCESTATE_RETURNS,
              "@returns resultset A JSON object with the status.");

REGISTER_HELP(CLUSTER_CHECKINSTANCESTATE_DETAIL,
              "Analyzes the instance executed GTIDs with the executed/purged "
              "GTIDs on the cluster "
              "to determine if the instance is valid for the cluster.");

REGISTER_HELP(CLUSTER_CHECKINSTANCESTATE_DETAIL1,
              "The instance definition is the connection data for the instance.");

REGISTER_HELP(CLUSTER_CHECKINSTANCESTATE_DETAIL2, "TOPIC_CONNECTION_DATA");

REGISTER_HELP(CLUSTER_CHECKINSTANCESTATE_DETAIL3,
              "The password may be contained on the instance definition, "
              "however, it can be overwritten "
              "if it is specified as a second parameter.");

REGISTER_HELP(CLUSTER_CHECKINSTANCESTATE_DETAIL4,
              "The returned JSON object contains the following attributes:");
REGISTER_HELP(CLUSTER_CHECKINSTANCESTATE_DETAIL5,
              "@li state: the state of the instance");
REGISTER_HELP(CLUSTER_CHECKINSTANCESTATE_DETAIL6,
              "@li reason: the reason for the state reported");

REGISTER_HELP(CLUSTER_CHECKINSTANCESTATE_DETAIL7,
              "The state of the instance can be one of the following:");
REGISTER_HELP(
    CLUSTER_CHECKINSTANCESTATE_DETAIL8,
    "@li ok: if the instance transaction state is valid for the cluster");
REGISTER_HELP(CLUSTER_CHECKINSTANCESTATE_DETAIL9,
              "@li error: if the instance "
              "transaction state is not "
              "valid for the cluster");
REGISTER_HELP(CLUSTER_CHECKINSTANCESTATE_DETAIL10,
              "The reason for the state reported can be one of the following:");
REGISTER_HELP(CLUSTER_CHECKINSTANCESTATE_DETAIL11,
              "@li new: if the instance doesn’t have any transactions");
REGISTER_HELP(CLUSTER_CHECKINSTANCESTATE_DETAIL12,
              "@li recoverable:  if the instance executed GTIDs are not "
              "conflicting with the executed GTIDs of the cluster instances");
REGISTER_HELP(CLUSTER_CHECKINSTANCESTATE_DETAIL13,
              "@li diverged: if the instance executed GTIDs diverged with the "
              "executed GTIDs of the cluster instances");
REGISTER_HELP(CLUSTER_CHECKINSTANCESTATE_DETAIL14,
              "@li lost_transactions: if the instance has more executed GTIDs "
              "than the executed GTIDs of the cluster instances");

/**
 * $(CLUSTER_CHECKINSTANCESTATE_BRIEF)
 *
 * $(CLUSTER_CHECKINSTANCESTATE_PARAM)
 * $(CLUSTER_CHECKINSTANCESTATE_PARAM1)
 *
 * $(CLUSTER_CHECKINSTANCESTATE_DETAIL)
 *
 * $(CLUSTER_CHECKINSTANCESTATE_DETAIL1)
 *
 * \copydoc connection_options
 *
 * Detailed description of the connection data format is available at \ref connection_data
 *
 * $(CLUSTER_CHECKINSTANCESTATE_DETAIL3)
 *
 * $(CLUSTER_CHECKINSTANCESTATE_DETAIL4)
 * $(CLUSTER_CHECKINSTANCESTATE_DETAIL5)
 * $(CLUSTER_CHECKINSTANCESTATE_DETAIL6)
 *
 * $(CLUSTER_CHECKINSTANCESTATE_DETAIL7)
 * $(CLUSTER_CHECKINSTANCESTATE_DETAIL8)
 * $(CLUSTER_CHECKINSTANCESTATE_DETAIL9)
 *
 * $(CLUSTER_CHECKINSTANCESTATE_DETAIL10)
 * $(CLUSTER_CHECKINSTANCESTATE_DETAIL11)
 * $(CLUSTER_CHECKINSTANCESTATE_DETAIL12)
 * $(CLUSTER_CHECKINSTANCESTATE_DETAIL13)
 * $(CLUSTER_CHECKINSTANCESTATE_DETAIL14)
 */
#if DOXYGEN_JS
Undefined Cluster::checkInstanceState(InstanceDef instance, String password) {}
#elif DOXYGEN_PY
None Cluster::check_instance_state(InstanceDef instance, str password) {}
#endif
shcore::Value Cluster::check_instance_state(const shcore::Argument_list &args) {
  // Throw an error if the cluster has already been dissolved
  assert_not_dissolved("checkInstanceState");

  args.ensure_count(1, 2, get_function_name("checkInstanceState").c_str());

  // Point the metadata session to the cluster session
  _metadata_storage->set_session(_session);

  check_preconditions("checkInstanceState");

  shcore::Value ret_val;
  // Verifies the transaction state of the instance ins relation to the cluster
  try {
    // Check if we have a Default ReplicaSet
    if (!_default_replica_set)
      throw shcore::Exception::logic_error("ReplicaSet not initialized.");

    ret_val = get_default_replicaset()->retrieve_instance_state(args);
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(
      get_function_name("checkInstanceState"));

  return ret_val;
}

ReplicationGroupState Cluster::check_preconditions(
    const std::string &function_name) const {
  // Point the metadata session to the cluster session
  _metadata_storage->set_session(_session);
  return check_function_preconditions(class_name(), function_name,
                                      get_function_name(function_name),
                                      _metadata_storage);
}

}  // namespace dba
}  // namespace mysqlsh
