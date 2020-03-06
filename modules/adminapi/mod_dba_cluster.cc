/*
 * Copyright (c) 2016, 2020, Oracle and/or its affiliates. All rights reserved.
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

#include "modules/adminapi/common/accounts.h"
#include "modules/adminapi/common/common.h"
#include "modules/adminapi/common/sql.h"
#include "modules/adminapi/mod_dba_cluster.h"
#include "modules/mysqlxtest_utils.h"
#include "mysqlshdk/include/scripting/type_info/custom.h"
#include "mysqlshdk/include/scripting/type_info/generic.h"
#include "mysqlshdk/include/shellcore/utils_help.h"
#include "mysqlshdk/libs/utils/debug.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/shellcore/shell_console.h"

using std::placeholders::_1;

DEBUG_OBJ_ENABLE(Cluster);

namespace mysqlsh {
namespace dba {
using mysqlshdk::db::uri::formats::only_transport;
using mysqlshdk::db::uri::formats::user_transport;
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

Cluster::Cluster(const std::shared_ptr<Cluster_impl> &impl) : m_impl(impl) {
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
  s_out.append("<" + class_name() + ":" + m_impl->get_name() + ">");
  return s_out;
}

void Cluster::append_json(shcore::JSON_dumper &dumper) const {
  dumper.start_object();
  dumper.append_string("class", class_name());
  dumper.append_string("name", m_impl->get_name());
  dumper.end_object();
}

bool Cluster::operator==(const Object_bridge &other) const {
  return class_name() == other.class_name() && this == &other;
}

void Cluster::init() {
  add_property("name", "getName");
  expose("addInstance", &Cluster::add_instance, "instanceDef", "?options");
  expose("rejoinInstance", &Cluster::rejoin_instance, "instanceDef",
         "?options");
  expose("removeInstance", &Cluster::remove_instance, "instanceDef",
         "?options");
  expose("describe", &Cluster::describe);
  expose("status", &Cluster::status, "?options");
  expose("dissolve", &Cluster::dissolve, "?options");
  expose("resetRecoveryAccountsPassword",
         &Cluster::reset_recovery_accounts_password, "?options");
  expose("checkInstanceState", &Cluster::check_instance_state, "instanceDef");
  expose("setupAdminAccount", &Cluster::setup_admin_account, "user",
         "?options");
  expose("setupRouterAccount", &Cluster::setup_router_account, "user",
         "?options");
  expose("rescan", &Cluster::rescan, "?options");
  expose("forceQuorumUsingPartitionOf",
         &Cluster::force_quorum_using_partition_of, "instanceDef", "?password");
  expose("disconnect", &Cluster::disconnect);
  expose("switchToSinglePrimaryMode", &Cluster::switch_to_single_primary_mode,
         "?instanceDef");
  expose("switchToMultiPrimaryMode", &Cluster::switch_to_multi_primary_mode);
  expose("setPrimaryInstance", &Cluster::set_primary_instance, "instanceDef");
  expose("options", &Cluster::options, "?options");
  expose("setOption", &Cluster::set_option, "option", "value");
  expose("setInstanceOption", &Cluster::set_instance_option, "instanceDef",
         "option", "value");
  expose("listRouters", &Cluster::list_routers, "?options");
  expose("removeRouterMetadata", &Cluster::remove_router_metadata, "routerDef");
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
    ret_val = shcore::Value(m_impl->get_name());
  else
    ret_val = shcore::Cpp_object_bridge::get_member(prop);
  return ret_val;
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
      name = get_member_name(option_name, shcore::current_naming_style());
      throw shcore::Exception::runtime_error(class_name() + "." + name + ": " +
                                             "Can't access object member '" +
                                             name + "' on a dissolved cluster");
    }
  }
  if (!m_impl->get_target_instance()) {
    throw shcore::Exception::runtime_error(
        "The cluster object is disconnected. Please use dba." +
        get_function_name("getCluster", false) +
        " to obtain a fresh cluster handle.");
  }
}

REGISTER_HELP_FUNCTION(addInstance, Cluster);
REGISTER_HELP_FUNCTION_TEXT(CLUSTER_ADDINSTANCE, R"*(
Adds an Instance to the cluster.

@param instance Connection options for the target instance to be added.
@param options Optional dictionary with options for the operation.

@returns nothing

This function adds an Instance to a InnoDB cluster.

${TOPIC_CONNECTION_MORE_INFO}

The options dictionary may contain the following attributes:

@li label: an identifier for the instance being added
@li recoveryMethod: Preferred method of state recovery. May be auto, clone or
incremental. Default is auto.
@li waitRecovery: Integer value to indicate if the command shall wait for the
recovery process to finish and its verbosity level.
@li password: the instance connection password
@li memberSslMode: SSL mode used on the instance
@li ipWhitelist: The list of hosts allowed to connect to the instance for group
replication
@li localAddress: string value with the Group Replication local address to be
used instead of the automatically generated one.
@li groupSeeds: string value with a comma-separated list of the Group
Replication peer addresses to be used instead of the automatically generated
one.
${OPT_INTERACTIVE}
${CLUSTER_OPT_EXIT_STATE_ACTION}
${CLUSTER_OPT_MEMBER_WEIGHT}
${CLUSTER_OPT_AUTO_REJOIN_TRIES}

The password may be contained on the instance definition, however, it can be
overwritten if it is specified on the options.

The recoveryMethod option supports the following values:

@li incremental: uses distributed state recovery, which applies missing
transactions copied from another cluster member. Clone will be disabled.
@li clone: clone: uses built-in MySQL clone support, which completely replaces
the state of the target instance with a full snapshot of another cluster member
before distributed recovery starts. Requires MySQL 8.0.17 or newer.
@li auto: let Group Replication choose whether or not a full snapshot has to be
taken, based on what the target server supports and the
group_replication_clone_threshold sysvar.
This is the default value. A prompt will be shown if not possible to safely
determine a safe way forward. If interaction is disabled, the operation will be
canceled instead.

@attention The memberSslMode option will be removed in a future release.

The memberSslMode option supports the following values:

@li REQUIRED: if used, SSL (encryption) will be enabled for the instance to
communicate with other members of the cluster
@li DISABLED: if used, SSL (encryption) will be disabled
@li AUTO: if used, SSL (encryption) will be automatically enabled or disabled
based on the cluster configuration

If memberSslMode is not specified AUTO will be used by default.

The waitRecovery option supports the following values:

@li 0: do not wait and let the recovery process to finish in the background.
@li 1: block until the recovery process to finishes.
@li 2: block until the recovery process finishes and show progress information.
@li 3: block until the recovery process finishes and show progress using
progress bars.

By default, if the standard output on which the Shell is running refers to a
terminal, the waitRecovery option has the value of 3. Otherwise, it has the
value of 2.

${CLUSTER_OPT_EXIT_STATE_ACTION_DETAIL}

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
an error is thrown.

The value for groupSeeds is used to set the Group Replication system variable
'group_replication_group_seeds'. The groupSeeds option accepts a
comma-separated list of addresses in the format: 'host1:port1,...,hostN:portN'.

${CLUSTER_OPT_EXIT_STATE_ACTION_EXTRA}

${CLUSTER_OPT_MEMBER_WEIGHT_DETAIL_EXTRA}

${CLUSTER_OPT_AUTO_REJOIN_TRIES_EXTRA}

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
@li If the value for the waitRecovery option is not in the range [0, 3].

@throw RuntimeError in the following scenarios:
@li If the instance accounts are invalid.
@li If the instance is not in bootstrapped state.
@li If the SSL mode specified is not compatible with the one used in the
cluster.
@li If the value for the localAddress, groupSeeds, exitStateAction,
memberWeight or autoRejoinTries options is not valid for Group Replication.
@li If the value of recoveryMethod is not auto and it cannot be used.
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
void Cluster::add_instance(const Connection_options &instance_def_,
                           const shcore::Dictionary_t &options) {
  auto instance_def = instance_def_;
  set_password_from_map(&instance_def, options);

  // Throw an error if the cluster has already been dissolved
  assert_valid("addInstance");

  validate_connection_options(instance_def);

  // Add the Instance to the Default ReplicaSet
  m_impl->add_instance(instance_def, options);
}

REGISTER_HELP_FUNCTION(rejoinInstance, Cluster);
REGISTER_HELP_FUNCTION_TEXT(CLUSTER_REJOININSTANCE, R"*(
Rejoins an Instance to the cluster.

@param instance An instance definition.
@param options Optional dictionary with options for the operation.

@returns Nothing.

This function rejoins an Instance to the cluster.

The instance definition is the connection data for the instance.

${TOPIC_CONNECTION_MORE_INFO}

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
@li If the instance is an active member of the cluster.
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

void Cluster::rejoin_instance(const Connection_options &instance_def_,
                              const shcore::Dictionary_t &options) {
  auto instance_def = instance_def_;
  set_password_from_map(&instance_def, options);

  // Throw an error if the cluster has already been dissolved
  assert_valid("rejoinInstance");

  bool interactive = current_shell_options()->get().wizards;
  const auto console = current_console();

  m_impl->check_preconditions("rejoinInstance");

  if (interactive) {
    // TODO(rennox): This was done on the interactive call, wondering if
    // either should be done only if interactive, or if it should be done at
    // all
    instance_def.set_default_connection_data();

    std::string message =
        "Rejoining the instance to the InnoDB cluster. "
        "Depending on the original\n"
        "problem that made the instance unavailable, the rejoin operation "
        "might not be\n"
        "successful and further manual steps will be needed to fix the "
        "underlying\n"
        "problem.\n"
        "\n"
        "Please monitor the output of the rejoin operation and take "
        "necessary "
        "action if\n"
        "the instance cannot rejoin.\n\n";

    console->print(message);

    console->println("Rejoining instance to the cluster ...");
    console->println();
  }

  validate_connection_options(instance_def);
  // rejoin the Instance to the Default ReplicaSet
  m_impl->rejoin_instance(instance_def, options);

  if (interactive) {
    console->println("The instance '" + instance_def.as_uri(only_transport()) +
                     "' was successfully rejoined on the cluster.");
    console->println();
  }
}

REGISTER_HELP_FUNCTION(removeInstance, Cluster);
REGISTER_HELP_FUNCTION_TEXT(CLUSTER_REMOVEINSTANCE, R"*(
Removes an Instance from the cluster.

@param instance An instance definition.
@param options Optional dictionary with options for the operation.

@returns Nothing.

This function removes an Instance from the cluster.

The instance definition is the connection data for the instance.

${TOPIC_CONNECTION_MORE_INFO}

The options dictionary may contain the following attributes:

@li password: the instance connection password
@li force: boolean, indicating if the instance must be removed (even if only
from metadata) in case it cannot be reached. By default, set to false.
${OPT_INTERACTIVE}

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

void Cluster::remove_instance(const Connection_options &instance_def_,
                              const shcore::Dictionary_t &options) {
  auto instance_def = instance_def_;
  set_password_from_map(&instance_def, options);

  // Throw an error if the cluster has already been dissolved
  assert_valid("removeInstance");

  // Remove the Instance from the Default ReplicaSet
  m_impl->remove_instance(instance_def, options);
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
@li topologyMode: the InnoDB Cluster topology mode.

Each instance dictionary contains the following attributes:

@li address: the instance address in the form of host:port
@li label: the instance name identifier
@li role: the instance role
@li version: the instance version (only available for instances >= 8.0.11)

@throw MetadataError in the following scenarios:
@li If the Metadata is inaccessible.
@throw RuntimeError in the following scenarios:
@li If the InnoDB Cluster topology mode does not match the current Group Replication configuration.
@li If the InnoDB Cluster name is not registered in the Metadata.
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

shcore::Value Cluster::describe(void) {
  // Throw an error if the cluster has already been dissolved
  assert_valid("describe");

  return m_impl->describe();
}

REGISTER_HELP_FUNCTION(status, Cluster);
REGISTER_HELP_FUNCTION_TEXT(CLUSTER_STATUS, R"*(
Describe the status of the cluster.

@param options Optional dictionary with options.

@returns A JSON object describing the status of the cluster.

This function describes the status of the cluster including its ReplicaSets and
Instances. The following options may be given to control the amount of
information gathered and returned.

@li extended: verbosity level of the command output.
@li queryMembers: if true, connect to each Instance of the ReplicaSets to query
for more detailed stats about the replication machinery.

@attention The queryMembers option will be removed in a future release. Please
use the extended option instead.

The extended option supports Integer or Boolean values:

@li 0: disables the command verbosity (default);
@li 1: includes information about the Metadata Version, Group Protocol
       Version, Group name, cluster member UUIDs, cluster member roles and
       states as reported byGroup Replication and the list of fenced system
       variables;
@li 2: includes information about transactions processed by connection and
       applier;
@li 3: includes more detailed stats about the replication machinery of each
       cluster member;
@li Boolean: equivalent to assign either 0 (false) or 1 (true).

@throw MetadataError in the following scenarios:
@li If the Metadata is inaccessible.
@throw RuntimeError in the following scenarios:
@li If the InnoDB Cluster topology mode does not match the current Group
    Replication configuration.
@li If the InnoDB Cluster name is not registered in the Metadata.
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

shcore::Value Cluster::status(const shcore::Dictionary_t &options) {
  // Throw an error if the cluster has already been dissolved
  assert_valid("status");

  mysqlshdk::utils::nullable<bool> query_members;
  uint64_t extended = 0;  // By default 0 (false).

  // Retrieves optional options
  // NOTE: Booleans are accepted as UInteger: false = 0, true = 1;
  Unpack_options(options)
      .optional("extended", &extended)
      .optional("queryMembers", &query_members)
      .end();

  // Validate extended option UInteger [0, 3] or Boolean.
  if (extended > 3) {
    throw shcore::Exception::argument_error(
        "Invalid value '" + std::to_string(extended) +
        "' for option 'extended'. It must be an integer in the range [0, 3].");
  }

  // The queryMembers option is deprecated.
  if (!query_members.is_null()) {
    auto console = mysqlsh::current_console();

    std::string specific_value = (*query_members) ? " with value 3" : "";
    console->print_warning(
        "The 'queryMembers' option is deprecated. "
        "Please use the 'extended' option" +
        specific_value + " instead.");
    console->println();

    // Currently, the queryMembers option overrides the extended option when
    // set to true. Thus, this behaviour is maintained until the option is
    // removed.
    if (*query_members) {
      extended = 3;
    }
  }

  return m_impl->status(extended);
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

  return m_impl->options(options);
}

REGISTER_HELP_FUNCTION(dissolve, Cluster);
REGISTER_HELP_FUNCTION_TEXT(CLUSTER_DISSOLVE, R"*(
Dissolves the cluster.

@param options Optional parameters as described below.

@returns Nothing.

This function stops group replication and unregisters all members from the
cluster metadata.

It keeps all the user's data intact.

The options dictionary may contain the following attributes:

@li force: boolean value used to confirm that the dissolve operation must be
executed, even if some members of the cluster cannot be reached or the timeout
was reached when waiting for members to catch up with replication changes. By
default, set to false.
${OPT_INTERACTIVE}

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

void Cluster::dissolve(const shcore::Dictionary_t &options) {
  // Throw an error if the cluster has already been dissolved
  assert_valid("dissolve");

  m_impl->dissolve(options);

  // Set the flag, marking this cluster instance as invalid.
  invalidate();
}

REGISTER_HELP_FUNCTION(resetRecoveryAccountsPassword, Cluster);
REGISTER_HELP_FUNCTION_TEXT(CLUSTER_RESETRECOVERYACCOUNTSPASSWORD, R"*(
Reset the password of the recovery accounts of the cluster.

@param options Dictionary with options for the operation.

@returns Nothing.

This function resets the passwords for all internal recovery user accounts
used by the Cluster.
It can be used to reset the passwords of the recovery user
accounts when needed for any security reasons. For example:
periodically to follow some custom password lifetime policy, or after
some security breach event.

The options dictionary may contain the following attributes:

@li force: boolean, indicating if the operation will continue in case an
error occurs when trying to reset the passwords on any of the
instances, for example if any of them is not online. By default, set to false.
${OPT_INTERACTIVE}

The use of the force option (set to true) is not recommended. Use it only
if really needed when instances are permanently not available (no longer
reachable) or never going to be reused again in a cluster. Prefer to bring the
non available instances back ONLINE or remove them from the cluster if they will
no longer be used.

@throw RuntimeError in the following scenarios:
@li If some cluster instance is not ONLINE and force option not used or set to false.
@li If the recovery user account was not created by InnoDB Cluster.
)*");

/**
 * $(CLUSTER_RESETRECOVERYACCOUNTSPASSWORD_BRIEF)
 *
 * $(CLUSTER_RESETRECOVERYACCOUNTSPASSWORD)
 */
