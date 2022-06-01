//@ {VER(>=8.0.27)}

// Tests for Group Replication's protocol stack management. Among other
// internal changes and details, the tests cover the main features:
//
//  - New option 'communicationStack' in  ClusterSet.createReplicaCluster()
//  - Automatic settings management
//  - Information about the protocol in .status() .options()
//  - Migration between protocols in dba.rebootClusterFromCompleteOutage() with
//  a new option 'switchCommunicationStack'

//@<> INCLUDE gr_utils.inc
//@<> INCLUDE clusterset_utils.inc

//@<> Setup
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: hostname});
testutil.deploySandbox(__mysql_sandbox_port2, "root", {report_host: hostname});
testutil.deploySandbox(__mysql_sandbox_port3, "root", {report_host: hostname});
testutil.deploySandbox(__mysql_sandbox_port4, "root", {report_host: hostname});

shell.connect(__sandbox_uri1);
cluster = dba.createCluster("test", {gtidSetIsComplete: 1});
cs = cluster.createClusterSet("cs");

//@<> Bad options in createReplicaCluster() (should fail)
EXPECT_THROWS_TYPE(function() { cs.createReplicaCluster(__sandbox_uri2, "replica", {communicationStack: "mysql", ipAllowlist: "localhost"}) }, "Cannot use 'ipAllowlist' when setting the 'communicationStack' option to 'MYSQL'", "ArgumentError");

EXPECT_THROWS_TYPE(function() { cs.createReplicaCluster(__sandbox_uri2, "replica", {ipAllowlist: "localhost"}) }, "Cannot use 'ipAllowlist' when the 'communicationStack' is 'MYSQL'", "ArgumentError");

var local_address_xcom = hostname + ":" + __mysql_sandbox_gr_port3;

EXPECT_THROWS_TYPE(function() { cs.createReplicaCluster(__sandbox_uri2, "replica", {communicationStack: "mysql", localAddress: local_address_xcom}) }, "Invalid port '" + __mysql_sandbox_gr_port3 + "' for localAddress option. When using 'MYSQL' communication stack, the port must be the same in use by MySQL Server", "ArgumentError");

//@<> By default, 'MySQL' must be the default communicationStack for >= 8.0.27
var replica;
EXPECT_NO_THROWS(function() { replica = cs.createReplicaCluster(__sandbox_uri2, "replica") });

session2 = mysql.getSession(__sandbox_uri2);

var communication_stack = session2.runSql("select @@group_replication_communication_stack").fetchOne()[0];
EXPECT_EQ("MYSQL", communication_stack);

// Confirm the generated value for localAddres is the same as the network
// address MySQL is listening on
check_gr_settings(replica, [__endpoint2], "MYSQL");

CHECK_REPLICA_CLUSTER([__sandbox_uri2], cluster, replica);

// Remove the Replica Cluster
cs.removeCluster("replica");

CHECK_REMOVED_CLUSTER([__sandbox_uri2], cluster, "replica");

//@<> set localAddress to a port other than the one in use by the server must be forbidden
var local_address_err = __hostname_uri1 + ":" + __mysql_sandbox_port3;

EXPECT_THROWS_TYPE(function() { cs.createReplicaCluster(__sandbox_uri2, "replica2", {communicationStack: "mysql", localAddress: local_address_err}) }, `Invalid port '${__mysql_sandbox_port3}' for localAddress option. When using 'MYSQL' communication stack, the port must be the same in use by MySQL Server`, "ArgumentError");

//@<> set localAddress to use the same port where server is running must be accepted
EXPECT_NO_THROWS(function() { replica = cs.createReplicaCluster(__sandbox_uri2, "replica2", {communicationStack: "mysql", localAddress: __endpoint2, recoveryMethod: "clone"}) });

check_gr_settings(replica, [__endpoint2], "MYSQL");
CHECK_REPLICA_CLUSTER([__sandbox_uri2], cluster, replica);

//@<> Bad options in addInstance() (should fail)
EXPECT_THROWS_TYPE(function() { replica.addInstance(__sandbox_uri3, {ipAllowlist: "localhost"}) }, "Cannot use 'ipAllowlist' when the Cluster's communication stack is 'MYSQL'", "ArgumentError");

EXPECT_NO_THROWS(function() {replica.addInstance(__sandbox_uri3, {recoveryMethod: "clone"}) });

check_gr_settings(replica, [__endpoint2, __endpoint3], "MYSQL");
CHECK_REPLICA_CLUSTER([__sandbox_uri2, __sandbox_uri3], cluster, replica);

//@<> rejoinInstance() must automatically generate and/or ensure the settings too
shell.connect(__sandbox_uri3);
session.runSql("STOP group_replication;");
session.runSql("RESET PERSIST group_replication_local_address");
session.runSql("RESET PERSIST group_replication_group_seeds");
session.runSql("RESET PERSIST group_replication_recovery_use_ssl");

// Bad options in rejoinInstance() (should fail)
EXPECT_THROWS_TYPE(function() { replica.rejoinInstance(__sandbox_uri3, {ipAllowlist: "localhost"}) }, "Cannot use 'ipAllowlist' when the Cluster's communication stack is 'MYSQL'", "ArgumentError");

EXPECT_NO_THROWS(function() { replica.rejoinInstance(__sandbox_uri3) });

check_gr_settings(replica, [__endpoint2, __endpoint3], "MYSQL");
CHECK_REPLICA_CLUSTER([__sandbox_uri2, __sandbox_uri3], cluster, replica);

