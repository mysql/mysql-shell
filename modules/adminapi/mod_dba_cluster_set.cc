/*
 * Copyright (c) 2020, 2024, Oracle and/or its affiliates.
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

#include "modules/adminapi/mod_dba_cluster_set.h"

#include "mysqlshdk/include/shellcore/utils_help.h"
#include "mysqlshdk/libs/utils/debug.h"

DEBUG_OBJ_ENABLE(ClusterSet);

namespace mysqlsh {
namespace dba {

// Documentation of the ClusterSet Class
REGISTER_HELP_CLASS_KW(
    ClusterSet, adminapi,
    (std::map<std::string, std::string>({{"FullType", "InnoDB ClusterSet"},
                                         {"Type", "ClusterSet"},
                                         {"type", "clusterset"}})));

REGISTER_HELP_CLASS_TEXT(CLUSTERSET, R"*(
Represents an InnoDB ClusterSet.

The clusterset object is the entry point to manage and monitor a MySQL
InnoDB ClusterSet.

ClusterSets allow InnoDB Cluster deployments to achieve fault-tolerance at a
whole Data Center / region or geographic location, by creating REPLICA clusters
in different locations (Data Centers), ensuring Disaster Recovery is possible.

For more help on a specific function, use the \\help shell command, e.g.:
\\help ClusterSet.<<<createReplicaCluster>>>
)*");

ClusterSet::ClusterSet(const std::shared_ptr<Cluster_set_impl> &clusterset)
    : m_impl(clusterset) {
  DEBUG_OBJ_ALLOC2(ClusterSet, [](void *ptr) {
    return "refs:" + std::to_string(reinterpret_cast<ClusterSet *>(ptr)
                                        ->shared_from_this()
                                        .use_count());
  });

  init();
}

ClusterSet::~ClusterSet() { DEBUG_OBJ_DEALLOC(ClusterSet); }

void ClusterSet::init() {
  add_property("name", "getName");

  expose("disconnect", &ClusterSet::disconnect)->cli(false);
  expose("createReplicaCluster", &ClusterSet::create_replica_cluster,
         "instanceDef", "clusterName", "?options")
      ->cli();
  expose("removeCluster", &ClusterSet::remove_cluster, "clusterName",
         "?options")
      ->cli();
  expose("rejoinCluster", &ClusterSet::rejoin_cluster, "clusterName",
         "?options")
      ->cli();
  expose("setPrimaryCluster", &ClusterSet::set_primary_cluster, "clusterName",
         "?options")
      ->cli();
  expose("forcePrimaryCluster", &ClusterSet::force_primary_cluster,
         "clusterName", "?options")
      ->cli();
  expose("status", &ClusterSet::status, "?options")->cli();
  expose("describe", &ClusterSet::describe)->cli();
  expose("dissolve", &ClusterSet::dissolve, "?options")->cli();
  expose("setupAdminAccount", &ClusterSet::setup_admin_account, "user",
         "?options")
      ->cli();
  expose("setupRouterAccount", &ClusterSet::setup_router_account, "user",
         "?options")
      ->cli();
  expose("options", &ClusterSet::options)->cli();
  expose("setOption", &ClusterSet::set_option, "option", "value")->cli();

  expose("listRouters", &ClusterSet::list_routers, "?router")->cli();
  expose("routingOptions", &ClusterSet::routing_options, "?router")->cli();
  expose("routerOptions", &ClusterSet::router_options, "?options")->cli();
  // TODO(konrad): cli does not support yet such overloads
  expose("setRoutingOption", &ClusterSet::set_routing_option, "option",
         "value");
  expose("setRoutingOption", &ClusterSet::set_routing_option, "router",
         "option", "value");

  expose("execute", &ClusterSet::execute, "cmd", "instances", "?options")
      ->cli();
  expose("createRoutingGuideline", &ClusterSet::create_routing_guideline,
         "name", "?json", "?options")
      ->cli();
  expose("getRoutingGuideline", &ClusterSet::get_routing_guideline, "?name")
      ->cli();
  expose("removeRoutingGuideline", &ClusterSet::remove_routing_guideline,
         "name")
      ->cli();
  expose("routingGuidelines", &ClusterSet::routing_guidelines)->cli();
  expose("importRoutingGuideline", &ClusterSet::import_routing_guideline,
         "file", "?options")
      ->cli();
}

void ClusterSet::assert_valid(const std::string &function_name) const {
  if (function_name == "disconnect" || function_name == "name" ||
      function_name == "getName")
    return;

  if (!impl()->check_valid()) {
    throw shcore::Exception::runtime_error(
        "The ClusterSet object is disconnected. Please use "
        "dba." +
        get_function_name("getClusterSet", false) +
        "() to obtain a fresh handle.");
  }
}

// Documentation of the getName function
REGISTER_HELP_FUNCTION(getName, ClusterSet);
REGISTER_HELP_PROPERTY(name, ClusterSet);
REGISTER_HELP(CLUSTERSET_NAME_BRIEF, "${CLUSTERSET_GETNAME_BRIEF}");
REGISTER_HELP_FUNCTION_TEXT(CLUSTERSET_GETNAME, R"*(
Returns the domain name of the clusterset.

@returns domain name of the clusterset.
)*");

/**
 * $(CLUSTERSET_GETNAME_BRIEF)
 *
 * $(CLUSTERSET_GETNAME)
 */
#if DOXYGEN_JS
String ClusterSet::getName() {}
#elif DOXYGEN_PY
str ClusterSet::get_name() {}
#endif

REGISTER_HELP_FUNCTION(disconnect, ClusterSet);
REGISTER_HELP_FUNCTION_TEXT(CLUSTERSET_DISCONNECT, R"*(
Disconnects all internal sessions used by the ClusterSet object.

@returns Nothing.

Disconnects the internal MySQL sessions used by the ClusterSet to query for
metadata and replication information.
)*");

