// Checks that the invariant "Any server part of a cluster must be SRO unless it's PRIMARY and ONLINE" always holds, specially after a server restart or while GR is stopped.

// Covers Bug #30545872	CONFLICTING TRANSACTION SETS FOLLOWING COMPLETE OUTAGE OF INNODB CLUSTER

//@<> Setup
testutil.deploySandbox(__mysql_sandbox_port1, "root");
testutil.deploySandbox(__mysql_sandbox_port2, "root");
testutil.snapshotSandboxConf(__mysql_sandbox_port1);
testutil.snapshotSandboxConf(__mysql_sandbox_port2);

//@<> create basic cluster

shell.connect(__sandbox_uri1);
c = dba.createCluster("cluster", {gtidSetIsComplete:1});

c.addInstance(__sandbox_uri2);

//@<> persist settings in 5.7 {VER(<8.0.0)}
dba.configureLocalInstance(__sandbox_uri1, {mycnfPath: testutil.getSandboxConfPath(__mysql_sandbox_port1)});
dba.configureLocalInstance(__sandbox_uri2, {mycnfPath: testutil.getSandboxConfPath(__mysql_sandbox_port2)});

//@<> check configs while servers are online
session1 = mysql.getSession(__sandbox_uri1);
session2 = mysql.getSession(__sandbox_uri2);

EXPECT_EQ(0, session1.runSql("SELECT @@global.super_read_only").fetchOne()[0], "sro at primary");
EXPECT_EQ(1, session2.runSql("SELECT @@global.super_read_only").fetchOne()[0], "sro at secondary");

//@<> check configs after a restart

// the following sequence is required to make sure the cluster is really dead
testutil.stopSandbox(__mysql_sandbox_port2);
testutil.waitMemberState(__mysql_sandbox_port2, "(MISSING)");
testutil.restartSandbox(__mysql_sandbox_port1);
testutil.startSandbox(__mysql_sandbox_port2);

session1 = mysql.getSession(__sandbox_uri1);
session2 = mysql.getSession(__sandbox_uri2);

// cluster is down, so everyone has to be sro, including the former primary

// right after
EXPECT_EQ(1, session1.runSql("SELECT @@global.super_read_only").fetchOne()[0], "sro at old primary");
EXPECT_EQ(1, session2.runSql("SELECT @@global.super_read_only").fetchOne()[0], "sro at old secondary");

// wait for start_on_boot to time out
// start_on_boot will set sro=1 when it starts but set it back to the original value if it fails after timeout
testutil.waitForDelayedGRStart(__mysql_sandbox_port1, "root");
testutil.waitForDelayedGRStart(__mysql_sandbox_port2, "root");

// after start_on_boot timeout
EXPECT_EQ(1, session1.runSql("SELECT @@global.super_read_only").fetchOne()[0], "sro at old primary");
EXPECT_EQ(1, session2.runSql("SELECT @@global.super_read_only").fetchOne()[0], "sro at old secondary");

//@<> check configs after rebootCluster (but pick the old secondary as primary)

shell.connect(__sandbox_uri2);
c = dba.rebootClusterFromCompleteOutage("cluster");

c.status();
EXPECT_EQ(0, session2.runSql("SELECT @@global.super_read_only").fetchOne()[0], "sro at new primary");
EXPECT_EQ(1, session1.runSql("SELECT @@global.super_read_only").fetchOne()[0], "sro at old primary");

//@<> rejoin the old primary as a secondary
c.rejoinInstance(__sandbox_uri1);
c.status();
EXPECT_EQ(0, session2.runSql("SELECT @@global.super_read_only").fetchOne()[0], "sro at new primary");
EXPECT_EQ(1, session1.runSql("SELECT @@global.super_read_only").fetchOne()[0], "sro at new secondary");

//@<> check while GR stopped
session2.runSql("stop group_replication");
EXPECT_EQ(1, session2.runSql("SELECT @@global.super_read_only").fetchOne()[0], "sro at new primary");

session2.runSql("start group_replication");
EXPECT_EQ(1, session2.runSql("SELECT @@global.super_read_only").fetchOne()[0], "sro at new primary");

//@<> check after removeInstance()

// we need a new handle because the cluster object still thinks sb2 is the primary and it doesn't auto-update to the new primary as of 8.0.19
shell.connect(__sandbox_uri1);
c = dba.getCluster();

c.removeInstance(__sandbox_uri2);

EXPECT_EQ(0, session1.runSql("SELECT @@global.super_read_only").fetchOne()[0], "sro at primary");
EXPECT_EQ(1, session2.runSql("SELECT @@global.super_read_only").fetchOne()[0], "sro at removed instance");

//@<> prepare for adopt test
c.addInstance(__sandbox_uri2);

session.runSql("DROP SCHEMA mysql_innodb_cluster_metadata");

//@<> clear SRO {VER(>=8.0.0)}
session1.runSql("SET PERSIST super_read_only=0");
session2.runSql("SET PERSIST super_read_only=0");

//@<> clear SRO {VER(<8.0.0)}
testutil.changeSandboxConf(__mysql_sandbox_port1, "super_read_only", "0");
testutil.changeSandboxConf(__mysql_sandbox_port2, "super_read_only", "0");

session1.runSql("SET GLOBAL super_read_only=0");
session2.runSql("SET GLOBAL super_read_only=0");

//@<> adopt and check if everything is SRO again
shell.dumpRows(session.runSql("SELECT * FROM performance_schema.replication_group_members"));

c = dba.createCluster("newcluster", {adoptFromGR:1});

//@<> persist settings in 5.7 after adopt {VER(<8.0.0)}
dba.configureLocalInstance(__sandbox_uri1, {mycnfPath: testutil.getSandboxConfPath(__mysql_sandbox_port1)});
dba.configureLocalInstance(__sandbox_uri2, {mycnfPath: testutil.getSandboxConfPath(__mysql_sandbox_port2)});

//@<> restart and check
testutil.restartSandbox(__mysql_sandbox_port1);
testutil.restartSandbox(__mysql_sandbox_port2);

session1 = mysql.getSession(__sandbox_uri1);
session2 = mysql.getSession(__sandbox_uri2);

session1.runSql("stop group_replication");
session2.runSql("stop group_replication");

EXPECT_EQ(1, session1.runSql("SELECT @@global.super_read_only").fetchOne()[0], "sro at primary");
EXPECT_EQ(1, session2.runSql("SELECT @@global.super_read_only").fetchOne()[0], "sro at secondary");

//@<> prepare for adopt test (BUG#31238233)
shell.connect(__sandbox_uri2);
c = dba.rebootClusterFromCompleteOutage("newcluster");

shell.dumpRows(session.runSql("SELECT * FROM performance_schema.replication_group_members"));

session.runSql("SET GLOBAL super_read_only=0");
session.runSql("DROP SCHEMA mysql_innodb_cluster_metadata");

// Wait for the changes to be applied on the secondary
testutil.waitMemberTransactions(__mysql_sandbox_port1);

//@<> adopt using a secondary and verify that SRO is kept (BUG#31238233)
shell.connect(__sandbox_uri1);
c = dba.createCluster("newcluster", {adoptFromGR:1});

EXPECT_EQ(0, session2.runSql("SELECT @@global.super_read_only").fetchOne()[0], "sro disabled at primary");
EXPECT_EQ(1, session1.runSql("SELECT @@global.super_read_only").fetchOne()[0], "sro enabled at secondary");

//@<> Cleanup
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
