//@ {VER(>=8.0.11)}

////////////////////////////////////////////////////////////////////////////////
// helpers

function mysqlsh(args) {
  return testutil.callMysqlsh(args, "", ["MYSQLSH_TERM_COLOR_MODE=nocolor"]);
}

////////////////////////////////////////////////////////////////////////////////

//@<> Setup
const __hostname_ip_uri1 = __hostname_uri1.replace(hostname, hostname_ip);
const __hostname_ip_uri2 = __hostname_uri2.replace(hostname, hostname_ip);
const __hostname_ip_uri3 = __hostname_uri3.replace(hostname, hostname_ip);

const __hostname_ip_x_uri1 = __hostname_ip_uri1.replace('mysql', 'mysqlx').replace(__mysql_sandbox_port1, __mysql_sandbox_x_port1);
const __hostname_ip_x_uri2 = __hostname_ip_uri2.replace('mysql', 'mysqlx').replace(__mysql_sandbox_port2, __mysql_sandbox_x_port2);
const __hostname_ip_x_uri3 = __hostname_ip_uri3.replace('mysql', 'mysqlx').replace(__mysql_sandbox_port3, __mysql_sandbox_x_port3);

testutil.deploySandbox(__mysql_sandbox_port1, 'root', {'report_host': hostname_ip});
testutil.deploySandbox(__mysql_sandbox_port2, 'root', {'report_host': hostname_ip});
testutil.deploySandbox(__mysql_sandbox_port3, 'root', {'report_host': hostname_ip});

shell.connect(__hostname_ip_uri1);
rs = dba.createReplicaSet('rs', {'gtidSetIsComplete': true});
rs.addInstance(__hostname_ip_uri2);
session.close();

////////////////////////////////////////////////////////////////////////////////
// WL13236-TSFR1_1: Validate that a command line option named --replicaset is supported by the Shell.
// WL13236-TSFR2_1: Validate that the help for the command line option named --redirect-primary contains information about the use with ReplicaSet.
// WL13236-TSFR3_1: Validate that the help for the command line option named --redirect-secondary contains information about the use with ReplicaSet.
// Tested in: mysqlsh_help_norecord.js

////////////////////////////////////////////////////////////////////////////////
// WL13236-TSFR1_2: Start the Shell with a valid connection data of an instance that is member of a ReplicaSet and use the --replicaset option. Validate that the object named `rs` is set to the ReplicaSet the target instance belongs to.

//@<> WL13236-TSFR1_2
function expect_rs_variable() {
  EXPECT_STDOUT_CONTAINS_MULTILINE(`You are connected to a member of replicaset 'rs'.
{
    "replicaSet": {
        "name": "rs",
        "primary": "${hostname_ip}:${__mysql_sandbox_port1}",
        "status": "AVAILABLE",
        "statusText": "All instances available.",
        "topology": {
            "${hostname_ip}:${__mysql_sandbox_port1}": {
                "address": "${hostname_ip}:${__mysql_sandbox_port1}",
                "instanceRole": "PRIMARY",
                "mode": "R/W",
                "status": "ONLINE"
            },
            "${hostname_ip}:${__mysql_sandbox_port2}": {
                "address": "${hostname_ip}:${__mysql_sandbox_port2}",
                "instanceRole": "SECONDARY",
                "mode": "R/O",
                "replication": {
                    "applierStatus": "APPLIED_ALL",
                    "applierThreadState": "Slave has read all relay log; waiting for more updates",
                    "receiverStatus": "ON",
                    "receiverThreadState": "Waiting for master to send event",
                    "replicationLag": null
                },
                "status": "ONLINE"
            }
        },
        "type": "ASYNC"
    }
}`);
}

mysqlsh([__hostname_ip_uri1, '--replicaset', '--execute', 'println(rs.status())']);

expect_rs_variable();

//@<> WL13236-TSFR1_2 - X
mysqlsh([__hostname_ip_x_uri1, '--replicaset', '--execute', 'println(rs.status())']);

