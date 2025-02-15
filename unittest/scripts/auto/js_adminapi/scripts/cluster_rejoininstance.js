function check_auto_increment_multi_primary(session, base_value) {

    EXPECT_EQ(base_value, get_sysvar(session, "auto_increment_increment"), "Incorrect auto_increment_increment value.");

    var server_id = session.runSql("SELECT @@server_id").fetchOne()[0];
    var expected_offset = 1 + (server_id % base_value)

    EXPECT_EQ(expected_offset, get_sysvar(session, "auto_increment_offset"), "Incorrect auto_increment_offset value.");
}

//@<> Initialization
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: hostname});
testutil.deploySandbox(__mysql_sandbox_port2, "root", {report_host: hostname});

//@<> Create cluster
shell.connect(__sandbox_uri1);

// Use XCOM comm stack since we'll test allowList that is not allowed with MYSQL comm stack
var c;
if (__version_num < 80027) {
  c = dba.createCluster('test', {gtidSetIsComplete: true});
} else {
  c = dba.createCluster('test', {gtidSetIsComplete: true, communicationStack: "xcom"});
}

//@<> AddInstance
c.addInstance(__sandbox_uri2);
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");

// Simulate the instance dropping the GR group
session.close();
shell.connect(__sandbox_uri2);
session.runSql("STOP group_replication");
session.close();
shell.connect(__sandbox_uri1);
testutil.waitMemberState(__mysql_sandbox_port2, "(MISSING)");

//@<> dryRun test
EXPECT_NO_THROWS(function() { c.rejoinInstance(__sandbox_uri2, {dryRun: 1}); });

EXPECT_OUTPUT_CONTAINS(`The safest and most convenient way to provision a new instance is through automatic clone provisioning, which will completely overwrite the state of '${__endpoint2}' with a physical snapshot from an existing cluster member. To use this method by default, set the 'recoveryMethod' option to 'clone'.
`);
EXPECT_OUTPUT_CONTAINS(`The incremental state recovery may be safely used if you are sure all updates ever executed in the cluster were done with GTIDs enabled, there are no purged transactions and the new instance contains the same GTID set as the cluster or a subset of it. To use this method by default, set the 'recoveryMethod' option to 'incremental'.`);
EXPECT_OUTPUT_CONTAINS(`Incremental state recovery was selected because it seems to be safely usable.`);
EXPECT_OUTPUT_CONTAINS(`Validating instance configuration at localhost:${__mysql_sandbox_port2}...`);
EXPECT_OUTPUT_CONTAINS(`Instance configuration is suitable.`);
EXPECT_OUTPUT_CONTAINS(`Rejoining instance '${__endpoint2}' to cluster 'test'...`);
EXPECT_OUTPUT_CONTAINS(`The instance '${__endpoint2}' was successfully rejoined to the cluster.`);
EXPECT_OUTPUT_CONTAINS(`dryRun finished.`);

// Confirm the rejoin didn't happen
testutil.waitMemberState(__mysql_sandbox_port2, "(MISSING)");

// BUG#29305551: ADMINAPI FAILS TO DETECT INSTANCE IS RUNNING ASYNCHRONOUS REPLICATION
//
// dba.checkInstance() reports that a target instance which is running the Slave
// SQL and IO threads is valid for InnoDB Cluster usage.
//
// As a consequence, the AdminAPI fails to detects that an instance has
// asynchronous replication running and both addInstance() and rejoinInstance()
// fail with useless/unfriendly errors on this scenario. There's not even
// information on how to overcome the issue.

//@<> BUG#29305551: Setup asynchronous replication
// Create Replication user
shell.connect(__sandbox_uri1);
session.runSql("CREATE USER 'repl'@'%' IDENTIFIED BY 'password' REQUIRE SSL");
session.runSql("GRANT REPLICATION SLAVE ON *.* TO 'repl'@'%';");

// Set async channel on instance2
session.close();
shell.connect(__sandbox_uri2);

