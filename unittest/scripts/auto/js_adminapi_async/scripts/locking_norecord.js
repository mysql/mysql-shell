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

//@<> INCLUDE async_utils.inc

//@<> Initialization.
var uuid1 = "5ef81566-9395-11e9-87e9-111111111111";
var uuid2 = "5ef81566-9395-11e9-87e9-222222222222";
var uuid3 = "5ef81566-9395-11e9-87e9-333333333333";
testutil.deploySandbox(__mysql_sandbox_port1, "root", {"report_host": localhost, server_uuid: uuid1, server_id: 11});
testutil.deploySandbox(__mysql_sandbox_port2, "root", {"report_host": localhost, server_uuid: uuid2, server_id: 22});
testutil.deploySandbox(__mysql_sandbox_port3, "root", {"report_host": localhost, server_uuid: uuid3, server_id: 33});
var session1 = mysql.getSession(__sandbox_uri1);
var session2 = mysql.getSession(__sandbox_uri2);
var session3 = mysql.getSession(__sandbox_uri3);

var lock_rset = "AdminAPI_replicaset";
var lock_instance = "AdminAPI_instance";
var lock_name = "AdminAPI_lock";

//@<> Installing locking UDFs should succeed even if instance is read-only (no warning).
session1.runSql("SET GLOBAL super_read_only=1");
shell.connect(__sandbox_uri1);
dba.createReplicaSet("myrs", {gtidSetIsComplete:true});
EXPECT_OUTPUT_NOT_CONTAINS("WARNING: Concurrent execution");
session1.runSql("SET GLOBAL read_only=0");
session1.runSql("DROP SCHEMA IF EXISTS mysql_innodb_cluster_metadata");

//@<> Create ReplicaSet another operation in progress (fail)
// Create ReplicaSet when another operation holds an exclusive lock (fail).
// Simulate exclusive lock hold by another operation on instance 1.
testutil.getExclusiveLock(session1, lock_instance, lock_name);

shell.connect(__sandbox_uri1);

// WL#13540: TSFR1_1
EXPECT_THROWS(function() {
    dba.createReplicaSet("myrs", {gtidSetIsComplete:true});
}, `Failed to acquire lock on instance '${localhost}:${__mysql_sandbox_port1}'`);
EXPECT_OUTPUT_CONTAINS(`The operation cannot be executed because it failed to acquire the lock on instance '${localhost}:${__mysql_sandbox_port1}'. Another operation requiring access to the instance is still in progress, please wait for it to finish and try again.`);

// Release locks on instance 1.
testutil.releaseLocks(session1, lock_instance);

//@<> Create ReplicaSet when another operation holds a shared lock (fail).
// Simulate shared lock hold by another operation on instance 1.
testutil.getSharedLock(session1, lock_instance, lock_name);

// WL#13540: TSFR1_1
EXPECT_THROWS(function() {
    dba.createReplicaSet("myrs", {gtidSetIsComplete:true});
}, `Failed to acquire lock on instance '${localhost}:${__mysql_sandbox_port1}'`);
EXPECT_OUTPUT_CONTAINS(`The operation cannot be executed because it failed to acquire the lock on instance '${localhost}:${__mysql_sandbox_port1}'. Another operation requiring access to the instance is still in progress, please wait for it to finish and try again.`);

// Release locks on instance 1.
testutil.releaseLocks(session1, lock_instance);

//@<> Create Async replica set (succeed).
var rs = dba.createReplicaSet("myrs", {gtidSetIsComplete:true});

//@<> Configure instance when another operation in progress on the target (fail)
// Configure instance when another operation holds an exclusive lock on the target instance (fail).
// Simulate exclusive lock hold by another operation on instance 2 (target).
testutil.getExclusiveLock(session2, lock_instance, lock_name);

