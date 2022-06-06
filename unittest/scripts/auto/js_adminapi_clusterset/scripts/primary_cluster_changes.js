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
rc = cs.createReplicaCluster(__sandbox_uri4, "cluster2", {recoveryMethod:"clone"});

//@<> addInstance (fail)
EXPECT_THROWS(function(){pc.addInstance(__sandbox_uri4);}, "Cluster.addInstance: The instance '127.0.0.1:"+__mysql_sandbox_port4+"' is already part of another InnoDB Cluster");

//@<> addInstance (success)
pc.addInstance(__sandbox_uri2);
pc.addInstance(__sandbox_uri3);

//@<> removeInstance on a random standalone instance (should fail)
EXPECT_THROWS(function(){pc.removeInstance(__sandbox_uri5);}, "Cluster.removeInstance: Metadata for instance localhost:"+__mysql_sandbox_port5+" not found");

//@<> removeInstance from a different cluster (should fail)

// the only one
// EXPECT_THROWS(function(){pc.removeInstance(__sandbox_uri4);}, "err");

// not the only one, but wrong cluster
// rc.addInstance(__sandbox_uri5);
// EXPECT_THROWS(function(){pc.removeInstance(__sandbox_uri5);}, "err");

//@<> removeInstance (success)
pc.removeInstance(__sandbox_uri2);

pc.addInstance(__sandbox_uri2);

//@<> setPrimaryInstance
pc.setPrimaryInstance(__sandbox_uri3);

//@<> switchToMultiPrimaryMode (should fail)
EXPECT_THROWS(function(){pc.switchToMultiPrimaryMode();}, "This function is not available through a session to an InnoDB Cluster that belongs to an InnoDB ClusterSet");

//@<> switchToSinglePrimaryMode
EXPECT_THROWS(function(){pc.switchToSinglePrimaryMode();}, "This function is not available through a session to an InnoDB Cluster that belongs to an InnoDB ClusterSet");

//@<> setOption

//@<> setInstanceOption

//@<> dissolve (should fail)
// EXPECT_THROWS(function(){pc.dissolve();}, "err");

//@<> forceQuorum
testutil.killSandbox(__mysql_sandbox_port1);
testutil.killSandbox(__mysql_sandbox_port2);

shell.connect(__sandbox_uri3);
testutil.waitMemberState(__mysql_sandbox_port2, "UNREACHABLE");

pc = dba.getCluster();

EXPECT_EQ("NO_QUORUM", pc.status()["defaultReplicaSet"]["status"]);

pc.forceQuorumUsingPartitionOf(__sandbox_uri3);
EXPECT_EQ("OK_NO_TOLERANCE_PARTIAL", pc.status()["defaultReplicaSet"]["status"]);

//@<> reboot
session.runSql("stop group_replication");

pc = dba.rebootClusterFromCompleteOutage();

//@<> Cleanup
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
testutil.destroySandbox(__mysql_sandbox_port3);
testutil.destroySandbox(__mysql_sandbox_port4);
testutil.destroySandbox(__mysql_sandbox_port5);