/**
 * $(CLUSTERSET_DISCONNECT_BRIEF)
 *
 * $(CLUSTERSET_DISCONNECT)
 */
#if DOXYGEN_JS
Undefined ClusterSet::disconnect() {}
#elif DOXYGEN_PY
None ClusterSet::disconnect() {}
#endif

void ClusterSet::disconnect() {
  assert_valid("disconnect");
  impl()->disconnect();
}

// Documentation of the createReplicaCluster function
REGISTER_HELP_FUNCTION(createReplicaCluster, ClusterSet);
REGISTER_HELP_FUNCTION_TEXT(CLUSTERSET_CREATEREPLICACLUSTER, R"*(
Creates a new InnoDB Cluster that is a Replica of the Primary Cluster.

@param instance host:port of the target instance to be used to create the
Replica Cluster
@param clusterName An identifier for the REPLICA cluster to be created.
@param options optional Dictionary with additional parameters described below.

@returns The created Replica Cluster object.

Creates a REPLICA InnoDB cluster of the current PRIMARY cluster with the given
cluster name and options, at the target instance.

If the target instance meets the requirements for InnoDB Cluster a new cluster
is created on it, replicating from the PRIMARY instance of the primary cluster
of the ClusterSet.

<b>Pre-requisites</b>

The following is a list of requirements to create a REPLICA cluster:

@li The target instance must comply with the requirements for InnoDB Cluster.
@li The target instance must be a standalone instance.
@li The target instance must running MySQL 8.0.27 or newer.
@li Unmanaged replication channels are not allowed.
@li The target instance 'server_id' and 'server_uuid' must be unique in the
ClusterSet
@li The target instance must have the same credentials used to manage the
ClusterSet.

The Replica Cluster's name must be non-empty and no greater than 63 characters
long. It can only start with an alphanumeric character or with _ (underscore),
and can only contain alphanumeric, _ ( underscore), . (period), or - ( hyphen)
characters.

For the detailed list of requirements to create an InnoDB Cluster, please use
\? createCluster

<b>Options</b>

The options dictionary can contain the following values:

@li dryRun: boolean if true, all validations and steps for creating a
Replica Cluster are executed, but no changes are actually made. An
exception will be thrown when finished.
@li recoveryMethod: Preferred method for state recovery/provisioning. May be
auto, clone or incremental. Default is auto.
@li recoveryProgress: Integer value to indicate the recovery process verbosity
level.
@li cloneDonor: host:port of an existing member of the PRIMARY cluster to
clone from. IPv6 addresses are not supported for this option.
@li manualStartOnBoot: boolean (default false). If false, Group Replication in
cluster instances will automatically start and rejoin when MySQL starts,
otherwise it must be started manually.
${CLUSTER_OPT_MEMBER_SSL_MODE}
${OPT_CERT_SUBJECT}
${CLUSTER_OPT_IP_ALLOWLIST}
${CLUSTER_OPT_LOCAL_ADDRESS}
${CLUSTER_OPT_EXIT_STATE_ACTION}
${CLUSTER_OPT_MEMBER_WEIGHT}
${CLUSTER_OPT_CONSISTENCY}
${CLUSTER_OPT_EXPEL_TIMEOUT}
${CLUSTER_OPT_AUTO_REJOIN_TRIES}
@li timeout: maximum number of seconds to wait for the instance to sync up
with the PRIMARY Cluster. Default is 0 and it means no timeout.
@li replicationAllowedHost: string value to use as the host name part of
internal replication accounts (i.e. 'mysql_innodb_cluster_###'@'hostname').
Default is %. It must be possible for any member of the Cluster to connect to
any other member using accounts with this hostname value.
${CLUSTER_OPT_COMM_STACK}
${CLUSTER_OPT_TRANSACTION_SIZE_LIMIT}
${CLUSTER_OPT_PAXOS_SINGLE_LEADER}
${CLUSTER_OPT_REPLICATION_OPTION_CONNECT_RETRY}
${CLUSTER_OPT_REPLICATION_OPTION_RETRY_COUNT}
${CLUSTER_OPT_REPLICATION_OPTION_HEARTBEAT_PERIOD}
${CLUSTER_OPT_REPLICATION_OPTION_COMPRESSION_ALGORITHMS}
${CLUSTER_OPT_REPLICATION_OPTION_ZSTD_COMPRESSION_LEVEL}
${CLUSTER_OPT_REPLICATION_OPTION_BIND}
${CLUSTER_OPT_REPLICATION_OPTION_NETWORK_NAMESPACE}

The recoveryMethod option supports the following values:

@li incremental: waits until the new instance has applied missing transactions
from the PRIMARY
@li clone: uses MySQL clone to provision the instance, which completely
replaces the state of the target instance with a full snapshot of another
ClusterSet member.
@li auto: compares the transaction set of the instance with that of the
PRIMARY to determine if incremental recovery is safe to be automatically
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

The cloneDonor option is used to override the automatic selection of a donor to
be used when clone is selected as the recovery method. By default, a SECONDARY
member will be chosen as donor. If no SECONDARY members are available the
PRIMARY will be selected. The option accepts values in the format: 'host:port'.
IPv6 addresses are not supported.

${CLUSTER_OPT_MEMBER_SSL_MODE_DETAIL}

${CLUSTER_OPT_IP_ALLOWLIST_EXTRA}

The groupName and localAddress are advanced options and their usage is
discouraged since incorrect values can lead to Group Replication errors.

The value for groupName is used to set the Group Replication system variable
'group_replication_group_name'.

${CLUSTER_OPT_LOCAL_ADDRESS_EXTRA}

${CLUSTER_OPT_EXIT_STATE_ACTION_DETAIL}

${CLUSTER_OPT_CONSISTENCY_DETAIL}

${CLUSTER_OPT_EXIT_STATE_ACTION_EXTRA}

${CLUSTER_OPT_MEMBER_WEIGHT_DETAIL_EXTRA}

${CLUSTER_OPT_CONSISTENCY_EXTRA}

${CLUSTER_OPT_EXPEL_TIMEOUT_EXTRA}

${CLUSTER_OPT_AUTO_REJOIN_TRIES_EXTRA}

${CLUSTER_OPT_COMM_STACK_EXTRA}

${CLUSTER_OPT_TRANSACTION_SIZE_LIMIT_EXTRA}

${CLUSTER_OPT_PAXOS_SINGLE_LEADER_EXTRA}
)*");
/**
 * $(CLUSTERSET_CREATEREPLICACLUSTER_BRIEF)
 *
 * $(CLUSTERSET_CREATEREPLICACLUSTER)
 */