// WL#13540: TSFR1_4
EXPECT_THROWS(function() {
    dba.configureReplicaSetInstance(__sandbox_uri2);
}, `Failed to acquire lock on instance '${localhost}:${__mysql_sandbox_port2}'`);
EXPECT_OUTPUT_CONTAINS(`The operation cannot be executed because it failed to acquire the lock on instance '${localhost}:${__mysql_sandbox_port2}'. Another operation requiring access to the instance is still in progress, please wait for it to finish and try again.`);

// Release locks on instance 2.
testutil.releaseLocks(session2, lock_instance);

//@<> Configure instance when another operation holds a shared lock on the target instance (fail).
// Simulate shared lock hold by another operation on instance 2 (target).
testutil.getSharedLock(session2, lock_instance, lock_name);

EXPECT_THROWS(function() {
    dba.configureReplicaSetInstance(__sandbox_uri2);
}, `Failed to acquire lock on instance '${localhost}:${__mysql_sandbox_port2}'`);
EXPECT_OUTPUT_CONTAINS(`The operation cannot be executed because it failed to acquire the lock on instance '${localhost}:${__mysql_sandbox_port2}'. Another operation requiring access to the instance is still in progress, please wait for it to finish and try again.`);

// Release locks on instance 2.
testutil.releaseLocks(session2, lock_instance);

//@<> Dissolve the replicaset (requires a replicaset-wide xlock)
rset_lock_check(session1, function() {
    rs.dissolve();
}, true, `${localhost}:${__mysql_sandbox_port1}`);

//@<> Rescan the replicaset (requires a replicaset-wide xlock)
rset_lock_check(session1, function() {
    rs.rescan();
}, true, `${localhost}:${__mysql_sandbox_port1}`);

//@<> Add instance when another operation holds an exclusive lock on the primary (fail).
// Simulate exclusive lock hold by another operation on instance 1 (primary).
testutil.getExclusiveLock(session1, lock_instance, lock_name);

// WL#13540: TSFR1_3, TSFR2_3
EXPECT_THROWS(function() {
    rs.addInstance(__sandbox_uri2);
}, `Failed to acquire lock on instance '${localhost}:${__mysql_sandbox_port1}'`);
EXPECT_OUTPUT_CONTAINS(`The operation cannot be executed because it failed to acquire the lock on instance '${localhost}:${__mysql_sandbox_port1}'. Another operation requiring access to the instance is still in progress, please wait for it to finish and try again.`);

// Release locks on instance 1.
testutil.releaseLocks(session1, lock_instance);

//@<> Add instance when another operation in progress on the target (fail)
// Add instance when another operation holds an exclusive lock on the target instance (fail).
// Simulate exclusive lock hold by another operation on instance 2 (target).
testutil.getExclusiveLock(session2, lock_instance, lock_name);

// WL#13540: TSFR3_1, TSFR2_3, TSFR3_1
EXPECT_THROWS(function() {
    rs.addInstance(__sandbox_uri2);
}, `Failed to acquire lock on instance '${localhost}:${__mysql_sandbox_port2}'`);
EXPECT_OUTPUT_CONTAINS(`The operation cannot be executed because it failed to acquire the lock on instance '${localhost}:${__mysql_sandbox_port2}'. Another operation requiring access to the instance is still in progress, please wait for it to finish and try again.`);

// Release locks on instance 2.
testutil.releaseLocks(session2, lock_instance);

//@<> Add instance when another operation holds a shared lock on the target instance (fail).
// Simulate shared lock hold by another operation on instance 2 (target).
testutil.getSharedLock(session2, lock_instance, lock_name);

EXPECT_THROWS(function() {
    rs.addInstance(__sandbox_uri2);
}, `Failed to acquire lock on instance '${localhost}:${__mysql_sandbox_port2}'`);
EXPECT_OUTPUT_CONTAINS(`The operation cannot be executed because it failed to acquire the lock on instance '${localhost}:${__mysql_sandbox_port2}'. Another operation requiring access to the instance is still in progress, please wait for it to finish and try again.`);