session.runSql("change " + get_replication_source_keyword() + " TO " + get_replication_option_keyword() + "_HOST='" + hostname + "', " + get_replication_option_keyword() + "_PORT=" + __mysql_sandbox_port1 + ", " + get_replication_option_keyword() + "_USER='repl', " + get_replication_option_keyword() + "_PASSWORD='password', " + get_replication_option_keyword() + "_AUTO_POSITION=1, " + get_replication_option_keyword() + "_SSL=1");
session.runSql("START " + get_replica_keyword());

testutil.waitMemberTransactions(__mysql_sandbox_port2, __mysql_sandbox_port1);

//@ rejoinInstance async replication error
c.rejoinInstance(__sandbox_uri2);

// Stop and reset async channel on instance2
session.runSql("STOP " + get_replica_keyword());

// BUG#32197197: ADMINAPI DOES NOT PROPERLY CHECK FOR PRECONFIGURED REPLICATION CHANNELS
//
// Even if replication is not running but configured, the warning/error has to
// be provided as implemented in BUG#29305551
session.runSql("STOP " + get_replica_keyword());

//@ rejoinInstance async replication error with channels stopped
c.rejoinInstance(__sandbox_uri2);

// BUG#32197197: clean-up
session.runSql("RESET " + get_replica_keyword() + " ALL FOR CHANNEL ''");

//@<> Verify that ipAllowlist sets the right sysvars depending on the version
var ip_allow_list = "10.10.10.1/15,127.0.0.1," + hostname_ip;

c.rejoinInstance(__sandbox_uri2, {ipAllowlist: ip_allow_list});

//@<> Verify that ipAllowlist sets group_replication_ip_allowlist in 8.0.23 {VER(>=8.0.23)}
EXPECT_EQ(ip_allow_list, get_sysvar(session, "group_replication_ip_allowlist"));

//@<> Verify that ipAllowlist sets group_replication_ip_whitelist in versions < 8.0.23 {VER(<8.0.23)}
EXPECT_EQ(ip_allow_list, get_sysvar(session, "group_replication_ip_whitelist"));

//@<> Simulate the instance dropping the GR group to rejoin it again
session.runSql("STOP group_replication");
session.close();
shell.connect(__sandbox_uri1);
testutil.waitMemberState(__mysql_sandbox_port2, "(MISSING)");

//@<> Verify the new option ipAllowlist sets the right sysvars depending on the version
ip_allow_list = "10.1.1.0/15,127.0.0.1," + hostname_ip;

c.rejoinInstance(__sandbox_uri2, {ipAllowlist:ip_allow_list});
session.close();
shell.connect(__sandbox_uri2);

//@<> Verify the new option ipAllowlist sets group_replication_ip_allowlist in 8.0.23 {VER(>=8.0.23)}
EXPECT_EQ(ip_allow_list, get_sysvar(session, "group_replication_ip_allowlist"));

//@<> Verify the new option ipAllowlist sets group_replication_ip_whitelist in versions < 8.0.23 {VER(<8.0.23)}
EXPECT_EQ(ip_allow_list, get_sysvar(session, "group_replication_ip_whitelist"));

//@<> Finalization
session.close();
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);

// BUG#29754915: rejoininstance fails if one or more cluster members have the recovering state
// NOTE: This issue was fixed from the Group Replication side for MySQL 8.0.17 by BUG#29754967.
//@<> BUG#29754915: deploy sandboxes.
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: hostname});
testutil.deploySandbox(__mysql_sandbox_port2, "root", {report_host: hostname});
testutil.deploySandbox(__mysql_sandbox_port3, "root", {report_host: hostname});
testutil.snapshotSandboxConf(__mysql_sandbox_port3);

//@<> BUG#29754915: create cluster.
// Use XCOM comm stack, otherwise, with MYSQL comm stack recovery won't even start
shell.connect(__sandbox_uri1);

var c;
if (__version_num < 80027) {
  c = dba.createCluster('test', {gtidSetIsComplete: true});
} else {
  c = dba.createCluster('test', {gtidSetIsComplete: true, communicationStack: "xcom"});
}