#if DOXYGEN_JS
Cluster ClusterSet::createReplicaCluster(InstanceDef instance,
                                         String clusterName,
                                         Dictionary options);
#elif DOXYGEN_PY
Cluster ClusterSet::create_replica_cluster(InstanceDef instance,
                                           str clusterName, dict options);
#endif
shcore::Value ClusterSet::create_replica_cluster(
    const std::string &instance_def, const std::string &cluster_name,
    shcore::Option_pack_ref<clusterset::Create_replica_cluster_options>
        options) {
  assert_valid("createReplicaCluster");

  return execute_with_pool(
      [&]() {
        return impl()->create_replica_cluster(instance_def, cluster_name,
                                              options->get_recovery_progress(),
                                              *options);
      },
      false);
}

// Documentation of the removeCluster function
REGISTER_HELP_FUNCTION(removeCluster, ClusterSet);
REGISTER_HELP_FUNCTION_TEXT(CLUSTERSET_REMOVECLUSTER, R"*(
Removes a Replica cluster from a ClusterSet.

@param clusterName The name identifier of the Replica cluster to be removed.
@param options optional Dictionary with additional parameters described below.

@returns Nothing.

Removes a MySQL InnoDB Replica Cluster from the target ClusterSet.

The Cluster is removed from the ClusterSet and implicitly dissolved, i.e. each
member of it becomes a standalone instance.

For the Cluster to be successfully removed from the ClusterSet the PRIMARY
Cluster must be available and the ClusterSet replication channel healthy. If
those conditions aren't met the Cluster can still be forcefully removed
using the 'force' option, however, its Metadata won't be updated
compromising the effortless usage of its members to create new Clusters
and/or add to existing Clusters. To re-use those instances, the Metadata
schema must be dropped using dba.<<<dropMetadataSchema>>>(), or the Cluster
rebooted from complete outage using
dba.<<<rebootClusterFromCompleteOutage>>>().

<b>Options</b>

The options dictionary can contain the following values:

@li force: boolean, indicating if the cluster must be removed (even if
           only from metadata) in case the PRIMARY cannot be reached, or
           the ClusterSet replication channel cannot be found or is stopped.
           By default, set to false.
@li dissolve: whether to dissolve the cluster after removing it from the
              ClusterSet. Set to false if you intend to use it as a standalone
              cluster. Default is true.
@li timeout: maximum number of seconds to wait for the instance to sync up
             with the PRIMARY Cluster. Default is 0 and it means no timeout.
@li dryRun: boolean if true, all validations and steps for removing a
            the Cluster from the ClusterSet are executed, but no changes are
            actually made. An exception will be thrown when finished.

NOTE: if dissolve is set to false it will not be possible to add the removed
cluster back to the ClusterSet, because transactions that are not in the ClusterSet
will be executed there.
)*");

/**
 * $(CLUSTERSET_REMOVECLUSTER_BRIEF)
 *
 * $(CLUSTERSET_REMOVECLUSTER)
 */
#if DOXYGEN_JS
Undefined ClusterSet::removeCluster(String clusterName, Dictionary options);
#elif DOXYGEN_PY
None ClusterSet::remove_cluster(str cluster_name, dict options);
#endif
void ClusterSet::remove_cluster(
    const std::string &cluster_name,
    const shcore::Option_pack_ref<clusterset::Remove_cluster_options>
        &options) {
  assert_valid("removeCluster");

  return execute_with_pool(
      [&]() { impl()->remove_cluster(cluster_name, *options); }, false);
}

// Documentation of the status function
REGISTER_HELP_FUNCTION(status, ClusterSet);
REGISTER_HELP_FUNCTION_TEXT(CLUSTERSET_STATUS, R"*(
Describe the status of the ClusterSet.

@param options optional Dictionary with additional parameters described below.

@returns A JSON object describing the status of the ClusterSet and its members.

This function describes the status of the ClusterSet including its
members (Clusters).

The function will gather state information from each member of the
ClusterSet and the replication channel of it to produce a status
report of the ClusterSet as a whole.

<b>Options</b>

The following options may be given to control the amount of information
gathered and returned.

@li extended: verbosity level of the command output. Default is 0.

Option 'extended' may have the following values:

@li 0: regular level of details. Only basic information about the status
of the ClusterSet and Cluster members.
@li 1: includes basic information about the status of each cluster, information
about each cluster member role and state as reported by Group
Replication, and information about the ClusterSet Replication channel.
@li 2: includes the list of the fenced system variables, applier worker
threads, member ID, etc. The information about the ClusterSet
Replication channel is extended to include information about the applier
queue size, applier queue GTID set, coordinator state, etc.
@li 3: includes important replication related configuration settings, such
as replication delay, heartbeat delay, retry count and connection retry
for the ClusterSet replication channel.
)*");
/**
 * $(CLUSTERSET_STATUS_BRIEF)
 *
 * $(CLUSTERSET_STATUS)
 */
