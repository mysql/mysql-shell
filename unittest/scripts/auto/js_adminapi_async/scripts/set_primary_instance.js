//@ {VER(>=8.0.11)}

// Tests setPrimaryInstance() specifically
// Only tests corner cases and negative cases since the positive ones will
// be tested everywhere else.

//@<> INCLUDE async_utils.inc

//@<> Setup

// Set report_host to a valid value, in case hostname is bogus
testutil.deploySandbox(__mysql_sandbox_port1, "root", {"report_host": hostname_ip, server_uuid:"5ef81566-9395-11e9-87e9-111111111111"});
testutil.deploySandbox(__mysql_sandbox_port2, "root", {"report_host": hostname_ip, server_uuid:"5ef81566-9395-11e9-87e9-222222222222"});
testutil.deploySandbox(__mysql_sandbox_port3, "root", {"report_host": hostname_ip, server_uuid:"5ef81566-9395-11e9-87e9-333333333333"});

shell.connect(__sandbox_uri1);
var rs = dba.createReplicaSet("myrs", {gtidSetIsComplete:true});
rs.addInstance(__sandbox_uri2);

testutil.waitMemberTransactions(__mysql_sandbox_port2, __mysql_sandbox_port1);

var session2 = mysql.getSession(__sandbox_uri2);
var session3 = mysql.getSession(__hostname_uri3);

// enable interactive by default
shell.options['useWizards'] = true;

// Negative tests based on environment and params
//--------------------------------

//@<> bad parameters (should fail)
EXPECT_THROWS_TYPE(function(){ rs.setPrimaryInstance(); }, "Invalid number of arguments, expected 1 to 2 but got 0", "ArgumentError");
EXPECT_THROWS_TYPE(function(){ rs.setPrimaryInstance(null); }, "Argument #1 is expected to be a string", "TypeError");
EXPECT_THROWS_TYPE(function(){ rs.setPrimaryInstance(null, null); }, "Argument #1 is expected to be a string", "TypeError");
EXPECT_THROWS_TYPE(function(){ rs.setPrimaryInstance(1, null); }, "Argument #1 is expected to be a string", "TypeError");
EXPECT_THROWS_TYPE(function(){ rs.setPrimaryInstance(__sandbox1, 1); }, "Argument #2 is expected to be a map", "TypeError");
EXPECT_THROWS_TYPE(function(){ rs.setPrimaryInstance(__sandbox1, {}, {}); }, "Invalid number of arguments, expected 1 to 2 but got 3", "ArgumentError");
EXPECT_THROWS_TYPE(function(){ rs.setPrimaryInstance(null, {}); }, "Argument #1 is expected to be a string", "TypeError");
EXPECT_THROWS_TYPE(function(){ rs.setPrimaryInstance({}, {}); }, "Argument #1 is expected to be a string", "TypeError");
EXPECT_THROWS_TYPE(function(){ rs.setPrimaryInstance(__sandbox1, {badOption:123}); }, "Argument #2: Invalid options: badOption", "ArgumentError");
EXPECT_THROWS_TYPE(function(){ rs.setPrimaryInstance([__sandbox1]); }, "Argument #1 is expected to be a string", "TypeError");

//@<> disconnected rs object (should fail)
rs.disconnect();

EXPECT_THROWS_TYPE(function(){
    rs.setPrimaryInstance(__sandbox2);
}, "The replicaset object is disconnected. Please use dba.getReplicaSet() to obtain a new object.", "RuntimeError");

rs = dba.getReplicaSet();

//@<> promoted isn't member (should fail)
EXPECT_THROWS_TYPE(function(){
    rs.setPrimaryInstance(__sandbox3);
}, `Target instance localhost:${__mysql_sandbox_port3} is not a managed instance.`, "MYSQLSH");

//@<> promoted doesn't exist (should fail)
EXPECT_THROWS(function(){
    rs.setPrimaryInstance("localhost:"+__mysql_sandbox_port3+1);
}, `Could not open connection to 'localhost:${__mysql_sandbox_port3}1': Can't connect to MySQL server on '${libmysql_host_description('localhost', "" + __mysql_sandbox_port3 + "1")}'`);

//@<> bad target with a different user (should fail)
EXPECT_THROWS_TYPE(function(){
    rs.setPrimaryInstance("admin@"+__sandbox1);
}, "Invalid target instance specification", "ArgumentError");

EXPECT_OUTPUT_CONTAINS("ERROR: Target instance must be given as host:port. Credentials will be taken from the main session and, if given, must match them");

//@<> bad target with a different password (should fail)
EXPECT_THROWS_TYPE(function(){
    rs.setPrimaryInstance("root:foo@"+__sandbox1);
}, "Invalid target instance specification", "ArgumentError");