// Release locks on instance 2.
testutil.releaseLocks(session2, lock_instance);

//@<> Add instance when another operation holds an shared lock on the primary (succeed).
// Simulate shared lock hold by another operation on instance 1 (primary).
testutil.getSharedLock(session1, lock_instance, lock_name);
// WL#13540: TSFR3_2
rs.addInstance(__sandbox_uri2);
// Release locks on instance 1.
testutil.releaseLocks(session1, lock_instance);

//@<> Remove instance when another operation in progress on the target (fail)
// Remove instance when another operation holds an exclusive lock on the target instance (fail).
// Simulate exclusive lock hold by another operation on instance 2 (target).
testutil.getExclusiveLock(session2, lock_instance, lock_name);

// WL#13540: TSFR3_1, TSFR2_3, TSFR3_1
EXPECT_THROWS(function() {
    rs.removeInstance(__sandbox_uri2);
}, `Failed to acquire lock on instance '${localhost}:${__mysql_sandbox_port2}'`);
EXPECT_OUTPUT_CONTAINS(`The operation cannot be executed because it failed to acquire the lock on instance '${localhost}:${__mysql_sandbox_port2}'. Another operation requiring access to the instance is still in progress, please wait for it to finish and try again.`);

// Release locks on instance 2.
testutil.releaseLocks(session2, lock_instance);

//@<> Remove instance when another operation holds a shared lock on the target instance (fail).
// Simulate shared lock hold by another operation on instance 2 (target).
testutil.getSharedLock(session2, lock_instance, lock_name);

EXPECT_THROWS(function() {
    rs.removeInstance(__sandbox_uri2);
}, `Failed to acquire lock on instance '${localhost}:${__mysql_sandbox_port2}'`);
EXPECT_OUTPUT_CONTAINS(`The operation cannot be executed because it failed to acquire the lock on instance '${localhost}:${__mysql_sandbox_port2}'. Another operation requiring access to the instance is still in progress, please wait for it to finish and try again.`);

// Release locks on instance 2.
testutil.releaseLocks(session2, lock_instance);

//@<> Remove instance when another operation holds an exclusive lock on the primary (fail).
// Simulate exclusive lock hold by another operation on instance 1 (primary).
testutil.getExclusiveLock(session1, lock_instance, lock_name);

// WL#13540: TSFR1_3, TSFR2_3
EXPECT_THROWS(function() {
    rs.removeInstance(__sandbox_uri2);
}, `Failed to acquire lock on instance '${localhost}:${__mysql_sandbox_port1}'`);
EXPECT_OUTPUT_CONTAINS(`The operation cannot be executed because it failed to acquire the lock on instance '${localhost}:${__mysql_sandbox_port1}'. Another operation requiring access to the instance is still in progress, please wait for it to finish and try again.`);

// Release locks on instance 1.
testutil.releaseLocks(session1, lock_instance);

//@<> Remove instance when another operation holds an shared lock on the primary (succeed).
// Simulate shared lock hold by another operation on instance 1 (primary).
testutil.getSharedLock(session1, lock_instance, lock_name);
// WL#13540: TSFR3_2
rs.removeInstance(__sandbox_uri2);
// Release locks on instance 1.
testutil.releaseLocks(session1, lock_instance);

//@<> Add instance 2 back again.
rs.addInstance(__sandbox_uri2);

//@<> Stop replication on instance 2 to rejoin it.
session2.runSql("STOP SLAVE");

//@<> Rejoin instance when another operation in progress on the target (fail)
// Rejoin instance when another operation holds an exclusive lock on the target instance (fail).
// Simulate exclusive lock hold by another operation on instance 2 (target).
testutil.getExclusiveLock(session2, lock_instance, lock_name);