expect_rs_variable();

////////////////////////////////////////////////////////////////////////////////
// WL13236-TSFR1_3: Start the Shell with a valid connection data to an instance that is not member of a ReplicaSet and use the --replicaset option. Validate that the `rs` object is not set and an exception is thrown because the target instance is not part of a ReplicaSet.

//@<> WL13236-TSFR1_3
function expect_error_standalone_instance() {
  EXPECT_STDOUT_CONTAINS_MULTILINE(`WARNING: Option --replicaset requires a session to a member of an InnoDB ReplicaSet.
ERROR: RuntimeError: This function is not available through a session to a standalone instance`);
}

mysqlsh([__hostname_ip_uri3, '--replicaset', '--execute', 'println(rs.status())']);

expect_error_standalone_instance();

//@<> WL13236-TSFR1_3 - X
mysqlsh([__hostname_ip_x_uri3, '--replicaset', '--execute', 'println(rs.status())']);

expect_error_standalone_instance();

////////////////////////////////////////////////////////////////////////////////
// WL13236-TSFR1_4: Start the Shell with no connection data and use the --replicaset option. Validate that an exception is thrown because no connection data is provided.

//@<> WL13236-TSFR1_4
mysqlsh(['--replicaset', '--execute', 'println(rs.status())']);

EXPECT_STDOUT_CONTAINS_MULTILINE(`WARNING: Option --replicaset requires a session to a member of an InnoDB ReplicaSet.
ERROR: RuntimeError: An open session is required to perform this operation.`);

////////////////////////////////////////////////////////////////////////////////
// WL13236-TSFR2_2: Start the Shell with a valid connection data of an instance that is secondary member of a ReplicaSet and use the --redirect-primary option. Validate that the the active session is created on the primary.

//@<> WL13236-TSFR2_2
mysqlsh([__hostname_ip_uri2, '--redirect-primary', '--execute', 'println(session)']);

EXPECT_STDOUT_CONTAINS_MULTILINE(`Reconnecting to the PRIMARY instance of an InnoDB ReplicaSet (${hostname_ip}:${__mysql_sandbox_port1})...
<ClassicSession:root@${hostname_ip}:${__mysql_sandbox_port1}>`);

//@<> WL13236-TSFR2_2 - X
mysqlsh([__hostname_ip_x_uri2, '--redirect-primary', '--execute', 'println(session)']);

EXPECT_STDOUT_CONTAINS_MULTILINE(`Reconnecting to the PRIMARY instance of an InnoDB ReplicaSet (${hostname_ip}:${__mysql_sandbox_x_port1})...
<Session:root@${hostname_ip}:${__mysql_sandbox_x_port1}>`);

////////////////////////////////////////////////////////////////////////////////
//@<> connect to secondary, use --redirect-primary + --replicaset, expect no error
mysqlsh([__hostname_ip_uri2, '--redirect-primary', '--replicaset', '--execute', 'println(rs.status())']);

expect_rs_variable();

////////////////////////////////////////////////////////////////////////////////
//@<> connect to secondary, use --redirect-primary + --replicaset - X, expect no error
mysqlsh([__hostname_ip_x_uri2, '--redirect-primary', '--replicaset', '--execute', 'println(rs.status())']);

expect_rs_variable();

////////////////////////////////////////////////////////////////////////////////
//@<> connect to primary, use --redirect-primary, expect no error
function expect_already_primary() {
  EXPECT_STDOUT_CONTAINS('NOTE: --redirect-primary ignored because target is already a PRIMARY');
}

mysqlsh([__hostname_ip_uri1, '--redirect-primary', '--execute', 'println(session)']);

expect_already_primary();
EXPECT_STDOUT_CONTAINS(`<ClassicSession:root@${hostname_ip}:${__mysql_sandbox_port1}>`);

