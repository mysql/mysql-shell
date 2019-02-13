/*
 * Copyright (c) 2016, 2019, Oracle and/or its affiliates. All rights reserved.
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

#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "modules/adminapi/cluster/cluster_options.h"
#include "modules/adminapi/cluster/cluster_set_option.h"
#include "modules/adminapi/cluster/cluster_status.h"
#include "modules/adminapi/common/common.h"
#include "modules/adminapi/common/metadata_storage.h"
#include "modules/adminapi/common/sql.h"
#include "modules/adminapi/mod_dba_cluster.h"
#include "modules/adminapi/replicaset/remove_instance.h"
#include "modules/adminapi/replicaset/replicaset.h"
#include "modules/mysqlxtest_utils.h"
#include "mysqlshdk/include/shellcore/console.h"
#include "mysqlshdk/libs/mysql/group_replication.h"
#include "mysqlshdk/libs/mysql/replication.h"
#include "shellcore/utils_help.h"
#include "utils/debug.h"
#include "utils/utils_general.h"

using std::placeholders::_1;

DEBUG_OBJ_ENABLE(Cluster);

namespace mysqlsh {
namespace dba {

// Documentation of the Cluster Class
REGISTER_HELP_CLASS(Cluster, adminapi);
REGISTER_HELP(CLUSTER_BRIEF, "Represents an InnoDB cluster.");
REGISTER_HELP(CLUSTER_DETAIL,
              "The cluster object is the entry point to manage and monitor "
              "a MySQL InnoDB cluster.");
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

Cluster::Cluster(const std::string &name,
                 std::shared_ptr<mysqlshdk::db::ISession> group_session,
                 std::shared_ptr<MetadataStorage> metadata_storage)
    : _name(name),
      m_invalidated(false),
      _group_session(group_session),
      _metadata_storage(metadata_storage) {
  DEBUG_OBJ_ALLOC2(Cluster, [](void *ptr) {
    return "refs:" + std::to_string(reinterpret_cast<Cluster *>(ptr)
                                        ->shared_from_this()
                                        .use_count());
  });
  init();
}

Cluster::~Cluster() { DEBUG_OBJ_DEALLOC(Cluster); }

std::string &Cluster::append_descr(std::string &s_out, int UNUSED(indent),
                                   int UNUSED(quote_strings)) const {
  s_out.append("<" + class_name() + ":" + _name + ">");
  return s_out;
}

void Cluster::append_json(shcore::JSON_dumper &dumper) const {
  dumper.start_object();
  dumper.append_string("class", class_name());
  dumper.append_string("name", _name);
  dumper.end_object();
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
  add_method("describe", std::bind(&Cluster::describe, this, _1));
  add_method("status", std::bind(&Cluster::status, this, _1));
  add_varargs_method("dissolve", std::bind(&Cluster::dissolve, this, _1));

  expose<shcore::Value, const std::string &, Cluster>(
      "checkInstanceState", &Cluster::check_instance_state, "instanceDef");
  expose<shcore::Value, const shcore::Dictionary_t &, Cluster>(
      "checkInstanceState", &Cluster::check_instance_state, "instanceDef");

  expose("rescan", &Cluster::rescan, "?options");
  add_varargs_method(
      "forceQuorumUsingPartitionOf",
      std::bind(&Cluster::force_quorum_using_partition_of, this, _1));
  add_method("disconnect", std::bind(&Cluster::disconnect, this, _1));

  expose<void, const std::string &, Cluster>(
      "switchToSinglePrimaryMode", &Cluster::switch_to_single_primary_mode,
      "instanceDef");
  expose<void, const shcore::Dictionary_t &, Cluster>(
      "switchToSinglePrimaryMode", &Cluster::switch_to_single_primary_mode,
      "instanceDef");
  expose("switchToSinglePrimaryMode", &Cluster::switch_to_single_primary_mode);
  expose("switchToMultiPrimaryMode", &Cluster::switch_to_multi_primary_mode);
  expose<void, const std::string &, Cluster>(
      "setPrimaryInstance", &Cluster::set_primary_instance, "instanceDef");
  expose<void, const shcore::Dictionary_t &, Cluster>(
      "setPrimaryInstance", &Cluster::set_primary_instance, "instanceDef");

  expose("options", &Cluster::options, "?options");

  expose("setOption", &Cluster::set_option, "option", "value");

  expose<void, const shcore::Dictionary_t &, const std::string &,
         const shcore::Value &, Cluster>("setInstanceOption",
                                         &Cluster::set_instance_option,
                                         "instanceDef", "option", "value");
  expose<void, const std::string &, const std::string &, const shcore::Value &,
         Cluster>("setInstanceOption", &Cluster::set_instance_option,
                  "instanceDef", "option", "value");
}

// Documentation of the getName function
REGISTER_HELP_FUNCTION(getName, Cluster);
REGISTER_HELP_PROPERTY(name, Cluster);
REGISTER_HELP(CLUSTER_NAME_BRIEF, "${CLUSTER_GETNAME_BRIEF}");
REGISTER_HELP_FUNCTION_TEXT(CLUSTER_GETNAME, R"*(
Retrieves the name of the cluster.

@returns The name of the cluster.
)*");

/**
 * $(CLUSTER_GETNAME_BRIEF)
 *
 * $(CLUSTER_GETNAME)
 */
#if DOXYGEN_JS
String Cluster::getName() {}
#elif DOXYGEN_PY
str Cluster::get_name() {}
#endif

shcore::Value Cluster::call(const std::string &name,
                            const shcore::Argument_list &args) {
  // Throw an error if the cluster has already been dissolved
  assert_valid(name);
  return Cpp_object_bridge::call(name, args);
}

shcore::Value Cluster::get_member(const std::string &prop) const {
  shcore::Value ret_val;

  // Throw an error if the cluster has already been dissolved
  assert_valid(prop);

  if (prop == "name")
    ret_val = shcore::Value(_name);
  else
    ret_val = shcore::Cpp_object_bridge::get_member(prop);
  return ret_val;
}

std::shared_ptr<mysqlshdk::innodbcluster::Metadata_mysql> Cluster::metadata()
    const {
  return _metadata_storage->get_new_metadata();
}

