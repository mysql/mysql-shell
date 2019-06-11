// Assumptions: smart deployment rountines available
//@ Initialization
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: hostname});
testutil.deploySandbox(__mysql_sandbox_port2, "root", {report_host: hostname});
testutil.deploySandbox(__mysql_sandbox_port3, "root", {report_host: hostname});

function get_cluster_id(cluster_name) {
    var result = session.runSql("SELECT cluster_id FROM mysql_innodb_cluster_metadata.clusters WHERE cluster_name = '" + cluster_name + "'");
    var row = result.fetchOne();
    return row[0];
}

function get_replicaset_ids(cluster_id) {
    var ret_res = "";
    var result = session.runSql("SELECT DISTINCT replicaset_id FROM mysql_innodb_cluster_metadata.replicasets WHERE cluster_id = " + cluster_id);
    var row = result.fetchOne();
    while (row) {
        ret_res += row[0];
        row = result.fetchOne();
        if (row) {
            ret_res += ", "
        }
    }
    return ret_res;
}

function exist_in_metadata_schema(cluster_id, replicaset_ids) {

    var result = session.runSql("SELECT COUNT(*) FROM mysql_innodb_cluster_metadata.clusters WHERE cluster_id = " + cluster_id);
    var row = result.fetchOne();
    if (row[0] != 0) {
        print("Cluster data for cluster_id '" + cluster_id + "' still exists in metadata.");
        return true;
    }
    result = session.runSql("SELECT COUNT(*) FROM mysql_innodb_cluster_metadata.replicasets WHERE cluster_id = " + cluster_id);
    row = result.fetchOne();
    if (row[0] != 0) {
        print("Replicasets data for cluster_id '" + cluster_id + "' still exists in metadata.");
        return true;
    }
    result = session.runSql("SELECT COUNT(*) FROM mysql_innodb_cluster_metadata.instances WHERE replicaset_id IN (" + replicaset_ids + ")");
    row = result.fetchOne();
    if (row[0] != 0) {
        print("Instances data for replicaset_id in '" + cluster_id + "' still exists in metadata.");
        return true;
    } else {
        return false;
    }
}

//@ Create cluster, enable interactive mode.
shell.connect(__sandbox_uri1);
var c = dba.createCluster('c');

//@<> Dissolve cluster, enable interactive mode.
// WL11889 FR1_01: new interactive option to enable interactive mode.
// WL11889 FR2_01: prompt to confirm dissolve in interactive mode.
// WL11889 FR2_02: force option no longer required.
// Regression for BUG#27837231: useless 'force' parameter for dissolve
testutil.expectPrompt("Are you sure you want to dissolve the cluster?", "y");

c.dissolve({interactive: true});

// ensure all accounts are dropped
shell.dumpRows(session.runSql("SELECT user,host FROM mysql.user WHERE user like 'mysql_inno%'"), "tabbed");
session.close();

//@ Create single-primary cluster
shell.connect(__sandbox_uri1);
var singleSession = session;

var single = dba.createCluster('single', {clearReadOnly: true});

//@ Success adding instance 2
testutil.waitMemberState(__mysql_sandbox_port1, "ONLINE");
single.addInstance(__sandbox_uri2);

// Waiting for the added instance to become online
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");

// Wait for the second added instance to fetch all the replication data
testutil.waitMemberTransactions(__mysql_sandbox_port2);

//@ Success adding instance 3
single.addInstance(__sandbox_uri3);

// Waiting for the added instance to become online
testutil.waitMemberState(__mysql_sandbox_port3, "ONLINE");

// Wait for the third added instance to fetch all the replication data
testutil.waitMemberTransactions(__mysql_sandbox_port3);

//@ Get cluster and replicaset ids (single-primary)
// WL11889 FR8_01: cluster data removed from metadata on all online instances.
// Regression for BUG#27833605: dissolve() leaving metadata behind.
var cluster_id = get_cluster_id('single');
var replicaset_ids = get_replicaset_ids(cluster_id);

//@ Success dissolving single-primary cluster
// WL11889 FR3_01: force option not needed to remove cluster.
// Regression for BUG#27837231: useless 'force' parameter for dissolve
single.dissolve();

//@ Cluster.dissolve already dissolved
single.dissolve();

//@<OUT> Verify cluster data removed from metadata on instance 1
// WL11889 FR8_01: cluster data removed from metadata on all online instances.
// Regression for BUG#27833605: dissolve() leaving metadata behind.
exist_in_metadata_schema(cluster_id, replicaset_ids);
session.close();
shell.connect(__sandbox_uri2);

