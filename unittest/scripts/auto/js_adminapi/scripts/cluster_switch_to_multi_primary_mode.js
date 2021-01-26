function print_metadata_replicasets_topology_type(session) {
    var res = session.runSql("select primary_mode from mysql_innodb_cluster_metadata.clusters");
    var row = res.fetchOne();
    print(row[0] + "\n");
}

// WL#12052 AdminAPI: single/multi primary mode change and primary election
//
// In 8.0.13, Group Replication introduces a framework to do wide-group
// configuration changes (WL#10378):
//
// Switch the GR mode: single-primary to multi-primary and vice-versa
// Trigger the election of a new primary instance in single-primary mode
//
// In order to support defining such wide-group configuration changes, the
// AdminAPI was extended by with three new commands:
//
// Cluster.switchToSinglePrimaryMode([instance])
// Cluster.switchToMultiPrimaryMode()
// Cluster.setPrimaryInstance(instance)
//

// Let "group configuration functions" be:
//
// [Cluster.]switchToSinglePrimaryMode([instance])
// [Cluster.]switchToMultiPrimaryMode()
// [Cluster.]setPrimaryInstance(instance)
//
// F4 - To execute a group configuration function the target cluster cannot
// contain members with version < 8.0.13.

//@<OUT> WL#12052: Create cluster
var scene = new ClusterScenario([__mysql_sandbox_port1, __mysql_sandbox_port2, __mysql_sandbox_port3]);
var cluster = scene.cluster

// Exceptions in switchToMultiPrimaryMode():
//   - If any of the cluster members has a version < 8.0.13
//   - If the cluster has no visible quorum
//   - If any of the cluster member is not ONLINE

//@<ERR> WL#12052: Error when executing switchToMultiPrimaryMode on a cluster with members < 8.0.13 {VER(<8.0.13)}
cluster.switchToMultiPrimaryMode()

// F6 - To execute a group configuration function all cluster members must be ONLINE.
//@ WL#12052: Error when executing switchToMultiPrimaryMode on a cluster with 1 or more members not ONLINE
testutil.killSandbox(__mysql_sandbox_port3);
testutil.waitMemberState(__mysql_sandbox_port3, "(MISSING)");
cluster.switchToMultiPrimaryMode()

//@<ERR> WL#12052: Error when executing switchToMultiPrimaryMode on a cluster with no visible quorum < 8.0.13 {VER(>=8.0.13)}
testutil.startSandbox(__mysql_sandbox_port3);
testutil.waitMemberState(__mysql_sandbox_port3, "ONLINE");

scene.make_no_quorum([__mysql_sandbox_port1])
cluster.switchToMultiPrimaryMode()

//@ WL#12052: Re-create the cluster {VER(>=8.0.13)}
scene.destroy();
var scene = new ClusterScenario([__mysql_sandbox_port1, __mysql_sandbox_port2, __mysql_sandbox_port3]);
var cluster = scene.cluster
var session = scene.session

// F2 - A new function switchToMultiPrimaryMode() shall be added to the Cluster object.
// F2.1 - On a successful switchToMultiPrimaryMode() call:
// F2.1.1 - The cluster mode should switch to multi-primary.

//@<OUT> WL#12052: Switch to multi-primary mode {VER(>=8.0.13)}
cluster.switchToMultiPrimaryMode()

