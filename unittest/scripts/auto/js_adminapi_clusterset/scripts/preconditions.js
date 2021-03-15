//@ {VER(>=8.0.27)}

// Tests all preconditions checks regarding ClusterSet

//@<> Setup
var scene = new ClusterScenario([__mysql_sandbox_port1]);
var session = scene.session
var cluster = scene.cluster
testutil.deploySandbox(__mysql_sandbox_port2, "root", {report_host: hostname});
testutil.deploySandbox(__mysql_sandbox_port3, "root", {report_host: hostname});
testutil.deploySandbox(__mysql_sandbox_port4, "root", {report_host: hostname});

cs = cluster.createClusterSet("domain");
rc = cs.createReplicaCluster(__sandbox_uri2, "replica");
rc.addInstance(__sandbox_uri3);

// Cluster topology-mode operations and dissolve() are blocked for Clusters members of ClusterSets:
//  - .switchToSinglePrimaryMode()
//  - .switchToMultiPrimaryMode()
//  - .dissolve()

//@<> Topology-mode operations and dissolve() must be blocked in ClusterSets
EXPECT_THROWS(function(){ cluster.switchToSinglePrimaryMode(); }, "This function is not available through a session to an InnoDB Cluster that belongs to an InnoDB ClusterSet");
EXPECT_THROWS(function(){ rc.switchToSinglePrimaryMode(); }, "This function is not available through a session to an InnoDB Cluster that belongs to an InnoDB ClusterSet");
EXPECT_THROWS(function(){ cluster.switchToMultiPrimaryMode(); }, "This function is not available through a session to an InnoDB Cluster that belongs to an InnoDB ClusterSet");
EXPECT_THROWS(function(){ rc.switchToMultiPrimaryMode(); }, "This function is not available through a session to an InnoDB Cluster that belongs to an InnoDB ClusterSet");
EXPECT_THROWS(function(){ cluster.dissolve(); }, "This function is not available through a session to an InnoDB Cluster that belongs to an InnoDB ClusterSet");
EXPECT_THROWS(function(){ rc.dissolve(); }, "This function is not available through a session to an InnoDB Cluster that belongs to an InnoDB ClusterSet");

// Cluster monitoring operations must be allowed at any ClusterSet member (cluster):
//  - cluster.status()
//  - cluster.describe()
//  - cluster.options()
//  - cluster.listRouters()
//  - cluster.checkInstanceState()

//@<> Cluster monitoring operations must be allowed in PRIMARY and REPLICA Clusters
EXPECT_NO_THROWS(function(){ cluster.status(); });
EXPECT_NO_THROWS(function(){ rc.status(); });
EXPECT_NO_THROWS(function(){ cluster.describe(); });
EXPECT_NO_THROWS(function(){ rc.describe(); });
EXPECT_NO_THROWS(function(){ cluster.options(); });
EXPECT_NO_THROWS(function(){ rc.options(); });
EXPECT_NO_THROWS(function(){ cluster.listRouters(); });
EXPECT_NO_THROWS(function(){ rc.listRouters(); });
EXPECT_NO_THROWS(function(){ cluster.checkInstanceState(__sandbox_uri4); });
EXPECT_NO_THROWS(function(){ rc.checkInstanceState(__sandbox_uri4); });

// Cluster topology-mode operations and dissolve() are NOT blocked for Clusters removed from a ClusterSet
//  - .switchToSinglePrimaryMode()
//  - .switchToMultiPrimaryMode()
//  - .dissolve()
rc2 = cs.createReplicaCluster(__sandbox_uri4, "replica2");
cs.removeCluster("replica2");

EXPECT_NO_THROWS(function(){ rc2.switchToMultiPrimaryMode(); });
EXPECT_NO_THROWS(function(){ rc2.switchToSinglePrimaryMode(); });
EXPECT_NO_THROWS(function(){ rc2.dissolve({force: true}); });

// Cluster topology changes are allowed only if the Replication channel is healthy
shell.connect(__sandbox_uri2);
session.runSql("stop replica;");

//@<> addInstance on REPLICA Cluster with replication channel unhealthy
EXPECT_THROWS(function(){ rc.addInstance(__sandbox_uri4); }, "The InnoDB Cluster is part of an InnoDB ClusterSet and has global state of OK_NOT_REPLICATING within the ClusterSet. Operation is not possible when in that state");
EXPECT_OUTPUT_CONTAINS("WARNING: The Cluster's Replication Channel is misconfigured or stopped, topology changes will not be allowed on the InnoDB Cluster 'replica'");
EXPECT_OUTPUT_CONTAINS("NOTE: To restore the Cluster and ClusterSet operations, the Replication Channel must be fixed using rejoinCluster()");