//@<OUT> Verify cluster data removed from metadata on instance 2
// WL11889 FR8_01: cluster data removed from metadata on all online instances.
// Regression for BUG#27833605: dissolve() leaving metadata behind.
exist_in_metadata_schema(cluster_id, replicaset_ids);
session.close();
shell.connect(__sandbox_uri3);

//@<OUT> Verify cluster data removed from metadata on instance 3
// WL11889 FR8_01: cluster data removed from metadata on all online instances.
// Regression for BUG#27833605: dissolve() leaving metadata behind.
exist_in_metadata_schema(cluster_id, replicaset_ids);
session.close();

shell.connect(__sandbox_uri1);
var multiSession = session;

//@ Create multi-primary cluster
var multi = dba.createCluster('multi', {multiPrimary: true, clearReadOnly: true, force: true});

//@ Success adding instance 2 mp
testutil.waitMemberState(__mysql_sandbox_port1, "ONLINE");
multi.addInstance(__sandbox_uri2);

// Waiting for the added instance to become online
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");

// Wait for the second added instance to fetch all the replication data
testutil.waitMemberTransactions(__mysql_sandbox_port2);

//@ Success adding instance 3 mp
multi.addInstance(__sandbox_uri3);

// Waiting for the added instance to become online
testutil.waitMemberState(__mysql_sandbox_port3, "ONLINE");

// Wait for the third added instance to fetch all the replication data
testutil.waitMemberTransactions(__mysql_sandbox_port3);

//@ Get cluster and replicaset ids (multi-primary)
// WL11889 FR8_01: cluster data removed from metadata on all online instances.
// Regression for BUG#27833605: dissolve() leaving metadata behind.
var cluster_id = get_cluster_id('multi');
var replicaset_ids = get_replicaset_ids(cluster_id);

//@ Success dissolving multi-primary cluster
// WL11889 FR3_01: force option not needed to remove cluster.
// Regression for BUG#27837231: useless 'force' parameter for dissolve
multi.dissolve();

//@<OUT> Verify cluster data removed from metadata on instance 1 (multi)
// WL11889 FR8_01: cluster data removed from metadata on all online instances.
// Regression for BUG#27833605: dissolve() leaving metadata behind.
exist_in_metadata_schema(cluster_id, replicaset_ids);
session.close();
shell.connect(__sandbox_uri2);

//@<OUT> Verify cluster data removed from metadata on instance 2 (multi)
// WL11889 FR8_01: cluster data removed from metadata on all online instances.
// Regression for BUG#27833605: dissolve() leaving metadata behind.
exist_in_metadata_schema(cluster_id, replicaset_ids);
session.close();
shell.connect(__sandbox_uri3);

//@<OUT> Verify cluster data removed from metadata on instance 3 (multi)
// WL11889 FR8_01: cluster data removed from metadata on all online instances.
// Regression for BUG#27833605: dissolve() leaving metadata behind.
exist_in_metadata_schema(cluster_id, replicaset_ids);
session.close();

//@ Create single-primary cluster 2
shell.connect(__sandbox_uri1);

var single2 = dba.createCluster('single2', {clearReadOnly: true});

//@ Success adding instance 2 2
testutil.waitMemberState(__mysql_sandbox_port1, "ONLINE");
single2.addInstance(__sandbox_uri2);

// Waiting for the added instance to become online
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");

// Wait for the second added instance to fetch all the replication data
testutil.waitMemberTransactions(__mysql_sandbox_port2);

//@ Success adding instance 3 2
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
single2.dissolve();

//@<> Dissolve fail because one instance is not reachable and force: false.
// WL11889 FR5_01: error if force: false and any instance is unreachable.
single2.dissolve({force: false});

// Regression test for BUG#26001653
//@ Success dissolving cluster 2
// WL11889 FR6_01: force: true to dissovle cluster with unreachable instances.
single2.dissolve({force: true});

// start instance 3
testutil.startSandbox(__mysql_sandbox_port3);
//the timeout for GR plugin to install a new view is 60s, so it should be at
// least that value the parameter for the timeout for the waitForDelayedGRStart
testutil.waitForDelayedGRStart(__mysql_sandbox_port3, 'root', 0);

//@ Create multi-primary cluster 2
var multiSession2 = session;

var multi2 = dba.createCluster('multi2', {clearReadOnly: true, multiPrimary: true, force: true});

//@ Success adding instance 2 mp 2
testutil.waitMemberState(__mysql_sandbox_port1, "ONLINE");
multi2.addInstance(__sandbox_uri2);

// Waiting for the added instance to become online
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");

// Wait for the second added instance to fetch all the replication data
testutil.waitMemberTransactions(__mysql_sandbox_port2);

//@ Success adding instance 3 mp 2
multi2.addInstance(__sandbox_uri3);

