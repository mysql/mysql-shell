//@ {DEF(MYSQLD57_PATH)}

//@<> Deploy sandbox using base server
EXPECT_NO_THROWS(function () {
    testutil.deploySandbox(__mysql_sandbox_port1, "root", { report_host: hostname, general_log:1 });
    testutil.snapshotSandboxConf(__mysql_sandbox_port1);
}, "Deploying 8.0 Sandbox");

var session1 = mysql.getSession(__sandbox_uri1);
var sb1_general_log = session1.runSql("select @@general_log_file").fetchOne()[0];

//@<> Deploy sandbox using 5.7 server
EXPECT_NO_THROWS(function () {
testutil.deploySandbox(__mysql_sandbox_port2, "root", { report_host: hostname, general_log:1 }, {mysqldPath: MYSQLD57_PATH});
testutil.snapshotSandboxConf(__mysql_sandbox_port2);
var mycnf_path2 = testutil.getSandboxConfPath(__mysql_sandbox_port2);
}, "Deploying 5.7 Sandbox");

var session2 = mysql.getSession(__sandbox_uri2);
var sb2_general_log = session2.runSql("select @@general_log_file").fetchOne()[0];

//@<> Create the cluster on the 5.7 instance
var cluster;
EXPECT_NO_THROWS(function () {
    shell.connect(__sandbox_uri2);
    cluster = dba.createCluster("testCluster")
}, "Creating the 5.7 Cluster");

//@<> Add an 8.0 instance to the cluster
EXPECT_NO_THROWS(function () {
    cluster.addInstance(__sandbox_uri1, {recoveryMethod:'incremental', waitRecovery:1});
}, "Adding an 8.0 Instance");

testutil.waitMemberState(__mysql_sandbox_port1, "ONLINE");

cluster.status();

//@<> Remove 5.7 server from cluster, which used to trigger a protocol upgrade
cluster.removeInstance(__sandbox_uri2);

EXPECT_OUTPUT_CONTAINS("NOTE: The communication protocol used by Group Replication can be upgraded to version 8.0.16");

//@<> Add 5.7 server back, which used to trigger a protocol downgrade (add should fail instead)
shell.connect(__sandbox_uri1);
cluster = dba.getCluster();

EXPECT_THROWS(function(){cluster.addInstance(__sandbox_uri2);}, "Instance major version '5' cannot be lower than the cluster lowest major version '8'.");

//@<> Bug#32769580 ADMINAPI MAY UNNECESSARILY INVOKE UDF TO SET GR COMMUNICATION PROTOCOL {__test_execution_mode != 'replay'}

var sb1_grproto = Array.from(testutil.grepFile(sb1_general_log, "group_replication_set_communication_protocol"));

// ensure that protocol version wasn't changed when instance was added
EXPECT_EQ(0, sb1_grproto.length);

//@ get cluster status
shell.connect(__sandbox_uri1);
cluster = dba.getCluster();

// no protocol version but clusterErrors should be in
cluster.status();

//@ extended status should have the protocol version
cluster.status({extended:1});

//@<> Rescan and upgrade protocol
cluster.rescan({upgradeCommProtocol:1});

//@<> Rescan and upgrade protocol (check) {__test_execution_mode != 'replay'}
var sb1_grproto = Array.from(testutil.grepFile(sb1_general_log, "group_replication_set_communication_protocol"));

// ensure that protocol was upgraded to 8.0.16 when instance was added
EXPECT_EQ(1, sb1_grproto.length);
EXPECT_EQ("SELECT group_replication_set_communication_protocol('8.0.16')", sb1_grproto[0].split("\t")[2]);

//@<> Rescan and upgrade protocol again
cluster.rescan({upgradeCommProtocol:1});

//@<> Rescan and upgrade protocol again (check) {__test_execution_mode != 'replay'}
var sb1_grproto = Array.from(testutil.grepFile(sb1_general_log, "group_replication_set_communication_protocol"));

// ensure that protocol version wasn't changed again when instance was added
EXPECT_EQ(1, sb1_grproto.length);

//@ upgraded status
cluster.status({extended:1});

//@<> Finalize
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);



