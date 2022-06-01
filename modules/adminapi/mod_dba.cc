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

#include "modules/adminapi/mod_dba.h"

#include <mysqld_error.h>
#include <algorithm>
#include <memory>
#include <random>
#include <string>
#include "utils/error.h"

#ifndef WIN32
#include <sys/un.h>
#endif

#include <iterator>
#include <utility>
#include <vector>

#include "modules/adminapi/cluster/cluster_join.h"
#include "modules/adminapi/common/clone_options.h"
#include "modules/adminapi/common/dba_errors.h"
#include "modules/adminapi/common/group_replication_options.h"
#include "modules/adminapi/common/metadata_management_mysql.h"
#include "modules/adminapi/common/metadata_storage.h"
#include "modules/adminapi/common/preconditions.h"
#include "modules/adminapi/common/sql.h"
#include "modules/adminapi/common/validations.h"
#include "modules/adminapi/dba/check_instance.h"
#include "modules/adminapi/dba/configure_instance.h"
#include "modules/adminapi/dba/configure_local_instance.h"
#include "modules/adminapi/dba/create_cluster.h"
#include "modules/adminapi/dba/upgrade_metadata.h"
#include "modules/adminapi/dba_utils.h"
#include "modules/mod_mysql_resultset.h"
#include "modules/mod_shell.h"
#include "modules/mod_utils.h"
#include "modules/mysqlxtest_utils.h"
#include "mysqlshdk/include/scripting/object_factory.h"
#include "mysqlshdk/include/scripting/type_info/custom.h"
#include "mysqlshdk/include/scripting/type_info/generic.h"
#include "mysqlshdk/include/scripting/types_cpp.h"
#include "mysqlshdk/include/shellcore/utils_help.h"
#include "mysqlshdk/libs/db/mysql/session.h"
#include "mysqlshdk/libs/mysql/async_replication.h"
#include "mysqlshdk/libs/mysql/group_replication.h"
#include "mysqlshdk/libs/mysql/replication.h"
#include "mysqlshdk/libs/mysql/utils.h"
#include "mysqlshdk/libs/utils/logger.h"
#include "mysqlshdk/libs/utils/utils_file.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_net.h"
#include "mysqlshdk/libs/utils/utils_path.h"
#include "mysqlshdk/libs/utils/utils_sqlstring.h"
#include "mysqlshdk/libs/utils/utils_string.h"
#include "mysqlshdk/shellcore/shell_console.h"

/*
  Sessions used by AdminAPI
  =========================

  The following is a list of the types of MySQL sessions that exist in the
  AdminAPI. These are conceptual types of sessions and don't necessarily
  use different classes (they're all db::mysql::Session) or different session
  object instances.

  Shell session
  -------------

  The session opened by the shell to connect to the server the user gives in
  the command line, \connect or shell.connect().

  Use by DevAPI and the SQL mode, but not supposed to be used by AdminAPI,
  except for figuring out what is the cluster being managed.

  Target member session
  ---------------------

  A session to the MySQL server instance that is being operated on for cluster
  operations, like the member being added or removed from the group.

  Only instance local operations are meant to be performed with this session.
  Group-wide operations must go through the Group session.

  Metadata session
  ----------------

  The session to the MySQL server that stores the metadata. The general design
  is meant to allow the metadata to be stored in a location different from the
  cluster being managed, although the implementation does not allow that today..

  The MySQL session used for metadata operations is not shared with the
  shell session.

  When making changes to the cluster, the metadata session must be read-write
  and thus, it will connect to the primary by default when created. The
  connection to primary will not be required if the option sets the option for
  that.

  Only metadata operations may use this session (that is, reads and writes on
  the mysql_innodb_cluster_metadata schema).

  Group session
  -------------

  A session to a member of a GR group (usually the primary), that will be used
  to query the performance_schema to know the live status of the group/cluster.

  Also the member from which group-wide changes are applied, like creating the
  replication user account for new members. Thus, if changes to the cluster are
  going to be made, the group session must be connected to the primary.

  If possible, the group session will shared the same session as the metadata,
  but not always.

  Scenarios
  ---------

  The shell's target is the primary of the group being managed:
  Metadata  - connect to target
  Group     - share from metadata
  Member    - connect to shell target or param URI

  The shell's target is the secondary of the group being managed and
  connectToPrimary is true:
  Metadata  - find primary from metadata in target and connect
  Group     - share from metadata
  Member    - connect to shell target or param URI

  The shell's target is the secondary of the group being managed and
  connectToPrimary is false:
  Metadata  - connect to target
  Group     - share from metadata
  Member    - connect to shell target or param URI

  Note: in the future, if the managed cluster is not where the metadata resides,
  the metadata schema should contain information about where to find the
  real metadata.
 */

using std::placeholders::_1;

