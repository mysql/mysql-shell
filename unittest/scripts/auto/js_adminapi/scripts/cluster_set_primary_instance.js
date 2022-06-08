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
//@ WL#12052: Error when executing setPrimaryInstance on a cluster with 1 or more members not ONLINE
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
var scene = new ClusterScenario([__mysql_sandbox_port1, __mysql_sandbox_port2, __mysql_sandbox_port3], {multiPrimary: true, force: true, gtidSetIsComplete: true});
var cluster = scene.cluster

// F3.2 - The function execution shall be allowed only on single-primary clusters.

//@ WL#12052: Error when executing setPrimaryInstance on a multi-primary cluster {VER(>=8.0.13)}
cluster.setPrimaryInstance(__sandbox_uri2);

//@ Re-create the cluster {VER(>=8.0.13)}
scene.destroy();
var scene = new ClusterScenario([__mysql_sandbox_port1, __mysql_sandbox_port2, __mysql_sandbox_port3]);
var cluster = scene.cluster

// F3 - A new function setPrimaryInstance(instance) shall be added to the Cluster object.
// F3.1 - On a successful setPrimaryInstance() call, the chosen cluster member shall become the new primary.

//@<OUT> WL#12052: Set new primary {VER(>=8.0.13)}
cluster.setPrimaryInstance(__sandbox_uri2);

//@<OUT> WL#12052: Set new primary 2 {VER(>=8.0.13)}
cluster.setPrimaryInstance(__sandbox_uri3);

//@<> set new primary through hostname {VER(>=8.0.13)}
cluster.setPrimaryInstance(hostname+":"+__mysql_sandbox_port2);

//@<> set new primary through localhost {VER(>=8.0.13)}
cluster.setPrimaryInstance("localhost:"+__mysql_sandbox_port1);

//@<> set new primary through hostname_ip {VER(>=8.0.13)}
cluster.setPrimaryInstance(hostname_ip+":"+__mysql_sandbox_port3);

//@<OUT> WL#12052: Cluster status after setting a new primary {VER(>=8.0.13)}
cluster.status()

//@<> setPrimary using localhost {VER(>=8.0.13)}
// covers Bug #30501628	REMOVEINSTANCE, SETPRIMARY ETC SHOULD WORK WITH ANY FORM OF ADDRESS

cluster.setPrimaryInstance("localhost:"+__mysql_sandbox_port2);

//@<> setPrimary using hostname_ip {VER(>=8.0.13)}
cluster.setPrimaryInstance(hostname_ip+":"+__mysql_sandbox_port3);

//@<> setPrimary using hostname {VER(>=8.0.13)}
cluster.setPrimaryInstance(hostname+":"+__mysql_sandbox_port2);

//@<> setPrimary using hostname, ignore case Bug #33893435 {VER(>=8.0.13)}
EXPECT_NO_THROWS(function() { cluster.setPrimaryInstance(hostname.toLowerCase() + ":" + __mysql_sandbox_port3); });
EXPECT_OUTPUT_CONTAINS("The instance '" + hostname.toLowerCase() + ":" + __mysql_sandbox_port3 + "' was successfully elected as primary.");

testutil.wipeAllOutput();
EXPECT_NO_THROWS(function() { cluster.setPrimaryInstance(hostname.toUpperCase() + ":" + __mysql_sandbox_port2); });
EXPECT_OUTPUT_CONTAINS("The instance '" + hostname.toUpperCase() + ":" + __mysql_sandbox_port2 + "' was successfully elected as primary.");

//@<> Ensure the call does not fail because of a transaction timeout {VER(>=8.0.29)}
shell.connect(__sandbox_uri2);

session2 = mysql.getSession(__sandbox_uri2);
session2.runSql("CREATE DATABASE test;");
session2.runSql("USE test;");
session2.runSql("CREATE TABLE t1 (a int, PRIMARY KEY(a))");
session2.runSql("START TRANSACTION");
session2.runSql("INSERT INTO t1 VALUES(1)");

EXPECT_NO_THROWS(function() {
    cluster.setPrimaryInstance(localhost + ":" + __mysql_sandbox_port1, {runningTransactionsTimeout: 0});
});
EXPECT_OUTPUT_CONTAINS("The instance '" + localhost + ":" + __mysql_sandbox_port1 + "' was successfully elected as primary.");

session2.close();
session.close();

//@<> Check if the primary version fails if there's an async channels running {VER(>=8.0.13)}
shell.connect(__sandbox_uri2);
session.runSql("SET GLOBAL super_read_only = 0");
session.runSql("CREATE USER 'repl'@'%' IDENTIFIED BY 'password' REQUIRE SSL");
session.runSql("GRANT REPLICATION SLAVE ON *.* TO 'repl'@'%';");
session.runSql("SET GLOBAL super_read_only = 1");
session.close();

shell.connect(__sandbox_uri1);
session.runSql("CHANGE MASTER TO MASTER_HOST='" + localhost + "', MASTER_PORT=" + __mysql_sandbox_port2 + ", MASTER_USER='repl', MASTER_PASSWORD='password', MASTER_AUTO_POSITION=1, MASTER_SSL=1");
session.runSql("START SLAVE");

do {
    r = session.runSql("SHOW SLAVE STATUS").fetchOne();
} while (r.Slave_IO_Running == 'Connecting')

EXPECT_THROWS(function() {
    cluster.setPrimaryInstance(localhost + ":" + __mysql_sandbox_port2);
}, "Instance cannot be set as primary");
EXPECT_OUTPUT_CONTAINS("Failed to set '" + localhost + ":" + __mysql_sandbox_port2 + "' as primary instance: The function 'group_replication_set_as_primary' failed. There is a slave channel running in the group's current primary member.");

session.close();

//@ Finalization
scene.destroy();
