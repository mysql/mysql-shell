/*
 * Copyright (c) 2020, 2021, Oracle and/or its affiliates.
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

#include "modules/adminapi/mod_dba_cluster_set.h"
#include "modules/adminapi/mod_dba_cluster.h"
#include "modules/mod_utils.h"
#include "mysqlshdk/include/scripting/type_info/custom.h"
#include "mysqlshdk/include/scripting/type_info/generic.h"
#include "mysqlshdk/include/shellcore/utils_help.h"
#include "mysqlshdk/libs/utils/debug.h"
#include "mysqlshdk/libs/utils/utils_json.h"

DEBUG_OBJ_ENABLE(ClusterSet);

namespace mysqlsh {
namespace dba {

// Documentation of the ClusterSet Class
REGISTER_HELP_CLASS(ClusterSet, adminapi);
REGISTER_HELP_CLASS_TEXT(CLUSTERSET, R"*(
Represents an InnoDB ClusterSet.

The clusterset object is the entry point to manage and monitor a MySQL
InnoDB ClusterSet.

ClusterSets allow InnoDB Cluster deployments to achieve fault-tolerance at a
whole Data Center / region or geographic location, by creating REPLICA clusters
in different locations (Data Centers), ensuring Disaster Recovery is possible.

For more help on a specific function use: clusterset.help('<functionName>')

e.g. clusterset.help('createReplicaCluster')
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

std::string &ClusterSet::append_descr(std::string &s_out, int UNUSED(indent),
                                      int UNUSED(quote_strings)) const {
  s_out.append("<" + class_name() + ":" + impl()->get_name() + ">");

  return s_out;
}

void ClusterSet::append_json(shcore::JSON_dumper &dumper) const {
  dumper.start_object();
  dumper.append_string("class", class_name());
  dumper.append_string("name", impl()->get_name());
  dumper.end_object();
}

bool ClusterSet::operator==(const Object_bridge &other) const {
  return class_name() == other.class_name() && this == &other;
}

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
         "?clusterName", "?options")
      ->cli();
  expose("status", &ClusterSet::status, "?options")->cli();
  expose("describe", &ClusterSet::describe)->cli();
}

void ClusterSet::assert_valid(const std::string &function_name) {
  std::string name;

  if (function_name == "disconnect") return;

  if (!m_impl->get_cluster_server()) {
    throw shcore::Exception::runtime_error(
        "The ClusterSet object is disconnected. Please use "
        "dba.<<<getClusterSet>>>() to obtain a fresh handle.");
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

shcore::Value ClusterSet::get_member(const std::string &prop) const {
  shcore::Value ret_val;

  if (prop == "name") {
    ret_val = shcore::Value(impl()->get_name());
  } else {
    ret_val = shcore::Cpp_object_bridge::get_member(prop);
  }

  return ret_val;
}

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

For the detailed list of requirements to create an InnoDB Cluster, please use
\? createCluster

<b>Options</b>

The options dictionary can contain the following values:

@li interactive: boolean value used to disable/enable the wizards in the
command execution, i.e. prompts and confirmations will be provided or
not according to the value set. The default value is equal to MySQL
Shell wizard mode.
@li dryRun: boolean if true, all validations and steps for creating a
Replica Cluster are executed, but no changes are actually made. An
exception will be thrown when finished.
@li recoveryMethod: Preferred method for state recovery/provisioning. May be
auto, clone or incremental. Default is auto.
@li recoveryProgress: Integer value to indicate the recovery process verbosity
level.
@li cloneDonor: host:port of an existing member of the PRIMARY cluster to
clone from. IPv6 addresses are not supported for this option.
@li memberSslMode: SSL mode used to configure the security state of the
communication between the InnoDB Cluster members.
@li ipAllowlist: The list of hosts allowed to connect to the instance for
Group Replication.
@li localAddress: string value with the Group Replication local address to
be used instead of the automatically generated one.
@li exitStateAction: string value indicating the Group Replication exit
state action.
@li memberWeight: integer value with a percentage weight for automatic
primary election on failover.
@li consistency: string value indicating the consistency guarantees
that the cluster provides.
@li expelTimeout: integer value to define the time period in seconds that
Cluster members should wait for a non-responding member before evicting
it from the Cluster.
@li autoRejoinTries: integer value to define the number of times an
instance will attempt to rejoin the Cluster after being expelled.
@li timeout: maximum number of seconds to wait for the instance to sync up
with the PRIMARY Cluster. Default is 0 and it means no timeout.

The recoveryMethod option supports the following values:

@li incremental: waits until the new instance has applied missing transactions
from the PRIMARY
@li clone: uses MySQL clone to provision the instance, which completely
replaces the state of the target instance with a full snapshot of another
ReplicaSet member.
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

The memberSslMode option supports the following values:

@li REQUIRED: if used, SSL (encryption) will be enabled for the instances to
communicate with other members of the Cluster
@li VERIFY_CA: Like REQUIRED, but additionally verify the server TLS
certificate against the configured Certificate Authority (CA) certificates.
@li VERIFY_IDENTITY: Like VERIFY_CA, but additionally verify that the server
certificate matches the host to which the connection is attempted.
@li DISABLED: if used, SSL (encryption) will be disabled
@li AUTO: if used, SSL (encryption) will be enabled if supported by the
instance, otherwise disabled

If memberSslMode is not specified AUTO will be used by default.

${CLUSTER_OPT_IP_ALLOWLIST_EXTRA}

The groupName, localAddress, and groupSeeds are advanced options and their
usage is discouraged since incorrect values can lead to Group Replication
errors.

The value for groupName is used to set the Group Replication system variable
'group_replication_group_name'.

The value for localAddress is used to set the Group Replication system variable
'group_replication_local_address'. The localAddress option accepts values in
the format: 'host:port' or 'host:' or ':port'. If the specified value does not
include a colon (:) and it is numeric, then it is assumed to be the port,
otherwise it is considered to be the host. When the host is not specified, the
default value is the value of the system variable 'report_host' if defined
(i.e., not 'NULL'), otherwise it is the hostname value. When the port is not
specified, the default value is the port of the current active connection
(session) * 10 + 1. In case the automatically determined default port value is
invalid (> 65535) then an error is thrown.

The value for groupSeeds is used to set the Group Replication system variable
'group_replication_group_seeds'. The groupSeeds option accepts a
comma-separated list of addresses in the format: 'host1:port1,...,hostN:portN'.

${CLUSTER_OPT_EXIT_STATE_ACTION_DETAIL}

${CLUSTER_OPT_CONSISTENCY_DETAIL}

${CLUSTER_OPT_EXIT_STATE_ACTION_EXTRA}

${CLUSTER_OPT_MEMBER_WEIGHT_DETAIL_EXTRA}

${CLUSTER_OPT_CONSISTENCY_EXTRA}

${CLUSTER_OPT_EXPEL_TIMEOUT_EXTRA}

${CLUSTER_OPT_AUTO_REJOIN_TRIES_EXTRA}
)*");
/**
 * $(CLUSTERSET_CREATEREPLICACLUSTER_BRIEF)
 *
 * $(CLUSTERSET_CREATEREPLICACLUSTER)
 */
