function cluster_lock_check(func, test_shared = true) {

    WIPE_SHELL_LOG();

    testutil.getExclusiveLock(session, "AdminAPI_cluster", "AdminAPI_lock");
    EXPECT_THROWS(function() {
        func();
    }, `Failed to acquire Cluster lock through primary member '${hostname}:${__mysql_sandbox_port1}'`);
    EXPECT_OUTPUT_CONTAINS(`The operation cannot be executed because it failed to acquire the Cluster lock through primary member '${hostname}:${__mysql_sandbox_port1}'. Another operation requiring access to the member is still in progress, please wait for it to finish and try again.`);
    testutil.releaseLocks(session, "AdminAPI_cluster");

    if (test_shared) {
        testutil.getSharedLock(session, "AdminAPI_cluster", "AdminAPI_lock");
        EXPECT_THROWS(function() {
            func();
        }, `Failed to acquire Cluster lock through primary member '${hostname}:${__mysql_sandbox_port1}'`);
        EXPECT_OUTPUT_CONTAINS(`The operation cannot be executed because it failed to acquire the Cluster lock through primary member '${hostname}:${__mysql_sandbox_port1}'. Another operation requiring access to the member is still in progress, please wait for it to finish and try again.`);
        testutil.releaseLocks(session, "AdminAPI_cluster");
    }
}

function instance_lock_check(session_lock, session_port, func, test_shared = true) {

    WIPE_SHELL_LOG();

    testutil.getExclusiveLock(session_lock, "AdminAPI_instance", "AdminAPI_lock");
    EXPECT_THROWS(function() {
        func();
    }, `Failed to acquire lock on instance '${hostname}:${session_port}'`);
    EXPECT_OUTPUT_CONTAINS(`The operation cannot be executed because it failed to acquire the lock on instance '${hostname}:${session_port}'. Another operation requiring access to the instance is still in progress, please wait for it to finish and try again.`);
    testutil.releaseLocks(session_lock, "AdminAPI_instance");

    if (test_shared) {
        testutil.getSharedLock(session_lock, "AdminAPI_instance", "AdminAPI_lock");
        EXPECT_THROWS(function() {
            func();
        }, `Failed to acquire lock on instance '${hostname}:${session_port}'`);
        EXPECT_OUTPUT_CONTAINS(`The operation cannot be executed because it failed to acquire the lock on instance '${hostname}:${session_port}'. Another operation requiring access to the instance is still in progress, please wait for it to finish and try again.`);
        testutil.releaseLocks(session_lock, "AdminAPI_instance");
    }
}

//@<> INCLUDE gr_utils.inc

//@<> Initialization
var lock_cluster = "AdminAPI_cluster";
var lock_instance = "AdminAPI_instance";
var lock_name = "AdminAPI_lock";

var allowlist = "127.0.0.1," + hostname_ip;
var scene = new ClusterScenario([__mysql_sandbox_port1, __mysql_sandbox_port2], {ipAllowlist: allowlist, gtidSetIsComplete: true});
testutil.waitMemberState(__mysql_sandbox_port1, "ONLINE");
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");

testutil.deploySandbox(__mysql_sandbox_port3, "root", {report_host: hostname});
testutil.deploySandbox(__mysql_sandbox_port4, "root", {report_host: hostname});

cluster = scene.cluster;
scene.session.close();

shell.connect(__sandbox_uri2);
cluster2 = dba.getCluster();

shell.connect(__sandbox_uri1); //because cluster_lock_check() uses the current session

// *************************
// Cluster locks (exclusive)

//@<> exclusive lock on cluster.resetRecoveryAccountsPassword()
cluster_lock_check(function() {
    cluster.resetRecoveryAccountsPassword();
});
cluster_lock_check(function() {
    cluster2.resetRecoveryAccountsPassword();
});

//@<> exclusive lock on cluster.setOption()
cluster_lock_check(function() {
    cluster.setOption("memberWeight", 25);
});
cluster_lock_check(function() {
    cluster2.setOption("memberWeight", 25);
});

// options "tag:" and "clusterName" must succeed
testutil.getExclusiveLock(session, lock_cluster, lock_name);
EXPECT_NO_THROWS(function() {
    cluster.setOption("tag:my_tag", "tag_val");
});
EXPECT_NO_THROWS(function() {
    cluster.setOption("clusterName", "clusterName");
});
testutil.releaseLocks(session, lock_cluster);

//@<> exclusive lock on cluster.forceQuorum()
cluster_lock_check(function() {
    cluster.forceQuorumUsingPartitionOf(__sandbox_uri2);
});
cluster_lock_check(function() {
    cluster2.forceQuorumUsingPartitionOf(__sandbox_uri2);
});

