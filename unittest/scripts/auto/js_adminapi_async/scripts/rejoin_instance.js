//@ {VER(>=8.0.11)}

// Tests rejoinInstance() function for async replicasets.

//@ INCLUDE async_utils.inc

//@<> Initialization.
var uuid1 = "5ef81566-9395-11e9-87e9-111111111111";
var uuid2 = "5ef81566-9395-11e9-87e9-222222222222";
testutil.deploySandbox(__mysql_sandbox_port1, "root", {"report_host": hostname_ip, server_uuid: uuid1, server_id: 11});
testutil.deploySandbox(__mysql_sandbox_port2, "root", {"report_host": hostname_ip, server_uuid: uuid2, server_id: 22});
var session1 = mysql.getSession(__sandbox_uri1);
var session2 = mysql.getSession(__sandbox_uri2);

//@<> Create Async replicaset.
shell.connect(__sandbox_uri1);
var rs = dba.createReplicaSet("myrs", {gtidSetIsComplete:true});
rs.addInstance(__sandbox_uri2);

//@ Invalid parameters (fail).
rs.rejoinInstance();
rs.rejoinInstance(null);
rs.rejoinInstance(0);
rs.rejoinInstance({});
rs.rejoinInstance([]);
rs.rejoinInstance("");
rs.rejoinInstance("", {});
rs.rejoinInstance(__sandbox1, {}, {});
rs.rejoinInstance(__sandbox1, {badOption:123});
rs.rejoinInstance(__sandbox3);
rs.rejoinInstance(__sandbox1, {recoveryMethod: "bogus"});
rs.rejoinInstance(__sandbox1, {recoveryMethod: "clone", waitRecovery:42});
rs.rejoinInstance(__sandbox1, {recoveryMethod: "incremental", waitRecovery:42});
rs.rejoinInstance(__sandbox1, {recoveryMethod: "incremental", cloneDonor:__sandbox1});
rs.rejoinInstance(__sandbox1, {recoveryMethod: "clone", cloneDonor:""});
rs.rejoinInstance(__sandbox1, {recoveryMethod: "clone", cloneDonor:"foobar"});
rs.rejoinInstance(__sandbox1, {recoveryMethod: "clone", cloneDonor:"root@foobar:3232"});
// IPv6 not supported for cloneDonor. We check for auto-chosen donors that are IPv6 in simple_ipv6.js
rs.rejoinInstance(__sandbox1, {recoveryMethod: "clone", cloneDonor:"[::1]:3232"});
rs.rejoinInstance(__sandbox1, {recoveryMethod: "clone", cloneDonor:"::1:3232"});
rs.rejoinInstance(__sandbox1, {recoveryMethod: "clone", cloneDonor:"::1"});

//@ Try rejoin ONLINE instance (fail).
rs.rejoinInstance(__sandbox2);

//@<> Deploy 3rd instance.
var uuid3 = "5ef81566-9395-11e9-87e9-333333333333";
testutil.deploySandbox(__mysql_sandbox_port3, "root", {"report_host": hostname_ip, server_uuid: uuid3, server_id: 33});
var session3 = mysql.getSession(__sandbox_uri3);

//@ Try rejoin instance not belonging to any replicaset (fail).
rs.rejoinInstance(__sandbox3);

//@<> Create a 2nd replicaset with instance 3.
shell.connect(__sandbox_uri3);
var rs2 = dba.createReplicaSet("myrs_2", {gtidSetIsComplete:true});
rs2.disconnect();

//@ Try rejoin instance belonging to another replicaset (fail).
shell.connect(__sandbox_uri1);
rs = dba.getReplicaSet();
rs.rejoinInstance(__sandbox3);

//@<> Remove instance 3 from the 2nd replicaset.
shell.connect(__sandbox_uri3);
rs2 = dba.getReplicaSet();
rs2.disconnect();
//TODO(pjesus): removeInstance() fails when executed on a PRIMARY, thus we
//              cannot add it to another replicaset. We should be able to
//              dissolve a replicaset (remove all instances and clear the
//              metadata) using the AdminAPI.
reset_instance(session3);

//@<> Add 3rd instance to Async replicaset.
rs.addInstance(__sandbox_uri3);

//@ Try rejoin instance with disconnected rs object (fail).
rs.disconnect();
rs.rejoinInstance(__sandbox3);
rs = dba.getReplicaSet();

//@ Try rejoin instance with a user different from the cluster admin user (fail).
other_user_uri = "test_usr:testpass@" + __sandbox3;
rs.rejoinInstance(other_user_uri);

