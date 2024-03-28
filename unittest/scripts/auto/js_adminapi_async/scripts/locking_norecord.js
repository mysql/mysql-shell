//@ {VER(>=8.0.11)}

// Tests locking of replica set operations to manage concurrent executions.

function rset_lock_check(session, func, test_shared, error_instance_name) {

    WIPE_SHELL_LOG();

    testutil.getExclusiveLock(session, "AdminAPI_replicaset", "AdminAPI_lock");
    EXPECT_THROWS(function() {
        func();
    }, `Failed to acquire ReplicaSet lock through primary member '${error_instance_name}'`);
    EXPECT_OUTPUT_CONTAINS(`The operation cannot be executed because it failed to acquire the ReplicaSet lock through primary member '${error_instance_name}'. Another operation requiring access to the member is still in progress, please wait for it to finish and try again.`);
    testutil.releaseLocks(session, "AdminAPI_replicaset");

    if (test_shared) {
        testutil.getSharedLock(session, "AdminAPI_replicaset", "AdminAPI_lock");
        EXPECT_THROWS(function() {
            func();
        }, `Failed to acquire ReplicaSet lock through primary member '${error_instance_name}'`);
        EXPECT_OUTPUT_CONTAINS(`The operation cannot be executed because it failed to acquire the ReplicaSet lock through primary member '${error_instance_name}'. Another operation requiring access to the member is still in progress, please wait for it to finish and try again.`);
        testutil.releaseLocks(session, "AdminAPI_replicaset");
    }
}

function instance_lock_check(session_lock, session_port, func, test_shared = true) {

    WIPE_SHELL_LOG();

    testutil.getExclusiveLock(session_lock, "AdminAPI_instance", "AdminAPI_lock");
    EXPECT_THROWS(function() {
        func();
    }, `Failed to acquire lock on instance '${localhost}:${session_port}'`);
    EXPECT_OUTPUT_CONTAINS(`The operation cannot be executed because it failed to acquire the lock on instance '${localhost}:${session_port}'. Another operation requiring access to the instance is still in progress, please wait for it to finish and try again.`);
    testutil.releaseLocks(session_lock, "AdminAPI_instance");

    if (test_shared) {
        testutil.getSharedLock(session_lock, "AdminAPI_instance", "AdminAPI_lock");
        EXPECT_THROWS(function() {
            func();
        }, `Failed to acquire lock on instance '${localhost}:${session_port}'`);
        EXPECT_OUTPUT_CONTAINS(`The operation cannot be executed because it failed to acquire the lock on instance '${localhost}:${session_port}'. Another operation requiring access to the instance is still in progress, please wait for it to finish and try again.`);
        testutil.releaseLocks(session_lock, "AdminAPI_instance");
    }
}

//@<> INCLUDE async_utils.inc

//@<> Initialization.
var lock_rset = "AdminAPI_replicaset";
var lock_instance = "AdminAPI_instance";
var lock_name = "AdminAPI_lock";

testutil.deploySandbox(__mysql_sandbox_port1, "root", {"report_host": localhost});
testutil.deploySandbox(__mysql_sandbox_port2, "root", {"report_host": localhost});
testutil.deploySandbox(__mysql_sandbox_port3, "root", {"report_host": localhost});
var session1 = mysql.getSession(__sandbox_uri1);
var session2 = mysql.getSession(__sandbox_uri2);

shell.connect(__sandbox_uri1);
var rs1 = dba.createReplicaSet("myrs", {gtidSetIsComplete:true});
rs1.addInstance(__sandbox_uri2, {recoveryMethod: "clone"});

shell.connect(__sandbox_uri2);
var rs2 = dba.getReplicaSet();

shell.connect(__sandbox_uri1);

// *************************
// ReplicaSet locks (exclusive)

//@<> exclusive lock on replicaset.setOption()
rset_lock_check(session1, function() {
    rs1.setOption("replicationAllowedHost", localhost);
}, true, `${localhost}:${__mysql_sandbox_port1}`);
rset_lock_check(session1, function() {
    rs2.setOption("replicationAllowedHost", localhost);
}, true, `${localhost}:${__mysql_sandbox_port1}`);

//@<> exclusive lock on replicaset.rescan()
rset_lock_check(session1, function() {
    rs1.rescan();
}, true, `${localhost}:${__mysql_sandbox_port1}`);
rset_lock_check(session1, function() {
    rs2.rescan();
}, true, `${localhost}:${__mysql_sandbox_port1}`);

//@<> exclusive lock on replicaset.setPrimaryInstance()
rset_lock_check(session1, function() {
    rs1.setPrimaryInstance(__sandbox_uri2);
}, true, `${localhost}:${__mysql_sandbox_port1}`);
rset_lock_check(session1, function() {
    rs2.setPrimaryInstance(__sandbox_uri2);
}, true, `${localhost}:${__mysql_sandbox_port1}`);

//@<> exclusive lock on replicaset.forcePrimaryInstance()
rset_lock_check(session1, function() {
    rs1.forcePrimaryInstance(__sandbox_uri2);
}, true, `${localhost}:${__mysql_sandbox_port1}`);
rset_lock_check(session1, function() {
    rs2.forcePrimaryInstance(__sandbox_uri2);
}, true, `${localhost}:${__mysql_sandbox_port1}`);

