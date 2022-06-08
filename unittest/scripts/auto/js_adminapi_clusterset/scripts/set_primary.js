//@ {VER(>=8.0.27)}

//@<> INCLUDE clusterset_utils.inc

// Test combinations of:
// # instance
// ## role: primary, secondary
// ## mode: rw, ro
// ## server state: up, down, unreachable
// ## member state: offline, online, recovering, online-split
// ## consistency: consistent, diverged, empty, lagging
// ## transient events: ftwrl, long transaction
// # cluster
// ## group state: ok, down, no_quorum, split, unreachable
// ## repl state: ok, stopped, recv error, apply error, wrong source
// ## static state: member, non-member, invalidated
// ## role: primary, replica
// ## fencing: unfenced, fenced-w, fenced-all

//@<> Setup
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

// - switch to invalid cluster
// - single member in {current, promoted} cluster {unreachable,offline}
// - switch to cluster in state {online,no_quorum,offline}
// - switch while other cluster in state {online,no_quorum,offline}
// - switch to cluster that's {outdated,errant trxs}
// - 


// Test switchover on a clusterset with a single cluster
// -----------------------------------------------------


// Test switchover on a clusterset with 2 1-member clusters
// --------------------------------------------------------

//@<> Create 1/1 clusterset

shell.connect(__sandbox_uri1);
c1 = dba.createCluster("cluster1", {gtidSetIsComplete:1, manualStartOnBoot:1});

cs = c1.createClusterSet("mydb");

session1.runSql("select * from performance_schema.replication_asynchronous_connection_failover_managed");

c2 = cs.createReplicaCluster(__sandbox_uri4, "cluster2", {recoveryMethod:"incremental", manualStartOnBoot:1});

CHECK_PRIMARY_CLUSTER([__sandbox_uri1], c1);
CHECK_REPLICA_CLUSTER([__sandbox_uri4], c1, c2);
CHECK_CLUSTER_SET(session);

EXPECT_OUTPUT_NOT_CONTAINS("WARNING");
EXPECT_OUTPUT_NOT_CONTAINS("ERROR");

//@<> Invalid switch
EXPECT_THROWS(function(){cs.setPrimaryCluster("cluster3");}, "ClusterSet.setPrimaryCluster: The cluster with the name 'cluster3' does not exist.");

//@<> Switch primary 1/1 (dryRun)
cs.setPrimaryCluster("cluster2", {dryRun:1});

CHECK_PRIMARY_CLUSTER([__sandbox_uri1], c1);
CHECK_REPLICA_CLUSTER([__sandbox_uri4], c1, c2);
CHECK_CLUSTER_SET(session);

EXPECT_OUTPUT_NOT_CONTAINS("WARNING");
EXPECT_OUTPUT_NOT_CONTAINS("ERROR");

//@<> Switch primary 1/1
cs.setPrimaryCluster("cluster2");

session1.runSql("select * from performance_schema.replication_asynchronous_connection_failover_managed");

c2.status();
c1.status();

CHECK_PRIMARY_CLUSTER([__sandbox_uri4], c2);
CHECK_REPLICA_CLUSTER([__sandbox_uri1], c2, c1);
CHECK_CLUSTER_SET(session);

// and back
cs.setPrimaryCluster("cluster1");

CHECK_PRIMARY_CLUSTER([__sandbox_uri1], c1);
CHECK_REPLICA_CLUSTER([__sandbox_uri4], c1, c2);
CHECK_CLUSTER_SET(session);

EXPECT_OUTPUT_NOT_CONTAINS("WARNING");
EXPECT_OUTPUT_NOT_CONTAINS("ERROR");

// Test switchover with view changes
// ---------------------------------
//@<> Make changes to the replica cluster and purge binlogs

// generate innocuous GTID changes on the replica cluster (from the view change) and then purge them, so that they can't be replicated back to the primary cluster

c2.addInstance(__sandbox_uri5);

testutil.waitMemberTransactions(__mysql_sandbox_port5, __mysql_sandbox_port1);