c.addInstance(__sandbox_uri2);
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");
c.addInstance(__sandbox_uri3);
testutil.waitMemberState(__mysql_sandbox_port3, "ONLINE");

//@<> BUG#29754915: keep instance 2 in RECOVERING state by setting a wrong recovery user.
session.close();
shell.connect(__sandbox_uri2);
session.runSql("change " + get_replication_source_keyword() + " TO " + get_replication_option_keyword() + "_USER = 'not_exist', " + get_replication_option_keyword() + "_PASSWORD = '' FOR CHANNEL 'group_replication_recovery'");
session.runSql("STOP GROUP_REPLICATION");
session1 = mysql.getSession(__sandbox_uri1);
session1.runSql("create schema foo");
session.runSql("START GROUP_REPLICATION");
testutil.waitMemberState(__mysql_sandbox_port2, "RECOVERING");

//@<> BUG#29754915: stop Group Replication on instance 3.

// Enable offline_mode (BUG#33396423)
var session3 = mysql.getSession(__sandbox_uri3);
session3.runSql("SET GLOBAL offline_mode=1");
session3.runSql("STOP GROUP_REPLICATION");

//@<> BUG#29754915: get cluster to try to rejoin instance 3.
session.close();
shell.connect(__sandbox_uri1);
var c = dba.getCluster();

//@<> BUG#29754915: rejoin instance 3 successfully.
EXPECT_NO_THROWS(function(){ c.rejoinInstance(__sandbox_uri3); });
testutil.waitMemberState(__mysql_sandbox_port3, "ONLINE");

// ensure offline_mode was disabled (BUG#33396423)
EXPECT_EQ(0, get_sysvar(__mysql_sandbox_port3, "offline_mode"));

// ensure offline_mode wasn't persisted (BUG#34778797) {VER(<8.0.11)}
EXPECT_THROWS(function() { testutil.getSandboxConf(__mysql_sandbox_port3, "offline_mode"); }, "Option 'offline_mode' does not exist in group 'mysqld'");

//@<> ensure offline_mode wasn't persisted (BUG#34778797) {VER(>=8.0.11)}
EXPECT_EQ(0, session3.runSql("SELECT count(*) FROM performance_schema.persisted_variables WHERE (variable_name = 'offline_mode')").fetchOne()[0], `Variable 'offline_mode' is persisted in '${__sandbox_uri3}'`);

//@<> BUG#29754915: confirm cluster status.
session3.close();

var topology = c.status()["defaultReplicaSet"]["topology"];
EXPECT_EQ(topology[`${hostname}:${__mysql_sandbox_port2}`]["status"], "RECOVERING");
EXPECT_EQ(topology[`${hostname}:${__mysql_sandbox_port3}`]["status"], "ONLINE");

//@<> BUG#29754915: clean-up.
c.disconnect();
session.close();
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
testutil.destroySandbox(__mysql_sandbox_port3);

//@<> Initialization IPv6 addresses supported WL#12758 {VER(>= 8.0.14)}
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: "::1"});
testutil.deploySandbox(__mysql_sandbox_port2, "root", {report_host: "::1"});
shell.connect(__sandbox_uri1);

// Use XCOM comm stack since allowLists are to be tested
var cluster;
if (__version_num < 80027) {
  cluster = dba.createCluster('cluster', {gtidSetIsComplete: true});
} else {
  cluster = dba.createCluster('cluster', {gtidSetIsComplete: true, communicationStack: "xcom"});
}

cluster.addInstance(__sandbox_uri2);
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");
session.close();
shell.connect(__sandbox_uri2);
session.runSql("STOP GROUP_REPLICATION");
session.close();
shell.connect(__sandbox_uri1);
cluster = dba.getCluster();

//@<> IPv6 addresses are supported on rejoinInstance ipAllowlist WL#12758 {VER(>= 8.0.14)}
var ip_allow_list = "::1, 127.0.0.1," + hostname_ip;
cluster.rejoinInstance(__sandbox_uri2, {ipAllowlist:ip_allow_list});
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");
session.close();
shell.connect(__sandbox_uri2);

