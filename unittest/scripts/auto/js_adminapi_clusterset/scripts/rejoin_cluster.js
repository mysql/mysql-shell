//@ {VER(>=8.0.27)}

//@<> INCLUDE clusterset_utils.inc

//@<> Setup

function invalidate_cluster(cluster, pcluster) {
  var session = mysql.getSession("mysql://root:root@"+pcluster.status()["defaultReplicaSet"]["primary"]);

  var csid = session.runSql("select clusterset_id from mysql_innodb_cluster_metadata.clustersets").fetchOne()[0];
  var c1id = session.runSql("select cluster_id from mysql_innodb_cluster_metadata.clusters where cluster_name='cluster1'").fetchOne()[0];
  var c2id = session.runSql("select cluster_id from mysql_innodb_cluster_metadata.clusters where cluster_name='cluster2'").fetchOne()[0];

  session.runSql("start transaction");
  session.runSql("call mysql_innodb_cluster_metadata.v2_cs_primary_changed(?, ?, '{}')", [csid, c1id]);
  session.runSql("call mysql_innodb_cluster_metadata.v2_cs_add_invalidated_member(?, ?)", [csid, c2id]);
  session.runSql("commit");
}

testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host:hostname});
testutil.deploySandbox(__mysql_sandbox_port2, "root", {report_host:hostname});
testutil.deploySandbox(__mysql_sandbox_port3, "root", {report_host:hostname});
testutil.deploySandbox(__mysql_sandbox_port4, "root", {report_host:hostname});
testutil.deploySandbox(__mysql_sandbox_port5, "root", {report_host:hostname});
testutil.deploySandbox(__mysql_sandbox_port6, "root", {report_host:hostname});

session1 = mysql.getSession(__sandbox_uri1);
session2 = mysql.getSession(__sandbox_uri2);
session3 = mysql.getSession(__sandbox_uri3);
session4 = mysql.getSession(__sandbox_uri4);
session5 = mysql.getSession(__sandbox_uri5);
session6 = mysql.getSession(__sandbox_uri6);

//@<> Create a 3/3 clusterset

shell.connect(__sandbox_uri1);
c1 = dba.createCluster("cluster1", {gtidSetIsComplete:1, manualStartOnBoot:1});
c1.addInstance(__sandbox_uri2);

cs = c1.createClusterSet("cs");

c2 = cs.createReplicaCluster(__sandbox_uri4, "cluster2", {manualStartOnBoot:1});
c2.addInstance(__sandbox_uri5);
c1.addInstance(__sandbox_uri3);
c2.addInstance(__sandbox_uri6);

CHECK_PRIMARY_CLUSTER([__sandbox_uri1, __sandbox_uri2, __sandbox_uri3], c1);
CHECK_REPLICA_CLUSTER([__sandbox_uri4, __sandbox_uri5, __sandbox_uri6], c1, c2);
CHECK_CLUSTER_SET(session);

//@<> rejoin non-invalidated cluster should just fix repl configs if wrong, or no-op if all ok

var view_id = session.runSql("select view_id from mysql_innodb_cluster_metadata.v2_cs_members").fetchOne()[0];

// dryRun -> no-op
EXPECT_DRYRUN(function(){cs.rejoinCluster("cluster1", {dryRun:1});}, __mysql_sandbox_port1);

cs.rejoinCluster("cluster1");

CHECK_PRIMARY_CLUSTER([__sandbox_uri1, __sandbox_uri2, __sandbox_uri3], c1);
CHECK_REPLICA_CLUSTER([__sandbox_uri4, __sandbox_uri5, __sandbox_uri6], c1, c2);
CHECK_CLUSTER_SET(session);

EXPECT_DRYRUN(function(){cs.rejoinCluster("cluster2", {dryRun:1});}, __mysql_sandbox_port1);
cs.rejoinCluster("cluster2");