// WL#13540: TSFR3_1, TSFR2_3, TSFR3_1
EXPECT_THROWS(function() {
    rs.rejoinInstance(__sandbox_uri2);
}, `Failed to acquire lock on instance '${localhost}:${__mysql_sandbox_port2}'`);
EXPECT_OUTPUT_CONTAINS(`The operation cannot be executed because it failed to acquire the lock on instance '${localhost}:${__mysql_sandbox_port2}'. Another operation requiring access to the instance is still in progress, please wait for it to finish and try again.`);

// Release locks on instance 2.
testutil.releaseLocks(session2, lock_instance);

//@<> Rejoin instance when another operation holds a shared lock on the target instance (fail).
// Simulate shared lock hold by another operation on instance 2 (target).
testutil.getSharedLock(session2, lock_instance, lock_name);

EXPECT_THROWS(function() {
    rs.rejoinInstance(__sandbox_uri2);
}, `Failed to acquire lock on instance '${localhost}:${__mysql_sandbox_port2}'`);
EXPECT_OUTPUT_CONTAINS(`The operation cannot be executed because it failed to acquire the lock on instance '${localhost}:${__mysql_sandbox_port2}'. Another operation requiring access to the instance is still in progress, please wait for it to finish and try again.`);

// Release locks on instance 2.
testutil.releaseLocks(session2, lock_instance);

//@<> Rejoin instance when another operation holds an exclusive lock on the primary (fail).
// Simulate exclusive lock hold by another operation on instance 1 (primary).
testutil.getExclusiveLock(session1, lock_instance, lock_name);

// WL#13540: TSFR1_3, TSFR2_3
EXPECT_THROWS(function() {
    rs.rejoinInstance(__sandbox_uri2);
}, `Failed to acquire lock on instance '${localhost}:${__mysql_sandbox_port1}'`);
EXPECT_OUTPUT_CONTAINS(`The operation cannot be executed because it failed to acquire the lock on instance '${localhost}:${__mysql_sandbox_port1}'. Another operation requiring access to the instance is still in progress, please wait for it to finish and try again.`);

// Release locks on instance 1.
testutil.releaseLocks(session1, lock_instance);

//@<> Rejoin instance when another operation holds an shared lock on the primary (succeed).
// Simulate shared lock hold by another operation on instance 1 (primary).
testutil.getSharedLock(session1, lock_instance, lock_name);
// WL#13540: TSFR3_2
rs.rejoinInstance(__sandbox_uri2);
// Release locks on instance 1.
testutil.releaseLocks(session1, lock_instance);

//@<> (BUG#30673719) rejoinInstance on primary should not fail due to double self lock (using localhost).
rs.rejoinInstance(__sandbox1);
EXPECT_OUTPUT_CONTAINS(`The instance '${localhost}:${__mysql_sandbox_port1}' is ONLINE and the primary of the ReplicaSet.`);

//@<> (BUG#30673719) rejoinInstance on primary should not fail due to double self lock (using hostname).
rs.rejoinInstance(__endpoint1);
EXPECT_OUTPUT_CONTAINS(`The instance '${localhost}:${__mysql_sandbox_port1}' is ONLINE and the primary of the ReplicaSet.`);

//@<> (BUG#30673719) rejoinInstance on primary should not fail due to double self lock (using hostname_ip).
var address_using_ip = hostname_ip + ":" + __mysql_sandbox_port1;
rs.rejoinInstance(address_using_ip);
EXPECT_OUTPUT_CONTAINS(`The instance '${localhost}:${__mysql_sandbox_port1}' is ONLINE and the primary of the ReplicaSet.`);

//@<> (BUG#30673719) addInstance on primary should not fail due to double self lock (using localhost).
EXPECT_THROWS(function() {
    rs.addInstance(__sandbox1);
}, `${localhost}:${__mysql_sandbox_port1} is already a member of this replicaset.`);

//@<> (BUG#30673719) addInstance on primary should not fail due to double self lock (using hostname).
EXPECT_THROWS(function() {
    rs.addInstance(__endpoint1);
}, `${localhost}:${__mysql_sandbox_port1} is already a member of this replicaset.`);