//@<> exclusive lock on cluster.rescan()
cluster_lock_check(function() {
    cluster.rescan({addInstances: [`${hostname}:${__mysql_sandbox_port1}`]});
});
cluster_lock_check(function() {
    cluster2.rescan({addInstances: [`${hostname}:${__mysql_sandbox_port1}`]});
});

//@<> exclusive lock on dba.upgradeMetadata()
cluster_lock_check(function() {
    dba.upgradeMetadata();
});

//@<> exclusive lock on cluster.setPrimaryInstance()
cluster_lock_check(function() {
    cluster.setPrimaryInstance(__sandbox_uri2);
});
cluster_lock_check(function() {
    cluster2.setPrimaryInstance(__sandbox_uri2);
});

//@<> exclusive lock on cluster.switchToMultiPrimaryMode()
cluster_lock_check(function() {
    cluster.switchToMultiPrimaryMode();
});
cluster_lock_check(function() {
    cluster2.switchToMultiPrimaryMode();
});

//@<> exclusive lock on cluster.switchToSinglePrimaryMode()
cluster_lock_check(function() {
    cluster.switchToSinglePrimaryMode();
});
cluster_lock_check(function() {
    cluster2.switchToSinglePrimaryMode();
});

//@<> exclusive lock on cluster.createClusterSet() {VER(>=8.0.27)}
cluster_lock_check(function() {
    cluster.createClusterSet("cset");
});
cluster_lock_check(function() {
    cluster2.createClusterSet("cset");
});

//@<> exclusive lock on cluster.addInstance()
cluster_lock_check(function() {
    cluster.addInstance(__sandbox_uri3, {ipAllowlist: allowlist});
});
cluster_lock_check(function() {
    cluster.addInstance(__sandbox_uri3, {ipAllowlist: allowlist});
});

//@<> exclusive lock on cluster.addReplicaInstance() {VER(>=8.0.23)}
cluster_lock_check(function() {
    cluster.addReplicaInstance(__sandbox_uri4);
});
cluster_lock_check(function() {
    cluster.addReplicaInstance(__sandbox_uri4);
});

//@<> exclusive lock on cluster.removeInstance()
cluster_lock_check(function() {
    cluster.removeInstance(__sandbox_uri3);
});
cluster_lock_check(function() {
    cluster.removeInstance(__sandbox_uri3);
});

//@<> exclusive lock on cluster.removeInstance() - Read Replica {VER(>=8.0.23)}
cluster_lock_check(function() {
    cluster.removeInstance(__sandbox_uri4);
});
cluster_lock_check(function() {
    cluster.removeInstance(__sandbox_uri4);
});

//@<> exclusive lock on cluster.dissolve()
cluster_lock_check(function() {
    cluster.dissolve();
});
cluster_lock_check(function() {
    cluster.dissolve();
});

// **********************
// Cluster locks (shared)

//@<> shared lock on cluster.rejoinInstance()

cluster.addInstance(__sandbox_uri3, {ipAllowlist: allowlist});

var session3 = mysql.getSession(__sandbox_uri3);
session3.runSql("STOP group_replication;");
session3.close();

cluster_lock_check(function() {
    cluster.rejoinInstance(__sandbox_uri3, {ipAllowlist: allowlist});
}, false);
cluster_lock_check(function() {
    cluster2.rejoinInstance(__sandbox_uri3, {ipAllowlist: allowlist});
}, false);

testutil.getSharedLock(session, lock_cluster, lock_name);
EXPECT_NO_THROWS(function() {
    cluster.rejoinInstance(__sandbox_uri3, {ipAllowlist: allowlist});
});
testutil.releaseLocks(session, lock_cluster);

cluster.removeInstance(__sandbox_uri3);

var session3 = mysql.getSession(__sandbox_uri3);
reset_instance(session3);
session3.close();

//@<> shared lock on cluster.setupAdminAccount()
cluster_lock_check(function() {
    cluster.setupAdminAccount("%", {password: "foo"});
}, false);
cluster_lock_check(function() {
    cluster2.setupAdminAccount("%", {password: "foo"});
}, false);

testutil.getSharedLock(session, lock_cluster, lock_name);
EXPECT_NO_THROWS(function() {
    cluster.setupAdminAccount("%", {password: "foo"});
});
testutil.releaseLocks(session, lock_cluster);

//@<> shared lock on cluster.setupRouterAccount()
cluster_lock_check(function() {
    cluster.setupRouterAccount("router@'%'", {password: "bar"});
}, false);
cluster_lock_check(function() {
    cluster2.setupRouterAccount("router@'%'", {password: "bar"});
}, false);

