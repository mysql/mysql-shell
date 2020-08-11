//@ {VER(>=8.0.11)}

// Tests addInstance() specifically
// Only tests corner cases and negative cases since the positive ones will
// be tested everywhere else.

//@ INCLUDE async_utils.inc

//@ Setup

// Set report_host to a valid value, in case hostname is bogus
var uuid1 = "5ef81566-9395-11e9-87e9-111111111111";
var uuid2 = "5ef81566-9395-11e9-87e9-222222222222";
testutil.deploySandbox(__mysql_sandbox_port1, "root", {"report_host": hostname_ip, server_uuid: uuid1, server_id:11});
testutil.deploySandbox(__mysql_sandbox_port2, "root", {"report_host": hostname_ip, server_uuid: uuid2, server_id:22});

shell.connect(__sandbox_uri1);
var rs = dba.createReplicaSet("myrs", {gtidSetIsComplete:true});

var session1 = mysql.getSession(__sandbox_uri1);
var session2 = mysql.getSession(__sandbox_uri2);

__endpoint_uri1 = hostname_ip+":"+__mysql_sandbox_port1;
__endpoint_uri2 = hostname_ip+":"+__mysql_sandbox_port2;
__endpoint_uri3 = hostname_ip+":"+__mysql_sandbox_port3;

// enable interactive by default
shell.options['useWizards'] = true;

// Negative tests based on environment and params
//--------------------------------

//@ bad parameters (should fail)
rs.addInstance();
rs.addInstance(null);
rs.addInstance(null, null);
rs.addInstance(1, null);
rs.addInstance(__sandbox1, 1);
rs.addInstance(__sandbox1, {}, {});
rs.addInstance(null, {});
rs.addInstance({}, {});
rs.addInstance(__sandbox1, {badOption:123});
rs.addInstance([__endpoint1]);
rs.addInstance(__sandbox3);
rs.addInstance(__sandbox2, {waitRecovery:0});
rs.addInstance(__sandbox2, {recoveryMethod: "bogus"});
rs.addInstance(__sandbox1, {recoveryMethod: "clone", waitRecovery:42});
rs.addInstance(__sandbox1, {recoveryMethod: "incremental", waitRecovery:42});
rs.addInstance(__sandbox1, {recoveryMethod: "incremental", cloneDonor:__sandbox1});
rs.addInstance(__sandbox1, {recoveryMethod: "clone", cloneDonor:""});
rs.addInstance(__sandbox1, {recoveryMethod: "clone", cloneDonor:"foobar"});
rs.addInstance(__sandbox1, {recoveryMethod: "clone", cloneDonor:"root@foobar:3232"});
// IPv6 not supported for cloneDonor. We check for auto-chosen donors that are IPv6 in simple_ipv6.js
rs.addInstance(__sandbox1, {recoveryMethod: "clone", cloneDonor:"[::1]:3232"});
rs.addInstance(__sandbox1, {recoveryMethod: "clone", cloneDonor:"::1:3232"});
rs.addInstance(__sandbox1, {recoveryMethod: "clone", cloneDonor:"::1"});

//@ disconnected rs object (should fail)
rs.disconnect();
rs.addInstance(__sandbox2);
rs = dba.getReplicaSet();

//@ bad config (should fail)
testutil.deployRawSandbox(__mysql_sandbox_port3, "root", {"report_host": hostname_ip});
testutil.snapshotSandboxConf(__mysql_sandbox_port3);
rs.addInstance(__sandbox3);
testutil.destroySandbox(__mysql_sandbox_port3);

//@ Invalid loopback ip (should fail)
testutil.deploySandbox(__mysql_sandbox_port3, "root", {report_host: "127.0.1.1"});
rs.addInstance(__sandbox3);
testutil.destroySandbox(__mysql_sandbox_port3);

//@ duplicated server_id with 1 server (should fail)
session2.runSql("SET GLOBAL server_id=11");
rs.addInstance(__sandbox2);

//@ duplicated server_uuid with 1 server (should fail)
testutil.stopSandbox(__mysql_sandbox_port2);
testutil.changeSandboxConf(__mysql_sandbox_port2, "server_uuid", uuid1);
testutil.startSandbox(__mysql_sandbox_port2);