#if DOXYGEN_JS
String ClusterSet::status(Dictionary options);
#elif DOXYGEN_PY
str ClusterSet::status(dict options);
#endif
shcore::Value ClusterSet::status(
    const shcore::Option_pack_ref<clusterset::Status_options> &options) {
  assert_valid("status");

  return execute_with_pool(
      [&]() {
        impl()->connect_primary();
        return shcore::Value(impl()->status(options->extended));
      },
      false);
}

// Documentation of the describe function
REGISTER_HELP_FUNCTION(describe, ClusterSet);
REGISTER_HELP_FUNCTION_TEXT(CLUSTERSET_DESCRIBE, R"*(
Describe the structure of the ClusterSet.

@returns A JSON object describing the structure of the ClusterSet.

This function describes the status of the ClusterSet including its
members (Clusters).

This function describes the structure of the ClusterSet including all its
information and Clusters belonging to it.

The returned JSON object contains the following attributes:

@li domainName: The ClusterSet domain name
@li primaryCluster: The current primary Cluster of the ClusterSet
@li clusters: the list of members of the ClusterSet

The clusters JSON object contains the following attributes:

@li clusterRole: The role of the Cluster
@li a list of dictionaries describing each instance belonging to
the Cluster.

Each instance dictionary contains the following attributes:

@li address: the instance address in the form of host:port
@li label: the instance name identifier
)*");
/**
 * $(CLUSTERSET_DESCRIBE_BRIEF)
 *
 * $(CLUSTERSET_DESCRIBE)
 */
#if DOXYGEN_JS
String ClusterSet::describe();
#elif DOXYGEN_PY
str ClusterSet::describe();
#endif
shcore::Value ClusterSet::describe() {
  assert_valid("describe");

  return execute_with_pool(
      [&]() {
        impl()->connect_primary();
        return shcore::Value(impl()->describe());
      },
      false);
}

REGISTER_HELP_FUNCTION(dissolve, ClusterSet);
REGISTER_HELP_FUNCTION_TEXT(CLUSTERSET_DISSOLVE, R"*(
Dissolves the ClusterSet.

@param options Dictionary with options for the operation.
@returns Nothing

This function dissolves the ClusterSet by removing all Clusters that
belong to it, implicitly dissolving each one.

It keeps all the user's data intact.

<b>Options</b>

The options dictionary may contain the following attributes:

@li force: set to true to confirm that the dissolve operation must be
executed, even if some members of the ClusterSet cannot be reached or
the timeout was reached when waiting for members to catch up with replication
changes. By default, set to false.
@li timeout: maximum number of seconds to wait for pending transactions to
be applied in each reachable instance of the ClusterSet (default value is
retrieved from the 'dba.gtidWaitTimeout' shell option).
@li dryRun: if true, all validations and steps for dissolving the
ClusterSet are executed, but no changes are actually made.

The force option (set to true) must only be used to dissolve a ClusterSet
with Clusters that are permanently unavailable (no longer reachable), or if
any instance of any Cluster is also permanently unavailable.
)*");

/**
 * $(CLUSTERSET_DISSOLVE_BRIEF)
 *
 * $(CLUSTERSET_DISSOLVE)
 */
#if DOXYGEN_JS
Undefined ClusterSet::dissolve(Dictionary options) {}
#elif DOXYGEN_PY
None ClusterSet::dissolve(dict options) {}
#endif

void ClusterSet::dissolve(
    const shcore::Option_pack_ref<clusterset::Dissolve_options> &options) {
  assert_valid("dissolve");

  execute_with_pool([&]() { impl()->dissolve(*options); }, false);
}

REGISTER_HELP_FUNCTION(setPrimaryCluster, ClusterSet);
REGISTER_HELP_FUNCTION_TEXT(CLUSTERSET_SETPRIMARYCLUSTER, R"*(
Performs a safe switchover of the PRIMARY Cluster of the ClusterSet.

@param clusterName Name of the REPLICA cluster to be promoted.
@param options optional Dictionary with additional parameters described below.

@returns Nothing

This command will perform a safe switchover of the PRIMARY Cluster
of a ClusterSet. The current PRIMARY will be demoted to a REPLICA
Cluster, while the promoted Cluster will be made the PRIMARY Cluster.
All other REPLICA Clusters will be updated to replicate from the new
PRIMARY.

During the switchover, the promoted Cluster will be synchronized with
the old PRIMARY, ensuring that all transactions present in the PRIMARY
are applied before the topology change is committed. The current PRIMARY
instance is also locked with 'FLUSH TABLES WITH READ LOCK' in order to prevent
changes during the switch. If either of these operations take too long or fails,
the switch will be aborted.

For a switchover to be possible, all instances of the target
Cluster must be reachable from the shell and have consistent transaction
sets with the current PRIMARY Cluster. If the PRIMARY Cluster is not
available and cannot be restored, a failover must be performed instead, using
ClusterSet.<<<forcePrimaryCluster>>>().

The switchover will be canceled if there are REPLICA Clusters that
are unreachable or unavailable. To continue, they must either be restored or
invalidated by including their name in the 'invalidateReplicaClusters' option.
Invalidated REPLICA Clusters must be either removed from the Cluster or restored
and rejoined, using <<<removeCluster>>>() or <<<rejoinCluster>>>().

Additionally, if any available REPLICA Cluster has members that are not ONLINE
and/or reachable, these members will not be in a properly configured state even
after being restored and rejoined. To ensure failover works correctly,
<<<rejoinCluster>>>() must be called on the Cluster once these members are
rejoined.

<b>Options</b>

The following options may be given:

@li dryRun: if true, will perform checks and log operations that would be
performed, but will not execute them. The operations that would be
performed can be viewed by enabling verbose output in the shell.
@li timeout: integer value to set the maximum number of seconds to wait
for the synchronization of the Cluster and also for the instance
being promoted to catch up with the current PRIMARY (in the case of the latter,
the default value is retrieved from the 'dba.gtidWaitTimeout' shell option).
@li invalidateReplicaClusters: list of names of REPLICA Clusters that are
unreachable or unavailable that are to be invalidated during the switchover.
)*");
/**
 * $(CLUSTERSET_SETPRIMARYCLUSTER_BRIEF)
 *
 * $(CLUSTERSET_SETPRIMARYCLUSTER)
 */