EXPECT_EQ(view_id, session.runSql("select view_id from mysql_innodb_cluster_metadata.v2_cs_members").fetchOne()[0]);

CHECK_PRIMARY_CLUSTER([__sandbox_uri1, __sandbox_uri2, __sandbox_uri3], c1);
CHECK_REPLICA_CLUSTER([__sandbox_uri4, __sandbox_uri5, __sandbox_uri6], c1, c2);
CHECK_CLUSTER_SET(session);

//@<> rejoin non-invalidated cluster should just fix repl if channel is stopped

var view_id = session.runSql("select view_id from mysql_innodb_cluster_metadata.v2_cs_members").fetchOne()[0];

session4.runSql("stop replica for channel 'clusterset_replication'");

session4.runSql("select * from mysql_innodb_cluster_metadata.clusterset_members");

// dryRun -> no-op
EXPECT_DRYRUN(function(){cs.rejoinCluster("cluster2", {dryRun:1});}, __mysql_sandbox_port1);

cs.rejoinCluster("cluster2");

EXPECT_EQ(view_id, session.runSql("select view_id from mysql_innodb_cluster_metadata.v2_cs_members").fetchOne()[0]);

CHECK_PRIMARY_CLUSTER([__sandbox_uri1, __sandbox_uri2, __sandbox_uri3], c1);
CHECK_REPLICA_CLUSTER([__sandbox_uri4, __sandbox_uri5, __sandbox_uri6], c1, c2);
CHECK_CLUSTER_SET(session);


//@<> rejoin non-invalidated cluster should just fix repl configs if channel is missing (primary)

session4.runSql("stop replica for channel 'clusterset_replication'");
session4.runSql("reset replica all for channel 'clusterset_replication'");

// dryRun -> no-op
EXPECT_DRYRUN(function(){cs.rejoinCluster("cluster2", {dryRun:1});}, __mysql_sandbox_port1);

cs.rejoinCluster("cluster2");

CHECK_PRIMARY_CLUSTER([__sandbox_uri1, __sandbox_uri2, __sandbox_uri3], c1);
CHECK_REPLICA_CLUSTER([__sandbox_uri4, __sandbox_uri5, __sandbox_uri6], c1, c2);
CHECK_CLUSTER_SET(session);


//@<> rejoin non-invalidated cluster should just fix repl configs if channel is missing (secondary)

session5.runSql("reset replica all for channel 'clusterset_replication'");

cs.rejoinCluster("cluster2");

CHECK_PRIMARY_CLUSTER([__sandbox_uri1, __sandbox_uri2, __sandbox_uri3], c1);
CHECK_REPLICA_CLUSTER([__sandbox_uri4, __sandbox_uri5, __sandbox_uri6], c1, c2);
CHECK_CLUSTER_SET(session);

//@<> rejoin primary cluster with bogus channel running

//@<> bad cluster
EXPECT_THROWS(function(){cs.rejoinCluster("cluster3");}, "The cluster with the name 'cluster3' does not exist.");

//@<> rejoin invalidated replica cluster
invalidate_cluster(c2, c1);

CHECK_PRIMARY_CLUSTER([__sandbox_uri1, __sandbox_uri2, __sandbox_uri3], c1);
CHECK_INVALIDATED_CLUSTER([__sandbox_uri4, __sandbox_uri5, __sandbox_uri6], c1, c2);
CHECK_CLUSTER_SET(session);

// dryRun -> no-op
EXPECT_DRYRUN(function(){cs.rejoinCluster("cluster2", {dryRun:1});}, __mysql_sandbox_port1);

// this rejoin will have no errant transactions
cs.rejoinCluster("cluster2");

CHECK_PRIMARY_CLUSTER([__sandbox_uri1, __sandbox_uri2, __sandbox_uri3], c1);
CHECK_REPLICA_CLUSTER([__sandbox_uri4, __sandbox_uri5, __sandbox_uri6], c1, c2);
CHECK_CLUSTER_SET(session);

