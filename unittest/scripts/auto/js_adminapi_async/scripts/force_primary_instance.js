//@ {VER(>=8.0.11)}

// Tests forcePrimaryInstance() specifically
// Only tests corner cases and negative cases since the positive ones will
// be tested elsewhere.

//@ INCLUDE async_utils.inc

//@<> Setup

// Set report_host to a valid value, in case hostname is bogus
testutil.deploySandbox(__mysql_sandbox_port1, "root", {"report_host": hostname_ip, server_uuid:"5ef81566-9395-11e9-87e9-111111111111"});
testutil.deploySandbox(__mysql_sandbox_port2, "root", {"report_host": hostname_ip, server_uuid:"5ef81566-9395-11e9-87e9-222222222222"});
testutil.deploySandbox(__mysql_sandbox_port3, "root", {"report_host": hostname_ip, server_uuid:"5ef81566-9395-11e9-87e9-333333333333"});

shell.connect(__sandbox_uri1);
var rs = dba.createReplicaSet("myrs", {gtidSetIsComplete:1});

var session2 = mysql.getSession(__sandbox_uri2);
var session3 = mysql.getSession(__hostname_uri3);

var userhost=session3.runSql("select user()").fetchOne()[0];

// faster connection timeouts
testutil.dbugSet("+d,contimeout");

// Negative tests based on environment and params
//--------------------------------

//@ bad parameters (should fail)
rs.forcePrimaryInstance(1, null);
rs.forcePrimaryInstance(__sandbox_uri1, 1);
rs.forcePrimaryInstance(__sandbox_uri1, {}, {});
rs.forcePrimaryInstance(__sandbox_uri1, {badOption:123});
rs.forcePrimaryInstance([__sandbox_uri1]);
rs.forcePrimaryInstance(__sandbox1, 1);
rs.forcePrimaryInstance(null, 1);

//@ disconnected rs object (should fail)
rs.disconnect();
rs.forcePrimaryInstance(__sandbox2);
rs = dba.getReplicaSet();

//@ auto-promote while there's only a primary (should fail)
rs.forcePrimaryInstance();

//@<> add 2nd member
rs.addInstance(__address2);
testutil.waitMemberTransactions(__mysql_sandbox_port2, __mysql_sandbox_port1);

//@ primary still available (should fail)
rs.forcePrimaryInstance(__sandbox2);

//@ primary promoted (should fail)
rs.forcePrimaryInstance(__sandbox1);

//@# stop primary and failover with bad handle (should fail)
testutil.stopSandbox(__mysql_sandbox_port1, {wait:1});

// this should fail
rs.forcePrimaryInstance(__sandbox2);

shell.connect(__sandbox_uri2);
rs=dba.getReplicaSet();

//@ promoted isn't member (should fail)
rs.forcePrimaryInstance(__sandbox3);

//@ promoted doesn't exist (should fail)
rs.forcePrimaryInstance("localhost:"+__mysql_sandbox_port3+1);

//@<> add 3rd instance
testutil.startSandbox(__mysql_sandbox_port1);
shell.connect(__sandbox_uri1);
rs = dba.getReplicaSet();
rs.addInstance(__sandbox_uri3);
// addInstance will sync sb3, but sb2 will be behind
testutil.waitMemberTransactions(__mysql_sandbox_port2, __mysql_sandbox_port1);
strip_status(rs.status());
cluster_id = session.runSql("SELECT cluster_id FROM mysql_innodb_cluster_metadata.clusters").fetchOne()[0];

// stop primary
testutil.stopSandbox(__mysql_sandbox_port1, {wait:1});
shell.connect(__sandbox_uri3);
rs = dba.getReplicaSet();
strip_status(rs.status());

// Positive tests for specific issues
//--------------------------------

//@ check state of instances after switch
rs.forcePrimaryInstance(__sandbox2);

// Wait for all transaction to be applied in sandbox 3 before checking its metadata.
// NOTE: Might need a new testutils funtion to also consider the GTID_RETRIEVED.
testutil.waitMemberTransactions(__mysql_sandbox_port3, __mysql_sandbox_port2);

// SRO
EXPECT_EQ(0, get_sysvar(session2, "super_read_only"));
EXPECT_EQ(1, get_sysvar(session3, "super_read_only"));
// metadata exists and instance is there (order result to get a deterministic output)
shell.dumpRows(session.runSql("SELECT * FROM mysql_innodb_cluster_metadata.v2_ar_members ORDER BY instance_id"), "tabbed");
// repl should be running
strip_status(rs.status());
// check that the promoted primary has no repl configured
shell.dumpRows(session2.runSql("SELECT * FROM mysql.slave_master_info"), "tabbed");

//@<OUT> check state of instances after switch - view change records
shell.dumpRows(session.runSql("SELECT * FROM mysql_innodb_cluster_metadata.async_cluster_views"), "tabbed");

//@ promoted is not super-read-only
testutil.startSandbox(__mysql_sandbox_port1);
shell.connect(__sandbox_uri1);
rs = rebuild_rs();
testutil.stopSandbox(__mysql_sandbox_port1, {wait:1});

shell.connect(__sandbox_uri2);
rs=dba.getReplicaSet();

session2.runSql("SET GLOBAL super_read_only=0");
rs.forcePrimaryInstance(__sandbox2);

