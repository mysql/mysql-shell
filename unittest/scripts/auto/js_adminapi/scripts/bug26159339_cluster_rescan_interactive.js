// Bug #26159339 SHELL: ADMINAPI DOES NOT TAKE GROUP_NAME INTO ACCOUNT

// The AdminAPI doesn't take the group_replication_group_name value of nodes
// into account when making modifications to the cluster.
// This can lead to some wrong/incorrect results.

// Create a 3 member cluster
var scene = new ClusterScenario([__mysql_sandbox_port1, __mysql_sandbox_port2, __mysql_sandbox_port3]);
testutil.touch(testutil.getSandboxLogPath(__mysql_sandbox_port1));
testutil.touch(testutil.getSandboxLogPath(__mysql_sandbox_port2));
testutil.touch(testutil.getSandboxLogPath(__mysql_sandbox_port3));
var cluster = scene.cluster;

//@<> Remove the persisted group_replication_start_on_boot and group_replication_group_name {VER(>=8.0.11)}
var s3 = mysql.getSession(__sandbox_uri3);
s3.runSql("RESET PERSIST IF EXISTS group_replication_start_on_boot");
s3.runSql("RESET PERSIST IF EXISTS group_replication_group_name");
s3.close();

//@<> Take third sandbox down, change group_name, start it back
testutil.stopSandbox(__mysql_sandbox_port3);
testutil.waitMemberState(__mysql_sandbox_port3, "(MISSING)");

testutil.removeFromSandboxConf(__mysql_sandbox_port3, "group_replication_start_on_boot");
testutil.changeSandboxConf(__mysql_sandbox_port3, "group_replication_group_name", "fd4b70e8-5cb1-11e7-a68b-b86b230042b9");
testutil.startSandbox(__mysql_sandbox_port3);

//@<> Should have 2 members ONLINE and one missing
var status = cluster.status();
EXPECT_EQ("ONLINE", status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port1}`]["status"])
EXPECT_EQ("ONLINE", status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port2}`]["status"])
EXPECT_EQ("(MISSING)", status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port3}`]["status"])

//@<> Rescan
testutil.expectPrompt("Would you like to remove it from the cluster metadata? [Y/n]: ", "n");
EXPECT_NO_THROWS(function(){ cluster.rescan(); });
EXPECT_OUTPUT_CONTAINS("Rescanning the cluster...");
EXPECT_OUTPUT_CONTAINS(`The instance '${hostname}:${__mysql_sandbox_port3}' is no longer part of the cluster.`);

//@<> Try to rejoin it (error)
EXPECT_THROWS(function(){
    cluster.rejoinInstance(__sandbox_uri3);
}, `The instance '${hostname}:${__mysql_sandbox_port3}' may belong to a different cluster as the one registered in the Metadata since the value of 'group_replication_group_name' does not match the one registered in the cluster's Metadata: possible split-brain scenario. Please remove the instance from the cluster.`);

//@<> getCluster() where the member we're connected to has a mismatched group_name vs metadata
session.close();
shell.connect(__sandbox_uri1);

//change the group_name in the metadata
session.runSql("update mysql_innodb_cluster_metadata.clusters set attributes = JSON_SET(attributes, '$.group_replication_group_name', 'fd4b70e8-5cb1-11e7-a68b-b86b230042b0')");

//@<> check error
EXPECT_THROWS(function(){
    dba.getCluster("cluster");
}, `Unable to get an InnoDB cluster handle. The instance '${hostname}:${__mysql_sandbox_port1}' may belong to a different cluster from the one registered in the Metadata since the value of 'group_replication_group_name' does not match the one registered in the Metadata: possible split-brain scenario. Please retry while connected to another member of the cluster.`);

//@<> Cleanup
session.close();
cluster.disconnect();

testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
testutil.destroySandbox(__mysql_sandbox_port3);
