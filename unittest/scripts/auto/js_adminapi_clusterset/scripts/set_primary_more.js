//@ {VER(>=8.0.27)}

//@<> INCLUDE clusterset_utils.inc

// Continuation of set_primary.js (breaking huge test down)

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
session6 = mysql.getSession(__sandbox_uri6);


//@<> Setup a 3/2/1 clusterset

shell.connect(__sandbox_uri1)
c1 = dba.createCluster("cluster1", {gtidSetIsComplete:1, manualStartOnBoot:1});
c1.addInstance(__sandbox_uri2);
c1.addInstance(__sandbox_uri3);

cs = c1.createClusterSet("clusterset");
c2 = cs.createReplicaCluster(__sandbox_uri4, "cluster2", {manualStartOnBoot:1});
c2.addInstance(__sandbox_uri6);

c3 = cs.createReplicaCluster(__sandbox_uri5, "cluster3", {manualStartOnBoot:1});

//@<> Switch primary cluster after primary instance change
c2.setPrimaryInstance(__sandbox_uri6);
cs.setPrimaryCluster("cluster2");

c2.setPrimaryInstance(__sandbox_uri4);
cs.setPrimaryCluster("cluster1");

//@<> Test switchover while primary cluster (cluster2) is no_quorum

// switch the primary to cluster2
cs.setPrimaryCluster("cluster2");

CHECK_REPLICA_CLUSTER([__sandbox_uri1, __sandbox_uri2, __sandbox_uri3], c2, c1);
CHECK_REPLICA_CLUSTER([__sandbox_uri5], c2, c3);  
CHECK_PRIMARY_CLUSTER([__sandbox_uri4, __sandbox_uri6], c2);

// make cluster2 no_quorum
shell.connect(__sandbox_uri4);
testutil.killSandbox(__mysql_sandbox_port6);
testutil.waitMemberState(__mysql_sandbox_port6, "UNREACHABLE");

cs = dba.getClusterSet();

EXPECT_THROWS(function(){cs.setPrimaryCluster("cluster1");}, "The InnoDB Cluster is part of an InnoDB ClusterSet and has global state of NOT_OK within the ClusterSet. Operation is not possible when in that state: Could not connect to an ONLINE member of Primary Cluster within quorum");

// EXPECT_OUTPUT_CONTAINS("ERROR: A connection to the PRIMARY instance of cluster 'cluster2' could not be established to perform this action.");
// EXPECT_OUTPUT_CONTAINS("ERROR: MYSQLSH 51011: Group has no quorum");

EXPECT_THROWS(function(){cs.setPrimaryCluster("cluster2");}, "The InnoDB Cluster is part of an InnoDB ClusterSet and has global state of NOT_OK within the ClusterSet. Operation is not possible when in that state: Could not connect to an ONLINE member of Primary Cluster within quorum");

//@<> Test switchover while primary cluster is offline
session4.runSql("stop group_replication");

EXPECT_THROWS(function(){cs.setPrimaryCluster("cluster1");}, "The InnoDB Cluster is part of an InnoDB ClusterSet and has global state of NOT_OK within the ClusterSet. Operation is not possible when in that state: All reachable members of Primary Cluster are OFFLINE, but there are some unreachable members that could be ONLINE");

EXPECT_THROWS(function(){cs.setPrimaryCluster("cluster2");}, "The InnoDB Cluster is part of an InnoDB ClusterSet and has global state of NOT_OK within the ClusterSet. Operation is not possible when in that state: All reachable members of Primary Cluster are OFFLINE, but there are some unreachable members that could be ONLINE");

//@<> Test switchover while primary cluster is down
testutil.stopSandbox(__mysql_sandbox_port4, {wait:1});

shell.connect(__sandbox_uri1);
cs = dba.getClusterSet();

EXPECT_THROWS(function(){cs.setPrimaryCluster("cluster1");}, "ClusterSet.setPrimaryCluster: The InnoDB Cluster is part of an InnoDB ClusterSet and has global state of NOT_OK within the ClusterSet. Operation is not possible when in that state: Could not connect to any ONLINE member of Primary Cluster");

EXPECT_THROWS(function(){cs.setPrimaryCluster("cluster2");}, "ClusterSet.setPrimaryCluster: The InnoDB Cluster is part of an InnoDB ClusterSet and has global state of NOT_OK within the ClusterSet. Operation is not possible when in that state: Could not connect to any ONLINE member of Primary Cluster");

//@<> bring back primary cluster
testutil.startSandbox(__mysql_sandbox_port4);
testutil.startSandbox(__mysql_sandbox_port6);

shell.connect(__sandbox_uri4);
c2 = dba.rebootClusterFromCompleteOutage();
c2.rejoinInstance(__sandbox_uri6);

c1 = dba.getCluster("cluster1");
c3 = dba.getCluster("cluster3");

//@<> switchover with 3 clusters while 1 replica cluster is offline

// cluster3 is totally down now
session5.runSql("stop group_replication");

EXPECT_THROWS(function(){cs.setPrimaryCluster("cluster1");}, "ClusterSet.setPrimaryCluster: One or more replica clusters are unavailable");

EXPECT_THROWS(function(){cs.setPrimaryCluster("cluster1", {dryRun:1});}, "ClusterSet.setPrimaryCluster: One or more replica clusters are unavailable");