session4 = mysql.getSession(__sandbox_uri4);
session5 = mysql.getSession(__sandbox_uri5);
// Purge the binary logs from all members
// BUG#34013718: reconciliation doesn't happen when binlogs purged from all members
session4.runSql("flush binary logs");
session5.runSql("flush binary logs");
os.sleep(1);
session4.runSql("purge binary logs before now(6)");
session5.runSql("purge binary logs before now(6)");

CHECK_PRIMARY_CLUSTER([__sandbox_uri1], c1);
CHECK_REPLICA_CLUSTER([__sandbox_uri4, __sandbox_uri5], c1, c2);
CHECK_CLUSTER_SET(session);

// promoting cluster2 should be OK
EXPECT_NO_THROWS(function() { cs.setPrimaryCluster("cluster2"); } );
EXPECT_OUTPUT_CONTAINS("* Reconciling internally generated GTIDs");

testutil.waitMemberTransactions(__mysql_sandbox_port4, __mysql_sandbox_port1);

CHECK_PRIMARY_CLUSTER([__sandbox_uri4, __sandbox_uri5], c2);
CHECK_REPLICA_CLUSTER([__sandbox_uri1], c2, c1);
CHECK_CLUSTER_SET(session);

cs.status({extended:2});

// promoting back cluster1 will fail unless we inject the view change GTIDs
cs.setPrimaryCluster("cluster1");

CHECK_PRIMARY_CLUSTER([__sandbox_uri1], c1);
CHECK_REPLICA_CLUSTER([__sandbox_uri4, __sandbox_uri5], c1, c2);
CHECK_CLUSTER_SET(session);
c2.removeInstance(__sandbox_uri5);

// EXPECT_OUTPUT_NOT_CONTAINS("WARNING");
EXPECT_OUTPUT_NOT_CONTAINS("ERROR");

// Test switchover on a clusterset with 1 3-member cluster and 1 1-member
// ----------------------------------------------------------------------

//@<> Create 3/1 clusterset
c1.addInstance(__sandbox_uri2);
c1.addInstance(__sandbox_uri3);

CHECK_PRIMARY_CLUSTER([__sandbox_uri1, __sandbox_uri2, __sandbox_uri3], c1);
CHECK_REPLICA_CLUSTER([__sandbox_uri4], c1, c2);
CHECK_CLUSTER_SET(session);

cs.setPrimaryCluster("cluster2");

CHECK_PRIMARY_CLUSTER([__sandbox_uri4], c2);
CHECK_REPLICA_CLUSTER([__sandbox_uri1, __sandbox_uri2, __sandbox_uri3], c2, c1);
CHECK_CLUSTER_SET(session);

cs.setPrimaryCluster("cluster1");

CHECK_PRIMARY_CLUSTER([__sandbox_uri1, __sandbox_uri2, __sandbox_uri3], c1);
CHECK_REPLICA_CLUSTER([__sandbox_uri4], c1, c2);
CHECK_CLUSTER_SET(session);

EXPECT_OUTPUT_NOT_CONTAINS("WARNING");
EXPECT_OUTPUT_NOT_CONTAINS("ERROR");

// Test switchover on a clusterset with 2 3-member clusters
// --------------------------------------------------------
//@<> Create 3/3 clusterset

c2.addInstance(__sandbox_uri5);
// Instance 6 was never part of the Cluster so it's missing all the transactions that were purged before, clone required
c2.addInstance(__sandbox_uri6, {recoveryMethod: "clone"});

testutil.waitMemberTransactions(__mysql_sandbox_port5, __mysql_sandbox_port1);
testutil.waitMemberTransactions(__mysql_sandbox_port6, __mysql_sandbox_port1);

CHECK_PRIMARY_CLUSTER([__sandbox_uri1, __sandbox_uri2, __sandbox_uri3], c1);
CHECK_REPLICA_CLUSTER([__sandbox_uri4, __sandbox_uri5, __sandbox_uri6], c1, c2);
CHECK_CLUSTER_SET(session);

cs.setPrimaryCluster("cluster2");

CHECK_REPLICA_CLUSTER([__sandbox_uri1, __sandbox_uri2, __sandbox_uri3], c2, c1);
CHECK_PRIMARY_CLUSTER([__sandbox_uri4, __sandbox_uri5, __sandbox_uri6], c2);
CHECK_CLUSTER_SET(session);