//@<> rejoin after account password reset for the cluster
invalidate_cluster(c2, c1);
repl_user = session4.runSql("show replica status for channel 'clusterset_replication'").fetchOne()[2];
session4.runSql("stop replica for channel 'clusterset_replication'");

session1.runSql("set password for ?@'%'='blarg'", [repl_user]);
cs.rejoinCluster("cluster2");

CHECK_PRIMARY_CLUSTER([__sandbox_uri1, __sandbox_uri2, __sandbox_uri3], c1);
CHECK_REPLICA_CLUSTER([__sandbox_uri4, __sandbox_uri5, __sandbox_uri6], c1, c2);
CHECK_CLUSTER_SET(session);

//@<> rejoin cluster with extra GTID events that are view-change-events

// Stop the CS replication channel
session4.runSql("stop replica for channel 'clusterset_replication'");

// Generate view change events in the replica Cluster
session6.runSql("STOP group_replication");
session6.runSql("START group_replication");

// Rejoin the Cluster
cs.rejoinCluster("cluster2");
if (__version_num < 80300) {
    EXPECT_OUTPUT_CONTAINS("* Reconciling 1 internally generated GTIDs");
}

// Purge the binary logs from all members
// BUG#34013718: reconciliation doesn't happen when binlogs purged from all members
session4.runSql("flush binary logs");
session5.runSql("flush binary logs");
session6.runSql("flush binary logs");
os.sleep(0.3);
session4.runSql("purge binary logs before now(6)");
session5.runSql("purge binary logs before now(6)");
session6.runSql("purge binary logs before now(6)");

// promoting cluster2 to primary should be OK
EXPECT_NO_THROWS(function() { cs.setPrimaryCluster("cluster2"); } );

testutil.waitMemberTransactions(__mysql_sandbox_port1, __mysql_sandbox_port4);

CHECK_PRIMARY_CLUSTER([__sandbox_uri4, __sandbox_uri6], c2);
CHECK_REPLICA_CLUSTER([__sandbox_uri1, __sandbox_uri2, __sandbox_uri3], c2, c1);
CHECK_CLUSTER_SET(session);

// promoting cluster1 back to primary
EXPECT_NO_THROWS(function() { cs.setPrimaryCluster("cluster1"); } );

testutil.waitMemberTransactions(__mysql_sandbox_port4, __mysql_sandbox_port1);

CHECK_PRIMARY_CLUSTER([__sandbox_uri1, __sandbox_uri2, __sandbox_uri3], c1);
CHECK_REPLICA_CLUSTER([__sandbox_uri4, __sandbox_uri6], c1, c2);
CHECK_CLUSTER_SET(session);

//@<> rejoin partially offline cluster

invalidate_cluster(c2, c1);

// note: we can't stop the primary because there's no AR failover right now
session5.runSql("stop group_replication");

cs.rejoinCluster("cluster2");

CHECK_PRIMARY_CLUSTER([__sandbox_uri1, __sandbox_uri2, __sandbox_uri3], c1);
CHECK_REPLICA_CLUSTER([__sandbox_uri4, __sandbox_uri6], c1, c2);
CHECK_CLUSTER_SET(session);

//@<> rejoin offline cluster (should fail)

invalidate_cluster(c2, c1);

cs.status();

testutil.stopGroup([__mysql_sandbox_port4,__mysql_sandbox_port6]);

EXPECT_THROWS(function() {
    cs.rejoinCluster("cluster2", {dryRun:1});
}, "The Cluster 'cluster2' is reachable but OFFLINE.");
EXPECT_OUTPUT_CONTAINS("The Cluster 'cluster2' is reachable but OFFLINE. Use the dba.rebootClusterFromCompleteOutage() command to restore it.");