testutil.getSharedLock(session, lock_cluster, lock_name);
EXPECT_NO_THROWS(function() {
    cluster.setupRouterAccount("router@'%'", {password: "bar"});
});
testutil.releaseLocks(session, lock_cluster);

//@<> shared lock on cluster.removeRouterMetadata()
cluster_lock_check(function() {
    cluster.removeRouterMetadata("bogus");
}, false);

cluster_lock_check(function() {
    cluster2.removeRouterMetadata("bogus");
}, false);

testutil.getSharedLock(session, lock_cluster, lock_name);
EXPECT_THROWS(function() {
    cluster.removeRouterMetadata("bogus");
}, "Invalid router instance 'bogus'");
testutil.releaseLocks(session, lock_cluster);

// **************
// Instance locks

var session1 = mysql.getSession(__sandbox_uri1);
var session3 = mysql.getSession(__sandbox_uri3);
var session4 = mysql.getSession(__sandbox_uri4);

//@<> exclusive instance lock on cluster.addInstance()
instance_lock_check(session3, __mysql_sandbox_port3, function() {
    cluster.addInstance(__sandbox_uri3, {ipAllowlist: allowlist});
});

//@<> exclusive instance lock on cluster.addReplicaInstance() {VER(>=8.0.23)}
instance_lock_check(session4, __mysql_sandbox_port4, function() {
    cluster.addReplicaInstance(__sandbox_uri4);
});

//@<> exclusive instance lock on cluster.removeInstance()
instance_lock_check(session1, __mysql_sandbox_port1, function() {
    cluster.removeInstance(__sandbox_uri1);
});

//@<> exclusive instance lock on cluster.rejoinInstance()
instance_lock_check(session1, __mysql_sandbox_port1, function() {
    cluster.rejoinInstance(__sandbox_uri1, {ipAllowlist: allowlist});
});

//@<> exclusive instance lock on cluster.setInstanceOption()
instance_lock_check(session1, __mysql_sandbox_port1, function() {
    cluster.setInstanceOption(__sandbox_uri1, "memberWeight", 100);
});

// options "tag:" and "label" must succeed
testutil.getExclusiveLock(session1, lock_instance, lock_name);
EXPECT_NO_THROWS(function() {
    cluster.setInstanceOption(__sandbox_uri1, "tag:my_tag", "tag_val");
});
EXPECT_NO_THROWS(function() {
    cluster.setInstanceOption(__sandbox_uri1, "label", "label_val");
});
testutil.releaseLocks(session1, lock_instance);

//@<> exclusive instance lock on dba.createCluster()
shell.connect(__sandbox_uri3);

instance_lock_check(session3, __mysql_sandbox_port3, function() {
    dba.createCluster("cluster");
});

//@<> exclusive instance lock on dba.configureInstance()
instance_lock_check(session3, __mysql_sandbox_port3, function() {
    dba.configureInstance(__sandbox_uri3);
});

session1.close();
session3.close();
session4.close();

//@<> exclusive instance lock on dba.rebootClusterFromCompleteOutage()

cluster.status();

session1 = mysql.getSession(__sandbox_uri1);
session2 = mysql.getSession(__sandbox_uri2);

session1.runSql("STOP group_replication;");
session2.runSql("STOP group_replication;");

shell.connect(__sandbox_uri1);

instance_lock_check(session1, __mysql_sandbox_port1, function() {
    dba.rebootClusterFromCompleteOutage("clusterName", {ipAllowlist: allowlist});
});
instance_lock_check(session2, __mysql_sandbox_port2, function() {
    dba.rebootClusterFromCompleteOutage("clusterName", {ipAllowlist: allowlist});
});
EXPECT_NO_THROWS(function() {
    dba.rebootClusterFromCompleteOutage("clusterName", {ipAllowlist: allowlist});
});

session1.close();
session2.close();

//@<> check if, in multi-primary mode, the same primary instance is chosen

session1 = mysql.getSession(__sandbox_uri1);
session2 = mysql.getSession(__sandbox_uri2);

shell.connect(__sandbox_uri1);

testutil.waitMemberState(__mysql_sandbox_port1, "ONLINE");
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");

cluster = dba.getCluster();
cluster.switchToMultiPrimaryMode();

primary_member_id = session.runSql("SELECT member_id FROM performance_schema.replication_group_members ORDER BY member_id").fetchOne()[0];
primary_address = session.runSql("SELECT address FROM mysql_innodb_cluster_metadata.instances WHERE mysql_server_uuid = ?", [primary_member_id]).fetchOne()[0];

testutil.getExclusiveLock(session1, lock_cluster, lock_name);
testutil.getExclusiveLock(session2, lock_cluster, lock_name);