cs.setPrimaryCluster("cluster1");

CHECK_PRIMARY_CLUSTER([__sandbox_uri1, __sandbox_uri2, __sandbox_uri3], c1);
CHECK_REPLICA_CLUSTER([__sandbox_uri4, __sandbox_uri5, __sandbox_uri6], c1, c2);
CHECK_CLUSTER_SET(session);

c2.removeInstance(__sandbox_uri5);
c2.removeInstance(__sandbox_uri6);

EXPECT_OUTPUT_NOT_CONTAINS("WARNING");
EXPECT_OUTPUT_NOT_CONTAINS("ERROR");

// Test switchover on a clusterset with 1 3-member cluster and 2 1-member
// ----------------------------------------------------------------------

//@<> Create 3/1/1 clusterset
c3 = cs.createReplicaCluster(__sandbox_uri5, "cluster3", {manualStartOnBoot:1});

CHECK_PRIMARY_CLUSTER([__sandbox_uri1, __sandbox_uri2, __sandbox_uri3], c1);
CHECK_REPLICA_CLUSTER([__sandbox_uri4], c1, c2);
CHECK_REPLICA_CLUSTER([__sandbox_uri5], c1, c3);
CHECK_CLUSTER_SET(session);

session5.runSql("show slave status");

//@<> Switch primary 3/1/1

cs.setPrimaryCluster("cluster2");

wait_channel_ready(session5, __mysql_sandbox_port5, "clusterset_replication");

CHECK_REPLICA_CLUSTER([__sandbox_uri1, __sandbox_uri2, __sandbox_uri3], c2, c1);
CHECK_PRIMARY_CLUSTER([__sandbox_uri4], c2);
CHECK_REPLICA_CLUSTER([__sandbox_uri5], c2, c3);
CHECK_CLUSTER_SET(session);

cs.setPrimaryCluster("cluster3");

CHECK_REPLICA_CLUSTER([__sandbox_uri1, __sandbox_uri2, __sandbox_uri3], c3, c1);
CHECK_REPLICA_CLUSTER([__sandbox_uri4], c3, c2);
CHECK_PRIMARY_CLUSTER([__sandbox_uri5], c3);
CHECK_CLUSTER_SET(session);

cs.setPrimaryCluster("cluster1");

CHECK_PRIMARY_CLUSTER([__sandbox_uri1, __sandbox_uri2, __sandbox_uri3], c1);
CHECK_REPLICA_CLUSTER([__sandbox_uri4], c1, c2);
CHECK_REPLICA_CLUSTER([__sandbox_uri5], c1, c3);
CHECK_CLUSTER_SET(session);

EXPECT_OUTPUT_NOT_CONTAINS("WARNING");
EXPECT_OUTPUT_NOT_CONTAINS("ERROR");

// Test various error scenarios
// ----------------------------

//@<> switchover to a cluster with errant trxs
errant_gtid = inject_errant_gtid(session4);

EXPECT_THROWS(function(){cs.setPrimaryCluster("cluster2");}, "ClusterSet.setPrimaryCluster: Errant transactions detected at "+hostname+":"+__mysql_sandbox_port4);

// remove the errant trx
inject_empty_trx(session1, errant_gtid);

session4.runSql("set global super_read_only=0");
session4.runSql("set sql_log_bin=0");
session4.runSql("drop schema if exists errant_trx_db");
session4.runSql("set sql_log_bin=1");
session4.runSql("set global super_read_only=1");

CHECK_PRIMARY_CLUSTER([__sandbox_uri1, __sandbox_uri2, __sandbox_uri3], c1);
CHECK_REPLICA_CLUSTER([__sandbox_uri4], c1, c2);
CHECK_REPLICA_CLUSTER([__sandbox_uri5], c1, c3);
CHECK_CLUSTER_SET(session);

//@<> timeout at primary cluster during switch (should timeout and revert)
session1.runSql("flush tables with read lock");

EXPECT_THROWS(function(){cs.setPrimaryCluster("cluster2", {timeout:2});}, "ClusterSet.setPrimaryCluster: Error while resetting password for replication account: "+hostname+":"+__mysql_sandbox_port1+": Lock wait timeout exceeded; try restarting transaction");