rs.addInstance(__sandbox2);

testutil.stopSandbox(__mysql_sandbox_port2);
testutil.changeSandboxConf(__mysql_sandbox_port2, "server_uuid", uuid2);
testutil.startSandbox(__mysql_sandbox_port2);
var session2 = mysql.getSession(__sandbox_uri2);

//@<> ensure the sandbox didn't get added
EXPECT_NE(undefined, rs.status().replicaSet.topology[__endpoint_uri1]);
EXPECT_EQ(undefined, rs.status().replicaSet.topology[__endpoint_uri2]);

//@ duplicated server_id with 2 servers - same as master (should fail)
// (add 2nd instance and prep 3rd sandbox)
rs.addInstance(__sandbox2);
EXPECT_NE(undefined, rs.status().replicaSet.topology[__endpoint_uri2]);
testutil.deploySandbox(__mysql_sandbox_port3, "root", {"report_host": hostname_ip});
var session3 = mysql.getSession(__sandbox_uri3);

session3.runSql("SET GLOBAL server_id=11");
rs.addInstance(__sandbox3);
EXPECT_EQ(undefined, rs.status().replicaSet.topology[__endpoint_uri3]);

//@ duplicated server_id with 2 servers - same as other (should fail)
session3.runSql("SET GLOBAL server_id=22");
rs.addInstance(__sandbox3);
EXPECT_EQ(undefined, rs.status().replicaSet.topology[__endpoint_uri3]);

//@ duplicated server_uuid with 2 servers - same as master (should fail)
session3.runSql("SET GLOBAL server_id=12");

testutil.stopSandbox(__mysql_sandbox_port3);
testutil.changeSandboxConf(__mysql_sandbox_port3, "server_uuid", uuid1);
testutil.startSandbox(__mysql_sandbox_port3);

rs.addInstance(__sandbox3);
EXPECT_EQ(undefined, rs.status().replicaSet.topology[__endpoint_uri3]);

//@ duplicated server_uuid with 2 servers - same as other (should fail)
testutil.stopSandbox(__mysql_sandbox_port3);
testutil.changeSandboxConf(__mysql_sandbox_port3, "server_uuid", uuid2);
testutil.startSandbox(__mysql_sandbox_port3);

rs.addInstance(__sandbox3);
EXPECT_NE(undefined, rs.status().replicaSet.topology[__endpoint_uri2]);
EXPECT_EQ(undefined, rs.status().replicaSet.topology[__endpoint_uri3]);

//@<> go back to master only
rs.status();
rs.removeInstance(__sandbox2);
EXPECT_NE(undefined, rs.status().replicaSet.topology[__endpoint_uri1]);
EXPECT_EQ(undefined, rs.status().replicaSet.topology[__endpoint_uri2]);
EXPECT_EQ(undefined, rs.status().replicaSet.topology[__endpoint_uri3]);

//@ replication filters (should fail)
testutil.destroySandbox(__mysql_sandbox_port3);
testutil.deploySandbox(__mysql_sandbox_port3, "root", {"report_host": hostname_ip});
session3 = mysql.getSession(__sandbox_uri3);
session3.runSql("CHANGE REPLICATION FILTER REPLICATE_IGNORE_DB = (foo)");
rs.addInstance(__sandbox3);

session3.runSql("CHANGE REPLICATION FILTER REPLICATE_IGNORE_DB = ()");

//@ binlog filters (should fail)
reset_instance(session3);
testutil.stopSandbox(__mysql_sandbox_port3);
testutil.changeSandboxConf(__mysql_sandbox_port3, "binlog_ignore_db", "igndata");
testutil.startSandbox(__mysql_sandbox_port3);

rs.addInstance(__sandbox3);

// cleanup
testutil.destroySandbox(__mysql_sandbox_port3);

//@ invalid instance (should fail)
rs.addInstance("localhost:1");

//@ admin account has mismatched passwords (should fail)
session1.runSql("SET SESSION sql_log_bin=0");
session1.runSql("CREATE USER foo@'%' IDENTIFIED BY 'foo'");
session1.runSql("GRANT ALL ON *.* TO foo@'%' WITH GRANT OPTION");
session1.runSql("SET SESSION sql_log_bin=1");

