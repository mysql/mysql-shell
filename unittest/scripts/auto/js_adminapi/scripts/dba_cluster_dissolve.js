// Assumptions: smart deployment rountines available
//@<> INCLUDE dba_utils.inc
//@<> Initialization
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: hostname});
testutil.deploySandbox(__mysql_sandbox_port2, "root", {report_host: hostname});
testutil.deploySandbox(__mysql_sandbox_port3, "root", {report_host: hostname});

var session1 = mysql.getSession(__sandbox_uri1);
var session2 = mysql.getSession(__sandbox_uri2);
var session3 = mysql.getSession(__sandbox_uri3);

function get_cluster_id(cluster_name) {
    var result = session.runSql("SELECT cluster_id FROM mysql_innodb_cluster_metadata.clusters WHERE cluster_name = '" + cluster_name + "'");
    var row = result.fetchOne();
    return row[0];
}

function get_replicaset_ids(cluster_id) {
    var ret_res = "";
    var result = session.runSql("SELECT DISTINCT cluster_id FROM mysql_innodb_cluster_metadata.clusters WHERE cluster_id = ?", [cluster_id]);
    var row = result.fetchOne();
    while (row) {
        ret_res += "'" + row[0] + "'";
        row = result.fetchOne();
        if (row) {
            ret_res += ", "
        }
    }
    return ret_res;
}

//@<> Create cluster, enable interactive mode.
shell.connect(__sandbox_uri1);
var c = dba.createCluster('c', {gtidSetIsComplete: true});

//@<> Dissolve cluster, enable interactive mode.
// WL11889 FR1_01: new interactive option to enable interactive mode.
// WL11889 FR2_01: prompt to confirm dissolve in interactive mode.
// WL11889 FR2_02: force option no longer required.
// Regression for BUG#27837231: useless 'force' parameter for dissolve
testutil.expectPrompt("Are you sure you want to dissolve the cluster?", "y");

c.dissolve({interactive: true});

if (__version_num >= 80011) {
EXPECT_OUTPUT_CONTAINS_MULTILINE(`
The cluster still has the following registered instances:
{
    "clusterName": "c",
    "defaultReplicaSet": {
        "name": "default",
        "topology": [
            {
                "address": "${hostname}:${__mysql_sandbox_port1}",
                "label": "${hostname}:${__mysql_sandbox_port1}",
                "role": "HA"
            }
        ],
        "topologyMode": "Single-Primary"
    }
}
WARNING: You are about to dissolve the whole cluster and lose the high availability features provided by it. This operation cannot be reverted. All members will be removed from the cluster and replication will be stopped, internal recovery user accounts and the cluster metadata will be dropped. User data will be maintained intact in all instances.

Are you sure you want to dissolve the cluster? [y/N]:
* Waiting for instance '${hostname}:${__mysql_sandbox_port1}' to synchronize with the primary...


* Dissolving the Cluster...
* Instance '${hostname}:${__mysql_sandbox_port1}' is attempting to leave the cluster...

The cluster was successfully dissolved.
Replication was disabled but user data was left intact.
`);
} else {
EXPECT_OUTPUT_CONTAINS_MULTILINE(`
The cluster still has the following registered instances:
{
    "clusterName": "c",
    "defaultReplicaSet": {
        "name": "default",
        "topology": [
            {
                "address": "${hostname}:${__mysql_sandbox_port1}",
                "label": "${hostname}:${__mysql_sandbox_port1}",
                "role": "HA"
            }
        ],
        "topologyMode": "Single-Primary"
    }
}
WARNING: You are about to dissolve the whole cluster and lose the high availability features provided by it. This operation cannot be reverted. All members will be removed from the cluster and replication will be stopped, internal recovery user accounts and the cluster metadata will be dropped. User data will be maintained intact in all instances.

Are you sure you want to dissolve the cluster? [y/N]:
* Waiting for instance '${hostname}:${__mysql_sandbox_port1}' to synchronize with the primary...


* Dissolving the Cluster...
* Instance '${hostname}:${__mysql_sandbox_port1}' is attempting to leave the cluster...
WARNING: On instance '${hostname}:${__mysql_sandbox_port1}' configuration cannot be persisted since MySQL version ${__version} does not support the SET PERSIST command (MySQL version >= 8.0.11 required). Please set the 'group_replication_start_on_boot' variable to 'OFF' in the server configuration file, otherwise it might rejoin the cluster upon restart.

The cluster was successfully dissolved.
Replication was disabled but user data was left intact.
`);
}