//@<> (BUG#30673719) addInstance on primary should not fail due to double self lock (using hostname_ip).
EXPECT_THROWS(function() {
    rs.addInstance(address_using_ip);
}, `${localhost}:${__mysql_sandbox_port1} is already a member of this replicaset.`);

//@<> Add instance 3.
rs.addInstance(__sandbox_uri3);

//@<> Set primary instance when another operation in progress on the replica set (fail)
// Set primary instance when another operation holds an exclusive lock on the target instance (fail).
// Simulate exclusive lock hold by another operation on instance 2 (target).
testutil.getExclusiveLock(session2, lock_instance, lock_name);
__locked_port = __mysql_sandbox_port2;

// WL#13540: TSFR2_1
EXPECT_THROWS(function() {
    rs.setPrimaryInstance(__sandbox2);
}, `Failed to acquire lock on instance '${localhost}:${__locked_port}'`);
EXPECT_OUTPUT_CONTAINS(`The operation cannot be executed because it failed to acquire the lock on instance '${localhost}:${__locked_port}'. Another operation requiring access to the instance is still in progress, please wait for it to finish and try again.`);

// Release locks on instance 2.
testutil.releaseLocks(session2, lock_instance);

//@<><> Set primary instance when another operation holds a shared lock on the target instance (fail).
// Simulate shared lock hold by another operation on instance 2 (target).
testutil.getSharedLock(session2, lock_instance, lock_name);

EXPECT_THROWS(function() {
    rs.setPrimaryInstance(__sandbox2);
}, `Failed to acquire lock on instance '${localhost}:${__locked_port}'`);
EXPECT_OUTPUT_CONTAINS(`The operation cannot be executed because it failed to acquire the lock on instance '${localhost}:${__locked_port}'. Another operation requiring access to the instance is still in progress, please wait for it to finish and try again.`);

// Release locks on instance 2.
testutil.releaseLocks(session2, lock_instance);

//@<> Set primary instance when another operation holds an exclusive lock on the primary (fail).
// Simulate exclusive lock hold by another operation on instance 1 (target).
testutil.getExclusiveLock(session1, lock_instance, lock_name);
__locked_port = __mysql_sandbox_port1;

// WL#13540: TSFR2_1
EXPECT_THROWS(function() {
    rs.setPrimaryInstance(__sandbox2);
}, `Failed to acquire lock on instance '${localhost}:${__locked_port}'`);
EXPECT_OUTPUT_CONTAINS(`The operation cannot be executed because it failed to acquire the lock on instance '${localhost}:${__locked_port}'. Another operation requiring access to the instance is still in progress, please wait for it to finish and try again.`);

// Release locks on instance 1.
testutil.releaseLocks(session1, lock_instance);

//@<> Set primary instance when another operation holds a shared lock on the primary (fail).
// Simulate shared lock hold by another operation on instance 1 (target).
testutil.getSharedLock(session1, lock_instance, lock_name);

EXPECT_THROWS(function() {
    rs.setPrimaryInstance(__sandbox2);
}, `Failed to acquire lock on instance '${localhost}:${__locked_port}'`);
EXPECT_OUTPUT_CONTAINS(`The operation cannot be executed because it failed to acquire the lock on instance '${localhost}:${__locked_port}'. Another operation requiring access to the instance is still in progress, please wait for it to finish and try again.`);

// Release locks on instance 1.
testutil.releaseLocks(session1, lock_instance);

//@<> Set primary instance when another operation holds an exclusive lock on another rs instance (fail).
// Simulate exclusive lock hold by another operation on instance 3 (target).
testutil.getExclusiveLock(session3, lock_instance, lock_name);
__locked_port = __mysql_sandbox_port3;

