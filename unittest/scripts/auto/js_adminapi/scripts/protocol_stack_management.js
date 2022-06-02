// Tests for Group Replication's protocol stack management. Among other
// internal changes and details, the tests cover the main features:
//
//  - New option 'communicationStack' in dba.createCluster()
//  - Automatic settings management
//  - Information about the protocol in .status() .options()
//  - Migration between protocols in dba.rebootClusterFromCompleteOutage() with
//  a new option 'switchCommunicationStack'

//@<> INCLUDE gr_utils.inc

//@<> Setup
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: hostname});
testutil.snapshotSandboxConf(__mysql_sandbox_port1);
testutil.deploySandbox(__mysql_sandbox_port2, "root", {report_host: hostname});
testutil.snapshotSandboxConf(__mysql_sandbox_port2);
testutil.deploySandbox(__mysql_sandbox_port3, "root", {report_host: hostname});

shell.connect(__sandbox_uri1);

//@<> Bad options in createCluster() (should fail)
if (__version_num < 80027) {
  EXPECT_THROWS_TYPE(function() { dba.createCluster("test", {communicationStack: "mySql"}) }, "Option 'communicationStack' not supported on target server version: '" + __version + "'", "RuntimeError");
  EXPECT_THROWS_TYPE(function() { dba.createCluster("test", {communicationStack: "XcOm"}) }, "Option 'communicationStack' not supported on target server version: '" + __version + "'", "RuntimeError");
} else {
  EXPECT_THROWS_TYPE(function() { dba.createCluster("test", {communicationStack: ""}); }, "Invalid value for 'communicationStack' option. String value cannot be empty.", "ArgumentError");

  EXPECT_THROWS_TYPE(function() { dba.createCluster("test", {communicationStack: "foo"}); }, "Invalid value for 'communicationStack' option. Supported values: MYSQL, XCOM", "ArgumentError");

  EXPECT_THROWS_TYPE(function() { dba.createCluster("test", {communicationStack: ""}); }, "Invalid value for 'communicationStack' option. String value cannot be empty.", "ArgumentError");

  EXPECT_THROWS_TYPE(function() { dba.createCluster("test", {communicationStack: "mysql", ipAllowlist: "localhost"}) }, "Cannot use 'ipAllowlist' when setting the 'communicationStack' option to 'MYSQL'", "ArgumentError");

  // If the server is >= 8.0.27, the default comm stack is 'mysql'
  EXPECT_THROWS_TYPE(function() { dba.createCluster("test", {ipAllowlist: "localhost"}) }, "Cannot use 'ipAllowlist' when the 'communicationStack' is 'MYSQL'", "ArgumentError");

  var local_address_xcom = hostname + ":" + __mysql_sandbox_gr_port2;

  EXPECT_THROWS_TYPE(function() { dba.createCluster("test", {communicationStack: "mysql", localAddress: local_address_xcom}) }, "Invalid port '" + __mysql_sandbox_gr_port2 + "' for localAddress option. When using 'MYSQL' communication stack, the port must be the same in use by MySQL Server", "ArgumentError");

  // The default comm stack in >= 8.0.27 is MYSQL
  EXPECT_THROWS_TYPE(function() { dba.createCluster("test", {localAddress: local_address_xcom}) }, "Invalid port '" + __mysql_sandbox_gr_port2 + "' for localAddress option. When using 'MYSQL' communication stack, the port must be the same in use by MySQL Server", "ArgumentError");
}

//@<> By default, 'MySQL' must be the default communicationStack for >= 8.0.27
var cluster;
EXPECT_NO_THROWS(function() { cluster = dba.createCluster("test", {memberSslMode: "VERIFY_CA"}) });

// NOTE: for previous versions is XCOM but we can't query the var since it doesn't exist
if (__version_num >= 80027) {
  var communication_stack = session.runSql("select @@group_replication_communication_stack").fetchOne()[0];
  EXPECT_EQ("MYSQL", communication_stack);

  // Confirm the generated value for localAddres is the same as the network
  // address MySQL is listening on. Validate the SSL settings too
  check_gr_settings(cluster, [__endpoint1], "MYSQL");
} else {
  check_gr_settings(cluster, [__endpoint1], "XCOM");
}