// ensure all accounts are dropped
CHECK_DISSOLVED_CLUSTER(session);

//@<> Create single-primary cluster
var single = dba.createCluster('single', {clearReadOnly: true, gtidSetIsComplete: true});

//@<> Success adding instance 2
testutil.waitMemberState(__mysql_sandbox_port1, "ONLINE");
single.addInstance(__sandbox_uri2);

// Waiting for the added instance to become online
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");

// Wait for the second added instance to fetch all the replication data
testutil.waitMemberTransactions(__mysql_sandbox_port2);

//@<> Success adding instance 3
single.addInstance(__sandbox_uri3);

// Waiting for the added instance to become online
testutil.waitMemberState(__mysql_sandbox_port3, "ONLINE");

// Wait for the third added instance to fetch all the replication data
testutil.waitMemberTransactions(__mysql_sandbox_port3);

//@<> Get cluster and replicaset ids (single-primary)
// WL11889 FR8_01: cluster data removed from metadata on all online instances.
// Regression for BUG#27833605: dissolve() leaving metadata behind.
var cluster_id = get_cluster_id('single');
var replicaset_ids = get_replicaset_ids(cluster_id);

//@<> Success dissolving single-primary cluster
// WL11889 FR3_01: force option not needed to remove cluster.
// Regression for BUG#27837231: useless 'force' parameter for dissolve
EXPECT_NO_THROWS(function() { single.dissolve(); });

CHECK_DISSOLVED_CLUSTER(session);

//@<> Cluster.dissolve already dissolved
EXPECT_THROWS(function() { single.dissolve(); }, "Can't call function 'dissolve' on an offline cluster");

//@<> Verify cluster data removed from metadata on all instances
// WL11889 FR8_01: cluster data removed from metadata on all online instances.
// Regression for BUG#27833605: dissolve() leaving metadata behind.
CHECK_DISSOLVED_CLUSTER(session);

shell.connect(__sandbox_uri2);
CHECK_DISSOLVED_CLUSTER(session);
session.close();

shell.connect(__sandbox_uri3);
CHECK_DISSOLVED_CLUSTER(session);
session.close();

shell.connect(__sandbox_uri1);

//@<> Create multi-primary cluster
var multi = dba.createCluster('multi', {multiPrimary: true, clearReadOnly: true, force: true, gtidSetIsComplete: true});

//@<> Success adding instance 2 mp
testutil.waitMemberState(__mysql_sandbox_port1, "ONLINE");
multi.addInstance(__sandbox_uri2);

// Waiting for the added instance to become online
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");

// Wait for the second added instance to fetch all the replication data
testutil.waitMemberTransactions(__mysql_sandbox_port2);

//@<> Success adding instance 3 mp
multi.addInstance(__sandbox_uri3);

// Waiting for the added instance to become online
testutil.waitMemberState(__mysql_sandbox_port3, "ONLINE");

// Wait for the third added instance to fetch all the replication data
testutil.waitMemberTransactions(__mysql_sandbox_port3);

//@<> Get cluster and replicaset ids (multi-primary)
// WL11889 FR8_01: cluster data removed from metadata on all online instances.
// Regression for BUG#27833605: dissolve() leaving metadata behind.
var cluster_id = get_cluster_id('multi');
var replicaset_ids = get_replicaset_ids(cluster_id);

//@<> Success dissolving multi-primary cluster
// WL11889 FR3_01: force option not needed to remove cluster.
// Regression for BUG#27837231: useless 'force' parameter for dissolve
EXPECT_NO_THROWS(function() { multi.dissolve(); });

//@<> Verify cluster data removed from metadata on all instances (multi)
// WL11889 FR8_01: cluster data removed from metadata on all online instances.
// Regression for BUG#27833605: dissolve() leaving metadata behind.
CHECK_DISSOLVED_CLUSTER(session);

session.close();
shell.connect(__sandbox_uri2);
CHECK_DISSOLVED_CLUSTER(session);

session.close();
shell.connect(__sandbox_uri3);
CHECK_DISSOLVED_CLUSTER(session);

//@<> Disable group_replication_enforce_update_everywhere_checks for 5.7 {VER(<8.0.0)}
// NOTE: Disable of enforce_update_everywhere_checks must be done manually for
//       5.7 servers in order to reuse them (in single-primary clusters).
//       since they don't support SET PERSIST.
session1.runSql("SET GLOBAL group_replication_enforce_update_everywhere_checks = 'OFF'");
session2.runSql("SET GLOBAL group_replication_enforce_update_everywhere_checks = 'OFF'");
session3.runSql("SET GLOBAL group_replication_enforce_update_everywhere_checks = 'OFF'");