//@<> rebootClusterFromCompleteOutage() must honor the communication stack and ensure the right settings
shutdown_cluster(replica);
shell.connect(__sandbox_uri2);

EXPECT_NO_THROWS(function() { replica = dba.rebootClusterFromCompleteOutage("replica2") });

check_gr_settings(replica, [__endpoint2, __endpoint3], "MYSQL");
CHECK_REPLICA_CLUSTER([__sandbox_uri2, __sandbox_uri3], cluster, replica);

//@<> clusterser.status() must display the communication stack in use when extended is enabled
EXPECT_EQ("MYSQL", cs.status({extended: 2})["clusters"]["replica2"]["communicationStack"]);

EXPECT_EQ("MYSQL", cs.status({extended: 2})["clusters"]["test"]["communicationStack"]);

// Migrate a cluster to a different protocol with rebootClusterFromCompleteOutage()
shutdown_cluster(replica);
shell.connect(__sandbox_uri2);

//@<> Switch the communication stack while rebooting a cluster from complete outage (switch to XCOM)
EXPECT_NO_THROWS(function() { replica = dba.rebootClusterFromCompleteOutage("replica2", {switchCommunicationStack: "XCOM"}) });

testutil.waitMemberState(__mysql_sandbox_port3, "ONLINE");

check_gr_settings(replica, [__endpoint2, __endpoint3], "XCOM");
CHECK_REPLICA_CLUSTER([__sandbox_uri2, __sandbox_uri3], cluster, replica);

//@<> Switch the communication stack while rebooting a cluster from complete outage (switch to MYSQL)
shutdown_cluster(replica);
shell.connect(__sandbox_uri2);

EXPECT_NO_THROWS(function() { replica = dba.rebootClusterFromCompleteOutage("replica2", {switchCommunicationStack: "mySqL"}) });

check_gr_settings(replica, [__endpoint2, __endpoint3], "MYSQL");
CHECK_REPLICA_CLUSTER([__sandbox_uri2, __sandbox_uri3], cluster, replica);

//@<> Test recovery account re-creation
// Remove the primary member of the replica cluster (sandbox2) and verify that a restart of any other member is successful, meaning the recovery accounts were re-created accordingly and they're not using the one from sandbox2
EXPECT_NO_THROWS(function() { replica.addInstance(__sandbox_uri4, {recoveryMethod: "clone"}); });
EXPECT_NO_THROWS(function() { replica.setPrimaryInstance(__sandbox_uri3) });
EXPECT_NO_THROWS(function() { replica.removeInstance(__sandbox_uri2); });
//ClusterSet switchover
EXPECT_NO_THROWS(function() { cs.setPrimaryCluster("replica2"); });
CHECK_PRIMARY_CLUSTER([__sandbox_uri3, __sandbox_uri4], replica);
CHECK_REPLICA_CLUSTER([__sandbox_uri1], replica, cluster);

testutil.restartSandbox(__mysql_sandbox_port4);
shell.connect(__sandbox_uri3);
testutil.waitMemberState(__mysql_sandbox_port4, "ONLINE");

//@<> Switch the communication stack while rebooting a cluster from complete outage and set localAddress and ipAllowlist (switch to XCOM) {VER(>=8.0.27)}
shutdown_cluster(replica);
shell.connect(__sandbox_uri3);

var __cfg_local_address = localhost + ":" + (__mysql_sandbox_port5*10+1);
var __allow_list = "localhost," + hostname_ip;

EXPECT_NO_THROWS(function() { replica = dba.rebootClusterFromCompleteOutage("replica2", {switchCommunicationStack: "XCOM", localAddress: __cfg_local_address, ipAllowlist: __allow_list}) });

check_gr_settings(replica, [__endpoint3, __endpoint4], "XCOM", __cfg_local_address, __allow_list);
CHECK_PRIMARY_CLUSTER([__sandbox_uri3, __sandbox_uri4], replica);

//@<> Switch the communication stack while rebooting a cluster from complete outage and set localAddress (switch to MYSQL) {VER(>=8.0.27)}
shutdown_cluster(replica);
shell.connect(__sandbox_uri3);

var __cfg_local_address = localhost + ":" + __mysql_sandbox_port3;

EXPECT_NO_THROWS(function() { replica = dba.rebootClusterFromCompleteOutage("replica2", {switchCommunicationStack: "MYSQL", localAddress: __cfg_local_address}) });

check_gr_settings(replica, [__endpoint3, __endpoint4], "MYSQL", __cfg_local_address);
CHECK_PRIMARY_CLUSTER([__sandbox_uri3, __sandbox_uri4], replica);

//@<> Test auto-rejoin of primary and replica cluster members
shell.connect(__sandbox_uri1);
cluster = dba.getCluster();
EXPECT_NO_THROWS(function() { cluster.addInstance(__sandbox_uri2, {recoveryMethod: "clone"}) });

// Primary Cluster: [sandbox3, sandbox4]
// Replica Cluster: [sandbox1, sandbox2]

testutil.restartSandbox(__mysql_sandbox_port2);
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");

shell.connect(__sandbox_uri3);
testutil.restartSandbox(__mysql_sandbox_port4);
testutil.waitMemberState(__mysql_sandbox_port4, "ONLINE");

//@<> Cleanup
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
testutil.destroySandbox(__mysql_sandbox_port3);
testutil.destroySandbox(__mysql_sandbox_port4);