#if DOXYGEN_JS
Undefined ClusterSet::setPrimaryCluster(String clusterName, Dictionary options);
#elif DOXYGEN_PY
None ClusterSet::set_primary_cluster(str clusterName, dict options);
#endif
void ClusterSet::set_primary_cluster(
    const std::string &cluster_name,
    const shcore::Option_pack_ref<clusterset::Set_primary_cluster_options>
        &options) {
  assert_valid("setPrimaryCluster");

  return execute_with_pool(
      [&]() { impl()->set_primary_cluster(cluster_name, *options); }, false);
}

REGISTER_HELP_FUNCTION(forcePrimaryCluster, ClusterSet);
REGISTER_HELP_FUNCTION_TEXT(CLUSTERSET_FORCEPRIMARYCLUSTER, R"*(
Performs a failover of the PRIMARY Cluster of the ClusterSet.

@param clusterName Name of the REPLICA cluster to be promoted.
@param options optional Dictionary with additional parameters described below.

@returns Nothing

This command will perform a failover of the PRIMARY Cluster
of a ClusterSet. The target cluster is promoted to the new PRIMARY Cluster
while the previous PRIMARY Cluster is invalidated. The previous PRIMARY Cluster
is presumed unavailable by the Shell, but if that is not the case, it is
recommended that instances of that Cluster are taken down to avoid or minimize
inconsistencies.

The failover will be canceled if there are REPLICA Clusters that
are unreachable or unavailable. To continue, they must either be restored or
invalidated by including their name in the 'invalidateReplicaClusters' option.

Additionally, if any available REPLICA Cluster has members that are not ONLINE
and/or reachable, these members will not be in a properly configured state even
after being restored and rejoined. To ensure failover works correctly,
<<<rejoinCluster>>>() must be called on the Cluster once these members are
rejoined.

Note that because a failover may result in loss of transactions, it is always
preferrable that the PRIMARY Cluster is restored.

<b>Aftermath of a Failover</b>

If a failover is the only viable option to recover from an outage, the following
must be considered:

@li The topology of the ClusterSet, including a record of "who is the primary",
is stored in the ClusterSet itself. In a failover, the metadata is updated to
reflect the new topology and the invalidation of the old primary.
However, if the invalidated primary is still ONLINE somewhere, the copy of the
metadata held there will will remain outdated and inconsistent with the actual
topology. MySQL Router instances that can connect to the new PRIMARY Cluster
will be able to tell which topology is the correct one, but those instances
that can only connect to the invalid Cluster will behave as if nothing changed.
If applications can still update the database through such Router instances,
there will be a "Split-Brain" and the database will become inconsistent. To
avoid such scenario, fence the old primary from all traffic using
Cluster.<<<fenceAllTraffic>>>(), or from write traffic only using
Cluster.<<<fenceWrites>>>().

@li An invalidated PRIMARY Cluster that is later restored can only be rejoined
if its GTID set has not diverged relative to the rest of the ClusterSet.

@li A diverged invalidated Cluster can only be removed from the ClusterSet.
Recovery and reconciliation of transactions that only exist in that Cluster
can only be done manually.

@li Because regular Asynchronous Replication is used between PRIMARY and
REPLICA Clusters, any transactions at the PRIMARY that were not yet replicated
at the time of the failover will be lost. Even if the original PRIMARY Cluster
is restored at some point, these transactions would have to be recovered
and reconciled manually.

Thus, the recommended course of action in event of an outage is to always
restore the PRIMARY Cluster if at all possible, even if a failover may be faster
and easier in the short term.

<b>Options</b>

The following options may be given:

@li dryRun: if true, will perform checks and log operations that would be
performed, but will not execute them. The operations that would be
performed can be viewed by enabling verbose output in the shell.
@li invalidateReplicaClusters: list of names of REPLICA Clusters that are
unreachable or unavailable that are to be invalidated during the failover.
@li timeout: integer value with the maximum number of seconds to wait for
pending transactions to be applied in each instance of the cluster (default
value is retrieved from the 'dba.gtidWaitTimeout' shell option).
)*");
/**
 * $(CLUSTERSET_FORCEPRIMARYCLUSTER_BRIEF)
 *
 * $(CLUSTERSET_FORCEPRIMARYCLUSTER)
 */
#if DOXYGEN_JS
Undefined ClusterSet::forcePrimaryCluster(String clusterName,
                                          Dictionary options);
#elif DOXYGEN_PY
None ClusterSet::force_primary_cluster(str clusterName, dict options);
#endif
void ClusterSet::force_primary_cluster(
    const std::string &cluster_name,
    const shcore::Option_pack_ref<clusterset::Force_primary_cluster_options>
        &options) {
  assert_valid("forcePrimaryCluster");

  return execute_with_pool(
      [&]() { impl()->force_primary_cluster(cluster_name, *options); }, false);
}