//@<> Create single-primary cluster 2
shell.connect(__sandbox_uri1);

var single2 = dba.createCluster('single2', {clearReadOnly: true, gtidSetIsComplete: true});

//@<> Success adding instance 2 2
testutil.waitMemberState(__mysql_sandbox_port1, "ONLINE");
single2.addInstance(__sandbox_uri2);

// Waiting for the added instance to become online
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");

// Wait for the second added instance to fetch all the replication data
testutil.waitMemberTransactions(__mysql_sandbox_port2);

//@<> Success adding instance 3 2
single2.addInstance(__sandbox_uri3);

// Waiting for the added instance to become online
testutil.waitMemberState(__mysql_sandbox_port3, "ONLINE");

// Wait for the third added instance to fetch all the replication data
testutil.waitMemberTransactions(__mysql_sandbox_port3);

//@<> Reset persisted gr_start_on_boot on instance 3
session.close();
disable_auto_rejoin(__mysql_sandbox_port3);
shell.connect(__sandbox_uri1);

// stop instance 3
// Use stop sandbox instance to make sure the instance is gone before restarting it
testutil.stopSandbox(__mysql_sandbox_port3);

testutil.waitMemberState(__mysql_sandbox_port3, "(MISSING)");

//@<> Dissolve fail because one instance is not reachable.
// WL11889 FR4_01: force: true required to dissolve cluster with unreachable instances.
EXPECT_THROWS_TYPE(function() { single2.dissolve(); }, "Cluster.dissolve: The instance '<<<hostname>>>:<<<__mysql_sandbox_port3>>>' is '(MISSING)'", "RuntimeError");

EXPECT_OUTPUT_CONTAINS(`ERROR: The instance '${hostname}:${__mysql_sandbox_port3}' cannot be removed because it is on a '(MISSING)' state. Please bring the instance back ONLINE and try to dissolve the cluster again. If the instance is permanently not reachable, then please use <Cluster>.dissolve() with the force option set to true to proceed with the operation and only remove the instance from the Cluster Metadata.`);

//@<> Dissolve fail because one instance is not reachable and force: false.
// WL11889 FR5_01: error if force: false and any instance is unreachable.
EXPECT_THROWS_TYPE(function() { single2.dissolve({force: false}); }, "Cluster.dissolve: The instance '<<<hostname>>>:<<<__mysql_sandbox_port3>>>' is '(MISSING)'", "RuntimeError");

EXPECT_OUTPUT_CONTAINS(`ERROR: The instance '${hostname}:${__mysql_sandbox_port3}' cannot be removed because it is on a '(MISSING)' state. Please bring the instance back ONLINE and try to dissolve the cluster again. If the instance is permanently not reachable, then please use <Cluster>.dissolve() with the force option set to true to proceed with the operation and only remove the instance from the Cluster Metadata.`);

// Regression test for BUG#26001653
//@<> Success dissolving cluster 2
// WL11889 FR6_01: force: true to dissovle cluster with unreachable instances.
EXPECT_NO_THROWS(function() { single2.dissolve({force: true}); });
CHECK_DISSOLVED_CLUSTER(session);

// start instance 3
testutil.startSandbox(__mysql_sandbox_port3);
//the timeout for GR plugin to install a new view is 60s, so it should be at
// least that value the parameter for the timeout for the waitForDelayedGRStart
testutil.waitForDelayedGRStart(__mysql_sandbox_port3, 'root', 0);

//@<> Create multi-primary cluster 2
var multi2 = dba.createCluster('multi2', {clearReadOnly: true, multiPrimary: true, force: true, gtidSetIsComplete: true});

//@<> Success adding instance 2 mp 2
testutil.waitMemberState(__mysql_sandbox_port1, "ONLINE");
multi2.addInstance(__sandbox_uri2);

// Waiting for the added instance to become online
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");

// Wait for the second added instance to fetch all the replication data
testutil.waitMemberTransactions(__mysql_sandbox_port2);

//@<> Success adding instance 3 mp 2
multi2.addInstance(__sandbox_uri3);

// Waiting for the added instance to become online
testutil.waitMemberState(__mysql_sandbox_port3, "ONLINE");

// Wait for the third added instance to fetch all the replication data
testutil.waitMemberTransactions(__mysql_sandbox_port3);

