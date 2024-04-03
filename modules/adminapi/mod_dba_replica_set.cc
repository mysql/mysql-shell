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

#include "modules/adminapi/mod_dba_replica_set.h"

#include <tuple>

#include "mysqlshdk/include/shellcore/utils_help.h"
#include "mysqlshdk/libs/utils/debug.h"

DEBUG_OBJ_ENABLE(ReplicaSet);

namespace mysqlsh {
namespace dba {

// Documentation of the ReplicaSet Class
REGISTER_HELP_CLASS_KW(
    ReplicaSet, adminapi,
    (std::map<std::string, std::string>({{"FullType", "InnoDB ReplicaSet"},
                                         {"Type", "ReplicaSet"},
                                         {"type", "replicaset"}})));
REGISTER_HELP(REPLICASET_BRIEF, "Represents an InnoDB ReplicaSet.");
REGISTER_HELP(
    REPLICASET_DETAIL,
    "The ReplicaSet object is used to manage MySQL server topologies that use "
    "asynchronous replication. It can be created using the "
    "dba.<<<createReplicaSet>>>() or dba.<<<getReplicaSet>>>() functions.");
REGISTER_HELP(REPLICASET_CLOSING,
              "For more help on a specific function, use the \\help shell "
              "command, e.g.: \\help ReplicaSet.<<<addInstance>>>");
ReplicaSet::ReplicaSet(const std::shared_ptr<Replica_set_impl> &cluster)
    : m_impl(cluster) {
  DEBUG_OBJ_ALLOC2(ReplicaSet, [](void *ptr) -> std::string {
    return "refs:" + std::to_string(reinterpret_cast<ReplicaSet *>(ptr)
                                        ->shared_from_this()
                                        .use_count());
  });

  init();
}

ReplicaSet::~ReplicaSet() { DEBUG_OBJ_DEALLOC(ReplicaSet); }

void ReplicaSet::init() {
  add_property("name", "getName");

  expose("addInstance", &ReplicaSet::add_instance, "instance", "?options")
      ->cli();
  expose("rejoinInstance", &ReplicaSet::rejoin_instance, "instance", "?options")
      ->cli();
  expose("removeInstance", &ReplicaSet::remove_instance, "instance", "?options")
      ->cli();

  expose("describe", &ReplicaSet::describe)->cli();
  expose("status", &ReplicaSet::status, "?options")->cli();
  expose("disconnect", &ReplicaSet::disconnect)->cli(false);

  expose("setPrimaryInstance", &ReplicaSet::set_primary_instance, "instance",
         "?options")
      ->cli();
  expose("forcePrimaryInstance", &ReplicaSet::force_primary_instance,
         "?instance", "?options")
      ->cli();

  expose("dissolve", &ReplicaSet::dissolve, "?options")->cli();

  expose("rescan", &ReplicaSet::rescan, "?options")->cli();

  expose("removeRouterMetadata", &ReplicaSet::remove_router_metadata,
         "routerDef")
      ->cli();
  expose("options", &ReplicaSet::options)->cli();
  expose("setOption", &ReplicaSet::set_option, "option", "value")->cli();
  expose("setInstanceOption", &ReplicaSet::set_instance_option, "instance",
         "option", "value")
      ->cli();

  expose("listRouters", &ReplicaSet::list_routers, "?options")->cli();
  expose("setupAdminAccount", &ReplicaSet::setup_admin_account, "user",
         "?options")
      ->cli();
  expose("setupRouterAccount", &ReplicaSet::setup_router_account, "user",
         "?options")
      ->cli();
  expose("routingOptions", &ReplicaSet::routing_options, "?router")->cli();
  expose("routerOptions", &ReplicaSet::router_options, "?options")->cli();
  expose("setRoutingOption", &ReplicaSet::set_routing_option, "option",
         "value");
  expose("setRoutingOption", &ReplicaSet::set_routing_option, "router",
         "option", "value");
}

// Documentation of the getName function
REGISTER_HELP_FUNCTION(getName, ReplicaSet);
REGISTER_HELP_PROPERTY(name, ReplicaSet);
REGISTER_HELP(REPLICASET_NAME_BRIEF, "${REPLICASET_GETNAME_BRIEF}");
REGISTER_HELP_FUNCTION_TEXT(REPLICASET_GETNAME, R"*(
Returns the name of the replicaset.

@returns name of the replicaset.
)*");

/**
 * $(REPLICASET_GETNAME_BRIEF)
 *
 * $(REPLICASET_GETNAME)
 */
#if DOXYGEN_JS
String ReplicaSet::getName() {}
#elif DOXYGEN_PY
str ReplicaSet::get_name() {}
#endif

void ReplicaSet::assert_valid(const std::string &option_name) const {
  if (option_name == "disconnect") return;

  if (has_member(option_name) && m_invalidated) {
    if (has_method(option_name)) {
      auto name = get_function_name(option_name, false);
      throw shcore::Exception::runtime_error(shcore::str_format(
          "%s.%s: Can't call function '%s' on a dissolved replicaset",
          class_name().c_str(), name.c_str(), name.c_str()));

    } else {
      auto name = get_member_name(option_name, shcore::current_naming_style());
      throw shcore::Exception::runtime_error(shcore::str_format(
          "%s.%s: Can't access object member '%s' on a dissolved replicaset",
          class_name().c_str(), name.c_str(), name.c_str()));
    }
  }

  if (!impl()->check_valid()) {
    throw shcore::Exception::runtime_error(shcore::str_subvars(
        "The replicaset object is disconnected. Please use "
        "dba.<<<getReplicaSet>>>() to obtain a new object."));
  }
}

REGISTER_HELP_FUNCTION(addInstance, ReplicaSet);
REGISTER_HELP_FUNCTION_TEXT(REPLICASET_ADDINSTANCE, R"*(
Adds an instance to the replicaset.

@param instance host:port of the target instance to be added.
@param options Optional dictionary with options for the operation.

@returns Nothing.

This function adds the given MySQL instance as a read-only SECONDARY replica of
the current PRIMARY of the replicaset. Replication will be configured between
the added instance and the PRIMARY. Replication will use an automatically
created MySQL account with a random password.

Once the instance is added to the replicaset, the function will wait for it to
apply all pending transactions. The transaction sync may timeout if the
transaction backlog is large. The timeout option may be used to increase that
time.

The Shell connects to the target instance using the same username and
password used to obtain the replicaset handle object. All instances of the
replicaset are expected to have this same admin account with the same grants
and passwords. A custom admin account with the required grants can be created
while an instance is configured with dba.<<<configureReplicaSetInstance>>>(),
but the root user may be used too.

The PRIMARY of the replicaset must be reachable and available during this
operation.

<b>Pre-Requisites</b>

The following pre-requisites are expected for instances added to a replicaset.
They will be automatically checked by <<<addInstance>>>(), which will stop
if any issues are found.

@li binary log and replication related options must have been validated
and/or configured by dba.<<<configureReplicaSetInstance>>>()

If the selected recovery method is incremental:

@li transaction set in the instance being added must not contain transactions
that don't exist in the PRIMARY
@li transaction set in the instance being added must not be missing transactions
that have been purged from the binary log of the PRIMARY

If clone is available, the pre-requisites listed above can be overcome by using
clone as the recovery method.

<b>Options</b>

The options dictionary may contain the following values:

@li dryRun: if true, performs checks and logs changes that would be made, but
does not execute them
@li label: an identifier for the instance being added, used in the output of
status()
@li recoveryMethod: Preferred method of state recovery. May be auto, clone or
incremental. Default is auto.
@li recoveryProgress: Integer value to indicate the recovery process verbosity
level.
@li cloneDonor: host:port of an existing replicaSet member to clone from.
IPv6 addresses are not supported for this option.
@li timeout: timeout in seconds for transaction sync operations; 0 disables
timeout and force the Shell to wait until the transaction sync finishes.
Defaults to 0.
${OPT_CERT_SUBJECT}
@li replicationConnectRetry: integer that specifies the interval in seconds
between the reconnection attempts that the replica makes after the connection
to the source times out.
@li replicationRetryCount: integer that sets the maximum number of reconnection
attempts that the replica makes after the connection to the source times out.
@li replicationHeartbeatPeriod: decimal that controls the heartbeat interval,
which stops the connection timeout occurring in the absence of data if the
connection is still good.
@li replicationCompressionAlgorithms: string that specifies the permitted
compression algorithms for connections to the replication source.
@li replicationZstdCompressionLevel: integer that specifies the compression
level to use for connections to the replication source server that use the
zstd compression algorithm.
@li replicationBind: string that determines which of the replica's network
interfaces is chosen for connecting to the source.
@li replicationNetworkNamespace: string that specifies the network namespace
to use for TCP/IP connections to the replication source server.

The recoveryMethod option supports the following values:

@li incremental: waits until the new instance has applied missing transactions
from the PRIMARY
@li clone: uses MySQL clone to provision the instance, which completely
replaces the state of the target instance with a full snapshot of another
ReplicaSet member. Requires MySQL 8.0.17 or newer.
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

The cloneDonor option is used to override the automatic selection of a donor to
be used when clone is selected as the recovery method. By default, a SECONDARY
member will be chosen as donor. If no SECONDARY members are available the
PRIMARY will be selected. The option accepts values in the format: 'host:port'.
IPv6 addresses are not supported.
)*");

/**
 * $(REPLICASET_ADDINSTANCE_BRIEF)
 *
 * $(REPLICASET_ADDINSTANCE)
 */
#if DOXYGEN_JS
Undefined ReplicaSet::addInstance(String instance, Dictionary options) {}
#elif DOXYGEN_PY
None ReplicaSet::add_instance(str instance, dict options) {}
#endif
void ReplicaSet::add_instance(
    const std::string &instance_def,
    const shcore::Option_pack_ref<replicaset::Add_instance_options> &options) {
  assert_valid("addInstance");

  // this validates the instance_def
  std::ignore = get_connection_options(shcore::Value(instance_def));

  // TODO(anyone): The implementation should be updated to receive the options
  // object
  auto opts = options;

  execute_with_pool(
      [&]() {
        impl()->add_instance(
            instance_def, options->ar_options, options->clone_options,
            options->instance_label, options->cert_subject,
            opts->get_recovery_progress(), options->timeout,
            current_shell_options()->get().wizards, options->dry_run);
      },
      current_shell_options()->get().wizards);
}

REGISTER_HELP_FUNCTION(rejoinInstance, ReplicaSet);
REGISTER_HELP_FUNCTION_TEXT(REPLICASET_REJOININSTANCE, R"*(
Rejoins an instance to the replicaset.

@param instance host:port of the target instance to be rejoined.
@param options Optional dictionary with options for the operation.

@returns Nothing.

This function rejoins the given MySQL instance to the replicaset, making it a
read-only SECONDARY replica of the current PRIMARY. Only instances previously
added to the replicaset can be rejoined,
otherwise the @<ReplicaSet@>.<<<addInstance>>>() function must be used to
add a new instance. This can also be used to update the replication channel of
instances (even if they are already ONLINE) if it does not have the expected
state, source and settings.

The PRIMARY of the replicaset must be reachable and available during this
operation.

<b>Pre-Requisites</b>

The following pre-requisites are expected for instances rejoined to a
replicaset. They will be automatically checked by <<<rejoinInstance>>>(), which
will stop if any issues are found.

If the selected recovery method is incremental:

@li transaction set in the instance being rejoined must not contain
transactions that don't exist in the PRIMARY
@li transaction set in the instance being rejoined must not be missing
transactions that have been purged from the binary log of the PRIMARY
@li executed transactions set (GTID_EXECUTED) in the instance being
rejoined must not be empty.

If clone is available, the pre-requisites listed above can be overcome by using
clone as the recovery method.

<b>Options</b>

The options dictionary may contain the following values:

@li dryRun: if true, performs checks and logs changes that would be made, but
does not execute them
@li recoveryMethod: Preferred method of state recovery. May be auto, clone or
incremental. Default is auto.
@li recoveryProgress: Integer value to indicate the recovery process verbosity
level.
@li cloneDonor: host:port of an existing replicaSet member to clone from.
IPv6 addresses are not supported for this option.
@li timeout: timeout in seconds for transaction sync operations; 0 disables
timeout and force the Shell to wait until the transaction sync finishes.
Defaults to 0.

The recoveryMethod option supports the following values:

@li incremental: waits until the rejoining instance has applied missing
transactions from the PRIMARY
@li clone: uses MySQL clone to provision the instance, which completely
replaces the state of the target instance with a full snapshot of another
ReplicaSet member. Requires MySQL 8.0.17 or newer.
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

The cloneDonor option is used to override the automatic selection of a donor to
be used when clone is selected as the recovery method. By default, a SECONDARY
member will be chosen as donor. If no SECONDARY members are available the
PRIMARY will be selected. The option accepts values in the format: 'host:port'.
IPv6 addresses are not supported.
)*");

/**
 * $(REPLICASET_REJOININSTANCE_BRIEF)
 *
 * $(REPLICASET_REJOININSTANCE)
 */
#if DOXYGEN_JS
Undefined ReplicaSet::rejoinInstance(String instance, Dictionary options) {}
#elif DOXYGEN_PY
None ReplicaSet::rejoin_instance(str instance, dict options) {}
#endif
void ReplicaSet::rejoin_instance(
    const std::string &instance_def,
    const shcore::Option_pack_ref<replicaset::Rejoin_instance_options>
        &options) {
  assert_valid("rejoinInstance");

  // this validates the instance_def
  std::ignore = get_connection_options(shcore::Value(instance_def));

  auto opts = options;

  execute_with_pool(
      [&]() {
        impl()->rejoin_instance(instance_def, opts->clone_options,
                                opts->get_recovery_progress(), opts->timeout,
                                current_shell_options()->get().wizards,
                                opts->dry_run);
      },
      current_shell_options()->get().wizards);
}

REGISTER_HELP_FUNCTION(removeInstance, ReplicaSet);
REGISTER_HELP_FUNCTION_TEXT(REPLICASET_REMOVEINSTANCE, R"*(
Removes an Instance from the replicaset.

@param instance host:port of the target instance to be removed
@param options Optional dictionary with options for the operation.

@returns Nothing.

This function removes an Instance from the replicaset and stops it from
replicating. The instance must not be the PRIMARY and it is expected to be
reachable.

The PRIMARY of the replicaset must be reachable and available during this
operation.

<b>Options</b>

The instance definition is the connection data for the instance.

The options dictionary may contain the following attributes:

@li force: boolean, indicating if the instance must be removed (even if only
from metadata) in case it cannot be reached. By default, set to false.
@li timeout: maximum number of seconds to wait for the instance to sync up with
the PRIMARY. 0 means no timeout and <0 will skip sync.

The force option (set to true) is required to remove instances that are
unreachable. Removed instances are normally synchronized with the rest of the
replicaset, so that their own copy of the metadata will be consistent and
the instance will be suitable for being added back to the replicaset. When
the force option is set during removal, that step will be skipped.
)*");

/**
 * $(REPLICASET_REMOVEINSTANCE_BRIEF)
 *
 * $(REPLICASET_REMOVEINSTANCE)
 */
#if DOXYGEN_JS
Undefined ReplicaSet::removeInstance(String instance, Dictionary options) {}
#elif DOXYGEN_PY
None ReplicaSet::remove_instance(str instance, dict options) {}
#endif

void ReplicaSet::remove_instance(
    const std::string &instance_def,
    const shcore::Option_pack_ref<replicaset::Remove_instance_options>
        &options) {
  assert_valid("removeInstance");

  // Remove the Instance from the Default ReplicaSet
  bool interactive = current_shell_options()->get().wizards;

  // this validates the instance_def
  (void)get_connection_options(shcore::Value(instance_def));

  execute_with_pool(
      [&]() {
        impl()->remove_instance(instance_def, options->force,
                                options->timeout());
        return shcore::Value();
      },
      interactive);
}

REGISTER_HELP_FUNCTION(describe, ReplicaSet);
REGISTER_HELP_FUNCTION_TEXT(REPLICASET_DESCRIBE, R"*(
Describe the structure of the ReplicaSet.

@returns A JSON object describing the structure of the ReplicaSet.

This function describes the structure of the ReplicaSet including all
its Instances.

The returned JSON object contains the following attributes:

@li name: the ReplicaSet name
@li topology: a list of dictionaries describing each instance belonging to
the ReplicaSet.

Each instance dictionary contains the following attributes:

@li address: the instance address in the form of host:port
@li label: the instance name identifier
@li instanceRole: the instance role (either "PRIMARY" or "REPLICA")
)*");

/**
 * $(REPLICASET_DESCRIBE_BRIEF)
 *
 * $(REPLICASET_DESCRIBE)
 */
#if DOXYGEN_JS
Dictionary ReplicaSet::describe() {}
#elif DOXYGEN_PY
dict ReplicaSet::describe() {}
#endif

shcore::Value ReplicaSet::describe() {
  assert_valid("describe");

  return execute_with_pool([&]() { return impl()->describe(); }, false);
}

REGISTER_HELP_FUNCTION(status, ReplicaSet);
REGISTER_HELP_FUNCTION_TEXT(REPLICASET_STATUS, R"*(
Describe the status of the ReplicaSet.

@param options Optional dictionary with options.

@returns A JSON object describing the status of the ReplicaSet.

This function will connect to each member of the ReplicaSet and query their
state, producing a status report of the ReplicaSet as a whole, as well
as of its individual members.

<b>Options</b>

The following options may be given to control the amount of information gathered
and returned.

@li extended: Can be 0, 1 or 2. Default is 0.

Option 'extended' may have the following values:
@li 0 - regular level of details. Only basic information about the status
of the instance and replication is included, in addition to non-default
or unexpected replication settings and status.
@li 1 - includes Metadata Version, server UUID and the raw information used
to derive the status of the instance, size of the applier queue, value of
system variables that protect against unexpected writes etc.
@li 2 - includes important replication related configuration settings, such as
SSL, worker threads, replication delay and heartbeat delay.
)*");

/**
 * $(REPLICASET_STATUS_BRIEF)
 *
 * $(REPLICASET_STATUS)
 */
#if DOXYGEN_JS
String ReplicaSet::status(Dictionary options) {}
#elif DOXYGEN_PY
str ReplicaSet::status(dict options) {}
#endif

shcore::Value ReplicaSet::status(
    const shcore::Option_pack_ref<replicaset::Status_options> &options) {
  assert_valid("status");

  return execute_with_pool([&]() { return impl()->status(options->extended); },
                           false);
}

REGISTER_HELP_FUNCTION(disconnect, ReplicaSet);
REGISTER_HELP_FUNCTION_TEXT(REPLICASET_DISCONNECT, R"*(
Disconnects all internal sessions used by the replicaset object.

@returns Nothing.

Disconnects the internal MySQL sessions used by the replicaset to query for
metadata and replication information.
)*");

/**
 * $(REPLICASET_DISCONNECT_BRIEF)
 *
 * $(REPLICASET_DISCONNECT)
 */
#if DOXYGEN_JS
Undefined ReplicaSet::disconnect() {}
#elif DOXYGEN_PY
None ReplicaSet::disconnect() {}
#endif

void ReplicaSet::disconnect() { impl()->disconnect(); }

REGISTER_HELP_FUNCTION(setPrimaryInstance, ReplicaSet);
REGISTER_HELP_FUNCTION_TEXT(REPLICASET_SETPRIMARYINSTANCE, R"*(
Performs a safe PRIMARY switchover, promoting the given instance.

@param instance host:port of the target instance to be promoted.
@param options Dictionary with additional options.
@returns Nothing

This command will perform a safe switchover of the PRIMARY of a replicaset.
The current PRIMARY will be demoted to a SECONDARY and made read-only, while
the promoted instance will be made a read-write master. All other SECONDARY
instances will be updated to replicate from the new PRIMARY.

During the switchover, the promoted instance will be synchronized with the
old PRIMARY, ensuring that all transactions present in the PRIMARY are
applied before the topology change is committed. If that synchronization
step takes too long or is not possible in any of the SECONDARY instances, the
switch will be aborted. These problematic SECONDARY instances must be either
repaired or removed from the replicaset for the fail over to be possible.

For a safe switchover to be possible, all replicaset instances must be reachable
from the shell and have consistent transaction sets. If the PRIMARY is not
available, a failover must be performed instead, using <<<forcePrimaryInstance>>>().

<b>Options</b>

The following options may be given:

@li dryRun: if true, will perform checks and log operations that would be
performed, but will not execute them. The operations that would be performed
can be viewed by enabling verbose output in the shell.
@li timeout: integer value with the maximum number of seconds to wait until
the instance being promoted catches up to the current PRIMARY (default
value is retrieved from the 'dba.gtidWaitTimeout' shell option).
)*");

/**
 * $(REPLICASET_SETPRIMARYINSTANCE_BRIEF)
 *
 * $(REPLICASET_SETPRIMARYINSTANCE)
 */
#if DOXYGEN_JS
Undefined ReplicaSet::setPrimaryInstance(String instance, Dictionary options) {}
#elif DOXYGEN_PY
None ReplicaSet::set_primary_instance(str instance, dict options) {}
#endif
void ReplicaSet::set_primary_instance(
    const std::string &instance_def,
    const shcore::Option_pack_ref<replicaset::Set_primary_instance_options>
        &options) {
  assert_valid("setPrimaryInstance");

  execute_with_pool(
      [&]() {
        impl()->set_primary_instance(instance_def, options->timeout(),
                                     options->dry_run);
        return shcore::Value();
      },
      false);
}

REGISTER_HELP_FUNCTION(forcePrimaryInstance, ReplicaSet);
REGISTER_HELP_FUNCTION_TEXT(REPLICASET_FORCEPRIMARYINSTANCE, R"*(
Performs a failover in a replicaset with an unavailable PRIMARY.

@param instance host:port of the target instance to be promoted. If blank, a
suitable instance will be selected automatically.
@param options Dictionary with additional options.
@returns Nothing

This command will perform a failover of the PRIMARY of a replicaset
in disaster scenarios where the current PRIMARY is unavailable and cannot be
restored. If given, the target instance will be promoted to a PRIMARY, while
other reachable SECONDARY instances will be switched to the new PRIMARY. The
target instance must have the most up-to-date GTID_EXECUTED set among reachable
instances, otherwise the operation will fail. If a target instance is not given
(or is null), the most up-to-date instance will be automatically selected and
promoted.

After a failover, the old PRIMARY will be considered invalid by
the new PRIMARY and can no longer be part of the replicaset. If the instance
is still usable, it must be removed from the replicaset and re-added as a
new instance. If there were any SECONDARY instances that could not be
switched to the new PRIMARY during the failover, they will also be considered
invalid.

@attention a failover is a potentially destructive action and should
only be used as a last resort measure.

Data loss is possible after a failover, because the old PRIMARY may
have had transactions that were not yet replicated to the SECONDARY being
promoted. Moreover, if the instance that was presumed to have failed is still
able to process updates, for example because the network where it is located
is still functioning but unreachable from the shell, it will continue diverging
from the promoted clusters.
Recovering or re-conciliating diverged transaction sets requires
manual intervention and may some times not be possible, even if the failed
MySQL servers can be recovered. In many cases, the fastest and simplest
way to recover from a disaster that required a failover is by
discarding such diverged transactions and re-provisioning a new instance
from the newly promoted PRIMARY.

<b>Options</b>

The following options may be given:

@li dryRun: if true, will perform checks and log operations that would be
performed, but will not execute them. The operations that would be performed
can be viewed by enabling verbose output in the shell.
@li timeout: integer value with the maximum number of seconds to wait until
the instance being promoted catches up to the current PRIMARY.
@li invalidateErrorInstances: if false, aborts the failover if any instance
other than the old master is unreachable or has errors. If true, such instances
will not be failed over and be invalidated.
)*");

/**
 * $(REPLICASET_FORCEPRIMARYINSTANCE_BRIEF)
 *
 * $(REPLICASET_FORCEPRIMARYINSTANCE)
 */
#if DOXYGEN_JS
Undefined ReplicaSet::forcePrimaryInstance(String instance,
                                           Dictionary options) {}
#elif DOXYGEN_PY
None ReplicaSet::force_primary_instance(str instance, dict options) {}
#endif
void ReplicaSet::force_primary_instance(
    const std::string &instance_def,
    const shcore::Option_pack_ref<replicaset::Force_primary_instance_options>
        &options) {
  assert_valid("forcePrimaryInstance");

  execute_with_pool(
      [&]() {
        impl()->force_primary_instance(instance_def, options->timeout(),
                                       options->invalidate_instances,
                                       options->dry_run);
      },
      false);
}

REGISTER_HELP_FUNCTION(dissolve, ReplicaSet);
REGISTER_HELP_FUNCTION_TEXT(REPLICASET_DISSOLVE, R"*(
Dissolves the ReplicaSet.

@param options Dictionary with options for the operation.
@returns Nothing

This function stops and deletes asynchronous replication channels and
unregisters all members from the ReplicaSet metadata.

It keeps all the user's data intact.

<b>Options</b>

The options dictionary may contain the following attributes:

@li force: set to true to confirm that the dissolve operation must
be executed, even if some members of the ReplicaSet cannot be reached
or the timeout was reached when waiting for members to catch up with
replication changes. By default, set to false.
@li timeout: maximum number of seconds to wait for pending transactions
to be applied in each reachable instance of the ReplicaSet (default
value is retrieved from the 'dba.gtidWaitTimeout' shell option).

The force option (set to true) must only be used to dissolve a ReplicaSet
with instances that are permanently not available (no longer reachable)
or never to be reused again in a ReplicaSet. This allows a ReplicaSet to be
dissolved and remove it from the metadata, including instances than can no
longer be recovered. Otherwise, the instances must be brought back ONLINE
and the ReplicaSet dissolved without the force option to avoid errors
trying to reuse the instances and add them back to a ReplicaSet.
)*");

/**
 * $(REPLICASET_DISSOLVE_BRIEF)
 *
 * $(REPLICASET_DISSOLVE)
 */
#if DOXYGEN_JS
Undefined ReplicaSet::dissolve(Dictionary options) {}
#elif DOXYGEN_PY
None ReplicaSet::dissolve(dict options) {}
#endif

void ReplicaSet::dissolve(
    const shcore::Option_pack_ref<replicaset::Dissolve_options> &options) {
  assert_valid("dissolve");

  execute_with_pool([this, &options]() { impl()->dissolve(*options); }, false);
}

REGISTER_HELP_FUNCTION(rescan, ReplicaSet);
REGISTER_HELP_FUNCTION_TEXT(REPLICASET_RESCAN, R"*(
Rescans the ReplicaSet.

@param options Dictionary with options for the operation.
@returns Nothing

This function re-scans the ReplicaSet for new (already part of the replication
topology but not managed in the ReplicaSet) and obsolete (no longer part of the
topology) replication members/instances, as well as change to the instances
configurations.

<b>Options</b>

The options dictionary may contain the following attributes:

@li addUnmanaged: if true, all newly discovered instances will be automatically
added to the metadata. Defaults to false.
@li removeObsolete: if true, all obsolete instances will be automatically
removed from the metadata. Defaults to false.
)*");

/**
 * $(REPLICASET_RESCAN_BRIEF)
 *
 * $(REPLICASET_RESCAN)
 */
#if DOXYGEN_JS
Undefined ReplicaSet::rescan(Dictionary options) {}
#elif DOXYGEN_PY
None ReplicaSet::rescan(dict options) {}
#endif

void ReplicaSet::rescan(
    const shcore::Option_pack_ref<replicaset::Rescan_options> &options) {
  assert_valid("rescan");

  execute_with_pool([this, &options]() { impl()->rescan(*options); }, false);
}

REGISTER_HELP_FUNCTION(listRouters, ReplicaSet);
REGISTER_HELP_FUNCTION_TEXT(REPLICASET_LISTROUTERS, LISTROUTERS_HELP_TEXT);

/**
 * $(REPLICASET_LISTROUTERS_BRIEF)
 *
 * $(REPLICASET_LISTROUTERS)
 */
#if DOXYGEN_JS
String ReplicaSet::listRouters(Dictionary options) {}
#elif DOXYGEN_PY
str ReplicaSet::list_routers(dict options) {}
#endif

REGISTER_HELP_FUNCTION(removeRouterMetadata, ReplicaSet);
REGISTER_HELP_FUNCTION_TEXT(REPLICASET_REMOVEROUTERMETADATA, R"*(
Removes metadata for a router instance.

@param routerDef identifier of the router instance to be removed (e.g.
192.168.45.70@::system)
@returns Nothing

MySQL Router automatically registers itself within the InnoDB cluster
metadata when bootstrapped. However, that metadata may be left behind when
instances are uninstalled or moved over to a different host. This function may
be used to clean up such instances that no longer exist.

The @<ReplicaSet@>.<<<listRouters>>>() function may be used to list
registered router instances, including their identifier.
)*");
/**
 * $(REPLICASET_REMOVEROUTERMETADATA_BRIEF)
 *
 * $(REPLICASET_REMOVEROUTERMETADATA)
 */
#if DOXYGEN_JS
String ReplicaSet::removeRouterMetadata(RouterDef routerDef) {}
#elif DOXYGEN_PY
str ReplicaSet::remove_router_metadata(RouterDef routerDef) {}
#endif
void ReplicaSet::remove_router_metadata(const std::string &router_def) {
  assert_valid("removeRouterMetadata");

  return execute_with_pool(
      [&]() { impl()->remove_router_metadata(router_def); }, false);
}

REGISTER_HELP_FUNCTION(setupAdminAccount, ReplicaSet);
REGISTER_HELP_FUNCTION_TEXT(REPLICASET_SETUPADMINACCOUNT,
                            SETUPADMINACCOUNT_HELP_TEXT);

/**
 * $(REPLICASET_SETUPADMINACCOUNT_BRIEF)
 *
 * $(REPLICASET_SETUPADMINACCOUNT)
 */
#if DOXYGEN_JS
Undefined ReplicaSet::setupAdminAccount(String user, Dictionary options) {}
#elif DOXYGEN_PY
None ReplicaSet::setup_admin_account(str user, dict options) {}
#endif

REGISTER_HELP_FUNCTION(setupRouterAccount, ReplicaSet);
REGISTER_HELP_FUNCTION_TEXT(REPLICASET_SETUPROUTERACCOUNT,
                            SETUPROUTERACCOUNT_HELP_TEXT);

/**
 * $(REPLICASET_SETUPROUTERACCOUNT_BRIEF)
 *
 * $(REPLICASET_SETUPROUTERACCOUNT)
 */
#if DOXYGEN_JS
Undefined ReplicaSet::setupRouterAccount(String user, Dictionary options) {}
#elif DOXYGEN_PY
None ReplicaSet::setup_router_account(str user, dict options) {}
#endif

REGISTER_HELP_FUNCTION(routingOptions, ReplicaSet);
REGISTER_HELP_FUNCTION_TEXT(REPLICASET_ROUTINGOPTIONS,
                            ROUTINGOPTIONS_HELP_TEXT);

/**
 * $(REPLICASET_ROUTINGOPTIONS_BRIEF)
 *
 * $(REPLICASET_ROUTINGOPTIONS)
 */
#if DOXYGEN_JS
Dictionary ReplicaSet::routingOptions(String router) {}
#elif DOXYGEN_PY
dict ReplicaSet::routing_options(str router) {}
#endif

REGISTER_HELP_FUNCTION(routerOptions, ReplicaSet);
REGISTER_HELP_FUNCTION_TEXT(REPLICASET_ROUTEROPTIONS, R"*(
Lists the configuration options of the ReplicaSet's Routers.

@param options Dictionary with options for the operation.

@returns A JSON object with the list of Router configuration options.

This function lists the Router configuration options of the ReplicaSet (global),
and the Router instances. By default, only the options that can be changed from
Shell (dynamic options) are displayed.
Router instances with different configurations than the global ones will
include the differences under their dedicated description.

The options dictionary may contain the following attributes:

@li extended: Verbosity level of the command output.
@li router: Identifier of the Router instance to be displayed.

The extended option supports Integer or Boolean values:

@li 0: Includes only options that can be changed from Shell (default);
@li 1: Includes all ReplicaSet global options and, per Router, only the options
that have a different value than the corresponding global one.
@li 2: Includes all Cluster and Router options.
@li Boolean: equivalent to assign either 0 (false) or 1 (true).
)*");

/**
 * $(REPLICASET_ROUTEROPTIONS_BRIEF)
 *
 * $(REPLICASET_ROUTEROPTIONS)
 */
#if DOXYGEN_JS
Dictionary ReplicaSet::routerOptions(Dictionary options) {}
#elif DOXYGEN_PY
dict ReplicaSet::router_options(dict options) {}
#endif

REGISTER_HELP_FUNCTION(setRoutingOption, ReplicaSet);
REGISTER_HELP_FUNCTION_TEXT(REPLICASET_SETROUTINGOPTION, R"*(
Changes the value of either a global ReplicaSet Routing option or of a single
Router instance.

@param router optional identifier of the target router instance (e.g.
192.168.45.70@::system).
@param option The Router option to be changed.
@param value The value that the option shall get (or null to unset).

@returns Nothing.

The accepted options are:

@li tags: Associates an arbitrary JSON object with custom key/value pairs with
the ReplicaSet metadata.
@li stats_updates_frequency: Number of seconds between updates that the Router
is to make to its statistics in the InnoDB Cluster metadata.

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
)*");

#if DOXYGEN_JS
Undefined ReplicaSet::setRoutingOption(String option, String value) {}
#elif DOXYGEN_PY
None ReplicaSet::set_routing_option(str option, str value) {}
#endif

/**
 * $(REPLICASET_SETROUTINGOPTION_BRIEF)
 *
 * $(REPLICASET_SETROUTINGOPTION)
 */
#if DOXYGEN_JS
Undefined ReplicaSet::setRoutingOption(String router, String option,
                                       String value) {}
#elif DOXYGEN_PY
None ReplicaSet::set_routing_option(str router, str option, str value) {}
#endif

REGISTER_HELP_FUNCTION(options, ReplicaSet);
REGISTER_HELP_FUNCTION_TEXT(REPLICASET_OPTIONS, R"*(
Lists the ReplicaSet configuration options.

@returns A JSON object describing the configuration options of the ReplicaSet.

This function lists the configuration options for the ReplicaSet and
its instances.
)*");

/**
 * $(REPLICASET_OPTIONS_BRIEF)
 *
 * $(REPLICASET_OPTIONS)
 */
#if DOXYGEN_JS
String ReplicaSet::options() {}
#elif DOXYGEN_PY
str ReplicaSet::options() {}
#endif

shcore::Value ReplicaSet::options() {
  assert_valid("options");

  return execute_with_pool([&]() { return impl()->options(); }, false);
}

REGISTER_HELP_FUNCTION(setOption, ReplicaSet);
REGISTER_HELP_FUNCTION_TEXT(REPLICASET_SETOPTION, R"*(
Changes the value of an option for the whole ReplicaSet.

@param option The option to be changed.
@param value The value that the option shall get.

@returns Nothing.

This function changes an option for the ReplicaSet.

The accepted options are:
@li replicationAllowedHost string value to use as the host name part of
internal replication accounts. Existing accounts will be re-created with the new
value.
${NAMESPACE_TAG}

${NAMESPACE_TAG_DETAIL_REPLICASET}
)*");

/**
 * $(REPLICASET_SETOPTION_BRIEF)
 *
 * $(REPLICASET_SETOPTION)
 */
#if DOXYGEN_JS
Undefined ReplicaSet::setOption(String option, String value) {}
#elif DOXYGEN_PY
None ReplicaSet::set_option(str option, str value) {}
#endif

void ReplicaSet::set_option(const std::string &option,
                            const shcore::Value &value) {
  assert_valid("setOption");

  execute_with_pool(
      [&]() {
        impl()->set_option(option, value);
        return shcore::Value();
      },
      false);
}

REGISTER_HELP_FUNCTION(setInstanceOption, ReplicaSet);
REGISTER_HELP_FUNCTION_TEXT(ReplicaSet_SETINSTANCEOPTION, R"*(
Changes the value of an option in a ReplicaSet member.

@param instance host:port of the target instance.
@param option The option to be changed.
@param value The value that the option shall get.

@returns Nothing.

This function changes an option for a member of the ReplicaSet.

The accepted options are:
${NAMESPACE_TAG}
@li replicationConnectRetry: integer that specifies the interval in seconds
between the reconnection attempts that the replica makes after the connection
to the source times out.
@li replicationRetryCount: integer that sets the maximum number of reconnection
attempts that the replica makes after the connection to the source times out.
@li replicationHeartbeatPeriod: decimal that controls the heartbeat interval,
which stops the connection timeout occurring in the absence of data if the
connection is still good.
@li replicationCompressionAlgorithms: string that specifies the permitted
compression algorithms for connections to the replication source.
@li replicationZstdCompressionLevel: integer that specifies the compression
level to use for connections to the replication source server that use the zstd
compression algorithm.
@li replicationBind: string that determines which of the replica's network
interfaces is chosen for connecting to the source.
@li replicationNetworkNamespace: string that specifies the network namespace
to use for TCP/IP connections to the replication source server.

@note Changing any of the "replication*" options won't immediately update the
replication channel. Only on the next call to rejoinInstance() will the updated
options take effect.
@note Any of the "replication*" options accepts 'null', which sets the
corresponding option to its default value on the next call to rejoinInstance().

${NAMESPACE_TAG_DETAIL_REPLICASET}

${NAMESPACE_TAG_INSTANCE_DETAILS_EXTRA}
)*");

/**
 * $(REPLICASET_SETINSTANCEOPTION_BRIEF)
 *
 * $(REPLICASET_SETINSTANCEOPTION)
 */
#if DOXYGEN_JS
Undefined ReplicaSet::setInstanceOption(String instance, String option,
                                        String value) {}
#elif DOXYGEN_PY
None ReplicaSet::set_instance_option(str instance, str option, str value);
#endif
void ReplicaSet::set_instance_option(const std::string &instance_def,
                                     const std::string &option,
                                     const shcore::Value &value) {
  assert_valid("setInstanceOption");

  execute_with_pool(
      [&]() {
        // Set the option in the Default ReplicaSet
        impl()->set_instance_option(instance_def, option, value);
        return shcore::Value();
      },
      false);
}

}  // namespace dba
}  // namespace mysqlsh