EXPECT_THROWS(function() {
    cluster.switchToSinglePrimaryMode();
}, `Failed to acquire Cluster lock through primary member '${primary_address}'`);
EXPECT_OUTPUT_CONTAINS(`The operation cannot be executed because it failed to acquire the Cluster lock through primary member '${primary_address}'. Another operation requiring access to the member is still in progress, please wait for it to finish and try again.`);

EXPECT_THROWS(function() {
    cluster.switchToSinglePrimaryMode();
}, `Failed to acquire Cluster lock through primary member '${primary_address}'`);
EXPECT_OUTPUT_CONTAINS(`The operation cannot be executed because it failed to acquire the Cluster lock through primary member '${primary_address}'. Another operation requiring access to the member is still in progress, please wait for it to finish and try again.`);

shell.connect(__sandbox_uri2);
cluster2 = dba.getCluster();

EXPECT_THROWS(function() {
    cluster2.switchToSinglePrimaryMode();
}, `Failed to acquire Cluster lock through primary member '${primary_address}'`);
EXPECT_OUTPUT_CONTAINS(`The operation cannot be executed because it failed to acquire the Cluster lock through primary member '${primary_address}'. Another operation requiring access to the member is still in progress, please wait for it to finish and try again.`);

EXPECT_THROWS(function() {
    cluster2.switchToSinglePrimaryMode();
}, `Failed to acquire Cluster lock through primary member '${primary_address}'`);
EXPECT_OUTPUT_CONTAINS(`The operation cannot be executed because it failed to acquire the Cluster lock through primary member '${primary_address}'. Another operation requiring access to the member is still in progress, please wait for it to finish and try again.`);

testutil.releaseLocks(session2, lock_cluster);
testutil.releaseLocks(session1, lock_cluster);

EXPECT_NO_THROWS(function() {
    cluster.switchToSinglePrimaryMode();
});

// moving forward, make sure that the primary is...
EXPECT_NO_THROWS(function() {
    cluster.setPrimaryInstance(__sandbox_uri1);
});

session1.close();
session2.close();

//@<> in a primary instance of a cluster, the lock service UDFs can and should be installed *and* not replicated

session2 = mysql.getSession(__sandbox_uri2);

shell.connect(__sandbox_uri1);

cluster = dba.getCluster();
status = cluster.status();
EXPECT_EQ(`${hostname}:${__mysql_sandbox_port1}`, status["defaultReplicaSet"]["primary"]);

session.runSql("DROP FUNCTION IF EXISTS service_get_read_locks");
session.runSql("DROP FUNCTION IF EXISTS service_get_write_locks");
session.runSql("DROP FUNCTION IF EXISTS service_release_locks");

testutil.waitMemberTransactions(__mysql_sandbox_port2, __mysql_sandbox_port1);

session2.runSql("set global super_read_only=0");
session2.runSql("DROP FUNCTION IF EXISTS service_get_read_locks");
session2.runSql("DROP FUNCTION IF EXISTS service_get_write_locks");
session2.runSql("DROP FUNCTION IF EXISTS service_release_locks");
session2.runSql("set global super_read_only=1");

EXPECT_NO_THROWS(function() {
    cluster.resetRecoveryAccountsPassword();
});

EXPECT_EQ(3, session.runSql("SELECT count(*) FROM mysql.func WHERE name IN ('service_get_read_locks', 'service_get_write_locks', 'service_release_locks')").fetchOne()[0]);
EXPECT_EQ(0, session2.runSql("SELECT count(*) FROM mysql.func WHERE name IN ('service_get_read_locks', 'service_get_write_locks', 'service_release_locks')").fetchOne()[0]);

//this will restore the lock service
session2.runSql("set global super_read_only=0");
testutil.getExclusiveLock(session2, lock_cluster, lock_name);
testutil.releaseLocks(session2, lock_cluster);
session2.runSql("set global super_read_only=1");

session2.close();

//@<> check if dba.createCluster with "adoptFromGR" locks all instances

var session1 = mysql.getSession(__sandbox_uri1);
var session2 = mysql.getSession(__sandbox_uri2);

dba.dropMetadataSchema({force: true});

instance_lock_check(session1, __mysql_sandbox_port1, function() {
    cluster = dba.createCluster('cluster', {adoptFromGR: true, force: true});
});
instance_lock_check(session2, __mysql_sandbox_port2, function() {
    cluster = dba.createCluster('cluster', {adoptFromGR: true, force: true});
});

EXPECT_NO_THROWS(function() {
    cluster = dba.createCluster('cluster', {adoptFromGR: true, force: true});
});

cluster.status();

session1.close();
session2.close();

//@<> Cleanup
scene.destroy();
testutil.destroySandbox(__mysql_sandbox_port3);
testutil.destroySandbox(__mysql_sandbox_port4);