CHECK_PRIMARY_CLUSTER([__sandbox_uri1, __sandbox_uri2, __sandbox_uri3], c1);
CHECK_REPLICA_CLUSTER([__sandbox_uri4], c1, c2);
CHECK_REPLICA_CLUSTER([__sandbox_uri5], c1, c3);
CHECK_CLUSTER_SET(session);

session1.runSql("unlock tables");

//@<> timeout at promoted replica cluster during switch (should timeout and revert)
session4.runSql("flush tables with read lock");

EXPECT_THROWS(function(){cs.setPrimaryCluster("cluster2", {timeout:2});}, "ClusterSet.setPrimaryCluster: Timeout reached waiting for transactions from "+hostname+":"+__mysql_sandbox_port1+" to be applied on instance '"+hostname+":"+__mysql_sandbox_port4+"'");

CHECK_PRIMARY_CLUSTER([__sandbox_uri1, __sandbox_uri2, __sandbox_uri3], c1);
CHECK_REPLICA_CLUSTER([__sandbox_uri4], c1, c2);
CHECK_REPLICA_CLUSTER([__sandbox_uri5], c1, c3);
CHECK_CLUSTER_SET(session);

session4.runSql("unlock tables");



// Test switchover on a clusterset with 3 clusters with 3, 2, 1 members
// --------------------------------------------------------------------

//@<> Create 3/2/1 clusterset
c2.addInstance(__sandbox_uri6);
testutil.waitMemberTransactions(__mysql_sandbox_port4, __mysql_sandbox_port1);

CHECK_PRIMARY_CLUSTER([__sandbox_uri1, __sandbox_uri2, __sandbox_uri3], c1);
CHECK_REPLICA_CLUSTER([__sandbox_uri4, __sandbox_uri6], c1, c2);
CHECK_REPLICA_CLUSTER([__sandbox_uri5], c1, c3);
CHECK_CLUSTER_SET(session);

cs.setPrimaryCluster("cluster2");

// check with old handles
CHECK_REPLICA_CLUSTER([__sandbox_uri1, __sandbox_uri2, __sandbox_uri3], c2, c1);
CHECK_PRIMARY_CLUSTER([__sandbox_uri4, __sandbox_uri6], c2);
CHECK_REPLICA_CLUSTER([__sandbox_uri5], c2, c3);
CHECK_CLUSTER_SET(session);

// check again with fresh handles
shell.connect(__sandbox_uri4);
c2 = dba.getCluster();

shell.connect(__sandbox_uri5);
c3 = dba.getCluster();

CHECK_REPLICA_CLUSTER([__sandbox_uri1, __sandbox_uri2, __sandbox_uri3], c2, c1);
CHECK_PRIMARY_CLUSTER([__sandbox_uri4, __sandbox_uri6], c2);
CHECK_REPLICA_CLUSTER([__sandbox_uri5], c2, c3);
CHECK_CLUSTER_SET(session);

cs.setPrimaryCluster("cluster3");

CHECK_REPLICA_CLUSTER([__sandbox_uri1, __sandbox_uri2, __sandbox_uri3], c3, c1);
CHECK_REPLICA_CLUSTER([__sandbox_uri4, __sandbox_uri6], c3, c2);
CHECK_PRIMARY_CLUSTER([__sandbox_uri5], c3);
CHECK_CLUSTER_SET(session);

// check again with fresh handles
shell.connect(__sandbox_uri4);
c2 = dba.getCluster();

shell.connect(__sandbox_uri5);
c3 = dba.getCluster();

CHECK_REPLICA_CLUSTER([__sandbox_uri1, __sandbox_uri2, __sandbox_uri3], c3, c1);
CHECK_REPLICA_CLUSTER([__sandbox_uri4, __sandbox_uri6], c3, c2);
CHECK_PRIMARY_CLUSTER([__sandbox_uri5], c3);
CHECK_CLUSTER_SET(session);

cs.setPrimaryCluster("cluster1");
testutil.waitMemberTransactions(__mysql_sandbox_port4, __mysql_sandbox_port1);
testutil.waitMemberTransactions(__mysql_sandbox_port5, __mysql_sandbox_port1);