namespace mysqlsh {
namespace dba {

namespace sandbox {
struct Op_data {
  std::string name;
  std::string progressive;
  std::string past;
};

static std::map<std::string, Op_data> Operations_text{
    {"delete", {"deleteSandboxInstance", "Deleting", "deleted"}},
    {"kill", {"killSandboxInstance", "Killing", "killed"}},
    {"stop", {"stopSandboxInstance", "Stopping", "stopped"}},
    {"start", {"startSandboxInstance", "Starting", "started"}}};

}  // namespace sandbox

namespace {
void validate_port(int port, const std::string &name) {
  if (port < 1024 || port > 65535)
    throw shcore::Exception::argument_error(
        "Invalid value for '" + name +
        "': Please use a valid TCP port number >= 1024 and <= 65535");
}

void throw_instance_op_error(const shcore::Array_t &errors) {
  if (!errors || errors->empty()) return;

  std::vector<std::string> str_errors;
  str_errors.reserve(errors->size());

  for (const auto &error : *errors) {
    auto data = error.as_map();
    auto error_type = data->get_string("type");
    auto error_text = data->get_string("msg");
    str_errors.push_back(error_type + ": " + error_text);
  }

  throw shcore::Exception::runtime_error(shcore::str_join(str_errors, "\n"));
}

}  // namespace

using mysqlshdk::db::uri::formats::only_transport;

/*
 * Global helper text for setOption and setInstance Option namespaces
 */
REGISTER_HELP(NAMESPACE_TAG,
              "@li tag:@<option@>: built-in and user-defined tags to be "
              "associated to the Cluster.");

REGISTER_HELP_DETAIL_TEXT(NAMESPACE_TAG_DETAIL_CLUSTER, R"*(
<b>Tags</b>

Tags make it possible to associate custom key/value pairs to a Cluster,
storing them in its metadata. Custom tag names can be any string
starting with letters and followed by letters, numbers and _.
Tag values may be any JSON value. If the value is null, the tag is deleted.
)*");

REGISTER_HELP_DETAIL_TEXT(NAMESPACE_TAG_DETAIL_REPLICASET, R"*(
<b>Tags</b>

Tags make it possible to associate custom key/value pairs to a ReplicaSet,
storing them in its metadata. Custom tag names can be any string
starting with letters and followed by letters, numbers and _.
Tag values may be any JSON value. If the value is null, the tag is deleted.
)*");

REGISTER_HELP_DETAIL_TEXT(NAMESPACE_TAG_INSTANCE_DETAILS_EXTRA, R"*(
The following pre-defined tags are available:
@li _hidden: bool - instructs the router to exclude the instance from the
list of possible destinations for client applications.
@li _disconnect_existing_sessions_when_hidden: bool - instructs the router
to disconnect existing connections from instances that are marked to be hidden.
@note '_hidden' and '_disconnect_existing_sessions_when_hidden' can be useful to shut
down the instance and perform maintenance on it without disrupting incoming
application traffic.
)*");

/*
 * Global helper text for InnoDB Cluster configuration options used in:
 *  - dba.createCluster()
 *  - Cluster.addInstance()
 *  - Cluster.setOption()
 *  - Cluster.setInstanceOption()
 *  - ClusterSet.createReplicaCluster()
 */
REGISTER_HELP(CLUSTER_OPT_EXIT_STATE_ACTION,
              "@li exitStateAction: string value indicating the group "
              "replication exit state action.");
REGISTER_HELP(CLUSTER_OPT_MEMBER_WEIGHT,
              "@li memberWeight: integer value with a percentage weight for "
              "automatic primary election on failover.");
REGISTER_HELP(CLUSTER_OPT_FAILOVER_CONSISTENCY,
              "@li failoverConsistency: string value indicating the "
              "consistency guarantees that the cluster provides.");
REGISTER_HELP(CLUSTER_OPT_CONSISTENCY,
              "@li consistency: string value indicating the "
              "consistency guarantees that the cluster provides.");
REGISTER_HELP(CLUSTER_OPT_EXPEL_TIMEOUT,
              "@li expelTimeout: integer value to define the time period in "
              "seconds that cluster members should wait for a non-responding "
              "member before evicting it from the cluster.");
REGISTER_HELP(
    CLUSTER_OPT_AUTO_REJOIN_TRIES,
    "@li autoRejoinTries: integer value to define the number of times an "
    "instance will attempt to rejoin the cluster after being expelled.");

REGISTER_HELP(CLUSTER_OPT_IP_WHITELIST,
              "@li ipWhitelist: The list of hosts allowed to connect to the "
              "instance for group replication. Deprecated.");
REGISTER_HELP(CLUSTER_OPT_IP_ALLOWLIST,
              "@li ipAllowlist: The list of hosts allowed to connect to the "
              "instance for group replication.");
REGISTER_HELP(CLUSTER_OPT_COMM_STACK,
              "@li communicationStack: The Group Replication protocol stack to "
              "be used in the Cluster: XCom (legacy) or MySQL.");
REGISTER_HELP(CLUSTER_OPT_LOCAL_ADDRESS,
              "@li localAddress: string value with the Group Replication local "
              "address to be used instead of the automatically generated one.");

REGISTER_HELP(
    OPT_INTERACTIVE,
    "@li interactive: boolean value used to disable/enable the wizards in the "
    "command execution, i.e. prompts and confirmations will be provided or not "
    "according to the value set. The default value is equal to MySQL Shell "
    "wizard mode.");

REGISTER_HELP(OPT_APPLIERWORKERTHREADS,
              "@li applierWorkerThreads: Number of threads used for applying "
              "replicated transactions. The default value is 4.");

REGISTER_HELP_DETAIL_TEXT(CLUSTER_OPT_EXIT_STATE_ACTION_DETAIL, R"*(
The exitStateAction option supports the following values:
@li ABORT_SERVER: if used, the instance shuts itself down if
it leaves the cluster unintentionally or exhausts its auto-rejoin attempts.
@li READ_ONLY: if used, the instance switches itself to
super-read-only mode if it leaves the cluster unintentionally or exhausts
its auto-rejoin attempts.
@li OFFLINE_MODE: if used, the instance switches itself to offline mode if
it leaves the cluster unintentionally or exhausts its auto-rejoin attempts.
Requires MySQL 8.0.18 or newer.

If exitStateAction is not specified READ_ONLY will be used by default.
)*");

REGISTER_HELP_DETAIL_TEXT(CLUSTER_OPT_EXIT_STATE_ACTION_EXTRA, R"*(
The value for exitStateAction is used to configure how Group
Replication behaves when a server instance leaves the group
unintentionally (for example after encountering an applier
error) or exhausts its auto-rejoin attempts.
When set to ABORT_SERVER, the instance shuts itself
down.
When set to READ_ONLY the server switches itself to
super-read-only mode.
When set to OFFLINE_MODE it switches itself to offline mode.
In this mode, connected client users are disconnected on their
next request and connections are no longer accepted, with the
exception of client users that have the CONNECTION_ADMIN or
SUPER privilege.
The exitStateAction option accepts case-insensitive string
values, being the accepted values: OFFLINE_MODE (or 2),
ABORT_SERVER (or 1) and READ_ONLY (or 0).

The default value is READ_ONLY.
)*");

REGISTER_HELP_DETAIL_TEXT(CLUSTER_OPT_MEMBER_WEIGHT_DETAIL_EXTRA, R"*(
The value for memberWeight is used to set the Group Replication
system variable 'group_replication_member_weight'. The
memberWeight option accepts integer values. Group Replication
limits the value range from 0 to 100, automatically adjusting
it if a lower/bigger value is provided.

Group Replication uses a default value of 50 if no value is provided.
)*");

REGISTER_HELP_DETAIL_TEXT(CLUSTER_OPT_CONSISTENCY_DETAIL, R"*(
The consistency option supports the following values:
@li BEFORE_ON_PRIMARY_FAILOVER: if used, new queries (read or
write) to the new primary will be put on hold until
after the backlog from the old primary is applied.
@li EVENTUAL: if used, read queries to the new primary are
allowed even if the backlog isn't applied.

If consistency is not specified, EVENTUAL will be used
by default.
)*");

REGISTER_HELP_DETAIL_TEXT(CLUSTER_OPT_CONSISTENCY_EXTRA, R"*(
The value for consistency is used to set the Group
Replication system variable 'group_replication_consistency' and
configure the transaction consistency guarantee which a cluster provides.

When set to to BEFORE_ON_PRIMARY_FAILOVER, whenever a primary failover happens
in single-primary mode (default), new queries (read or write) to the newly
elected primary that is applying backlog from the old primary, will be hold
before execution until the backlog is applied. When set to EVENTUAL, read
queries to the new primary are allowed even if the backlog isn't applied but
writes will fail (if the backlog isn't applied) due to super-read-only mode
being enabled. The client may return old values.

When set to BEFORE, each transaction (RW or RO) waits until all preceding
transactions are complete before starting its execution. This ensures that each
transaction is executed on the most up-to-date snapshot of the data, regardless
of which member it is executed on. The latency of the transaction is affected
but the overhead of synchronization on RW transactions is reduced since
synchronization is used only on RO transactions.

When set to AFTER, each RW transaction waits until its changes have been
applied on all of the other members. This ensures that once this transaction
completes, all following transactions read a database state that includes its
changes, regardless of which member they are executed on. This mode shall only
be used on a group that is used for predominantly RO operations to  to ensure
that subsequent reads fetch the latest data which includes the latest writes.
The overhead of synchronization on every RO transaction is reduced since
synchronization is used only on RW transactions.

When set to BEFORE_AND_AFTER, each RW transaction waits for all preceding
transactions to complete before being applied and until its changes have been
applied on other members. A RO transaction waits for all preceding transactions
to complete before execution takes place. This ensures the guarantees given by
BEFORE and by AFTER. The overhead of synchronization is higher.

The consistency option accepts case-insensitive string values, being the
accepted values: EVENTUAL (or 0), BEFORE_ON_PRIMARY_FAILOVER (or 1), BEFORE
(or 2), AFTER (or 3), and BEFORE_AND_AFTER (or 4).

The default value is EVENTUAL.
)*");

REGISTER_HELP_DETAIL_TEXT(CLUSTER_OPT_EXPEL_TIMEOUT_EXTRA, R"*(
The value for expelTimeout is used to set the Group Replication
system variable 'group_replication_member_expel_timeout' and
configure how long Group Replication will wait before expelling
from the group any members suspected of having failed. On slow
networks, or when there are expected machine slowdowns,
increase the value of this option. The expelTimeout option
accepts positive integer values and, since 8.0.21, defaults to 5
seconds.
)*");

REGISTER_HELP_DETAIL_TEXT(CLUSTER_OPT_AUTO_REJOIN_TRIES_EXTRA, R"*(
The value for autoRejoinTries is used to set the Group Replication system
variable 'group_replication_autorejoin_tries' and configure how many
times an instance will try to rejoin a Group Replication group after
being expelled. In scenarios where network glitches happen but recover
quickly, setting this option prevents users from having to manually add
the expelled node back to the group. The autoRejoinTries option accepts
positive integer values and, since 8.0.21, defaults to 3.
)*");

REGISTER_HELP_DETAIL_TEXT(CLUSTER_OPT_IP_ALLOWLIST_EXTRA, R"*(
The ipAllowlist format is a comma separated list of IP addresses or subnet CIDR
notation, for example: 192.168.1.0/24,10.0.0.1. By default the value is set to
AUTOMATIC, allowing addresses from the instance private network to be
automatically set for the allowlist.
)*");

REGISTER_HELP_DETAIL_TEXT(CLUSTER_OPT_COMM_STACK_EXTRA, R"*(
The value for communicationStack is used to choose which Group Replication
communication stack must be used in the Cluster. It's used to set the value
of the Group Replication system variable
'group_replication_communication_stack'.

When set to legacy 'XCom', all internal GCS network traffic (PAXOS and
communication infrastructure) flows through a separate network address: the
localAddress.

When set to 'MySQL', such traffic re-uses the existing MySQL Server facilities
to establish connections among Cluster members. It allows a simpler and safer
setup as it obsoletes the usage of IP allowlists (ipAllowlist), removes the
explicit need to have a separate network address (localAddress), and introduces
user-based authentication.

The default value for Clusters running 8.0.27+ is 'MySQL'.)*");

REGISTER_HELP_DETAIL_TEXT(CLUSTER_OPT_LOCAL_ADDRESS_EXTRA, R"*(
The value for localAddress is used to set the Group Replication system variable
'group_replication_local_address'. The localAddress option accepts values in
the format: 'host:port' or 'host:' or ':port'. If the specified value does not
include a colon (:) and it is numeric, then it is assumed to be the port,
otherwise it is considered to be the host. When the host is not specified, the
default value is the value of the system variable 'report_host' if defined
(i.e., not 'NULL'), otherwise it is the hostname value. When the port is not
specified, the default value is the port of the target instance if the
communication stack in use by the Cluster is 'MYSQL', otherwise, port * 10 + 1
 when the communication stack is 'XCOM'. In case the automatically determined
 default port value is invalid (> 65535) then an error is thrown.)*");

// TODO create a dedicated topic for InnoDB clusters and replicasets,
// with a quick tutorial for both, in addition to more deep technical info

// Documentation of the DBA Class
REGISTER_HELP_TOPIC(AdminAPI, CATEGORY, adminapi, Contents, SCRIPTING);
REGISTER_HELP_TOPIC_WITH_BRIEF_TEXT(ADMINAPI, R"*(
The <b>AdminAPI</b> is an API that enables configuring and managing InnoDB
Clusters, ReplicaSets, ClusterSets, among other things.

The AdminAPI can be used interactively from the MySQL Shell prompt and
non-interactively from JavaScript and Python scripts and directly from the
command line.

For more information about the <b>dba</b> object use: \\? dba

In the AdminAPI, an InnoDB Cluster is represented as an instance of the
<b>Cluster</b> class, while ReplicaSets are represented as an instance
of the <b>ReplicaSet</b> class, and ClusterSets are represented as an
instance of the <b>ClusterSet</b> class.

For more information about the <b>Cluster</b> class use: \\? Cluster

For more information about the <b>ClusterSet</b> class use: \\? ClusterSet

For more information about the <b>ReplicaSet</b> class use: \\? ReplicaSet

<b>Scripting</b>

Through the JavaScript and Python bindings of the MySQL Shell, the AdminAPI can
be used from scripts, which can in turn be used interactively or
non-interactively. To execute a script, use the -f command line option, followed
by the script file path. Options that follow the path are passed directly
to the script being executed, which can access them from sys.argv

@code
    mysqlsh root@localhost -f myscript.py arg1 arg2
@endcode

If the script finishes successfully, the Shell will exit with code 0, while
uncaught exceptions/errors cause it to exist with a non-0 code.

By default, the AdminAPI enables interactivity, which will cause functions to
prompt for missing passwords, confirmations and bits of information that cannot
be obtained automatically.

Prompts can be completely disabled with the --no-wizard command line option
or using the "interactive" boolean option available in some of functions.
If interactivity is disabled and some information is missing (e.g. a password),
an error will be raised instead of being prompted.

<b>Secure Password Handling</b>

Passwords can be safely stored locally, using the system's native secrets
storage functionality (or login-paths in Linux). Whenever the Shell needs
a password, it will first query for the password in the system, before
prompting for it.

Passwords can be stored during interactive use, by confirming in the Store
Password prompt. They can also be stored programmatically, using the
shell.<<<storeCredential>>>() function.

You can also use environment variables to pass information to your scripts.
In JavaScript, the os.getenv() function can be used to access them.

<b>Command Line Interface</b>

In addition to the scripting interface, the MySQL Shell supports generic
command line integration, which allows calling AdminAPI functions directly
from the system shell (e.g. bash). Examples:

@code
    $ mysqlsh -- dba configure-instance root@localhost

    is equivalent to:

    > dba.configureInstance("root@localhost")

    $ mysqlsh root@localhost -- cluster status --extended

    is equivalent to:

    > dba.getCluster().status({extended:true})
@endcode

The mapping from AdminAPI function signatures works as follows:

@li The first argument after a -- can be a shell global object, such as dba.
As a special case, cluster and rs are also accepted.
@li The second argument is the name of the function of the object to be called.
The naming convention is automatically converted from camelCase/snake_case
to lower case separated by dashes.
@li The rest of the arguments are used in the same order as their JS/Python
counterparts. Instances can be given as URIs. Option dictionaries can be passed
as --options, where the option name is the same as in JS/Python.
)*");

REGISTER_HELP_GLOBAL_OBJECT(dba, adminapi);
REGISTER_HELP(
    DBA_BRIEF,
    "InnoDB Cluster, ReplicaSet, and ClusterSet management functions.");
REGISTER_HELP(
    DBA_GLOBAL_BRIEF,
    "Used for InnoDB Cluster, ReplicaSet, and ClusterSet administration.");

REGISTER_HELP_TOPIC_TEXT(DBA, R"*(
Entry point for AdminAPI functions, including InnoDB Clusters, ReplicaSets, and
ClusterSets.

<b>InnoDB Clusters</b>

The dba.<<<configureInstance>>>() function can be used to configure a MySQL
instance with the settings required to use it in an InnoDB Cluster.

InnoDB Clusters can be created with the dba.<<<createCluster>>>() function.

Once created, InnoDB Cluster management objects can be obtained with the
dba.<<<getCluster>>>() function.

<b>InnoDB ReplicaSets</b>

The dba.<<<configureReplicaSetInstance>>>() function can be used to configure a
MySQL instance with the settings required to use it in a ReplicaSet.

ReplicaSets can be created with the dba.<<<createReplicaSet>>>()
function.

Once created, ReplicaSet management objects can be obtained with the
dba.<<<getReplicaSet>>>() function.

<b>InnoDB ClusterSets</b>

ClusterSets can be created with the <Cluster>.<<<createClusterSet>>>()
function.

Once created, ClusterSet management objected can be obtained with the
dba.<<<getClusterSet>>>() or <Cluster>.<<<getClusterSet>>>() functions.

<b>Sandboxes</b>

Utility functions are provided to create sandbox MySQL instances, which can
be used to create test Clusters and ReplicaSets.
)*");

REGISTER_HELP(DBA_CLOSING, "<b>SEE ALSO</b>");
REGISTER_HELP(
    DBA_CLOSING1,
    "@li For general information about the AdminAPI use: \\? AdminAPI");
REGISTER_HELP(
    DBA_CLOSING2,
    "@li For help on a specific function use: \\? dba.<functionName>");
REGISTER_HELP(DBA_CLOSING3, "e.g. \\? dba.<<<deploySandboxInstance>>>");

Dba::Dba(shcore::IShell_core *owner) : _shell_core(owner) { init(); }

Dba::Dba(const std::shared_ptr<ShellBaseSession> &session)
    : m_session(session) {
  init();
}

Dba::~Dba() {}

bool Dba::operator==(const Object_bridge &other) const {
  return class_name() == other.class_name() && this == &other;
}

void Dba::init() {
  add_property("verbose");
  add_property("session");

  // Pure functions
  expose("createCluster", &Dba::create_cluster, "name", "?options")->cli();

  expose("getCluster", &Dba::get_cluster, "?name", "?options")->cli(false);
  expose("dropMetadataSchema", &Dba::drop_metadata_schema, "?options")->cli();
  expose("checkInstanceConfiguration", &Dba::check_instance_configuration,
         "?instance", "?options")
      ->cli();
  expose("deploySandboxInstance", &Dba::deploy_sandbox_instance, "port",
         "?options")
      ->cli();
  expose("startSandboxInstance", &Dba::start_sandbox_instance, "port",
         "?options")
      ->cli();
  expose("stopSandboxInstance", &Dba::stop_sandbox_instance, "port", "?options")
      ->cli();
  expose("deleteSandboxInstance", &Dba::delete_sandbox_instance, "port",
         "?options")
      ->cli();
  expose("killSandboxInstance", &Dba::kill_sandbox_instance, "port", "?options")
      ->cli();
  expose("rebootClusterFromCompleteOutage",
         &Dba::reboot_cluster_from_complete_outage, "?clusterName", "?options")
      ->cli();
  expose("upgradeMetadata", &Dba::upgrade_metadata, "?options")->cli();
  expose("configureLocalInstance", &Dba::configure_local_instance, "?instance",
         "?options")
      ->cli();
  expose("configureInstance", &Dba::configure_instance, "?instance", "?options")
      ->cli();
  expose("configureReplicaSetInstance", &Dba::configure_replica_set_instance,
         "?instance", "?options")
      ->cli();
  expose("createReplicaSet", &Dba::create_replica_set, "name", "?options")
      ->cli();
  expose("getReplicaSet", &Dba::get_replica_set)->cli(false);
  expose("getClusterSet", &Dba::get_cluster_set)->cli(false);
}

void Dba::set_member(const std::string &prop, shcore::Value value) {
  if (prop == "verbose") {
    try {
      int verbosity = value.as_int();
      _provisioning_interface.set_verbose(verbosity);
    } catch (const shcore::Exception &e) {
      throw shcore::Exception::value_error(
          "Invalid value for property 'verbose', use either boolean or integer "
          "value.");
    }
  } else {
    Cpp_object_bridge::set_member(prop, value);
  }
}

REGISTER_HELP_PROPERTY(verbose, dba);
REGISTER_HELP_PROPERTY_TEXT(DBA_VERBOSE, R"*(
Controls debug message verbosity for sandbox related <b>dba</b> operations.

The assigned value can be either boolean or integer, the result
depends on the assigned value:
@li 0: disables mysqlprovision verbosity
@li 1: enables mysqlprovision verbosity
@li >1: enables mysqlprovision debug verbosity
@li Boolean: equivalent to assign either 0 or 1
)*");

/**
 * $(DBA_VERBOSE_BRIEF)
 *
 * $(DBA_VERBOSE)
 */
#if DOXYGEN_JS || DOXYGEN_PY
int Dba::verbose;
#endif

REGISTER_HELP_PROPERTY(session, dba);
REGISTER_HELP_PROPERTY_TEXT(DBA_SESSION, R"*(
The session the dba object will use by default.

Reference to the MySQL session that will be used as the default target for
AdminAPI operations such as <<<getCluster>>>() or <<<createCluster>>>().

This is a read-only property.
)*");
/**
 * $(DBA_SESSION_BRIEF)
 *
 * $(DBA_SESSION)
 */
#if DOXYGEN_JS || DOXYGEN_PY
Session Dba::session;
#endif
shcore::Value Dba::get_member(const std::string &prop) const {
  shcore::Value ret_val;

  if (prop == "verbose") {
    ret_val = shcore::Value(_provisioning_interface.get_verbose());
  } else if (prop == "session") {
    if (m_session) {
      ret_val = shcore::Value(m_session);
    } else if (_shell_core && _shell_core->get_dev_session()) {
      ret_val = shcore::Value(_shell_core->get_dev_session());
    }
  } else {
    ret_val = Cpp_object_bridge::get_member(prop);
  }

  return ret_val;
}

constexpr const char INNODB_SSH_NOT_SUPPORTED[] =
    "InnoDB cluster functionality is not available through SSH tunneling";

/*
  Open a classic session to the instance the shell is connected to.
  If the shell's session is X protocol, it will query it for the classic
  port and connect to it.
 */
std::shared_ptr<Instance> Dba::connect_to_target_member() const {
  DBUG_TRACE;

  auto active_shell_session = get_active_shell_session();
  if (active_shell_session) {
    if (active_shell_session->get_connection_options().has_ssh_options()) {
      throw shcore::Exception::logic_error(INNODB_SSH_NOT_SUPPORTED);
    }
    return Instance::connect(
        get_classic_connection_options(active_shell_session), false);
  }

  return {};
}

/*
   Create a metadata object and a MySQL session for managing the InnoDB cluster
   which the shell's active session target is part of.

   If a target_member_session is given, it will be reused for the metadata
   and group sessions if possible. Note that the target_member_session must
   not be the shell's global session.
 */
void Dba::connect_to_target_group(
    std::shared_ptr<Instance> target_member,
    std::shared_ptr<MetadataStorage> *out_metadata,
    std::shared_ptr<Instance> *out_group_server,
    bool connect_to_primary) const {
  bool owns_target_member_session = false;
  if (!target_member) {
    target_member = connect_to_target_member();
    owns_target_member_session = true;
  } else if (target_member->get_connection_options().has_ssh_options()) {
    throw shcore::Exception::logic_error(INNODB_SSH_NOT_SUPPORTED);
  }
  if (!target_member) {
    throw shcore::Exception::logic_error(
        "The shell must be connected to a member of the InnoDB cluster being "
        "managed");
  }

  if (connect_to_primary) {
    std::string primary_uri = find_primary_member_uri(target_member, false);

    if (primary_uri.empty()) {
      throw shcore::Exception::runtime_error(
          "Unable to find a primary member in the cluster");
    } else if (!mysqlshdk::utils::are_endpoints_equal(
                   primary_uri,
                   target_member->get_connection_options().uri_endpoint())) {
      log_info("%s is not a primary, will try to find one and reconnect",
               target_member->get_connection_options().as_uri().c_str());

      try {
        mysqlshdk::db::Connection_options coptions(primary_uri);

        coptions.set_login_options_from(
            target_member->get_connection_options());

        log_info("Opening session to primary of InnoDB cluster at %s...",
                 coptions.as_uri().c_str());

        auto instance = Instance::connect(coptions);
        mysqlshdk::gr::Member_state state =
            mysqlshdk::gr::get_member_state(*instance);

        // Check whether the member we connected to is really ONLINE. If e.g.
        // the PRIMARY restarted, it will not be expelled from the group until
        // some time has passed.
        if (state != mysqlshdk::gr::Member_state::ONLINE) {
          log_warning("PRIMARY member %s is currently in state %s",
                      instance->descr().c_str(),
                      mysqlshdk::gr::to_string(state).c_str());
          throw shcore::Exception("Group PRIMARY is not ONLINE",
                                  SHERR_DBA_GROUP_MEMBER_NOT_ONLINE);
        }

        log_info(
            "Metadata and group sessions are now using the primary member");
        if (owns_target_member_session) target_member->close_session();
        target_member = instance;
      } catch (const shcore::Exception &) {
        throw;
      } catch (const std::exception &e) {
        throw shcore::Exception::runtime_error(
            std::string("Unable to find a primary member in the cluster: ") +
            e.what());
      }
    }
  }

  if (out_group_server) *out_group_server = target_member;

  // Check if the metadataStorage doesn't have an open session and create it if
  // necessary
  if (out_metadata &&
      (!out_metadata->get() || !out_metadata->get()->is_valid())) {
    *out_metadata = std::make_shared<MetadataStorage>(target_member);
  }

  // Check if we need to reset the MetadataStorage to use the Primary member
  bool is_md_connected_to_primary = mysqlshdk::utils::are_endpoints_equal(
      out_metadata->get()->get_md_server()->get_connection_options().as_uri(),
      target_member->get_connection_options().as_uri());

  if (connect_to_primary && !is_md_connected_to_primary) {
    *out_metadata = std::make_shared<MetadataStorage>(target_member);
  }

  // Check if the target instance has MD schema
  mysqlshdk::utils::Version md_version;
  bool has_md_schema = out_metadata->get()->check_version(&md_version);

  // Check if the target member belongs to a ClusterSet and get the connection
  // to the primary instance of the primary cluster
  if (out_metadata && has_md_schema &&
      md_version >= mysqlshdk::utils::Version(2, 1, 0)) {
    Cluster_metadata cluster_md;
    out_metadata->get()->get_cluster_for_server_uuid(target_member->get_uuid(),
                                                     &cluster_md);

    // IF the cluster is part of a ClusterSet
    // Get the primary cluster
    if (!cluster_md.cluster_set_id.empty() && connect_to_primary) {
      std::vector<Cluster_set_member_metadata> cs_members;
      Cluster_metadata primary_cluster;
      if (!out_metadata->get()->get_cluster_set(cluster_md.cluster_set_id,
                                                false, nullptr, &cs_members)) {
        throw std::runtime_error("Metadata not found");
      }

      for (const auto &m : cs_members) {
        if (m.primary_cluster) {
          primary_cluster = m.cluster;
          break;
        }
      }

      // Check if the MD server we're connected to is the PRIMARY Cluster.
      // If not, we need to establish a session to the primary member of the
      // PRIMARY cluster
      if (out_metadata->get()->get_md_server()->get_group_name() !=
          primary_cluster.group_name) {
        Scoped_instance_pool ipool(
            *out_metadata, current_shell_options()->get().wizards,
            Instance_pool::Auth_options(
                target_member->get_connection_options()));

        try {
          std::shared_ptr<Instance> primary_instance(
              ipool->connect_group_primary(primary_cluster.group_name));

          primary_instance->steal();

          out_metadata->reset(new MetadataStorage(primary_instance));
        } catch (const shcore::Exception &e) {
          auto console = current_console();
          log_error(
              "Could not connect to any member of the PRIMARY Cluster: %s",
              e.what());

          if (e.code() == SHERR_DBA_GROUP_HAS_NO_PRIMARY ||
              e.code() == SHERR_DBA_GROUP_REPLICATION_NOT_RUNNING) {
            console->print_warning(
                "Could not connect to any member of PRIMARY Cluster '" +
                primary_cluster.cluster_name +
                "', topology changes will not be allowed on the InnoDB Cluster "
                "'" +
                cluster_md.cluster_name + "'");
          } else if (e.code() == SHERR_DBA_GROUP_HAS_NO_QUORUM) {
            console->print_warning("The PRIMARY Cluster '" +
                                   primary_cluster.cluster_name +
                                   "' lost the quorum, topology changes will "
                                   "not be allowed on the InnoDB Cluster "
                                   "'" +
                                   cluster_md.cluster_name + "'");
            console->print_note(
                "To restore the Cluster and ClusterSet operations, restore the "
                "quorum on the PRIMARY Cluster using "
                "<<<forceQuorumUsingPartitionOf>>>()");
          } else {
            throw;
          }
        } catch (const mysqlshdk::db::Error &) {
          throw;
        }
      }
    }
  }
}

std::shared_ptr<mysqlshdk::db::ISession> Dba::get_active_shell_session() const {
  if (m_session) return m_session->get_core_session();

  if (_shell_core && _shell_core->get_dev_session())
    return _shell_core->get_dev_session()->get_core_session();
  return {};
}

// Documentation of the getCluster function
REGISTER_HELP_FUNCTION(getCluster, dba);
REGISTER_HELP_FUNCTION_TEXT(DBA_GETCLUSTER, R"*(
Retrieves a cluster from the Metadata Store.

@param name Optional parameter to specify the name of the cluster to be returned.
@param options Optional dictionary with additional options.

@returns The cluster object identified by the given name or the default cluster.

If name is not specified or is null, the default cluster will be returned.

If name is specified, and no cluster with the indicated name is found, an
error will be raised.

The options dictionary accepts the connectToPrimary option,
which defaults to true and indicates the shell to automatically
connect to the primary member of the cluster.
)*");
/**
 * $(DBA_GETCLUSTER_BRIEF)
 *
 * $(DBA_GETCLUSTER)
 */
#if DOXYGEN_JS
Cluster Dba::getCluster(String name, Dictionary options) {}
#elif DOXYGEN_PY
Cluster Dba::get_cluster(str name, dict options) {}
#endif
std::shared_ptr<Cluster> Dba::get_cluster(
    const mysqlshdk::null_string &cluster_name,
    const shcore::Dictionary_t &options) const {
  std::shared_ptr<Instance> target_member, group_server;
  std::shared_ptr<MetadataStorage> metadata;

  auto console = mysqlsh::current_console();

  try {
    bool connect_to_primary = true;
    bool fallback_to_anything = true;

    target_member = connect_to_target_member();

    if (target_member) {
      metadata = std::make_shared<MetadataStorage>(target_member);
    }

    check_function_preconditions("Dba.getCluster", metadata, target_member);

    if (options) {
      Unpack_options(options)
          .optional("connectToPrimary", &connect_to_primary)
          .end();

      if (options->has_key("connectToPrimary")) {
        fallback_to_anything = false;
      }
    }

    // This will throw if not a cluster member
    try {
      // Connect to the target cluster member and
      // also find the primary and connect to it, unless target is already
      // primary or connectToPrimary:false was given
      connect_to_target_group(target_member, &metadata, &group_server,
                              connect_to_primary);
    } catch (const shcore::Exception &e) {
      // Print warning in case a cluster error is found (e.g., no quorum).
      if (e.code() == SHERR_DBA_GROUP_HAS_NO_QUORUM) {
        console->print_warning(
            "Cluster has no quorum and cannot process write transactions: " +
            std::string(e.what()));
      } else {
        console->print_warning("Cluster error connecting to target: " +
                               e.format());
      }

      if ((e.code() == SHERR_DBA_GROUP_HAS_NO_QUORUM ||
           e.code() == SHERR_DBA_GROUP_MEMBER_NOT_ONLINE) &&
          fallback_to_anything && connect_to_primary) {
        log_info("Retrying getCluster() without connectToPrimary");
        connect_to_target_group({}, &metadata, &group_server, false);
      } else {
        throw;
      }
    }

    Scoped_instance_pool ipool(
        metadata, current_shell_options()->get().wizards,
        Instance_pool::Auth_options(group_server->get_connection_options()));

    if (current_shell_options()->get().wizards) {
      auto state = get_cluster_check_info(*metadata);
      if (state.source_state == mysqlsh::dba::ManagedInstance::OnlineRO) {
        console->print_info(
            "WARNING: You are connected to an instance in state '" +
            mysqlsh::dba::ManagedInstance::describe(
                static_cast<mysqlsh::dba::ManagedInstance::State>(
                    state.source_state)) +
            "'\n"
            "Write operations on the InnoDB cluster will not be allowed.\n");
      } else if (state.source_state !=
                 mysqlsh::dba::ManagedInstance::OnlineRW) {
        console->print_info(
            "WARNING: You are connected to an instance in state '" +
            mysqlsh::dba::ManagedInstance::describe(
                static_cast<mysqlsh::dba::ManagedInstance::State>(
                    state.source_state)) +
            "'\n"
            "Write operations on the InnoDB cluster will not be allowed.\n"
            "Output from describe() and status() may be outdated.\n");
      }
    }

    auto cluster = get_cluster(!cluster_name ? nullptr : cluster_name->c_str(),
                               metadata, group_server, false, true);

    // Check if the target Cluster is invalidated in a clusterset
    if (cluster->impl()->is_cluster_set_member())
      cluster->impl()->check_and_get_cluster_set_for_cluster();

    return cluster;
  } catch (const shcore::Exception &e) {
    if (e.code() == SHERR_DBA_GROUP_HAS_NO_QUORUM) {
      throw shcore::Exception::runtime_error(
          get_function_name("getCluster") +
          ": Unable to find a cluster PRIMARY member from the active shell "
          "session because the cluster has too many UNREACHABLE members and "
          "no quorum is possible.\n"
          "Use " +
          get_function_name("getCluster") +
          "(null, {connectToPrimary:false}) to get a read-only cluster "
          "handle.");
    } else if (e.code() == SHERR_DBA_METADATA_MISSING) {
      // The Metadata does not contain this Cluster, however, the preconditions
      // checker verified the existence of the Cluster in the Metadata schema of
      // the current Shell session meaning:
      //
      //   - The Cluster is ONLINE
      //   - The Cluster belongs to a ClusterSet, however it's not the valid
      //   PRIMARY Cluster
      //   - The Cluster's Metadata is not the most up-to-date Metadata copy of
      //   the ClusterSet hence it does not exist in that Metadata copy
      //
      // This means the Cluster has been removed from the ClusterSet Metadata so
      // we must check if the Cluster is part of a ClusterSet to print a
      // warning and obtain the Cluster object from its own Metadata.
      // If not, we must error out right away.
      if (target_member) {
        std::shared_ptr<MetadataStorage> metadata_at_target =
            std::make_shared<MetadataStorage>(target_member);

        auto cluster =
            get_cluster(!cluster_name ? nullptr : cluster_name->c_str(),
                        metadata_at_target, group_server);

        std::string cs_domain_name;
        // The correct MD thinks this cluster is a member, check if it thinks
        // it's a member itself
        if (metadata_at_target->check_cluster_set(target_member.get(), nullptr,
                                                  &cs_domain_name)) {
          console->print_warning(
              "The Cluster '" + cluster->impl()->get_name() +
              "' appears to have been removed from the ClusterSet '" +
              cs_domain_name +
              "', however its own metadata copy wasn't properly updated during "
              "the removal");

          // Mark that the cluster was already removed from the clusterset but
          // doesn't know about it yet
          cluster->impl()->set_cluster_set_remove_pending(true);

          return cluster;
        }
      }
    } else if (e.code() == SHERR_DBA_BADARG_INSTANCE_MANAGED_IN_REPLICASET) {
      // we can inform to the user with an hint (the user called getCluster on a
      // replicat set)
      console->print_info(
          "No InnoDB Cluster found, did you meant to call "
          "dba.<<<getReplicaSet>>>()?");
      throw e;
    }

    throw;
  }
}

std::shared_ptr<Cluster> Dba::get_cluster(
    const char *name, std::shared_ptr<MetadataStorage> metadata,
    std::shared_ptr<Instance> group_server, bool reboot_cluster,
    bool allow_invalidated) const {
  Cluster_metadata target_cm;
  Cluster_metadata group_server_cm;

  if (!metadata->get_cluster_for_server_uuid(group_server->get_uuid(),
                                             &group_server_cm)) {
    throw shcore::Exception("No cluster metadata found for " +
                                group_server->get_uuid() + " of instance " +
                                group_server->descr(),
                            SHERR_DBA_METADATA_MISSING);
  }

  if (!name) {
    target_cm = group_server_cm;
  } else {
    if (!metadata->get_cluster_for_cluster_name(name, &target_cm,
                                                allow_invalidated)) {
      throw shcore::Exception(
          shcore::str_format("The cluster with the name '%s' does not exist.",
                             name),
          SHERR_DBA_METADATA_MISSING);
    }
  }

  // check whether the instance we're connected to is a member of the named
  // group, otherwise connect to one
  if ((target_cm.group_name != group_server->get_group_name()) &&
      !reboot_cluster) {
    log_debug(
        "Target instance '%s' not a member of '%s', opening new connection...",
        group_server->descr().c_str(), target_cm.cluster_name.c_str());

    auto ipool = current_ipool();
    group_server = ipool->connect_group_member(target_cm.group_name);
    metadata = std::make_shared<MetadataStorage>(group_server);
    group_server->steal();
  }

  auto cluster = std::make_shared<Cluster_impl>(
      target_cm, group_server, metadata, Cluster_availability::ONLINE);

  // Verify if the current session instance group_replication_group_name
  // value differs from the one registered in the Metadata for that instance
  if (!validate_cluster_group_name(*group_server, cluster->get_group_name())) {
    std::string nice_error =
        "Unable to get an InnoDB cluster handle. "
        "The instance '" +
        group_server->descr() +
        "' may belong to a different cluster from the one registered in the "
        "Metadata since the value of 'group_replication_group_name' does "
        "not match the one registered in the Metadata: possible split-brain "
        "scenario. Please retry while connected to another member of the "
        "cluster.";

    throw shcore::Exception::runtime_error(nice_error);
  }

  return std::make_shared<mysqlsh::dba::Cluster>(cluster);
}

REGISTER_HELP_FUNCTION(createCluster, dba);
REGISTER_HELP_FUNCTION_TEXT(DBA_CREATECLUSTER, R"*(
Creates a MySQL InnoDB cluster.

@param name An identifier for the cluster to be created.
@param options Optional dictionary with additional parameters described below.

@returns The created cluster object.

Creates a MySQL InnoDB cluster taking as seed instance the server the shell
is currently connected to.

The options dictionary can contain the following values:

@li disableClone: boolean value used to disable the clone usage on the cluster.
@li gtidSetIsComplete: boolean value which indicates whether the GTID set
of the seed instance corresponds to all transactions executed. Default is false.
@li multiPrimary: boolean value used to define an InnoDB cluster with multiple
writable instances.
@li force: boolean, confirms that the multiPrimary option must be applied
and/or the operation must proceed even if unmanaged replication channels
were detected.
${OPT_INTERACTIVE}
@li adoptFromGR: boolean value used to create the InnoDB cluster based on
existing replication group.
@li memberSslMode: SSL mode used to configure the members of the cluster.
${CLUSTER_OPT_IP_WHITELIST}
${CLUSTER_OPT_IP_ALLOWLIST}
@li groupName: string value with the Group Replication group name UUID to be
used instead of the automatically generated one.
${CLUSTER_OPT_LOCAL_ADDRESS}
@li groupSeeds: string value with a comma-separated list of the Group
Replication peer addresses to be used instead of the automatically generated
one. Deprecated and ignored.
@li manualStartOnBoot: boolean (default false). If false, Group Replication in
cluster instances will automatically start and rejoin when MySQL starts,
otherwise it must be started manually.
@li replicationAllowedHost: string value to use as the host name part of
internal replication accounts (i.e. 'mysql_innodb_cluster_###'@'hostname').
Default is %. It must be possible for any member of the Cluster to connect to
any other member using accounts with this hostname value.
${CLUSTER_OPT_EXIT_STATE_ACTION}
${CLUSTER_OPT_MEMBER_WEIGHT}
${CLUSTER_OPT_CONSISTENCY}
${CLUSTER_OPT_FAILOVER_CONSISTENCY}
${CLUSTER_OPT_EXPEL_TIMEOUT}
${CLUSTER_OPT_AUTO_REJOIN_TRIES}
@li clearReadOnly: boolean value used to confirm that super_read_only must be
disabled. Deprecated.
@li multiMaster: boolean value used to define an InnoDB cluster with multiple
writable instances. Deprecated.
${CLUSTER_OPT_COMM_STACK}

An InnoDB cluster may be setup in two ways:

@li Single Primary: One member of the cluster allows write operations while the
rest are read-only secondaries.
@li Multi Primary: All the members in the cluster allow both read and write
operations.

Note that Multi-Primary mode has limitations about what can be safely executed.
Make sure to read the MySQL documentation for Group Replication and be aware of
what is and is not safely executable in such setups.

By default this function creates a Single Primary cluster. Use the multiPrimary
option set to true if a Multi Primary cluster is required.

<b>Options</b>

interactive controls whether prompts are shown for MySQL passwords,
confirmations and handling of cases where user feedback may be required.
Defaults to true, unless the Shell is started with the --no-wizards option.

disableClone should be set to true if built-in clone support should be
completely disabled, even in instances where that is supported. Built-in clone
support is available starting with MySQL 8.0.17 and allows automatically
provisioning new cluster members by copying state from an existing cluster
member. Note that clone will completely delete all data in the instance being
added to the cluster.

gtidSetIsComplete is used to indicate that GTIDs have been always enabled
at the cluster seed instance and that GTID_EXECUTED contains all transactions
ever executed. It must be left as false if data was inserted or modified while
GTIDs were disabled or if RESET MASTER was executed. This flag affects how
cluster.<<<addInstance>>>() decides which recovery methods are safe to use.
Distributed recovery based on replaying the transaction history is only assumed
to be safe if the transaction history is known to be complete, otherwise
cluster members could end up with incomplete data sets.

adoptFromGR allows creating an InnoDB cluster from an existing unmanaged
Group Replication setup, enabling use of MySQL Router and the shell AdminAPI
for managing it.

The memberSslMode option supports the following values:

@li REQUIRED: if used, SSL (encryption) will be enabled for the instances to
communicate with other members of the cluster
@li VERIFY_CA: Like REQUIRED, but additionally verify the server TLS
certificate against the configured Certificate Authority (CA) certificates.
@li VERIFY_IDENTITY: Like VERIFY_CA, but additionally verify that the server
certificate matches the host to which the connection is attempted.
@li DISABLED: if used, SSL (encryption) will be disabled
@li AUTO: if used, SSL (encryption) will be enabled if supported by the
instance, otherwise disabled

If memberSslMode is not specified AUTO will be used by default.

${CLUSTER_OPT_IP_ALLOWLIST_EXTRA}

The groupName and localAddress are advanced options and their usage is
discouraged since incorrect values can lead to Group Replication errors.

The value for groupName is used to set the Group Replication system variable
'group_replication_group_name'.

${CLUSTER_OPT_LOCAL_ADDRESS_EXTRA}

The groupSeeds option is deprecated as of MySQL Shell 8.0.28 and is ignored.
'group_replication_group_seeds' is automatically set based on the current
topology.

${CLUSTER_OPT_EXIT_STATE_ACTION_DETAIL}

${CLUSTER_OPT_CONSISTENCY_DETAIL}

${CLUSTER_OPT_EXIT_STATE_ACTION_EXTRA}

${CLUSTER_OPT_MEMBER_WEIGHT_DETAIL_EXTRA}

${CLUSTER_OPT_CONSISTENCY_EXTRA}

${CLUSTER_OPT_EXPEL_TIMEOUT_EXTRA}

${CLUSTER_OPT_AUTO_REJOIN_TRIES_EXTRA}

${CLUSTER_OPT_COMM_STACK_EXTRA}

@attention The clearReadOnly option will be removed in a future release.

@attention The multiMaster option will be removed in a future release. Please
use the multiPrimary option instead.

@attention The failoverConsistency option will be removed in a future release.
Please use the consistency option instead.

@attention The ipWhitelist option will be removed in a future release.
Please use the ipAllowlist option instead.

@attention The groupSeeds option will be removed in a future release.
)*");

/**
 * $(DBA_CREATECLUSTER_BRIEF)
 *
 * $(DBA_CREATECLUSTER)
 */

#if DOXYGEN_JS
Cluster Dba::createCluster(String name, Dictionary options) {}
#elif DOXYGEN_PY
Cluster Dba::create_cluster(str name, dict options) {}
#endif
shcore::Value Dba::create_cluster(
    const std::string &cluster_name,
    const shcore::Option_pack_ref<Create_cluster_options> &options) {
  std::string instance_label;

  std::shared_ptr<Instance> group_server;
  std::shared_ptr<MetadataStorage> metadata;

  // We're in createCluster(), so there's no metadata yet, but the metadata
  // object can already exist now to hold a session
  // connect_to_primary must be false, because we don't have a cluster yet
  // and this will be the seed instance anyway. If we start storing metadata
  // outside the managed cluster, then this will have to be updated.
  connect_to_target_group({}, &metadata, &group_server, false);

  // Check preconditions.
  Cluster_check_info state;
  try {
    state = check_function_preconditions("Dba.createCluster", metadata,
                                         group_server);
  } catch (const shcore::Exception &e) {
    if (e.code() == SHERR_DBA_BADARG_INSTANCE_MANAGED_IN_CLUSTER) {
      /*
       * For V1.0 we only support one single Cluster. That one shall be the
       * default Cluster. We must check if there's already a Default Cluster
       * assigned, and if so thrown an exception. And we must check if there's
       * already one Cluster on the MD and if so assign it to Default
       */
      std::string nice_error =
          get_function_name("createCluster") +
          ": Unable to create cluster. The instance '" + group_server->descr() +
          "' already belongs to an InnoDB cluster. Use "
          "dba." +
          get_function_name("getCluster", false) + "() to access it.";
      throw shcore::Exception::runtime_error(nice_error);
    } else if (e.code() == SHERR_DBA_BADARG_INSTANCE_NOT_ONLINE) {
      // BUG#29271400:
      // createCluster() should not be allowed in instance with metadata
      std::string nice_error =
          "dba.<<<createCluster>>>: Unable to create cluster. The instance '" +
          group_server->descr() +
          "' has a populated Metadata schema and belongs to that Metadata. Use "
          "either dba.<<<dropMetadataSchema>>>() to drop the schema, or "
          "dba.<<<rebootClusterFromCompleteOutage>>>() to reboot the cluster "
          "from complete outage.";
      throw shcore::Exception::runtime_error(shcore::str_subvars(nice_error));
    } else {
      throw;
    }
  } catch (const shcore::Error &dberr) {
    throw shcore::Exception::mysql_error_with_code(dberr.what(), dberr.code());
  }

  // Create the cluster
  {
    // Create the add_instance command and execute it.
    Create_cluster op_create_cluster(group_server, cluster_name, *options);

    // Always execute finish when leaving "try catch".
    auto finally = shcore::on_leave_scope(
        [&op_create_cluster]() { op_create_cluster.finish(); });

    // Prepare the add_instance command execution (validations).
    op_create_cluster.prepare();

    // Execute add_instance operations.
    return op_create_cluster.execute();
  }
}

REGISTER_HELP_FUNCTION(dropMetadataSchema, dba);
REGISTER_HELP_FUNCTION_TEXT(DBA_DROPMETADATASCHEMA, R"*(
Drops the Metadata Schema.

@param options Dictionary containing an option to confirm the drop operation.

@returns Nothing.

The options dictionary may contain the following options:

@li force: boolean, confirms that the drop operation must be executed.
@li clearReadOnly: boolean value used to confirm that super_read_only must be
disabled
)*");

/**
 * $(DBA_DROPMETADATASCHEMA_BRIEF)
 *
 * $(DBA_DROPMETADATASCHEMA)
 */
#if DOXYGEN_JS
Undefined Dba::dropMetadataSchema(Dictionary options) {}
#elif DOXYGEN_PY
None Dba::drop_metadata_schema(dict options) {}
#endif

void Dba::drop_metadata_schema(
    const shcore::Option_pack_ref<Drop_metadata_schema_options> &options) {
  auto instance = connect_to_target_member();
  std::shared_ptr<MetadataStorage> metadata;

  if (instance) {
    metadata = std::make_shared<MetadataStorage>(instance);
  }

  auto state = check_function_preconditions("Dba.dropMetadataSchema", metadata,
                                            instance);

  bool interactive = current_shell_options()->get().wizards;

  // The drop operation is only allowed on members of an InnoDB Cluster or a
  // Replica Set, and only for online members that are either RW or RO, however
  // if it is RO we need to get to a primary instance to perform the operation
  if (state.source_state == ManagedInstance::OnlineRO) {
    if (state.source_type == TargetType::InnoDBCluster) {
      connect_to_target_group({}, &metadata, nullptr, true);
    } else {
      Scoped_instance_pool ipool(
          metadata, interactive,
          Instance_pool::Auth_options{instance->get_connection_options()});

      auto rs = get_replica_set(metadata, instance);

      rs->acquire_primary();
      auto finally = shcore::on_leave_scope([&rs]() { rs->release_primary(); });

      metadata = ipool->get_metadata();
    }
  }

  instance = metadata->get_md_server();

  auto console = current_console();

  mysqlshdk::null_bool force = options->force;

  if (force.is_null() && interactive &&
      console->confirm("Are you sure you want to remove the Metadata?",
                       mysqlsh::Prompt_answer::NO) ==
          mysqlsh::Prompt_answer::YES) {
    force = true;
  }

  if (force.get_safe(false)) {
    // Check if super_read_only is turned off and disable it if required
    // NOTE: this is left for last to avoid setting super_read_only to true
    // and right before some execution failure of the command leaving the
    // instance in an incorrect state
    validate_super_read_only(*instance, options->clear_read_only, interactive);

    metadata::uninstall(instance);
    if (interactive) {
      console->print_info("Metadata Schema successfully removed.");
    }
  } else {
    if (interactive) {
      console->print_info("No changes made to the Metadata Schema.");
    } else {
      throw shcore::Exception::runtime_error(
          "No operation executed, use the 'force' option");
    }
  }
}

REGISTER_HELP_FUNCTION(checkInstanceConfiguration, dba);
REGISTER_HELP_FUNCTION_TEXT(DBA_CHECKINSTANCECONFIGURATION, R"*(
Validates an instance for MySQL InnoDB Cluster usage.

@param instance An instance definition.
@param options Optional data for the operation.

@returns A descriptive text of the operation result.

This function reviews the instance configuration to identify if it is valid for
usage with group replication. Use this to check for possible configuration
issues on MySQL instances before creating a cluster with them or adding them to
an existing cluster.

The instance definition is the connection data for the instance.

${TOPIC_CONNECTION_MORE_INFO}

The options dictionary may contain the following options:

@li mycnfPath: Optional path to the MySQL configuration file for the instance.
Alias for verifyMyCnf
@li verifyMyCnf: Optional path to the MySQL configuration file for the
instance. If this option is given, the configuration file will be verified for
the expected option values, in addition to the global MySQL system variables.
@li password: The password to get connected to the instance.
${OPT_INTERACTIVE}

The connection password may be contained on the instance definition, however,
it can be overwritten if it is specified on the options.

The returned descriptive text of the operation result indicates whether the
instance is valid for InnoDB Cluster usage or not. If not, a table containing
the following information is presented:

@li Variable: the invalid configuration variable.
@li Current Value: the current value for the invalid configuration variable.
@li Required Value: the required value for the configuration variable.
@li Note: the action to be taken.

The note can be one of the following:

@li Update the config file and update or restart the server variable.
@li Update the config file and restart the server.
@li Update the config file.
@li Update the server variable.
@li Restart the server.
)*");

/**
 * $(DBA_CHECKINSTANCECONFIGURATION_BRIEF)
 *
 * $(DBA_CHECKINSTANCECONFIGURATION)
 */
#if DOXYGEN_JS
JSON Dba::checkInstanceConfiguration(InstanceDef instance, Dictionary options) {
}
#elif DOXYGEN_PY
JSON Dba::check_instance_configuration(InstanceDef instance, dict options) {}
#endif
shcore::Value Dba::check_instance_configuration(
    const mysqlshdk::utils::nullable<Connection_options> &instance_def_,
    const shcore::Option_pack_ref<Check_instance_configuration_options>
        &options) {
  auto instance_def = instance_def_;
  const auto has_co = instance_def && instance_def->has_data();

  if (has_co && !options->password.is_null()) {
    auto connection_options = instance_def.operator->();
    connection_options->clear_password();
    connection_options->set_password(*options->password);
  }

  shcore::Value ret_val;
  std::shared_ptr<Instance> instance;
  std::shared_ptr<MetadataStorage> metadata;

  // Establish the session to the target instance
  if (has_co) {
    if (instance_def->has_ssh_options())
      throw shcore::Exception::logic_error(INNODB_SSH_NOT_SUPPORTED);
    instance = Instance::connect(*instance_def, options->interactive());
  } else {
    instance = connect_to_target_member();
  }

  if (instance) {
    metadata = std::make_shared<MetadataStorage>(instance);
  }

  check_function_preconditions("Dba.checkInstanceConfiguration", metadata,
                               instance);

  // Get the Connection_options
  Connection_options coptions = instance->get_connection_options();

  // Close the session
  instance->close_session();

  // Call the API
  Check_instance op_check_instance{coptions, options->mycnf_path};

  op_check_instance.prepare();
  ret_val = op_check_instance.execute();
  op_check_instance.finish();

  return ret_val;
}

// -----------------------------------------------------------------------------

REGISTER_HELP_FUNCTION(getReplicaSet, dba);
REGISTER_HELP_FUNCTION_TEXT(DBA_GETREPLICASET, R"*(
Returns an object representing a ReplicaSet.

The returned object is identical to the one returned by
<<<createReplicaSet>>>() and can be used to manage the replicaset.

The function will work regardless of whether the target instance is a PRIMARY
or a SECONDARY, but its copy of the metadata is expected to be up-to-date.
This function will also work if the PRIMARY is unreachable or unavailable,
although replicaset change operations will not be possible, except for
<<<forcePrimaryInstance>>>().
)*");
/**
 * $(DBA_GETREPLICASET_BRIEF)
 *
 * $(DBA_GETREPLICASET)
 */
#if DOXYGEN_JS
ReplicaSet Dba::getReplicaSet() {}
#elif DOXYGEN_PY
ReplicaSet Dba::get_replica_set() {}
#endif
std::shared_ptr<ReplicaSet> Dba::get_replica_set() {
  const auto console = mysqlsh::current_console();
  const auto target_server = connect_to_target_member();
  std::shared_ptr<MetadataStorage> metadata;

  if (target_server) {
    metadata = std::make_shared<MetadataStorage>(target_server);
  }

  check_function_preconditions("Dba.getReplicaSet", metadata, target_server);

  const auto rs = get_replica_set(metadata, target_server);

  console->print_info("You are connected to a member of replicaset '" +
                      rs->get_name() + "'.");

  return std::make_shared<ReplicaSet>(rs);
}

std::shared_ptr<Replica_set_impl> Dba::get_replica_set(
    const std::shared_ptr<MetadataStorage> &metadata,
    const std::shared_ptr<Instance> &target_server) {
  Cluster_metadata target_server_cm;

  if (!metadata->get_cluster_for_server_uuid(target_server->get_uuid(),
                                             &target_server_cm)) {
    throw shcore::Exception("No metadata found for " + target_server->descr() +
                                " (" + target_server->get_uuid() + ")",
                            SHERR_DBA_METADATA_MISSING);
  }

  return std::make_shared<Replica_set_impl>(target_server_cm, target_server,
                                            metadata);
}

REGISTER_HELP_FUNCTION(createReplicaSet, dba);
REGISTER_HELP_FUNCTION_TEXT(DBA_CREATEREPLICASET, R"*(
Creates a MySQL InnoDB ReplicaSet.

@param name An identifier for the replicaset to be created.
@param options Optional dictionary with additional parameters described below.

@returns The created replicaset object.

This function will create a managed replicaset using MySQL master/slave
replication, as opposed to Group Replication. The MySQL instance the shell is
connected to will be the initial PRIMARY of the replica set.

The function will perform several checks to ensure the instance state and
configuration are compatible with a managed replicaset and if so, a metadata
schema will be initialized there.

New replica instances can be added through the <<<addInstance>>>() function of
the returned replicaset object. Status of the instances and their replication
channels can be inspected with <<<status>>>().

<b>InnoDB ReplicaSets</b>

A replicaset allows managing a GTID-based MySQL replication setup, in a single
PRIMARY/multiple SECONDARY topology.

A replicaset has several limitations compared to a InnoDB cluster
and thus, it is recommended that InnoDB clusters be preferred unless not
possible. Generally, a ReplicaSet on its own does not provide High Availability.
Among its limitations are:

@li No automatic failover
@li No protection against inconsistencies or partial data loss in a crash

<b>Pre-Requisites</b>

The following is a non-exhaustive list of requirements for managed replicasets.
The dba.<<<configureInstance>>>() command can be used to make necessary
configuration changes automatically.

@li MySQL 8.0 or newer required
@li Statement Based Replication (SBR) is unsupported, only Row Based Replication
@li GTIDs required
@li Replication filters are not allowed
@li All instances in the replicaset must be managed
@li Unmanaged replication channels are not allowed in any instance

<b>Adopting an Existing Topology</b>

Existing asynchronous setups can be managed by calling this function with the
adoptFromAR option. The topology will be automatically scanned and validated,
starting from the instance the shell is connected to, and all instances that are
part of the topology will be automatically added to the replicaset.

The only changes made by this function to an adopted replicaset are the
creation of the metadata schema. Existing replication channels will not be
changed during adoption, although they will be changed during PRIMARY switch
operations.

However, it is only possible to manage setups that use supported configurations
and topology. Configuration of all instances will be checked during adoption,
to ensure they are compatible. All replication channels must be active and
their transaction sets as verified through GTID sets must be consistent.
The data set of all instances are expected to be identical, but is not verified.

<b>Options</b>

The options dictionary can contain the following values:

@li adoptFromAR: boolean value used to create the replicaset based on an
existing asynchronous replication setup.
@li instanceLabel: string a name to identify the target instance.
Defaults to hostname:port
@li dryRun: boolean if true, all validations and steps for creating a replica
set are executed, but no changes are actually made. An exception will be thrown
when finished.
@li gtidSetIsComplete: boolean value which indicates whether the GTID set
of the seed instance corresponds to all transactions executed. Default is false.
@li replicationAllowedHost: string value to use as the host name part of
internal replication accounts (i.e. 'mysql_innodb_rs_###'@'hostname'). Default is %.
It must be possible for any member of the ReplicaSet to connect to any other
member using accounts with this hostname value.
${OPT_INTERACTIVE}
)*");
/**
 * $(DBA_CREATEREPLICASET_BRIEF)
 *
 * $(DBA_CREATEREPLICASET)
 */
#if DOXYGEN_JS
ReplicaSet Dba::createReplicaSet(String name, Dictionary options) {}
#elif DOXYGEN_PY
ReplicaSet Dba::create_replica_set(str name, dict options) {}
#endif
shcore::Value Dba::create_replica_set(
    const std::string &full_rs_name,
    const shcore::Option_pack_ref<Create_replicaset_options> &options) {
  std::shared_ptr<Instance> target_server = connect_to_target_member();

  try {
    std::shared_ptr<MetadataStorage> metadata;
    if (target_server)
      metadata = std::make_shared<MetadataStorage>(target_server);

    check_function_preconditions("Dba.createReplicaSet", metadata,
                                 target_server);
  } catch (const shcore::Exception &e) {
    if (e.code() == SHERR_DBA_BADARG_INSTANCE_MANAGED_IN_CLUSTER) {
      throw shcore::Exception("Unable to create replicaset. The instance '" +
                                  target_server->descr() +
                                  "' already belongs to an InnoDB cluster. Use "
                                  "dba.<<<getCluster>>>() to access it.",
                              e.code());
    } else if (e.code() == SHERR_DBA_BADARG_INSTANCE_MANAGED_IN_REPLICASET) {
      throw shcore::Exception("Unable to create replicaset. The instance '" +
                                  target_server->descr() +
                                  "' already belongs to a replicaset. Use "
                                  "dba.<<<getReplicaSet>>>() to access it.",
                              e.code());
    } else {
      throw;
    }
  } catch (const shcore::Error &dberr) {
    throw shcore::Exception::mysql_error_with_code(dberr.what(), dberr.code());
  }

  Scoped_instance_pool ipool(
      nullptr, options->interactive(),
      Instance_pool::Auth_options{target_server->get_connection_options()});

  auto topology_type = Global_topology_type::SINGLE_PRIMARY_TREE;

  auto cluster = Replica_set_impl::create(full_rs_name, topology_type,
                                          target_server, *options);

  auto console = mysqlsh::current_console();
  console->print_info("ReplicaSet object successfully created for " +
                      target_server->descr() +
                      ".\nUse rs.<<<addInstance>>>() to add more "
                      "asynchronously replicated instances to this replicaset "
                      "and rs.<<<status>>>() to check its status.");
  console->print_info();

  return shcore::Value(std::make_shared<ReplicaSet>(cluster));
}

REGISTER_HELP_FUNCTION(getClusterSet, dba);
REGISTER_HELP_FUNCTION_TEXT(DBA_GETCLUSTERSET, R"*(
Returns an object representing a ClusterSet.

@returns The ClusterSet object to which the current session belongs to.

The returned object is identical to the one returned by
<<<createClusterSet>>>() and can be used to manage the ClusterSet.

The function will work regardless of whether the active session is established
to an instance that belongs to a PRIMARY or a REPLICA Cluster, but its copy of
the metadata is expected to be up-to-date.

This function will also work if the PRIMARY Cluster is unreachable or
unavailable, although ClusterSet change operations will not be possible, except
for failover with <<<forcePrimaryCluster>>>().
)*");
/**
 * $(DBA_GETCLUSTERSET_BRIEF)
 *
 * $(DBA_GETCLUSTERSET)
 */
#if DOXYGEN_JS
ReplicaSet Dba::getClusterSet() {}
#elif DOXYGEN_PY
ReplicaSet Dba::get_cluster_set() {}
#endif
std::shared_ptr<ClusterSet> Dba::get_cluster_set() {
  const auto target_server = connect_to_target_member();

  std::shared_ptr<MetadataStorage> metadata;

  if (target_server) {
    metadata = std::make_shared<MetadataStorage>(target_server);
  }

  check_function_preconditions("Dba.getClusterSet", metadata, target_server);

  // Validate if the target instance is a member of an InnoDB Cluster
  Cluster_metadata cluster_md;
  if (!metadata->get_cluster_for_server_uuid(target_server->get_uuid(),
                                             &cluster_md)) {
    throw shcore::Exception("No metadata found for " + target_server->descr() +
                                " (" + target_server->get_uuid() + ")",
                            SHERR_DBA_METADATA_MISSING);
  }

  auto cluster = get_cluster(cluster_md.cluster_name.c_str(), metadata,
                             target_server, false, true);

  auto cs = cluster->impl()->check_and_get_cluster_set_for_cluster();

  cluster->impl()->invalidate_cluster_set_metadata_cache();

  if (!cs || !cluster->impl()->is_cluster_set_member() ||
      cluster->impl()->is_cluster_set_remove_pending()) {
    throw shcore::Exception("Cluster is not part of a ClusterSet",
                            SHERR_DBA_CLUSTER_DOES_NOT_BELONG_TO_CLUSTERSET);
  }

  return std::make_shared<mysqlsh::dba::ClusterSet>(cs);
}

// -----

void Dba::exec_instance_op(const std::string &function, int port,
                           const std::string &sandbox_dir,
                           const std::string &password) {
  validate_port(port, "port");

  bool interactive = current_shell_options()->get().wizards;

  // Prints the operation introductory message
  auto console = mysqlsh::current_console();

  shcore::Value::Array_type_ref errors;

  if (interactive) {
    console->print_info();
    console->print_info(sandbox::Operations_text[function].progressive +
                        " MySQL instance...");
  }

  int rc = 0;
  if (function == "delete") {
    rc = _provisioning_interface.delete_sandbox(port, sandbox_dir, false,
                                                &errors);
  } else if (function == "kill") {
    rc = _provisioning_interface.kill_sandbox(port, sandbox_dir, &errors);
  } else if (function == "stop") {
    rc = _provisioning_interface.stop_sandbox(port, sandbox_dir, password,
                                              &errors);
  } else if (function == "start") {
    rc = _provisioning_interface.start_sandbox(port, sandbox_dir, &errors);
  }

  if (rc != 0) {
    throw_instance_op_error(errors);
  } else if (interactive) {
    console->print_info();
    console->print_info("Instance localhost:" + std::to_string(port) +
                        " successfully " +
                        sandbox::Operations_text[function].past + ".");
    console->print_info();
  }
}

REGISTER_HELP_FUNCTION(deploySandboxInstance, dba);
REGISTER_HELP_FUNCTION_TEXT(DBA_DEPLOYSANDBOXINSTANCE, R"*(
Creates a new MySQL Server instance on localhost.

@param port The port where the new instance will listen for connections.
@param options Optional dictionary with options affecting the new deployed
instance.

@returns Nothing.

This function will deploy a new MySQL Server instance, the result may be
affected by the provided options:

@li portx: port where the new instance will listen for X Protocol connections.
@li sandboxDir: path where the new instance will be deployed.
@li password: password for the MySQL root user on the new instance.
@li allowRootFrom: create remote root account, restricted to the given address
pattern (default: %).
@li ignoreSslError: ignore errors when adding SSL support for the new instance,
by default: true.
@li mysqldOptions: list of MySQL configuration options to write to the my.cnf
file, as option=value strings.

If the portx option is not specified, it will be automatically calculated as 10
times the value of the provided MySQL port.

The password option specifies the MySQL root password on the new instance.

The sandboxDir must be an existing folder where the new instance will be
deployed. If not specified the new instance will be deployed at:

  ~/mysql-sandboxes on Unix-like systems
or @%userprofile@%\\MySQL\\mysql-sandboxes on Windows systems.

SSL support is added by default if not already available for the new instance,
but if it fails to be added then the error is ignored. Set the ignoreSslError
option to false to ensure the new instance is deployed with SSL support.
)*");

/**
 * $(DBA_DEPLOYSANDBOXINSTANCE_BRIEF)
 *
 * $(DBA_DEPLOYSANDBOXINSTANCE)
 */
#if DOXYGEN_JS
Instance Dba::deploySandboxInstance(Integer port, Dictionary options) {}
#elif DOXYGEN_PY
Instance Dba::deploy_sandbox_instance(int port, dict options) {}
#endif
void Dba::deploy_sandbox_instance(
    int port, const shcore::Option_pack_ref<Deploy_sandbox_options> &options) {
  validate_port(port, "port");

  const Deploy_sandbox_options &opts = *options;

  if (!opts.xport.is_null()) {
    validate_port(*opts.xport, "portx");
  }

  mysqlshdk::null_string password = opts.password;
  bool interactive = current_shell_options()->get().wizards;
  auto console = mysqlsh::current_console();
  std::string path =
      shcore::path::join_path(options->sandbox_dir, std::to_string(port));

  if (interactive) {
    console->print_info(
        "A new MySQL sandbox instance will be created on this host in \n" +
        path +
        "\n\nWarning: Sandbox instances are only suitable for deploying and "
        "\nrunning on your local machine for testing purposes and are not "
        "\naccessible from external networks.\n");

    if (password.is_null()) {
      std::string answer;
      if (console->prompt_password(
              "Please enter a MySQL root password for the new instance: ",
              &answer) == shcore::Prompt_result::Ok) {
        password = answer;
      } else {
        return;
      }
    }
  }

  if (password.is_null()) {
    throw shcore::Exception::argument_error(
        "Missing root password for the deployed instance");
  }

  if (interactive) {
    console->print_info();
    console->print_info("Deploying new MySQL instance...");
  }

  shcore::Array_t errors;
  int rc = _provisioning_interface.create_sandbox(
      port, opts.xport.get_safe(0), options->sandbox_dir, *password,
      shcore::Value(opts.mysqld_options), true, opts.ignore_ssl_error, 0, "",
      &errors);

  if (rc != 0) throw_instance_op_error(errors);

  // Create root@<allowRootFrom> account
  // Default:
  //   allowRootFrom: %
  // Valid values:
  //   allowRootFrom: address
  //   allowRootFrom: %
  if (!opts.allow_root_from.empty()) {
    std::string uri = "root@localhost:" + std::to_string(port);
    mysqlshdk::db::Connection_options instance_def(uri);
    instance_def.set_password(*password);

    std::shared_ptr<Instance> instance = Instance::connect(instance_def);

    log_info("Creating root@%s account for sandbox %i",
             opts.allow_root_from.c_str(), port);

    instance->execute("SET sql_log_bin = 0");
    shcore::Scoped_callback enableLogBin(
        [&instance]() { instance->execute("SET sql_log_bin = 1"); });

    {
      std::string pwd;
      if (instance_def.has_password()) pwd = instance_def.get_password();

      shcore::sqlstring create_user(
          "CREATE USER root@? IDENTIFIED BY /*((*/ ? /*))*/", 0);
      create_user << opts.allow_root_from << pwd;
      create_user.done();
      instance->execute(create_user);
    }
    {
      shcore::sqlstring grant("GRANT ALL ON *.* TO root@? WITH GRANT OPTION",
                              0);
      grant << opts.allow_root_from;
      grant.done();
      instance->execute(grant);
    }
  }

  log_warning(
      "Sandbox instances are only suitable for deploying and running on "
      "your local machine for testing purposes and are not accessible "
      "from external networks.");

  if (current_shell_options()->get().wizards) {
    console->print_info();
    console->print_info("Instance localhost:" + std::to_string(port) +
                        " successfully deployed and started.");

    console->print_info(
        "Use shell.connect('root@localhost:" + std::to_string(port) +
        "') to connect to the instance.");
    console->print_info();
  }
}

REGISTER_HELP_FUNCTION(deleteSandboxInstance, dba);
REGISTER_HELP_FUNCTION_TEXT(DBA_DELETESANDBOXINSTANCE, R"*(
Deletes an existing MySQL Server instance on localhost.

@param port The port of the instance to be deleted.
@param options Optional dictionary with options that modify the way this
function is executed.

@returns Nothing.

This function will delete an existing MySQL Server instance on the local host.
The following options affect the result:

@li sandboxDir: path where the instance is located.

The sandboxDir must be the one where the MySQL instance was deployed. If not
specified it will use:

  ~/mysql-sandboxes on Unix-like systems
or @%userprofile@%\\MySQL\\mysql-sandboxes on Windows systems.

If the instance is not located on the used path an error will occur.
)*");

/**
 * $(DBA_DELETESANDBOXINSTANCE_BRIEF)
 *
 * $(DBA_DELETESANDBOXINSTANCE)
 */
#if DOXYGEN_JS
Undefined Dba::deleteSandboxInstance(Integer port, Dictionary options) {}
#elif DOXYGEN_PY
None Dba::delete_sandbox_instance(int port, dict options) {}
#endif
void Dba::delete_sandbox_instance(
    int port, const shcore::Option_pack_ref<Common_sandbox_options> &options) {
  exec_instance_op("delete", port, options->sandbox_dir);
}

REGISTER_HELP_FUNCTION(killSandboxInstance, dba);
REGISTER_HELP_FUNCTION_TEXT(DBA_KILLSANDBOXINSTANCE, R"*(
Kills a running MySQL Server instance on localhost.

@param port The port of the instance to be killed.
@param options Optional dictionary with options affecting the result.

@returns Nothing.

This function will kill the process of a running MySQL Server instance on the
local host. The following options affect the result:

@li sandboxDir: path where the instance is located.

The sandboxDir must be the one where the MySQL instance was deployed. If not
specified it will use:

  ~/mysql-sandboxes on Unix-like systems
or @%userprofile@%\\MySQL\\mysql-sandboxes on Windows systems.

If the instance is not located on the used path an error will occur.
)*");

/**
 * $(DBA_KILLSANDBOXINSTANCE_BRIEF)
 *
 * $(DBA_KILLSANDBOXINSTANCE)
 */
#if DOXYGEN_JS
Undefined Dba::killSandboxInstance(Integer port, Dictionary options) {}
#elif DOXYGEN_PY
None Dba::kill_sandbox_instance(int port, dict options) {}
#endif
void Dba::kill_sandbox_instance(
    int port, const shcore::Option_pack_ref<Common_sandbox_options> &options) {
  exec_instance_op("kill", port, options->sandbox_dir);
}

REGISTER_HELP_FUNCTION(stopSandboxInstance, dba);
REGISTER_HELP_FUNCTION_TEXT(DBA_STOPSANDBOXINSTANCE, R"*(
Stops a running MySQL Server instance on localhost.

@param port The port of the instance to be stopped.
@param options Optional dictionary with options affecting the result.

@returns Nothing.

This function will gracefully stop a running MySQL Server instance on the local
host. The following options affect the result:

@li sandboxDir: path where the instance is located.
@li password: password for the MySQL root user on the instance.

The sandboxDir must be the one where the MySQL instance was deployed. If not
specified it will use:

  ~/mysql-sandboxes on Unix-like systems
or @%userprofile@%\\MySQL\\mysql-sandboxes on Windows systems.

If the instance is not located on the used path an error will occur.
)*");

/**
 * $(DBA_STOPSANDBOXINSTANCE_BRIEF)
 *
 * $(DBA_STOPSANDBOXINSTANCE)
 */
#if DOXYGEN_JS
Undefined Dba::stopSandboxInstance(Integer port, Dictionary options) {}
#elif DOXYGEN_PY
None Dba::stop_sandbox_instance(int port, dict options) {}
#endif
void Dba::stop_sandbox_instance(
    int port, const shcore::Option_pack_ref<Stop_sandbox_options> &options) {
  std::string sandbox_dir;

  validate_port(port, "port");

  mysqlshdk::null_string password = options->password;

  bool interactive = current_shell_options()->get().wizards;
  auto console = mysqlsh::current_console();
  std::string path = shcore::path::join_path(sandbox_dir, std::to_string(port));
  if (interactive) {
    console->print_info("The MySQL sandbox instance on this host in \n" + path +
                        " will be " + sandbox::Operations_text["stop"].past +
                        "\n");

    if (password.is_null()) {
      std::string answer;
      if (console->prompt_password(
              "Please enter the MySQL root password for the instance "
              "'localhost:" +
                  std::to_string(port) + "': ",
              &answer) == shcore::Prompt_result::Ok) {
        password = answer;
      } else {
        return;
      }
    }
  }

  if (password.is_null()) {
    throw shcore::Exception::argument_error(
        "Missing root password for the instance");
  }

  auto instance = std::make_shared<Instance>(get_active_shell_session());
  if (instance && instance->get_session()) {
    auto connection_opts = instance->get_connection_options();
    if ((connection_opts.get_host() == "localhost" ||
         connection_opts.get_host() == "127.0.0.1") &&
        connection_opts.get_port() == port) {
      if (interactive) {
        console->print_info(
            "The active session is established to the sandbox being stopped "
            "so it's going to be closed.");
      }
      instance->close_session();
    }
  }

  exec_instance_op("stop", port, options->sandbox_dir, *password);
}

REGISTER_HELP_FUNCTION(startSandboxInstance, dba);
REGISTER_HELP_FUNCTION_TEXT(DBA_STARTSANDBOXINSTANCE, R"*(
Starts an existing MySQL Server instance on localhost.

@param port The port where the instance listens for MySQL connections.
@param options Optional dictionary with options affecting the result.

@returns Nothing.

This function will start an existing MySQL Server instance on the local host.
The following options affect the result:

@li sandboxDir: path where the instance is located.

The sandboxDir must be the one where the MySQL instance was deployed. If not
specified it will use:

  ~/mysql-sandboxes on Unix-like systems
or @%userprofile@%\\MySQL\\mysql-sandboxes on Windows systems.

If the instance is not located on the used path an error will occur.
)*");

/**
 * $(DBA_STARTSANDBOXINSTANCE_BRIEF)
 *
 * $(DBA_STARTSANDBOXINSTANCE)
 */
#if DOXYGEN_JS
Undefined Dba::startSandboxInstance(Integer port, Dictionary options) {}
#elif DOXYGEN_PY
None Dba::start_sandbox_instance(int port, dict options) {}
#endif
void Dba::start_sandbox_instance(
    int port, const shcore::Option_pack_ref<Common_sandbox_options> &options) {
  exec_instance_op("start", port, options->sandbox_dir);
}

void Dba::do_configure_instance(
    const mysqlshdk::db::Connection_options &instance_def_,
    const Configure_instance_options &options, Cluster_type purpose) {
  shcore::Value ret_val;
  auto instance_def = instance_def_;

  if (instance_def.has_data() && !options.password.is_null()) {
    instance_def.clear_password();
    instance_def.set_password(*options.password);
  }

  std::shared_ptr<Instance> target_instance;
  std::shared_ptr<MetadataStorage> metadata;

  Cluster_check_info state;

  // Establish the session to the target instance
  if (instance_def.has_data()) {
    if (instance_def.has_ssh_options())
      throw shcore::Exception::logic_error(INNODB_SSH_NOT_SUPPORTED);
    target_instance = Instance::connect(instance_def, options.interactive());
  } else {
    target_instance = connect_to_target_member();
  }

  if (target_instance) {
    metadata = std::make_shared<MetadataStorage>(target_instance);
  }

  // Check the function preconditions
  if (options.cluster_type == Cluster_type::ASYNC_REPLICATION) {
    state = check_function_preconditions("Dba.configureReplicaSetInstance",
                                         metadata, target_instance);
  } else {
    state = check_function_preconditions(
        options.local ? "Dba.configureLocalInstance" : "Dba.configureInstance",
        metadata, target_instance);
  }

  auto warnings_callback = [](const std::string &sql, int code,
                              const std::string &level,
                              const std::string &msg) {
    auto console = current_console();
    std::string debug_msg;

    debug_msg += level + ": '" + msg + "'. (Code " + std::to_string(code) +
                 ") When executing query: '" + sql + "'.";

    log_debug("%s", debug_msg.c_str());

    if (level == "WARNING") {
      console->print_info();
      console->print_warning(msg + " (Code " + std::to_string(code) + ").");
    }
  };

  // Add the warnings callback
  target_instance->register_warnings_callback(warnings_callback);

  Scoped_instance_pool ipool(
      nullptr, options.interactive(),
      Instance_pool::Auth_options{target_instance->get_connection_options()});

  {
    // Call the API
    std::unique_ptr<Configure_instance> op_configure_instance;
    if (options.local) {
      op_configure_instance.reset(
          new Configure_local_instance(target_instance, options, purpose));
    } else {
      op_configure_instance.reset(new Configure_instance(
          target_instance, options, state.source_type, purpose));
    }

    // Prepare and execute the operation.
    op_configure_instance->prepare();
    op_configure_instance->execute();
  }
}

REGISTER_HELP_FUNCTION(configureLocalInstance, dba);
REGISTER_HELP_FUNCTION_TEXT(DBA_CONFIGURELOCALINSTANCE, R"*(
Validates and configures a local instance for MySQL InnoDB Cluster usage.

@param instance Optional An instance definition.
@param options Optional Additional options for the operation.

@returns Nothing

This function reviews the instance configuration to identify if it is valid for
usage in an InnoDB cluster, making configuration changes if necessary.

The instance definition is the connection data for the instance.

${TOPIC_CONNECTION_MORE_INFO}

${CONFIGURE_INSTANCE_COMMON_OPTIONS}

${CONFIGURE_INSTANCE_COMMON_DETAILS_1}

The returned descriptive text of the operation result indicates whether the
instance was successfully configured for InnoDB Cluster usage or if it was
already valid for InnoDB Cluster usage.

${CONFIGURE_INSTANCE_COMMON_DETAILS_2}
)*");

/**
 * $(DBA_CONFIGURELOCALINSTANCE_BRIEF)
 *
 * $(DBA_CONFIGURELOCALINSTANCE)
 */
#if DOXYGEN_JS
Undefined Dba::configureLocalInstance(InstanceDef instance,
                                      Dictionary options) {}
#elif DOXYGEN_PY
None Dba::configure_local_instance(InstanceDef instance, dict options) {}
#endif
void Dba::configure_local_instance(
    const mysqlshdk::utils::nullable<Connection_options> &instance_def,
    const shcore::Option_pack_ref<Configure_cluster_local_instance_options>
        &options) {
  return do_configure_instance(
      instance_def ? *instance_def : Connection_options{}, *options,
      Cluster_type::GROUP_REPLICATION);
}

REGISTER_HELP_FUNCTION(configureInstance, dba);
REGISTER_HELP_FUNCTION_TEXT(DBA_CONFIGUREINSTANCE, R"*(
Validates and configures an instance for MySQL InnoDB Cluster usage.

@param instance Optional An instance definition.
@param options Optional Additional options for the operation.

@returns A descriptive text of the operation result.

This function auto-configures the instance for InnoDB Cluster usage. If the
target instance already belongs to an InnoDB Cluster it errors out.

The instance definition is the connection data for the instance.

${TOPIC_CONNECTION_MORE_INFO}

${CONFIGURE_INSTANCE_COMMON_OPTIONS}
${OPT_APPLIERWORKERTHREADS}

${CONFIGURE_INSTANCE_COMMON_DETAILS_1}

This function reviews the instance configuration to identify if it is valid for
usage in group replication and cluster. An exception is thrown if not.

${CONFIGURE_INSTANCE_COMMON_DETAILS_2}
)*");

REGISTER_HELP_TOPIC_TEXT(CONFIGURE_INSTANCE_COMMON_OPTIONS, R"*(
The options dictionary may contain the following options:

@li mycnfPath: The path to the MySQL configuration file of the instance.
@li outputMycnfPath: Alternative output path to write the MySQL configuration
file of the instance.
@li password: The password to be used on the connection.
@li clusterAdmin: The name of the "cluster administrator" account.
@li clusterAdminPassword: The password for the "cluster administrator" account.
@li clearReadOnly: boolean value used to confirm that super_read_only must be
disabled.
@li restart: boolean value used to indicate that a remote restart of the target
instance should be performed to finalize the operation.
${OPT_INTERACTIVE}
)*");

REGISTER_HELP_TOPIC_TEXT(CONFIGURE_INSTANCE_COMMON_DETAILS_1, R"*(
If the outputMycnfPath option is used, only that file is updated and mycnfPath
is treated as read-only.

The connection password may be contained on the instance definition, however,
it can be overwritten if it is specified on the options.

The clusterAdmin must be a standard MySQL account name. It could be either an
existing account or an account to be created.

The clusterAdminPassword must be specified only if the clusterAdmin account will
be created.
)*");

REGISTER_HELP_TOPIC_TEXT(CONFIGURE_INSTANCE_COMMON_DETAILS_2, R"*(
If the instance was not valid for InnoDB Cluster and interaction is enabled,
before configuring the instance a prompt to confirm the changes is presented
and a table with the following information:

@li Variable: the invalid configuration variable.
@li Current Value: the current value for the invalid configuration variable.
@li Required Value: the required value for the configuration variable.
)*");

/**
 * $(DBA_CONFIGUREINSTANCE_BRIEF)
 *
 * $(DBA_CONFIGUREINSTANCE)
 */
#if DOXYGEN_JS
Undefined Dba::configureInstance(InstanceDef instance, Dictionary options) {}
#elif DOXYGEN_PY
None Dba::configure_instance(InstanceDef instance, dict options) {}
#endif
void Dba::configure_instance(
    const mysqlshdk::utils::nullable<Connection_options> &instance_def,
    const shcore::Option_pack_ref<Configure_cluster_instance_options>
        &options) {
  do_configure_instance(instance_def ? *instance_def : Connection_options{},
                        *options, Cluster_type::GROUP_REPLICATION);
}

REGISTER_HELP_FUNCTION(configureReplicaSetInstance, dba);
REGISTER_HELP_FUNCTION_TEXT(DBA_CONFIGUREREPLICASETINSTANCE, R"*(
Validates and configures an instance for use in an InnoDB ReplicaSet.

@param instance Optional An instance definition. By default, the active shell
session is used.
@param options Optional Additional options for the operation.

@returns Nothing

This function will verify and automatically configure the target instance for
use in an InnoDB ReplicaSet.

The function can optionally create a "cluster administrator" account, if the
"clusterAdmin" and "clusterAdminPassword" options are given. The account is
created with the minimal set of privileges required to manage InnoDB clusters
or ReplicaSets. The "cluster administrator" account must have matching username
and password across all instances of the same cluster or replicaset.

<b>Options</b>

The instance definition is the connection data for the instance.

${TOPIC_CONNECTION_MORE_INFO}

The options dictionary may contain the following options:

@li password: The password to be used on the connection.
@li clusterAdmin: The name of a "cluster administrator" user to be
created. The supported format is the standard MySQL account name format.
@li clusterAdminPassword: The password for the "cluster administrator" account.
${OPT_INTERACTIVE}
@li restart: boolean value used to indicate that a remote restart of the target
instance should be performed to finalize the operation.
${OPT_APPLIERWORKERTHREADS}

The connection password may be contained on the instance definition, however,
it can be overwritten if it is specified on the options.

This function reviews the instance configuration to identify if it is valid for
usage in replicasets. An exception is thrown if not.

If the instance was not valid for InnoDB ReplicaSet and interaction is enabled,
before configuring the instance a prompt to confirm the changes is presented
and a table with the following information:

@li Variable: the invalid configuration variable.
@li Current Value: the current value for the invalid configuration variable.
@li Required Value: the required value for the configuration variable.
)*");

/**
 * $(DBA_CONFIGUREREPLICASETINSTANCE_BRIEF)
 *
 * $(DBA_CONFIGUREREPLICASETINSTANCE)
 */
#if DOXYGEN_JS
Undefined Dba::configureReplicaSetInstance(InstanceDef instance,
                                           Dictionary options) {}
#elif DOXYGEN_PY
None Dba::configure_replica_set_instance(InstanceDef instance, dict options) {}
#endif
void Dba::configure_replica_set_instance(
    const mysqlshdk::utils::nullable<Connection_options> &instance_def,
    const shcore::Option_pack_ref<Configure_replicaset_instance_options>
        &options) {
  return do_configure_instance(
      instance_def ? *instance_def : Connection_options{}, *options,
      Cluster_type::ASYNC_REPLICATION);
}

REGISTER_HELP_FUNCTION(rebootClusterFromCompleteOutage, dba);
REGISTER_HELP_FUNCTION_TEXT(DBA_REBOOTCLUSTERFROMCOMPLETEOUTAGE, R"*(
Brings a cluster back ONLINE when all members are OFFLINE.

@param clusterName Optional The name of the cluster to be rebooted.
@param options Optional dictionary with options that modify the behavior of
this function.

@returns The rebooted cluster object.

The options dictionary can contain the next values:

@li user: The user used for the instances sessions required operations.
@li password: The password used for the instances sessions required operations.
@li clearReadOnly: boolean value used to confirm that super_read_only must be
disabled.
@li force: boolean value to indicate that the operation must be executed even if some members of
the Cluster cannot be reached, or the primary instance selected has a diverging or lower GTID_SET.
@li dryRun: boolean value that if true, all validations and steps of the command are
executed, but no changes are actually made. An exception will be thrown when finished.
@li primary: Instance definition representing the instance that must be selected as the primary.
@li switchCommunicationStack: The Group Replication protocol stack to be used by the Cluster after the reboot.
@li ipAllowList: The list of hosts allowed to connect to the instance for Group Replication traffic when using the 'XCOM' protocol stack.
@li localAddress: string value with the Group Replication local address to be used instead of the automatically generated one when using the 'XCOM' protocol stack.

The value for switchCommunicationStack is used to choose which Group
Replication communication stack must be used in the Cluster after the reboot is complete. It's used to set the value of the Group Replication system variable
'group_replication_communication_stack'.

When set to legacy 'XCom', all internal GCS network traffic (PAXOS and
communication infrastructure) flows through a separate network address:
the localAddress.

When set to 'MySQL', such traffic re-uses the existing MySQL Server
facilities to establish connections among Cluster members. It allows a
simpler and safer setup as it obsoletes the usage of IP allowlists
(ipAllowlist), removes the explicit need to have a separate network
address (localAddress), and introduces user-based authentication.

The option is used to switch the communication stack previously in
use by the Cluster, to another one. Setting the same communication stack
results in no changes.

The ipAllowlist format is a comma separated list of IP addresses or
subnet CIDR notation, for example: 192.168.1.0/24,10.0.0.1. By default
the value is set to AUTOMATIC, allowing addresses from the instance
private network to be automatically set for the allowlist.

${CLUSTER_OPT_LOCAL_ADDRESS_EXTRA}

@attention The clearReadOnly option will be removed in a future release.

@attention The user option will be removed in a future release.

@attention The password option will be removed in a future release.

This function reboots a cluster from complete outage. It obtains the
Cluster information from the instance MySQL Shell is connected to and
uses the most up-to-date instance in regards to transactions as new seed
instance to recover the cluster. The remaining Cluster members are
automatically rejoined.

On success, the restored cluster object is returned by the function.

The current session must be connected to a former instance of the
cluster.

If name is not specified, the default cluster will be returned.

@note The user and password options are no longer used, the connection data is
taken from the active shell session.

@note The clearReadOnly option is no longer used, super_read_only is automatically cleared.
)*");

/**
 * $(DBA_REBOOTCLUSTERFROMCOMPLETEOUTAGE_BRIEF)
 *
 * $(DBA_REBOOTCLUSTERFROMCOMPLETEOUTAGE)
 */
#if DOXYGEN_JS
Cluster Dba::rebootClusterFromCompleteOutage(String clusterName,
                                             Dictionary options) {}
#elif DOXYGEN_PY
Cluster Dba::reboot_cluster_from_complete_outage(str clusterName,
                                                 dict options) {}
#endif

REGISTER_HELP_FUNCTION(upgradeMetadata, dba);
REGISTER_HELP_FUNCTION_TEXT(DBA_UPGRADEMETADATA, R"*(
Upgrades (or restores) the metadata to the version supported by the Shell.

@param options Optional dictionary with option for the operation.

This function will compare the version of the installed metadata schema
with the version of the metadata schema supported by this Shell. If the
installed metadata version is lower, an upgrade process will be started.

The options dictionary accepts the following attributes:

@li dryRun: boolean value used to enable a dry run of the upgrade process.
${OPT_INTERACTIVE}

If dryRun is used, the function will determine whether a metadata upgrade
or restore is required and inform the user without actually executing the
operation.

The interactive option can be used to explicitly enable or disable the
interactive prompts that help the user through te upgrade process. The default
value is equal to MySQL Shell wizard mode.

<b>The Upgrade Process</b>

When upgrading the metadata schema of clusters deployed by Shell versions before
8.0.19, a rolling upgrade of existing MySQL Router instances is required.
This process allows minimizing disruption to applications during the upgrade.

The rolling upgrade process must be performed in the following order:

-# Execute dba.<<<upgradeMetadata>>>() using the latest Shell version. The
upgrade function will stop if outdated Router instances are detected, at
which point you can stop the upgrade to resume later.
-# Upgrade MySQL Router to the latest version (same version number as the Shell)
-# Continue or re-execute dba.<<<upgradeMetadata>>>()


<b>Failed Upgrades</b>

If the installed metadata is not available because a previous call to this
function ended unexpectedly, this function will restore the metadata to the
state it was before the failed upgrade operation.
)*");
/**
 * $(DBA_UPGRADEMETADATA_BRIEF)
 *
 * $(DBA_UPGRADEMETADATA)
 */
#if DOXYGEN_JS
Undefined Dba::upgradeMetadata(Dictionary options) {}
#elif DOXYGEN_PY
None Dba::upgrade_metadata(dict options) {}
#endif
void Dba::upgrade_metadata(
    const shcore::Option_pack_ref<Upgrade_metadata_options> &options) {
  auto instance = connect_to_target_member();
  std::shared_ptr<MetadataStorage> metadata;

  if (instance) {
    metadata = std::make_shared<MetadataStorage>(instance);
  }

  auto state =
      check_function_preconditions("Dba.upgradeMetadata", metadata, instance);

  // The pool is initialized with the metadata using the current session

  Scoped_instance_pool ipool(
      metadata, options->interactive(),
      Instance_pool::Auth_options{instance->get_connection_options()});

  // If it happens we are on a RO instance, we update the metadata to make it
  // use a session to the PRIMARY of the IDC/RSET
  if (state.source_state == ManagedInstance::OnlineRO) {
    if (state.source_type == TargetType::InnoDBCluster) {
      connect_to_target_group({}, &metadata, nullptr, true);
    } else {
      auto rs = get_replica_set(metadata, instance);

      rs->acquire_primary();
      auto finally = shcore::on_leave_scope([&rs]() { rs->release_primary(); });

      metadata = ipool->get_metadata();
    }
  }

  Upgrade_metadata op_upgrade(metadata, options->interactive(),
                              options->dry_run);

  auto finally =
      shcore::on_leave_scope([&op_upgrade]() { op_upgrade.finish(); });

  op_upgrade.prepare();

  op_upgrade.execute();
}

}  // namespace dba
}  // namespace mysqlsh
