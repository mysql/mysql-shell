//@ {VER(>=8.0.25)}

//@<> Deploy

testutil.deploySandbox(__mysql_sandbox_port1, "root");
testutil.deploySandbox(__mysql_sandbox_port2, "root");
testutil.deploySandbox(__mysql_sandbox_port3, "root");
testutil.deploySandbox(__mysql_sandbox_port4, "root");
testutil.deploySandbox(__mysql_sandbox_port5, "root");


//@<> Setup the cluster set
shell.connect(__sandbox_uri1);
pc = dba.createCluster("cluster1", {gtidSetIsComplete:true});

cs = pc.createClusterSet("mydomain");
rc = cs.createReplicaCluster(__sandbox_uri3, "cluster2", {recoveryMethod:"incremental"});

// Tests while primary cluster is ONLINE
//--------------------------------------

//@<> addInstance (fail)
EXPECT_THROWS(function(){rc.addInstance(__sandbox_uri1);}, "Cluster.addInstance: The instance '127.0.0.1:"+__mysql_sandbox_port1+"' is already part of another InnoDB Cluster");

//@<> addInstance (success)
rc.addInstance(__sandbox_uri4);
rc.addInstance(__sandbox_uri5);

//@<> removeInstance on a random standalone instance (should fail)
EXPECT_THROWS(function(){rc.removeInstance(__sandbox_uri2);}, "Cluster.removeInstance: Metadata for instance localhost:"+__mysql_sandbox_port2+" not found");

//@<> removeInstance from a different cluster (should fail)

// the only one
// EXPECT_THROWS(function(){rc.removeInstance(__sandbox_uri4);}, "err");

// not the only one, but wrong cluster
// pc.addInstance(__sandbox_uri5);
// EXPECT_THROWS(function(){rc.removeInstance(__sandbox_uri5);}, "err");

//@<> removeInstance (success)
rc.removeInstance(__sandbox_uri4);

rc.addInstance(__sandbox_uri4);

//@<> setPrimaryInstance
rc.setPrimaryInstance(__sandbox_uri4);

//@<> switchToMultiPrimaryMode (should fail)
EXPECT_THROWS(function(){rc.switchToMultiPrimaryMode();}, "This function is not available through a session to an InnoDB Cluster that belongs to an InnoDB ClusterSet");

//@<> switchToSinglePrimaryMode
EXPECT_THROWS(function(){rc.switchToSinglePrimaryMode();}, "This function is not available through a session to an InnoDB Cluster that belongs to an InnoDB ClusterSet");

//@<> setOption

//@<> setInstanceOption

//@<> dissolve (should fail)
// EXPECT_THROWS(function(){rc.dissolve();}, "err");

//@<> forceQuorum
testutil.killSandbox(__mysql_sandbox_port3);
testutil.killSandbox(__mysql_sandbox_port4);

shell.connect(__sandbox_uri5);
testutil.waitMemberState(__mysql_sandbox_port4, "UNREACHABLE");

rc = dba.getCluster();

EXPECT_EQ("NO_QUORUM", rc.status()["defaultReplicaSet"]["status"]);

rc.forceQuorumUsingPartitionOf(__sandbox_uri5);
EXPECT_EQ("OK_NO_TOLERANCE", pc.status()["defaultReplicaSet"]["status"]);

//@<> reboot
session.runSql("stop group_replication");

rc = dba.rebootClusterFromCompleteOutage("cluster2", {force: true});

// Tests while primary cluster is OFFLINE
//---------------------------------------


//@<> Cleanup
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
testutil.destroySandbox(__mysql_sandbox_port3);
testutil.destroySandbox(__mysql_sandbox_port4);
testutil.destroySandbox(__mysql_sandbox_port5);

