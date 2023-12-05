//@{!__dbug_off}

//@<> Setup
var scene = new ClusterScenario([__mysql_sandbox_port1]);
testutil.deploySandbox(__mysql_sandbox_port2, "root");

var cluster = scene.cluster;
shell.connect(__sandbox_uri1);

//@<> APIs that no longer exist
EXPECT_THROWS_TYPE(function() { dba.configureLocalInstance(); }, "Invalid object member configureLocalInstance", "AttributeError");

EXPECT_THROWS_TYPE(function() { cluster.checkInstanceState(); }, "Invalid object member checkInstanceState", "AttributeError");

// ** Cluster / ClusterSet **

//@<> Check if the "interactive" option no longer exists in the following commands for Cluster / ClusterSet

EXPECT_THROWS_TYPE(function() { dba.createCluster("c", {interactive: true}); }, "Invalid options: interactive", "ArgumentError");
EXPECT_THROWS_TYPE(function() { dba.checkInstanceConfiguration(__sandbox_uri1, {interactive: true}); }, "Invalid options: interactive", "ArgumentError");
EXPECT_THROWS_TYPE(function() { dba.configureInstance(__sandbox_uri1, {interactive: true}); }, "Invalid options: interactive", "ArgumentError");
EXPECT_THROWS_TYPE(function() { dba.configureReplicaSetInstance(__sandbox_uri1, {interactive: true}); }, "Invalid options: interactive", "ArgumentError");
EXPECT_THROWS_TYPE(function() { dba.upgradeMetadata({interactive: true}); }, "Invalid options: interactive", "ArgumentError");
EXPECT_THROWS_TYPE(function() { cluster.addInstance(__sandbox_uri1, {interactive: true}); }, "Invalid options: interactive", "ArgumentError");
EXPECT_THROWS_TYPE(function() { cluster.dissolve({interactive: true}); }, "Invalid options: interactive", "ArgumentError");
EXPECT_THROWS_TYPE(function() { cluster.rejoinInstance(__sandbox_uri1, {interactive: true}); }, "Invalid options: interactive", "ArgumentError");
EXPECT_THROWS_TYPE(function() { cluster.removeInstance(__sandbox_uri1, {interactive: true}); }, "Invalid options: interactive", "ArgumentError");
EXPECT_THROWS_TYPE(function() { cluster.rescan({interactive: true}); }, "Invalid options: interactive", "ArgumentError");
EXPECT_THROWS_TYPE(function() { cluster.resetRecoveryAccountsPassword({interactive: true}); }, "Invalid options: interactive", "ArgumentError");
EXPECT_THROWS_TYPE(function() { cluster.setupAdminAccount("bla@foo", {interactive: true}); }, "Invalid options: interactive", "ArgumentError");
EXPECT_THROWS_TYPE(function() { cluster.setupRouterAccount("bla@foo", {interactive: true}); }, "Invalid options: interactive", "ArgumentError");

if (__version_num > 80027) {
  EXPECT_NO_THROWS(function() { cset = cluster.createClusterSet("cs"); })

  EXPECT_THROWS_TYPE(function() { cset.createReplicaCluster(__endpoint2, "c2", {interactive: true}); }, "Invalid options: interactive", "ArgumentError");
}

//@<> Check if the "ipWhitelist" option no longer exists in the following commands for Cluster / ClusterSet
EXPECT_THROWS_TYPE(function() { dba.createCluster("c", {ipWhitelist: "foo"}); }, "Invalid options: ipWhitelist", "ArgumentError");
EXPECT_THROWS_TYPE(function() { cluster.addInstance(__sandbox_uri1, {ipWhitelist: "foo"}); }, "Invalid options: ipWhitelist", "ArgumentError");
EXPECT_THROWS_TYPE(function() { cluster.rejoinInstance(__sandbox_uri1, {ipWhitelist: "foo"}); }, "Invalid options: ipWhitelist", "ArgumentError");

//@<> Check if the "connectToPrimary" option (or any option) no longer exists in the following commands for Cluster / ClusterSet
EXPECT_THROWS_TYPE(function() { dba.getCluster("c", {connectToPrimary: true}); }, "Invalid number of arguments, expected 0 to 1 but got 2", "ArgumentError");

//@<> Check if the "clearReadOnly" option no longer exists in the following commands for Cluster / ClusterSet
EXPECT_THROWS_TYPE(function() { dba.createCluster("c", {clearReadOnly: true}); }, "Invalid options: clearReadOnly", "ArgumentError");
EXPECT_THROWS_TYPE(function() { dba.dropMetadataSchema({clearReadOnly: true}); }, "Invalid options: clearReadOnly", "ArgumentError");
EXPECT_THROWS_TYPE(function() { dba.configureInstance(__sandbox_uri1, {clearReadOnly: true}); }, "Invalid options: clearReadOnly", "ArgumentError");
EXPECT_THROWS_TYPE(function() { dba.rebootClusterFromCompleteOutage("c", {clearReadOnly: true}); }, "Invalid options: clearReadOnly", "ArgumentError");

//@<> Check if the "multiMaster" option no longer exists in the following commands for Cluster / ClusterSet
EXPECT_THROWS_TYPE(function() { dba.createCluster("c", {multiMaster: true}); }, "Invalid options: multiMaster", "ArgumentError");