////////////////////////////////////////////////////////////////////////////////
//@<> connect to primary, use --redirect-primary, expect no error - X
mysqlsh([__hostname_ip_x_uri1, '--redirect-primary', '--execute', 'println(session)']);

expect_already_primary();
EXPECT_STDOUT_CONTAINS(`<Session:root@${hostname_ip}:${__mysql_sandbox_x_port1}>`);

////////////////////////////////////////////////////////////////////////////////
//@<> connect to primary, use --redirect-primary + --replicaset, expect no error
mysqlsh([__hostname_ip_uri1, '--redirect-primary', '--replicaset', '--execute', 'println(rs.status())']);

expect_rs_variable();

////////////////////////////////////////////////////////////////////////////////
//@<> connect to primary, use --redirect-primary + --replicaset - X, expect no error
mysqlsh([__hostname_ip_x_uri1, '--redirect-primary', '--replicaset', '--execute', 'println(rs.status())']);

expect_rs_variable();

////////////////////////////////////////////////////////////////////////////////
// WL13236-TSFR2_3: Start the Shell with a valid connection data to an instance that is not member of a ReplicaSet and use the --redirect-primary option. Validate that an exception is thrown because the target instance is not part of a ReplicaSet.

//@<> WL13236-TSFR2_3
function expect_error_redirect_primary_missing_metadata() {
  EXPECT_STDOUT_CONTAINS('While handling --redirect-primary:');
  EXPECT_STDOUT_CONTAINS('Metadata schema of an InnoDB cluster or ReplicaSet not found');
}

mysqlsh([__hostname_ip_uri3, '--redirect-primary', '--execute', 'println(session)']);

expect_error_redirect_primary_missing_metadata();

//@<> WL13236-TSFR2_3 - X
mysqlsh([__hostname_ip_x_uri3, '--redirect-primary', '--execute', 'println(session)']);

expect_error_redirect_primary_missing_metadata();

////////////////////////////////////////////////////////////////////////////////
//@<> connect to an instance that does not belong to a replicaset, use --redirect-primary + --replicaset, expect error
mysqlsh([__hostname_ip_uri3, '--redirect-primary', '--replicaset', '--execute', 'println(rs.status())']);

expect_error_redirect_primary_missing_metadata();

////////////////////////////////////////////////////////////////////////////////
//@<> connect to an instance that does not belong to a replicaset, use --redirect-primary + --replicaset - X, expect error
mysqlsh([__hostname_ip_x_uri3, '--redirect-primary', '--replicaset', '--execute', 'println(rs.status())']);

expect_error_redirect_primary_missing_metadata();

////////////////////////////////////////////////////////////////////////////////
// WL13236-TSFR2_4: Start the Shell with no connection data and use the --redirect-primary option. Validate that an exception is thrown because no connection data is provided.

//@<> WL13236-TSFR2_4
function expect_error_redirect_primary_no_session() {
  EXPECT_STDOUT_CONTAINS('ERROR: The --redirect-primary option requires a session to a member of an InnoDB cluster or ReplicaSet.');
}

mysqlsh(['--redirect-primary', '--execute', 'println(session)']);

expect_error_redirect_primary_no_session();

////////////////////////////////////////////////////////////////////////////////
//@<> use --redirect-primary + --replicaset without connection, expect error
mysqlsh(['--redirect-primary', '--replicaset', '--execute', 'println(rs.status())']);

expect_error_redirect_primary_no_session();

////////////////////////////////////////////////////////////////////////////////
// WL13236-TSFR3_2: Start the Shell with a valid connection data of an instance that is the primary member of a ReplicaSet and use the --redirect-secondary option. Validate that the the active session is created on a secondary instance.

//@<> WL13236-TSFR3_2
mysqlsh([__hostname_ip_uri1, '--redirect-secondary', '--execute', 'println(session)']);

