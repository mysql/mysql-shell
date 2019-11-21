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


//@<> Cleanup
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