// Waiting for the added instance to become online
testutil.waitMemberState(__mysql_sandbox_port3, "ONLINE");

// Wait for the third added instance to fetch all the replication data
testutil.waitMemberTransactions(__mysql_sandbox_port3);

//@ Reset persisted gr_start_on_boot on instance3 {VER(>=8.0.11)}
// NOTE: This trick is required to reuse unreachable servers in tests and avoid
// them from automatically rejoining the group.
// Changing the my.cnf is not enough since persisted variables have precedence.
session.close();
multi2.disconnect();
shell.connect(__sandbox_uri3);
session.runSql("RESET PERSIST group_replication_start_on_boot");
session.close();
shell.connect(__sandbox_uri1);
var multi2 = dba.getCluster('multi2');

//@ stop instance 3
// Use stop sandbox instance to make sure the instance is gone before restarting it
testutil.stopSandbox(__mysql_sandbox_port3);

testutil.waitMemberState(__mysql_sandbox_port3, "(MISSING)");

// Regression test for BUG#26001653
//@ Success dissolving multi-primary cluster 2
// WL11889 FR6_01: force: true to dissovle cluster with unreachable instances.
multi2.dissolve({force: true});

//@ Dissolve post action on unreachable instance (ensure GR is not started)
// NOTE: This is not enough for server version >= 8.0.11 because persisted
// variables take precedence, therefore reseting group_replication_start_on_boot
// is also needed.
testutil.changeSandboxConf(__mysql_sandbox_port3, 'group_replication_start_on_boot', 'OFF');

//@ Restart instance on port3
testutil.startSandbox(__mysql_sandbox_port3);

//@ Create cluster, instance with replication errors.
var c = dba.createCluster('c', {clearReadOnly: true});

//@ Add instance on port2, again.
c.addInstance(__sandbox_uri2);
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");

//@ Add instance on port3, again.
c.addInstance(__sandbox_uri3);
testutil.waitMemberState(__mysql_sandbox_port3, "ONLINE");

//@ Connect to instance on port2 to introduce a replication error
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

//@ Avoid server from aborting (killing itself) {VER(>=8.0.12)}
// NOTE: Starting from version 8.0.12 GR will automatically abort (kill) the
// mysql server if a replication error occurs. The GR exit state action need
// to be changed to avoid this.
// WL11889 - FR9_01 and FR10_01
session.runSql('SET GLOBAL group_replication_exit_state_action = READ_ONLY');

//@ Connect to seed and get cluster
// WL11889 - FR9_01
session.close();
shell.connect(__sandbox_uri1);
var c = dba.getCluster('c');

//@ Change shell option dba.gtidWaitTimeout to 1 second (to issue error faster)
// WL11889 - FR9_01 and FR10_01
shell.options["dba.gtidWaitTimeout"] = 1;

//@ Execute trx that will lead to error on instance2
// WL11889 - FR9_01
session.runSql("CREATE USER 'replication_error'@'%' IDENTIFIED BY 'somepass';");

//@<> Dissolve stopped because instance cannot catch up with cluster (no force option).
// WL11889 FR9_01: error, instance cannot catch up (no force option, default false)
c.dissolve();

//@ Connect to instance on port2 to fix error and add new one
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

//@ Connect to seed and get cluster again
// WL11889 - FR9_01
session.close();
shell.connect(__sandbox_uri1);
var c = dba.getCluster('c');
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");

//@ Execute trx that will lead to error on instance2, again
// WL11889 - FR9_01
session.runSql("CREATE USER 'replication_error_2'@'%' IDENTIFIED BY 'somepass';");

//@<> Dissolve stopped because instance cannot catch up with cluster (force: false).
// WL11889 FR9_01: error, instance cannot catch up (force: false)
c.dissolve({force: false});

//@ Connect to instance on port2 to fix error and add new one, one last time
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

//@ Connect to seed and get cluster one last time
// WL11889 - FR10_01
session.close();
shell.connect(__sandbox_uri1);
var c = dba.getCluster('c');
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");

//@ Execute trx that will lead to error on instance2, one last time
// WL11889 - FR10_01
session.runSql("CREATE USER 'replication_error_3'@'%' IDENTIFIED BY 'somepass';");

//@<> Dissolve continues because instance cannot catch up with cluster (force: true).
// WL11889 FR10_01: continue (no error), instance cannot catch up (force: true)
c.dissolve({force: true});


//@ Finalization
// Will close opened sessions and delete the sandboxes ONLY if this test was executed standalone
session.close();
singleSession.close();
multiSession.close();
multiSession2.close();

testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
testutil.destroySandbox(__mysql_sandbox_port3);