//@<> bad target but allowed for compatibility
EXPECT_NO_THROWS(function(){ rs.setPrimaryInstance("root@"+__sandbox1); });
EXPECT_OUTPUT_CONTAINS(`Target instance ${hostname_ip}:${__mysql_sandbox_port1} is already the PRIMARY.`);

//@<> add 3rd instance
rs.addInstance(__sandbox3);

// Positive tests for specific issues
//--------------------------------

//@ check state of instances after switch
rs.setPrimaryInstance(__sandbox2);

// SRO
EXPECT_TRUE(get_sysvar(session, "super_read_only"));
EXPECT_FALSE(get_sysvar(session2, "super_read_only"));
EXPECT_TRUE(get_sysvar(session3, "super_read_only"));
// metadata exists and instance is there
shell.dumpRows(session.runSql("SELECT * FROM mysql_innodb_cluster_metadata.v2_ar_members ORDER BY instance_id"), "tabbed");
// repl should be running
rs.status();
// check that the promoted primary has no repl configured
shell.dumpRows(session2.runSql("SELECT * FROM mysql.slave_master_info"), "tabbed");

//@<OUT> check state of instances after switch - view change records
var cluster_id = session.runSql("SELECT cluster_id FROM mysql_innodb_cluster_metadata.v2_ar_members").fetchOne()[0];
var userhost=session3.runSql("select user()").fetchOne()[0];
shell.dumpRows(session2.runSql("SELECT * FROM mysql_innodb_cluster_metadata.async_cluster_views"), "tabbed");

//@<> promote back
rs.setPrimaryInstance(__sandbox1);
EXPECT_OUTPUT_CONTAINS(`The current PRIMARY is ${hostname_ip}:${__mysql_sandbox_port2}.`);
EXPECT_OUTPUT_CONTAINS(`${hostname_ip}:${__mysql_sandbox_port1} was promoted to PRIMARY.`);

//@<> primary is super-read-only (error ok)
session.runSql("SET GLOBAL super_read_only=1");

EXPECT_THROWS_TYPE(function(){
    rs.setPrimaryInstance(__sandbox2);
}, `Replication or configuration errors at ${hostname_ip}:${__mysql_sandbox_port1}`, "MYSQLSH");

session.runSql("SET GLOBAL super_read_only=0");
session.runSql("SET GLOBAL read_only=0");

//@<> promoted is already primary
EXPECT_NO_THROWS(function(){ rs.setPrimaryInstance(__sandbox1); });
EXPECT_OUTPUT_CONTAINS(`Target instance ${hostname_ip}:${__mysql_sandbox_port1} is already the PRIMARY.`);

//@<> promote via URL
EXPECT_NO_THROWS(function(){ rs.setPrimaryInstance(__sandbox_uri2); });

EXPECT_OUTPUT_CONTAINS(`The current PRIMARY is ${hostname_ip}:${__mysql_sandbox_port1}.`);
EXPECT_OUTPUT_CONTAINS(`${hostname_ip}:${__mysql_sandbox_port2} was promoted to PRIMARY.`);

// Options
//--------------------------------
//@ dryRun

WIPE_OUTPUT();
WIPE_SHELL_LOG();

shell.options["dba.logSql"] = 2;

rs.setPrimaryInstance(__sandbox1, {dryRun:true});
EXPECT_SHELL_LOG_NOT_CONTAINS("FLUSH TABLES WITH READ LOCK");

shell.options["dba.logSql"] = 0;

//@ timeout (should fail)
rs.setPrimaryInstance(__sandbox1);

session2.runSql("FLUSH TABLES WITH READ LOCK");
// sandbox2 slave will be stuck because of the lock
session.runSql("CREATE SCHEMA testdb");

// sandbox2 slave will be stuck trying to apply this because of the lock, so a timeout will occur
var before = strip_status(rs.status());

rs.setPrimaryInstance(__sandbox2, {timeout: 1});

EXPECT_EQ(before, strip_status(rs.status()));

//@ try to switch to a different one - (should fail because sb2 wont sync)
var before = strip_status(rs.status());

rs.setPrimaryInstance(__sandbox3, {timeout: 1});

EXPECT_EQ(before, strip_status(rs.status()));

session2.runSql("UNLOCK TABLES");

//@<> Runtime problems
rs = rebuild_rs();

//@ primary is down (should fail)
testutil.stopSandbox(__mysql_sandbox_port1, {wait:1});

rs.setPrimaryInstance(__sandbox2);

// again with a new rs handle
shell.connect(__sandbox_uri2);
rs = dba.getReplicaSet();
rs.setPrimaryInstance(__sandbox2);