void Cluster::assert_valid(const std::string &option_name) const {
  std::string name;

  if (option_name == "disconnect") return;

  if (has_member(option_name) && m_invalidated) {
    if (has_method(option_name)) {
      name = get_function_name(option_name, false);
      throw shcore::Exception::runtime_error(class_name() + "." + name + ": " +
                                             "Can't call function '" + name +
                                             "' on a dissolved cluster");
    } else {
      name = get_member_name(option_name, naming_style);
      throw shcore::Exception::runtime_error(class_name() + "." + name + ": " +
                                             "Can't access object member '" +
                                             name + "' on a dissolved cluster");
    }
  }
  if (!_group_session) {
    throw shcore::Exception::runtime_error(
        "The cluster object is disconnected. Please use <Dba>." +
        get_function_name("getCluster", false) +
        " to obtain a fresh cluster handle.");
  }
}

shcore::Value Cluster::add_seed_instance(
    mysqlshdk::mysql::IInstance *target_instance,
    const Group_replication_options &gr_options, bool multi_primary,
    bool is_adopted, const std::string &replication_user,
    const std::string &replication_pwd) {
  shcore::Value ret_val;

  MetadataStorage::Transaction tx(_metadata_storage);
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
    // Create the Default ReplicaSet and assign it to the Cluster's
    // default_replica_set var
    create_default_replicaset("default", multi_primary, "", is_adopted);
  }
  if (!is_adopted) {
    // Add the Instance to the Default ReplicaSet passing already created
    // replication user and the group_name (if provided)
    ret_val = _default_replica_set->add_instance({}, target_instance,
                                                 gr_options, replication_user,
                                                 replication_pwd, true, true);
  }
  std::string group_replication_group_name =
      get_gr_replicaset_group_name(_group_session);
  _default_replica_set->set_group_name(group_replication_group_name);

  tx.commit();

  return ret_val;
}

REGISTER_HELP_FUNCTION(addInstance, Cluster);
REGISTER_HELP_FUNCTION_TEXT(CLUSTER_ADDINSTANCE, R"*(
Adds an Instance to the cluster.

@param instance Connection options for the target instance to be added.
@param options Optional dictionary with options for the operation.

@returns nothing

This function adds an Instance to the default replica set of the cluster.

${TOPIC_CONNECTION_MORE_INFO_TCP_ONLY}

The options dictionary may contain the following attributes:

@li label: an identifier for the instance being added
@li password: the instance connection password
@li memberSslMode: SSL mode used on the instance
@li ipWhitelist: The list of hosts allowed to connect to the instance for group
replication
@li localAddress: string value with the Group Replication local address to be
used instead of the automatically generated one.
@li groupSeeds: string value with a comma-separated list of the Group
Replication peer addresses to be used instead of the automatically generated
one.
@li exitStateAction: string value indicating the group replication exit state
action.
@li memberWeight: integer value with a percentage weight for automatic primary
election on failover.

The password may be contained on the instance definition, however, it can be
overwritten if it is specified on the options.

@attention The memberSslMode option will be removed in a future release.

The memberSslMode option supports the following values:

@li REQUIRED: if used, SSL (encryption) will be enabled for the instance to
communicate with other members of the cluster
@li DISABLED: if used, SSL (encryption) will be disabled
@li AUTO: if used, SSL (encryption) will be automatically enabled or disabled
based on the cluster configuration

If memberSslMode is not specified AUTO will be used by default.

The exitStateAction option supports the following values:

@li ABORT_SERVER: if used, the instance shuts itself down if it leaves the
cluster unintentionally.
@li READ_ONLY: if used, the instance switches itself to super-read-only mode if
it leaves the cluster unintentionally.

If exitStateAction is not specified READ_ONLY will be used by default.

The ipWhitelist format is a comma separated list of IP addresses or subnet CIDR
notation, for example: 192.168.1.0/24,10.0.0.1. By default the value is set to
AUTOMATIC, allowing addresses from the instance private network to be
automatically set for the whitelist.

The localAddress and groupSeeds are advanced options and their usage is
discouraged since incorrect values can lead to Group Replication errors.

The value for localAddress is used to set the Group Replication system variable
'group_replication_local_address'. The localAddress option accepts values in
the format: 'host:port' or 'host:' or ':port'. If the specified value does not
include a colon (:) and it is numeric, then it is assumed to be the port,
otherwise it is considered to be the host. When the host is not specified, the
default value is the value of the system variable 'report_host' if defined
(i.e., not 'NULL'), otherwise it is the hostname value. When the port is not
specified, the default value is the port of the target instance * 10 + 1. In
case the automatically determined default port value is invalid (> 65535) then
a random value in the range [10000, 65535] is used.

The value for groupSeeds is used to set the Group Replication system variable
'group_replication_group_seeds'. The groupSeeds option accepts a
comma-separated list of addresses in the format: 'host1:port1,...,hostN:portN'.

The value for exitStateAction is used to configure how Group Replication
behaves when a server instance leaves the group unintentionally, for example
after encountering an applier error. When set to ABORT_SERVER, the instance
shuts itself down, and when set to READ_ONLY the server switches itself to
super-read-only mode. The exitStateAction option accepts case-insensitive
string values, being the accepted values: ABORT_SERVER (or 1) and READ_ONLY (or
0). The default value is READ_ONLY.

The value for memberWeight is used to set the Group Replication system variable
'group_replication_member_weight'. The memberWeight option accepts integer
values. Group Replication limits the value range from 0 to 100, automatically
adjusting it if a lower/bigger value is provided. Group Replication uses a
default value of 50 if no value is provided.

@throw MetadataError in the following scenarios:
@li If the Metadata is inaccessible.
@li If the Metadata update operation failed.

@throw ArgumentError in the following scenarios:
@li If the instance parameter is empty.
@li If the instance definition is invalid.
@li If the instance definition is a connection dictionary but empty.
@li If the value for the memberSslMode option is not one of the allowed:
"AUTO", "DISABLED", "REQUIRED".
@li If the value for the ipWhitelist, localAddress, groupSeeds, or
exitStateAction options is empty.
@li If the instance definition cannot be used for Group Replication.

@throw RuntimeError in the following scenarios:
@li If the instance accounts are invalid.
@li If the instance is not in bootstrapped state.
@li If the SSL mode specified is not compatible with the one used in the
cluster.
@li If the value for the localAddress, groupSeeds, exitStateAction, or
memberWeight options is not valid for Group Replication.
)*");

/**
 * $(CLUSTER_ADDINSTANCE_BRIEF)
 *
 * $(CLUSTER_ADDINSTANCE)
 */
