//@<> Setup
testutil.deployRawSandbox(__mysql_sandbox_port1, "root");
testutil.deploySandbox(__mysql_sandbox_port2, "root", {log_slave_updates:0});

//@<> configure nothing (should fail)
EXPECT_THROWS(function(){dba.configureReplicaSetInstance()},
    "Dba.configureReplicaSetInstance: An open session is required to perform this operation.");

//@<> configure bad URI (should fail)
EXPECT_THROWS(function(){dba.configureReplicaSetInstance(__sandbox_uri3)},
    "Dba.configureReplicaSetInstance: Can't connect to MySQL server on 'localhost'");

//@<> configure raw 5.7 server (should fail) {VER(<8.0.0)}
EXPECT_THROWS(function(){dba.configureReplicaSetInstance(__sandbox_uri1)},
    "Dba.configureReplicaSetInstance: Unsupported server version: This AdminAPI operation requires MySQL version 8.0 or newer, but target is 5.7.");

//@<> configure 5.7 server (should fail) {VER(<8.0.0)}
EXPECT_THROWS(function(){dba.configureReplicaSetInstance(__sandbox_uri2)},
    "Dba.configureReplicaSetInstance: Unsupported server version: This AdminAPI operation requires MySQL version 8.0 or newer, but target is 5.7.");

//@<> configuring applierWorkerThreads in versions lower that 8.0.23 (should fail) {VER(>=8.0.0) && VER(<8.0.23)}
EXPECT_THROWS(function(){dba.configureReplicaSetInstance(__sandbox_uri2, {applierWorkerThreads: 5});}, "Option 'applierWorkerThreads' not supported on target server version: '" + __version + "'");

//@<> configure default session {VER(>8.0.0)}
shell.connect(__sandbox_uri1);
EXPECT_NO_THROWS(function(){dba.configureReplicaSetInstance()});
EXPECT_THROWS(function(){dba.configureReplicaSetInstance("")}, "Dba.configureReplicaSetInstance: Argument #1: Invalid URI: empty.");
EXPECT_NO_THROWS(function(){dba.configureReplicaSetInstance(null)});
EXPECT_THROWS(function(){dba.configureReplicaSetInstance({})}, "Dba.configureReplicaSetInstance: Argument #1: Invalid connection options, no options provided.");

//@ configure and restart:0 {VER(>8.0.0)}
// Covers Bug #30510625 DBA.CONFIGURE_REPLICA_SET_INSTANCE: RESTART = TRUE IGNORED, NO ERROR GIVEN

shell.connect(__sandbox_uri2);

dba.configureReplicaSetInstance(__sandbox_uri2, {restart:0});

//@ configure and restart:1 {VER(>8.0.0)}
// Covers Bug #30510625 DBA.CONFIGURE_REPLICA_SET_INSTANCE: RESTART = TRUE IGNORED, NO ERROR GIVEN
// Reproducible if target is a sandbox only
dba.configureReplicaSetInstance(__sandbox_uri2, {restart:1});

// wait for the restarted server to come back
testutil.waitSandboxAlive(__mysql_sandbox_port2);

// check again to ensure configs were applied
dba.configureReplicaSetInstance(__sandbox_uri2);

//@<> Verify that the default value for applierWorkerThreads was set (4) {VER(>=8.0.23)}
EXPECT_EQ(4, get_sysvar(__mysql_sandbox_port2, "slave_parallel_workers"));

//@<> Change the value of applierWorkerThreads {VER(>=8.0.23)}
dba.configureReplicaSetInstance(__sandbox_uri2, {applierWorkerThreads: 10})
EXPECT_EQ(10, get_sysvar(__mysql_sandbox_port2, "slave_parallel_workers"));

// Verify that configureInstance() enables parallel-appliers on a replicaSet member that doesn't have them enabled (upgrade scenario)

//@<> Create a ReplicaSet {VER(>=8.0.23)}
shell.connect(__sandbox_uri2);
dba.createReplicaSet("test");

//@<> Manually disable some parallel-applier settings {VER(>=8.0.23)}
session.runSql("SET global slave_preserve_commit_order=OFF");
session.runSql("SET global slave_parallel_workers=0");

//@ Verify that configureInstance() detects and fixes the wrong settings {VER(>=8.0.23)}
dba.configureReplicaSetInstance();
testutil.waitSandboxAlive(__mysql_sandbox_port2);

//@<> Verify that the default value for applierWorkerThreads was set and the wrong config fixed {VER(>=8.0.23)}
EXPECT_EQ(4, get_sysvar(__mysql_sandbox_port2, "slave_parallel_workers"));
EXPECT_EQ(1, get_sysvar(__mysql_sandbox_port2, "slave_preserve_commit_order"));


//@ configure and check admin user {VER(>8.0.0)}
shell.connect(__sandbox_uri1);
session.runSql("DROP USER root@'%'");
dba.configureReplicaSetInstance(__sandbox_uri1);

//@ configure and check admin user interactive {VER(>8.0.0)}
shell.options.useWizards=1;
testutil.expectPrompt("Please select an option [1]: ", "1");
testutil.expectPrompt("Account Host", "%");
testutil.expectPrompt("Do you want to perform the required configuration changes?", "y");
testutil.expectPrompt("Do you want to restart the instance after configuring it?", "n");

EXPECT_EQ("root@localhost", session.runSql("SELECT group_concat(concat(user,'@',host)) FROM mysql.user WHERE user='root'").fetchOne()[0]);

dba.configureReplicaSetInstance(__sandbox_uri1);

EXPECT_EQ("root@%,root@localhost", session.runSql("SELECT group_concat(concat(user,'@',host)) FROM mysql.user WHERE user='root' ORDER BY host").fetchOne()[0]);

//@<> Cleanup
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