//@<> Reset persisted gr_start_on_boot on instance3 {VER(>=8.0.11)}
// NOTE: This trick is required to reuse unreachable servers in tests and avoid
// them from automatically rejoining the group.
// Changing the my.cnf is not enough since persisted variables have precedence.
session.close();
multi2.disconnect();
shell.connect(__sandbox_uri3);
session.runSql("RESET PERSIST group_replication_start_on_boot");
session.runSql("RESET PERSIST group_replication_enforce_update_everywhere_checks");
session.close();
shell.connect(__sandbox_uri1);
var multi2 = dba.getCluster('multi2');

//@<> stop instance 3
// Use stop sandbox instance to make sure the instance is gone before restarting it
testutil.stopSandbox(__mysql_sandbox_port3);
testutil.waitMemberState(__mysql_sandbox_port3, "(MISSING)");

// Regression test for BUG#26001653
//@<> Success dissolving multi-primary cluster 2
// WL11889 FR6_01: force: true to dissovle cluster with unreachable instances.
EXPECT_NO_THROWS(function() { multi2.dissolve({force: true}); });

//@<> Disable enforce_update_everywhere_checks for 5.7 {VER(<8.0.0)}
// NOTE: Disable of enforce_update_everywhere_checks must be done manually for
//       5.7 servers in order to reuse them (in single-primary clusters).
//       since they don't support SET PERSIST.
session1.runSql("SET GLOBAL group_replication_enforce_update_everywhere_checks = 'OFF'");
session2.runSql("SET GLOBAL group_replication_enforce_update_everywhere_checks = 'OFF'");

//@<> Dissolve post action on unreachable instance (ensure GR is not started)
// NOTE: This is not enough for server version >= 8.0.11 because persisted
// variables take precedence, therefore reseting group_replication_start_on_boot
// is also needed.
testutil.changeSandboxConf(__mysql_sandbox_port3, 'group_replication_start_on_boot', 'OFF');
// NOTE: group_replication_enforce_update_everywhere_checks must be disabled
//       manually because the instance was previously part of a multi-primary
//       cluster that was dissolved without it, otherwise an error is issued
//       when trying to add it to a single-primary cluster.
testutil.changeSandboxConf(__mysql_sandbox_port3, 'group_replication_enforce_update_everywhere_checks', 'OFF');

//@<> Restart instance on port3
testutil.startSandbox(__mysql_sandbox_port3);

//@<> Create cluster, instance with replication errors.
var c = dba.createCluster('c', {clearReadOnly: true, gtidSetIsComplete: true});

//@<> Add instance on port2, again.
c.addInstance(__sandbox_uri2);
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");

//@<> Add instance on port3, again.
c.addInstance(__sandbox_uri3);
testutil.waitMemberState(__mysql_sandbox_port3, "ONLINE");

//@<> Connect to instance on port2 to introduce a replication error
// IMPORTANT NOTE: GR members with replication errors are automatically removed
// from the GR group after a short (non-deterministic) time, becoming
// unreachable '(MISSING)'. Therefore, this replication error scenario is
// temporary and created immediately before each dissolve in an attempt to
// verify the desired test case. If the test fail because the instance is
// unreachable then remove this test case since it cannot be tested in a
// deterministic way.
// WL11889 - FR9_01
session.close();
c.disconnect();
shell.connect(__sandbox_uri2);
session.runSql("SET sql_log_bin = 0");
session.runSql("SET GLOBAL super_read_only = 0");
session.runSql("CREATE USER 'replication_error'@'%' IDENTIFIED BY 'somepass';");
session.runSql("SET GLOBAL super_read_only = 1");
session.runSql("SET sql_log_bin = 1");

//@<> Avoid server from aborting (killing itself) {VER(>=8.0.12)}
// NOTE: Starting from version 8.0.12 GR will automatically abort (kill) the
// mysql server if a replication error occurs. The GR exit state action need
// to be changed to avoid this.
// WL11889 - FR9_01 and FR10_01
session.runSql('SET GLOBAL group_replication_exit_state_action = READ_ONLY');

//@<> Connect to seed and get cluster
// WL11889 - FR9_01
session.close();
shell.connect(__sandbox_uri1);
var c = dba.getCluster('c');

//@<> Change shell option dba.gtidWaitTimeout to 5 second
// WL11889 - FR9_01 and FR10_01
shell.options["dba.gtidWaitTimeout"] = 5;