EXPECT_THROWS(function() {
    cs.rejoinCluster("cluster2");
}, "The Cluster 'cluster2' is reachable but OFFLINE.");
EXPECT_OUTPUT_CONTAINS("The Cluster 'cluster2' is reachable but OFFLINE. Use the dba.rebootClusterFromCompleteOutage() command to restore it.");

CHECK_PRIMARY_CLUSTER([__sandbox_uri1, __sandbox_uri2, __sandbox_uri3], c1);
CHECK_INVALIDATED_CLUSTER([__sandbox_uri4, __sandbox_uri5, __sandbox_uri6], c1, c2);
CHECK_CLUSTER_SET(session);

// undo invalidation
delete_last_view(session1);

shell.connect(__sandbox_uri4);

c2 = dba.rebootClusterFromCompleteOutage();

testutil.waitMemberTransactions(__mysql_sandbox_port4, __mysql_sandbox_port1);

c2.rejoinInstance(__sandbox_uri5);
c2.rejoinInstance(__sandbox_uri6);
testutil.waitMemberTransactions(__mysql_sandbox_port5, __mysql_sandbox_port4);

//@<> Try to rejoin an unreachable replica and check the exception / error message (BUG#34535236)
shell.connect(__sandbox_uri4);

testutil.killSandbox(__mysql_sandbox_port5);
testutil.waitMemberState(__mysql_sandbox_port5, "UNREACHABLE");
testutil.killSandbox(__mysql_sandbox_port6);
testutil.waitMemberState(__mysql_sandbox_port6, "UNREACHABLE");

shell.connect(__sandbox_uri1);

testutil.killSandbox(__mysql_sandbox_port4);

cs = dba.getClusterSet();

EXPECT_THROWS(function() {
    cs.rejoinCluster("cluster2");
}, "Could not connect to a PRIMARY member of cluster 'cluster2'");
EXPECT_OUTPUT_CONTAINS("Could not reach cluster 'cluster2'");

testutil.startSandbox(__mysql_sandbox_port4);
testutil.startSandbox(__mysql_sandbox_port5);
testutil.startSandbox(__mysql_sandbox_port6);

shell.connect(__sandbox_uri4);

c2 = dba.rebootClusterFromCompleteOutage();

session4 = mysql.getSession(__sandbox_uri4);
session5 = mysql.getSession(__sandbox_uri5);
session6 = mysql.getSession(__sandbox_uri6);

//@<> rejoin no_quorum (should fail)
c2.status();

session6.runSql("stop group_replication");
testutil.waitMemberState(__mysql_sandbox_port6, "(MISSING)");
testutil.killSandbox(__mysql_sandbox_port5);
testutil.waitMemberState(__mysql_sandbox_port5, "UNREACHABLE");

EXPECT_THROWS(function(){cs.rejoinCluster("cluster2", {dryRun:1});}, "Cluster 'cluster2' has no quorum");

EXPECT_THROWS(function(){cs.rejoinCluster("cluster2");}, "Cluster 'cluster2' has no quorum");

//@<> rejoin restored quorum

// channel was reset by previous test, we just restore the cluster now
c2.forceQuorumUsingPartitionOf(__sandbox_uri4);

testutil.startSandbox(__mysql_sandbox_port5);
c2.rejoinInstance(__sandbox_uri5);
c2.rejoinInstance(__sandbox_uri6);

c2.status();

cs.rejoinCluster("cluster2");

CHECK_PRIMARY_CLUSTER([__sandbox_uri1, __sandbox_uri2, __sandbox_uri3], c1);
CHECK_REPLICA_CLUSTER([__sandbox_uri4, __sandbox_uri5, __sandbox_uri6], c1, c2);
CHECK_CLUSTER_SET(session);

//@<> rejoin with gtids that were created at the primary after invalidation (should pass)
invalidate_cluster(c2, c1);
session4.runSql("stop replica for channel 'clusterset_replication'");

session1.runSql("create schema newschema1");

EXPECT_DRYRUN(function(){cs.rejoinCluster("cluster2", {dryRun:1});}, __mysql_sandbox_port1);