// WL#13540: TSFR2_1
EXPECT_THROWS(function() {
    rs.setPrimaryInstance(__sandbox2);
}, `Failed to acquire lock on instance '${localhost}:${__locked_port}'`);
EXPECT_OUTPUT_CONTAINS(`The operation cannot be executed because it failed to acquire the lock on instance '${localhost}:${__locked_port}'. Another operation requiring access to the instance is still in progress, please wait for it to finish and try again.`);

// Release locks on instance 3.
testutil.releaseLocks(session3, lock_instance);

//@<> Set primary instance when another operation holds a shared lock on another rs instance (fail).
// Simulate shared lock hold by another operation on instance 3 (target).
testutil.getSharedLock(session3, lock_instance, lock_name);

EXPECT_THROWS(function() {
    rs.setPrimaryInstance(__sandbox2);
}, `Failed to acquire lock on instance '${localhost}:${__locked_port}'`);
EXPECT_OUTPUT_CONTAINS(`The operation cannot be executed because it failed to acquire the lock on instance '${localhost}:${__locked_port}'. Another operation requiring access to the instance is still in progress, please wait for it to finish and try again.`);

// Release locks on instance 3.
testutil.releaseLocks(session3, lock_instance);

//@<> Force primary instance when another operation in progress on the replica set (fail)
// Force primary instance when another operation holds an exclusive lock on the target instance (fail).
// Simulate exclusive lock hold by another operation on instance 2 (target).
testutil.getExclusiveLock(session2, lock_instance, lock_name);
__locked_port = __mysql_sandbox_port2;

// WL#13540: TSFR2_1
EXPECT_THROWS(function() {
    rs.forcePrimaryInstance(__sandbox2);
}, `Failed to acquire lock on instance '${localhost}:${__locked_port}'`);
EXPECT_OUTPUT_CONTAINS(`The operation cannot be executed because it failed to acquire the lock on instance '${localhost}:${__locked_port}'. Another operation requiring access to the instance is still in progress, please wait for it to finish and try again.`);

// Release locks on instance 2.
testutil.releaseLocks(session2, lock_instance);

//@<> Force primary instance when another operation holds an exclusive lock on another rs instance (fail).
// Simulate exclusive lock hold by another operation on instance 3 (target).
testutil.getExclusiveLock(session3, lock_instance, lock_name);
__locked_port = __mysql_sandbox_port3;

// WL#13540: TSFR2_1
EXPECT_THROWS(function() {
    rs.forcePrimaryInstance(__sandbox2);
}, `Failed to acquire lock on instance '${localhost}:${__locked_port}'`);
EXPECT_OUTPUT_CONTAINS(`The operation cannot be executed because it failed to acquire the lock on instance '${localhost}:${__locked_port}'. Another operation requiring access to the instance is still in progress, please wait for it to finish and try again.`);

// Release locks on instance 3.
testutil.releaseLocks(session3, lock_instance);

//@<> Force primary instance when another operation holds a shared lock on another rs instance (fail).
// Simulate shared lock hold by another operation on instance 3 (target).
testutil.getSharedLock(session3, lock_instance, lock_name);

EXPECT_THROWS(function() {
    rs.forcePrimaryInstance(__sandbox2);
}, `Failed to acquire lock on instance '${localhost}:${__locked_port}'`);
EXPECT_OUTPUT_CONTAINS(`The operation cannot be executed because it failed to acquire the lock on instance '${localhost}:${__locked_port}'. Another operation requiring access to the instance is still in progress, please wait for it to finish and try again.`);

// Release locks on instance 3.
testutil.releaseLocks(session3, lock_instance);

//@<> Remove router metadata when another operation holds an exclusive lock on the primary (fail).
// Simulate exclusive lock hold by another operation on instance 1 (primary).
testutil.getExclusiveLock(session1, lock_instance, lock_name);

// WL#13540: TSFR1_3
EXPECT_THROWS(function() {
    rs.removeRouterMetadata("routerhost");
}, `Failed to acquire lock on instance '${localhost}:${__mysql_sandbox_port1}'`);
EXPECT_OUTPUT_CONTAINS(`The operation cannot be executed because it failed to acquire the lock on instance '${localhost}:${__mysql_sandbox_port1}'. Another operation requiring access to the instance is still in progress, please wait for it to finish and try again.`);