EXPECT_THROWS(function(){ rc.removeInstance(__sandbox_uri4); }, "The InnoDB Cluster is part of an InnoDB ClusterSet and has global state of OK_NOT_REPLICATING within the ClusterSet. Operation is not possible when in that state");
EXPECT_OUTPUT_CONTAINS("WARNING: The Cluster's Replication Channel is misconfigured or stopped, topology changes will not be allowed on the InnoDB Cluster 'replica'");
EXPECT_OUTPUT_CONTAINS("NOTE: To restore the Cluster and ClusterSet operations, the Replication Channel must be fixed using rejoinCluster()");

EXPECT_THROWS(function(){ rc.rejoinInstance(__sandbox_uri4); }, "The InnoDB Cluster is part of an InnoDB ClusterSet and has global state of OK_NOT_REPLICATING within the ClusterSet. Operation is not possible when in that state");
EXPECT_OUTPUT_CONTAINS("WARNING: The Cluster's Replication Channel is misconfigured or stopped, topology changes will not be allowed on the InnoDB Cluster 'replica'");
EXPECT_OUTPUT_CONTAINS("NOTE: To restore the Cluster and ClusterSet operations, the Replication Channel must be fixed using rejoinCluster()");

EXPECT_THROWS(function(){ rc.rescan(); }, "The InnoDB Cluster is part of an InnoDB ClusterSet and has global state of OK_NOT_REPLICATING within the ClusterSet. Operation is not possible when in that state");
EXPECT_OUTPUT_CONTAINS("WARNING: The Cluster's Replication Channel is misconfigured or stopped, topology changes will not be allowed on the InnoDB Cluster 'replica'");
EXPECT_OUTPUT_CONTAINS("NOTE: To restore the Cluster and ClusterSet operations, the Replication Channel must be fixed using rejoinCluster()");

// I4: Cluster topology changes are allowed only if the PRIMARY Cluster is available.

// Make the PRIMARY cluster unavailable
testutil.killSandbox(__mysql_sandbox_port1);

shell.connect(__sandbox_uri2);
rc = dba.getCluster();

//@<> addInstance on REPLICA Cluster with PRIMARY unavailable
EXPECT_THROWS(function(){ rc.addInstance(__sandbox_uri4); }, "The InnoDB Cluster is part of an InnoDB ClusterSet and has global state of NOT_OK within the ClusterSet. Operation is not possible when in that state");
EXPECT_OUTPUT_CONTAINS("WARNING: Could not connect to any member of the PRIMARY Cluster, topology changes will not be allowed");

//@<> removeInstance on REPLICA Cluster with PRIMARY unavailable
EXPECT_THROWS(function(){ rc.removeInstance(__sandbox_uri4); }, "The InnoDB Cluster is part of an InnoDB ClusterSet and has global state of NOT_OK within the ClusterSet. Operation is not possible when in that state");
EXPECT_OUTPUT_CONTAINS("WARNING: Could not connect to any member of the PRIMARY Cluster, topology changes will not be allowed");

//@<> rejoinInstance on REPLICA Cluster with PRIMARY unavailable
EXPECT_THROWS(function(){ rc.rejoinInstance(__sandbox_uri4); }, "The InnoDB Cluster is part of an InnoDB ClusterSet and has global state of NOT_OK within the ClusterSet. Operation is not possible when in that state");
EXPECT_OUTPUT_CONTAINS("WARNING: Could not connect to any member of the PRIMARY Cluster, topology changes will not be allowed");

//@<> rescan on REPLICA Cluster with PRIMARY unavailable
EXPECT_THROWS(function(){ rc.rescan(); }, "The InnoDB Cluster is part of an InnoDB ClusterSet and has global state of NOT_OK within the ClusterSet. Operation is not possible when in that state");
EXPECT_OUTPUT_CONTAINS("WARNING: Could not connect to any member of the PRIMARY Cluster, topology changes will not be allowed");

//@<> setOption on REPLICA Cluster with PRIMARY unavailable
EXPECT_THROWS(function(){ rc.setOption("memberWeight", 50); }, "The InnoDB Cluster is part of an InnoDB ClusterSet and has global state of NOT_OK within the ClusterSet. Operation is not possible when in that state");
EXPECT_OUTPUT_CONTAINS("WARNING: Could not connect to any member of the PRIMARY Cluster, topology changes will not be allowed");