//@<> Stop replication at instance 3.
sb3 = hostname_ip+":"+__mysql_sandbox_port3;
session3.runSql("STOP SLAVE");
s = rs.status();
EXPECT_EQ(s.replicaSet.topology[sb3].status, "OFFLINE");

//@<> Change a required setting to verify that rejoin fails {VER(<8.0.23)}
session3.runSql("SET GLOBAL binlog_format='STATEMENT'");

//@ Rejoin instance with wrong settings (fail) {VER(<8.0.23)}
rs.rejoinInstance(__sandbox3);

//@<> Change the required setting back to the required value {VER(<8.0.23)}
session3.runSql("SET GLOBAL binlog_format='ROW'");

//@<> Change a required parallel-appliers setting to verify that rejoin fails {VER(>=8.0.23)}
session3.runSql("SET GLOBAL slave_preserve_commit_order=OFF");

//@ Rejoin instance with wrong parallel-applier settings (fail) {VER(>=8.0.23)}
rs.rejoinInstance(__sandbox3);

//@<> Change the required parallel-appliers setting back to the required value {VER(>=8.0.23)}
session3.runSql("SET GLOBAL slave_preserve_commit_order=ON");

//@ Rejoin instance with replication stopped (succeed).
rs.rejoinInstance(__sandbox3);
s = rs.status();
EXPECT_EQ(s.replicaSet.topology[sb3].status, "ONLINE");

//@<> Stop SQL thread at instance 3.
session3.runSql("STOP SLAVE SQL_THREAD");
s = rs.status();
EXPECT_EQ(s.replicaSet.topology[sb3].status, "OFFLINE");

//@ Rejoin instance with replication SQL thread stopped (succeed).
rs.rejoinInstance(__sandbox3);
s = rs.status();
EXPECT_EQ(s.replicaSet.topology[sb3].status, "ONLINE");

//@<> Stop IO thread at instance 3.
session3.runSql("STOP SLAVE IO_THREAD");
s = rs.status();
EXPECT_EQ(s.replicaSet.topology[sb3].status, "OFFLINE");

//@ Rejoin instance with replication IO thread stopped (succeed).
rs.rejoinInstance(__sandbox3);
s = rs.status();
EXPECT_EQ(s.replicaSet.topology[sb3].status, "ONLINE");

//@<> Reset and Stop replication at instance 3.
sb3 = hostname_ip+":"+__mysql_sandbox_port3;
session3.runSql("STOP SLAVE");
session3.runSql("RESET SLAVE ALL");
s = rs.status();
EXPECT_EQ(s.replicaSet.topology[sb3].status, "ERROR");

//@ Rejoin instance with replication reset and stopped (succeed).
rs.rejoinInstance(__sandbox3);
s = rs.status();
EXPECT_EQ(s.replicaSet.topology[sb3].status, "ONLINE");

//@<> Shutdown primary and failover to instance 2.
testutil.stopSandbox(__mysql_sandbox_port1, {wait:1});
shell.connect(__sandbox_uri2);
rs = dba.getReplicaSet();
rs.forcePrimaryInstance(__sandbox2);

//@<> Restart old primary and confirm it is INVALIDATED.
sb1 = hostname_ip+":"+__mysql_sandbox_port1;
testutil.startSandbox(__mysql_sandbox_port1);
s = rs.status();
EXPECT_EQ(s.replicaSet.topology[sb1].status, "INVALIDATED");

//@ Rejoin old primary to replicaset (success) and confirm status.
rs.rejoinInstance(__sandbox1);
s = rs.status();
EXPECT_EQ(s.replicaSet.topology[sb1].status, "ONLINE");

//@<> Add an errant transaction on instance 3.
// NOTE: Get the GTID of the errant transaction to fix later.
gtid_executed = session3.runSql("SELECT @@GTID_EXECUTED").fetchOne()[0];
inject_errant_gtid(session3);
errant_trx_gtid = session3.runSql("SELECT GTID_SUBTRACT(@@GTID_EXECUTED,'" + gtid_executed + "')").fetchOne()[0];
s = rs.status();
EXPECT_EQ(s.replicaSet.topology[sb3].status, "INCONSISTENT");

//@ Try to rejoin instance with errant transaction (fail).
rs.rejoinInstance(__sandbox3, {recoveryMethod: "incremental"});

//@<> Fix the errant transaction (inject empty transaction).
inject_empty_trx(session2, errant_trx_gtid);
s = rs.status();
EXPECT_EQ(s.replicaSet.topology[sb3].status, "ONLINE");