CHECK_PRIMARY_CLUSTER([__sandbox_uri1, __sandbox_uri2, __sandbox_uri3], c1);
CHECK_REPLICA_CLUSTER([__sandbox_uri4, __sandbox_uri6], c1, c2);
CHECK_REPLICA_CLUSTER([__sandbox_uri5], c1, c3);
CHECK_CLUSTER_SET(session);

EXPECT_OUTPUT_NOT_CONTAINS("WARNING");
EXPECT_OUTPUT_NOT_CONTAINS("ERROR");

//@<> dryRun with 3 clusters

if (__test_execution_mode != 'replay') {
ts1 = genlog_last_timestamp(__mysql_sandbox_port1);
ts2 = genlog_last_timestamp(__mysql_sandbox_port2);
ts3 = genlog_last_timestamp(__mysql_sandbox_port3);
ts4 = genlog_last_timestamp(__mysql_sandbox_port4);
ts5 = genlog_last_timestamp(__mysql_sandbox_port5);
ts6 = genlog_last_timestamp(__mysql_sandbox_port4);
}

cs.setPrimaryCluster("cluster2", {dryRun:1});

CHECK_PRIMARY_CLUSTER([__sandbox_uri1, __sandbox_uri2, __sandbox_uri3], c1);
CHECK_REPLICA_CLUSTER([__sandbox_uri4, __sandbox_uri6], c1, c2);
CHECK_REPLICA_CLUSTER([__sandbox_uri5], c1, c3);
CHECK_CLUSTER_SET(session);

EXPECT_OUTPUT_NOT_CONTAINS("WARNING");
EXPECT_OUTPUT_NOT_CONTAINS("ERROR");

//@<> ensure only R/O ops everywhere  {__test_execution_mode != 'replay'}
var logs = testutil.readGeneralLog(__mysql_sandbox_port1, ts1);
var logs = genlog_filter_reads(logs, /^SELECT.*FROM|SHOW|SET (SESSION|@|lock_wait)|SELECT (@@|COALESCE|GTID_EXECUTED|GTID_PURGED|GTID_SUB|group_replication_get_)/i);
var logs = logs.map(function(x){return x["sql"];});
EXPECT_EQ(["FLUSH TABLES WITH READ LOCK", "UNLOCK TABLES"], logs);
var logs = testutil.readGeneralLog(__mysql_sandbox_port2, ts2);
var logs = genlog_filter_reads(logs, /^SELECT.*FROM|SHOW|SET (SESSION|@|lock_wait)|SELECT (@@|COALESCE|GTID_EXECUTED|GTID_PURGED|GTID_SUB|group_replication_get_)/i);
var logs = logs.map(function(x){return x["sql"];});
EXPECT_EQ([], logs);
var logs = testutil.readGeneralLog(__mysql_sandbox_port3, ts3);
var logs = genlog_filter_reads(logs, /^SELECT.*FROM|SHOW|SET (SESSION|@|lock_wait)|SELECT (@@|COALESCE|GTID_EXECUTED|GTID_PURGED|GTID_SUB|group_replication_get_)/i);
var logs = logs.map(function(x){return x["sql"];});
EXPECT_EQ([], logs);
var logs = testutil.readGeneralLog(__mysql_sandbox_port4, ts4);
var logs = genlog_filter_reads(logs, /^SELECT.*FROM|SHOW|SET (SESSION|@|lock_wait)|SELECT (@@|COALESCE|GTID_EXECUTED|GTID_PURGED|GTID_SUB|group_replication_get_)/i);
var logs = logs.map(function(x){return x["sql"];});
EXPECT_EQ([], logs);
var logs = testutil.readGeneralLog(__mysql_sandbox_port5, ts5);
var logs = genlog_filter_reads(logs, /^SELECT.*FROM|SHOW|SET (SESSION|@|lock_wait)|SELECT (@@|COALESCE|GTID_EXECUTED|GTID_PURGED|GTID_SUB|group_replication_get_)/i);
var logs = logs.map(function(x){return x["sql"];});
EXPECT_EQ([], logs);
var logs = testutil.readGeneralLog(__mysql_sandbox_port6, ts6);
var logs = genlog_filter_reads(logs, /^SELECT.*FROM|SHOW|SET (SESSION|@|lock_wait)|SELECT (@@|COALESCE|GTID_EXECUTED|GTID_PURGED|GTID_SUB|group_replication_get_)/i);
var logs = logs.map(function(x){return x["sql"];});
EXPECT_EQ([], logs);