#if DOXYGEN_JS
Undefined Cluster::resetRecoveryAccountsPassword(Dictionary options) {}
#elif DOXYGEN_PY
None Cluster::reset_recovery_accounts_password(dict options) {}
#endif

void Cluster::reset_recovery_accounts_password(
    const shcore::Dictionary_t &options) {
  // Throw an error if the cluster has already been dissolved
  assert_valid("resetRecoveryAccountsPassword");

  // Reset the recovery passwords.
  m_impl->reset_recovery_password(options);
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
${OPT_INTERACTIVE}
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

  m_impl->rescan(options);
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

void Cluster::disconnect() { m_impl->disconnect(); }

REGISTER_HELP_FUNCTION(forceQuorumUsingPartitionOf, Cluster);
REGISTER_HELP_FUNCTION_TEXT(CLUSTER_FORCEQUORUMUSINGPARTITIONOF, R"*(
Restores the cluster from quorum loss.

@param instance An instance definition to derive the forced group from.
@param password Optional string with the password for the connection.

@returns Nothing.

This function restores the cluster back into operational status from a loss of
quorum scenario. Such a scenario can occur if a group is partitioned or more
crashes than tolerable occur.

The instance definition is the connection data for the instance.

${TOPIC_CONNECTION_MORE_INFO}

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

void Cluster::force_quorum_using_partition_of(
    const Connection_options &instance_def_, const char *password) {
  auto instance_def = instance_def_;
  set_password_from_string(&instance_def, password);

  // Throw an error if the cluster has already been dissolved
  assert_valid("forceQuorumUsingPartitionOf");

  m_impl->check_preconditions("forceQuorumUsingPartitionOf");

  bool interactive = current_shell_options()->get().wizards;
  const auto console = current_console();

  auto default_rs = m_impl->get_default_replicaset();

  if (!default_rs)
    throw shcore::Exception::logic_error("cluster not initialized.");

  std::vector<Instance_metadata> online_instances =
      default_rs->get_active_instances();

  std::vector<std::string> online_instances_array;
  for (const auto &instance : online_instances) {
    online_instances_array.push_back(instance.endpoint);
  }

  if (online_instances_array.empty())
    throw shcore::Exception::logic_error(
        "No online instances are visible from the given one.");

  auto group_peers = shcore::str_join(online_instances_array, ",");

  // Remove the trailing comma of group_peers
  if (group_peers.back() == ',') group_peers.pop_back();

  if (interactive) {
    std::string message = "Restoring cluster '" + impl()->get_name() +
                          "' from loss of quorum, by using the partition "
                          "composed of [" +
                          group_peers + "]\n\n";
    console->print(message);

    console->println("Restoring the InnoDB cluster ...");
    console->println();
  }

  m_impl->force_quorum_using_partition_of(instance_def);

  if (interactive) {
    console->println(
        "The InnoDB cluster was successfully restored using the partition from "
        "the instance '" +
        instance_def.as_uri(user_transport()) + "'.");
    console->println();
    console->println(
        "WARNING: To avoid a split-brain scenario, ensure that all other "
        "members of the cluster are removed or joined back to the group that "
        "was restored.");
    console->println();
  }
}

REGISTER_HELP_FUNCTION(checkInstanceState, Cluster);
REGISTER_HELP_FUNCTION_TEXT(CLUSTER_CHECKINSTANCESTATE, R"*(
Verifies the instance gtid state in relation to the cluster.

@param instance An instance definition.

@returns resultset A JSON object with the status.

Analyzes the instance executed GTIDs with the executed/purged GTIDs on the
cluster to determine if the instance is valid for the cluster.

The instance definition is the connection data for the instance.

${TOPIC_CONNECTION_MORE_INFO}

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

  return m_impl->check_instance_state(instance_def);
}

REGISTER_HELP_FUNCTION(switchToSinglePrimaryMode, Cluster);
REGISTER_HELP_FUNCTION_TEXT(CLUSTER_SWITCHTOSINGLEPRIMARYMODE, R"*(
Switches the cluster to single-primary mode.

@param instance Optional An instance definition.

@returns Nothing.

This function changes a cluster running in multi-primary mode to single-primary
mode.

The instance definition is the connection data for the instance.

${TOPIC_CONNECTION_MORE_INFO}

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

  // Switch to single-primary mode
  m_impl->switch_to_single_primary_mode(instance_def);
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

  // Switch to single-primary mode
  m_impl->switch_to_multi_primary_mode();
}

REGISTER_HELP_FUNCTION(setPrimaryInstance, Cluster);
REGISTER_HELP_FUNCTION_TEXT(CLUSTER_SETPRIMARYINSTANCE, R"*(
Elects a specific cluster member as the new primary.

@param instance An instance definition.

@returns Nothing.

This function forces the election of a new primary, overriding any
election process.

The instance definition is the connection data for the instance.

${TOPIC_CONNECTION_MORE_INFO}

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

  m_impl->set_primary_instance(instance_def);
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
${CLUSTER_OPT_CONSISTENCY}
${CLUSTER_OPT_EXPEL_TIMEOUT}
${CLUSTER_OPT_AUTO_REJOIN_TRIES}
@li disableClone: boolean value used to disable the clone usage on the cluster.

@attention The failoverConsistency option will be removed in a future release.
Please use the consistency option instead.

The value for the configuration option is used to set the Group Replication
system variable that corresponds to it.

${CLUSTER_OPT_EXIT_STATE_ACTION_DETAIL}

${CLUSTER_OPT_CONSISTENCY_DETAIL}

${CLUSTER_OPT_EXIT_STATE_ACTION_EXTRA}

${CLUSTER_OPT_MEMBER_WEIGHT_DETAIL_EXTRA}

${CLUSTER_OPT_CONSISTENCY_EXTRA}

${CLUSTER_OPT_EXPEL_TIMEOUT_EXTRA}

${CLUSTER_OPT_AUTO_REJOIN_TRIES_EXTRA}

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

  m_impl->set_option(option, value);
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

${TOPIC_CONNECTION_MORE_INFO}

The option parameter is the name of the configuration option to be changed

The value parameter is the value that the configuration option shall get.

The accepted values for the configuration option are:
${CLUSTER_OPT_EXIT_STATE_ACTION}
${CLUSTER_OPT_MEMBER_WEIGHT}
${CLUSTER_OPT_AUTO_REJOIN_TRIES}
@li label a string identifier of the instance.

${CLUSTER_OPT_EXIT_STATE_ACTION_DETAIL}

${CLUSTER_OPT_EXIT_STATE_ACTION_EXTRA}

${CLUSTER_OPT_MEMBER_WEIGHT_DETAIL_EXTRA}

${CLUSTER_OPT_AUTO_REJOIN_TRIES_EXTRA}

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

  // Set the option in the Default ReplicaSet
  m_impl->set_instance_option(instance_def, option, value);
}

REGISTER_HELP_FUNCTION(listRouters, Cluster);
REGISTER_HELP_FUNCTION_TEXT(CLUSTER_LISTROUTERS, R"*(
Lists the Router instances.

@param options Optional dictionary with options for the operation.

@returns A JSON object listing the Router instances associated to the cluster.

This function lists and provides information about all Router instances
registered for the cluster.

Whenever a Metadata Schema upgrade is necessary, the recommended process
is to upgrade MySQL Router instances to the latest version before upgrading
the Metadata itself, in order to minimize service disruption.

The options dictionary may contain the following attributes:

@li onlyUpgradeRequired: boolean, enables filtering so only router instances
that support older version of the Metadata Schema and require upgrade are
included.

@throw MetadataError in the following scenarios:
@li If the Metadata is inaccessible.
@throw RuntimeError in the following scenarios:
@li If the InnoDB Cluster topology mode does not match the current Group
    Replication configuration.
@li If the InnoDB Cluster name is not registered in the Metadata.
)*");

/**
 * $(CLUSTER_LISTROUTERS_BRIEF)
 *
 * $(CLUSTER_LISTROUTERS)
 */
#if DOXYGEN_JS
String Cluster::listRouters(Dictionary options) {}
#elif DOXYGEN_PY
str Cluster::list_routers(dict options) {}
#endif
shcore::Dictionary_t Cluster::list_routers(
    const shcore::Dictionary_t &options) {
  // Throw an error if the cluster has already been dissolved
  assert_valid("listRouters");

  bool only_upgrade_required = false;

  // Throw an error if the cluster has already been dissolved
  check_function_preconditions("Cluster.listRouters",
                               m_impl->get_target_instance());

  Unpack_options(options)
      .optional("onlyUpgradeRequired", &only_upgrade_required)
      .end();

  auto ret_val = m_impl->list_routers(only_upgrade_required);

  return ret_val.as_map();
}

REGISTER_HELP_FUNCTION(removeRouterMetadata, Cluster);
REGISTER_HELP_FUNCTION_TEXT(CLUSTER_REMOVEROUTERMETADATA, R"*(
Removes metadata for a router instance.

@param routerDef identifier of the router instance to be removed (e.g.
192.168.45.70@::system)
@returns Nothing

MySQL Router automatically registers itself within the InnoDB cluster
metadata when bootstrapped. However, that metadata may be left behind when
instances are uninstalled or moved over to a different host. This function may
be used to clean up such instances that no longer exist.

The Cluster.<<<listRouters>>>() function may be used to list registered router
instances, including their identifier.

@throw ArgumentError in the following scenarios:
@li if router_def is not a registered router instance
)*");
/**
 * $(CLUSTER_REMOVEROUTERMETADATA_BRIEF)
 *
 * $(CLUSTER_REMOVEROUTERMETADATA)
 */
#if DOXYGEN_JS
String Cluster::removeRouterMetadata(RouterDef routerDef) {}
#elif DOXYGEN_PY
str Cluster::remove_router_metadata(RouterDef routerDef) {}
#endif
void Cluster::remove_router_metadata(const std::string &router_def) {
  // Throw an error if the cluster has already been dissolved
  assert_valid("removeRouterMetadata");

  // Throw an error if the cluster has already been dissolved
  check_function_preconditions("Cluster.removeRouterMetadata",
                               m_impl->get_target_instance());

  if (!m_impl->get_metadata_storage()->remove_router(router_def)) {
    throw shcore::Exception::argument_error("Invalid router instance '" +
                                            router_def + "'");
  }
}

REGISTER_HELP_FUNCTION(setupAdminAccount, Cluster);
REGISTER_HELP_FUNCTION_TEXT(CLUSTER_SETUPADMINACCOUNT, R"*(
Create or upgrade an InnoDB Cluster admin account.

@param user Name of the InnoDB cluster administrator account.
@param options Dictionary with options for the operation.

@returns Nothing.

This function creates/upgrades a MySQL user account with the necessary
privileges to administer an InnoDB cluster.

This function also allows a user to upgrade an existing admin account
with the necessary privileges before a dba.<<<upgradeMetadata>>>() call.

The mandatory argument user is the name of the MySQL account we want to create
or upgrade to be used as Administrator account. The accepted format is
username[@@host] where the host part is optional and if not provided defaults to
'%'.

The options dictionary may contain the following attributes:

@li password: The password for the InnoDB cluster administrator account.
@li dryRun: boolean value used to enable a dry run of the account setup
process. Default value is False.
${OPT_INTERACTIVE}
@li update: boolean value that must be enabled to allow updating the privileges
and/or password of existing accounts. Default value is False.

If the user account does not exist, the password is mandatory.

If the user account exists, the update option must be enabled.

If dryRun is used, the function will display information about the permissions
to be granted to `user` account without actually creating and/or performing any
changes on it.

The interactive option can be used to explicitly enable or disable the
interactive prompts that help the user through the account setup process.

The update option must be enabled to allow updating an existing account's
privileges and/or password.

@throw RuntimeError in the following scenarios:
@li The user account name does not exist on the Cluster and update is True.
@li The user account name does not exist on the Cluster and no password was
provided.
@li The user account name exists on the Cluster and update is False.
@li The account used to grant the privileges to the admin user doesn't have the
necessary privileges.
)*");

/**
 * $(CLUSTER_SETUPADMINACCOUNT)
 */
#if DOXYGEN_JS
Undefined Cluster::setupAdminAccount(String user, Dictionary options) {}
#elif DOXYGEN_PY
None Cluster::setup_admin_account(str user, dict options) {}
#endif

void Cluster::setup_admin_account(const std::string &user,
                                  const shcore::Dictionary_t &options) {
  // Throw an error if the cluster has already been dissolved
  assert_valid("setupAdminAccount");

  // set default values for dictionary options
  bool dry_run = false;
  bool interactive = current_shell_options()->get().wizards;
  bool update = false;
  mysqlshdk::utils::nullable<std::string> password;

  // split user into user/host
  std::string username, host;
  std::tie(username, host) = validate_account_name(user);

  // Get optional options.
  if (options) {
    Unpack_options(options)
        .optional("dryRun", &dry_run)
        .optional("interactive", &interactive)
        .optional("update", &update)
        .optional("password", &password)
        .end();
  }

  m_impl->setup_admin_account(username, host, interactive, update, dry_run,
                              password);
}

REGISTER_HELP_FUNCTION(setupRouterAccount, Cluster);
REGISTER_HELP_FUNCTION_TEXT(CLUSTER_SETUPROUTERACCOUNT, R"*(
Create or upgrade a MySQL account to use with MySQL Router.

@param user Name of the account to create/upgrade for MySQL Router.
@param options Dictionary with options for the operation.

@returns Nothing.

This function creates/upgrades a MySQL user account with the necessary
privileges to be used by MySQL Router.

This function also allows a user to upgrade existing MySQL router accounts
with the necessary privileges after a dba.<<<upgradeMetadata>>>() call.

The mandatory argument user is the name of the MySQL account we want to create
or upgrade to be used by MySQL Router. The accepted format is
username[@@host] where the host part is optional and if not provided defaults to
'%'.

The options dictionary may contain the following attributes:

@li password: The password for the MySQL Router account.
@li dryRun: boolean value used to enable a dry run of the account setup
process. Default value is False.
${OPT_INTERACTIVE}
@li update: boolean value that must be enabled to allow updating the privileges
and/or password of existing accounts. Default value is False.

If the user account does not exist, the password is mandatory.

If the user account exists, the update option must be enabled.

If dryRun is used, the function will display information about the permissions
to be granted to `user` account without actually creating and/or performing any
changes on it.

The interactive option can be used to explicitly enable or disable the
interactive prompts that help the user through the account setup process.

The update option must be enabled to allow updating an existing account's
privileges and/or password.

@throw RuntimeError in the following scenarios:
@li The user account name does not exist on the Cluster and update is True.
@li The user account name does not exist on the Cluster and no password was
provided.
@li The user account name exists on the Cluster and update is False.
@li The account used to grant the privileges to the router user doesn't have the
necessary privileges.
)*");

/**
 * $(CLUSTER_SETUPROUTERACCOUNT)
 */
#if DOXYGEN_JS
Undefined Cluster::setupRouterAccount(String user, Dictionary options) {}
#elif DOXYGEN_PY
None Cluster::setup_router_account(str user, dict options) {}
#endif
void Cluster::setup_router_account(const std::string &user,
                                   const shcore::Dictionary_t &options) {
  // Throw an error if the cluster has already been dissolved
  assert_valid("setupRouterAccount");

  // set default values for dictionary options
  bool dry_run = false;
  bool interactive = current_shell_options()->get().wizards;
  bool update = false;
  mysqlshdk::utils::nullable<std::string> password;

  // split user into user/host
  std::string username, host;
  std::tie(username, host) = validate_account_name(user);

  // Get optional options.
  if (options) {
    Unpack_options(options)
        .optional("dryRun", &dry_run)
        .optional("interactive", &interactive)
        .optional("update", &update)
        .optional("password", &password)
        .end();
  }

  m_impl->setup_router_account(username, host, interactive, update, dry_run,
                               password);
}

}  // namespace dba
}  // namespace mysqlsh