//@<> setInstanceOption on REPLICA Cluster with PRIMARY unavailable
EXPECT_THROWS(function(){ rc.setInstanceOption(__sandbox_uri2, "memberWeight", 25); }, "The InnoDB Cluster is part of an InnoDB ClusterSet and has global state of NOT_OK within the ClusterSet. Operation is not possible when in that state");
EXPECT_OUTPUT_CONTAINS("WARNING: Could not connect to any member of the PRIMARY Cluster, topology changes will not be allowed");

//@<> setupAdminAccount on REPLICA Cluster with PRIMARY unavailable
EXPECT_THROWS(function(){ rc.setupAdminAccount("cadmin@'%'", {password:"boo"}); }, "The InnoDB Cluster is part of an InnoDB ClusterSet and has global state of NOT_OK within the ClusterSet. Operation is not possible when in that state");
EXPECT_OUTPUT_CONTAINS("WARNING: Could not connect to any member of the PRIMARY Cluster, topology changes will not be allowed");

//@<> setupRouterAccount on REPLICA Cluster with PRIMARY unavailable
EXPECT_THROWS(function(){ rc.setupRouterAccount("router@'%'", {password:"boo"}); }, "The InnoDB Cluster is part of an InnoDB ClusterSet and has global state of NOT_OK within the ClusterSet. Operation is not possible when in that state");
EXPECT_OUTPUT_CONTAINS("WARNING: Could not connect to any member of the PRIMARY Cluster, topology changes will not be allowed");

//@<> resetRecoveryAccountsPasswords on REPLICA Cluster with PRIMARY unavailable
EXPECT_THROWS(function(){ rc.resetRecoveryAccountsPassword(); }, "The InnoDB Cluster is part of an InnoDB ClusterSet and has global state of NOT_OK within the ClusterSet. Operation is not possible when in that state");
EXPECT_OUTPUT_CONTAINS("WARNING: Could not connect to any member of the PRIMARY Cluster, topology changes will not be allowed");


// ClusterSet topology changes are allowed only if the PRIMARY Cluster is available:
// - createReplicaCluster()
// - removeCluster()
// - TODO: setPrimaryCluster()
// - TODO: rejoinCluster()

//@<> createReplicaCluster() with PRIMARY unavailable
cs = dba.getClusterSet();
EXPECT_THROWS(function(){ cs.createReplicaCluster(__sandbox_uri4, "replica2"); }, "The InnoDB Cluster is part of an InnoDB ClusterSet and has global state of NOT_OK within the ClusterSet. Operation is not possible when in that state");
EXPECT_OUTPUT_CONTAINS("WARNING: Could not connect to any member of the PRIMARY Cluster, topology changes will not be allowed");

//@<> removeCluster() with PRIMARY unavailable
EXPECT_THROWS(function(){ cs.removeCluster("replica"); }, "The InnoDB Cluster is part of an InnoDB ClusterSet and has global state of NOT_OK within the ClusterSet. Operation is not possible when in that state");
EXPECT_OUTPUT_CONTAINS("WARNING: Could not connect to any member of the PRIMARY Cluster, topology changes will not be allowed");

// Topology changes on a REPLICA cluster must be allowed IFF the Cluster and the PRIMARY Cluster are OK.
EXPECT_THROWS(function(){ rc.setPrimaryInstance(__sandbox_uri4); }, "The InnoDB Cluster is part of an InnoDB ClusterSet and has global state of NOT_OK within the ClusterSet. Operation is not possible when in that state");
EXPECT_OUTPUT_CONTAINS("WARNING: Could not connect to any member of the PRIMARY Cluster, topology changes will not be allowed");

//@<> Rebooting from complete outage a REPLICA Cluster when PRIMARY is OFFLINE should not be blocked

// Make the REPLICA Cluster offline
disable_auto_rejoin(__mysql_sandbox_port2);
disable_auto_rejoin(__mysql_sandbox_port3);

shell.connect(__sandbox_uri2);
testutil.killSandbox(__mysql_sandbox_port3);
testutil.waitMemberState(__mysql_sandbox_port3, "(MISSING),UNREACHABLE");
testutil.killSandbox(__mysql_sandbox_port2);

testutil.startSandbox(__mysql_sandbox_port3)
testutil.startSandbox(__mysql_sandbox_port2)

shell.connect(__sandbox_uri2);

EXPECT_NO_THROWS(function(){ rebooted_cluster = dba.rebootClusterFromCompleteOutage(); });
EXPECT_NE(rebooted_cluster, null);

//@<> Cleanup
scene.destroy();
testutil.destroySandbox(__mysql_sandbox_port2);
testutil.destroySandbox(__mysql_sandbox_port3);
testutil.destroySandbox(__mysql_sandbox_port4);