cs.rejoinCluster("cluster2");
CHECK_PRIMARY_CLUSTER([__sandbox_uri1, __sandbox_uri2, __sandbox_uri3], c1);
CHECK_REPLICA_CLUSTER([__sandbox_uri4, __sandbox_uri5, __sandbox_uri6], c1, c2);
CHECK_CLUSTER_SET(session);

//@<> rejoin with gtids that were purged from the primary but not secondaries (should pass)
invalidate_cluster(c2, c1);
session4.runSql("stop replica for channel 'clusterset_replication'");

session1.runSql("set @gtid_before=@@gtid_executed");
session1.runSql("create schema newschema2");
gtid_newschema2 = session1.runSql("select gtid_subtract(@@gtid_executed, @gtid_before)").fetchOne()[0];

session1.runSql("select @@gtid_executed");
session1.runSql("flush binary logs");
os.sleep(1);
// this will also purge group change events
session1.runSql("purge binary logs before now(6)");

session1.runSql("select @@gtid_purged");
session1.runSql("select @@gtid_executed");

EXPECT_NO_THROWS(function(){cs.rejoinCluster("cluster2", {dryRun:1});});

EXPECT_NO_THROWS(function(){cs.rejoinCluster("cluster2");});

//@<> rejoin with gtidset mismatch/reset (should fail from errant trxs)
// there will be both errant and unrecoverable trxs, but errant trxs has higher priority
// unrecoverable trxs can be repaired by just cloning over the instance, but not errant trxs

invalidate_cluster(c2, c1);

session4.runSql("set global super_read_only=0");

session4.runSql("set @gtid_before=@@gtid_executed");
session4.runSql("create schema diverger");
gtid_diverge = session4.runSql("select gtid_subtract(@@gtid_executed, @gtid_before)").fetchOne()[0];
session4.runSql("set global super_read_only=1");

EXPECT_THROWS(function(){cs.rejoinCluster("cluster2", {dryRun:1});}, "Errant transactions detected at "+hostname+":"+__mysql_sandbox_port4);

EXPECT_THROWS(function(){cs.rejoinCluster("cluster2");}, "Errant transactions detected at "+hostname+":"+__mysql_sandbox_port4);

// purge from all members
session2.runSql("flush binary logs");
session3.runSql("flush binary logs");
os.sleep(1);
session2.runSql("purge binary logs before now(6)");
session3.runSql("purge binary logs before now(6)");
session1.runSql("select @@gtid_purged");
session2.runSql("select @@gtid_purged");
session3.runSql("select @@gtid_purged");

//@<> rejoin with purged trxs + errant trxs should fail from errant trxs
EXPECT_THROWS(function(){cs.rejoinCluster("cluster2", {dryRun:1});}, "Errant transactions detected at "+hostname+":"+__mysql_sandbox_port4);

EXPECT_THROWS(function(){cs.rejoinCluster("cluster2");}, "Errant transactions detected at "+hostname+":"+__mysql_sandbox_port4);

// fix errants
inject_empty_trx(session1, gtid_diverge);

//@<> rejoin with purged trxs from all members (should fail)
// Bug#34692990	ClusterSet primary purged binlog, rebootClusterFromCompleteOutage() wait forever

EXPECT_THROWS(function() { cs.rejoinCluster("cluster2"); }, "Cluster is unable to recover one or more transactions that have been purged from the PRIMARY cluster");

EXPECT_OUTPUT_CONTAINS(`ERROR: The following transactions were purged from cluster1 and not present at ${hostname}:${__mysql_sandbox_port4}: `);

EXPECT_OUTPUT_CONTAINS("ERROR: Cluster 'cluster2' cannot be rejoined because it's missing transactions that have been purged from the binary log of");

// fix purged trxs
executed = session4.runSql("select @@gtid_executed").fetchOne()[0];

