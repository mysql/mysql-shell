//@ {VER(>=8.0.11)}

// Tests setPrimaryInstance() specifically
// Only tests corner cases and negative cases since the positive ones will
// be tested everywhere else.

//@ INCLUDE async_utils.inc

//@ Setup

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

//@ bad parameters (should fail)
rs.setPrimaryInstance();
rs.setPrimaryInstance(null);
rs.setPrimaryInstance(null, null);
rs.setPrimaryInstance(1, null);
rs.setPrimaryInstance(__sandbox1, 1);
rs.setPrimaryInstance(__sandbox1, {}, {});
rs.setPrimaryInstance(null, {});
rs.setPrimaryInstance({}, {});
rs.setPrimaryInstance(__sandbox1, {badOption:123});
rs.setPrimaryInstance([__sandbox1]);

//@ disconnected rs object (should fail)
rs.disconnect();
rs.setPrimaryInstance(__sandbox2);
rs = dba.getReplicaSet();

//@ promoted isn't member (should fail)
rs.setPrimaryInstance(__sandbox3);

//@ promoted doesn't exist (should fail)
rs.setPrimaryInstance("localhost:"+__mysql_sandbox_port3+1);

//@ bad target with a different user (should fail)
rs.setPrimaryInstance("admin@"+__sandbox1);

//@ bad target with a different password (should fail)
rs.setPrimaryInstance("root:foo@"+__sandbox1);

//@ bad target but allowed for compatibility
rs.setPrimaryInstance("root@"+__sandbox1);

//@ add 3rd instance
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

//@ promote back
rs.setPrimaryInstance(__sandbox1);

//@ primary is super-read-only (error ok)
session.runSql("SET GLOBAL super_read_only=1");
rs.setPrimaryInstance(__sandbox2);

session.runSql("SET GLOBAL super_read_only=0");
session.runSql("SET GLOBAL read_only=0");

//@ promoted is already primary
rs.setPrimaryInstance(__sandbox1);

//@ promote via URL
rs.setPrimaryInstance(__sandbox_uri2);


// Options
//--------------------------------
//@ dryRun
rs.setPrimaryInstance(__sandbox1, {dryRun:true});

//@ timeout (should fail)
rs.setPrimaryInstance(__sandbox1);

session2.runSql("FLUSH TABLES WITH READ LOCK");
// sandbox2 slave will be stuck because of the lock
session.runSql("CREATE SCHEMA testdb");

// sandbox2 slave will be stuck trying to apply this because of the lock, so a timeout will occur
var before = strip_status(rs.status());

rs.setPrimaryInstance(__sandbox2, {timeout: 1});

EXPECT_EQ(before, strip_status(rs.status()));

//@ try to switch to a different one - (should fail because sb2 wont sync)
var before = strip_status(rs.status());

rs.setPrimaryInstance(__sandbox3, {timeout: 1});

EXPECT_EQ(before, strip_status(rs.status()));

session2.runSql("UNLOCK TABLES");

//@ Runtime problems
//--------------------------------
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
session2.runSql("STOP SLAVE");
session2.runSql("START SLAVE");
session3.runSql("STOP SLAVE");
session3.runSql("START SLAVE");

//@ promoted has broken/stopped replication (should fail)
session2.runSql("STOP SLAVE");

rs.setPrimaryInstance(__sandbox2);

//@ a secondary has broken replication (should fail)
rs.setPrimaryInstance(__sandbox3);

session2.runSql("START SLAVE");

//@ primary has unexpected replication channel (should fail)
session.runSql("CHANGE MASTER TO MASTER_HOST='localhost', MASTER_PORT=?, MASTER_USER='root', MASTER_PASSWORD='root'", [__mysql_sandbox_port2]);
session.runSql("START SLAVE");
rs.status();

rs.setPrimaryInstance(__sandbox2);

session.runSql("STOP SLAVE");
session.runSql("RESET SLAVE ALL");

//@ promoted is down (should fail)
testutil.stopSandbox(__mysql_sandbox_port3, {wait:1});

rs.setPrimaryInstance(__sandbox3);

//@ a secondary is down (should fail)
rs.setPrimaryInstance(__sandbox2);

testutil.startSandbox(__mysql_sandbox_port3);

//@ a secondary has errant GTIDs (should fail)
var session3 = mysql.getSession(__sandbox_uri3);
var before2 = strip_slave_status(session2.runSql("SHOW SLAVE STATUS").fetchAll());
var before3 = strip_slave_status(session3.runSql("SHOW SLAVE STATUS").fetchAll());

inject_errant_gtid(session3);

rs.setPrimaryInstance(__sandbox2);

EXPECT_EQ([], session.runSql("SHOW SLAVE STATUS").fetchAll());
EXPECT_EQ(before2, strip_slave_status(session2.runSql("SHOW SLAVE STATUS").fetchAll()));
EXPECT_EQ(before3, strip_slave_status(session3.runSql("SHOW SLAVE STATUS").fetchAll()));

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

//@ Cleanup
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
testutil.destroySandbox(__mysql_sandbox_port3);