session2.runSql("SET GLOBAL super_read_only=0");
session2.runSql("SET SESSION sql_log_bin=0");
session2.runSql("CREATE USER foo@'localhost' IDENTIFIED BY 'bar'");
session2.runSql("GRANT ALL ON *.* TO foo@'localhost' WITH GRANT OPTION");
session2.runSql("SET SESSION sql_log_bin=1");

shell.connect("mysql://foo:foo@localhost:"+__mysql_sandbox_port1);
rs = dba.getReplicaSet();
rs.addInstance("localhost:"+__mysql_sandbox_port2);

//@ admin account doesn't allow connection from source host (should fail)
// will connect as foo@hostname, but only foo@localhost exists, so should fail
rs.addInstance(hostname+":"+__mysql_sandbox_port2);

//@ bad URI with a different user (should fail)
rs.addInstance("admin2@"+__sandbox2);

//@ bad URI with a different password (should fail)
rs.addInstance("root:bla@"+__sandbox2);

//@ instance running unmanaged GR (should fail)
var session2 = mysql.getSession(__sandbox_uri2);
reset_instance(session2);
start_standalone_gr(session2, __mysql_sandbox_gr_port2);

shell.connect(__sandbox_uri1);
var rs = dba.getReplicaSet();
rs.addInstance(__sandbox2);

stop_standalone_gr(session2);

//@ instance belongs to a rs (should fail)
reset_instance(session2);
shell.connect(__sandbox_uri2);
rs2 = dba.createReplicaSet("rs", {gtidSetIsComplete:true});
rs.addInstance(__sandbox2);

//@ instance running unmanaged AR (should fail)
reset_instance(session2);
setup_slave(session2, __mysql_sandbox_port1);

rs.addInstance(__sandbox2);

//@ instance already in the same rs (should fail)
reset_instance(session2);

// should pass
rs.addInstance(__sandbox2);
// should fail
rs.addInstance(__sandbox2);

//rebuild
reset_instance(session1);
reset_instance(session2);

shell.connect(__sandbox_uri1);
rs = dba.createReplicaSet("myrs", {gtidSetIsComplete:true});

// 3 sandbox tests
//@ prepare rs with 2 members and a 3rd sandbox
rs.addInstance(__sandbox2);

testutil.deploySandbox(__mysql_sandbox_port3, "root", {"report_host": hostname_ip});

//@ add while the instance we got the rs from is down (should fail)
shell.connect(__sandbox_uri1);
var rs = dba.getReplicaSet();
testutil.stopSandbox(__mysql_sandbox_port1, {wait:true});

rs.addInstance(__sandbox3);

//@ add while PRIMARY down (should fail)
testutil.stopSandbox(__mysql_sandbox_port1);
shell.connect(__sandbox_uri2);
var rs = dba.getReplicaSet();
rs.addInstance(__sandbox3);
testutil.startSandbox(__mysql_sandbox_port1);

//@ add while some secondary down
testutil.stopSandbox(__mysql_sandbox_port2);
shell.connect(__sandbox_uri1);
var rs = dba.getReplicaSet();
rs.addInstance(__sandbox3);
rs.removeInstance(__sandbox3);
testutil.startSandbox(__mysql_sandbox_port2);

session2 = mysql.getSession(__sandbox_uri2);

// Positive tests for specific issues
//--------------------------------

//@ addInstance on a URI
shell.connect(__sandbox_uri1);
reset_instance(session);
reset_instance(session2);

var rs = dba.createReplicaSet("myrs", {gtidSetIsComplete:true});
rs.addInstance(__sandbox_uri2);

//@ check state of the added instance
reset_instance(session);
reset_instance(session2);

var rs = dba.createReplicaSet("myrs", {gtidSetIsComplete:true});
rs.addInstance(__sandbox2);

var cluster_id = session.runSql("SELECT cluster_id FROM mysql_innodb_cluster_metadata.clusters").fetchOne()[0];

// added instance should be:
// SRO
EXPECT_EQ(1, get_sysvar(session2, "super_read_only"));
// metadata exists and instance is there
shell.dumpRows(session2.runSql("SELECT * FROM mysql_innodb_cluster_metadata.v2_ar_members ORDER BY instance_id"), "tabbed");
// repl should be running
shell.dumpRows(session2.runSql("SHOW SLAVE STATUS"), "vertical");