unrecoverable_trxs = session1.runSql("select gtid_subtract(@@gtid_purged, gtid_subtract(@@gtid_purged, gtid_subtract(@@gtid_executed, ?)))", [executed]).fetchOne()[0];

session4.runSql("set global super_read_only=0");
testutil.injectGtidSet(__mysql_sandbox_port4, unrecoverable_trxs);
session4.runSql("set global super_read_only=1");

//@<> check we can still rejoin after clearing unrecoverable

// inject the purged binary log at the replica
session4.runSql("set global super_read_only=0");
inject_empty_trx(session4, gtid_newschema2);
session4.runSql("set global super_read_only=1");

c2.status();

cs.rejoinCluster("cluster2");

CHECK_PRIMARY_CLUSTER([__sandbox_uri1, __sandbox_uri2, __sandbox_uri3], c1);
CHECK_REPLICA_CLUSTER([__sandbox_uri4, __sandbox_uri5, __sandbox_uri6], c1, c2);
CHECK_CLUSTER_SET(session);

//@<> ensure rejoinCluster will fix up password of all instances
session6.runSql("change replication source to source_password='bogus' for channel 'clusterset_replication'");

cs.rejoinCluster("cluster2");

EXPECT_NO_THROWS(function(){c2.setPrimaryInstance(__sandbox_uri6);});
wait_channel_ready(session6, __mysql_sandbox_port1, "clusterset_replication");

status = cs.status({extended:1});
EXPECT_EQ("PRIMARY", status["clusters"]["cluster2"]["topology"][`${hostname}:${__mysql_sandbox_port6}`]["memberRole"])

c2 = dba.getCluster();
EXPECT_NO_THROWS(function(){c2.setPrimaryInstance(__sandbox_uri4);});
wait_channel_ready(session4, __mysql_sandbox_port1, "clusterset_replication");

