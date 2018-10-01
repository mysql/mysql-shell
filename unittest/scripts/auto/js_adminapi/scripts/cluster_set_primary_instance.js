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
// Cluster.switchToSinglePrimaryMode([instanceDef])
// Cluster.switchToMultiPrimaryMode()
// Cluster.setPrimaryInstance(instanceDef)
//

// Let "group configuration functions" be:
//
// [Cluster.]switchToSinglePrimaryMode([instance])
// [Cluster.]switchToMultiPrimaryMode()
// [Cluster.]setPrimaryInstance(instance)
//
// F4 - To execute a group configuration function the target cluster cannot
// contain members with version < 8.0.13.

//@<OUT> WL#12052: Create single-primary cluster
var scene = new ClusterScenario([__mysql_sandbox_port1, __mysql_sandbox_port2, __mysql_sandbox_port3]);
var cluster = scene.cluster

// Exceptions in setPrimaryInstance():
//   - If the instance parameter is empty.
//   - If the instance definition is invalid.
//   - If the instance definition is a connection dictionary but empty.
//
//   - If the cluster is in multi-primary mode
//   - If any of the cluster members has a version < 8.0.13
//   - If the cluster has no visible quorum
//   - If 'instance' does not refer to a cluster member
//   - If any of the cluster members is not ONLINE

//@<ERR> WL#12052: Error when executing setPrimaryInstance on a cluster with members < 8.0.13 {VER(<8.0.13)}
cluster.setPrimaryInstance(__sandbox_uri2);

// F3.3 - The function shall have a mandatory parameter instance:
// F3.3.1 - The instance option shall be a string representing the connection data for the instance.
// F3.3.2 - The instance option shall refer to an ONLINE cluster member

//@ WL#12052: ArgumentErrors of setPrimaryInstance {VER(>=8.0.13)}
cluster.setPrimaryInstance("")
cluster.setPrimaryInstance(1234)
cluster.setPrimaryInstance({})
cluster.setPrimaryInstance("localhost:3355")

// F6 - To execute a group configuration function all cluster members must be ONLINE.
//@ WL#12052: Error when executing setPrimaryInstance on a cluster with 1 or more members not ONLINE < 8.0.13 {VER(>=8.0.13)}
testutil.killSandbox(__mysql_sandbox_port3);
testutil.waitMemberState(__mysql_sandbox_port3, "(MISSING)");
cluster.setPrimaryInstance(__sandbox_uri2)

//@<ERR> WL#12052: Error when executing setPrimaryInstance on a cluster with no visible quorum < 8.0.13 {VER(>=8.0.13)}
testutil.startSandbox(__mysql_sandbox_port3);
testutil.waitMemberState(__mysql_sandbox_port3, "ONLINE");

scene.make_no_quorum([__mysql_sandbox_port1])
cluster.setPrimaryInstance(__sandbox_uri2)

//@ WL#12052: Re-create the cluster but in multi-primary mode {VER(>=8.0.13)}
scene.destroy();
var scene = new ClusterScenario([__mysql_sandbox_port1, __mysql_sandbox_port2, __mysql_sandbox_port3], "mm");
var cluster = scene.cluster

// F3.2 - The function execution shall be allowed only on single-primary clusters.

//@ WL#12052: Error when executing setPrimaryInstance on a multi-primary cluster {VER(>=8.0.13)}
cluster.setPrimaryInstance(__sandbox_uri2);

//@ WL#12052: Re-create the cluster {VER(>=8.0.13)}
scene.destroy();
var scene = new ClusterScenario([__mysql_sandbox_port1, __mysql_sandbox_port2, __mysql_sandbox_port3]);
var cluster = scene.cluster

// F3 - A new function setPrimaryInstance(instance) shall be added to the Cluster object.
// F3.1 - On a successful setPrimaryInstance() call, the chosen cluster member shall become the new primary.

//@<OUT> WL#12052: Set new primary {VER(>=8.0.13)}
cluster.setPrimaryInstance(__sandbox_uri2);

//@<OUT> WL#12052: Set new primary 2 {VER(>=8.0.13)}
cluster.setPrimaryInstance(__sandbox_uri3);

//@<OUT> WL#12052: Cluster status after setting a new primary {VER(>=8.0.13)}
cluster.status()

//@ WL#12052: Finalization
scene.destroy();