// Release locks on instance 1.
testutil.releaseLocks(session1, lock_instance);

//@<> Upgrade Metadata another operation in progress (fail)
// Upgrade Metadata when another operation holds an exclusive lock on the primary (fail).
// Simulate exclusive lock hold by another operation on instance 1 (primary).
testutil.getExclusiveLock(session1, lock_instance, lock_name);

shell.connect(__sandbox_uri1);

// WL#13540: TSFR1_2
EXPECT_THROWS(function() {
    dba.upgradeMetadata();
}, `Failed to acquire lock on instance '${localhost}:${__mysql_sandbox_port1}'`);
EXPECT_OUTPUT_CONTAINS(`The operation cannot be executed because it failed to acquire the lock on instance '${localhost}:${__mysql_sandbox_port1}'. Another operation requiring access to the instance is still in progress, please wait for it to finish and try again.`);

// Release locks on instance 1.
testutil.releaseLocks(session1, lock_instance);

//@<> Upgrade Metadata when another operation holds a shared lock on the primary (fail).
// Simulate shared lock hold by another operation on instance 1.
testutil.getSharedLock(session1, lock_instance, lock_name);

// WL#13540: TSFR1_3
EXPECT_THROWS(function() {
    dba.upgradeMetadata();
}, `Failed to acquire lock on instance '${localhost}:${__mysql_sandbox_port1}'`);
EXPECT_OUTPUT_CONTAINS(`The operation cannot be executed because it failed to acquire the lock on instance '${localhost}:${__mysql_sandbox_port1}'. Another operation requiring access to the instance is still in progress, please wait for it to finish and try again.`);

// Release locks on instance 1.
testutil.releaseLocks(session1, lock_instance);

//@<> Trying to invoke rescan when another operation holds an shared / exclusive lock on any other instance
testutil.getSharedLock(session1, lock_instance, lock_name);
EXPECT_THROWS(function() {
    rs.rescan();
}, `Failed to acquire lock on instance '${localhost}:${__mysql_sandbox_port1}'`);
testutil.releaseLocks(session1, lock_instance);

testutil.getExclusiveLock(session2, lock_instance, lock_name);
EXPECT_THROWS(function() {
    rs.rescan();
}, `Failed to acquire lock on instance '${localhost}:${__mysql_sandbox_port2}'`);
testutil.releaseLocks(session2, lock_instance);

testutil.getSharedLock(session3, lock_instance, lock_name);
EXPECT_THROWS(function() {
    rs.rescan();
}, `Failed to acquire lock on instance '${localhost}:${__mysql_sandbox_port3}'`);
testutil.releaseLocks(session3, lock_instance);

//@<> Trying to invoke dissolve when another operation holds an shared / exclusive lock on any other instance

testutil.getSharedLock(session1, lock_instance, lock_name);
EXPECT_THROWS(function() {
    rs.dissolve();
}, `Failed to acquire lock on instance '${localhost}:${__mysql_sandbox_port1}'`);
testutil.releaseLocks(session1, lock_instance);

testutil.getExclusiveLock(session2, lock_instance, lock_name);
EXPECT_THROWS(function() {
    rs.dissolve();
}, `Failed to acquire lock on instance '${localhost}:${__mysql_sandbox_port2}'`);
testutil.releaseLocks(session2, lock_instance);

testutil.getSharedLock(session3, lock_instance, lock_name);
EXPECT_THROWS(function() {
    rs.dissolve();
}, `Failed to acquire lock on instance '${localhost}:${__mysql_sandbox_port3}'`);
testutil.releaseLocks(session3, lock_instance);

//@<> Cleanup.
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
testutil.destroySandbox(__mysql_sandbox_port3);