dba.dropMetadataSchema({force: true});

if (__version_num >= 80027) {
  EXPECT_THROWS_TYPE(function() { dba.createCluster("test", {communicationStack: "mysql", adoptFromGR: true}) }, "Cannot use the 'communicationStack' option if 'adoptFromGR' is set to true", "ArgumentError");
}

//@<> adoptFromGR can be used when communicationStack is *not* set
EXPECT_NO_THROWS(function() { cluster = dba.createCluster("test", {adoptFromGR: true, gtidSetIsComplete: true}) });

// Reset the instance to start-over
reset_instance(session);

//@<> set localAddress to a port other than the one in use by the server must be forbidden {VER(>=8.0.27)}
shell.connect(__sandbox_uri1);

var local_address_err = __hostname_uri1 + ":" + __mysql_sandbox_port2;

EXPECT_THROWS_TYPE(function() { dba.createCluster("test", {communicationStack: "mysql", localAddress: local_address_err}) }, `Invalid port '${__mysql_sandbox_port2}' for localAddress option. When using 'MYSQL' communication stack, the port must be the same in use by MySQL Server`, "ArgumentError");

EXPECT_THROWS_TYPE(function() { dba.createCluster("test", {communicationStack: "mysql", localAddress: "123"}) }, `Invalid port '123' for localAddress option. When using 'MYSQL' communication stack, the port must be the same in use by MySQL Server`, "ArgumentError");

//@<> set localAddress to use the same port where server is running must be accepted {VER(>=8.0.27)}
EXPECT_NO_THROWS(function() { cluster = dba.createCluster("test", {communicationStack: "mysql", localAddress: __endpoint1, gtidSetIsComplete: true}) });

check_gr_settings(cluster, [__endpoint1], "MYSQL");

//@<> addInstance() must automatically generate and apply localAddress, groupSeeds and memberSslMode according to the communication stack
if (__version_num < 80027) {
  EXPECT_NO_THROWS(function() { cluster = dba.createCluster("test", {gtidSetIsComplete: true}) });

  check_gr_settings(cluster, [__endpoint1], "XCOM");
}

//@<> Bad options in addInstance() (should fail)
if (__version_num >= 80027) {
  EXPECT_THROWS_TYPE(function() { cluster.addInstance(__sandbox_uri2, {ipAllowlist: "localhost"}) }, "Cannot use 'ipAllowlist' when the Cluster's communication stack is 'MYSQL'", "ArgumentError");

  var local_address_xcom = hostname + ":" + __mysql_sandbox_gr_port3;

  // The default comm stack in >= 8.0.27 is MYSQL
  EXPECT_THROWS_TYPE(function() { cluster.addInstance(__sandbox_uri2, {localAddress: local_address_xcom}) }, "Invalid port '" + __mysql_sandbox_gr_port3 + "' for localAddress option. When using 'MYSQL' communication stack, the port must be the same in use by MySQL Server", "ArgumentError");
}

EXPECT_NO_THROWS(function() {cluster.addInstance(__sandbox_uri2) });

if (__version_num < 80000) {
  var mycnf1 = testutil.getSandboxConfPath(__mysql_sandbox_port1);
  var mycnf2 = testutil.getSandboxConfPath(__mysql_sandbox_port2);
  dba.configureLocalInstance(__sandbox_uri1, {mycnfPath: mycnf1});
  dba.configureLocalInstance(__sandbox_uri2, {mycnfPath: mycnf2});
}

if (__version_num < 80027) {
  check_gr_settings(cluster, [__endpoint1, __endpoint2], "XCOM");
} else {
  check_gr_settings(cluster, [__endpoint1, __endpoint2], "MYSQL");
}