rs.removeInstance(__sandbox2);
reset_instance(session2);

//@ target is super-read-only
session2.runSql("SET GLOBAL super_read_only=1");
rs.addInstance(__sandbox2);

rs.removeInstance(__sandbox2);
reset_instance(session2);
session2.runSql("SET GLOBAL super_read_only=0");

//@ target is read-only
session2.runSql("SET GLOBAL read_only=1");
rs.addInstance(__sandbox2);

rs.removeInstance(__sandbox2);
reset_instance(session2);
session2.runSql("SET GLOBAL read_only=0");

// Options
//--------------------------------
//@ dryRun - 1
var session2 = mysql.getSession(__sandbox_uri2);
reset_instance(session2);

var snap1 = repl_snapshot(session);
var snap2 = repl_snapshot(session2);

rs.addInstance(__sandbox2, {label:"blargh", dryRun:true});

EXPECT_JSON_EQ(snap1, repl_snapshot(session));
EXPECT_JSON_EQ(snap2, repl_snapshot(session2));

session2.runSql("SET global super_read_only=1");
var snap2 = repl_snapshot(session2);

//@ dryRun - 2
rs.addInstance(__sandbox2, {label:"blargh", dryRun:true});

EXPECT_JSON_EQ(snap2, repl_snapshot(session2));
session2.runSql("SET global super_read_only=0");

//@ label
rs.addInstance(__sandbox2, {label:"blargh"});
shell.dumpRows(session.runSql("SELECT instance_name FROM mysql_innodb_cluster_metadata.instances"), "tabbed");
rs.removeInstance(__sandbox_uri2);

//@ timeout -1 {__dbug}
testutil.dbugSet("+d,dba_add_instance_master_delay"); // add a master_delay of 3s
session.runSql("create schema foobar1");

rs.addInstance(__sandbox2, {timeout:-1}); // should finish without waiting
EXPECT_EQ(null, session2.runSql("SHOW SCHEMAS LIKE 'foobar1'").fetchOne());
rs.removeInstance(__sandbox_uri2);

//@ timeout 2 and rollback (should fail) {__dbug}
var snap1 = repl_snapshot(session);
var snap2 = repl_snapshot(session2);

session.runSql("create schema foobar2");

rs.addInstance(__sandbox2, {timeout:2}); // should wait for 2s and fail
EXPECT_EQ(null, session2.runSql("SHOW SCHEMAS LIKE 'foobar2'").fetchOne());

rs.status();

// check that the operation rolled back
EXPECT_JSON_EQ(snap1, repl_snapshot(session));
EXPECT_JSON_EQ(snap2, repl_snapshot(session2));

//@ timeout 10 {__dbug}
session.runSql("create schema foobar3");

rs.addInstance(__sandbox2, {timeout:10}); // should wait for 3s
EXPECT_EQ(['foobar3'], session2.runSql("SHOW SCHEMAS LIKE 'foobar3'").fetchOne());
rs.removeInstance(__sandbox_uri2);

//@ timeout 0 {__dbug}
session.runSql("create schema foobar4");

rs.addInstance(__sandbox2, {timeout:0}); // should wait for 3s
EXPECT_EQ(['foobar4'], session2.runSql("SHOW SCHEMAS LIKE 'foobar4'").fetchOne());
rs.removeInstance(__sandbox_uri2);

testutil.dbugSet("");

// Runtime problems
//--------------------------------

//@ rebuild test setup
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
testutil.deploySandbox(__mysql_sandbox_port1, "root", {"report_host": hostname_ip, server_uuid: uuid1, server_id:11});
testutil.deploySandbox(__mysql_sandbox_port2, "root", {"report_host": hostname_ip, server_uuid: uuid2, server_id:22});

shell.connect(__sandbox_uri1);
var rs = dba.createReplicaSet("myrs", {gtidSetIsComplete:true});

//@ Replication conflict error (should fail)
// Create a DB at the slave and then create the same one in the master
var session2 = mysql.getSession(__sandbox_uri2);
reset_instance(session2);
inject_applier_error(session, session2);