//@<> Execute trx that will lead to error on instance2
// WL11889 - FR9_01
session.runSql("CREATE USER 'replication_error'@'%' IDENTIFIED BY 'somepass';");

//@<> Dissolve stopped because instance cannot catch up with cluster (no force option).
// WL11889 FR9_01: error, instance cannot catch up (no force option, default false)
if (__version_num < 80023) {
EXPECT_THROWS_TYPE(function() { c.dissolve(); }, "Cluster.dissolve: " + hostname + ":" + __mysql_sandbox_port2 + ": Error found in replication applier thread", "MYSQLSH");

EXPECT_STDOUT_CONTAINS("ERROR: Applier error in replication channel 'group_replication_applier':");
} else {
EXPECT_THROWS_TYPE(function() { c.dissolve(); }, "Cluster.dissolve: " + hostname + ":" + __mysql_sandbox_port2 + ": Error found in replication coordinator thread", "MYSQLSH");

EXPECT_STDOUT_CONTAINS("ERROR: Coordinator error in replication channel 'group_replication_applier':");
}

EXPECT_STDOUT_CONTAINS(`ERROR: The instance '${hostname}:${__mysql_sandbox_port2}' was unable to catch up with cluster transactions. There might be too many transactions to apply or some replication error. In the former case, you can retry the operation (using a higher timeout value by setting the global shell option 'dba.gtidWaitTimeout'). In the later case, analyze and fix any replication error. You can also choose to skip this error using the 'force: true' option, but it might leave the instance in an inconsistent state and lead to errors if you want to reuse it.`);

//@<> Connect to instance on port2 to fix error and add new one
// IMPORTANT NOTE: GR members with replication errors are automatically removed
// from the GR group after a short (non-deterministic) time, becoming
// unreachable '(MISSING)'. Therefore, this replication error scenario is
// temporary and created immediately before each dissolve in an attempt to
// verify the desired test case. If the test fail because the instance is
// unreachable then remove this test case since it cannot be tested in a
// deterministic way.
// WL11889 - FR9_01
session.close();
c.disconnect();
shell.connect(__sandbox_uri2);
session.runSql("STOP GROUP_REPLICATION");
session.runSql("SET sql_log_bin = 0");
session.runSql("SET GLOBAL super_read_only = 0");
session.runSql("DROP USER 'replication_error'@'%'");
session.runSql("CREATE USER 'replication_error_2'@'%' IDENTIFIED BY 'somepass';");
session.runSql("SET GLOBAL super_read_only = 1");
session.runSql("SET sql_log_bin = 1");
session.runSql("START GROUP_REPLICATION");

//@<> Connect to seed and get cluster again
// WL11889 - FR9_01
session.close();
shell.connect(__sandbox_uri1);
var c = dba.getCluster('c');
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");

//@<> Execute trx that will lead to error on instance2, again
// WL11889 - FR9_01
session.runSql("CREATE USER 'replication_error_2'@'%' IDENTIFIED BY 'somepass';");

//@<> Dissolve stopped because instance cannot catch up with cluster (force: false).
// WL11889 FR9_01: error, instance cannot catch up (force: false)
if (__version_num < 80023) {
EXPECT_THROWS_TYPE(function() { c.dissolve({force: false}); }, "Cluster.dissolve: " + hostname + ":" + __mysql_sandbox_port2 + ": Error found in replication applier thread", "MYSQLSH");

EXPECT_STDOUT_CONTAINS("ERROR: Applier error in replication channel 'group_replication_applier':");
} else {
EXPECT_THROWS_TYPE(function() { c.dissolve({force: false}); }, "Cluster.dissolve: " + hostname + ":" + __mysql_sandbox_port2 + ": Error found in replication coordinator thread", "MYSQLSH");

EXPECT_STDOUT_CONTAINS("ERROR: Coordinator error in replication channel 'group_replication_applier':");
}

EXPECT_STDOUT_CONTAINS(`ERROR: The instance '${hostname}:${__mysql_sandbox_port2}' was unable to catch up with cluster transactions. There might be too many transactions to apply or some replication error. In the former case, you can retry the operation (using a higher timeout value by setting the global shell option 'dba.gtidWaitTimeout'). In the later case, analyze and fix any replication error. You can also choose to skip this error using the 'force: true' option, but it might leave the instance in an inconsistent state and lead to errors if you want to reuse it.`);