//@<> Check if the "failoverConsistency" option no longer exists in the following commands for Cluster / ClusterSet
EXPECT_THROWS_TYPE(function() { dba.createCluster("c", {failoverConsistency: "BEFORE_ON_PRIMARY_FAILOVER"}); }, "Invalid options: failoverConsistency", "ArgumentError");
EXPECT_THROWS_TYPE(function() { cluster.setOption("failoverConsistency", "BEFORE_ON_PRIMARY_FAILOVER"); }, "Option 'failoverConsistency' not supported.", "ArgumentError");

//@<> Check if the "groupSeeds" option no longer exists in the following commands for Cluster / ClusterSet
EXPECT_THROWS_TYPE(function() { dba.createCluster("c", {groupSeeds: "foo"}); }, "Invalid options: groupSeeds", "ArgumentError");
EXPECT_THROWS_TYPE(function() { cluster.addInstance(__sandbox_uri1, {groupSeeds: "foo"}); }, "Invalid options: groupSeeds", "ArgumentError");

//@<> Check if the "memberSslMode" option no longer exists in the following commands for Cluster / ClusterSet
EXPECT_THROWS_TYPE(function() { cluster.addInstance(__sandbox_uri1, {memberSslMode: "REQUIRED"}); }, "Invalid options: memberSslMode", "ArgumentError");
EXPECT_THROWS_TYPE(function() { cluster.rejoinInstance(__sandbox_uri1, {memberSslMode: "REQUIRED"}); }, "Invalid options: memberSslMode", "ArgumentError");

//@<> Check if the "queryMembers" option no longer exists in the following commands for Cluster / ClusterSet
EXPECT_THROWS_TYPE(function() { cluster.status({queryMembers: true}); }, "Invalid options: queryMembers", "ArgumentError");

//@<> Check if the "updateTopologyMode" option no longer exists in the following commands for Cluster / ClusterSet
EXPECT_THROWS_TYPE(function() { cluster.rescan({updateTopologyMode: true}); }, "Invalid options: updateTopologyMode", "ArgumentError");

//@<> Check if the "user" option no longer exists in the following commands for Cluster / ClusterSet
EXPECT_THROWS_TYPE(function() { dba.rebootClusterFromCompleteOutage("c", {user: "foo"}); }, "Invalid options: user", "ArgumentError");

//@<> Check if the "password" option no longer exists in the following commands for Cluster / ClusterSet
EXPECT_THROWS_TYPE(function() { dba.checkInstanceConfiguration(__sandbox_uri1, {password: "foo"}); }, "Invalid options: password", "ArgumentError");
EXPECT_THROWS_TYPE(function() { dba.configureInstance(__sandbox_uri1, {password: "foo"}); }, "Invalid options: password", "ArgumentError");
EXPECT_THROWS_TYPE(function() { dba.configureReplicaSetInstance(__sandbox_uri1, {password: "foo"}); }, "Invalid options: password", "ArgumentError");
EXPECT_THROWS_TYPE(function() { dba.rebootClusterFromCompleteOutage("c", {password: "foo"}); }, "Invalid options: password", "ArgumentError");
EXPECT_THROWS_TYPE(function() { cluster.addInstance(__sandbox_uri1, {password: "foo"}); }, "Invalid options: password", "ArgumentError");
EXPECT_THROWS_TYPE(function() { cluster.rejoinInstance(__sandbox_uri1, {password: "foo"}); }, "Invalid options: password", "ArgumentError");
EXPECT_THROWS_TYPE(function() { cluster.removeInstance(__sandbox_uri1, {password: "foo"}); }, "Invalid options: password", "ArgumentError");

EXPECT_THROWS_TYPE(function() { cluster.forceQuorumUsingPartitionOf(__sandbox_uri1, {password: "foo"}); }, "Invalid number of arguments, expected 1 but got 2", "ArgumentError");

//@<> Check if the "waitRecovery" option no longer exists in the following commands for Cluster / ClusterSet
EXPECT_THROWS_TYPE(function() { cluster.addInstance(__sandbox_uri1, {waitRecovery: "foo"}); }, "Invalid options: waitRecovery", "ArgumentError");
EXPECT_THROWS_TYPE(function() { cluster.rejoinInstance(__sandbox_uri1, {waitRecovery: "foo"}); }, "Invalid options: waitRecovery", "ArgumentError");

//@<> Cleanup Cluster / ClusterSet
shell.connect(__sandbox_uri1);
reset_instance(session);

// ** ReplicaSet **

//@<> Setup ReplicaSet
shell.connect(__sandbox_uri1);
EXPECT_NO_THROWS(function() { rset = dba.createReplicaSet("rs"); });

//@<> Check if the "interactive" option no longer exists in the following commands for ReplicaSet

EXPECT_THROWS_TYPE(function() { dba.createReplicaSet("rs", {interactive: true}); }, "Invalid options: interactive", "ArgumentError");
EXPECT_THROWS_TYPE(function() { rset.addInstance(__sandbox_uri1, {interactive: true}); }, "Invalid options: interactive", "ArgumentError");
EXPECT_THROWS_TYPE(function() { rset.rejoinInstance(__sandbox_uri1, {interactive: true}); }, "Invalid options: interactive", "ArgumentError");

//@<> Check if the "waitRecovery" option no longer exists in the following commands for ReplicaSet
EXPECT_THROWS_TYPE(function() { rset.addInstance(__sandbox_uri1, {waitRecovery: true}); }, "Invalid options: waitRecovery", "ArgumentError");
EXPECT_THROWS_TYPE(function() { rset.rejoinInstance(__sandbox_uri1, {waitRecovery: true}); }, "Invalid options: waitRecovery", "ArgumentError");

//@<> Cleanup
scene.destroy();
testutil.destroySandbox(__mysql_sandbox_port2);
