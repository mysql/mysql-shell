//@ {VER(>=8.0.27)}

//@<> INCLUDE clusterset_utils.inc

//@<> Setup

testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host:hostname});
testutil.deploySandbox(__mysql_sandbox_port2, "root", {report_host:hostname});
testutil.deploySandbox(__mysql_sandbox_port3, "root", {report_host:hostname});
testutil.deploySandbox(__mysql_sandbox_port4, "root", {report_host:hostname});
testutil.deploySandbox(__mysql_sandbox_port5, "root", {report_host:hostname});

// shell.options["dba.connectTimeout"]=1;

//@<> Create a 1/2/2 clusterset
shell.connect(__sandbox_uri1);
c1 = dba.createCluster("cluster1", {manualStartOnBoot:1});

cs = c1.createClusterSet("cs");

c2 = cs.createReplicaCluster(__sandbox_uri2, "cluster2", {recoveryMethod:"clone",manualStartOnBoot:1});
c3 = cs.createReplicaCluster(__sandbox_uri4, "cluster3", {recoveryMethod:"clone",manualStartOnBoot:1});

c2.addInstance(__sandbox_uri3, {recoveryMethod:"clone"});
c3.addInstance(__sandbox_uri5, {recoveryMethod:"clone"});

session1 = mysql.getSession(__sandbox_uri1);
session2 = mysql.getSession(__sandbox_uri2);
session3 = mysql.getSession(__sandbox_uri3);
session4 = mysql.getSession(__sandbox_uri4);
session5 = mysql.getSession(__sandbox_uri5);

//@<> failover to cluster2
testutil.stopSandbox(__mysql_sandbox_port1);

shell.connect(__sandbox_uri2);
cs = dba.getClusterSet();

cs.forcePrimaryCluster("cluster2");

//@<> failover to cluster2 (restore)
testutil.startSandbox(__mysql_sandbox_port1);
shell.connect(__sandbox_uri1);
c1 = dba.rebootClusterFromCompleteOutage();
cs.rejoinCluster("cluster1");
cs.setPrimaryCluster("cluster1");

//@<> failover to cluster2 after cluster primary change
c2 = dba.getCluster("cluster2");
c2.setPrimaryInstance(__sandbox_uri3);

testutil.stopSandbox(__mysql_sandbox_port1);

shell.connect(__sandbox_uri2);
cs.forcePrimaryCluster("cluster2");

//@<> failover to cluster2 after cluster primary change (restore)
testutil.startSandbox(__mysql_sandbox_port1);
shell.connect(__sandbox_uri1);
c1 = dba.rebootClusterFromCompleteOutage();
cs.rejoinCluster("cluster1");
cs.setPrimaryCluster("cluster1");

