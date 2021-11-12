//@ {VER(>=8.0.27)}

//@<> INCLUDE clusterset_utils.inc

//@<> Setup

testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host:hostname});
testutil.deploySandbox(__mysql_sandbox_port2, "root", {report_host:hostname});

testutil.deploySandbox(__mysql_sandbox_port4, "root", {report_host:hostname});
testutil.deploySandbox(__mysql_sandbox_port5, "root", {report_host:hostname});


session1 = mysql.getSession(__sandbox_uri1);
session2 = mysql.getSession(__sandbox_uri2);

session4 = mysql.getSession(__sandbox_uri4);
session5 = mysql.getSession(__sandbox_uri5);


//@<> Create a 1/1 clusterset

shell.connect(__sandbox_uri1);
c1 = dba.createCluster("cluster1", {gtidSetIsComplete:1});

cs = c1.createClusterSet("cs");

c2 = cs.createReplicaCluster(__sandbox_uri4, "cluster2");

CHECK_PRIMARY_CLUSTER([__sandbox_uri1], c1);
CHECK_REPLICA_CLUSTER([__sandbox_uri4], c1, c2);
CHECK_CLUSTER_SET(session);

//@<> failover while the primary is just fine (1/1) (should fail)

EXPECT_THROWS(function(){cs.forcePrimaryCluster("cluster2");}, "PRIMARY cluster 'cluster1' is still available");

EXPECT_THROWS(function(){cs.forcePrimaryCluster("cluster2", {dryRun:1});}, "PRIMARY cluster 'cluster1' is still available");

CHECK_PRIMARY_CLUSTER([__sandbox_uri1], c1);
CHECK_REPLICA_CLUSTER([__sandbox_uri4], c1, c2);
CHECK_CLUSTER_SET(session);

//@<> failover while the primary is just offline (1/1) (should fail)
session1.runSql("stop group_replication");

EXPECT_THROWS(function(){cs.forcePrimaryCluster("cluster2");}, "PRIMARY cluster 'cluster1' is in state OFFLINE but can still be restored");

//@<> failover while the primary is down (1/1)

testutil.stopSandbox(__mysql_sandbox_port1, {wait:1});

shell.connect(__sandbox_uri4);
cs = dba.getClusterSet();
c2 = dba.getCluster();

//@<> check that dryRun does nothing
EXPECT_DRYRUN(function(){cs.forcePrimaryCluster("cluster2", {dryRun:1});}, __mysql_sandbox_port4);

//@<> bad args
EXPECT_THROWS(function(){cs.forcePrimaryCluster("1234");}, "ClusterSet.forcePrimaryCluster: The cluster with the name '1234' does not exist.");
EXPECT_THROWS(function(){cs.forcePrimaryCluster();}, "ClusterSet.forcePrimaryCluster: Invalid number of arguments, expected 1 to 2 but got 0");
EXPECT_THROWS(function(){cs.forcePrimaryCluster(1234);}, "ClusterSet.forcePrimaryCluster: Argument #1 is expected to be a string");
EXPECT_THROWS(function(){cs.forcePrimaryCluster(null);}, "ClusterSet.forcePrimaryCluster: Argument #1 is expected to be a string");

//@<> check failover positive case
cs.forcePrimaryCluster("cluster2");

CHECK_PRIMARY_CLUSTER([__sandbox_uri4], c2);
CHECK_INVALIDATED_CLUSTER_NAMED(session4, "cluster1");
CHECK_CLUSTER_SET(session);

//@<> restore cluster1
testutil.startSandbox(__mysql_sandbox_port1);
session1 = mysql.getSession(__sandbox_uri1);
shell.connect(__sandbox_uri1);
c1 = dba.rebootClusterFromCompleteOutage();

CHECK_PRIMARY_CLUSTER([__sandbox_uri4], c2);
CHECK_INVALIDATED_CLUSTER_NAMED(session4, "cluster1");
CHECK_CLUSTER_SET(session);

cs.rejoinCluster("cluster1");

shell.connect(__sandbox_uri1);
c1= dba.getCluster();

shell.connect(__sandbox_uri4);
c2= dba.getCluster();

CHECK_PRIMARY_CLUSTER([__sandbox_uri4], c2);
CHECK_REPLICA_CLUSTER([__sandbox_uri1], c2, c1);
CHECK_CLUSTER_SET(session);