status = c2.status({extended:1});
EXPECT_EQ("PRIMARY", status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port4}`]["memberRole"])

//@<> rejoinCluster primary cluster after rebooting it
// see Bug #33166390 CLUSTERSET: when doing rejoincluster, a member is kicked out of GR

testutil.stopSandbox(__mysql_sandbox_port1);
testutil.stopSandbox(__mysql_sandbox_port2);
testutil.stopSandbox(__mysql_sandbox_port3);

shell.connect(__sandbox_uri4);
c2 = dba.getCluster();
cs = dba.getClusterSet();
cs.forcePrimaryCluster("cluster2");

testutil.startSandbox(__mysql_sandbox_port1);
testutil.startSandbox(__mysql_sandbox_port2);
testutil.startSandbox(__mysql_sandbox_port3);
session1 = mysql.getSession(__sandbox_uri1);
session2 = mysql.getSession(__sandbox_uri2);
session3 = mysql.getSession(__sandbox_uri3);

shell.connect(__sandbox_uri1);

c1 = dba.rebootClusterFromCompleteOutage("cluster1");

cs.rejoinCluster("cluster1");

// Rejoin the instances now
c1 = dba.getCluster();
c1.rejoinInstance(__sandbox_uri2);
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");
c1.rejoinInstance(__sandbox_uri3);
testutil.waitMemberState(__mysql_sandbox_port3, "ONLINE");

cs.status({extended:1});

CHECK_REPLICA_CLUSTER([__sandbox_uri1, __sandbox_uri2, __sandbox_uri3], c2, c1);

cs.setPrimaryCluster("cluster1");
cs = dba.getClusterSet();

//@<> turn into a 3/2/1 clusterset
c2.removeInstance(__sandbox_uri6);

shell.connect(__sandbox_uri4);
c1 = dba.getCluster("cluster1");
c2 = dba.getCluster("cluster2");

c3 = cs.createReplicaCluster(__sandbox_uri6, "cluster3");

CHECK_PRIMARY_CLUSTER([__sandbox_uri1, __sandbox_uri2, __sandbox_uri3], c1);
CHECK_REPLICA_CLUSTER([__sandbox_uri4, __sandbox_uri5], c1, c2);
CHECK_REPLICA_CLUSTER([__sandbox_uri6], c1, c3);
CHECK_CLUSTER_SET(session);

//@<> rejoin while primary is no_quorum
shell.connect(__sandbox_uri1);

// invalidate the replica
invalidate_cluster(c2, c1);

session3.runSql("stop group_replication");
testutil.killSandbox(__mysql_sandbox_port2);
testutil.waitMemberState(__mysql_sandbox_port2, "UNREACHABLE");

EXPECT_THROWS(function(){cs.rejoinCluster("cluster2", {dryRun:1});}, "The InnoDB Cluster is part of an InnoDB ClusterSet and has global state of NOT_OK within the ClusterSet. Operation is not possible when in that state: Could not connect to an ONLINE member of Primary Cluster within quorum");

EXPECT_THROWS(function(){cs.rejoinCluster("cluster2");}, "The InnoDB Cluster is part of an InnoDB ClusterSet and has global state of NOT_OK within the ClusterSet. Operation is not possible when in that state: Could not connect to an ONLINE member of Primary Cluster within quorum");

//@<> rejoin while primary is offline
session1.runSql("stop group_replication");

EXPECT_THROWS(function(){cs.rejoinCluster("cluster2", {dryRun:1});}, "The InnoDB Cluster is part of an InnoDB ClusterSet and its PRIMARY instance isn't available. Operation is not possible when in that state: All reachable members of Primary Cluster are OFFLINE, but there are some unreachable members that could be ONLINE.");

EXPECT_THROWS(function(){cs.rejoinCluster("cluster2");}, "The InnoDB Cluster is part of an InnoDB ClusterSet and its PRIMARY instance isn't available. Operation is not possible when in that state: All reachable members of Primary Cluster are OFFLINE, but there are some unreachable members that could be ONLINE.");

shell.connect(__sandbox_uri6);
tmpcs= dba.getClusterSet();
EXPECT_THROWS(function(){tmpcs.rejoinCluster("cluster2");}, "The InnoDB Cluster is part of an InnoDB ClusterSet and its PRIMARY instance isn't available. Operation is not possible when in that state: All reachable members of Primary Cluster are OFFLINE, but there are some unreachable members that could be ONLINE.");

//@<> rejoin while primary is down
testutil.stopSandbox(__mysql_sandbox_port1, {wait:1});
testutil.stopSandbox(__mysql_sandbox_port3, {wait:1});

// this will fail because the primary went down
EXPECT_THROWS(function(){cs.rejoinCluster("cluster2", {dryRun:1});}, "The ClusterSet object is disconnected. Please use dba.getClusterSet() to obtain a fresh handle.");

EXPECT_THROWS(function(){cs.rejoinCluster("cluster2");}, "The ClusterSet object is disconnected. Please use dba.getClusterSet() to obtain a fresh handle.");

shell.connect(__sandbox_uri6);
tmpcs= dba.getClusterSet();
EXPECT_THROWS(function(){tmpcs.rejoinCluster("cluster2", {dryRun:1});}, "The InnoDB Cluster is part of an InnoDB ClusterSet and its PRIMARY instance isn't available. Operation is not possible when in that state: Could not connect to any ONLINE member of Primary Cluster.");

EXPECT_THROWS(function(){tmpcs.rejoinCluster("cluster2");}, "The InnoDB Cluster is part of an InnoDB ClusterSet and its PRIMARY instance isn't available. Operation is not possible when in that state: Could not connect to any ONLINE member of Primary Cluster.");

//@<> Destroy
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
testutil.destroySandbox(__mysql_sandbox_port3);
testutil.destroySandbox(__mysql_sandbox_port4);
testutil.destroySandbox(__mysql_sandbox_port5);
testutil.destroySandbox(__mysql_sandbox_port6);
