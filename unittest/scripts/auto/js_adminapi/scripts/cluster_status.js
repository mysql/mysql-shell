// <Cluster.>status() tests

//@<> Create cluster
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host:hostname});
testutil.deploySandbox(__mysql_sandbox_port2, "root", {report_host:hostname});
testutil.deploySandbox(__mysql_sandbox_port3, "root", {report_host:hostname});

shell.connect(__sandbox_uri1);
cluster = dba.createCluster("cluster", {gtidSetIsComplete:1});
cluster.addInstance(__sandbox_uri2);

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

session1.runSql("update mysql_innodb_cluster_metadata.instances set mysql_server_uuid=? where instance_id=?", [uuid, instance_id]);

//@<OUT> Status cluster
var stat = cluster.status();
println(stat);

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