#if DOXYGEN_JS
Cluster create_replica_cluster(InstanceDef instance, String clusterName,
                               dict options);
#elif DOXYGEN_PY
Cluster create_replica_cluster(InstanceDef instance, String clusterName,
                               dict options);
#endif
shcore::Value ClusterSet::create_replica_cluster(
    const std::string &instance_def, const std::string &cluster_name,
    const shcore::Option_pack_ref<clusterset::Create_replica_cluster_options>
        &options) {
  assert_valid("createReplicaCluster");

  bool interactive = current_shell_options()->get().wizards;

  Scoped_instance_pool scoped_pool(impl()->get_metadata_storage(), interactive,
                                   impl()->default_admin_credentials());

  // Init progress_style
  Recovery_progress_style progress_style = Recovery_progress_style::TEXTUAL;

  if (options->recovery_verbosity == 0) {
    progress_style = Recovery_progress_style::NOINFO;
  } else if (options->recovery_verbosity == 1) {
    progress_style = Recovery_progress_style::TEXTUAL;
  } else if (options->recovery_verbosity == 2) {
    progress_style = Recovery_progress_style::PROGRESSBAR;
  }

  return impl()->create_replica_cluster(instance_def, cluster_name,
                                        progress_style, *options);
}

// Documentation of the removeCluster function
REGISTER_HELP_FUNCTION(removeCluster, ClusterSet);
REGISTER_HELP_FUNCTION_TEXT(CLUSTERSET_REMOVECLUSTER, R"*(
Removes a Replica cluster from a ClusterSet.

@param clusterName The name identifier of the Replica cluster to be removed.
@param options optional Dictionary with additional parameters described below.

@returns Nothing.

Removes a MySQL InnoDB Replica Cluster from the target ClusterSet.

The ClusterSet topology is updated, i.e. the Cluster no longer belongs to the
ClusterSet, however, the Cluster remains unchanged.

<b>Options</b>

The options dictionary can contain the following values:

@li force: boolean, indicating if the cluster must be removed (even if
           only from metadata) in case the PRIMARY cannot be reached. By
           default, set to false.
@li timeout: maximum number of seconds to wait for the instance to sync up
             with the PRIMARY Cluster. Default is 0 and it means no timeout.
@li dryRun: boolean if true, all validations and steps for removing a
            the Cluster from the ClusterSet are executed, but no changes are
            actually made. An exception will be thrown when finished.
)*");
/**
 * $(CLUSTERSET_REMOVECLUSTER_BRIEF)
 *
 * $(CLUSTERSET_REMOVECLUSTER)
 */
#if DOXYGEN_JS
Undefined removeCluster(String clusterName, Dictionary options);
#elif DOXYGEN_PY
None remove_cluster(str cluster_name, dict options);
#endif
void ClusterSet::remove_cluster(
    const std::string &cluster_name,
    const shcore::Option_pack_ref<clusterset::Remove_cluster_options>
        &options) {
  assert_valid("removeCluster");

  bool interactive = current_shell_options()->get().wizards;

  Scoped_instance_pool scoped_pool(impl()->get_metadata_storage(), interactive,
                                   impl()->default_admin_credentials());

  impl()->remove_cluster(cluster_name, *options);
}

}  // namespace dba
}  // namespace mysqlsh