// Test switchover with unavailable members or clusters
// ----------------------------------------------------

//@<> switchover with 3 clusters while 1 member is offline

// take down sb6 because sb4 is primary and there's no group failover of AR yet
session6 = mysql.getSession(__sandbox_uri6);
session6.runSql("stop group_replication");
shell.connect(__sandbox_uri4);
testutil.waitMemberState(__mysql_sandbox_port6, "(MISSING)");

CHECK_PRIMARY_CLUSTER([__sandbox_uri1, __sandbox_uri2, __sandbox_uri3], c1);
CHECK_REPLICA_CLUSTER([__sandbox_uri4], c1, c2);
CHECK_REPLICA_CLUSTER([__sandbox_uri5], c1, c3);
CHECK_CLUSTER_SET(session);

cs.setPrimaryCluster("cluster2");
CHECK_REPLICA_CLUSTER([__sandbox_uri1, __sandbox_uri2, __sandbox_uri3], c2, c1);
CHECK_PRIMARY_CLUSTER([__sandbox_uri4], c2);
CHECK_REPLICA_CLUSTER([__sandbox_uri5], c2, c3);
CHECK_CLUSTER_SET(session);

cs.setPrimaryCluster("cluster3");
CHECK_REPLICA_CLUSTER([__sandbox_uri1, __sandbox_uri2, __sandbox_uri3], c3, c1);
CHECK_REPLICA_CLUSTER([__sandbox_uri4], c3, c2);
CHECK_PRIMARY_CLUSTER([__sandbox_uri5], c3);
CHECK_CLUSTER_SET(session);

session6.runSql("start group_replication");
shell.connect(__sandbox_uri4);
testutil.waitMemberState(__mysql_sandbox_port6, "ONLINE");

cs.setPrimaryCluster("cluster1");

wait_channel_ready(session4, __mysql_sandbox_port1, "clusterset_replication");

CHECK_PRIMARY_CLUSTER([__sandbox_uri1, __sandbox_uri2, __sandbox_uri3], c1);
CHECK_REPLICA_CLUSTER([__sandbox_uri4, __sandbox_uri6], c1, c2);
CHECK_REPLICA_CLUSTER([__sandbox_uri5], c1, c3);
CHECK_CLUSTER_SET(session);

EXPECT_OUTPUT_NOT_CONTAINS("WARNING");
EXPECT_OUTPUT_NOT_CONTAINS("ERROR");

//@<> switchover with 3 clusters while 1 member (from cluster2) is down
testutil.stopSandbox(__mysql_sandbox_port6, {wait:1});
shell.connect(__sandbox_uri4);
testutil.waitMemberState(__mysql_sandbox_port6, "(MISSING)");
CHECK_PRIMARY_CLUSTER([__sandbox_uri1, __sandbox_uri2, __sandbox_uri3], c1);
CHECK_REPLICA_CLUSTER([__sandbox_uri4], c1, c2);
CHECK_REPLICA_CLUSTER([__sandbox_uri5], c1, c3);
CHECK_CLUSTER_SET(session);

cs.setPrimaryCluster("cluster2");
CHECK_REPLICA_CLUSTER([__sandbox_uri1, __sandbox_uri2, __sandbox_uri3], c2, c1);
CHECK_PRIMARY_CLUSTER([__sandbox_uri4], c2);
CHECK_REPLICA_CLUSTER([__sandbox_uri5], c2, c3);
CHECK_CLUSTER_SET(session);

cs.setPrimaryCluster("cluster3");
CHECK_REPLICA_CLUSTER([__sandbox_uri1, __sandbox_uri2, __sandbox_uri3], c3, c1);
CHECK_REPLICA_CLUSTER([__sandbox_uri4], c3, c2);
CHECK_PRIMARY_CLUSTER([__sandbox_uri5], c3);
CHECK_CLUSTER_SET(session);

