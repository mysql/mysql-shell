// <Cluster.>status() tests

//@<> Create cluster
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host:hostname});
testutil.deploySandbox(__mysql_sandbox_port2, "root", {report_host:hostname});
testutil.deploySandbox(__mysql_sandbox_port3, "root", {report_host:hostname});

shell.connect(__sandbox_uri1);
cluster = dba.createCluster("cluster", {gtidSetIsComplete:1});

// BUG#32582745 - ADMINAPI: OPERATIONS LOGGING USELESS MD STATE INFORMATION
// When run in debug mode, the MD state check logging should be present
WIPE_SHELL_LOG()
var user_path = testutil.getUserConfigPath();
testutil.callMysqlsh([__sandbox_uri1, "--log-level=debug", "--", "cluster", "status"], "", ["MYSQLSH_TERM_COLOR_MODE=nocolor", "MYSQLSH_USER_CONFIG_HOME=" + user_path])
EXPECT_SHELL_LOG_CONTAINS("Debug: Detected state of MD schema as OK");

cluster.addInstance(__sandbox_uri2);

//@<> set recoveryAccount to empty user
//BUG#32157182
session.runSql("UPDATE mysql_innodb_cluster_metadata.instances SET attributes = json_remove(attributes, '$.recoveryAccountUser', '$.recoveryAccountHost')");
var status = cluster.status();
EXPECT_EQ("WARNING: The replication recovery account in use by the instance is not stored in the metadata. Use Cluster.rescan() to update the metadata.", status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port1}`]["instanceErrors"][0])
cluster.rescan();
var status = cluster.status();
EXPECT_FALSE("instanceErrors" in status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port1}`]);
EXPECT_SHELL_LOG_CONTAINS("Fixing instances missing the replication recovery account...");
WIPE_STDOUT()

EXPECT_EQ(0, session.runSql("SELECT COUNT(*) FROM mysql_innodb_cluster_metadata.instances WHERE (attributes->>'$.recoveryAccountUser') IS NULL").fetchOne()[0], "Rescan didn't fix all recovery accounts");

//@<> set recoveryAccount to invalid user
//BUG#32157182
session.runSql("UPDATE mysql_innodb_cluster_metadata.instances SET attributes =  json_set(COALESCE(attributes, '{}'),  '$.recoveryAccountUser', 'invalid_user', '$.recoveryAccountHost', '%')");
var status = cluster.status();
EXPECT_EQ("WARNING: The replication recovery account in use by the instance is not stored in the metadata. Use Cluster.rescan() to update the metadata.", status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port1}`]["instanceErrors"][0])
cluster.rescan();
var status = cluster.status();
EXPECT_FALSE("instanceErrors" in status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port1}`]);
EXPECT_SHELL_LOG_CONTAINS("Fixing instances missing the replication recovery account...");
WIPE_STDOUT()
EXPECT_EQ(0, session.runSql("SELECT COUNT(*) FROM mysql_innodb_cluster_metadata.instances WHERE (attributes->>'$.recoveryAccountUser') IS NULL").fetchOne()[0], "Rescan didn't fix all recovery accounts");

//@<> set recoveryAccount to invalid user that looks like cluster created user
//BUG#32157182
session.runSql("UPDATE mysql_innodb_cluster_metadata.instances SET attributes =  json_set(COALESCE(attributes, '{}'),  '$.recoveryAccountUser', 'mysql_innodb_cluster_100000', '$.recoveryAccountHost', '%')");
var status = cluster.status();
EXPECT_EQ("WARNING: The replication recovery account in use by the instance is not stored in the metadata. Use Cluster.rescan() to update the metadata.", status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port1}`]["instanceErrors"][0])
cluster.rescan();
var status = cluster.status();
EXPECT_FALSE("instanceErrors" in status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port1}`]);
EXPECT_SHELL_LOG_CONTAINS("Fixing instances missing the replication recovery account...");
WIPE_STDOUT()
EXPECT_EQ(0, session.runSql("SELECT COUNT(*) FROM mysql_innodb_cluster_metadata.instances WHERE (attributes->>'$.recoveryAccountUser') IS NULL").fetchOne()[0], "Rescan didn't fix all recovery accounts");