shell.connect(__sandbox_uri3);
c2 = dba.getCluster("cluster2");
EXPECT_NO_THROWS(function(){c2.setPrimaryInstance(__sandbox_uri2);});
status = c2.status({extended:1});
EXPECT_EQ("PRIMARY", status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port2}`]["memberRole"])

//@<> failover while primary is no_quorum (should fail)
cs.setPrimaryCluster("cluster2");
testutil.waitMemberTransactions(__mysql_sandbox_port1, __mysql_sandbox_port2);
testutil.waitMemberTransactions(__mysql_sandbox_port4, __mysql_sandbox_port2);

testutil.killSandbox(__mysql_sandbox_port3);
shell.connect(__sandbox_uri2);
testutil.waitMemberState(__mysql_sandbox_port3, "UNREACHABLE");

shell.connect(__sandbox_uri4);
EXPECT_THROWS(function(){cs.forcePrimaryCluster("cluster1");}, "PRIMARY cluster 'cluster2' is in state NO_QUORUM but can still be restored");
EXPECT_THROWS(function(){cs.forcePrimaryCluster("cluster2");}, "PRIMARY cluster 'cluster2' is in state NO_QUORUM but can still be restored");
EXPECT_THROWS(function(){cs.forcePrimaryCluster("cluster3");}, "PRIMARY cluster 'cluster2' is in state NO_QUORUM but can still be restored");

//@<> restore quorum of cluster2, but keep 1 instance down
shell.connect(__sandbox_uri2);
c2 = dba.getCluster("cluster2");

shell.connect(__sandbox_uri4);
c3 = dba.getCluster("cluster3");

shell.connect(__sandbox_uri2);
c2 = dba.getCluster();
c2.forceQuorumUsingPartitionOf(__sandbox_uri2);
cs.rejoinCluster("cluster2");

//@<> failover while 1 of 2 members of primary is down (should fail) 
EXPECT_THROWS(function(){cs.forcePrimaryCluster("cluster1");}, "PRIMARY cluster 'cluster2' is still available");
EXPECT_THROWS(function(){cs.forcePrimaryCluster("cluster2");}, "PRIMARY cluster 'cluster2' is still available");
EXPECT_THROWS(function(){cs.forcePrimaryCluster("cluster3");}, "PRIMARY cluster 'cluster2' is still available");

//@<> failover while cluster2 (primary) offline
cs.status();

session2.runSql("stop group_replication");

shell.connect(__sandbox_uri4);
cs = dba.getClusterSet();

cs.status({extended:1});

EXPECT_THROWS(function(){cs.forcePrimaryCluster("cluster1");}, "PRIMARY cluster 'cluster2' is in state OFFLINE but can still be restored");
EXPECT_THROWS(function(){cs.forcePrimaryCluster("cluster2");}, "PRIMARY cluster 'cluster2' is in state OFFLINE but can still be restored");
EXPECT_THROWS(function(){cs.forcePrimaryCluster("cluster3");}, "PRIMARY cluster 'cluster2' is in state OFFLINE but can still be restored");

//@<> restore cluster2, take down cluster1
testutil.startSandbox(__mysql_sandbox_port3);
shell.connect(__sandbox_uri2);
c2 = dba.rebootClusterFromCompleteOutage();
c2.rejoinInstance(__sandbox_uri3);

cs.setPrimaryCluster("cluster1");

testutil.stopSandbox(__mysql_sandbox_port1);

//@<> failover to cluster3 (should work)
cs.forcePrimaryCluster("cluster3");

testutil.startSandbox(__mysql_sandbox_port1);
shell.connect(__sandbox_uri1);
c1 = dba.rebootClusterFromCompleteOutage();
session1 = mysql.getSession(__sandbox_uri1);

cs.rejoinCluster("cluster1");

//@<> failover to a cluster that's not the most up-to-date (should fail)
cs.setPrimaryCluster("cluster1");

cs.status({extended:1});

// restore the cluster and do some work while one of the replicas is locked up
session4.runSql("stop replica for channel 'clusterset_replication'");
session4.runSql("change replication source to source_delay=600 for channel 'clusterset_replication'");
session4.runSql("start replica for channel 'clusterset_replication'");
session1.runSql("create schema bla123");
session1.runSql("create schema bla124");
session1.runSql("create schema bla125");
testutil.waitMemberTransactions(__mysql_sandbox_port2, __mysql_sandbox_port1);

testutil.stopSandbox(__mysql_sandbox_port1);

shell.connect(__sandbox_uri2);
cs = dba.getClusterSet();

EXPECT_THROWS(function(){cs.forcePrimaryCluster("cluster3");}, "ClusterSet.forcePrimaryCluster: Target cluster is behind other candidates");

//@<> failover to the most up-to-date cluster should succeed

cs.forcePrimaryCluster("cluster2");

//@<> Destroy
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
testutil.destroySandbox(__mysql_sandbox_port3);
testutil.destroySandbox(__mysql_sandbox_port4);
testutil.destroySandbox(__mysql_sandbox_port5);