testutil.startSandbox(__mysql_sandbox_port6);
c2.rejoinInstance(__sandbox_uri6);
shell.connect(__sandbox_uri4);
testutil.waitMemberState(__mysql_sandbox_port6, "ONLINE");

c2 = dba.getCluster();

CHECK_REPLICA_CLUSTER([__sandbox_uri1, __sandbox_uri2, __sandbox_uri3], c3, c1);
CHECK_REPLICA_CLUSTER([__sandbox_uri4, __sandbox_uri6], c3, c2);
CHECK_PRIMARY_CLUSTER([__sandbox_uri5], c3);

cs.setPrimaryCluster("cluster1");
CHECK_PRIMARY_CLUSTER([__sandbox_uri1, __sandbox_uri2, __sandbox_uri3], c1);
CHECK_REPLICA_CLUSTER([__sandbox_uri4, __sandbox_uri6], c1, c2);
CHECK_REPLICA_CLUSTER([__sandbox_uri5], c1, c3);
CHECK_CLUSTER_SET(session);

EXPECT_OUTPUT_NOT_CONTAINS("WARNING");
EXPECT_OUTPUT_NOT_CONTAINS("ERROR");

//@<> Test switchover to a no_quorum cluster
CHECK_PRIMARY_CLUSTER([__sandbox_uri1, __sandbox_uri2, __sandbox_uri3], c1);
CHECK_REPLICA_CLUSTER([__sandbox_uri4, __sandbox_uri6], c1, c2);
CHECK_REPLICA_CLUSTER([__sandbox_uri5], c1, c3);
CHECK_CLUSTER_SET(session);

testutil.killSandbox(__mysql_sandbox_port6);
testutil.waitMemberState(__mysql_sandbox_port6, "UNREACHABLE");

EXPECT_THROWS(function(){cs.setPrimaryCluster("cluster2");}, "ClusterSet.setPrimaryCluster: Cluster 'cluster2' has no quorum");

CHECK_PRIMARY_CLUSTER([__sandbox_uri1, __sandbox_uri2, __sandbox_uri3], c1);
CHECK_REPLICA_CLUSTER([__sandbox_uri4], c1, c2);
CHECK_REPLICA_CLUSTER([__sandbox_uri5], c1, c3);
CHECK_CLUSTER_SET(session);

//@<> Test switchover while a cluster is no_quorum (needs to invalidate cluster2)
EXPECT_THROWS(function(){cs.setPrimaryCluster("cluster3");}, "ClusterSet.setPrimaryCluster: One or more replica clusters are unavailable");

cs.setPrimaryCluster("cluster3", {invalidateReplicaClusters:["cluster2"]});
CHECK_REPLICA_CLUSTER([__sandbox_uri1, __sandbox_uri2, __sandbox_uri3], c3, c1);
CHECK_INVALIDATED_CLUSTER([], c3, c2);
CHECK_PRIMARY_CLUSTER([__sandbox_uri5], c3);

//@<> rebuild as cluster4
testutil.destroySandbox(__mysql_sandbox_port4);
testutil.destroySandbox(__mysql_sandbox_port6);

testutil.deploySandbox(__mysql_sandbox_port4, "root", {report_host:hostname});
testutil.deploySandbox(__mysql_sandbox_port6, "root", {report_host:hostname});

c4 = cs.createReplicaCluster(__sandbox_uri6, "cluster4", {manualStartOnBoot:1, recoveryMethod: "clone"});
c4.addInstance(__sandbox_uri4, {recoveryMethod: "clone"});

session4 = mysql.getSession(__sandbox_uri4);
session6 = mysql.getSession(__sandbox_uri6);

CHECK_REPLICA_CLUSTER([__sandbox_uri1, __sandbox_uri2, __sandbox_uri3], c3, c1);
CHECK_INVALIDATED_CLUSTER([], c3, c2);
CHECK_PRIMARY_CLUSTER([__sandbox_uri5], c3);
CHECK_REPLICA_CLUSTER([__sandbox_uri6, __sandbox_uri4], c3, c4);

//@<> Test switchover while primary cluster (cluster4) is no_quorum

// switch the primary to cluster4
cs.setPrimaryCluster("cluster4");

wait_channel_ready(session1, __mysql_sandbox_port6, "clusterset_replication");

