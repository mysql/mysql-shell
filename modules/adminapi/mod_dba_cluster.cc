/*
 * Copyright (c) 2016, 2024, Oracle and/or its affiliates.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.0,
 * as published by the Free Software Foundation.
 *
 * This program is designed to work with certain software (including
 * but not limited to OpenSSL) that is licensed under separate terms,
 * as designated in a particular file or component or in included license
 * documentation.  The authors of MySQL hereby grant you an additional
 * permission to link the program and your derivative works with the
 * separately licensed software that they have either included with
 * the program or referenced in the documentation.
 *
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

#include <map>
#include <memory>
#include <string>

#include "modules/adminapi/common/common.h"
#include "modules/adminapi/mod_dba_cluster_set.h"
#include "mysqlshdk/include/shellcore/utils_help.h"
#include "mysqlshdk/libs/utils/debug.h"
#include "mysqlshdk/libs/utils/utils_general.h"

DEBUG_OBJ_ENABLE(Cluster);

namespace mysqlsh {
namespace dba {

namespace {
void validate_instance_label(const std::optional<std::string> &label,
                             const Cluster_impl &impl) {
  if (!label.has_value()) return;

  mysqlsh::dba::validate_label(*label);

  const auto &md = impl.get_metadata_storage();
  if (md->is_instance_label_unique(impl.get_id(), *label)) return;

  auto instance_md = md->get_instance_by_label(*label);

  throw shcore::Exception::argument_error(
      shcore::str_format("Instance '%s' is already using label '%s'.",
                         instance_md.address.c_str(), label->c_str()));
}
}  // namespace

using mysqlshdk::db::uri::formats::only_transport;
using mysqlshdk::db::uri::formats::user_transport;

// Documentation of the Cluster Class
REGISTER_HELP_CLASS_KW(
    Cluster, adminapi,
    (std::map<std::string, std::string>({{"FullType", "InnoDB Cluster"},
                                         {"Type", "Cluster"},
                                         {"type", "cluster"}})));
REGISTER_HELP(CLUSTER_BRIEF, "Represents an InnoDB Cluster.");
REGISTER_HELP(CLUSTER_DETAIL,
              "The cluster object is the entry point to manage and monitor "
              "a MySQL InnoDB Cluster.");
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

void Cluster::init() {
  add_property("name", "getName");
  expose("addInstance", &Cluster::add_instance, "instance", "?options")->cli();
  expose("rejoinInstance", &Cluster::rejoin_instance, "instance", "?options")
      ->cli();
  expose("removeInstance", &Cluster::remove_instance, "instance", "?options")
      ->cli();
  expose("describe", &Cluster::describe)->cli();
  expose("status", &Cluster::status, "?options")->cli();
  expose("dissolve", &Cluster::dissolve, "?options")->cli();
  expose("resetRecoveryAccountsPassword",
         &Cluster::reset_recovery_accounts_password, "?options")
      ->cli();
  expose("rescan", &Cluster::rescan, "?options")->cli();
  expose("forceQuorumUsingPartitionOf",
         &Cluster::force_quorum_using_partition_of, "instance")
      ->cli();
  expose("disconnect", &Cluster::disconnect)->cli(false);
  expose("switchToSinglePrimaryMode", &Cluster::switch_to_single_primary_mode,
         "?instance")
      ->cli();
  expose("switchToMultiPrimaryMode", &Cluster::switch_to_multi_primary_mode);
  expose("setPrimaryInstance", &Cluster::set_primary_instance, "instance",
         "?options")
      ->cli();
  expose("options", &Cluster::options, "?options")->cli();
  expose("setOption", &Cluster::set_option, "option", "value")->cli();
  expose("setInstanceOption", &Cluster::set_instance_option, "instance",
         "option", "value")
      ->cli();
  expose("removeRouterMetadata", &Cluster::remove_router_metadata, "router")
      ->cli();
  expose("routingOptions", &Cluster::routing_options, "?router")->cli();
  expose("routerOptions", &Cluster::router_options, "?options")->cli();
  expose("setRoutingOption", &Cluster::set_routing_option, "option", "value");
  expose("setRoutingOption", &Cluster::set_routing_option, "router", "option",
         "value");

  expose("createClusterSet", &Cluster::create_cluster_set, "domainName",
         "?options")
      ->cli();
  expose("getClusterSet", &Cluster::get_cluster_set)->cli(false);
  expose("fenceAllTraffic", &Cluster::fence_all_traffic)->cli();
  expose("fenceWrites", &Cluster::fence_writes)->cli();
  expose("unfenceWrites", &Cluster::unfence_writes)->cli();

  expose("listRouters", &Cluster::list_routers, "?options")->cli();
  expose("setupAdminAccount", &Cluster::setup_admin_account, "user", "?options")
      ->cli();
  expose("setupRouterAccount", &Cluster::setup_router_account, "user",
         "?options")
      ->cli();
  expose("addReplicaInstance", &Cluster::add_replica_instance, "instance",
         "?options")
      ->cli();

  expose("execute", &Cluster::execute, "cmd", "instances", "?options")->cli();
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

void Cluster::assert_valid(const std::string &option_name) const {
  std::string name;

  if (option_name == "disconnect" || option_name == "name") return;

  if (has_member(option_name) && m_invalidated) {
    if (has_method(option_name)) {
      name = get_function_name(option_name, false);
      throw shcore::Exception::runtime_error("Can't call function '" + name +
                                             "' on an offline Cluster");
    } else {
      name = get_member_name(option_name, shcore::current_naming_style());
      throw shcore::Exception::runtime_error("Can't access object member '" +
                                             name + "' on an offline Cluster");
    }
  }

  if (!impl()->check_valid()) {
    throw shcore::Exception::runtime_error(
        "The cluster object is disconnected. Please use dba." +
        get_function_name("getCluster", false) +
        "() to obtain a fresh cluster handle.");
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
@li recoveryProgress: Integer value to indicate the recovery process verbosity
level.
${CLUSTER_OPT_IP_ALLOWLIST}
${CLUSTER_OPT_LOCAL_ADDRESS}
${CLUSTER_OPT_EXIT_STATE_ACTION}
${CLUSTER_OPT_MEMBER_WEIGHT}
${CLUSTER_OPT_AUTO_REJOIN_TRIES}
${OPT_CERT_SUBJECT}

The label must be non-empty and no greater than 256 characters long. It must be
unique within the Cluster and can only contain alphanumeric, _ (underscore),
. (period), - (hyphen), or : (colon) characters.

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

The recoveryProgress option supports the following values:

@li 0: do not show any progress information.
@li 1: show detailed static progress information.
@li 2: show detailed dynamic progress information using progress bars.

${CLUSTER_OPT_EXIT_STATE_ACTION_DETAIL}

${CLUSTER_OPT_IP_ALLOWLIST_EXTRA}

The localAddress and groupSeeds are advanced options and their usage is
discouraged since incorrect values can lead to Group Replication errors.

${CLUSTER_OPT_LOCAL_ADDRESS_EXTRA}

${CLUSTER_OPT_EXIT_STATE_ACTION_EXTRA}

${CLUSTER_OPT_MEMBER_WEIGHT_DETAIL_EXTRA}

${CLUSTER_OPT_AUTO_REJOIN_TRIES_EXTRA}
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
void Cluster::add_instance(
    const Connection_options &instance_def,
    const shcore::Option_pack_ref<cluster::Add_instance_options> &options) {
  // Throw an error if the cluster has already been dissolved
  assert_valid("addInstance");

  validate_instance_label(options->label, *impl());

  return execute_with_pool(
      [&]() {
        // Add the Instance to the Cluster
        impl()->add_instance(instance_def, *options);
      },
      false);
}

REGISTER_HELP_FUNCTION(rejoinInstance, Cluster);
REGISTER_HELP_FUNCTION_TEXT(CLUSTER_REJOININSTANCE, R"*(
Rejoins an Instance to the cluster.

@param instance An instance definition.
@param options Optional dictionary with options for the operation.

@returns A JSON object with the result of the operation.

This function rejoins an Instance to the cluster.

The instance definition is the connection data for the instance.

${TOPIC_CONNECTION_MORE_INFO}

The options dictionary may contain the following attributes:

@li recoveryMethod: Preferred method of state recovery. May be auto, clone or
incremental. Default is auto.
@li recoveryProgress: Integer value to indicate the recovery process verbosity
level.
recovery process to finish and its verbosity level.
${CLUSTER_OPT_IP_ALLOWLIST}
${CLUSTER_OPT_LOCAL_ADDRESS}
@li dryRun: boolean if true, all validations and steps for rejoining the
instance are executed, but no changes are actually made.
@li cloneDonor: The Cluster member to be used as donor when performing
clone-based recovery. Available only for Read Replicas.
@li timeout: maximum number of seconds to wait for the instance to sync up
with the PRIMARY after it's provisioned and the replication channel is
established. If reached, the operation is rolled-back. Default is 0 (no
timeout). Available only for Read Replicas.

The recoveryMethod option supports the following values:

@li incremental: uses distributed state recovery, which applies missing
transactions copied from another cluster member. Clone will be disabled.
@li clone: uses built-in MySQL clone support, which completely replaces
the state of the target instance with a full snapshot of another cluster member
before distributed recovery starts. Requires MySQL 8.0.17 or newer.
@li auto: let Group Replication choose whether or not a full snapshot has to be
taken, based on what the target server supports and the
group_replication_clone_threshold sysvar.
This is the default value. A prompt will be shown if not possible to safely
determine a safe way forward. If interaction is disabled, the operation will be
canceled instead.

If recoveryMethod is not specified 'auto' will be used by default.

The recoveryProgress option supports the following values:

@li 0: do not show any progress information.
@li 1: show detailed static progress information.
@li 2: show detailed dynamic progress information using progress bars.

By default, if the standard output on which the Shell is running refers to a
terminal, the recoveryProgress option has the value of 2. Otherwise, it has the
value of 1.

${CLUSTER_OPT_IP_ALLOWLIST_EXTRA}

The localAddress is an advanced option and its usage is discouraged
since incorrect values can lead to Group Replication errors.

${CLUSTER_OPT_LOCAL_ADDRESS_EXTRA}
)*");

/**
 * $(CLUSTER_REJOININSTANCE_BRIEF)
 *
 * $(CLUSTER_REJOININSTANCE)
 */
