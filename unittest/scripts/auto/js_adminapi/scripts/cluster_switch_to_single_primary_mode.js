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

//@<OUT> WL#12052: Create multi-primary cluster
var scene = new ClusterScenario([__mysql_sandbox_port1, __mysql_sandbox_port2, __mysql_sandbox_port3], {multiPrimary: true, force: true, gtidSetIsComplete: true});
var cluster = scene.cluster

// Exceptions in switchToSinglePrimaryMode():
//   - If the instance parameter is empty.
//   - If the instance definition is invalid.
//   - If the instance definition is a connection dictionary but empty.
//
//   - If the cluster is in multi-primary mode
//   - If any of the cluster members has a version < 8.0.13
//   - If the cluster has no visible quorum
//   - If 'instance' does not refer to a cluster member
//   - If any of the cluster members is not ONLINE

//@<ERR> WL#12052: Error when executing switchToSinglePrimaryMode on a cluster with members < 8.0.13 {VER(<8.0.13)}
cluster.switchToSinglePrimaryMode()

//@ WL#12052: ArgumentErrors of switchToSinglePrimaryMode {VER(>=8.0.13)}
cluster.switchToSinglePrimaryMode("")
cluster.switchToSinglePrimaryMode(1234)
cluster.switchToSinglePrimaryMode({})

// F6 - To execute a group configuration function all cluster members must be ONLINE.
//@ WL#12052: Error when executing switchToSinglePrimaryMode on a cluster with 1 or more members not ONLINE
testutil.killSandbox(__mysql_sandbox_port3);
testutil.waitMemberState(__mysql_sandbox_port3, "(MISSING)");
cluster.switchToSinglePrimaryMode()

//@<ERR> WL#12052: Error when executing switchToSinglePrimaryMode on a cluster with no visible quorum < 8.0.13 {VER(>=8.0.13)}
testutil.startSandbox(__mysql_sandbox_port3);
testutil.waitMemberState(__mysql_sandbox_port3, "ONLINE");

scene.make_no_quorum([__mysql_sandbox_port1])
cluster.switchToSinglePrimaryMode()

//@ WL#12052: Re-create the cluster {VER(>=8.0.13)}
scene.destroy();
var scene = new ClusterScenario([__mysql_sandbox_port1, __mysql_sandbox_port2, __mysql_sandbox_port3], {multiPrimary: true, force: true, gtidSetIsComplete: true});
var cluster = scene.cluster
var session = scene.session

// F1 - A new function switchToSinglePrimaryMode() shall be added to the Cluster object.
// F1.1 - On a successful switchToSinglePrimaryMode() call:
// F1.1.1 - The cluster mode should switch to single-primary.
// F1.2.3 - If instance is not set, the automatic primary election takes place.

//@<OUT> WL#12052: Switch to single-primary mode {VER(>=8.0.13)}
cluster.switchToSinglePrimaryMode()

//@<OUT> WL#12052: Check cluster status after changing to single-primary {VER(>=8.0.13)}
cluster.status()

// F1.1.2 - The auto-increment settings should be updated accordingly in all
// cluster members to auto_increment_increment = 1 and auto_increment_offset = 2

//@<> WL#12052: Verify the values of auto_increment_% in the seed instance {VER(>=8.0.13)}
EXPECT_EQ(1, get_sysvar(__mysql_sandbox_port1, "auto_increment_increment"));
EXPECT_EQ(2, get_sysvar(__mysql_sandbox_port1, "auto_increment_offset"));

//@<> WL#12052: Verify the values of auto_increment_% in the member2 {VER(>=8.0.13)}
EXPECT_EQ(1, get_sysvar(__mysql_sandbox_port2, "auto_increment_increment"));
EXPECT_EQ(2, get_sysvar(__mysql_sandbox_port2, "auto_increment_offset"));

//@<> WL#12052: Verify the values of auto_increment_% in the member3 {VER(>=8.0.13)}
EXPECT_EQ(1, get_sysvar(__mysql_sandbox_port3, "auto_increment_increment"));
EXPECT_EQ(2, get_sysvar(__mysql_sandbox_port3, "auto_increment_offset"));

// F1.1.3 - The Metadata schema must be updated to change the replicasets.topology_type value to "pm"

//@<OUT> WL#12052: Verify the value of replicasets.topology_type {VER(>=8.0.13)}
shell.connect(__sandbox_uri1);
print_metadata_replicasets_topology_type(session);

// F1.3 - The function shall be idempotent:
// F1.3.1 - Any change to a single-primary cluster when already in single-primary mode is a no-op.

//@<OUT> WL#12052: Switch a single-primary cluster to single-primary is a no-op {VER(>=8.0.13)}
cluster.switchToSinglePrimaryMode()

//@<OUT> WL#12052: Check cluster status after changing to single-primary no-op {VER(>=8.0.13)}
cluster.status()

// F1.2 - The function shall have an optional parameter instance:
// F1.2.1 - The instance option shall be a string representing the connection data for the instance.
// F1.2.2 - If instance is set, the referent cluster member shall be chosen as the new primary.

//@ WL#12052: Re-create the multi-primary cluster {VER(>=8.0.13)}
scene.destroy();
var scene = new ClusterScenario([__mysql_sandbox_port1, __mysql_sandbox_port2, __mysql_sandbox_port3], {multiPrimary: true, force: true, gtidSetIsComplete: true});
var cluster = scene.cluster
var session = scene.session

//@<OUT> WL#12052: Switch to single-primary mode defining which shall be the primary {VER(>=8.0.13)}
cluster.switchToSinglePrimaryMode(__sandbox_uri2);

//@<OUT> WL#12052: Check cluster status after switching to single-primary and defining the primary {VER(>=8.0.13)}
cluster.status()

//@ WL#12052: Finalization
scene.destroy();