CHECK_REPLICA_CLUSTER([__sandbox_uri1, __sandbox_uri2, __sandbox_uri3], c4, c1);
CHECK_INVALIDATED_CLUSTER([], c4, c2);
CHECK_REPLICA_CLUSTER([__sandbox_uri5], c4, c3);
CHECK_PRIMARY_CLUSTER([__sandbox_uri6, __sandbox_uri4], c4);

// make cluster4 no_quorum
shell.connect(__sandbox_uri4);

testutil.killSandbox(__mysql_sandbox_port6);
testutil.waitMemberState(__mysql_sandbox_port6, "UNREACHABLE");

cs = dba.getClusterSet();

EXPECT_THROWS(function(){cs.setPrimaryCluster("cluster1");}, "ClusterSet.setPrimaryCluster: The InnoDB Cluster is part of an InnoDB ClusterSet and has global state of NOT_OK within the ClusterSet. Operation is not possible when in that state: Could not connect to an ONLINE member of Primary Cluster within quorum");

EXPECT_THROWS(function(){cs.setPrimaryCluster("cluster1", {dryRun:1});}, "ClusterSet.setPrimaryCluster: The InnoDB Cluster is part of an InnoDB ClusterSet and has global state of NOT_OK within the ClusterSet. Operation is not possible when in that state: Could not connect to an ONLINE member of Primary Cluster within quorum");

// EXPECT_OUTPUT_CONTAINS("ERROR: A connection to the PRIMARY instance of cluster 'cluster4' could not be established to perform this action.");
// EXPECT_OUTPUT_CONTAINS("ERROR: MYSQLSH 51011: Group has no quorum");

EXPECT_THROWS(function(){cs.setPrimaryCluster("cluster4");}, "ClusterSet.setPrimaryCluster: The InnoDB Cluster is part of an InnoDB ClusterSet and has global state of NOT_OK within the ClusterSet. Operation is not possible when in that state: Could not connect to an ONLINE member of Primary Cluster within quorum");

//@<> Test switchover while primary cluster is offline
session4.runSql("stop group_replication");

cs.status({extended:1});

EXPECT_THROWS(function(){cs.setPrimaryCluster("cluster1");}, "The InnoDB Cluster is part of an InnoDB ClusterSet and has global state of NOT_OK within the ClusterSet. Operation is not possible when in that state: All reachable members of Primary Cluster are OFFLINE, but there are some unreachable members that could be ONLINE");

EXPECT_THROWS(function(){cs.setPrimaryCluster("cluster4");}, "The InnoDB Cluster is part of an InnoDB ClusterSet and has global state of NOT_OK within the ClusterSet. Operation is not possible when in that state: All reachable members of Primary Cluster are OFFLINE, but there are some unreachable members that could be ONLINE");

//@<> Test switchover while primary cluster is down
testutil.stopSandbox(__mysql_sandbox_port4, {wait:1});

shell.connect(__sandbox_uri1);
cs = dba.getClusterSet();

EXPECT_THROWS(function(){cs.setPrimaryCluster("cluster1");}, "The InnoDB Cluster is part of an InnoDB ClusterSet and has global state of NOT_OK within the ClusterSet. Operation is not possible when in that state: Could not connect to any ONLINE member of Primary Cluster");

EXPECT_THROWS(function(){cs.setPrimaryCluster("cluster1", {dryRun:1});}, "The InnoDB Cluster is part of an InnoDB ClusterSet and has global state of NOT_OK within the ClusterSet. Operation is not possible when in that state: Could not connect to any ONLINE member of Primary Cluster");

EXPECT_THROWS(function(){cs.setPrimaryCluster("cluster4");}, "The InnoDB Cluster is part of an InnoDB ClusterSet and has global state of NOT_OK within the ClusterSet. Operation is not possible when in that state: Could not connect to any ONLINE member of Primary Cluster");

// Continued in set_primary_more.js

//@<> Destroy
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
testutil.destroySandbox(__mysql_sandbox_port3);
testutil.destroySandbox(__mysql_sandbox_port4);
testutil.destroySandbox(__mysql_sandbox_port5);
testutil.destroySandbox(__mysql_sandbox_port6);