//@<> set recoveryAccount to invalid user that looks like old cluster created user
//BUG#32157182
session.runSql("UPDATE mysql_innodb_cluster_metadata.instances SET attributes =  json_set(COALESCE(attributes, '{}'),  '$.recoveryAccountUser', 'mysql_innodb_cluster_r100000', '$.recoveryAccountHost', '%')");
var status = cluster.status();
EXPECT_EQ("WARNING: The replication recovery account in use by the instance is not stored in the metadata. Use Cluster.rescan() to update the metadata.", status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port1}`]["instanceErrors"][0])
cluster.rescan();
var status = cluster.status();
EXPECT_FALSE("instanceErrors" in status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port1}`]);
EXPECT_SHELL_LOG_CONTAINS("Fixing instances missing the replication recovery account...");
WIPE_STDOUT()
EXPECT_EQ(0, session.runSql("SELECT COUNT(*) FROM mysql_innodb_cluster_metadata.instances WHERE (attributes->>'$.recoveryAccountUser') IS NULL").fetchOne()[0], "Rescan didn't fix all recovery accounts");

//@<> set recoveryAccount to custom user
//BUG#32157182
testutil.deploySandbox(__mysql_sandbox_port4, "root", {report_host:hostname});
cluster.addInstance(__sandbox_uri4);
shell.connect(__sandbox_uri4);
session.runSql("CHANGE MASTER TO MASTER_USER='custom_user', MASTER_PASSWORD='custom_user' FOR CHANNEL 'group_replication_recovery'");
var status = cluster.status();
EXPECT_EQ("WARNING: Unsupported recovery account 'custom_user' has been found for instance '<<<hostname>>>:<<<__mysql_sandbox_port4>>>'. Operations such as Cluster.resetRecoveryAccountsPassword() and Cluster.addInstance() may fail. Please remove and add the instance back to the Cluster to ensure a supported recovery account is used.", status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port4}`]["instanceErrors"][0])
WIPE_STDOUT()
cluster.rescan();
EXPECT_STDOUT_CONTAINS("ERROR: Unsupported recovery account 'custom_user' has been found for instance '<<<hostname>>>:<<<__mysql_sandbox_port4>>>'. Operations such as <Cluster>.resetRecoveryAccountsPassword() and <Cluster>.addInstance() may fail. Please remove and add the instance back to the Cluster to ensure a supported recovery account is used.")

//@<> clean up cluster - remove sandbox 4 from cluster
//BUG#32157182
shell.connect(__sandbox_uri1);
cluster.removeInstance(__sandbox_uri4)
testutil.destroySandbox(__mysql_sandbox_port4);


//@<> prepare testdb
function create_testdb(session)  {
    session.runSql("set global super_read_only=0");
    session.runSql("set sql_log_bin=0");
    session.runSql("create schema if not exists testdb");
    session.runSql("create table if not exists testdb.data (k int primary key auto_increment, v longtext)");
    session.runSql("set sql_log_bin=1");
    session.runSql("set global super_read_only=1");
}

// insert some data on sb1
session.runSql("create schema if not exists testdb");
session.runSql("create table if not exists testdb.data (k int primary key auto_increment, v longtext)");


// Exceptions in <Cluster.>status():
//    - If the InnoDB Cluster topology mode does not match the current Group
//      Replication configuration.

//@<> Manually change the topology mode in the metadata
session.runSql("SET sql_log_bin = 0");
session.runSql("update mysql_innodb_cluster_metadata.clusters set primary_mode = \"mm\"");
session.runSql("SET sql_log_bin = 0");

var cluster = dba.getCluster();

//@ Error when executing status on a cluster with the topology mode different than GR
cluster.status();

//@<> Manually change back the topology mode in the metadata and change the cluster name
session.runSql("SET sql_log_bin = 0");
session.runSql("update mysql_innodb_cluster_metadata.clusters set primary_mode = \"pm\"");
session.runSql("update mysql_innodb_cluster_metadata.clusters set cluster_name = \"newName\"");
session.runSql("SET sql_log_bin = 0");

//@ Error when executing status on a cluster that its name is not registered in the metadata
cluster.status();

//@<> Manually change back the the cluster name
session.runSql("SET sql_log_bin = 0");
session.runSql("update mysql_innodb_cluster_metadata.clusters set cluster_name = \"cluster\"");
session.runSql("SET sql_log_bin = 0");
var cluster = dba.getCluster();


// ---- memberState

//@ memberState=OFFLINE
// and instanceError with GR stopped

var session1 = mysql.getSession(__sandbox_uri1);
var session2 = mysql.getSession(__sandbox_uri2);
var session3 = mysql.getSession(__sandbox_uri3);

session2.runSql("stop group_replication");

shell.connect(__sandbox_uri1);
var cluster = dba.getCluster();
cluster.status();

//@ memberState=ERROR
// and applierChannel with error
// and instanceError with applier channel error

cluster.rejoinInstance(__sandbox_uri2);
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");
// force an applier error
session2.runSql("set global super_read_only=0");
session2.runSql("set sql_log_bin=0");
session2.runSql("create schema foobar");
session2.runSql("set sql_log_bin=1");
session2.runSql("set global super_read_only=1");
session1.runSql("create schema foobar");

testutil.waitMemberState(__mysql_sandbox_port2, "(MISSING)");
cluster.status();

cluster.status({extended:1});

//@<> reset sandbox2
cluster.removeInstance(__sandbox_uri2, {force:1});
session2.runSql("stop group_replication");
reset_instance(session2);

//@<> 1 RECOVERING, 1 UNREACHABLE {false}
// Note: this test currently doesn't work because it requires getCluster() to
// work on a cluster with only a RECOVERING member (no ONLINE), which isn't supported
create_testdb(session2);

// now we want to add the instance and leave it stuck on recovering
cluster.addInstance(__sandbox_uri2);

session2.runSql("stop group_replication");

session1.runSql("insert into testdb.data values (default, repeat('#', 200000))");
// freeze recovery
session2.runSql("lock tables testdb.data read");
cluster.rejoinInstance(__sandbox_uri2);

status = cluster.status();

EXPECT_EQ("ONLINE", status.defaultReplicaSet.topology[__endpoint1].status);
EXPECT_EQ("RECOVERING", status.defaultReplicaSet.topology[__endpoint2].status);

testutil.stopSandbox(__mysql_sandbox_port1, {wait:1});

// sb1 is down, so we need to get the cluster through sb3, because sb2 doesn't
// have the MD replicated yet (b/c of the frozen recovery)
shell.connect(__sandbox_uri2);
cluster = dba.getCluster();

status = cluster.status();

session2.runSql("unlock tables");

// now should be ONLINE
status = cluster.status();

// ---- recovery status

//@ recoveryChannel during recovery

cluster.addInstance(__sandbox_uri3);

// create the table in sb2 so we can lock it
create_testdb(session2);
session2.runSql("lock tables testdb.data read");

session1.runSql("insert into testdb.data values (default, repeat('x', 200000))");

cluster.addInstance(__sandbox_uri2, {waitRecovery:0});

cluster.status({extended:1});

session2.runSql("unlock tables");

//@ recoveryChannel with error
// and instanceError with recovery channel error
cluster.removeInstance(__sandbox_uri2);
reset_instance(session2);
create_testdb(session2);
session2.runSql("set global group_replication_recovery_retry_count=2");

session1.runSql("insert into testdb.data values (default, repeat('!', 200000))");
session1.runSql("insert into testdb.data values (42, 'conflict')");

// force recovery error
session2.runSql("set global super_read_only=0");
session2.runSql("set sql_log_bin=0");
session2.runSql("insert into testdb.data values (42, 'breakme')");
session2.runSql("set sql_log_bin=1");
session2.runSql("set global super_read_only=1");

// freeze recovery when it tries to insert to this table
session2.runSql("lock tables testdb.data read");

cluster.addInstance(__sandbox_uri2, {waitRecovery:0});

session2.runSql("unlock tables");

cluster.status();
wait(200, 1, function(){return cluster.status().defaultReplicaSet.topology[hostname+":"+__mysql_sandbox_port2].status == "(MISSING)"; });

cluster.status();
// ---- other instanceErrors

//@ instanceError with split-brain
session2.runSql("stop group_replication");
session2.runSql("set global group_replication_bootstrap_group=1");
session2.runSql("start group_replication");
session2.runSql("set global group_replication_bootstrap_group=0");

cluster.status();

//@ instanceError with unmanaged member
session2.runSql("stop group_replication");
cluster.removeInstance(__sandbox_uri2, {force:1});
reset_instance(session2);
cluster.addInstance(__sandbox_uri2);
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");

session1.runSql("delete from mysql_innodb_cluster_metadata.instances where instance_name like ?", ["%"+__mysql_sandbox_port2]);

cluster.status();

// insert back the instance
cluster.rescan({addInstances:"auto"});

//@ instanceError with an instance that has its UUID changed
shell.connect(__sandbox_uri1);
cluster = dba.getCluster();

var row = session2.runSql("select instance_id, mysql_server_uuid from mysql_innodb_cluster_metadata.instances where mysql_server_uuid=cast(@@server_uuid as char)").fetchOne();
instance_id = row[0];
uuid = row[1];
session1.runSql("update mysql_innodb_cluster_metadata.instances set mysql_server_uuid=uuid() where instance_id=?", [instance_id]);

cluster.status();

//@ instanceError mismatch would not be reported correctly if the member is not in the group
// Bug#31991496 INSTANCEERRORS DO NOT PILE UP IN CLUSTER.STATUS
session2.runSql("stop group_replication");

cluster.status();

session2.runSql("start group_replication");
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");

//@<> restore the original uuid
session1.runSql("update mysql_innodb_cluster_metadata.instances set mysql_server_uuid=? where instance_id=?", [uuid, instance_id]);

//@<OUT> Status cluster
var stat = cluster.status();
println(stat);

// BUG#32015164: MISSING INFORMATION ABOUT REQUIRED PARALLEL-APPLIERS SETTINGS ON UPGRADE SCNARIO
// If a cluster member with a version >= 8.0.23 doesn't have parallel-appliers enabled, that information
// must be included in 'instanceErrors'

//@<> BUG#32015164: preparation {VER(>=8.0.23)}
// Stop GR
session2.runSql("STOP group_replication");
testutil.waitMemberState(__mysql_sandbox_port2, "(MISSING)");
// Disable parallel-appliers
testutil.removeFromSandboxConf(__mysql_sandbox_port2, "binlog_transaction_dependency_tracking");
testutil.removeFromSandboxConf(__mysql_sandbox_port2, "slave_parallel_type");
testutil.removeFromSandboxConf(__mysql_sandbox_port2, "slave_preserve_commit_order");
testutil.removeFromSandboxConf(__mysql_sandbox_port2, "transaction_write_set_extraction");
testutil.removeFromSandboxConf(__mysql_sandbox_port2, "slave_parallel_workers");
testutil.snapshotSandboxConf(__mysql_sandbox_port2);
testutil.restartSandbox(__mysql_sandbox_port2);
// Let the instance rejoin the cluster
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");

//@ BUG#32015164: instanceErrors must report missing parallel-appliers {VER(>=8.0.23)}
cluster.status();

//@<> BUG#32015164: fix with dba.configureInstance() {VER(>=8.0.23)}
dba.configureInstance(__sandbox_uri2);

//@<> BUG#32015164: rejoin instance after fix {VER(>=8.0.23)}
testutil.restartSandbox(__mysql_sandbox_port2);
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");

//@<OUT> BUG#32015164: status should be fine now {VER(>=8.0.23)}
var stat = cluster.status();
println(stat);

// BUG#32226871: MISSING INFORMATION ABOUT METADATA NOT INCLUDING SERVER_ID ON UPGRADE SCENARIO
// If a cluster member with a version >= 8.0.23 doesn't have the server_id registered in the
// metadata that information must be included in 'instanceErrors'

//@<> BUG#32226871: preparation (remove server_id to simulate the upgrade) {VER(>=8.0.23)}
session.runSql(`UPDATE mysql_innodb_cluster_metadata.instances set attributes=JSON_REMOVE(attributes, '$.server_id') WHERE address= '${hostname}:${__mysql_sandbox_port2}'`);

//@<> BUG#32226871: instanceErrors must report server_id {VER(>=8.0.23)}
EXPECT_EQ(cluster.status()["defaultReplicaSet"]["topology"][hostname+":"+__mysql_sandbox_port2]["instanceErrors"][0],
                "NOTE: instance server_id is not registered in the metadata. Use cluster.rescan() to update the metadata.");

//@<> BUG#32226871: fix with cluster.rescan() {VER(>=8.0.23)}
cluster.rescan()

//@<> BUG#32226871: status should be fine now (no instanceErrors regarding server_id) {VER(>=8.0.23)}
EXPECT_FALSE("instanceErrors" in cluster.status()["defaultReplicaSet"]["topology"][hostname+":"+__mysql_sandbox_port2])

//@<> WL#13084 - TSF4_5: extended: 0 is the default (same as with no options).
var ext_0_status = cluster.status({extended: 0});
EXPECT_EQ(ext_0_status, stat);

//@<> WL#13084 - TSF4_1: extended: false is the same as extended: 0.
var ext_false_status =cluster.status({extended: false});
EXPECT_EQ(ext_false_status, ext_0_status);

//@<> WL#13084 - TSF2_1: Set super_read_only to ON on primary.
set_sysvar(session, "super_read_only", 1);

//@<> WL#13084 - TSF2_1: Set super_read_only to OFF on a secondary.
// also checks instanceError with super_read_only=0 on a secondary
session.close();
shell.connect(__sandbox_uri2);
set_sysvar(session, "super_read_only", 0);
session.close();

//@<OUT> WL#13084 - TSF2_1: status show accurate mode based on super_read_only.
shell.connect(__sandbox_uri1);
var cluster = dba.getCluster();
cluster.status();

//@<> WL#13084 - TSF2_1: Reset super_read_only back on primary and secondary.
session.close();
shell.connect(__sandbox_uri2);
set_sysvar(session, "super_read_only", 1);
session.close();
shell.connect(__sandbox_uri1);
set_sysvar(session, "super_read_only", 0);
var cluster = dba.getCluster();

//@<> Remove two instances of the cluster and add back one of them with a different label
cluster.removeInstance(__sandbox_uri2);
cluster.removeInstance(__sandbox_uri3);
// NOTE: set the highest weight so it's picked as primary
cluster.addInstance(__sandbox_uri2, {label: "zzLabel", memberWeight: 100});
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");

//@<OUT> Status cluster with 2 instances having one of them a non-default label
cluster.status();

//@<> Add R/O instance back to the cluster
cluster.addInstance(__sandbox_uri3);
testutil.waitMemberState(__mysql_sandbox_port3, "ONLINE");

//@<OUT> Status cluster after adding R/O instance back
cluster.status()

//@<> Execute status from a read-only member of the cluster
cluster.disconnect();
session.close();
shell.connect(__sandbox_uri2);
var cluster = dba.getCluster();

//@<OUT> Status from a read-only member
cluster.status();

//@<> Remove primary instance to force the election of a new one
cluster.removeInstance(__sandbox_uri1);
cluster.disconnect();
session.close();
shell.connect(__sandbox_uri2);
var cluster = dba.getCluster();

//@<OUT> Status with a new primary
cluster.status();

//@<OUT> WL#13084 - TSF5_1: queryMembers option is deprecated (true).
cluster.status({queryMembers:true});

//@<OUT> WL#13084 - TSF5_1: queryMembers option is deprecated (false).
cluster.status({queryMembers:false});

// BUG#30401048: ERROR VERIFYING METADATA VERSION
// Regression test to ensure that when a consistency level of "BEFORE", "AFTER" or
// "BEFORE_AND_AFTER" was set in the cluster any other AdminAPI call can be executed
// concurrently

//@<> BUG#30401048: Add instance 1 back to the cluster and change the consistency level to "BEFORE_AND_AFTER" {VER(>=8.0.14)}
cluster.setOption("consistency", "BEFORE_AND_AFTER");
cluster.addInstance(__sandbox_uri1);
testutil.waitMemberState(__mysql_sandbox_port1, "ONLINE");

//@<> BUG#30401048: Keep instance 1 in RECOVERING state {VER(>=8.0.14)}
session.close();
shell.connect(__sandbox_uri1);
session.runSql("CHANGE MASTER TO MASTER_USER = 'not_exist', MASTER_PASSWORD = '' FOR CHANNEL 'group_replication_recovery'");
session.runSql("STOP GROUP_REPLICATION");
session.runSql("START GROUP_REPLICATION");
testutil.waitMemberState(__mysql_sandbox_port1, "RECOVERING");
session.close();
shell.connect(__sandbox_uri2);

//@<> BUG#30401048: get the cluster from a different instance and run status() {VER(>=8.0.14)}
var cluster = dba.getCluster();
cluster.status();

//@<> Check that cluster.status() timeout is < 10s
// Bug #30884174   CLUSTER.STATUS() HANGS FOR TOO LONG WHEN CONNECTION/READ TIMEOUTS HAPPEN

// force timeout by making cluster.status() try to connect to the GR port
var iid = session.runSql("select instance_id from mysql_innodb_cluster_metadata.instances order by instance_id desc limit 1").fetchOne()[0];
session.runSql("update mysql_innodb_cluster_metadata.instances set addresses=json_set(addresses, '$.mysqlClassic', concat(addresses->>'$.mysqlClassic', '1')) where instance_id=?", [iid])
var start = new Date();

cluster.status();

var end = new Date();

// time elapsed should be around 5000ms, but we give an extra 1s margin
EXPECT_GT(6000, end-start);

//@<> Finalization
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
testutil.destroySandbox(__mysql_sandbox_port3);