REGISTER_HELP_FUNCTION(rejoinCluster, ClusterSet);
REGISTER_HELP_FUNCTION_TEXT(CLUSTERSET_REJOINCLUSTER, R"*(
Rejoin an invalidated Cluster back to the ClusterSet and update replication.

@param clusterName Name of the Cluster to be rejoined.
@param options optional Dictionary with additional parameters described below.

@returns Nothing

Rejoins a Cluster that was invalidated as part of a failover or switchover,
if possible. This can also be used to update replication channel in REPLICA
Clusters, if it does not have the expected state, source and settings.

The PRIMARY Cluster of the ClusterSet must be reachable and
available during the operation.

<b>Pre-Requisites</b>

The following pre-requisites are expected for Clusters rejoined to a
ClusterSet. They will be automatically checked by rejoinCluster(), which
will stop if any issues are found.

@li The target Cluster must belong to the ClusterSet metadata and be reachable.
@li The target Cluster must not be an active member of the ClusterSet.
@li The target Cluster must not be holding any Metadata or InnoDB
  transaction lock.
@li The target Cluster's transaction set must not contain transactions
  that don't exist in the PRIMARY Cluster.
@li The target Cluster's transaction set must not be missing transactions
  that have been purged from the PRIMARY Cluster.
@li The target Cluster's executed transaction set (GTID_EXECUTED) must not
  be empty.

<b>Options</b>

The following options may be given:

@li dryRun: if true, will perform checks and log operations that would be
  performed, but will not execute them. The operations that would be
  performed can be viewed by enabling verbose output in the shell.
)*");
/**
 * $(CLUSTERSET_REJOINCLUSTER_BRIEF)
 *
 * $(CLUSTERSET_REJOINCLUSTER)
 */
#if DOXYGEN_JS
Undefined ClusterSet::rejoinCluster(String clusterName, Dictionary options);
#elif DOXYGEN_PY
None ClusterSet::rejoin_cluster(str clusterName, dict options);
#endif
void ClusterSet::rejoin_cluster(
    const std::string &cluster_name,
    const shcore::Option_pack_ref<clusterset::Rejoin_cluster_options>
        &options) {
  assert_valid("rejoinCluster");

  return execute_with_pool(
      [&]() { impl()->rejoin_cluster(cluster_name, *options); }, false);
}

REGISTER_HELP_FUNCTION(options, ClusterSet);
REGISTER_HELP_FUNCTION_TEXT(CLUSTERSET_OPTIONS, R"*(
Lists the ClusterSet configuration options.

@returns A JSON object describing the configuration options of the ClusterSet.

This function lists the configuration options for the ClusterSet.
)*");

/**
 * $(CLUSTERSET_OPTIONS_BRIEF)
 *
 * $(CLUSTERSET_OPTIONS)
 */
#if DOXYGEN_JS
String ClusterSet::options() {}
#elif DOXYGEN_PY
str ClusterSet::options() {}
#endif

shcore::Value ClusterSet::options() {
  // Throw an error if the clusterset is invalid
  assert_valid("options");

  return execute_with_pool([&]() { return impl()->options(); }, false);
}

REGISTER_HELP_FUNCTION(setOption, ClusterSet);
REGISTER_HELP_FUNCTION_TEXT(CLUSTERSET_SETOPTION, R"*(
Changes the value of an option for the whole ClusterSet.

@param option The option to be changed.
@param value The value that the option shall get.

@returns Nothing.

This function changes an option for the ClusterSet.

The accepted options are:
@li replicationAllowedHost string value to use as the host name part of
internal replication accounts. Existing accounts will be re-created with the new
value.

)*");

/**
 * $(CLUSTERSET_SETOPTION_BRIEF)
 *
 * $(CLUSTERSET_SETOPTION)
 */
#if DOXYGEN_JS
Undefined ClusterSet::setOption(String option, String value) {}
#elif DOXYGEN_PY
None ClusterSet::set_option(str option, str value) {}
#endif

void ClusterSet::set_option(const std::string &option,
                            const shcore::Value &value) {
  assert_valid("setOption");

  return execute_with_pool([&]() { impl()->set_option(option, value); }, false);
}

REGISTER_HELP_FUNCTION(listRouters, ClusterSet);
REGISTER_HELP_FUNCTION_TEXT(CLUSTERSET_LISTROUTERS, R"*(
Lists the Router instances of the ClusterSet, or a single Router instance.

@param router optional identifier of the target router instance (e.g. 192.168.45.70@::system)

@returns A JSON object listing the Router instances registered in the ClusterSet.

This function lists and provides information about all Router instances
registered on the Clusters members of the ClusterSet.

For each router, the following information is provided, when available:

@li hostname: Hostname.
@li lastCheckIn: Timestamp of the last statistics update (check-in).
@li roPort: Read-only port (Classic protocol).
@li roXPort: Read-only port (X protocol).
@li rwPort: Read-write port (Classic protocol).
@li rwSplitPort: Read-write split port (Classic protocol).
@li rwXPort: Read-write port (X protocol).
@li targetCluster: Target Cluster for Router routing operations.
@li version: Version.
)*");

/**
 * $(CLUSTERSET_LISTROUTERS_BRIEF)
 *
 * $(CLUSTERSET_LISTROUTERS)
 */
#if DOXYGEN_JS
Dictionary ClusterSet::listRouters(String router) {}
#elif DOXYGEN_PY
dict ClusterSet::list_routers(str router) {}
#endif
shcore::Value ClusterSet::list_routers(const std::string &router) {
  return execute_with_pool([&]() { return impl()->list_routers(router); },
                           false);
}