//@<> Adding multiple instances using clone and waitRecovery:0 must not fail {VER(>=8.0.27)}
//
// BUG#34237375: instance fails to join cluster using 'mysql' commstack, clone and waitRecovery:0
//
//  When the communication stack is 'MYSQL', the AdminAPI reconfigures the
// recovery account in every single active member of the cluster to use its own
// GR recovery replication credentials and not the one that was created for the
// joining instance. However, when waitRecovery is zero and clone was used, each
// instance has configured on the replication channel the credentials of the
// donor member (since it was cloned), and also has the information about the
// recovery account outdated.
// That reconfiguration happens in most cases before clone has finished so the
// credentials for the replication channel (that are cloned) won't match since
// they were already reset in the Cluster by that reconfiguration mechanism.
EXPECT_NO_THROWS(function() {cluster.removeInstance(__sandbox_uri2) });
EXPECT_NO_THROWS(function() {cluster.addInstance(__sandbox_uri2, {recoveryMethod: "clone", waitRecovery: 0}) });
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");
EXPECT_NO_THROWS(function() {cluster.addInstance(__sandbox_uri3, {recoveryMethod: "clone", waitRecovery: 0}) });
testutil.waitMemberState(__mysql_sandbox_port3, "ONLINE");

//@<> Cluster.rescan() must ensure the right recovery accounts are used in the cluster when clone recovery is used with waitRecovery:0 {VER(>=8.0.27)}
EXPECT_NO_THROWS(function() {cluster.removeInstance(__sandbox_uri2) });
EXPECT_NO_THROWS(function() {cluster.removeInstance(__sandbox_uri3) });
EXPECT_NO_THROWS(function() {cluster.addInstance(__sandbox_uri2, {recoveryMethod: "clone", waitRecovery: 0}) });
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");

EXPECT_NO_THROWS(function() {cluster.rescan(); });

testutil.restartSandbox(__mysql_sandbox_port2);
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");

//@<> rejoinInstance() must automatically generate and/or ensure the settings too {VER(>=8.0.27)}
shell.connect(__sandbox_uri2);
session.runSql("STOP group_replication;");
session.runSql("RESET PERSIST group_replication_local_address");
session.runSql("RESET PERSIST group_replication_group_seeds");
session.runSql("RESET PERSIST group_replication_recovery_use_ssl");

// Bad options in rejoinInstance() (should fail)
EXPECT_THROWS_TYPE(function() { cluster.rejoinInstance(__sandbox_uri2, {ipAllowlist: "localhost"}) }, "Cannot use 'ipAllowlist' when the Cluster's communication stack is 'MYSQL'", "ArgumentError");

EXPECT_NO_THROWS(function() { cluster.rejoinInstance(__sandbox_uri2) });

check_gr_settings(cluster, [__endpoint1, __endpoint2], "MYSQL");

//@<> rebootClusterFromCompleteOutage() must honor the communication stack and ensure the right settings
shutdown_cluster(cluster);
shell.connect(__sandbox_uri1);

EXPECT_NO_THROWS(function() { cluster = dba.rebootClusterFromCompleteOutage("test", {rejoinInstances: [__endpoint2]}) });

if (__version_num < 80027) {
  check_gr_settings(cluster, [__endpoint1, __endpoint2], "XCOM");
} else {
  check_gr_settings(cluster, [__endpoint1, __endpoint2], "MYSQL");
}

//@<> cluster.status() must display the communication stack in use when extended is enabled
if (__version_num < 80027) {
  EXPECT_EQ("XCOM", cluster.status({extended: 1})["defaultReplicaSet"]["communicationStack"]);
} else {
  EXPECT_EQ("MYSQL", cluster.status({extended: 1})["defaultReplicaSet"]["communicationStack"]);
}

//@<> cluster.options() must display the communication stack in use if the cluster supports it
if (__version_num < 80027) {
  var global_options = cluster.options()["defaultReplicaSet"]["globalOptions"];
  var found = false;

  for (option of global_options) {
    if (option["option"] == "communicationStack") {
      found = true;
    }
  }

  EXPECT_FALSE(found, "unexpected communicationStack option found");
} else {
  var global_options = cluster.options()["defaultReplicaSet"]["globalOptions"];
  var found = false;

  for (option of global_options) {
    if (option["option"] == "communicationStack") {
      EXPECT_EQ("MYSQL", option["value"]);
      found = true;
    }
  }

  EXPECT_TRUE(found, "communicationStack option not found");
}