//@<> Confirm that ipAllowlist is set {VER(>= 8.0.14) && VER(<8.0.23)}
EXPECT_EQ(ip_allow_list, get_sysvar(session, "group_replication_ip_whitelist"));

//@<> Confirm that ipAllowlist is set {VER(>= 8.0.23)}
EXPECT_EQ(ip_allow_list, get_sysvar(session, "group_replication_ip_allowlist"));

//@<> localAddress supported on rejoinInstance WL#14765 {VER(>= 8.0.14)}
session.runSql("STOP GROUP_REPLICATION");

var local_address = localhost + ":" + __mysql_sandbox_port4;
cluster.rejoinInstance(__sandbox_uri2, {localAddress: local_address});
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");
session.close();
shell.connect(__sandbox_uri2);

//@<> Confirm that localAddress is set {VER(>= 8.0.14)}
EXPECT_EQ(local_address, get_sysvar(session, "group_replication_local_address"));

//@<> Cleanup IPv6 addresses supported WL#12758 {VER(>= 8.0.14)}
session.close();
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);

//@<> Initialization canonical IPv6 addresses are not supported below 8.0.14 WL#12758 {VER(< 8.0.14)}
testutil.deploySandbox(__mysql_sandbox_port1, "root");
testutil.deploySandbox(__mysql_sandbox_port2, "root");
testutil.snapshotSandboxConf(__mysql_sandbox_port2);
shell.connect(__sandbox_uri1);
var cluster = dba.createCluster("cluster", {gtidSetIsComplete: true});
cluster.addInstance(__sandbox_uri2);
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");
session.close();
shell.connect(__sandbox_uri2);
session.runSql("STOP GROUP_REPLICATION");
session.close();
testutil.changeSandboxConf(__mysql_sandbox_port2, "report_host", "::1");
testutil.restartSandbox(__mysql_sandbox_port2);
shell.connect(__sandbox_uri1);
cluster = dba.getCluster();

//@ canonical IPv6 addresses are not supported below 8.0.14 WL#12758 {VER(< 8.0.14)}
cluster.rejoinInstance(__sandbox_uri2);

//@ IPv6 on ipAllowlist is not supported below 8.0.14 WL#12758 {VER(< 8.0.14)}
testutil.changeSandboxConf(__mysql_sandbox_port2, "report_host", "127.0.0.1");
testutil.restartSandbox(__mysql_sandbox_port2);
cluster.rejoinInstance(__sandbox_uri2, {ipAllowlist: "::1, 127.0.0.1"});

//@<> Cleanup canonical IPv6 addresses are not supported below 8.0.14 WL#12758 {VER(< 8.0.14)}
session.close();
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);

// BUG#29953812 : ADD_INSTANCE() PICKY ABOUT GTID_EXECUTED, REJOIN_INSTANCE() NOT: DATA NOT COPIED
// Verify that rejoinInstance fails when:
//   - Instance data has diverged
//   - Instance is irrecoverable

//@<> Initialization (BUG#29953812)
var scene = new ClusterScenario([__mysql_sandbox_port1, __mysql_sandbox_port2]);
var cluster = scene.cluster;
var session = scene.session;
var session2 = mysql.getSession(__sandbox_uri2);

var gtid_executed = session.runSql("SELECT @@global.gtid_executed").fetchOne()[0];

// Force the instance to drop out of the group
session2.runSql("stop group_replication;")

// Simulate errant transactions
session2.runSql("RESET " + get_reset_binary_logs_keyword());
session2.runSql("SET GLOBAL gtid_purged=?", [gtid_executed+",00025721-1111-1111-1111-111111111111:1"]);

//@ Rejoin instance fails if the target instance contains errant transactions (BUG#29953812) {VER(>=8.0.17)}
cluster.rejoinInstance(__sandbox_uri2);

//@ Rejoin instance fails if the target instance contains errant transactions 5.7 (BUG#29953812) {VER(<8.0.17)}
cluster.rejoinInstance(__sandbox_uri2);