//@<> WL#12052: Check cluster status after changing to multi-primary {VER(>=8.0.13)}
var status = cluster.status();
EXPECT_EQ("Multi-Primary", status["defaultReplicaSet"]["topologyMode"])
EXPECT_EQ("ONLINE", status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port1}`]["status"])
EXPECT_EQ("ONLINE", status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port2}`]["status"])
EXPECT_EQ("ONLINE", status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port3}`]["status"])
EXPECT_EQ("PRIMARY", status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port1}`]["memberRole"])
EXPECT_EQ("PRIMARY", status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port2}`]["memberRole"])
EXPECT_EQ("PRIMARY", status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port3}`]["memberRole"])
EXPECT_EQ("R/W", status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port1}`]["mode"])
EXPECT_EQ("R/W", status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port2}`]["mode"])
EXPECT_EQ("R/W", status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port3}`]["mode"])


// F2.1.2 - The auto-increment settings should be updated accordingly in all
// cluster members to auto_increment_increment = n and auto_increment_offset = 1 + server_id % n.
// The value of n shall be equal to 7 if cluster members <= 7, otherwise is equal to the cluster members.

session.close()

// Get the server_id to calculate the expected value of auto_increment_offset
shell.connect(__sandbox_uri1);
var result = session.runSql("SELECT @@server_id");
var row = result.fetchOne();
var server_id = row[0];
var __expected_auto_inc_offset = 1 + server_id%7
session.close();

//@<> WL#12052: Verify the values of auto_increment_% in the seed instance {VER(>=8.0.13)}
EXPECT_EQ(7, get_sysvar(__mysql_sandbox_port1, "auto_increment_increment"));
EXPECT_EQ(__expected_auto_inc_offset, get_sysvar(__mysql_sandbox_port1, "auto_increment_offset"));

//@<> WL#12052: Verify the values of auto_increment_% in the member2 {VER(>=8.0.13)}
// Get the server_id to calculate the expected value of auto_increment_offset
shell.connect(__sandbox_uri2);
var result = session.runSql("SELECT @@server_id");
var row = result.fetchOne();
var server_id = row[0];
var __expected_auto_inc_offset = 1 + server_id%7
session.close();

EXPECT_EQ(7, get_sysvar(__mysql_sandbox_port2, "auto_increment_increment"));
EXPECT_EQ(__expected_auto_inc_offset, get_sysvar(__mysql_sandbox_port2, "auto_increment_offset"));

//@<> WL#12052: Verify the values of auto_increment_% in the member3 {VER(>=8.0.13)}
// Get the server_id to calculate the expected value of auto_increment_offset
shell.connect(__sandbox_uri3);
var result = session.runSql("SELECT @@server_id");
var row = result.fetchOne();
var server_id = row[0];
var __expected_auto_inc_offset = 1 + server_id%7
session.close();

EXPECT_EQ(7, get_sysvar(__mysql_sandbox_port3, "auto_increment_increment"));
EXPECT_EQ(__expected_auto_inc_offset, get_sysvar(__mysql_sandbox_port3, "auto_increment_offset"));

// F2.1.3 - The Metadata schema must be updated to change the replicasets.topology_type value to "mm"

//@<OUT> WL#12052: Verify the value of replicasets.topology_type {VER(>=8.0.13)}
shell.connect(__sandbox_uri1);
print_metadata_replicasets_topology_type(session);

// F2.2 - The function shall be idempotent:
// F2.2.1 - Any change to a multi-primary cluster when already in multi-primary mode is a no-op.

//@<> WL#12052: Switch a multi-primary cluster to multi-primary is a no-op {VER(>=8.0.13)}
cluster.switchToMultiPrimaryMode()

EXPECT_OUTPUT_CONTAINS("Switching cluster 'cluster' to Multi-Primary mode...");

EXPECT_OUTPUT_CONTAINS("Instance '"+hostname+":"+__mysql_sandbox_port1+"' remains PRIMARY.");
EXPECT_OUTPUT_CONTAINS("Instance '"+hostname+":"+__mysql_sandbox_port2+"' remains PRIMARY.");
EXPECT_OUTPUT_CONTAINS("Instance '"+hostname+":"+__mysql_sandbox_port3+"' remains PRIMARY.");

EXPECT_OUTPUT_CONTAINS("The cluster successfully switched to Multi-Primary mode.");


//@<> WL#12052: Check cluster status after changing to multi-primary no-op {VER(>=8.0.13)}
var status = cluster.status();
EXPECT_EQ("Multi-Primary", status["defaultReplicaSet"]["topologyMode"])
EXPECT_EQ("ONLINE", status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port1}`]["status"])
EXPECT_EQ("ONLINE", status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port2}`]["status"])
EXPECT_EQ("ONLINE", status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port3}`]["status"])
EXPECT_EQ("PRIMARY", status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port1}`]["memberRole"])
EXPECT_EQ("PRIMARY", status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port2}`]["memberRole"])
EXPECT_EQ("PRIMARY", status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port3}`]["memberRole"])
EXPECT_EQ("R/W", status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port1}`]["mode"])
EXPECT_EQ("R/W", status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port2}`]["mode"])
EXPECT_EQ("R/W", status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port3}`]["mode"])

//@ WL#12052: Finalization
scene.destroy();