EXPECT_STDOUT_CONTAINS_MULTILINE(`Reconnecting to the SECONDARY instance of an InnoDB ReplicaSet (${hostname_ip}:${__mysql_sandbox_port2})...
<ClassicSession:root@${hostname_ip}:${__mysql_sandbox_port2}>`);

//@<> WL13236-TSFR3_2 - X
mysqlsh([__hostname_ip_x_uri1, '--redirect-secondary', '--execute', 'println(session)']);

EXPECT_STDOUT_CONTAINS_MULTILINE(`Reconnecting to the SECONDARY instance of an InnoDB ReplicaSet (${hostname_ip}:${__mysql_sandbox_x_port2})...
<Session:root@${hostname_ip}:${__mysql_sandbox_x_port2}>`);

////////////////////////////////////////////////////////////////////////////////
//@<> connect to primary, use --redirect-secondary + --replicaset, expect no error
mysqlsh([__hostname_ip_uri1, '--redirect-secondary', '--replicaset', '--execute', 'println(rs.status())']);

expect_rs_variable();

////////////////////////////////////////////////////////////////////////////////
//@<> connect to primary, use --redirect-secondary + --replicaset - X, expect no error
mysqlsh([__hostname_ip_x_uri1, '--redirect-secondary', '--replicaset', '--execute', 'println(rs.status())']);

expect_rs_variable();

////////////////////////////////////////////////////////////////////////////////
//@<> connect to secondary, use --redirect-secondary, expect no error
function expect_already_secondary() {
  EXPECT_STDOUT_CONTAINS('NOTE: --redirect-secondary ignored because target is already a SECONDARY');
}

mysqlsh([__hostname_ip_uri2, '--redirect-secondary', '--execute', 'println(session)']);

expect_already_secondary();
EXPECT_STDOUT_CONTAINS(`<ClassicSession:root@${hostname_ip}:${__mysql_sandbox_port2}>`);

////////////////////////////////////////////////////////////////////////////////
//@<> connect to secondary, use --redirect-secondary, expect no error - X
mysqlsh([__hostname_ip_x_uri2, '--redirect-secondary', '--execute', 'println(session)']);

expect_already_secondary();
EXPECT_STDOUT_CONTAINS(`<Session:root@${hostname_ip}:${__mysql_sandbox_x_port2}>`);

////////////////////////////////////////////////////////////////////////////////
//@<> connect to secondary, use --redirect-secondary + --replicaset, expect no error
mysqlsh([__hostname_ip_uri2, '--redirect-secondary', '--replicaset', '--execute', 'println(rs.status())']);

expect_rs_variable();

////////////////////////////////////////////////////////////////////////////////
//@<> connect to secondary, use --redirect-secondary + --replicaset - X, expect no error
mysqlsh([__hostname_ip_x_uri2, '--redirect-secondary', '--replicaset', '--execute', 'println(rs.status())']);

expect_rs_variable();

////////////////////////////////////////////////////////////////////////////////
// WL13236-TSFR3_3: Start the Shell with a valid connection data to an instance that is not member of a ReplicaSet and use the --redirect-secondary option. Validate that an exception is thrown because the target instance is not part of a ReplicaSet.

//@<> WL13236-TSFR3_3
function expect_error_redirect_secondary_missing_metadata() {
  EXPECT_STDOUT_CONTAINS('While handling --redirect-secondary:');
  EXPECT_STDOUT_CONTAINS('Metadata schema of an InnoDB cluster or ReplicaSet not found');
}

mysqlsh([__hostname_ip_uri3, '--redirect-secondary', '--execute', 'println(session)']);

expect_error_redirect_secondary_missing_metadata();

//@<> WL13236-TSFR3_3 - X
mysqlsh([__hostname_ip_x_uri3, '--redirect-secondary', '--execute', 'println(session)']);

expect_error_redirect_secondary_missing_metadata();

////////////////////////////////////////////////////////////////////////////////
//@<> connect to an instance that does not belong to a replicaset, use --redirect-secondary + --replicaset, expect error
mysqlsh([__hostname_ip_uri3, '--redirect-secondary', '--replicaset', '--execute', 'println(rs.status())']);