REGISTER_HELP_FUNCTION(setRoutingOption, ClusterSet);
REGISTER_HELP_FUNCTION_TEXT(CLUSTERSET_SETROUTINGOPTION, R"*(
Changes the value of either a global Routing option or of a single Router
instance.

@param router optional identifier of the target router instance (e.g.
192.168.45.70@::system).
@param option The Router option to be changed.
@param value The value that the option shall get (or null to unset).

@returns Nothing.

The accepted options are:

@li target_cluster: Target Cluster for Router routing operations. Default value
is 'primary'.
@li invalidated_cluster_policy: Routing policy to be taken when the target
cluster is detected as being invalidated. Default value is 'drop_all'.
@li stats_updates_frequency: Number of seconds between updates that the Router
is to make to its statistics in the InnoDB Cluster metadata.
@li use_replica_primary_as_rw: Enable/Disable the RW Port in Replica Clusters.
Disabled by default.
@li tags: Associates an arbitrary JSON object with custom key/value pairs with
the ClusterSet metadata.
@li read_only_targets: Routing policy to define Router's usage of Read
Replicas. Default is 'append'.
@li guideline: Name of the Routing Guideline to be set as active
in the ClusterSet.

The target_cluster option supports the following values:

@li primary: Follow the Primary Cluster whenever it changes in runtime (
default).
@li @<clusterName@>: Use the Cluster named @<clusterName@> as target.

The invalidated_cluster_policy option supports the following values:

@li accept_ro: all the RW connections are be dropped and no new RW connections
are be accepted. RO connections keep being accepted and handled.
@li drop_all: all connections to the target Cluster are closed and no new
connections will be accepted (default).

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

The use_replica_primary_as_rw option accepts a boolean value to configure
whether the Router should enable or disable the RW Port for the target Cluster.

When enabled, forces the RW port of Routers targeting a specific Cluster
(target_cluster != 'primary') to always route to the PRIMARY of that Cluster,
even when it is in a REPLICA cluster and thus, read-only. By default, the
option is false and Router blocks connections to the RW port in this
scenario.

The read_only_targets option supports the following values:

@li all: All Read Replicas of the target Cluster should be used along the
other SECONDARY Cluster members for R/O traffic.
@li read_replicas: Only Read Replicas of the target Cluster should be used for
R/O traffic.
@li secondaries: Only Secondary members of the target Cluster should be used
for R/O traffic (default).

@attention When the ClusterSet has an active Routing Guideline, the options
'read_only_targets' and 'target_cluster' are ignored since the Guideline has
precedence over them.
)*");

#if DOXYGEN_JS
Undefined ClusterSet::setRoutingOption(String option, String value) {}
#elif DOXYGEN_PY
None ClusterSet::set_routing_option(str option, str value) {}
#endif

/**
 * $(CLUSTERSET_SETROUTINGOPTION_BRIEF)
 *
 * $(CLUSTERSET_SETROUTINGOPTION)
 */
#if DOXYGEN_JS
Undefined ClusterSet::setRoutingOption(String router, String option,
                                       String value) {}
#elif DOXYGEN_PY
None ClusterSet::set_routing_option(str router, str option, str value) {}
#endif

REGISTER_HELP_FUNCTION(routingOptions, ClusterSet);
REGISTER_HELP_FUNCTION_TEXT(CLUSTERSET_ROUTINGOPTIONS,
                            ROUTINGOPTIONS_HELP_TEXT);

/**
 * $(CLUSTERSET_ROUTINGOPTIONS_BRIEF)
 *
 * $(CLUSTERSET_ROUTINGOPTIONS)
 */
#if DOXYGEN_JS
Dictionary ClusterSet::routingOptions(String router) {}
#elif DOXYGEN_PY
dict ClusterSet::routing_options(str router) {}
#endif

REGISTER_HELP_FUNCTION(routerOptions, ClusterSet);
REGISTER_HELP_FUNCTION_TEXT(CLUSTERSET_ROUTEROPTIONS, R"*(
Lists the configuration options of the ClusterSet's Routers.

@param options Dictionary with options for the operation.

@returns A JSON object with the list of Router configuration options.

This function lists the Router configuration options of the ClusterSet (global),
and the Router instances. By default, only the options that can be changed from
Shell (dynamic options) are displayed.
Router instances with different configurations than the global ones will
include the differences under their dedicated description.

The options dictionary may contain the following attributes:

@li extended: Verbosity level of the command output.
@li router: Identifier of the Router instance to be displayed.

The extended option supports Integer or Boolean values:

@li 0: Includes only options that can be changed from Shell (default);
@li 1: Includes all ClusterSet global options and, per Router, only the options
that have a different value than the corresponding global one.
@li 2: Includes all Cluster and Router options.
@li Boolean: equivalent to assign either 0 (false) or 1 (true).
)*");

/**
 * $(CLUSTERSET_ROUTEROPTIONS_BRIEF)
 *
 * $(CLUSTERSET_ROUTEROPTIONS)
 */

REGISTER_HELP_FUNCTION(setupAdminAccount, ClusterSet);
REGISTER_HELP_FUNCTION_TEXT(CLUSTERSET_SETUPADMINACCOUNT,
                            SETUPADMINACCOUNT_HELP_TEXT);

/**
 * $(CLUSTERSET_SETUPADMINACCOUNT_BRIEF)
 *
 * $(CLUSTERSET_SETUPADMINACCOUNT)
 */
#if DOXYGEN_JS
Undefined ClusterSet::setupAdminAccount(String user, Dictionary options) {}
#elif DOXYGEN_PY
None ClusterSet::setup_admin_account(str user, dict options) {}
#endif

REGISTER_HELP_FUNCTION(setupRouterAccount, ClusterSet);
REGISTER_HELP_FUNCTION_TEXT(CLUSTERSET_SETUPROUTERACCOUNT,
                            SETUPROUTERACCOUNT_HELP_TEXT);

/**
 * $(CLUSTERSET_SETUPROUTERACCOUNT_BRIEF)
 *
 * $(CLUSTERSET_SETUPROUTERACCOUNT)
 */