//@<> Rejoin instance with an empty gtid-set - auto incremental since gtidSetIsComplete was used when creating the cluster
session2.runSql("RESET " + get_reset_binary_logs_keyword());

EXPECT_NO_THROWS(function() { cluster.rejoinInstance(__sandbox_uri2); });

//@ Rejoin instance fails if the transactions were purged from the cluster (BUG#29953812)
session2.runSql("stop group_replication;")
session.runSql("FLUSH BINARY LOGS");
session.runSql("PURGE BINARY LOGS BEFORE DATE_ADD(NOW(6), INTERVAL 1 DAY)");
session2.runSql("RESET " + get_reset_binary_logs_keyword());

cluster.rejoinInstance(__sandbox_uri2);

//@<> Finalization (BUG#29953812)
session.close();
session2.close();
scene.destroy();

//@<> BUG#34711038: Verify the values of auto_increment_% multi-primary
var scene = new ClusterScenario([__mysql_sandbox_port1, __mysql_sandbox_port2], {multiPrimary: true, force: true} );
var cluster = scene.cluster;

var result = session.runSql(`SELECT cluster_id, address, instance_name, addresses, attributes, description FROM mysql_innodb_cluster_metadata.instances WHERE (address = '${hostname}:${__mysql_sandbox_port1}') ORDER BY instance_id`);
var row = result.fetchOne();
session.runSql("INSERT INTO mysql_innodb_cluster_metadata.instances(cluster_id, address, mysql_server_uuid, instance_name, addresses, attributes, description) VALUES (?, ?, ?, ?, ?, ?, ?)", [row[0], row[1], "aaaaaaaa-bbbb-cccc-dddd-aaaaaaaaaaaa", row[2], row[3], row[4], row[5]]);
session.runSql("INSERT INTO mysql_innodb_cluster_metadata.instances(cluster_id, address, mysql_server_uuid, instance_name, addresses, attributes, description) VALUES (?, ?, ?, ?, ?, ?, ?)", [row[0], row[1], "aaaaaaaa-bbbb-cccc-dddd-bbbbbbbbbbbb", row[2], row[3], row[4], row[5]]);
session.runSql("INSERT INTO mysql_innodb_cluster_metadata.instances(cluster_id, address, mysql_server_uuid, instance_name, addresses, attributes, description) VALUES (?, ?, ?, ?, ?, ?, ?)", [row[0], row[1], "aaaaaaaa-bbbb-cccc-dddd-cccccccccccc", row[2], row[3], row[4], row[5]]);
session.runSql("INSERT INTO mysql_innodb_cluster_metadata.instances(cluster_id, address, mysql_server_uuid, instance_name, addresses, attributes, description) VALUES (?, ?, ?, ?, ?, ?, ?)", [row[0], row[1], "aaaaaaaa-bbbb-cccc-dddd-dddddddddddd", row[2], row[3], row[4], row[5]]);
session.runSql("INSERT INTO mysql_innodb_cluster_metadata.instances(cluster_id, address, mysql_server_uuid, instance_name, addresses, attributes, description) VALUES (?, ?, ?, ?, ?, ?, ?)", [row[0], row[1], "aaaaaaaa-bbbb-cccc-dddd-eeeeeeeeeeee", row[2], row[3], row[4], row[5]]);
session.runSql("INSERT INTO mysql_innodb_cluster_metadata.instances(cluster_id, address, mysql_server_uuid, instance_name, addresses, attributes, description) VALUES (?, ?, ?, ?, ?, ?, ?)", [row[0], row[1], "aaaaaaaa-bbbb-cccc-dddd-ffffffffffff", row[2], row[3], row[4], row[5]]);

shell.connect(__sandbox_uri2);
session.runSql("STOP GROUP_REPLICATION");
cluster.rejoinInstance(__sandbox_uri2);

shell.connect(__sandbox_uri2);
check_auto_increment_multi_primary(session, 8);

//@<> rejoin instance in ERROR state (should succeed)