expect_error_redirect_secondary_missing_metadata();

////////////////////////////////////////////////////////////////////////////////
//@<> connect to an instance that does not belong to a replicaset, use --redirect-secondary + --replicaset - X, expect error
mysqlsh([__hostname_ip_x_uri3, '--redirect-secondary', '--replicaset', '--execute', 'println(rs.status())']);

expect_error_redirect_secondary_missing_metadata();

////////////////////////////////////////////////////////////////////////////////
// WL13236-TSFR3_4: Start the Shell with no connection data and use the --redirect-secondary option. Validate that an exception is thrown because no connection data is provided.

//@<> WL13236-TSFR3_4
function expect_error_redirect_secondary_no_session() {
  EXPECT_STDOUT_CONTAINS('ERROR: The --redirect-secondary option requires a session to a member of an InnoDB cluster or ReplicaSet.');
}

mysqlsh(['--redirect-secondary', '--execute', 'println(session)']);

expect_error_redirect_secondary_no_session();

////////////////////////////////////////////////////////////////////////////////
//@<> use --redirect-secondary + --replicaset without connection, expect error
mysqlsh(['--redirect-secondary', '--replicaset', '--execute', 'println(rs.status())']);

expect_error_redirect_secondary_no_session();

////////////////////////////////////////////////////////////////////////////////
//@<> use --cluster with a replicaset member, expect error
function expect_error_cluster_with_replicaset() {
  EXPECT_STDOUT_CONTAINS('Option --cluster requires a session to a member of an InnoDB cluster.');
  EXPECT_STDOUT_CONTAINS('This function is not available through a session to an instance that is a member of an InnoDB ReplicaSet');
}

mysqlsh([__hostname_ip_uri1, '--cluster', '--execute', 'println(cluster.status())']);

expect_error_cluster_with_replicaset();

////////////////////////////////////////////////////////////////////////////////
//@<> use --cluster with a replicaset member - X, expect error
mysqlsh([__hostname_ip_x_uri1, '--cluster', '--execute', 'println(cluster.status())']);

expect_error_cluster_with_replicaset();

////////////////////////////////////////////////////////////////////////////////
//@<> connect to a primary of a replicaset, use --redirect-primary + --cluster, expect error
mysqlsh([__hostname_ip_uri1, '--redirect-primary', '--cluster', '--execute', 'println(cluster.status())']);

expect_already_primary();
expect_error_cluster_with_replicaset();

////////////////////////////////////////////////////////////////////////////////
//@<> connect to a primary of a replicaset, use --redirect-primary + --cluster - X, expect error
mysqlsh([__hostname_ip_x_uri1, '--redirect-primary', '--cluster', '--execute', 'println(cluster.status())']);

expect_already_primary();
expect_error_cluster_with_replicaset();

////////////////////////////////////////////////////////////////////////////////
//@<> connect to a secondary of a replicaset, use --redirect-primary + --cluster, expect error
mysqlsh([__hostname_ip_uri2, '--redirect-primary', '--cluster', '--execute', 'println(cluster.status())']);

EXPECT_STDOUT_CONTAINS(`Reconnecting to the PRIMARY instance of an InnoDB ReplicaSet (${hostname_ip}:${__mysql_sandbox_port1})...`);
expect_error_cluster_with_replicaset();

////////////////////////////////////////////////////////////////////////////////
//@<> connect to a secondary of a replicaset, use --redirect-primary + --cluster - X, expect error
mysqlsh([__hostname_ip_x_uri2, '--redirect-primary', '--cluster', '--execute', 'println(cluster.status())']);

EXPECT_STDOUT_CONTAINS(`Reconnecting to the PRIMARY instance of an InnoDB ReplicaSet (${hostname_ip}:${__mysql_sandbox_x_port1})...`);
expect_error_cluster_with_replicaset();