// Now an error should occur while the slave is catching up, which should
// abort and rollback
rs.addInstance(__sandbox2);

EXPECT_EQ(undefined, rs.status().replicaSet.topology[__endpoint_uri2]);

// Checks for transaction set compatibility

//@ instance has more GTIDs (should fail)
reset_instance(session2);
inject_errant_gtid(session2);

rs.addInstance(__sandbox2, {recoveryMethod: "incremental"});

//@ instance has a subset of the master GTID set
reset_provision_instance(session2, session);

// Falls-back automatically to incremental recovery
rs.addInstance(__sandbox2);

rs.removeInstance(__sandbox2);

//@ instance has more GTIDs (should work with clone) {VER(>=8.0.17)}
var session2 = mysql.getSession(__sandbox_uri2);
reset_instance(session2);
inject_errant_gtid(session2);
rs.addInstance(__sandbox2, {recoveryMethod: "clone"});

//@<> remove instance (diverged GTID-set) {VER(>=8.0.17)}
rs.removeInstance(__sandbox2);

//@ master has purged GTIDs (should fail)
var session2 = mysql.getSession(__sandbox_uri2);
inject_purged_gtids(session);
reset_instance(session2);

rs.addInstance(__sandbox2, {recoveryMethod: "incremental"});

//@ master has purged GTIDs (should work with clone) {VER(>=8.0.17)}
rs.addInstance(__sandbox2, {recoveryMethod: "clone"});

//@<> remove instance (purged GTIDs) {VER(>=8.0.17)}
rs.removeInstance(__sandbox2);

//@ Re-create the replicaset without gtidSetIsComplete
reset_instance(session);

shell.connect(__sandbox_uri1);
var rs = dba.createReplicaSet("myrs", {gtidSetIsComplete:false});

//@ instance has empty GTID set + gtidSetIsComplete:0 + not interactive (should fail)
var session2 = mysql.getSession(__sandbox_uri2);
reset_instance(session2);

rs.addInstance(__sandbox2, {interactive:false});

//@ instance has empty GTID set + gtidSetIsComplete:0 prompt-no (should fail) {VER(<8.0.17)}
reset_instance(session2);
testutil.expectPrompt("Please select a recovery method [I]ncremental recovery/[A]bort (default Incremental recovery): ", "a");
rs.addInstance(__sandbox2, {interactive:true});

//@ instance has empty GTID set + gtidSetIsComplete:0 prompt-no (should fail) {VER(>=8.0.17)}
reset_instance(session2);
testutil.expectPrompt("Please select a recovery method [C]lone/[I]ncremental recovery/[A]bort (default Clone): ", "a");
rs.addInstance(__sandbox2, {interactive:true});

//@ instance has empty GTID set + gtidSetIsComplete:0 prompt-yes {VER(<8.0.17)}
reset_instance(session2);
testutil.expectPrompt("Please select a recovery method [I]ncremental recovery/[A]bort (default Incremental recovery): ", "i");
rs.addInstance(__sandbox2, {interactive:true});

//@ instance has empty GTID set + gtidSetIsComplete:0 prompt-yes {VER(>=8.0.17)}
reset_instance(session2);
testutil.expectPrompt("Please select a recovery method [C]lone/[I]ncremental recovery/[A]bort (default Clone): ", "i");
rs.addInstance(__sandbox2, {interactive:true});

//@ instance has empty GTID set + gtidSetIsComplete:0 + recoveryMethod:INCREMENTAL
rs.removeInstance(__sandbox2);
reset_instance(session2);

rs.addInstance(__sandbox2, {interactive:true, recoveryMethod:"INCREMENTAL"});

//@ instance has empty GTID set + gtidSetIsComplete:0 + recoveryMethod:CLONE {VER(>=8.0.17)}
rs.removeInstance(__sandbox2);
reset_instance(session2);

rs.addInstance(__sandbox2, {interactive:true, recoveryMethod:"CLONE"});

//@ instance has a subset of the master GTID set + gtidSetIsComplete:0
// Falls-back automatically to incremental recovery
rs.removeInstance(__sandbox2);

rs.addInstance(__sandbox2);