// Make instance2 go into ERROR state
shell.connect(__sandbox_uri2);
session.runSql("SET GLOBAL super_read_only = 0");
session.runSql("SET GLOBAL read_only = 0");
session.runSql("SET sql_log_bin = 0");
session.runSql("CREATE DATABASE error_trx_db");
session.runSql("SET sql_log_bin = 1");
session.runSql("SET GLOBAL super_read_only = 1");

shell.connect(__sandbox_uri1);
session.runSql("CREATE DATABASE error_trx_db");

shell.connect(__sandbox_uri2);
testutil.waitMemberState(__mysql_sandbox_port2, "ERROR");

// rejoin with clone (errant transactions) - dryRun test
EXPECT_NO_THROWS(function() { cluster.rejoinInstance(__sandbox_uri2, {dryRun: 1}); });

EXPECT_OUTPUT_CONTAINS(`NOTE: '${__endpoint2}' is in an ERROR state. Attempting to stop Group Replication to proceed with the rejoin.`);
EXPECT_OUTPUT_CONTAINS(`Rejoining instance '${__endpoint2}' to cluster 'cluster'...`);
EXPECT_OUTPUT_CONTAINS(`The instance '${__endpoint2}' was successfully rejoined to the cluster.`);
EXPECT_OUTPUT_CONTAINS(`dryRun finished.`);

// rejoin with clone (errant transactions)
EXPECT_NO_THROWS(function() { cluster.rejoinInstance(__sandbox_uri2, {recoveryMethod: "clone"}); });

shell.connect(__sandbox_uri2);
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");

//@<> Check if proper clone validations regarding compatibility are done {!__dbug_off}
shell.connect(__sandbox_uri2);
reset_instance(session);

shell.connect(__sandbox_uri1);
reset_instance(session);

testutil.deploySandbox(__mysql_sandbox_port3, "root", {report_host: hostname});

cluster = dba.createCluster("sample");

EXPECT_NO_THROWS(function() { cluster.addInstance(__sandbox_uri2, {recoveryMethod: "incremental"}); });
EXPECT_NO_THROWS(function() { cluster.addInstance(__sandbox_uri3, {recoveryMethod: "incremental"}); });

shell.connect(__sandbox_uri2);
session.runSql("STOP group_replication")
session.runSql("RESET " + get_reset_binary_logs_keyword());

testutil.dbugSet("+d,dba_clone_no_compatible_donors");

WIPE_OUTPUT();
EXPECT_THROWS(function(){
    cluster.rejoinInstance(__sandbox_uri2);
}, "Instance provisioning required");
EXPECT_OUTPUT_CONTAINS(`WARNING: None of the online Cluster members are compatible with the recipient (${__endpoint2}) for cloning due to version/platform incompatibilities. Therefore, no instance is available to serve as a valid donor for cloning`);

EXPECT_OUTPUT_CONTAINS("The incremental state recovery may be safely used if you are sure all updates ever executed in the cluster were done with GTIDs enabled, there are no purged transactions and the new instance contains the same GTID set as the cluster or a subset of it. To use this method by default, set the 'recoveryMethod' option to 'incremental'.");

testutil.assertNoPrompts(); // No prompts

// Test with interaction enabled
shell.options.useWizards=1;

EXPECT_THROWS(function(){
    cluster.rejoinInstance(__sandbox_uri2);
}, "Instance provisioning required");

testutil.assertNoPrompts(); // No prompts

WIPE_OUTPUT();

EXPECT_THROWS(function(){
    cluster.rejoinInstance(__sandbox_uri2, {recoveryMethod:"clone"});
}, "Cannot use recoveryMethod=clone because the selected donor is incompatible or no compatible donors are available due to version/platform incompatibilities.");
EXPECT_OUTPUT_CONTAINS(`WARNING: None of the online Cluster members are compatible with the recipient (${__endpoint2}) for cloning due to version/platform incompatibilities. Therefore, no instance is available to serve as a valid donor for cloning`);

testutil.dbugSet("");

scene.destroy();
testutil.destroySandbox(__mysql_sandbox_port3);