////////////////////////////////////////////////////////////////////////////////
//@<> connect to a primary of a replicaset, use --redirect-secondary + --cluster, expect error
mysqlsh([__hostname_ip_uri1, '--redirect-secondary', '--cluster', '--execute', 'println(cluster.status())']);

EXPECT_STDOUT_CONTAINS(`Reconnecting to the SECONDARY instance of an InnoDB ReplicaSet (${hostname_ip}:${__mysql_sandbox_port2})...`);
expect_error_cluster_with_replicaset();

////////////////////////////////////////////////////////////////////////////////
//@<> connect to a primary of a replicaset, use --redirect-secondary + --cluster - X, expect error
mysqlsh([__hostname_ip_x_uri1, '--redirect-secondary', '--cluster', '--execute', 'println(cluster.status())']);

EXPECT_STDOUT_CONTAINS(`Reconnecting to the SECONDARY instance of an InnoDB ReplicaSet (${hostname_ip}:${__mysql_sandbox_x_port2})...`);
expect_error_cluster_with_replicaset();

////////////////////////////////////////////////////////////////////////////////
//@<> connect to a secondary of a replicaset, use --redirect-secondary + --cluster, expect error
mysqlsh([__hostname_ip_uri2, '--redirect-secondary', '--cluster', '--execute', 'println(cluster.status())']);

expect_already_secondary();
expect_error_cluster_with_replicaset();

////////////////////////////////////////////////////////////////////////////////
//@<> connect to a secondary of a replicaset, use --redirect-secondary + --cluster - X, expect error
mysqlsh([__hostname_ip_x_uri2, '--redirect-secondary', '--cluster', '--execute', 'println(cluster.status())']);

expect_already_secondary();
expect_error_cluster_with_replicaset();

////////////////////////////////////////////////////////////////////////////////
// try to redirect to non-existing secondary

//@<> stop the secondary
testutil.stopSandbox(__mysql_sandbox_port2, {'wait': true});

//@<> connect to primary, try to redirect to secondary, expect error
function expect_error_no_secondaries() {
  EXPECT_STDOUT_CONTAINS('While handling --redirect-secondary:');
  EXPECT_STDOUT_CONTAINS('No secondaries found in an InnoDB ReplicaSet');
}

mysqlsh([__hostname_ip_uri1, '--redirect-secondary', '--execute', 'println(session)']);

expect_error_no_secondaries();

//@<> connect to primary, try to redirect to secondary, expect error - X
mysqlsh([__hostname_ip_x_uri1, '--redirect-secondary', '--execute', 'println(session)']);

expect_error_no_secondaries();

//@<> restart the secondary
testutil.startSandbox(__mysql_sandbox_port2);
testutil.waitSandboxAlive(__mysql_sandbox_port2);

////////////////////////////////////////////////////////////////////////////////
// try to redirect to non-existing primary

//@<> stop the primary
testutil.stopSandbox(__mysql_sandbox_port1, {'wait': true});

//@<> connect to secondary, try to redirect to primary, expect error
function expect_error_no_primaries() {
  EXPECT_STDOUT_CONTAINS('While handling --redirect-primary:');
  EXPECT_STDOUT_CONTAINS('No primaries found in an InnoDB ReplicaSet');
}

mysqlsh([__hostname_ip_uri2, '--redirect-primary', '--execute', 'println(session)']);

expect_error_no_primaries();

//@<> connect to secondary, try to redirect to primary, expect error - X
mysqlsh([__hostname_ip_x_uri2, '--redirect-primary', '--execute', 'println(session)']);

expect_error_no_primaries();

//@<> restart the primary
testutil.startSandbox(__mysql_sandbox_port1);
testutil.waitSandboxAlive(__mysql_sandbox_port1);

////////////////////////////////////////////////////////////////////////////////

//@<> Cleanup
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
testutil.destroySandbox(__mysql_sandbox_port3);