//@<> Connect to instance on port2 to fix error and add new one, one last time
// IMPORTANT NOTE: GR members with replication errors are automatically removed
// from the GR group after a short (non-deterministic) time, becoming
// unreachable '(MISSING)'. Therefore, this replication error scenario is
// temporary and created immediately before each dissolve in an attempt to
// verify the desired test case. If the test fail because the instance is
// unreachable then remove this test case since it cannot be tested in a
// deterministic way.
// WL11889 - FR10_01
session.close();
c.disconnect();
shell.connect(__sandbox_uri2);
session.runSql("STOP GROUP_REPLICATION");
session.runSql("SET sql_log_bin = 0");
session.runSql("SET GLOBAL super_read_only = 0");
session.runSql("DROP USER 'replication_error_2'@'%'");
session.runSql("CREATE USER 'replication_error_3'@'%' IDENTIFIED BY 'somepass';");
session.runSql("SET GLOBAL super_read_only = 1");
session.runSql("SET sql_log_bin = 1");
session.runSql("START GROUP_REPLICATION");

//@<> Connect to seed and get cluster one last time
// WL11889 - FR10_01
session.close();
shell.connect(__sandbox_uri1);
var c = dba.getCluster('c');
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");

//@<> Execute trx that will lead to error on instance2, one last time
// WL11889 - FR10_01
session.runSql("CREATE USER 'replication_error_3'@'%' IDENTIFIED BY 'somepass';");

//@<> Dissolve continues because instance cannot catch up with cluster (force: true).
// WL11889 FR10_01: continue (no error), instance cannot catch up (force: true)
c.dissolve({force: true});

if (__version_num < 80011) {
EXPECT_OUTPUT_CONTAINS_MULTILINE(`
WARNING: An error occurred when trying to catch up with cluster transactions and instance '${hostname}:${__mysql_sandbox_port2}' might have been left in an inconsistent state that will lead to errors if it is reused.

* Instance '${hostname}:${__mysql_sandbox_port2}' is attempting to leave the cluster...
WARNING: On instance '${hostname}:${__mysql_sandbox_port2}' configuration cannot be persisted since MySQL version ${__version} does not support the SET PERSIST command (MySQL version >= 8.0.11 required). Please set the 'group_replication_start_on_boot' variable to 'OFF' in the server configuration file, otherwise it might rejoin the cluster upon restart.
* Waiting for instance '${hostname}:${__mysql_sandbox_port3}' to synchronize with the primary...


* Instance '${hostname}:${__mysql_sandbox_port3}' is attempting to leave the cluster...
WARNING: On instance '${hostname}:${__mysql_sandbox_port3}' configuration cannot be persisted since MySQL version ${__version} does not support the SET PERSIST command (MySQL version >= 8.0.11 required). Please set the 'group_replication_start_on_boot' variable to 'OFF' in the server configuration file, otherwise it might rejoin the cluster upon restart.
* Instance '${hostname}:${__mysql_sandbox_port1}' is attempting to leave the cluster...
WARNING: On instance '${hostname}:${__mysql_sandbox_port1}' configuration cannot be persisted since MySQL version ${__version} does not support the SET PERSIST command (MySQL version >= 8.0.11 required). Please set the 'group_replication_start_on_boot' variable to 'OFF' in the server configuration file, otherwise it might rejoin the cluster upon restart.

WARNING: The cluster was successfully dissolved, but the following instance was unable to catch up with the cluster transactions: '${hostname}:${__mysql_sandbox_port2}'. Please make sure the cluster metadata was removed on this instance in order to be able to be reused.
`);
} else {
EXPECT_OUTPUT_CONTAINS_MULTILINE(`
WARNING: An error occurred when trying to catch up with cluster transactions and instance '${hostname}:${__mysql_sandbox_port2}' might have been left in an inconsistent state that will lead to errors if it is reused.

* Instance '${hostname}:${__mysql_sandbox_port2}' is attempting to leave the cluster...
* Waiting for instance '${hostname}:${__mysql_sandbox_port3}' to synchronize with the primary...


* Instance '${hostname}:${__mysql_sandbox_port3}' is attempting to leave the cluster...
* Instance '${hostname}:${__mysql_sandbox_port1}' is attempting to leave the cluster...

WARNING: The cluster was successfully dissolved, but the following instance was unable to catch up with the cluster transactions: '${hostname}:${__mysql_sandbox_port2}'. Please make sure the cluster metadata was removed on this instance in order to be able to be reused.
`);
}

//@<> Finalization
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
testutil.destroySandbox(__mysql_sandbox_port3);