// Migrate a cluster to a different protocol with rebootClusterFromCompleteOutage()
shutdown_cluster(cluster);
shell.connect(__sandbox_uri1);

//@<> Attempt to switch to the comm stack on a cluster that does not support it {VER(<8.0.27)}
EXPECT_THROWS_TYPE(function() { dba.rebootClusterFromCompleteOutage("test", {switchCommunicationStack: "mysql"}) }, "Option 'switchCommunicationStack' not supported on target server version: '" + __version + "'", "RuntimeError");

EXPECT_THROWS_TYPE(function() { dba.rebootClusterFromCompleteOutage("test", {switchCommunicationStack: "XCOM"}) }, "Option 'switchCommunicationStack' not supported on target server version: '" + __version + "'", "RuntimeError");

//@<> Setting an allowList must be forbidden when also switching the comm stack to MYSQL {VER(>=8.0.27)}
EXPECT_THROWS_TYPE(function() { dba.rebootClusterFromCompleteOutage("test", {switchCommunicationStack: "mySql", ipAllowlist: "localhost"}) }, "Cannot use 'ipAllowlist' when setting the 'switchCommunicationStack' option to 'MYSQL'", "ArgumentError");

//@<> Reboot with localAddress set to a port other than the one in use by the server must be forbidden {VER(>=8.0.27)}
shell.connect(__sandbox_uri1);

var local_address_err = __hostname_uri1 + ":" + __mysql_sandbox_port2;

EXPECT_THROWS_TYPE(function() { dba.rebootClusterFromCompleteOutage("test", {switchCommunicationStack: "mysql", localAddress: local_address_err}) }, `Invalid port '${__mysql_sandbox_port2}' for localAddress option. When using 'MYSQL' communication stack, the port must be the same in use by MySQL Server`, "ArgumentError");

//@<> Attempt to switch the communication stack while rebooting a cluster but with unreachable members (must fail) {VER(>=8.0.27)}
testutil.stopSandbox(__mysql_sandbox_port2);
EXPECT_THROWS_TYPE(function() { dba.rebootClusterFromCompleteOutage("test", {switchCommunicationStack: "mysql"}) }, `Unable to switch the communication stack. The following instances: '${__endpoint2}' are unreachable.`, "RuntimeError");
testutil.startSandbox(__mysql_sandbox_port2);

//@<> Switch the communication stack while rebooting a cluster from complete outage (switch to XCOM) {VER(>=8.0.27)}
EXPECT_NO_THROWS(function() { cluster = dba.rebootClusterFromCompleteOutage("test", {rejoinInstances: [__endpoint2], switchCommunicationStack: "XCOM"}) });

check_gr_settings(cluster, [__endpoint1, __endpoint2], "XCOM");

//@<> Switch the communication stack while rebooting a cluster from complete outage (switch to MYSQL) {VER(>=8.0.27)}
shutdown_cluster(cluster);
shell.connect(__sandbox_uri1);

EXPECT_NO_THROWS(function() { cluster = dba.rebootClusterFromCompleteOutage("test", {rejoinInstances: [__endpoint2], switchCommunicationStack: "mySqL"}) });

testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");

check_gr_settings(cluster, [__endpoint1, __endpoint2], "MYSQL");

//@<> Test recovery account re-creation {VER(>=8.0.27)}
// Remove the latest member added to the cluster (sandbox3) and verify that a restart of any other member is successful, meaning the recovery accounts were re-created accordingly and they're not using the one from sandbox3
EXPECT_NO_THROWS(function() { cluster.addInstance(__sandbox_uri3); });
EXPECT_NO_THROWS(function() { cluster.removeInstance(__sandbox_uri3); });
testutil.restartSandbox(__mysql_sandbox_port2);
shell.connect(__sandbox_uri1);
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");

//@<> Switch the communication stack while rebooting a cluster from complete outage and set localAddress and ipAllowlist (switch to XCOM) {VER(>=8.0.27)}
shutdown_cluster(cluster);
shell.connect(__sandbox_uri1);