//@<> Add a replication error at instance 3 (no errant trx, disabling binary log).
session3.runSql("SET GLOBAL super_read_only = 0");
session3.runSql("SET GLOBAL read_only = 0");
session3.runSql("SET sql_log_bin = 0");
session3.runSql("CREATE DATABASE error_trx_db");
session3.runSql("SET sql_log_bin = 1");
session3.runSql("SET GLOBAL super_read_only = 1");

session2.runSql("CREATE DATABASE error_trx_db");
testutil.waitForRplApplierError(__mysql_sandbox_port3, "");
s = rs.status();
EXPECT_EQ(s.replicaSet.topology[sb3].status, "ERROR");

//@ Try to rejoin instance with unsolved replication error (fail).
rs.rejoinInstance(__sandbox3);

//@<> Fix the replication error.
session3.runSql("SET GLOBAL super_read_only = 0");
session3.runSql("SET GLOBAL read_only = 0");
session3.runSql("SET sql_log_bin = 0");
session3.runSql("DROP DATABASE error_trx_db");
session3.runSql("SET sql_log_bin = 1");
session3.runSql("SET GLOBAL super_read_only = 1");
s = rs.status();
EXPECT_EQ(s.replicaSet.topology[sb3].status, "ERROR");

//@ Rejoin instance after solving replication error (succeed).
rs.rejoinInstance(__sandbox3);
s = rs.status();
EXPECT_EQ(s.replicaSet.topology[sb3].status, "ONLINE");

//@<> Add a connection failure (change password of rpl user for instance 3).
rpl_user3 = "mysql_innodb_rs_33";
session2.runSql("SET PASSWORD FOR '" + rpl_user3 + "'@'%' = 'wrong_pass'");
session3.runSql("STOP SLAVE");
session3.runSql("START SLAVE");
s = rs.status();
EXPECT_EQ(s.replicaSet.topology[sb3].status, "ERROR");

//@ Rejoin instance with connection failure, rpl user password reset (succeed).
rs.rejoinInstance(__sandbox3);
s = rs.status();
EXPECT_EQ(s.replicaSet.topology[sb3].status, "ONLINE");

//@<> Stop replication at instance 3 and purge transactions from the PRIMARY.
session3.runSql("STOP SLAVE");
session2.runSql("CREATE DATABASE purged_trx_db");
session2.runSql("FLUSH BINARY LOGS");
session2.runSql("PURGE BINARY LOGS BEFORE DATE_ADD(NOW(), INTERVAL 1 DAY)");

// BUG#30884590: ADDING AN INSTANCE WITH COMPATIBLE GTID SET SHOULDN'T PROMPT FOR CLONE
//@ Try to rejoin instance with purged transactions on PRIMARY (should work, clone automatically selected)
rs.rejoinInstance(__sandbox3);

//@<> Stop replication at instance 3..
var session3 = mysql.getSession(__sandbox_uri3);
session3.runSql("STOP SLAVE");

//@ Try to rejoin instance with purged transactions on PRIMARY and gtid-set empty (should fail)
session3.runSql("RESET MASTER");
rs.rejoinInstance(__sandbox3);

//@<> Stop replication at instance 3...
var session3 = mysql.getSession(__sandbox_uri3);
session3.runSql("STOP SLAVE");

//@ Try to rejoin instance with purged transactions on PRIMARY (should work with clone) {VER(>=8.0.17)}
rs.rejoinInstance(__sandbox3, {recoveryMethod: "clone"});

//TODO(pjesus): try rejoin instance belonging to another cluster (fail).

//@<> Stop replication at instance 3
var session3 = mysql.getSession(__sandbox_uri3);
session3.runSql("STOP SLAVE");

//@ cloneDonor valid {VER(>=8.0.17)}
rs.rejoinInstance(__sandbox3, {interactive:true, recoveryMethod:"clone", cloneDonor: __sandbox1});

//@<> Stop replication at instance 3 again
var session3 = mysql.getSession(__sandbox_uri3);
session3.runSql("STOP SLAVE");

//@ cloneDonor valid 2 {VER(>=8.0.17)}
rs.rejoinInstance(__sandbox3, {interactive:true, recoveryMethod:"clone", cloneDonor: __sandbox2});

// BUG#30628746: ADD_INSTANCE: CLONEDONOR FAILS, USER DOES NOT EXIST
// This bug caused a failure when a clone donor was selected that was processing transactions.
// A new sync was added to ensure the donor was in sync with the primary before starting clone
// so to test the fix we need to simulate an wait for that sync to happen. To simplify the test
// we simply lock the mysql.user table triggering that desired wait and wait until the timeout happens.