//@ cloneDonor invalid: not a ReplicaSet member {VER(>=8.0.17)}
var session2 = mysql.getSession(__sandbox_uri2);
rs.removeInstance(__sandbox2);
reset_instance(session2);

rs.addInstance(__sandbox2, {interactive:true, recoveryMethod:"clone", cloneDonor: __sandbox3});

//@ cloneDonor valid {VER(>=8.0.17)}
rs.addInstance(__sandbox2, {interactive:true, recoveryMethod:"clone", cloneDonor: __sandbox1});

//@ cloneDonor valid 2 {VER(>=8.0.17)}
rs.addInstance(__sandbox3, {interactive:true, recoveryMethod:"clone", cloneDonor: __sandbox2});

// BUG#30628746: ADD_INSTANCE: CLONEDONOR FAILS, USER DOES NOT EXIST
// This bug caused a failure when a clone donor was selected that was processing transactions.
// A new sync was added to ensure the donor was in sync with the primary before starting clone
// so to test the fix we need to simulate an wait for that sync to happen. To simplify the test
// we simply lock the mysql.user table triggering that desired wait and wait until the timeout happens.

//@<> BUG#30628746: preparation {VER(>=8.0.17)}
rs.removeInstance(__sandbox3);
var session2 = mysql.getSession(__sandbox_uri2);
session2.runSql("lock tables mysql.user read");

//@ BUG#30628746: wait for timeout {VER(>=8.0.17)}
rs.addInstance(__sandbox3, {interactive:true, timeout:3, recoveryMethod:"clone", cloneDonor: __sandbox2});

//@ BUG#30628746: donor primary should not error with timeout {VER(>=8.0.17)}
rs.addInstance(__sandbox3, {interactive:true, timeout:3, recoveryMethod:"clone", cloneDonor: __sandbox1});

session2.runSql("unlock tables");

//@<> BUG#30632029: preparation
rs.removeInstance(__sandbox3);

// We must verify if the slave is stopped and the channels reset
//@<> BUG#30632029: add instance using clone and a secondary as donor {VER(<8.0.22)}
var bug_30632029 = [
    "STOP SLAVE FOR CHANNEL ''",
    "RESET SLAVE ALL FOR CHANNEL ''"
];

//@<> BUG#30632029: add instance using clone and a secondary as donor {VER(>=8.0.22)}
var bug_30632029 = [
    "STOP REPLICA FOR CHANNEL ''",
    "RESET REPLICA ALL FOR CHANNEL ''"
];

\option dba.logSql = 2
WIPE_SHELL_LOG();

rs.addInstance(__sandbox3, {interactive:true, recoveryMethod:"clone", cloneDonor: __sandbox2});

EXPECT_SHELL_LOG_CONTAINS(bug_30632029[0]);
EXPECT_SHELL_LOG_CONTAINS(bug_30632029[1]);
WIPE_SHELL_LOG();

//@ BUG#30281908: add instance using clone and simulating a restart timeout {VER(>= 8.0.17)}
rs.removeInstance(__sandbox3);
testutil.changeSandboxConf(__mysql_sandbox_port3, "foo", "bar");
// Also tests the restartWaitTimeout option
shell.options["dba.restartWaitTimeout"] = 1;
rs.addInstance(__sandbox_uri3, {interactive:true, recoveryMethod:"clone"});
shell.options["dba.restartWaitTimeout"] = 60;

// TODO(miguel):
// BUG#30657911: <REPLICASET>.ADDINSTANCE() USING CLONE SEGFAULTS WHEN RESTART IS NOT AVAILABLE
//<> BUG#30657911: preparation
//rs.removeInstance(__sandbox3);
// BUG#30657911: add instance using clone and simulating a restart timeout
//testutil.dbugSet("+d,dba_abort_monitor_clone_recovery_wait_restart");
//rs.addInstance(__sandbox3, {interactive:true, recoveryMethod:"clone"});
//testutil.dbugSet("");
// BUG#30657911: retry adding instance without using a recoveryMethod
//rs.addInstance(__sandbox3, {interactive:true});

// Rollback
//--------------------------------
// Ensure clean rollback after failures


//@ Cleanup
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
testutil.destroySandbox(__mysql_sandbox_port3);