var __cfg_local_address = localhost + ":" + (__mysql_sandbox_port3*10+1);
var __allow_list = "localhost," + hostname_ip;

EXPECT_NO_THROWS(function() { cluster = dba.rebootClusterFromCompleteOutage("test", {rejoinInstances: [__endpoint2], switchCommunicationStack: "XCOM", localAddress: __cfg_local_address, ipAllowlist: __allow_list}) });

EXPECT_OUTPUT_CONTAINS("WARNING: The value used for 'localAddress' only applies to the current session instance (seed). If the values generated automatically for other rejoining Cluster members are not valid, please use <Cluster>.rejoinInstance() with the 'localAddress' option.");

check_gr_settings(cluster, [__endpoint1, __endpoint2], "XCOM", __cfg_local_address, __allow_list);

//@<> Switch the communication stack while rebooting a cluster from complete outage and set localAddress (switch to MYSQL) {VER(>=8.0.27)}
shutdown_cluster(cluster);
shell.connect(__sandbox_uri1);

var __cfg_local_address = localhost + ":" + __mysql_sandbox_port1;

EXPECT_NO_THROWS(function() { cluster = dba.rebootClusterFromCompleteOutage("test", {rejoinInstances: [__endpoint2], switchCommunicationStack: "MYSQL", localAddress: __cfg_local_address}) });

EXPECT_OUTPUT_CONTAINS("WARNING: The value used for 'localAddress' only applies to the current session instance (seed). If the values generated automatically for other rejoining Cluster members are not valid, please use <Cluster>.rejoinInstance() with the 'localAddress' option.");

check_gr_settings(cluster, [__endpoint1, __endpoint2], "MYSQL", __cfg_local_address);

//@<> Switch the communication stack while rebooting a cluster from complete outage and rejoin instances separately (switch to XCOM) {VER(>=8.0.27)}
shutdown_cluster(cluster);
shell.connect(__sandbox_uri1);

EXPECT_NO_THROWS(function() { cluster = dba.rebootClusterFromCompleteOutage("test", {switchCommunicationStack: "XCOM"}) });

check_gr_settings(cluster, [__endpoint1], "XCOM");

var __cfg_local_address = localhost + ":" + __mysql_sandbox_port4;
// Test the usage of localAddress in rejoinInstance too
EXPECT_NO_THROWS(function() { cluster.rejoinInstance(__endpoint2, {localAddress: __cfg_local_address}); });

session2 = mysql.getSession(__sandbox_uri2);
var configured_local_address = session2.runSql("SELECT @@group_replication_local_address").fetchOne()[0];

EXPECT_EQ(configured_local_address, __cfg_local_address);

//@<> Switch the communication stack while rebooting a cluster from complete outage and rejoin instances separately (switch to MYSQL) {VER(>=8.0.27)}
shutdown_cluster(cluster);
shell.connect(__sandbox_uri1);

EXPECT_NO_THROWS(function() { cluster = dba.rebootClusterFromCompleteOutage("test", {switchCommunicationStack: "MYSQL"}) });

check_gr_settings(cluster, [__endpoint1], "MYSQL");

EXPECT_NO_THROWS(function() { cluster.rejoinInstance(__endpoint2); });

check_gr_settings(cluster, [__endpoint1, __endpoint2], "MYSQL");

//@<> A failure in addInstance() while using MYSQL comm stack must not leave inconsistencies related to the recovery accounts and/or metadata {VER(>=8.0.27) && !__dbug_off}
testutil.dbugSet("+d,fail_add_instance_mysql_stack");

EXPECT_THROWS_TYPE(function() { cluster.addInstance(__sandbox_uri3, {recoveryMethod: "clone"}) }, "debug", "LogicError");

var status = cluster.status();
EXPECT_FALSE("instanceErrors" in status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port1}`]);
EXPECT_FALSE("instanceErrors" in status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port2}`]);

testutil.dbugSet("");

//@<> Cleanup
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
testutil.destroySandbox(__mysql_sandbox_port3);