#if DOXYGEN_JS
Undefined ClusterSet::setupRouterAccount(String user, Dictionary options) {}
#elif DOXYGEN_PY
None ClusterSet::setup_router_account(str user, dict options) {}
#endif

REGISTER_HELP_FUNCTION(execute, ClusterSet);
REGISTER_HELP_FUNCTION_TEXT(CLUSTERSET_EXECUTE, R"*(
Executes a SQL statement at selected instances of the ClusterSet.

@param cmd The SQL statement to execute.
@param instances The instances where cmd should be executed.
@param options Dictionary with options for the operation.

@returns A JSON object with a list of results / information regarding the
executing of the SQL statement on each of the target instances.

This function allows a single MySQL SQL statement to be executed on
multiple instances of the ClusterSet.

The 'instances' parameter can be either a string (keyword) or a list of
instance addresses where cmd should be executed. If a string, the allowed
keywords are:
@li "all" / "a": all reachable instances.
@li "primary" / "p": the primary instance of the primary cluster.
@li "secondaries" / "s": the secondary instances on all clusters.
@li "read-replicas" / "rr": the read-replicas instances on all clusters.

The options dictionary may contain the following attributes:

@li exclude: similar to the instances parameter, it can be either a string
(keyword) or a list of instance addresses to exclude from the instances
specified in instances. It accepts the same keywords, except "all".
@li timeout: integer value with the maximum number of seconds to wait for
cmd to execute in each target instance. Default value is 0 meaning it
doesn't timeout.
@li dryRun: boolean if true, all validations and steps for executing cmd
are performed, but no cmd is actually executed on any instance.

To calculate the final list of instances where cmd should be executed,
the function starts by parsing the instances parameter and then subtract
from that list the ones specified in the exclude option. For example, if
instances is "all" and exclude is "secondaries", then all primaries (on the
primary and replica clusters) and all read-replicas are targeted.
)*");

/**
 * $(CLUSTERSET_EXECUTE_BRIEF)
 *
 * $(CLUSTERSET_EXECUTE)
 */
#if DOXYGEN_JS
Dictionary ClusterSet::execute(String cmd, Object instances,
                               Dictionary options) {}
#elif DOXYGEN_PY
dict ClusterSet::execute(str cmd, Object instances, dict options);
#endif

REGISTER_HELP_FUNCTION(createRoutingGuideline, ClusterSet);
REGISTER_HELP_FUNCTION_TEXT(CLUSTERSET_CREATEROUTINGGUIDELINE,
                            CREATEROUTINGGUIDELINE_HELP_TEXT);
/**
 * $(CLUSTERSET_CREATEROUTINGGUIDELINE_BRIEF)
 *
 * $(CLUSTERSET_CREATEROUTINGGUIDELINE)
 */
#if DOXYGEN_JS
RoutingGuideline createRoutingGuideline(String name, Dictionary json,
                                        Dictionary options) {}
#elif DOXYGEN_PY
RoutingGuideline create_routing_guideline(str name, dict json, dict options) {}
#endif

REGISTER_HELP_FUNCTION(getRoutingGuideline, ClusterSet);
REGISTER_HELP_FUNCTION_TEXT(CLUSTERSET_GETROUTINGGUIDELINE,
                            GETROUTINGGUIDELINE_HELP_TEXT);
/**
 * $(CLUSTERSET_GETROUTINGGUIDELINE_BRIEF)
 *
 * $(CLUSTERSET_GETROUTINGGUIDELINE)
 */
#if DOXYGEN_JS
RoutingGuideline getRoutingGuideline(String name) {}
#elif DOXYGEN_PY
RoutingGuideline get_routing_guideline(str name) {}
#endif

REGISTER_HELP_FUNCTION(removeRoutingGuideline, ClusterSet);
REGISTER_HELP_FUNCTION_TEXT(CLUSTERSET_REMOVEROUTINGGUIDELINE,
                            REMOVEROUTINGGUIDELINE_HELP_TEXT);
/**
 * $(CLUSTERSET_REMOVEROUTINGGUIDELINE_BRIEF)
 *
 * $(CLUSTERSET_REMOVEROUTINGGUIDELINE)
 */
#if DOXYGEN_JS
Undefined removeRoutingGuideline(String name) {}
#elif DOXYGEN_PY
None remove_routing_guideline(str name) {}
#endif

REGISTER_HELP_FUNCTION(routingGuidelines, ClusterSet);
REGISTER_HELP_FUNCTION_TEXT(CLUSTERSET_ROUTINGGUIDELINES,
                            ROUTINGGUIDELINES_HELP_TEXT);
/**
 * $(CLUSTERSET_ROUTINGGUIDELINES_BRIEF)
 *
 * $(CLUSTERSET_ROUTINGGUIDELINES)
 */
#if DOXYGEN_JS
List routingGuidelines() {}
#elif DOXYGEN_PY
list routing_guidelines() {}
#endif

REGISTER_HELP_FUNCTION(importRoutingGuideline, ClusterSet);
REGISTER_HELP_FUNCTION_TEXT(CLUSTERSET_IMPORTROUTINGGUIDELINE,
                            IMPORTROUTINGGUIDELINE_HELP_TEXT);
/**
 * $(CLUSTERSET_IMPORTROUTINGGUIDELINE_BRIEF)
 *
 * $(CLUSTERSET_IMPORTROUTINGGUIDELINE)
 */
#if DOXYGEN_JS
RoutingGuideline ClusterSet::importRoutingGuideline(String file,
                                                    Dictionary options) {}
#elif DOXYGEN_PY
RoutingGuideline ClusterSet::import_routing_guideline(str file, dict options) {}
#endif

}  // namespace dba
}  // namespace mysqlsh