//@ promote another one right after (should fail)
rs.forcePrimaryInstance(__sandbox3);

//@ normal promotion
rs.setPrimaryInstance(__sandbox3);

//@ automatically pick the promoted primary
testutil.startSandbox(__mysql_sandbox_port1);
shell.connect(__sandbox_uri1);
rs = rebuild_rs();
testutil.stopSandbox(__mysql_sandbox_port1, {wait:1});

shell.connect(__sandbox_uri2);
rs=dba.getReplicaSet();

strip_status(rs.status());
rs.forcePrimaryInstance();
// both instances should be in sync, so it should pick the 1st (sb2)
strip_status(rs.status());

//@<> automatically pick the promoted primary, while one of them is behind (prepare)
testutil.startSandbox(__mysql_sandbox_port1);
shell.connect(__sandbox_uri1);
rs = rebuild_rs();
// Make instance 2 behind, ensure it does not receive one trx from primary.
// NOTE: Delaying replication on instance 2 might not avoid it from being
//       elected because all the retrieved GTIDs must be applied by the failover
//       algorithm to avoid losing trxs. Thus, the only way to make it behind
//       is to ensure it failed to receive all trxs from the primary.
session2.runSql("STOP SLAVE IO_THREAD");
session.runSql("create schema testdb");
testutil.stopSandbox(__mysql_sandbox_port1, {wait:1});
session2.runSql("START SLAVE IO_THREAD");

shell.connect(__sandbox_uri2);
rs=dba.getReplicaSet();

//@ automatically pick the promoted primary, while one of them is behind
strip_status(rs.status());
rs.forcePrimaryInstance();
// sb3 should be picked
strip_status(rs.status());

// Options
//--------------------------------
//@ dryRun prepare
testutil.startSandbox(__mysql_sandbox_port1);
shell.connect(__sandbox_uri1);
rs = rebuild_rs();
testutil.stopSandbox(__mysql_sandbox_port1, {wait:true});
shell.connect(__sandbox_uri2);
rs=dba.getReplicaSet();

// wait until sb2 notices the replication error
testutil.waitForReplConnectionError(__mysql_sandbox_port2, "");

//@ dryRun
var before = rs.status();
println("PRIMARY", before.replicaSet.primary);

begin_dba_log_sql();

rs.forcePrimaryInstance(__sandbox2, {dryRun:true});

var logs = end_dba_log_sql();
EXPECT_NO_SQL(__sandbox1, logs);
EXPECT_NO_SQL(__sandbox2, logs);
EXPECT_NO_SQL(__sandbox3, logs);

EXPECT_EQ(before.replicaSet.primary, rs.status().replicaSet.primary);

rs.forcePrimaryInstance(__sandbox2, {dryRun:1, timeout:1});

EXPECT_EQ(before.replicaSet.primary, rs.status().replicaSet.primary);

//@ timeout (should fail)
testutil.startSandbox(__mysql_sandbox_port1);
shell.connect(__sandbox_uri1);

session2.runSql("FLUSH TABLES WITH READ LOCK");
// sandbox2 slave will be stuck because of the lock, even if its not getting
// anything from the primary
session.runSql("CREATE SCHEMA testdb");
// wait til it gets stuck
var i = 10;
while (i > 0) {
    var state = session2.runSql("SELECT PROCESSLIST_STATE from performance_schema.threads where name='thread/sql/slave_sql'").fetchOne()[0];
    if (state == "Waiting for global read lock")
        break;
    os.sleep(0.5);
    i--;
}

// take primary down
testutil.stopSandbox(__mysql_sandbox_port1, {wait:1});

shell.connect(__sandbox_uri2);
rs = dba.getReplicaSet();
// sandbox3 slave will be stuck trying to apply this because of the lock
strip_status(rs.status());
// fail now
rs.forcePrimaryInstance(__sandbox2, {timeout:1});

//@ ensure switch over rolls back after the timeout
rs = dba.getReplicaSet();
strip_status(rs.status());

//@ try to switch to a different one (should work this time)
// the promoted member is not the frozen one, so it's OK
rs = dba.getReplicaSet();
rs.forcePrimaryInstance(__sandbox3, {timeout: 1});

strip_status(rs.status());

session2.runSql("UNLOCK TABLES");

//@ the frozen instance should finish applying its relay log
testutil.waitMemberTransactions(__mysql_sandbox_port2, __mysql_sandbox_port3);

strip_status(rs.status());

//@ invalidateErrorInstances - preparation
testutil.startSandbox(__mysql_sandbox_port1);
shell.connect(__sandbox_uri1);
rs = rebuild_rs();
testutil.stopSandbox(__mysql_sandbox_port1, {wait:1});

testutil.stopSandbox(__mysql_sandbox_port2, {wait:1});
shell.connect(__sandbox_uri3);
rs=dba.getReplicaSet();

//@ a different secondary is down (should fail)
rs.forcePrimaryInstance(__sandbox3);

//@ a different secondary is down with invalidateErrorInstances
rs.forcePrimaryInstance(__sandbox3, {invalidateErrorInstances: 1});

// sb1 and sb2 should now be invalidated
strip_status(rs.status());

testutil.startSandbox(__mysql_sandbox_port2);

//@<> Cleanup
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
testutil.destroySandbox(__mysql_sandbox_port3);