//@<> BUG#30628746: preparation {VER(>=8.0.17)}
var session3 = mysql.getSession(__sandbox_uri3);
session3.runSql("STOP SLAVE");
var session1 = mysql.getSession(__sandbox_uri1);
session1.runSql("lock tables mysql.user read");

//@ BUG#30628746: wait for timeout {VER(>=8.0.17)}
rs.rejoinInstance(__sandbox3, {interactive:true, timeout:3, recoveryMethod:"clone", cloneDonor: __sandbox1});

//@ BUG#30628746: donor primary should not error with timeout {VER(>=8.0.17)}
rs.rejoinInstance(__sandbox3, {interactive:true, timeout:3, recoveryMethod:"clone", cloneDonor: __sandbox2});
session1.runSql("unlock tables");

//@<> BUG#30632029: preparation
var session3 = mysql.getSession(__sandbox_uri3);
session3.runSql("STOP SLAVE");

// We must verify if the slave is stopped and the channels reset
//@<> BUG#30632029: add instance using clone and a secondary as donor {VER(<8.0.22)}
var bug_30632029 = [
    // "STOP SLAVE FOR CHANNEL ''",
    "RESET SLAVE ALL FOR CHANNEL ''"
];

//@<> BUG#30632029: add instance using clone and a secondary as donor {VER(>=8.0.22)}
var bug_30632029 = [
    // "STOP REPLICA FOR CHANNEL ''", (this is not executed if the channel doesn't exist)
    "RESET REPLICA ALL FOR CHANNEL ''"
];

//@<> BUG#30632029: add instance using clone and a secondary as donor rest
\option dba.logSql = 2
WIPE_SHELL_LOG();

rs.rejoinInstance(__sandbox3, {interactive:true, recoveryMethod:"clone", cloneDonor: __sandbox2});

EXPECT_SHELL_LOG_CONTAINS(bug_30632029[0]);
// EXPECT_SHELL_LOG_CONTAINS(bug_30632029[1]);

//@<> Check if rejoining an instance with a different server_uuid updates the UUID and also doesn't crash (BUG #34038210)

// first try should fail

testutil.stopSandbox(__mysql_sandbox_port3);
testutil.changeSandboxConf(__mysql_sandbox_port3, "server_uuid", "5ef81566-9395-11e9-87e9-333333333302");
testutil.startSandbox(__mysql_sandbox_port3);

EXPECT_EQ("5ef81566-9395-11e9-87e9-333333333333", session.runSql("SELECT mysql_server_uuid FROM mysql_innodb_cluster_metadata.instances WHERE instance_name = ?", [hostname_ip+":"+__mysql_sandbox_port3]).fetchOne()[0]);

EXPECT_THROWS(function() {
    rs.rejoinInstance(__sandbox3, {recoveryMethod:"clone"});
}, `Invalid status to execute operation, ${hostname_ip}:${__mysql_sandbox_port3} is ONLINE`);

EXPECT_EQ("5ef81566-9395-11e9-87e9-333333333333", session.runSql("SELECT mysql_server_uuid FROM mysql_innodb_cluster_metadata.instances WHERE instance_name= ?", [hostname_ip+":"+__mysql_sandbox_port3]).fetchOne()[0]);

//repeat but successfull

testutil.stopSandbox(__mysql_sandbox_port3);
testutil.changeSandboxConf(__mysql_sandbox_port3, "server_uuid", "5ef81566-9395-11e9-87e9-333333333303");
testutil.startSandbox(__mysql_sandbox_port3);

EXPECT_EQ("5ef81566-9395-11e9-87e9-333333333333", session.runSql("SELECT mysql_server_uuid FROM mysql_innodb_cluster_metadata.instances WHERE instance_name= ?", [hostname_ip+":"+__mysql_sandbox_port3]).fetchOne()[0]);

session3 = mysql.getSession(__sandbox_uri3);
session3.runSql("STOP SLAVE");
s = rs.status();
EXPECT_EQ(s.replicaSet.topology[`${hostname_ip}:${__mysql_sandbox_port3}`].status, "OFFLINE");

EXPECT_NO_THROWS(function() { rs.rejoinInstance(__sandbox3, {recoveryMethod:"clone"}); });

EXPECT_EQ("5ef81566-9395-11e9-87e9-333333333303", session.runSql("SELECT mysql_server_uuid FROM mysql_innodb_cluster_metadata.instances WHERE instance_name= ?", [hostname_ip+":"+__mysql_sandbox_port3]).fetchOne()[0]);

//@<> Cleanup.
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
testutil.destroySandbox(__mysql_sandbox_port3);