EXPECT_THROWS(function(){cs.setPrimaryCluster("cluster3");}, "ClusterSet.setPrimaryCluster: Could not connect to a PRIMARY member of cluster 'cluster3'");

EXPECT_THROWS(function(){cs.setPrimaryCluster("cluster2");}, "ClusterSet.setPrimaryCluster: Cluster 'cluster2' is already the PRIMARY cluster");

// cluster1=up, cluster2=gone, cluster3=down, cluster2(primary)=up

//@<> can't switch to unavailable cluster even with invalidate
EXPECT_THROWS(function(){cs.setPrimaryCluster("cluster3", {invalidateReplicaClusters:["cluster3"]});}, "ClusterSet.setPrimaryCluster: Could not connect to a PRIMARY member of cluster 'cluster3'");

//@<> invalidate bogus cluster
EXPECT_THROWS(function(){cs.setPrimaryCluster("cluster1", {invalidateReplicaClusters:["cluster5"]});}, "ClusterSet.setPrimaryCluster: Invalid cluster name 'cluster5' in option invalidateReplicaClusters");

//@<> invalidate available cluster
EXPECT_THROWS(function(){cs.setPrimaryCluster("cluster1", {invalidateReplicaClusters:["cluster1", "cluster3"]});}, "ClusterSet.setPrimaryCluster: Invalid value for option invalidateReplicaClusters");
EXPECT_OUTPUT_CONTAINS("ERROR: Cluster 'cluster1' is available and cannot be INVALIDATED");

//@<> switchover with 1 cluster offline, invalidate it
testutil.waitMemberTransactions(__mysql_sandbox_port1, __mysql_sandbox_port4);

CHECK_PRIMARY_CLUSTER([__sandbox_uri4, __sandbox_uri6], c2);
CHECK_REPLICA_CLUSTER([__sandbox_uri1, __sandbox_uri2, __sandbox_uri3], c2, c1);
CHECK_CLUSTER_SET(session);

// this would invalidate cluster3 but dryRun
cs.setPrimaryCluster("cluster1", {invalidateReplicaClusters:["cluster3"], dryRun:1});
CHECK_PRIMARY_CLUSTER([__sandbox_uri4, __sandbox_uri6], c2);
CHECK_REPLICA_CLUSTER([__sandbox_uri1, __sandbox_uri2, __sandbox_uri3], c2, c1);
CHECK_CLUSTER_SET(session);

// this will invalidate cluster3
cs.setPrimaryCluster("cluster1", {invalidateReplicaClusters:["cluster3"]});

var cstatus = cs.status();
EXPECT_EQ("OK", cstatus["clusters"]["cluster1"]["globalStatus"]);
EXPECT_EQ("OK", cstatus["clusters"]["cluster2"]["globalStatus"]);
EXPECT_EQ("INVALIDATED", cstatus["clusters"]["cluster3"]["globalStatus"]);

CHECK_PRIMARY_CLUSTER([__sandbox_uri1, __sandbox_uri2, __sandbox_uri3], c1);
CHECK_REPLICA_CLUSTER([__sandbox_uri4, __sandbox_uri6], c1, c2);
CHECK_INVALIDATED_CLUSTER([], c1, c3);
CHECK_CLUSTER_SET(session);

// reboot cluster3 and check again
shell.connect(__sandbox_uri5);
c3 = dba.rebootClusterFromCompleteOutage();
CHECK_INVALIDATED_CLUSTER([], c1, c3);
EXPECT_OUTPUT_NOT_CONTAINS("ERROR");

//@<> Test switchover to an invalidated cluster

EXPECT_THROWS(function(){cs.setPrimaryCluster("cluster3");}, "ClusterSet.setPrimaryCluster: Cluster 'cluster3' is invalidated");

cs.status();

CHECK_PRIMARY_CLUSTER([__sandbox_uri1, __sandbox_uri2, __sandbox_uri3], c1);
CHECK_REPLICA_CLUSTER([__sandbox_uri4, __sandbox_uri6], c1, c2);
CHECK_INVALIDATED_CLUSTER([], c1, c3);
CHECK_CLUSTER_SET(session);

//@<> Test switchover with an invalidated cluster

cs.setPrimaryCluster("cluster2");

CHECK_REPLICA_CLUSTER([__sandbox_uri1, __sandbox_uri2, __sandbox_uri3], c2, c1);
CHECK_PRIMARY_CLUSTER([__sandbox_uri4, __sandbox_uri6], c2);
CHECK_INVALIDATED_CLUSTER([], c2, c3);
CHECK_CLUSTER_SET(session);

cs.setPrimaryCluster("cluster1");

CHECK_PRIMARY_CLUSTER([__sandbox_uri1, __sandbox_uri2, __sandbox_uri3], c1);
CHECK_REPLICA_CLUSTER([__sandbox_uri4, __sandbox_uri6], c1, c2);
CHECK_INVALIDATED_CLUSTER([], c1, c3);
CHECK_CLUSTER_SET(session);

//@<> Destroy
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
testutil.destroySandbox(__mysql_sandbox_port3);
testutil.destroySandbox(__mysql_sandbox_port4);
testutil.destroySandbox(__mysql_sandbox_port5);
testutil.destroySandbox(__mysql_sandbox_port6);