//@<> Extend to a 2/1/1 clusterset

c1.addInstance(__sandbox_uri2, {recoveryMethod:"clone"});

c3 = cs.createReplicaCluster(__sandbox_uri5, "cluster3");

CHECK_PRIMARY_CLUSTER([__sandbox_uri4], c2);
CHECK_REPLICA_CLUSTER([__sandbox_uri1, __sandbox_uri2], c2, c1);
CHECK_REPLICA_CLUSTER([__sandbox_uri5], c2, c3);
CHECK_CLUSTER_SET(session);

//@<> failover while a replica (or target replica) is no_quorum (should fail)

// take down primary (cluster2)
testutil.stopSandbox(__mysql_sandbox_port4, {wait:1});

// make cluster1 no_quorum
testutil.killSandbox(__mysql_sandbox_port2);

shell.connect(__sandbox_uri1);
testutil.waitMemberState(__mysql_sandbox_port2, "UNREACHABLE");

cs = dba.getClusterSet();

EXPECT_THROWS(function(){cs.forcePrimaryCluster("cluster2");}, "ClusterSet.forcePrimaryCluster: Target cluster 'cluster2' is the current PRIMARY");
EXPECT_THROWS(function(){cs.forcePrimaryCluster("cluster2", {dryRun:1});}, "ClusterSet.forcePrimaryCluster: Target cluster 'cluster2' is the current PRIMARY");

EXPECT_THROWS(function(){cs.forcePrimaryCluster("cluster1");}, "Cluster 'cluster1' has no quorum");
EXPECT_THROWS(function(){cs.forcePrimaryCluster("cluster1", {dryRun:1});}, "Cluster 'cluster1' has no quorum");

EXPECT_THROWS(function(){cs.forcePrimaryCluster("cluster3");}, "One or more replica clusters are unavailable");
EXPECT_THROWS(function(){cs.forcePrimaryCluster("cluster3", {dryRun:1});}, "One or more replica clusters are unavailable");

//@<> failover while a replica (or target replica) (cluster1) is no_quorum + invalidate

EXPECT_THROWS(function(){cs.forcePrimaryCluster("cluster2", {invalidateReplicaClusters:["cluster1"]});}, "ClusterSet.forcePrimaryCluster: Target cluster 'cluster2' is the current PRIMARY");

EXPECT_THROWS(function(){cs.forcePrimaryCluster("cluster1", {invalidateReplicaClusters:["cluster1"]});}, "Cluster 'cluster1' has no quorum");

// cluster2 (primary) is down, cluster1 is no_quorum, cluster3 is ok
cs.forcePrimaryCluster("cluster3", {invalidateReplicaClusters:["cluster1"], dryRun:1});

// cluster2 (primary) is down, cluster1 is no_quorum, cluster3 is ok
cs.forcePrimaryCluster("cluster3", {invalidateReplicaClusters:["cluster1"]});

shell.connect(__sandbox_uri5);
c3 = dba.getCluster();

CHECK_PRIMARY_CLUSTER([__sandbox_uri5], c3);
CHECK_INVALIDATED_CLUSTER_NAMED(session, "cluster1");
CHECK_INVALIDATED_CLUSTER_NAMED(session, "cluster2");
CHECK_CLUSTER_SET(session);

testutil.startSandbox(__mysql_sandbox_port4);
shell.connect(__sandbox_uri4);
c2 = dba.rebootClusterFromCompleteOutage();

//TODO(miguel): it won't be necessary to call getClusterSet() whenever the refactor to ensure the objects freshness is done
cs = dba.getClusterSet();
cs.rejoinCluster("cluster2");

//@<> failover to an invalidated cluster1 should fail
testutil.stopSandbox(__mysql_sandbox_port5, {wait:1});

shell.connect(__sandbox_uri4);
cs = dba.getClusterSet();

cs.status();

EXPECT_THROWS(function(){cs.forcePrimaryCluster("cluster1");}, "ClusterSet.forcePrimaryCluster: Cluster 'cluster1' is invalidated");

EXPECT_THROWS(function(){cs.forcePrimaryCluster("cluster1", {dryRun:1});}, "ClusterSet.forcePrimaryCluster: Cluster 'cluster1' is invalidated");

//@<> Destroy
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);

testutil.destroySandbox(__mysql_sandbox_port4);
testutil.destroySandbox(__mysql_sandbox_port5);