//@<> exclusive lock on replicaset.addInstance()
rset_lock_check(session1, function() {
    rs1.addInstance(__sandbox_uri2);
}, true, `${localhost}:${__mysql_sandbox_port1}`);
rset_lock_check(session1, function() {
    rs2.addInstance(__sandbox_uri2);
}, true, `${localhost}:${__mysql_sandbox_port1}`);

//@<> exclusive lock on replicaset.removeInstance()
rset_lock_check(session1, function() {
    rs1.removeInstance(__sandbox_uri2);
}, true, `${localhost}:${__mysql_sandbox_port1}`);
rset_lock_check(session1, function() {
    rs2.removeInstance(__sandbox_uri2);
}, true, `${localhost}:${__mysql_sandbox_port1}`);

//@<> exclusive lock on replicaset.dissolve()
rset_lock_check(session1, function() {
    rs1.dissolve();
}, true, `${localhost}:${__mysql_sandbox_port1}`);
rset_lock_check(session1, function() {
    rs2.dissolve();
}, true, `${localhost}:${__mysql_sandbox_port1}`);

// **********************
// ReplicaSet locks (shared)

//@<> shared lock on replicaset.rejoinInstance()

rs1.addInstance(__sandbox_uri3);

var session3 = mysql.getSession(__sandbox_uri3);
session3.runSql("STOP " + get_replica_keyword());
session3.runSql("RESET " + get_replica_keyword() + " ALL");
session3.close();

rset_lock_check(session1, function() {
    rs1.rejoinInstance(__sandbox_uri3);
}, false, `${localhost}:${__mysql_sandbox_port1}`);
rset_lock_check(session1, function() {
    rs2.rejoinInstance(__sandbox_uri3);
}, false, `${localhost}:${__mysql_sandbox_port1}`);

testutil.getSharedLock(session1, lock_rset, lock_name);
EXPECT_NO_THROWS(function() {
    rs1.rejoinInstance(__sandbox_uri3);
});
testutil.releaseLocks(session1, lock_rset);

rs1.removeInstance(__sandbox_uri3);

//@<> shared lock on replicaset.setupAdminAccount()
rset_lock_check(session1, function() {
    rs1.setupAdminAccount("%", {password: "foo"});
}, false, `${localhost}:${__mysql_sandbox_port1}`);
rset_lock_check(session1, function() {
    rs2.setupAdminAccount("%", {password: "foo"});
}, false, `${localhost}:${__mysql_sandbox_port1}`);

testutil.getSharedLock(session1, lock_rset, lock_name);
EXPECT_NO_THROWS(function() {
    rs1.setupAdminAccount("%", {password: "foo"});
});
testutil.releaseLocks(session1, lock_rset);

//@<> shared lock on replicaset.setupRouterAccount()
rset_lock_check(session1, function() {
    rs1.setupRouterAccount("router@'%'", {password: "bar"});
}, false, `${localhost}:${__mysql_sandbox_port1}`);
rset_lock_check(session1, function() {
    rs2.setupRouterAccount("router@'%'", {password: "bar"});
}, false, `${localhost}:${__mysql_sandbox_port1}`);

testutil.getSharedLock(session1, lock_rset, lock_name);
EXPECT_NO_THROWS(function() {
    rs1.setupRouterAccount("router@'%'", {password: "bar"});
});
testutil.releaseLocks(session1, lock_rset);

//@<> shared lock on replicaset.execute()
rset_lock_check(session1, function() {
    rs1.execute("select 1;", "p");
}, false, `${localhost}:${__mysql_sandbox_port1}`);
rset_lock_check(session1, function() {
    rs2.execute("select 2;", "p");
}, false, `${localhost}:${__mysql_sandbox_port1}`);

testutil.getSharedLock(session1, lock_rset, lock_name);
EXPECT_NO_THROWS(function() {
    rs1.execute("select 3;", "p");
});
testutil.releaseLocks(session1, lock_rset);

// **************
// Instance locks

var session3 = mysql.getSession(__sandbox_uri3);

//@<> exclusive instance lock on replicaset.addInstance()
instance_lock_check(session3, __mysql_sandbox_port3, function() {
    rs1.addInstance(__sandbox_uri3);
});

//@<> exclusive instance lock on replicaset.removeInstance()
instance_lock_check(session1, __mysql_sandbox_port1, function() {
    rs1.removeInstance(__sandbox_uri1);
});

//@<> exclusive instance lock on replicaset.rejoinInstance()
instance_lock_check(session1, __mysql_sandbox_port1, function() {
    rs1.rejoinInstance(__sandbox_uri1);
});


//@<> exclusive instance lock on replicaset.setInstanceOption()
instance_lock_check(session1, __mysql_sandbox_port1, function() {
    rs1.setInstanceOption(__sandbox_uri1, "replicationRetryCount", 10);
});

//@<> exclusive instance lock on dba.createReplicaSet()
shell.connect(__sandbox_uri3);

instance_lock_check(session3, __mysql_sandbox_port3, function() {
    dba.createReplicaSet("rset");
});

session3.close();

//@<> Cleanup
session.close();
session1.close();
session2.close();

testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
testutil.destroySandbox(__mysql_sandbox_port3);