// restore
testutil.startSandbox(__mysql_sandbox_port1);
shell.connect(__sandbox_uri1);
session2.runSql("STOP " + get_replica_keyword());
session2.runSql("START " + get_replica_keyword());
session3.runSql("STOP " + get_replica_keyword());
session3.runSql("START " + get_replica_keyword());

//@ promoted has broken/stopped replication (should fail)
session2.runSql("STOP " + get_replica_keyword());

rs.setPrimaryInstance(__sandbox2);

//@ a secondary has broken replication (should fail)
rs.setPrimaryInstance(__sandbox3);

session2.runSql("START " + get_replica_keyword());

//@ primary has unexpected replication channel (should fail)
session.runSql("change " + get_replication_source_keyword() + " TO " + get_replication_option_keyword() + "_HOST='localhost', " + get_replication_option_keyword() + "_PORT=?, " + get_replication_option_keyword() + "_USER='root', " + get_replication_option_keyword() + "_PASSWORD='root'", [__mysql_sandbox_port2]);
session.runSql("START " + get_replica_keyword());
rs.status();

rs.setPrimaryInstance(__sandbox2);

session.runSql("STOP " + get_replica_keyword());
session.runSql("RESET " + get_replica_keyword() + " ALL");

//@ promoted is down (should fail)
testutil.stopSandbox(__mysql_sandbox_port3, {wait:1});

rs.setPrimaryInstance(__sandbox3);

//@ a secondary is down (should fail)
rs.setPrimaryInstance(__sandbox2);

testutil.startSandbox(__mysql_sandbox_port3);

//@ a secondary has errant GTIDs (should fail)
var session3 = mysql.getSession(__sandbox_uri3);
var before2 = strip_slave_status(session2.runSql("SHOW " + get_replica_keyword() + " STATUS").fetchAll());
var before3 = strip_slave_status(session3.runSql("SHOW " + get_replica_keyword() + " STATUS").fetchAll());

inject_errant_gtid(session3);

rs.setPrimaryInstance(__sandbox2);

EXPECT_EQ([], session.runSql("SHOW " + get_replica_keyword() + " STATUS").fetchAll());
EXPECT_EQ(before2, strip_slave_status(session2.runSql("SHOW " + get_replica_keyword() + " STATUS").fetchAll()));
EXPECT_EQ(before3, strip_slave_status(session3.runSql("SHOW " + get_replica_keyword() + " STATUS").fetchAll()));

//@ Replication conflict error (should fail)
// Create a DB at the slave and then create the same one in the master
rs = rebuild_rs();
inject_applier_error(session, session2);

// Now an error should occur while the slave is catching up, which should
// abort and rollback the addInstance
rs.setPrimaryInstance(__sandbox2);

//@ promoted has errant GTIDs (should fail)
rs = rebuild_rs();
inject_errant_gtid(session2);

rs.setPrimaryInstance(__sandbox2);

// [BEGIN] - BUG#30574971: replicaset tries writing to secondary member

//@<> BUG#30574971 - Initialization, replicaset (only with two members).
rs = rebuild_rs();
rs.removeInstance(__sandbox3);

//@<> BUG#30574971 - Get 2 replicaset objects (rs1 and rs2), connecting to the primary.
// NOTE: execute status() on each object to reproduce issue from BUG#30574971.
var rs1 = dba.getReplicaSet();
rs1.status();

var rs2 = dba.getReplicaSet();
rs2.status();

//@ BUG#30574971 - Switch primary using rs2.
rs2.setPrimaryInstance(__sandbox_uri2);

//@ BUG#30574971 - Add 3rd instance using the other replicaset object rs1.
rs1.addInstance(__sandbox_uri3);

//@<> BUG#30574971 - status for rs1.
var r1 = rs1.status();

//@<> BUG#30574971 - status for rs2.
var r2 = rs2.status();

//@<> BUG#30574971 - rs1 and rs2 status() result should be equal.
EXPECT_EQ(r1, r2);

// [END] - BUG#30574971: replicaset tries writing to secondary member

//@<> Check if changing primary with an instance with a different server_uuid doesn't crash (BUG #34038210)
rs = rebuild_rs();

testutil.stopSandbox(__mysql_sandbox_port2);
testutil.changeSandboxConf(__mysql_sandbox_port2, "server_uuid", "5ef81566-9395-11e9-87e9-333333333302");
testutil.startSandbox(__mysql_sandbox_port2);

testutil.waitMemberTransactions(__mysql_sandbox_port2, __mysql_sandbox_port1);

EXPECT_NO_THROWS(function() { rs.setPrimaryInstance(__sandbox3); });

//@<> Cleanup
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
testutil.destroySandbox(__mysql_sandbox_port3);