#if DOXYGEN_JS
Undefined Cluster::addInstance(InstanceDef instance, Dictionary options) {}
#elif DOXYGEN_PY
None Cluster::add_instance(InstanceDef instance, dict options) {}
#endif
shcore::Value Cluster::add_instance(const shcore::Argument_list &args) {
  // Throw an error if the cluster has already been dissolved
  assert_valid("addInstance");

  args.ensure_count(1, 2, get_function_name("addInstance").c_str());

  // Add the Instance to the Default ReplicaSet
  shcore::Value ret_val;
  try {
    check_preconditions("addInstance");

    // Check if we have a Default ReplicaSet
    if (!_default_replica_set)
      throw shcore::Exception::logic_error("ReplicaSet not initialized.");

    auto connection_options =
        mysqlsh::get_connection_options(args, PasswordFormat::OPTIONS);

    validate_connection_options(connection_options);

    shcore::Dictionary_t rest;
    if (args.size() == 2) rest = args.at(1).as_map();

    ret_val = _default_replica_set->add_instance_(connection_options, rest);
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name("addInstance"));

  return ret_val;
}

REGISTER_HELP_FUNCTION(rejoinInstance, Cluster);
REGISTER_HELP_FUNCTION_TEXT(CLUSTER_REJOININSTANCE, R"*(
Rejoins an Instance to the cluster.

@param instance An instance definition.
@param options Optional dictionary with options for the operation.

@returns Nothing.

This function rejoins an Instance to the cluster.

The instance definition is the connection data for the instance.

${TOPIC_CONNECTION_MORE_INFO_TCP_ONLY}

The options dictionary may contain the following attributes:

@li label: an identifier for the instance being added
@li password: the instance connection password
@li memberSslMode: SSL mode used on the instance
@li ipWhitelist: The list of hosts allowed to connect to the instance for group
replication

The password may be contained on the instance definition, however, it can be
overwritten if it is specified on the options.

@attention The memberSslMode option will be removed in a future release.

The memberSslMode option supports these values:

@li REQUIRED: if used, SSL (encryption) will be enabled for the instance to
communicate with other members of the cluster
@li DISABLED: if used, SSL (encryption) will be disabled
@li AUTO: if used, SSL (encryption) will be automatically enabled or disabled
based on the cluster configuration

If memberSslMode is not specified AUTO will be used by default.

The ipWhitelist format is a comma separated list of IP addresses or subnet CIDR
notation, for example: 192.168.1.0/24,10.0.0.1. By default the value is set to
AUTOMATIC, allowing addresses from the instance private network to be
automatically set for the whitelist.

@throw MetadataError in the following scenarios:
@li If the Metadata is inaccessible.
@li If the Metadata update operation failed.

@throw ArgumentError in the following scenarios:
@li If the value for the memberSslMode option is not one of the allowed:
"AUTO", "DISABLED", "REQUIRED".
@li If the instance definition cannot be used for Group Replication.

@throw RuntimeError in the following scenarios:
@li If the instance does not exist.
@li If the instance accounts are invalid.
@li If the instance is not in bootstrapped state.
@li If the SSL mode specified is not compatible with the one used in the
cluster.
@li If the instance is an active member of the ReplicaSet.
)*");

/**
 * $(CLUSTER_REJOININSTANCE_BRIEF)
 *
 * $(CLUSTER_REJOININSTANCE)
 */
#if DOXYGEN_JS
Undefined Cluster::rejoinInstance(InstanceDef instance, Dictionary options) {}
#elif DOXYGEN_PY
None Cluster::rejoin_instance(InstanceDef instance, dict options) {}
#endif

shcore::Value Cluster::rejoin_instance(const shcore::Argument_list &args) {
  // Throw an error if the cluster has already been dissolved
  assert_valid("rejoinInstance");

  args.ensure_count(1, 2, get_function_name("rejoinInstance").c_str());

  // rejoin the Instance to the Default ReplicaSet
  shcore::Value ret_val;
  try {
    check_preconditions("rejoinInstance");

    // Check if we have a Default ReplicaSet
    if (!_default_replica_set)
      throw shcore::Exception::logic_error("ReplicaSet not initialized.");

    auto instance_def =
        mysqlsh::get_connection_options(args, mysqlsh::PasswordFormat::OPTIONS);

    validate_connection_options(instance_def);

    shcore::Value::Map_type_ref options;

    if (args.size() == 2) options = args.map_at(1);

    // if not, call mysqlprovision to join the instance to its own group
    ret_val = _default_replica_set->rejoin_instance(&instance_def, options);
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name("rejoinInstance"));

  return ret_val;
}

REGISTER_HELP_FUNCTION(removeInstance, Cluster);
REGISTER_HELP_FUNCTION_TEXT(CLUSTER_REMOVEINSTANCE, R"*(
Removes an Instance from the cluster.

@param instance An instance definition.
@param options Optional dictionary with options for the operation.

@returns Nothing.

This function removes an Instance from the default replicaSet of the cluster.

The instance definition is the connection data for the instance.

${TOPIC_CONNECTION_MORE_INFO_TCP_ONLY}

The options dictionary may contain the following attributes:

@li password: the instance connection password
@li force: boolean, indicating if the instance must be removed (even if only
from metadata) in case it cannot be reached. By default, set to false.
@li interactive: boolean value used to disable/enable the wizards in the
command execution, i.e. prompts and confirmations will be provided or not
according to the value set. The default value is equal to MySQL Shell wizard
mode.

The password may be contained in the instance definition, however, it can be
overwritten if it is specified on the options.

The force option (set to true) must only be used to remove instances that are
permanently not available (no longer reachable) or never to be reused again in
a cluster. This allows to remove from the metadata an instance than can no
longer be recovered. Otherwise, the instance must be brought back ONLINE and
removed without the force option to avoid errors trying to add it back to a
cluster.

@throw MetadataError in the following scenarios:
@li If the Metadata is inaccessible.
@li If the Metadata update operation failed.

@throw ArgumentError in the following scenarios:
@li If the instance parameter is empty.
@li If the instance definition is invalid.
@li If the instance definition is a connection dictionary but empty.
@li If the instance definition cannot be used for Group Replication.

@throw RuntimeError in the following scenarios:
@li If the instance accounts are invalid.
@li If an error occurs when trying to remove the instance (e.g., instance is
not reachable).
)*");

/**
 * $(CLUSTER_REMOVEINSTANCE_BRIEF)
 *
 * $(CLUSTER_REMOVEINSTANCE)
 */
#if DOXYGEN_JS
Undefined Cluster::removeInstance(InstanceDef instance, Dictionary options) {}
#elif DOXYGEN_PY
None Cluster::remove_instance(InstanceDef instance, dict options) {}
#endif