#if DOXYGEN_JS
Dictionary Cluster::rejoinInstance(InstanceDef instance, Dictionary options) {}
#elif DOXYGEN_PY
Dict Cluster::rejoin_instance(InstanceDef instance, dict options) {}
#endif

void Cluster::rejoin_instance(
    const Connection_options &instance_def,
    const shcore::Option_pack_ref<cluster::Rejoin_instance_options> &options) {
  // Throw an error if the cluster has already been dissolved
  assert_valid("rejoinInstance");

  return execute_with_pool(
      [&]() {
        // Rejoin the Instance to the Cluster
        impl()->rejoin_instance(instance_def, *options);
      },
      false);
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

@li force: boolean, indicating if the instance must be removed (even if only
from metadata) in case it cannot be reached. By default, set to false.
@li dryRun: boolean if true, all validations and steps for removing the
instance are executed, but no changes are actually made. An exception will be
thrown when finished.
@li timeout: maximum number of seconds to wait for the instance to sync up
with the PRIMARY. If reached, the operation is rolled-back. Default is 0 (no
timeout).

The force option (set to true) must only be used to remove instances that are
permanently not available (no longer reachable) or never to be reused again in
a cluster. This allows to remove from the metadata an instance than can no
longer be recovered. Otherwise, the instance must be brought back ONLINE and
removed without the force option to avoid errors trying to add it back to a
cluster.
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

void Cluster::remove_instance(
    const Connection_options &instance_def,
    const shcore::Option_pack_ref<cluster::Remove_instance_options> &options) {
  // Throw an error if the cluster has already been dissolved
  assert_valid("removeInstance");

  return execute_with_pool(
      [&]() {
        // Remove the Instance from the Cluster
        impl()->remove_instance(instance_def, *options);
      },
      false);
}

REGISTER_HELP_FUNCTION(describe, Cluster);
REGISTER_HELP_FUNCTION_TEXT(CLUSTER_DESCRIBE, R"*(
Describe the structure of the Cluster.

@returns A JSON object describing the structure of the Cluster.

This function describes the structure of the Cluster including all its
information and members.

The returned JSON object contains the following attributes:

@li clusterName: the name of the Cluster
@li defaultReplicaSet: the default ReplicaSet object

The defaultReplicaSet JSON object contains the following attributes:

@li name: the ReplicaSet name (default)
@li topology: a list of dictionaries describing each instance belonging to the
Cluster.
@li topologyMode: the InnoDB Cluster topology mode.

Each instance dictionary contains the following attributes:

@li address: the instance address in the form of host:port
@li label: the instance name identifier
@li role: the instance role
)*");

/**
 * $(CLUSTER_DESCRIBE_BRIEF)
 *
 * $(CLUSTER_DESCRIBE)
 */
#if DOXYGEN_JS
Dictionary Cluster::describe() {}
#elif DOXYGEN_PY
dict Cluster::describe() {}
#endif

shcore::Value Cluster::describe(void) {
  // Throw an error if the cluster has already been dissolved
  assert_valid("describe");

  return execute_with_pool([&]() { return impl()->describe(); }, false);
}

REGISTER_HELP_FUNCTION(status, Cluster);
REGISTER_HELP_FUNCTION_TEXT(CLUSTER_STATUS, R"*(
Describe the status of the Cluster.

@param options Optional dictionary with options.

@returns A JSON object describing the status of the Cluster.

This function describes the status of the Cluster and its members. The
following options may be given to control the amount of information gathered
and returned.

@li extended: verbosity level of the command output.

The extended option supports Integer or Boolean values:

@li 0: disables the command verbosity (default);
@li 1: includes information about the Metadata Version, Group Protocol
       Version, Group name, cluster member UUIDs, cluster member roles and
       states as reported by Group Replication and the list of fenced system
       variables;
@li 2: includes information about transactions processed by connection and
       applier;
@li 3: includes more detailed stats about the replication machinery of each
       cluster member;
@li Boolean: equivalent to assign either 0 (false) or 1 (true).
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

shcore::Value Cluster::status(
    const shcore::Option_pack_ref<cluster::Status_options> &options) {
  // Throw an error if the cluster has already been dissolved
  assert_valid("status");

  return execute_with_pool([&]() { return impl()->status(options->extended); },
                           false);
}

REGISTER_HELP_FUNCTION(options, Cluster);
REGISTER_HELP_FUNCTION_TEXT(CLUSTER_OPTIONS, R"*(
Lists the cluster configuration options.

@param options Optional Dictionary with options.

@returns A JSON object describing the configuration options of the cluster.

This function lists the cluster configuration options for its ReplicaSets and
Instances. The following options may be given to control the amount of
information gathered and returned.

@li all: if true, includes information about all group_replication system
variables.
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

shcore::Value Cluster::options(
    const shcore::Option_pack_ref<cluster::Options_options> &options) {
  // Throw an error if the cluster has already been dissolved
  assert_valid("options");

  return execute_with_pool([&]() { return impl()->options(options->all); },
                           false);
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

The force option (set to true) must only be used to dissolve a cluster with
instances that are permanently not available (no longer reachable) or never to
be reused again in a cluster. This allows to dissolve a cluster and remove it
from the metadata, including instances than can no longer be recovered.
Otherwise, the instances must be brought back ONLINE and the cluster dissolved
without the force option to avoid errors trying to reuse the instances and add
them back to a cluster.
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

void Cluster::dissolve(const shcore::Option_pack_ref<Force_options> &options) {
  // Throw an error if the cluster has already been dissolved
  assert_valid("dissolve");

  return execute_with_pool(
      [&]() {
        impl()->dissolve(options->force,
                         current_shell_options()->get().wizards);
        // Set the flag, marking this cluster instance as invalid.
        invalidate();
      },
      false);
}

REGISTER_HELP_FUNCTION(resetRecoveryAccountsPassword, Cluster);
REGISTER_HELP_FUNCTION_TEXT(CLUSTER_RESETRECOVERYACCOUNTSPASSWORD, R"*(
Resets the password of the recovery and replication accounts of the Cluster.

@param options Dictionary with options for the operation.

@returns Nothing.

This function resets the passwords for all internal recovery user accounts
used by the Cluster as well as for all replication accounts used by any
Read Replica instance.
It can be used to reset the passwords of the recovery or replication user
accounts when needed for any security reasons. For example:
periodically to follow some custom password lifetime policy, or after
some security breach event.

The options dictionary may contain the following attributes:

@li force: boolean, indicating if the operation will continue in case an
error occurs when trying to reset the passwords on any of the
instances, for example if any of them is not online. By default, set to false.

The use of the force option (set to true) is not recommended. Use it only
if really needed when instances are permanently not available (no longer
reachable) or never going to be reused again in a cluster. Prefer to bring the
non available instances back ONLINE or remove them from the cluster if they will
no longer be used.
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
    const shcore::Option_pack_ref<Force_options> &options) {
  // Throw an error if the cluster has already been dissolved
  assert_valid("resetRecoveryAccountsPassword");

  return execute_with_pool(
      [&]() {
        // Reset the recovery passwords.
        impl()->reset_recovery_password(options->force,
                                        current_shell_options()->get().wizards);
      },
      false);
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
metadata. Deprecated.
@li removeInstances: List with the connection data of the obsolete instances to
remove from the metadata, or "auto" to automatically remove obsolete instances
from the metadata. Deprecated.
@li upgradeCommProtocol: boolean. Set to true to upgrade the Group Replication
communication protocol to the highest version possible.
@li updateViewChangeUuid: boolean value used to indicate if the command should
generate and set a value for Group Replication View Change UUID in the whole
Cluster. Required for InnoDB ClusterSet usage (if running MySQL version lower
than 8.3.0).
@li addUnmanaged: set to true to automatically add newly discovered instances,
i.e. already part of the replication topology but not managed in the Cluster,
to the metadata. Defaults to false.
@li removeObsolete: set to true to automatically remove all obsolete instances,
i.e. no longer part of the replication topology, from the metadata. Defaults
to false.
@li repairMetadata: boolean. Set to true to repair the Metadata if detected to
be inconsistent.

The value for addInstances and removeInstances is used to specify which
instances to add or remove from the metadata, respectively. Both options accept
list connection data. In addition, the "auto" value can be used for both
options in order to automatically add or remove the instances in the metadata,
without having to explicitly specify them.

'repairMetadata' is used to eliminate any inconsistencies detected in the
Metadata. These inconsistencies may arise from a few scenarios, such as the
failure of one or more commands. Clusters detected in the ClusterSet Metadata
that do not qualify as valid members will be removed.

@attention The addInstances and removeInstances options will be removed in a
future release. Use addUnmanaged and removeObsolete instead.
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
void Cluster::rescan(
    const shcore::Option_pack_ref<cluster::Rescan_options> &options) {
  assert_valid("rescan");

  return execute_with_pool([&]() { impl()->rescan(*options); }, false);
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

void Cluster::disconnect() { impl()->disconnect(); }

REGISTER_HELP_FUNCTION(forceQuorumUsingPartitionOf, Cluster);
REGISTER_HELP_FUNCTION_TEXT(CLUSTER_FORCEQUORUMUSINGPARTITIONOF, R"*(
Restores the cluster from quorum loss.

@param instance An instance definition to derive the forced group from.

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
)*");

/**
 * $(CLUSTER_FORCEQUORUMUSINGPARTITIONOF_BRIEF)
 *
 * $(CLUSTER_FORCEQUORUMUSINGPARTITIONOF)
 */
#if DOXYGEN_JS
Undefined Cluster::forceQuorumUsingPartitionOf(InstanceDef instance) {}
#elif DOXYGEN_PY
None Cluster::force_quorum_using_partition_of(InstanceDef instance) {}
#endif

void Cluster::force_quorum_using_partition_of(
    const Connection_options &instance_def_) {
  // Throw an error if the cluster has already been dissolved
  assert_valid("forceQuorumUsingPartitionOf");

  auto instance_def = instance_def_;
  if (!instance_def.has_port() && !instance_def.has_socket()) {
    instance_def.set_port(mysqlshdk::db::k_default_mysql_port);
  }

  return execute_with_pool(
      [&]() {
        impl()->force_quorum_using_partition_of(
            instance_def, current_shell_options()->get().wizards);
      },
      false);
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

  return execute_with_pool(
      [&]() {
        // Switch to single-primary mode
        impl()->switch_to_single_primary_mode(instance_def);
      },
      false);
}

REGISTER_HELP_FUNCTION(switchToMultiPrimaryMode, Cluster);
REGISTER_HELP_FUNCTION_TEXT(CLUSTER_SWITCHTOMULTIPRIMARYMODE, R"*(
Switches the cluster to multi-primary mode.

@returns Nothing.

This function changes a cluster running in single-primary mode to multi-primary
mode.
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

  return execute_with_pool(
      [&]() {
        // Switch to multi-primary mode
        impl()->switch_to_multi_primary_mode();
      },
      false);
}

REGISTER_HELP_FUNCTION(setPrimaryInstance, Cluster);
REGISTER_HELP_FUNCTION_TEXT(CLUSTER_SETPRIMARYINSTANCE, R"*(
Elects a specific cluster member as the new primary.

@param instance An instance definition.
@param options Optional dictionary with options for the operation.

@returns Nothing.

This function forces the election of a new primary, overriding any
election process.

The instance definition is the connection data for the instance.

${TOPIC_CONNECTION_MORE_INFO}

The instance definition is mandatory and is the identifier of the cluster
member that shall become the new primary.

The options dictionary may contain the following attributes:

@li runningTransactionsTimeout: integer value to define the time period, in
seconds, that the primary election waits for ongoing transactions to complete.
After the timeout is reached, any ongoing transaction is rolled back allowing
the operation to complete.

)*");

/**
 * $(CLUSTER_SETPRIMARYINSTANCE_BRIEF)
 *
 * $(CLUSTER_SETPRIMARYINSTANCE)
 */
#if DOXYGEN_JS
Undefined Cluster::setPrimaryInstance(InstanceDef instance,
                                      Dictionary options) {}
#elif DOXYGEN_PY
None Cluster::set_primary_instance(InstanceDef instance, dict options) {}
#endif

void Cluster::set_primary_instance(
    const Connection_options &instance_def,
    const shcore::Option_pack_ref<cluster::Set_primary_instance_options>
        &options) {
  assert_valid("setPrimaryInstance");

  return execute_with_pool(
      [&]() {
        // Set primary-instance
        impl()->set_primary_instance(instance_def, *options);
      },
      false);
}

REGISTER_HELP_FUNCTION(setOption, Cluster);
REGISTER_HELP_FUNCTION_TEXT(CLUSTER_SETOPTION, R"*(
Changes the value of an option for the whole Cluster.

@param option The option to be changed.
@param value The value that the option shall get.

@returns Nothing.

This function changes an option for the Cluster.

The accepted options are:
${NAMESPACE_TAG}
@li clusterName: string value to define the cluster name.
${CLUSTER_OPT_EXIT_STATE_ACTION}
${CLUSTER_OPT_MEMBER_WEIGHT}
${CLUSTER_OPT_CONSISTENCY}
${CLUSTER_OPT_EXPEL_TIMEOUT}
${CLUSTER_OPT_AUTO_REJOIN_TRIES}
${CLUSTER_OPT_IP_ALLOWLIST}
@li disableClone: boolean value used to disable the clone usage on the cluster.
@li replicationAllowedHost string value to use as the host name part of
internal replication accounts. Existing accounts will be re-created with the new
value.
${CLUSTER_OPT_TRANSACTION_SIZE_LIMIT}
${CLUSTER_OPT_REPLICATION_OPTION_CONNECT_RETRY}
${CLUSTER_OPT_REPLICATION_OPTION_RETRY_COUNT}
${CLUSTER_OPT_REPLICATION_OPTION_HEARTBEAT_PERIOD}
${CLUSTER_OPT_REPLICATION_OPTION_COMPRESSION_ALGORITHMS}
${CLUSTER_OPT_REPLICATION_OPTION_ZSTD_COMPRESSION_LEVEL}
${CLUSTER_OPT_REPLICATION_OPTION_BIND}
${CLUSTER_OPT_REPLICATION_OPTION_NETWORK_NAMESPACE}

The clusterName must be non-empty and no greater than 63 characters long. It
can only start with an alphanumeric character or with _ (underscore), and can
only contain alphanumeric, _ ( underscore), . (period), or - (hyphen)
characters.

@attention The transactionSizeLimit option is not supported on Replica Clusters of InnoDB ClusterSets.

@note Changing any of the "clusterSetReplication*" options won't immediately
update the replication channel. ClusterSet.rejoinCluster() must be used to to reconfigure
and restart the replication channel of that Cluster.
@note Any of the "clusterSetReplication*" options accepts 'null', which resets
the corresponding option to its default value on the next call to
ClusterSet.rejoinCluster().

The value for the configuration option is used to set the Group Replication
system variable that corresponds to it.

${CLUSTER_OPT_EXIT_STATE_ACTION_DETAIL}

${CLUSTER_OPT_CONSISTENCY_DETAIL}

${CLUSTER_OPT_EXIT_STATE_ACTION_EXTRA}

${CLUSTER_OPT_MEMBER_WEIGHT_DETAIL_EXTRA}

${CLUSTER_OPT_CONSISTENCY_EXTRA}

${CLUSTER_OPT_EXPEL_TIMEOUT_EXTRA}

${CLUSTER_OPT_AUTO_REJOIN_TRIES_EXTRA}

${CLUSTER_OPT_TRANSACTION_SIZE_LIMIT_EXTRA}

${CLUSTER_OPT_IP_ALLOWLIST_EXTRA}

${NAMESPACE_TAG_DETAIL_CLUSTER}
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

  return execute_with_pool([&]() { impl()->set_option(option, value); }, false);
}

REGISTER_HELP_FUNCTION(setInstanceOption, Cluster);
REGISTER_HELP_FUNCTION_TEXT(CLUSTER_SETINSTANCEOPTION, R"*(
Changes the value of an option in a Cluster member.

@param instance An instance definition.
@param option The option to be changed.
@param value The value that the option shall get.

@returns Nothing.

This function changes an option for a member of the cluster.

The instance definition is the connection data for the instance.

${TOPIC_CONNECTION_MORE_INFO}

The accepted options are:
${NAMESPACE_TAG}
${CLUSTER_OPT_EXIT_STATE_ACTION}
${CLUSTER_OPT_MEMBER_WEIGHT}
${CLUSTER_OPT_AUTO_REJOIN_TRIES}
${CLUSTER_OPT_IP_ALLOWLIST}
@li label: a string identifier of the instance.
@li replicationSources: The list of sources for a Read Replica Instance.

${CLUSTER_OPT_EXIT_STATE_ACTION_DETAIL}

${CLUSTER_OPT_EXIT_STATE_ACTION_EXTRA}

${CLUSTER_OPT_MEMBER_WEIGHT_DETAIL_EXTRA}

${CLUSTER_OPT_AUTO_REJOIN_TRIES_EXTRA}

${CLUSTER_OPT_IP_ALLOWLIST_EXTRA}

${NAMESPACE_TAG_DETAIL_CLUSTER}

${NAMESPACE_TAG_INSTANCE_DETAILS_EXTRA}

The label must be non-empty and no greater than 256 characters long. It must be
unique within the Cluster and can only contain alphanumeric, _ (underscore),
. (period), - (hyphen), or : (colon) characters.

The replicationSources is a comma separated list of instances (host:port) to
act as sources of the replication channel, i.e. to provide source failover of
the channel. The first member of the list is configured with the highest
priority among all members so when the channel activates it will be chosen for
the first connection attempt. By default, the source list is automatically
managed by Group Replication according to the current group membership and the
primary member of the Cluster is the current source for the replication
channel, this is the same as setting to "primary". Alternatively, it's
possible to set to "secondary" to instruct Group Replication to automatically
manage the list but use a secondary member of the Cluster as source.

For the change to be effective, the Read-Replica must be reconfigured after
using Cluster.<<<rejoinInstance>>>().
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

  return execute_with_pool(
      [&]() {
        // Set the option in the Default ReplicaSet
        impl()->set_instance_option(instance_def.as_uri(), option, value);
      },
      false);
}

REGISTER_HELP_FUNCTION(listRouters, Cluster);
REGISTER_HELP_FUNCTION_TEXT(CLUSTER_LISTROUTERS, LISTROUTERS_HELP_TEXT);

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

REGISTER_HELP_FUNCTION(setRoutingOption, Cluster);
REGISTER_HELP_FUNCTION_TEXT(CLUSTER_SETROUTINGOPTION, R"*(
Changes the value of either a global Cluster Routing option or of a single
Router instance.

@param router optional identifier of the target router instance (e.g.
192.168.45.70@::system).
@param option The Router option to be changed.
@param value The value that the option shall get (or null to unset).

@returns Nothing.

The accepted options are:

@li tags: Associates an arbitrary JSON object with custom key/value pairs with
the Cluster metadata.
@li read_only_targets: Routing policy to define Router's usage of Read Only
instance. Default is 'secondaries'.
@li stats_updates_frequency: Number of seconds between updates that the Router
is to make to its statistics in the InnoDB Cluster metadata.
@li unreachable_quorum_allowed_traffic: Routing policy to define Router's behavior
regarding traffic destinations (ports) when it loses access to the Cluster's quorum.

The read_only_targets option supports the following values:

@li all: All Read Replicas of the target Cluster should be used along the
other SECONDARY Cluster members for R/O traffic.
@li read_replicas: Only Read Replicas of the target Cluster should be used for
R/O traffic.
@li secondaries: Only Secondary members of the target Cluster should be used
for R/O traffic (default).

The stats_updates_frequency option accepts positive integers and sets the
frequency of updates of Router stats (timestamp, version, etc.), in seconds,
in the Metadata. If set to 0 (default), no periodic updates are done. Router
will round up the value to be a multiple of Router's TTL, i.e.:

@li If lower than TTL its gets rounded up to TTL, e.g. TTL=30, and
stats_updates_frequency=1, effective frequency is 30 seconds.

@li If not a multiple of TTL it will be rounded up and adjusted according to
the TTL, e.g. TTL=5, stats_updates_frequency=11, effective frequency is 15
seconds; TTL=5, stats_updates_frequency=13, effective frequency is 15 seconds.

If the value is null, the option value is cleared and the default value (0)
takes effect.

The unreachable_quorum_allowed_traffic option allows configuring Router's behavior
in the event of a loss of quorum on the only reachable Cluster partition. By
default, Router will disconnect all existing connections and refuse new ones,
but that can be configured using the following options:

@li read: Router will keep using ONLINE members as Read-Only destination, leaving
the RO and RW-split ports open.
@li all: Router will keep ONLINE members as Read/Write destinations, leaving all
ports (RW, RO, and RW-split), open.
@li none: All current connections are disconnected and new ones are refused (default behavior).

@attention Setting this option to a different value other than the default may have unwanted
consequences: the consistency guarantees provided by InnoDB Cluster are broken
since the data read can be stale; different Routers may be accessing different
partitions, thus return different data; and different Routers may also have different
behavior (i.e. some provide only read traffic while others read and write traffic). Note that
writes on a partition with no quorum will block until quorum is restored.

This option has no practical effect if group_replication_unreachable_majority_timeout
is set to a positive value and group_replication_exit_state_action is either
OFFLINE_MODE or ABORT_SERVER.
)*");

#if DOXYGEN_JS
Undefined Cluster::setRoutingOption(String option, String value) {}
#elif DOXYGEN_PY
None Cluster::set_routing_option(str option, str value) {}
#endif

/**
 * $(CLUSTER_SETROUTINGOPTION_BRIEF)
 *
 * $(CLUSTER_SETROUTINGOPTION)
 */
#if DOXYGEN_JS
Undefined Cluster::setRoutingOption(String router, String option,
                                    String value) {}
#elif DOXYGEN_PY
None Cluster::set_routing_option(str router, str option, str value) {}
#endif

REGISTER_HELP_FUNCTION(routingOptions, Cluster);
REGISTER_HELP_FUNCTION_TEXT(CLUSTER_ROUTINGOPTIONS, ROUTINGOPTIONS_HELP_TEXT);
/**
 * $(CLUSTER_ROUTINGOPTIONS_BRIEF)
 *
 * $(CLUSTER_ROUTINGOPTIONS)
 */
#if DOXYGEN_JS
Dictionary Cluster::routingOptions(String router) {}
#elif DOXYGEN_PY
dict Cluster::routing_options(str router) {}
#endif

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
)*");

REGISTER_HELP_FUNCTION(routerOptions, Cluster);
REGISTER_HELP_FUNCTION_TEXT(CLUSTER_ROUTEROPTIONS, R"*(
Lists the configuration options of the Cluster's Routers.

@param options Dictionary with options for the operation.

@returns A JSON object with the list of Router configuration options.

This function lists the Router configuration options of the Cluster (global),
and the Router instances. By default, only the options that can be changed from
Shell (dynamic options) are displayed.
Router instances with different configurations than the global ones will
include the differences under their dedicated description.

The options dictionary may contain the following attributes:

@li extended: Verbosity level of the command output.
@li router: Identifier of the Router instance to be displayed.

The extended option supports Integer or Boolean values:

@li 0: Includes only options that can be changed from Shell (default);
@li 1: Includes all Cluster global options and, per Router, only the options
that have a different value than the corresponding global one.
@li 2: Includes all Cluster and Router options.
@li Boolean: equivalent to assign either 0 (false) or 1 (true).
)*");

/**
 * $(CLUSTER_ROUTEROPTIONS_BRIEF)
 *
 * $(CLUSTER_ROUTEROPTIONS)
 */
#if DOXYGEN_JS
Dictionary Cluster::routerOptions(Dictionary options) {}
#elif DOXYGEN_PY
dict Cluster::router_options(dict options) {}
#endif

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

  return execute_with_pool(
      [&]() { impl()->remove_router_metadata(router_def); }, false);
}

REGISTER_HELP_FUNCTION(setupAdminAccount, Cluster);
REGISTER_HELP_FUNCTION_TEXT(CLUSTER_SETUPADMINACCOUNT,
                            SETUPADMINACCOUNT_HELP_TEXT);

/**
 * $(CLUSTER_SETUPADMINACCOUNT_BRIEF)
 *
 * $(CLUSTER_SETUPADMINACCOUNT)
 */
#if DOXYGEN_JS
Undefined Cluster::setupAdminAccount(String user, Dictionary options) {}
#elif DOXYGEN_PY
None Cluster::setup_admin_account(str user, dict options) {}
#endif

REGISTER_HELP_FUNCTION(setupRouterAccount, Cluster);
REGISTER_HELP_FUNCTION_TEXT(CLUSTER_SETUPROUTERACCOUNT,
                            SETUPROUTERACCOUNT_HELP_TEXT);

/**
 * $(CLUSTER_SETUPROUTERACCOUNT_BRIEF)
 *
 * $(CLUSTER_SETUPROUTERACCOUNT)
 */
#if DOXYGEN_JS
Undefined Cluster::setupRouterAccount(String user, Dictionary options) {}
#elif DOXYGEN_PY
None Cluster::setup_router_account(str user, dict options) {}
#endif

REGISTER_HELP_FUNCTION(fenceAllTraffic, Cluster);
REGISTER_HELP_FUNCTION_TEXT(CLUSTER_FENCEALLTRAFFIC, R"*(
Fences a Cluster from All Traffic.

@returns Nothing

This function fences a Cluster from all Traffic by ensuring the Group
Replication is completely shut down and all members are Read-Only and in
Offline mode, preventing regular client connections from connecting to it.

Use this function when performing a PRIMARY Cluster failover in a
ClusterSet to prevent a split-brain.

@attention The Cluster will be put OFFLINE but Cluster members will not be
shut down. This is the equivalent of a graceful shutdown of Group Replication.
To restore the Cluster use dba.<<<rebootClusterFromCompleteOutage>>>().
)*");

/**
 * $(CLUSTER_FENCEALLTRAFFIC_BRIEF)
 *
 * $(CLUSTER_FENCEALLTRAFFIC)
 */
#if DOXYGEN_JS
Undefined Cluster::fenceAllTraffic() {}
#elif DOXYGEN_PY
None Cluster::fence_all_traffic() {}
#endif
void Cluster::fence_all_traffic() {
  assert_valid("fenceAllTraffic");

  return execute_with_pool([&]() { impl()->fence_all_traffic(); }, false);

  // Invalidate the cluster object
  invalidate();
}

REGISTER_HELP_FUNCTION(fenceWrites, Cluster);
REGISTER_HELP_FUNCTION_TEXT(CLUSTER_FENCEWRITES, R"*(
Fences a Cluster from Write Traffic.

@returns Nothing

This function fences a Cluster member of a ClusterSet from all Write Traffic by
ensuring all of its members are Read-Only regardless of any topology change on
it. The Cluster will be put into READ ONLY mode and all members will remain
available for reads.
To unfence the Cluster so it restores its normal functioning and can accept all
traffic use Cluster.unfenceWrites().

Use this function when performing a PRIMARY Cluster failover in a ClusterSet to
allow only read traffic in the previous Primary Cluster in the event of a
split-brain.

The function is not permitted on standalone Clusters.
)*");
/**
 * $(CLUSTER_FENCEWRITES_BRIEF)
 *
 * $(CLUSTER_FENCEWRITES)
 */
#if DOXYGEN_JS
Undefined Cluster::fenceWrites() {}
#elif DOXYGEN_PY
None Cluster::fence_writes() {}
#endif
void Cluster::fence_writes() {
  assert_valid("fenceWrites");

  return execute_with_pool([&]() { impl()->fence_writes(); }, false);
}

REGISTER_HELP_FUNCTION(unfenceWrites, Cluster);
REGISTER_HELP_FUNCTION_TEXT(CLUSTER_UNFENCEWRITES, R"*(
Unfences a Cluster.

@returns Nothing

This function unfences a Cluster that was previously fenced to Write traffic
with Cluster.<<<fenceWrites>>>().

@attention This function does not unfence Clusters that have been fenced to ALL
traffic. Those Cluster are completely shut down and can only be restored using
dba.<<<rebootClusterFromCompleteOutage>>>().
)*");
/**
 * $(CLUSTER_UNFENCEWRITES_BRIEF)
 *
 * $(CLUSTER_UNFENCEWRITES)
 */
#if DOXYGEN_JS
Undefined Cluster::unfenceWrites() {}
#elif DOXYGEN_PY
None Cluster::unfence_writes() {}
#endif
void Cluster::unfence_writes() {
  assert_valid("unfenceWrites");

  return execute_with_pool([&]() { impl()->unfence_writes(); }, false);
}

REGISTER_HELP_FUNCTION(createClusterSet, Cluster);
REGISTER_HELP_FUNCTION_TEXT(CLUSTER_CREATECLUSTERSET, R"*(
Creates a MySQL InnoDB ClusterSet from an existing standalone InnoDB Cluster.

@param domainName An identifier for the ClusterSet's logical dataset.
@param options Optional dictionary with additional parameters described below.

@returns The created ClusterSet object.

Creates a ClusterSet object from an existing cluster, with the given data
domain name.

Several checks and validations are performed to ensure that the target Cluster
complies with the requirements for ClusterSets and if so, the Metadata schema
will be updated to create the new ClusterSet and the target Cluster becomes
the PRIMARY cluster of the ClusterSet.

<b>InnoDB ClusterSet</b>

A ClusterSet is composed of a single PRIMARY InnoDB Cluster that can have one
or more replica InnoDB Clusters that replicate from the PRIMARY using
asynchronous replication.

ClusterSets allow InnoDB Cluster deployments to achieve fault-tolerance at
a whole Data Center / region or geographic location, by creating REPLICA clusters
in different locations (Data Centers), ensuring Disaster Recovery is possible.

If the PRIMARY InnoDB Cluster becomes completely unavailable, it's possible to
promote a REPLICA of that cluster to take over its duties with minimal downtime
or data loss.

All Cluster operations are available at each individual member (cluster) of the
ClusterSet. The AdminAPI ensures all updates are performed at the PRIMARY and
controls the command availability depending on the individual status of each
Cluster.

Please note that InnoDB ClusterSets don't have the same consistency and data
loss guarantees as InnoDB Clusters. To read more about ClusterSets, see \?
ClusterSet or refer to the MySQL manual.

<b>Pre-requisites</b>

The following is a non-exhaustive list of requirements to create a ClusterSet:

@li The target cluster must not already be part of a ClusterSet.
@li MySQL 8.0.27 or newer.
@li The target cluster's Metadata schema version is 2.1.0 or newer.
@li Unmanaged replication channels are not allowed.

The domainName must be non-empty and no greater than 63 characters long. It can
only start with an alphanumeric character or with _ (underscore), and can only
contain alphanumeric, _ ( underscore), . (period), or - (hyphen) characters.

<b>Options</b>

The options dictionary can contain the following values:

@li dryRun: boolean if true, all validations and steps for creating a
ClusterSet are executed, but no changes are actually made. An
exception will be thrown when finished.
@li clusterSetReplicationSslMode: SSL mode for the ClusterSet replication channels.
@li replicationAllowedHost: string value to use as the host name part of
internal replication accounts (i.e. 'mysql_innodb_cs_###'@'hostname').
Default is %. It must be possible for any member of the ClusterSet to connect to
any other member using accounts with this hostname value.

The clusterSetReplicationSslMode option supports the following values:

@li DISABLED: TLS encryption is disabled for the ClusterSet replication channels.
@li REQUIRED: TLS encryption is enabled for the ClusterSet replication channels.
@li VERIFY_CA: like REQUIRED, but additionally verify the peer server TLS certificate against the configured Certificate Authority (CA) certificates.
@li VERIFY_IDENTITY: like VERIFY_CA, but additionally verify that the peer server certificate matches the host to which the connection is attempted.
@li AUTO: TLS encryption will be enabled if supported by the instance, otherwise disabled.

If clusterSetReplicationSslMode is not specified, it defaults to the value of the cluster's memberSslMode option.
)*");

/**
 * $(CLUSTER_CREATECLUSTERSET_BRIEF)
 *
 * $(CLUSTER_CREATECLUSTERSET)
 */
#if DOXYGEN_JS
ClusterSet Cluster::createClusterSet(String domainName, Dictionary options) {}
#elif DOXYGEN_PY
ClusterSet Cluster::create_cluster_set(str domainName, dict options) {}
#endif
shcore::Value Cluster::create_cluster_set(
    const std::string &domain_name,
    const shcore::Option_pack_ref<clusterset::Create_cluster_set_options>
        &options) {
  // Throw an error if the cluster has already been dissolved
  assert_valid("createClusterSet");

  return execute_with_pool(
      [&]() { return impl()->create_cluster_set(domain_name, *options); },
      false);
}

REGISTER_HELP_FUNCTION(getClusterSet, Cluster);
REGISTER_HELP_FUNCTION_TEXT(CLUSTER_GETCLUSTERSET, R"*(
Returns an object representing a ClusterSet.

@returns The ClusterSet object to which the current cluster belongs to.

The returned object is identical to the one returned by
<<<createClusterSet>>>() and can be used to manage the ClusterSet.

The function will work regardless of whether the target cluster is a
PRIMARY or a REPLICA Cluster, but its copy of the metadata is expected
to be up-to-date.

This function will also work if the PRIMARY Cluster is unreachable or
unavailable, although ClusterSet change operations will not be possible, except
for failover with <<<forcePrimaryCluster>>>().
)*");
/**
 * $(CLUSTER_GETCLUSTERSET_BRIEF)
 *
 * $(CLUSTER_GETCLUSTERSET)
 */
#if DOXYGEN_JS
ReplicaSet Cluster::getClusterSet() {}
#elif DOXYGEN_PY
ReplicaSet Cluster::get_cluster_set() {}
#endif
std::shared_ptr<ClusterSet> Cluster::get_cluster_set() const {
  assert_valid("getClusterSet");

  // Init the connection pool
  impl()->get_metadata_storage()->invalidate_cached();
  Scoped_instance_pool ipool(
      impl()->get_metadata_storage(), false,
      Instance_pool::Auth_options(impl()->default_admin_credentials()));

  return impl()->get_cluster_set();
}

// Read-Replicas

REGISTER_HELP_FUNCTION(addReplicaInstance, Cluster);
REGISTER_HELP_FUNCTION_TEXT(CLUSTER_ADDREPLICAINSTANCE, R"*(
Adds a Read Replica Instance to the Cluster.

@param instance host:port of the target instance to be added as a Read Replica.
@param options optional Dictionary with additional parameters described below.

@returns nothing

This function adds an Instance acting as Read-Replica of an InnoDB Cluster.

<b>Pre-requisites</b>

The following is a list of requirements to create a REPLICA cluster:

@li The target instance must be a standalone instance.
@li The target instance and Cluster must running MySQL 8.0.23 or newer.
@li Unmanaged replication channels are not allowed.
@li The target instance server_id and server_uuid must be unique in the
topology, including among OFFLINE or unreachable members

<b>Options</b>

The options dictionary may contain the following values:

@li dryRun: boolean if true, all validations and steps for creating a
Read Replica Instance are executed, but no changes are actually made. An
exception will be thrown when finished.
@li label: an identifier for the Read Replica Instance being added, used in
the output of status() and describe().
@li replicationSources: The list of sources for the Read Replica Instance. By
default, the list is automatically managed by Group Replication and the
primary member is used as source.
@li recoveryMethod: Preferred method for state recovery/provisioning. May be
auto, clone or incremental. Default is auto.
@li recoveryProgress: Integer value to indicate the recovery process verbosity
level.
@li timeout: maximum number of seconds to wait for the instance to sync up
with the PRIMARY after it's provisioned and the replication channel is
established. If reached, the operation is rolled-back. Default is 0 (no
timeout).
${CLUSTER_OPT_CLONE_DONOR}
${OPT_CERT_SUBJECT}

The label must be non-empty and no greater than 256 characters long. It must be
unique within the Cluster and can only contain alphanumeric, _ (underscore),
. (period), - (hyphen), or : (colon) characters.

The replicationSources is a comma separated list of instances (host:port) to
act as sources of the replication channel, i.e. to provide source failover of
the channel. The first member of the list is configured with the highest
priority among all members so when the channel activates it will be chosen for
the first connection attempt. By default, the source list is automatically
managed by Group Replication according to the current group membership and the
primary member of the Cluster is the current source for the replication
channel, this is the same as setting to "primary". Alternatively, it's
possible to set to "secondary" to instruct Group Replication to automatically
manage the list too but use a secondary member of the Cluster as source.

The recoveryMethod option supports the following values:

@li incremental: waits until the new instance has applied missing transactions
from the source.
@li clone: uses MySQL clone to provision the instance, which completely
replaces the state of the target instance with a full snapshot of the
instance's source.
@li auto: compares the transaction set of the instance with that of the
source to determine if incremental recovery is safe to be automatically
chosen as the most appropriate recovery method.
A prompt will be shown if not possible to safely determine a safe way forward.
If interaction is disabled, the operation will be canceled instead.

If recoveryMethod is not specified 'auto' will be used by default.

The recoveryProgress option supports the following values:

@li 0: do not show any progress information.
@li 1: show detailed static progress information.
@li 2: show detailed dynamic progress information using progress bars.

By default, if the standard output on which the Shell is running refers to a
terminal, the recoveryProgress option has the value of 2. Otherwise, it has the
value of 1.
)*");

/**
 * $(CLUSTER_ADDREPLICAINSTANCE_BRIEF)
 *
 * $(CLUSTER_ADDREPLICAINSTANCE)
 */
#if DOXYGEN_JS
Undefined Cluster::addReplicaInstance(InstanceDef instance,
                                      Dictionary options) {}
#elif DOXYGEN_PY
None Cluster::add_replica_instance(InstanceDef instance, dict options);
#endif
void Cluster::add_replica_instance(
    const std::string &instance_def,
    const shcore::Option_pack_ref<cluster::Add_replica_instance_options>
        &options) {
  assert_valid("addReplicaInstance");

  validate_instance_label(options->label, *impl());

  return execute_with_pool(
      [&]() {
        return impl()->add_replica_instance(Connection_options{instance_def},
                                            *options);
      },
      false);
}

REGISTER_HELP_FUNCTION(execute, Cluster);
REGISTER_HELP_FUNCTION_TEXT(CLUSTER_EXECUTE, R"*(
Executes a SQL statement at selected instances of the Cluster.

@param cmd The SQL statement to execute.
@param instances The instances where cmd should be executed.
@param options Dictionary with options for the operation.

@returns A JSON object with a list of results / information regarding the
executing of the SQL statement on each of the target instances.

This function allows a single MySQL SQL statement to be executed on
multiple instances of the Cluster.

The 'instances' parameter can be either a string (keyword) or a list of
instance addresses where cmd should be executed. If a string, the allowed
keywords are:
@li "all" / "a": all reachable instances.
@li "primary" / "p": the primary instance on a single-primary Cluster or all primaries on a
multi-primary Cluster.
@li "secondaries" / "s": the secondary instances.
@li "read-replicas" / "rr": the read-replicas instances.

The options dictionary may contain the following attributes:

@li exclude: similar to the instances parameter, it can be either a string
(keyword) or a list of instance addresses to exclude from the instances
specified in instances. It accepts the same keywords, except "all".
@li timeout: integer value with the maximum number of seconds to wait for
cmd to execute in each target instance. Default value is 0 meaning it
doesn't timeout.
@li dryRun: boolean if true, all validations and steps for executing cmd
are performed, but no cmd is actually executed on any instance.

The keyword "secondaries" / "s" is not permitted on a multi-primary Cluster.

To calculate the final list of instances where cmd should be executed,
the function starts by parsing the instances parameter and then subtract
from that list the ones specified in the exclude option. For example, if
instances is "all" and exclude is "read-replicas", then all (primary and
secondaries) instances are targeted, except the read-replicas.
)*");

/**
 * $(CLUSTER_EXECUTE_BRIEF)
 *
 * $(CLUSTER_EXECUTE)
 */
#if DOXYGEN_JS
Dictionary Cluster::execute(String cmd, Object instances, Dictionary options) {}
#elif DOXYGEN_PY
dict Cluster::execute(str cmd, Object instances, dict options);
#endif

}  // namespace dba
}  // namespace mysqlsh
