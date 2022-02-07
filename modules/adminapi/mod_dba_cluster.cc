/*
 * Copyright (c) 2016, 2022, Oracle and/or its affiliates.
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

#include "modules/adminapi/cluster_set/cluster_set_impl.h"
#include "modules/adminapi/common/accounts.h"
#include "modules/adminapi/common/common.h"
#include "modules/adminapi/common/dba_errors.h"
#include "modules/adminapi/common/preconditions.h"
#include "modules/adminapi/common/sql.h"
#include "modules/adminapi/mod_dba_cluster.h"
#include "modules/adminapi/mod_dba_cluster_set.h"
#include "modules/mysqlxtest_utils.h"
#include "mysqlshdk/include/scripting/type_info/custom.h"
#include "mysqlshdk/include/scripting/type_info/generic.h"
#include "mysqlshdk/include/shellcore/utils_help.h"
#include "mysqlshdk/libs/utils/debug.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_json.h"
#include "mysqlshdk/shellcore/shell_console.h"

using std::placeholders::_1;

DEBUG_OBJ_ENABLE(Cluster);

namespace mysqlsh {
namespace dba {
using mysqlshdk::db::uri::formats::only_transport;
using mysqlshdk::db::uri::formats::user_transport;
// Documentation of the Cluster Class
REGISTER_HELP_CLASS(Cluster, adminapi);
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
  expose("checkInstanceState", &Cluster::check_instance_state, "instance")
      ->cli();
  expose("setupAdminAccount", &Cluster::setup_admin_account, "user", "?options")
      ->cli();
  expose("setupRouterAccount", &Cluster::setup_router_account, "user",
         "?options")
      ->cli();
  expose("rescan", &Cluster::rescan, "?options")->cli();
  expose("forceQuorumUsingPartitionOf",
         &Cluster::force_quorum_using_partition_of, "instance", "?password")
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
  expose("listRouters", &Cluster::list_routers, "?options")->cli();
  expose("removeRouterMetadata", &Cluster::remove_router_metadata, "router")
      ->cli();
  expose("createClusterSet", &Cluster::create_cluster_set, "domainName",
         "?options")
      ->cli();
  expose("getClusterSet", &Cluster::get_cluster_set)->cli(false);
  expose("fenceAllTraffic", &Cluster::fence_all_traffic)->cli();
  expose("fenceWrites", &Cluster::fence_writes)->cli();
  expose("unfenceWrites", &Cluster::unfence_writes)->cli();
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
                                             "' on an offline cluster");
    } else {
      name = get_member_name(option_name, shcore::current_naming_style());
      throw shcore::Exception::runtime_error(class_name() + "." + name + ": " +
                                             "Can't access object member '" +
                                             name + "' on an offline cluster");
    }
  }
  if (!m_impl->get_cluster_server()) {
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
${CLUSTER_OPT_IP_WHITELIST}
${CLUSTER_OPT_IP_ALLOWLIST}
@li localAddress: string value with the Group Replication local address to be
used instead of the automatically generated one.
@li groupSeeds: string value with a comma-separated list of the Group
Replication peer addresses to be used instead of the automatically generated
one. Deprecated and ignored.
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
@li VERIFY_CA: Like REQUIRED, but additionally verify the server TLS
certificate against the configured Certificate Authority (CA) certificates.
@li VERIFY_IDENTITY: Like VERIFY_CA, but additionally verify that the server
certificate matches the host to which the connection is attempted.
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

${CLUSTER_OPT_IP_ALLOWLIST_EXTRA}

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

The groupSeeds option is deprecated as of MySQL Shell 8.0.28 and is ignored.
'group_replication_group_seeds' is automatically set based on the current
topology.

${CLUSTER_OPT_EXIT_STATE_ACTION_EXTRA}

${CLUSTER_OPT_MEMBER_WEIGHT_DETAIL_EXTRA}

${CLUSTER_OPT_AUTO_REJOIN_TRIES_EXTRA}

@attention The ipWhitelist option will be removed in a future release.
Please use the ipAllowlist option instead.

@attention The groupSeeds option will be removed in a future release.
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
    const Connection_options &instance_def_,
    const shcore::Option_pack_ref<cluster::Add_instance_options> &options) {
  auto instance_def = instance_def_;

  if (!options->password.is_null()) {
    instance_def.clear_password();
    instance_def.set_password(*(options->password));
  }

  Instance_pool::Auth_options auth_opts;
  auth_opts.get(m_impl->get_cluster_server()->get_connection_options());
  Scoped_instance_pool ipool(options->interactive(), auth_opts);

  // Validate the label value.
  if (!options->label.is_null()) {
    mysqlsh::dba::validate_label(*(options->label));

    if (!m_impl->get_metadata_storage()->is_instance_label_unique(
            m_impl->get_id(), *(options->label))) {
      throw shcore::Exception::argument_error(
          "An instance with label '" + *(options->label) +
          "' is already part of this InnoDB cluster");
    }
  }

  // Init progress_style
  Recovery_progress_style progress_style;

  if (options->wait_recovery == 0) {
    progress_style = Recovery_progress_style::NOWAIT;
  } else if (options->wait_recovery == 1) {
    progress_style = Recovery_progress_style::NOINFO;
  } else if (options->wait_recovery == 2) {
    progress_style = Recovery_progress_style::TEXTUAL;
  } else {
    progress_style = Recovery_progress_style::PROGRESSBAR;
  }

  // Add the Instance to the Cluster
  m_impl->add_instance(instance_def, *options, progress_style);
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

@li password: the instance connection password
@li memberSslMode: SSL mode used on the instance
${OPT_INTERACTIVE}
${CLUSTER_OPT_IP_WHITELIST}
${CLUSTER_OPT_IP_ALLOWLIST}

The password may be contained on the instance definition, however, it can be
overwritten if it is specified on the options.

@attention The memberSslMode option will be removed in a future release.

The memberSslMode option supports these values:

@li REQUIRED: if used, SSL (encryption) will be enabled for the instance to
communicate with other members of the cluster
@li VERIFY_CA: Like REQUIRED, but additionally verify the server TLS
certificate against the configured Certificate Authority (CA) certificates.
@li VERIFY_IDENTITY: Like VERIFY_CA, but additionally verify that the server
certificate matches the host to which the connection is attempted.
@li DISABLED: if used, SSL (encryption) will be disabled
@li AUTO: if used, SSL (encryption) will be automatically enabled or disabled
based on the cluster configuration

If memberSslMode is not specified AUTO will be used by default.

${CLUSTER_OPT_IP_ALLOWLIST_EXTRA}

@attention The ipWhitelist option will be removed in a future release.
Please use the ipAllowlist option instead.
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
    const Connection_options &instance_def_,
    const shcore::Option_pack_ref<cluster::Rejoin_instance_options> &options) {
  auto instance_def = instance_def_;

  if (!options->password.is_null()) {
    instance_def.clear_password();
    instance_def.set_password(*(options->password));
  }

  // Throw an error if the cluster has already been dissolved
  assert_valid("rejoinInstance");

  m_impl->check_preconditions("rejoinInstance");

  Instance_pool::Auth_options auth_opts;
  auth_opts.get(m_impl->get_cluster_server()->get_connection_options());
  Scoped_instance_pool ipool(options->interactive(), auth_opts);

  // Rejoin the Instance to the Cluster
  m_impl->rejoin_instance(instance_def, options->gr_options,
                          options->interactive(), false);
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
    const Connection_options &instance_def_,
    const shcore::Option_pack_ref<cluster::Remove_instance_options> &options) {
  auto instance_def = instance_def_;

  if (!options->password.is_null()) {
    instance_def.clear_password();
    instance_def.set_password(*(options->password));
  }

  // Throw an error if the cluster has already been dissolved
  assert_valid("removeInstance");

  Instance_pool::Auth_options auth_opts;
  auth_opts.get(m_impl->get_cluster_server()->get_connection_options());
  Scoped_instance_pool ipool(options->interactive(), auth_opts);

  // Remove the Instance from the Cluster
  m_impl->remove_instance(instance_def, options->force, options->interactive());
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

  Instance_pool::Auth_options auth_opts;
  auth_opts.get(m_impl->get_cluster_server()->get_connection_options());
  Scoped_instance_pool ipool(false, auth_opts);

  return m_impl->status(options->extended);
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

  return m_impl->options(options->all);
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

void Cluster::dissolve(
    const shcore::Option_pack_ref<Force_interactive_options> &options) {
  // Throw an error if the cluster has already been dissolved
  assert_valid("dissolve");

  Instance_pool::Auth_options auth_opts;
  auth_opts.get(m_impl->get_cluster_server()->get_connection_options());
  Scoped_instance_pool ipool(options->interactive(), auth_opts);

  m_impl->dissolve(options->force, options->interactive());

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
    const shcore::Option_pack_ref<Force_interactive_options> &options) {
  // Throw an error if the cluster has already been dissolved
  assert_valid("resetRecoveryAccountsPassword");

  // Reset the recovery passwords.
  m_impl->reset_recovery_password(options->force, options->interactive());
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
metadata is not updated (false). Deprecated.
@li upgradeCommProtocol: boolean. Set to true to upgrade the Group Replication
communication protocol to the highest version possible.
@li updateViewChangeUuid: boolean value used to indicate if the command should
generate and set a value for Group Replication View Change UUID in the whole
Cluster. Required for InnoDB ClusterSet usage.

The value for addInstances and removeInstances is used to specify which
instances to add or remove from the metadata, respectively. Both options accept
list connection data. In addition, the "auto" value can be used for both
options in order to automatically add or remove the instances in the metadata,
without having to explicitly specify them.

@attention The updateTopologyMode option will be removed in a future release.
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

  Instance_pool::Auth_options auth_opts;
  auth_opts.get(m_impl->get_cluster_server()->get_connection_options());
  Scoped_instance_pool ipool(false, auth_opts);

  m_impl->rescan(*options);
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

  if (!instance_def.has_port() && !instance_def.has_socket()) {
    instance_def.set_port(mysqlshdk::db::k_default_mysql_port);
  }

  instance_def.set_default_data();

  bool interactive = current_shell_options()->get().wizards;

  m_impl->force_quorum_using_partition_of(instance_def, interactive);
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

  Instance_pool::Auth_options auth_opts;
  auth_opts.get(m_impl->get_cluster_server()->get_connection_options());
  Scoped_instance_pool ipool(false, auth_opts);

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

  m_impl->set_primary_instance(instance_def, *options);
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
${CLUSTER_OPT_FAILOVER_CONSISTENCY}
${CLUSTER_OPT_CONSISTENCY}
${CLUSTER_OPT_EXPEL_TIMEOUT}
${CLUSTER_OPT_AUTO_REJOIN_TRIES}
@li disableClone: boolean value used to disable the clone usage on the cluster.
@li replicationAllowedHost string value to use as the host name part of
internal replication accounts. Existing accounts will be re-created with the new
value.

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

  m_impl->set_option(option, value);
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
@li label a string identifier of the instance.

${CLUSTER_OPT_EXIT_STATE_ACTION_DETAIL}

${CLUSTER_OPT_EXIT_STATE_ACTION_EXTRA}

${CLUSTER_OPT_MEMBER_WEIGHT_DETAIL_EXTRA}

${CLUSTER_OPT_AUTO_REJOIN_TRIES_EXTRA}

${NAMESPACE_TAG_DETAIL_CLUSTER}

${NAMESPACE_TAG_INSTANCE_DETAILS_EXTRA}
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
  m_impl->set_instance_option(instance_def.as_uri(), option, value);
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
    const shcore::Option_pack_ref<List_routers_options> &options) {
  // Throw an error if the cluster has already been dissolved
  assert_valid("listRouters");

  auto target_instance = m_impl->get_cluster_server();
  auto metadadata = m_impl->get_metadata_storage();

  // Throw an error if the cluster has already been dissolved
  check_function_preconditions("Cluster.listRouters", metadadata,
                               target_instance);

  auto ret_val = m_impl->list_routers(options->only_upgrade_required);

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

  auto target_instance = m_impl->get_cluster_server();
  auto metadadata = m_impl->get_metadata_storage();

  // Throw an error if the cluster has already been dissolved
  check_function_preconditions("Cluster.removeRouterMetadata", metadadata,
                               target_instance);

  Instance_pool::Auth_options auth_opts;
  auth_opts.get(m_impl->get_cluster_server()->get_connection_options());
  Scoped_instance_pool ipool(false, auth_opts);

  m_impl->acquire_primary();
  auto finally_primary =
      shcore::on_leave_scope([this]() { m_impl->release_primary(); });

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
)*");

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

void Cluster::setup_admin_account(
    const std::string &user,
    const shcore::Option_pack_ref<Setup_account_options> &options) {
  // Throw an error if the cluster has already been dissolved
  assert_valid("setupAdminAccount");

  // split user into user/host
  std::string username, host;
  std::tie(username, host) = validate_account_name(user);

  m_impl->setup_admin_account(username, host, *options);
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
)*");

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
void Cluster::setup_router_account(
    const std::string &user,
    const shcore::Option_pack_ref<Setup_account_options> &options) {
  // Throw an error if the cluster has already been dissolved
  assert_valid("setupRouterAccount");

  // split user into user/host
  std::string username, host;
  std::tie(username, host) = validate_account_name(user);

  m_impl->setup_router_account(username, host, *options);
}

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

  m_impl->fence_all_traffic();

  // Invalidate the cluster object
  invalidate();
}

REGISTER_HELP_FUNCTION(fenceWrites, Cluster);
REGISTER_HELP_FUNCTION_TEXT(CLUSTER_FENCEWRITES, R"*(
Fences a Cluster from Write Traffic.

@returns Nothing

This function fences a PRIMARY Cluster from all Write Traffic by ensuring all
of its members are Read-Only regardless of any topology change on it.
The Cluster will be put into READ ONLY mode and all members will remain
available for reads.
To unfence the Cluster so it restores its normal functioning and can accept all
traffic use Cluster.unfence().

Use this function when performing a PRIMARY Cluster failover in a ClusterSet to
allow only read traffic in the previous Primary Cluster in the event of a
split-brain.

The function is not permitted on REPLICA Clusters.
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

  m_impl->fence_writes();
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

  m_impl->unfence_writes();
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

@li REQUIRED: if used, SSL (encryption) will be enabled for the ClusterSet
replication channels.
@li DISABLED: if used, SSL (encryption) will be disabled for the ClusterSet
replication channels.
@li AUTO: if used, SSL (encryption) will be enabled if supported by the
instance, otherwise disabled.

If clusterSetReplicationSslMode is not specified AUTO will be used by default.
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
  Instance_pool::Auth_options auth_opts;
  auth_opts.get(m_impl->get_cluster_server()->get_connection_options());
  Scoped_instance_pool ipool(false, auth_opts);

  return m_impl->create_cluster_set(domain_name, *options);
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
std::shared_ptr<ClusterSet> Cluster::get_cluster_set() {
  m_impl->check_preconditions("getClusterSet");

  auto cs = m_impl->get_cluster_set();

  // Check if the target Cluster is invalidated
  if (impl()->is_invalidated()) {
    mysqlsh::current_console()->print_warning(
        "Cluster '" + impl()->get_name() +
        "' was INVALIDATED and must be removed from the ClusterSet.");
  }

  // Check if the target Cluster still belongs to the ClusterSet
  Cluster_set_member_metadata cluster_member_md;
  if (!cs->get_metadata_storage()->get_cluster_set_member(impl()->get_id(),
                                                          &cluster_member_md)) {
    current_console()->print_error("The Cluster '" + impl()->get_name() +
                                   "' appears to have been removed from "
                                   "the ClusterSet '" +
                                   cs->get_name() +
                                   "', however its own metadata copy wasn't "
                                   "properly updated during the removal");

    throw shcore::Exception("The cluster '" + impl()->get_name() +
                                "' is not a part of the ClusterSet '" +
                                cs->get_name() + "'",
                            SHERR_DBA_CLUSTER_DOES_NOT_BELONG_TO_CLUSTERSET);
  }

  return std::make_shared<mysqlsh::dba::ClusterSet>(cs);
}

}  // namespace dba
}  // namespace mysqlsh