shcore::Value Cluster::remove_instance(const shcore::Argument_list &args) {
  shcore::Value ret_val;

  // Check arguments count.
  // NOTE: check for arguments need to be performed here for the correct
  // context "Cluster.removeInstance" to be used in the error message
  // (not ReplicaSet.removeInstance).
  args.ensure_count(1, 2, get_function_name("removeInstance").c_str());

  // Throw an error if the cluster has already been dissolved
  assert_valid("removeInstance");

  // Remove the Instance from the Default ReplicaSet
  try {
    check_preconditions("removeInstance");

    // Check if we have a Default ReplicaSet
    if (!_default_replica_set)
      throw shcore::Exception::logic_error("ReplicaSet not initialized.");

    ret_val = _default_replica_set->remove_instance(args);
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name("removeInstance"));

  return ret_val;
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
  assert_valid("getReplicaSet");

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

void Cluster::set_default_replicaset(const std::string &name,
                                     const std::string &topology_type,
                                     const std::string &group_name) {
  _default_replica_set = std::make_shared<ReplicaSet>(
      name, topology_type, group_name, _metadata_storage);

  _default_replica_set->set_cluster(shared_from_this());
}

std::shared_ptr<ReplicaSet> Cluster::create_default_replicaset(
    const std::string &name, bool multi_primary, const std::string &group_name,
    bool is_adopted) {
  std::string topology_type = ReplicaSet::kTopologySinglePrimary;
  if (multi_primary) {
    topology_type = ReplicaSet::kTopologyMultiPrimary;
  }
  _default_replica_set = std::make_shared<ReplicaSet>(
      name, topology_type, group_name, _metadata_storage);

  _default_replica_set->set_cluster(shared_from_this());

  // Update the Cluster table with the Default ReplicaSet on the Metadata
  _metadata_storage->insert_replica_set(_default_replica_set, true, is_adopted);

  return _default_replica_set;
}

REGISTER_HELP_FUNCTION(describe, Cluster);
REGISTER_HELP_FUNCTION_TEXT(CLUSTER_DESCRIBE, R"*(
Describe the structure of the cluster.

@returns A JSON object describing the structure of the cluster.

This function describes the structure of the cluster including all its
information, ReplicaSets and Instances.

The returned JSON object contains the following attributes:

@li clusterName: the cluster name
@li defaultReplicaSet: the default ReplicaSet object

The defaultReplicaSet JSON object contains the following attributes:

@li name: the ReplicaSet name
@li topology: a list of dictionaries describing each instance belonging to the
ReplicaSet.

Each instance dictionary contains the following attributes:

@li address: the instance address in the form of host:port
@li label: the instance name identifier
@li role: the instance role

@throw MetadataError in the following scenarios:
@li If the Metadata is inaccessible.
@li If the Metadata update operation failed.
)*");

/**
 * $(CLUSTER_DESCRIBE_BRIEF)
 *
 * $(CLUSTER_DESCRIBE)
 */
#if DOXYGEN_JS
String Cluster::describe() {}
#elif DOXYGEN_PY
str Cluster::describe() {}
#endif

shcore::Value Cluster::describe(const shcore::Argument_list &args) {
  // Throw an error if the cluster has already been dissolved
  assert_valid("describe");

  args.ensure_count(0, get_function_name("describe").c_str());

  shcore::Value ret_val;
  try {
    auto state = check_preconditions("describe");

    bool warning = (state.source_state != ManagedInstance::OnlineRW &&
                    state.source_state != ManagedInstance::OnlineRO);

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

REGISTER_HELP_FUNCTION(status, Cluster);
REGISTER_HELP_FUNCTION_TEXT(CLUSTER_STATUS, R"*(
Describe the status of the cluster.

@param options Optional dictionary with options.

@returns A JSON object describing the status of the cluster.

This function describes the status of the cluster including its ReplicaSets and
Instances. The following options may be given to control the amount of
information gathered and returned.

@li extended: if true, includes information about transactions processed by
connection and applier, as well as groupName and memberId values.
@li queryMembers: if true, connect to each Instance of the ReplicaSets to query
for more detailed stats about the replication machinery.

@throw MetadataError in the following scenarios:
@li If the Metadata is inaccessible.
@li If the Metadata update operation failed.
)*");

/**
 * $(CLUSTER_STATUS_BRIEF)
 *
 * $(CLUSTER_STATUS)
 */
#if DOXYGEN_JS
String Cluster::status(Dictionary options) {}
#elif DOXYGEN_PY
str Cluster::status(dict options) {}
#endif

shcore::Value Cluster::status(const shcore::Argument_list &args) {
  // Throw an error if the cluster has already been dissolved
  assert_valid("status");

  args.ensure_count(0, 1, get_function_name("status").c_str());

  bool query_members = false;
  bool extended = false;
  if (args.size() > 0) {
    shcore::Value::Map_type_ref options = args.map_at(0);

    Unpack_options(options)
        .optional("extended", &extended)
        .optional("queryMembers", &query_members)
        .end();
  }

  shcore::Value ret_val;
  try {
    auto state = check_preconditions("status");

    bool warning = (state.source_state != ManagedInstance::OnlineRW &&
                    state.source_state != ManagedInstance::OnlineRO);

    // Create the Cluster_status command and execute it.
    Cluster_status op_status(*this, extended, query_members);
    // Always execute finish when leaving "try catch".
    auto finally =
        shcore::on_leave_scope([&op_status]() { op_status.finish(); });
    // Prepare the Cluster_status command execution (validations).
    op_status.prepare();
    // Execute Cluster_status operations.
    ret_val = op_status.execute();

    if (warning) {
      std::string warning =
          "The instance status may be inaccurate as it was generated from an "
          "instance in ";
      warning.append(ManagedInstance::describe(
          static_cast<ManagedInstance::State>(state.source_state)));
      warning.append(" state");
      (*ret_val.as_map())["warning"] = shcore::Value(warning);
    }
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name("status"));

  return ret_val;
}

REGISTER_HELP_FUNCTION(options, Cluster);
REGISTER_HELP_FUNCTION_TEXT(CLUSTER_OPTIONS, R"*(
Lists the cluster configuration options.

@param options Optional Dictionary with options.

@returns A JSON object describing the configuration options of the cluster.

This function lists the cluster configuration options for its ReplicaSets and
Instances. The following options may be given to controlthe amount of
information gathered and returned.

@li all: if true, includes information about all group_replication system
variables.

@throw MetadataError in the following scenarios:
@li If the Metadata is inaccessible.
)*");

/**
 * $(CLUSTER_OPTIONS_BRIEF)
 *
 * $(CLUSTER_OPTIONS)
 */
#if DOXYGEN_JS
String Cluster::options(Dictionary options) {}
#elif DOXYGEN_PY
str Cluster::options(dict options) {}
#endif

shcore::Value Cluster::options(const shcore::Dictionary_t &options) {
  // Throw an error if the cluster has already been dissolved
  assert_valid("options");
  auto state = check_preconditions("options");

  bool all = false;
  // Retrieves optional options
  Unpack_options(options).optional("all", &all).end();

  // Create the Cluster_options command and execute it.
  Cluster_options op_option(*this, all);
  // Always execute finish when leaving "try catch".
  auto finally = shcore::on_leave_scope([&op_option]() { op_option.finish(); });
  // Prepare the Cluster_options command execution (validations).
  op_option.prepare();
  // Execute Cluster_options operations.
  return op_option.execute();
}

REGISTER_HELP_FUNCTION(dissolve, Cluster);
REGISTER_HELP_FUNCTION_TEXT(CLUSTER_DISSOLVE, R"*(
Dissolves the cluster.

@param options Optional parameter to specify if it should deactivate
replication and unregister the ReplicaSets from the cluster.

@returns Nothing.

This function disables replication on the ReplicaSets, unregisters them and the
the cluster from the metadata.

It keeps all the user's data intact.

The options dictionary may contain the following attributes:

@li force: boolean value used to confirm that the dissolve operation must be
executed, even if some members of the cluster cannot be reached or the timeout
was reached when waiting for members to catch up with replication changes. By
default, set to false.
@li interactive: boolean value used to disable/enable the wizards in the
command execution, i.e. prompts and confirmations will be provided or not
according to the value set. The default value is equal to MySQL Shell wizard
mode.

The force option (set to true) must only be used to dissolve a cluster with
instances that are permanently not available (no longer reachable) or never to
be reused again in a cluster. This allows to dissolve a cluster and remove it
from the metadata, including instances than can no longer be recovered.
Otherwise, the instances must be brought back ONLINE and the cluster dissolved
without the force option to avoid errors trying to reuse the instances and add
them back to a cluster.

@throw MetadataError in the following scenarios:
@li If the Metadata is inaccessible.
@li If the Metadata update operation failed.
)*");

/**
 * $(CLUSTER_DISSOLVE_BRIEF)
 *
 * $(CLUSTER_DISSOLVE)
 */
#if DOXYGEN_JS
Undefined Cluster::dissolve(Dictionary options) {}
#elif DOXYGEN_PY
None Cluster::dissolve(dict options) {}
#endif

shcore::Value Cluster::dissolve(const shcore::Argument_list &args) {
  shcore::Value ret_val;

  // Check arguments count.
  // NOTE: check for arguments need to be performed here for the correct
  // context "Cluster.removeInstance" to be used in the error message
  // (not ReplicaSet.removeInstance).
  args.ensure_count(0, 1, get_function_name("dissolve").c_str());

  // Throw an error if the cluster has already been dissolved
  assert_valid("dissolve");

  // Dissolve the default replicaset.
  try {
    // We need to check if the group has quorum and if not we must abort the
    // operation otherwise GR blocks the writes to preserve the consistency
    // of the group and we end up with a hang.
    // This check is done at check_preconditions()
    check_preconditions("dissolve");

    // Check if we have a Default ReplicaSet
    if (!_default_replica_set)
      throw shcore::Exception::logic_error("ReplicaSet not initialized.");

    ret_val = _default_replica_set->dissolve(args);
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name("dissolve"))

  return ret_val;
}

REGISTER_HELP_FUNCTION(rescan, Cluster);
REGISTER_HELP_FUNCTION_TEXT(CLUSTER_RESCAN, R"*(
Rescans the cluster.

@param options Optional Dictionary with options for the operation.

@returns Nothing.

This function rescans the cluster for new and obsolete Group Replication
members/instances, as well as changes in the used topology mode (i.e.,
single-primary and multi-primary).

The options dictionary may contain the following attributes:

@li addInstances: List with the connection data of the new active instances to
add to the metadata, or "auto" to automatically add missing instances to the
metadata.
@li interactive: boolean value used to disable/enable the wizards in the
command execution, i.e. prompts and confirmations will be provided or not
according to the value set. The default value is equal to MySQL Shell wizard
mode.
@li removeInstances: List with the connection data of the obsolete instances to
remove from the metadata, or "auto" to automatically remove obsolete instances
from the metadata.
@li updateTopologyMode: boolean value used to indicate if the topology mode
(single-primary or multi-primary) in the metadata should be updated (true) or
not (false) to match the one being used by the cluster. By default, the
metadata is not updated (false).

The value for addInstances and removeInstances is used to specify which
instances to add or remove from the metadata, respectively. Both options accept
list connection data. In addition, the "auto" value can be used for both
options in order to automatically add or remove the instances in the metadata,
without having to explicitly specify them.

@throw ArgumentError in the following scenarios:
@li If the value for `addInstances` or `removeInstance` is empty.
@li If the value for `addInstances` or `removeInstance` is invalid.

@throw MetadataError in the following scenarios:
@li If the Metadata is inaccessible.
@li If the Metadata update operation failed.

@throw LogicError in the following scenarios:
@li If the cluster does not exist.

@throw RuntimeError in the following scenarios:
@li If all the ReplicaSet instances of any ReplicaSet are offline.
@li If an instance specified for `addInstances` is not an active member of the
replication group.
@li If an instance specified for `removeInstances` is an active member of the
replication group.
)*");

/**
 * $(CLUSTER_RESCAN_BRIEF)
 *
 * $(CLUSTER_RESCAN)
 */
#if DOXYGEN_JS
Undefined Cluster::rescan(Dictionary options) {}
#elif DOXYGEN_PY
None Cluster::rescan(dict options) {}
#endif

void Cluster::rescan(const shcore::Dictionary_t &options) {
  assert_valid("rescan");

  check_preconditions("rescan");

  if (!_default_replica_set)
    throw shcore::Exception::logic_error("ReplicaSet not initialized.");

  _default_replica_set->rescan(options);
}

REGISTER_HELP_FUNCTION(disconnect, Cluster);
REGISTER_HELP_FUNCTION_TEXT(CLUSTER_DISCONNECT, R"*(
Disconnects all internal sessions used by the cluster object.

@returns Nothing.

Disconnects the internal MySQL sessions used by the cluster to query for
metadata and replication information.
)*");

/**
 * $(CLUSTER_DISCONNECT_BRIEF)
 *
 * $(CLUSTER_DISCONNECT)
 */
#if DOXYGEN_JS
Undefined Cluster::disconnect() {}
#elif DOXYGEN_PY
None Cluster::disconnect() {}
#endif

shcore::Value Cluster::disconnect(const shcore::Argument_list &args) {
  args.ensure_count(0, get_function_name("disconnect").c_str());

  try {
    if (_group_session) {
      // no preconditions check needed for just disconnecting everything
      _group_session->close();
      _group_session.reset();
    }
    if (_metadata_storage->get_session()) {
      _metadata_storage->get_session()->close();
    }
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name("disconnect"));

  return shcore::Value();
}

REGISTER_HELP_FUNCTION(forceQuorumUsingPartitionOf, Cluster);
REGISTER_HELP_FUNCTION_TEXT(CLUSTER_FORCEQUORUMUSINGPARTITIONOF, R"*(
Restores the cluster from quorum loss.

@param instance An instance definition to derive the forced group from.
@param password Optional string with the password for the connection.

@returns Nothing.

This function restores the cluster's default replicaset back into operational
status from a loss of quorum scenario. Such a scenario can occur if a group is
partitioned or more crashes than tolerable occur.

The instance definition is the connection data for the instance.

${TOPIC_CONNECTION_MORE_INFO_TCP_ONLY}

Note that this operation is DANGEROUS as it can create a split-brain if
incorrectly used and should be considered a last resort. Make absolutely sure
that there are no partitions of this group that are still operating somewhere
in the network, but not accessible from your location.

When this function is used, all the members that are ONLINE from the point of
view of the given instance definition will be added to the group.

@throw ArgumentError in the following scenarios:
@li If the instance parameter is empty.
@li If the instance definition cannot be used for Group Replication.

@throw RuntimeError in the following scenarios:
@li If the instance does not exist on the Metadata.
@li If the instance is not on the ONLINE state.
@li If the instance does is not an active member of a replication group.
@li If there are no ONLINE instances visible from the given one.

@throw LogicError in the following scenarios:
@li If the cluster does not exist.
)*");

/**
 * $(CLUSTER_FORCEQUORUMUSINGPARTITIONOF_BRIEF)
 *
 * $(CLUSTER_FORCEQUORUMUSINGPARTITIONOF)
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
  assert_valid("forceQuorumUsingPartitionOf");

  args.ensure_count(1, 2,
                    get_function_name("forceQuorumUsingPartitionOf").c_str());

  shcore::Value ret_val;
  try {
    check_preconditions("forceQuorumUsingPartitionOf");

    // Check if we have a Default ReplicaSet
    if (!_default_replica_set)
      throw shcore::Exception::logic_error("ReplicaSet not initialized.");

    // TODO(alfredo) - unpack the arguments here insetad of at
    // force_quorum_using_partition_of
    ret_val = _default_replica_set->force_quorum_using_partition_of(args);
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(
      get_function_name("forceQuorumUsingPartitionOf"));

  return ret_val;
}

void Cluster::set_attribute(const std::string &attribute,
                            const shcore::Value &value) {
  if (!_attributes) _attributes.reset(new shcore::Value::Map_type());

  (*_attributes)[attribute] = value;
}

REGISTER_HELP_FUNCTION(checkInstanceState, Cluster);
REGISTER_HELP_FUNCTION_TEXT(CLUSTER_CHECKINSTANCESTATE, R"*(
Verifies the instance gtid state in relation to the cluster.

@param instance An instance definition.

@returns resultset A JSON object with the status.

Analyzes the instance executed GTIDs with the executed/purged GTIDs on the
cluster to determine if the instance is valid for the cluster.

The instance definition is the connection data for the instance.

${TOPIC_CONNECTION_MORE_INFO_TCP_ONLY}

The returned JSON object contains the following attributes:

@li state: the state of the instance
@li reason: the reason for the state reported

The state of the instance can be one of the following:

@li ok: if the instance transaction state is valid for the cluster
@li error: if the instance transaction state is not valid for the cluster

The reason for the state reported can be one of the following:

@li new: if the instance doesn't have any transactions
@li recoverable:  if the instance executed GTIDs are not conflicting with the
executed GTIDs of the cluster instances
@li diverged: if the instance executed GTIDs diverged with the executed GTIDs
of the cluster instances
@li lost_transactions: if the instance has more executed GTIDs than the
executed GTIDs of the cluster instances

@throw ArgumentError in the following scenarios:
@li If the 'instance' parameter is empty.
@li If the 'instance' parameter is invalid.
@li If the 'instance' definition is a connection dictionary but empty.

@throw RuntimeError in the following scenarios:
@li If the 'instance' is unreachable/offline.
@li If the 'instance' is a cluster member.
@li If the 'instance' belongs to a Group Replication group that is not managed
as an InnoDB cluster.
@li If the 'instance' is a standalone instance but is part of a different
InnoDB Cluster.
@li If the 'instance' has an unknown state.
)*");

/**
 * $(CLUSTER_CHECKINSTANCESTATE_BRIEF)
 *
 * $(CLUSTER_CHECKINSTANCESTATE)
 */
#if DOXYGEN_JS
Undefined Cluster::checkInstanceState(InstanceDef instance) {}
#elif DOXYGEN_PY
None Cluster::check_instance_state(InstanceDef instance) {}
#endif

shcore::Value Cluster::check_instance_state(
    const Connection_options &instance_def) {
  assert_valid("checkInstanceState");
  check_preconditions("checkInstanceState");

  // Check if we have a Default ReplicaSet
  if (!_default_replica_set)
    throw shcore::Exception::logic_error("ReplicaSet not initialized.");

  return _default_replica_set->check_instance_state(instance_def);
}

shcore::Value Cluster::check_instance_state(const std::string &instance_def) {
  return check_instance_state(get_connection_options(instance_def));
}

shcore::Value Cluster::check_instance_state(
    const shcore::Dictionary_t &instance_def) {
  return check_instance_state(get_connection_options(instance_def));
}

REGISTER_HELP_FUNCTION(switchToSinglePrimaryMode, Cluster);
REGISTER_HELP_FUNCTION_TEXT(CLUSTER_SWITCHTOSINGLEPRIMARYMODE, R"*(
Switches the cluster to single-primary mode.

@param instance Optional An instance definition.

@returns Nothing.

This function changes a cluster running in multi-primary mode to single-primary
mode.

The instance definition is the connection data for the instance.

${TOPIC_CONNECTION_MORE_INFO_TCP_ONLY}

The instance definition is optional and is the identifier of the cluster member
that shall become the new primary.

If the instance definition is not provided, the new primary will be the
instance with the highest member weight (and the lowest UUID in case of a tie
on member weight).

@throw ArgumentError in the following scenarios:
@li If the instance parameter is empty.
@li If the instance definition is invalid.

@throw RuntimeError in the following scenarios:
@li If 'instance' does not refer to a cluster member.
@li If any of the cluster members has a version < 8.0.13.
@li If the cluster has no visible quorum.
@li If any of the cluster members is not ONLINE.
)*");

/**
 * $(CLUSTER_SWITCHTOSINGLEPRIMARYMODE_BRIEF)
 *
 * $(CLUSTER_SWITCHTOSINGLEPRIMARYMODE)
 */
#if DOXYGEN_JS
Undefined Cluster::switchToSinglePrimaryMode(InstanceDef instance) {}
#elif DOXYGEN_PY
None Cluster::switch_to_single_primary_mode(InstanceDef instance) {}
#endif

void Cluster::switch_to_single_primary_mode(
    const Connection_options &instance_def) {
  assert_valid("switchToSinglePrimaryMode");
  check_preconditions("switchToSinglePrimaryMode");

  // Check if we have a Default ReplicaSet
  if (!_default_replica_set)
    throw shcore::Exception::logic_error("ReplicaSet not initialized.");

  // Switch to single-primary mode
  _default_replica_set->switch_to_single_primary_mode(instance_def);
}

REGISTER_HELP_FUNCTION(switchToMultiPrimaryMode, Cluster);
REGISTER_HELP_FUNCTION_TEXT(CLUSTER_SWITCHTOMULTIPRIMARYMODE, R"*(
Switches the cluster to multi-primary mode.

@returns Nothing.

This function changes a cluster running in single-primary mode to multi-primary
mode.

@throw RuntimeError in the following scenarios:
@li If any of the cluster members has a version < 8.0.13.
@li If the cluster has no visible quorum.
@li If any of the cluster members is not ONLINE.
)*");

/**
 * $(CLUSTER_SWITCHTOMULTIPRIMARYMODE_BRIEF)
 *
 * $(CLUSTER_SWITCHTOMULTIPRIMARYMODE)
 */
#if DOXYGEN_JS
Undefined Cluster::switchToMultiPrimaryMode() {}
#elif DOXYGEN_PY
None Cluster::switch_to_multi_primary_mode() {}
#endif

void Cluster::switch_to_multi_primary_mode(void) {
  assert_valid("switchToMultiPrimaryMode");
  check_preconditions("switchToMultiPrimaryMode");

  // Switch to single-primary mode

  // Check if we have a Default ReplicaSet
  if (!_default_replica_set)
    throw shcore::Exception::logic_error("ReplicaSet not initialized.");

  _default_replica_set->switch_to_multi_primary_mode();
}

REGISTER_HELP_FUNCTION(setPrimaryInstance, Cluster);
REGISTER_HELP_FUNCTION_TEXT(CLUSTER_SETPRIMARYINSTANCE, R"*(
Elects a specific cluster member as the new primary.

@param instance An instance definition.

@returns Nothing.

This function forces the election of a new primary, overriding any
election process. 

The instance definition is the connection data for the instance.

${TOPIC_CONNECTION_MORE_INFO_TCP_ONLY}

The instance definition is mandatory and is the identifier of the cluster
member that shall become the new primary.

@throw ArgumentError in the following scenarios:
@li If the instance parameter is empty.
@li If the instance definition is invalid.

@throw RuntimeError in the following scenarios:
@li If the cluster is in multi-primary mode.
@li If 'instance' does not refer to a cluster member.
@li If any of the cluster members has a version < 8.0.13.
@li If the cluster has no visible quorum.
@li If any of the cluster members is not ONLINE.
)*");

/**
 * $(CLUSTER_SETPRIMARYINSTANCE_BRIEF)
 *
 * $(CLUSTER_SETPRIMARYINSTANCE)
 */
#if DOXYGEN_JS
Undefined Cluster::setPrimaryInstance(InstanceDef instance) {}
#elif DOXYGEN_PY
None Cluster::set_primary_instance(InstanceDef instance) {}
#endif

void Cluster::set_primary_instance(const Connection_options &instance_def) {
  assert_valid("setPrimaryInstance");
  check_preconditions("setPrimaryInstance");

  // Set primary instance

  // Check if we have a Default ReplicaSet
  if (!_default_replica_set)
    throw shcore::Exception::logic_error("ReplicaSet not initialized.");

  _default_replica_set->set_primary_instance(instance_def);
}

void Cluster::set_primary_instance(const std::string &instance_def) {
  mysqlshdk::db::Connection_options target_instance;

  target_instance = get_connection_options(instance_def);

  set_primary_instance(target_instance);
}

void Cluster::set_primary_instance(const shcore::Dictionary_t &instance_def) {
  mysqlshdk::db::Connection_options target_instance;

  target_instance = get_connection_options(instance_def);

  set_primary_instance(target_instance);
}

REGISTER_HELP_FUNCTION(setOption, Cluster);
REGISTER_HELP_FUNCTION_TEXT(CLUSTER_SETOPTION, R"*(
Changes the value of a configuration option for the whole cluster.

@param option The configuration option to be changed.
@param value The value that the configuration option shall get.

@returns Nothing.

This function changes an InnoDB Cluster configuration option in all members of
the cluster.

The 'option' parameter is the name of the configuration option to be changed.

The value parameter is the value that the configuration option shall get.

The accepted values for the configuration option are:

@li clusterName: string value to define the cluster name.
${CLUSTER_OPT_EXIT_STATE_ACTION}
${CLUSTER_OPT_MEMBER_WEIGHT}
${CLUSTER_OPT_FAILOVER_CONSISTENCY}
${CLUSTER_OPT_EXPEL_TIMEOUT}

The value for the configuration option is used to set the Group Replication
system variable that corresponds to it.

${CLUSTER_OPT_EXIT_STATE_ACTION_DETAIL}

${CLUSTER_OPT_FAILOVER_CONSISTENCY_DETAIL}

${CLUSTER_OPT_MEMBER_WEIGHT_DETAIL_EXTRA}

${CLUSTER_OPT_EXIT_STATE_ACTION_EXTRA}

${CLUSTER_OPT_FAILOVER_CONSISTENCY_EXTRA}

${CLUSTER_OPT_EXPEL_TIMEOUT_EXTRA}

@throw ArgumentError in the following scenarios:
@li If the 'option' parameter is empty.
@li If the 'value' parameter is empty.
@li If the 'option' parameter is invalid.

@throw RuntimeError in the following scenarios:
@li If any of the cluster members do not support the configuration option
passed in 'option'.
@li If the value passed in 'option' is not valid for Group Replication.
@li If the cluster has no visible quorum.
@li If any of the cluster members is not ONLINE.
)*");

/**
 * $(CLUSTER_SETOPTION_BRIEF)
 *
 * $(CLUSTER_SETOPTION)
 */
#if DOXYGEN_JS
Undefined Cluster::setOption(String option, String value) {}
#elif DOXYGEN_PY
None Cluster::set_option(str option, str value) {}
#endif

void Cluster::set_option(const std::string &option,
                         const shcore::Value &value) {
  assert_valid("setOption");
  check_preconditions("setOption");

  // Check if we have a Default ReplicaSet
  if (!_default_replica_set)
    throw shcore::Exception::logic_error("ReplicaSet not initialized.");

  // Set Cluster configuration option

  // Create the Cluster_set_option object and execute it.
  std::unique_ptr<Cluster_set_option> op_cluster_set_option;

  // Validation types due to a limitation on the expose() framework.
  // Currently, it's not possible to do overloading of functions that overload
  // an argument of type string/int since the type int is convertible to
  // string, thus overloading becomes ambiguous. As soon as that limitation is
  // gone, this type checking shall go away too.
  if (value.type == shcore::String) {
    std::string value_str = value.as_string();
    op_cluster_set_option =
        shcore::make_unique<Cluster_set_option>(this, option, value_str);
  } else if (value.type == shcore::Integer || value.type == shcore::UInteger) {
    int64_t value_int = value.as_int();
    op_cluster_set_option =
        shcore::make_unique<Cluster_set_option>(this, option, value_int);
  } else {
    throw shcore::Exception::argument_error(
        "Argument #2 is expected to be a string or an Integer.");
  }

  // Always execute finish when leaving "try catch".
  auto finally = shcore::on_leave_scope(
      [&op_cluster_set_option]() { op_cluster_set_option->finish(); });

  // Prepare the Set_option command execution (validations).
  op_cluster_set_option->prepare();

  // Execute Set_instance_option operations.
  op_cluster_set_option->execute();
}

REGISTER_HELP_FUNCTION(setInstanceOption, Cluster);
REGISTER_HELP_FUNCTION_TEXT(CLUSTER_SETINSTANCEOPTION, R"*(
Changes the value of a configuration option in a Cluster member.

@param instance An instance definition.
@param option The configuration option to be changed.
@param value The value that the configuration option shall get.

@returns Nothing.

This function changes an InnoDB Cluster configuration option in a member of the
cluster.

The instance definition is the connection data for the instance.

${TOPIC_CONNECTION_MORE_INFO_TCP_ONLY}

The option parameter is the name of the configuration option to be changed

The value parameter is the value that the configuration option shall get.

The accepted values for the configuration option are:
${CLUSTER_OPT_EXIT_STATE_ACTION}
${CLUSTER_OPT_MEMBER_WEIGHT}
@li label a string identifier of the instance.

${CLUSTER_OPT_EXIT_STATE_ACTION_DETAIL}

${CLUSTER_OPT_MEMBER_WEIGHT_DETAIL_EXTRA}

${CLUSTER_OPT_EXIT_STATE_ACTION_EXTRA}

@throw ArgumentError in the following scenarios:
@li If the 'instance' parameter is empty.
@li If the 'instance' parameter is invalid.
@li If the 'instance' definition is a connection dictionary but empty.
@li If the 'option' parameter is empty.
@li If the 'value' parameter is empty.
@li If the 'option' parameter is invalid.

@throw RuntimeError in the following scenarios:
@li If 'instance' does not refer to a cluster member.
@li If the cluster has no visible quorum.
@li If 'instance' is not ONLINE.
@li If 'instance' does not support the configuration option passed in 'option'.
@li If the value passed in 'option' is not valid for Group Replication.
)*");

/**
 * $(CLUSTER_SETINSTANCEOPTION_BRIEF)
 *
 * $(CLUSTER_SETINSTANCEOPTION)
 */
#if DOXYGEN_JS
Undefined Cluster::setInstanceOption(InstanceDef instance, String option,
                                     String value) {}
#elif DOXYGEN_PY
None Cluster::set_instance_option(InstanceDef instance, str option, str value);
#endif
void Cluster::set_instance_option(const Connection_options &instance_def,
                                  const std::string &option,
                                  const shcore::Value &value) {
  assert_valid("setInstanceOption");
  check_preconditions("setInstanceOption");

  // Check if we have a Default ReplicaSet
  if (!_default_replica_set)
    throw shcore::Exception::logic_error("ReplicaSet not initialized.");

  // Set the option in the Default ReplicaSet
  _default_replica_set->set_instance_option(instance_def, option, value);
}

void Cluster::set_instance_option(const std::string &instance_def,
                                  const std::string &option,
                                  const shcore::Value &value) {
  mysqlshdk::db::Connection_options target_instance;

  target_instance = get_connection_options(instance_def);

  set_instance_option(target_instance, option, value);
}

void Cluster::set_instance_option(const shcore::Dictionary_t &instance_def,
                                  const std::string &option,
                                  const shcore::Value &value) {
  mysqlshdk::db::Connection_options target_instance;

  target_instance = get_connection_options(instance_def);

  set_instance_option(target_instance, option, value);
}

Cluster_check_info Cluster::check_preconditions(
    const std::string &function_name) const {
  return check_function_preconditions("Cluster." + function_name,
                                      _group_session);
}

void Cluster::sync_transactions(
    const mysqlshdk::mysql::IInstance &target_instance) const {
  // Must get the value of the 'gtid_executed' variable with GLOBAL scope to get
  // the GTID of ALL transactions, otherwise only a set of transactions written
  // to the cache in the current session might be returned.
  std::string gtid_set = _group_session->query("SELECT @@GLOBAL.GTID_EXECUTED")
                             ->fetch_one()
                             ->get_string(0);

  bool sync_res = mysqlshdk::mysql::wait_for_gtid_set(
      target_instance, gtid_set,
      current_shell_options()->get().dba_gtid_wait_timeout);
  if (!sync_res) {
    std::string instance_address =
        target_instance.get_connection_options().as_uri(
            mysqlshdk::db::uri::formats::only_transport());
    throw shcore::Exception::runtime_error(
        "Timeout reached waiting for cluster transactions to be applied on "
        "instance '" +
        instance_address + "'");
  }
}

}  // namespace dba
}  // namespace mysqlsh
