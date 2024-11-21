//@ {VER(>=8.0.27)}

function clusterset_lock_check(func, test_shared = true) {

    testutil.getExclusiveLock(session, "AdminAPI_clusterset", "AdminAPI_lock");
    EXPECT_THROWS(function () {
        func();
    }, `Failed to acquire ClusterSet lock through global primary member '${hostname}:${__mysql_sandbox_port1}'`);
    EXPECT_OUTPUT_CONTAINS(`The operation cannot be executed because it failed to acquire the ClusterSet lock through global primary member '${hostname}:${__mysql_sandbox_port1}'. Another operation requiring access to the member is still in progress, please wait for it to finish and try again.`);
    testutil.releaseLocks(session, "AdminAPI_clusterset");

    EXPECT_SHELL_LOG_NOT_CONTAINS("AdminAPI_metadata");
}

function cluster_lock_check(func) {

    testutil.getExclusiveLock(session, "AdminAPI_cluster", "AdminAPI_lock");
    EXPECT_THROWS(function () {
        func();
    }, `Failed to acquire Cluster lock through primary member '${hostname}:${__mysql_sandbox_port1}'`);
    EXPECT_OUTPUT_CONTAINS(`The operation cannot be executed because it failed to acquire the Cluster lock through primary member '${hostname}:${__mysql_sandbox_port1}'. Another operation requiring access to the member is still in progress, please wait for it to finish and try again.`);
    testutil.releaseLocks(session, "AdminAPI_cluster");

    testutil.getSharedLock(session, "AdminAPI_cluster", "AdminAPI_lock");
    EXPECT_THROWS(function () {
        func();
    }, `Failed to acquire Cluster lock through primary member '${hostname}:${__mysql_sandbox_port1}'`);
    EXPECT_OUTPUT_CONTAINS(`The operation cannot be executed because it failed to acquire the Cluster lock through primary member '${hostname}:${__mysql_sandbox_port1}'. Another operation requiring access to the member is still in progress, please wait for it to finish and try again.`);
    testutil.releaseLocks(session, "AdminAPI_cluster");

    EXPECT_SHELL_LOG_NOT_CONTAINS("AdminAPI_metadata");
}

//@<> INCLUDE gr_utils.inc

//@<> Initialization
var lock_cluster = "AdminAPI_cluster";
var lock_cluster_set = "AdminAPI_clusterset";
var lock_instance = "AdminAPI_instance";
var lock_name = "AdminAPI_lock";

var allowlist = "127.0.0.1," + hostname_ip;
var scene = new ClusterScenario([__mysql_sandbox_port1, __mysql_sandbox_port2], { ipAllowlist: allowlist, gtidSetIsComplete: true });
testutil.waitMemberState(__mysql_sandbox_port1, "ONLINE");
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");

testutil.deploySandbox(__mysql_sandbox_port3, "root", { report_host: hostname });
testutil.deploySandbox(__mysql_sandbox_port4, "root", { report_host: hostname });

cluster = scene.cluster;
scene.session.close();

shell.connect(__sandbox_uri2);
cluster2 = dba.getCluster();

shell.connect(__sandbox_uri1); //because clusterset_lock_check() and cluster_lock_check() use the current session

cset = cluster.createClusterSet("cset");

var session1 = mysql.getSession(__sandbox_uri1);
var session3 = mysql.getSession(__sandbox_uri3);

// *************************
// Cluster locks (exclusive)

//@<> exclusive lock on cluster.fenceWrites()

cluster_lock_check(function () {
    cluster.fenceWrites();
});
cluster_lock_check(function () {
    cluster2.fenceWrites();
});

//@<> exclusive lock on cluster.fenceAllTraffic()
cluster_lock_check(function () {
    cluster.fenceAllTraffic();
});
cluster_lock_check(function () {
    cluster2.fenceAllTraffic();
});

//@<> exclusive lock on cluster.unfenceWrites()
cluster_lock_check(function () {
    cluster.unfenceWrites();
});
cluster_lock_check(function () {
    cluster2.unfenceWrites();
});

// *************************
// ClusterSet locks (shared)

//@<> shared lock on clusterset.createReplicaCluster() (with an exclusive lock on target instance)

clusterset_lock_check(function () {
    cset.createReplicaCluster(__sandbox_uri3, "rcluster1", { recoveryMethod: "clone" });
});

testutil.getSharedLock(session1, lock_cluster_set, lock_name);
testutil.getExclusiveLock(session3, lock_instance, lock_name);

EXPECT_THROWS(function () {
    cset.createReplicaCluster(__sandbox_uri3, "rcluster1", { recoveryMethod: "clone" });
}, `Failed to acquire lock on instance '${hostname}:${__mysql_sandbox_port3}'`);
EXPECT_OUTPUT_CONTAINS(`The operation cannot be executed because it failed to acquire the lock on instance '${hostname}:${__mysql_sandbox_port3}'. Another operation requiring access to the instance is still in progress, please wait for it to finish and try again.`);
EXPECT_SHELL_LOG_NOT_CONTAINS("AdminAPI_metadata");

testutil.releaseLocks(session3, lock_instance);

EXPECT_NO_THROWS(function () {
    cset.createReplicaCluster(__sandbox_uri3, "rcluster1", { recoveryMethod: "clone" });
});
EXPECT_SHELL_LOG_NOT_CONTAINS("AdminAPI_metadata");

testutil.releaseLocks(session1, lock_cluster_set);

//@<> shared lock on clusterset.rejoinCluster() (with a shared lock on the primary cluster and an exclusive lock on the target cluster)
cset.createReplicaCluster(__sandbox_uri4, "rcluster2", { recoveryMethod: "clone" });
EXPECT_SHELL_LOG_NOT_CONTAINS("AdminAPI_metadata");

disable_auto_rejoin(__mysql_sandbox_port4);

testutil.killSandbox(__mysql_sandbox_port4);
testutil.waitMemberState(__mysql_sandbox_port4, "(MISSING)");
testutil.startSandbox(__mysql_sandbox_port4);

shell.connect(__sandbox_uri4);
dba.rebootClusterFromCompleteOutage();

shell.connect(__sandbox_uri1);

var session4 = mysql.getSession(__sandbox_uri4);

testutil.getExclusiveLock(session1, lock_cluster_set, lock_name);
testutil.getExclusiveLock(session1, lock_cluster, lock_name);

EXPECT_THROWS(function () {
    cset.rejoinCluster("rcluster2");
}, `Failed to acquire ClusterSet lock through global primary member '${hostname}:${__mysql_sandbox_port1}'`);
EXPECT_OUTPUT_CONTAINS(`The operation cannot be executed because it failed to acquire the ClusterSet lock through global primary member '${hostname}:${__mysql_sandbox_port1}'. Another operation requiring access to the member is still in progress, please wait for it to finish and try again.`);

testutil.releaseLocks(session1, lock_cluster_set);
testutil.getSharedLock(session1, lock_cluster_set, lock_name);

EXPECT_THROWS(function () {
    cset.rejoinCluster("rcluster2");
}, `Failed to acquire Cluster lock through primary member '${hostname}:${__mysql_sandbox_port1}'`);
EXPECT_OUTPUT_CONTAINS(`The operation cannot be executed because it failed to acquire the Cluster lock through primary member '${hostname}:${__mysql_sandbox_port1}'. Another operation requiring access to the member is still in progress, please wait for it to finish and try again.`);

testutil.releaseLocks(session1, lock_cluster);
testutil.getSharedLock(session1, lock_cluster, lock_name);
testutil.getExclusiveLock(session4, lock_cluster, lock_name);

EXPECT_THROWS(function () {
    cset.rejoinCluster("rcluster2");
}, `Failed to acquire Cluster lock through primary member '${hostname}:${__mysql_sandbox_port4}'`);
EXPECT_OUTPUT_CONTAINS(`The operation cannot be executed because it failed to acquire the Cluster lock through primary member '${hostname}:${__mysql_sandbox_port4}'. Another operation requiring access to the member is still in progress, please wait for it to finish and try again.`);

testutil.releaseLocks(session4, lock_cluster);

EXPECT_NO_THROWS(function () {
    cset.rejoinCluster("rcluster2");
});

testutil.releaseLocks(session1, lock_cluster_set);
testutil.releaseLocks(session1, lock_cluster);
testutil.releaseLocks(session4, lock_cluster);

session4.close();

// *************************
// ClusterSet locks (exclusive)

//@<> exclusive lock on clusterset.setPrimaryCluster() (with exclusive locks on all clusters of the set)

session1 = mysql.getSession(__sandbox_uri1);
session3 = mysql.getSession(__sandbox_uri3);
session4 = mysql.getSession(__sandbox_uri4);

testutil.getExclusiveLock(session1, lock_cluster_set, lock_name);
testutil.getExclusiveLock(session1, lock_cluster, lock_name);

EXPECT_THROWS(function () {
    cset.setPrimaryCluster("rcluster1");
}, `Failed to acquire ClusterSet lock through global primary member '${hostname}:${__mysql_sandbox_port1}'`);
EXPECT_OUTPUT_CONTAINS(`The operation cannot be executed because it failed to acquire the ClusterSet lock through global primary member '${hostname}:${__mysql_sandbox_port1}'. Another operation requiring access to the member is still in progress, please wait for it to finish and try again.`);

testutil.releaseLocks(session1, lock_cluster_set);

EXPECT_THROWS(function () {
    cset.setPrimaryCluster("rcluster1");
}, `Failed to acquire Cluster lock through primary member '${hostname}:${__mysql_sandbox_port1}'`);
EXPECT_OUTPUT_CONTAINS(`The operation cannot be executed because it failed to acquire the Cluster lock through primary member '${hostname}:${__mysql_sandbox_port1}'. Another operation requiring access to the member is still in progress, please wait for it to finish and try again.`);

testutil.releaseLocks(session1, lock_cluster);
testutil.getExclusiveLock(session3, lock_cluster, lock_name);

EXPECT_THROWS(function () {
    cset.setPrimaryCluster("rcluster1");
}, `Failed to acquire Cluster lock through primary member '${hostname}:${__mysql_sandbox_port3}'`);
EXPECT_OUTPUT_CONTAINS(`The operation cannot be executed because it failed to acquire the Cluster lock through primary member '${hostname}:${__mysql_sandbox_port3}'. Another operation requiring access to the member is still in progress, please wait for it to finish and try again.`);

testutil.releaseLocks(session3, lock_cluster);
testutil.getExclusiveLock(session4, lock_cluster, lock_name);

EXPECT_THROWS(function () {
    cset.setPrimaryCluster("rcluster1");
}, `Failed to acquire Cluster lock through primary member '${hostname}:${__mysql_sandbox_port4}'`);
EXPECT_OUTPUT_CONTAINS(`The operation cannot be executed because it failed to acquire the Cluster lock through primary member '${hostname}:${__mysql_sandbox_port4}'. Another operation requiring access to the member is still in progress, please wait for it to finish and try again.`);

testutil.releaseLocks(session4, lock_cluster);

EXPECT_NO_THROWS(function () {
    cset.setPrimaryCluster("rcluster1");
});

//@<> NO lock on clusterset.forcePrimaryCluster() (but with exclusive locks on all clusters of the set)

shell.connect(__sandbox_uri1);

testutil.killSandbox(__mysql_sandbox_port3);
testutil.waitMemberState(__mysql_sandbox_port3, "(MISSING)");

session1 = mysql.getSession(__sandbox_uri1);
session4 = mysql.getSession(__sandbox_uri4);

testutil.getExclusiveLock(session1, lock_cluster, lock_name);

EXPECT_THROWS(function () {
    cset.forcePrimaryCluster("cluster");
}, `Failed to acquire Cluster lock through primary member '${hostname}:${__mysql_sandbox_port1}'`);
EXPECT_OUTPUT_CONTAINS(`The operation cannot be executed because it failed to acquire the Cluster lock through primary member '${hostname}:${__mysql_sandbox_port1}'. Another operation requiring access to the member is still in progress, please wait for it to finish and try again.`);

testutil.releaseLocks(session1, lock_cluster);
testutil.getExclusiveLock(session4, lock_cluster, lock_name);

EXPECT_THROWS(function () {
    cset.forcePrimaryCluster("cluster");
}, `Failed to acquire Cluster lock through primary member '${hostname}:${__mysql_sandbox_port4}'`);
EXPECT_OUTPUT_CONTAINS(`The operation cannot be executed because it failed to acquire the Cluster lock through primary member '${hostname}:${__mysql_sandbox_port4}'. Another operation requiring access to the member is still in progress, please wait for it to finish and try again.`);

testutil.releaseLocks(session4, lock_cluster);

EXPECT_NO_THROWS(function () {
    cset.forcePrimaryCluster("cluster");
});

testutil.startSandbox(__mysql_sandbox_port3);

shell.connect(__sandbox_uri3);
dba.rebootClusterFromCompleteOutage();

shell.connect(__sandbox_uri1);
cset.rejoinCluster("rcluster1");

//@<> exclusive lock on clusterset.removeCluster() (with exclusive locks on the primary cluster and target cluster)

testutil.getExclusiveLock(session1, lock_cluster_set, lock_name);
testutil.getExclusiveLock(session1, lock_cluster, lock_name);

EXPECT_THROWS(function () {
    cset.removeCluster("rcluster2");
}, `Failed to acquire ClusterSet lock through global primary member '${hostname}:${__mysql_sandbox_port1}'`);
EXPECT_OUTPUT_CONTAINS(`The operation cannot be executed because it failed to acquire the ClusterSet lock through global primary member '${hostname}:${__mysql_sandbox_port1}'. Another operation requiring access to the member is still in progress, please wait for it to finish and try again.`);
EXPECT_SHELL_LOG_NOT_CONTAINS("AdminAPI_metadata");

testutil.releaseLocks(session1, lock_cluster_set);

EXPECT_THROWS(function () {
    cset.removeCluster("rcluster2");
}, `Failed to acquire Cluster lock through primary member '${hostname}:${__mysql_sandbox_port1}'`);
EXPECT_OUTPUT_CONTAINS(`The operation cannot be executed because it failed to acquire the Cluster lock through primary member '${hostname}:${__mysql_sandbox_port1}'. Another operation requiring access to the member is still in progress, please wait for it to finish and try again.`);
EXPECT_SHELL_LOG_NOT_CONTAINS("AdminAPI_metadata");

testutil.releaseLocks(session1, lock_cluster);
testutil.getExclusiveLock(session4, lock_cluster, lock_name);

EXPECT_THROWS(function () {
    cset.removeCluster("rcluster2");
}, `Failed to acquire Cluster lock through primary member '${hostname}:${__mysql_sandbox_port4}'`);
EXPECT_OUTPUT_CONTAINS(`The operation cannot be executed because it failed to acquire the Cluster lock through primary member '${hostname}:${__mysql_sandbox_port4}'. Another operation requiring access to the member is still in progress, please wait for it to finish and try again.`);
EXPECT_SHELL_LOG_NOT_CONTAINS("AdminAPI_metadata");

testutil.releaseLocks(session4, lock_cluster);

EXPECT_NO_THROWS(function () {
    cset.removeCluster("rcluster2");
});
EXPECT_SHELL_LOG_NOT_CONTAINS("AdminAPI_metadata");

//@<> exclusive lock on clusterset.setOption() (with an optional exclusive lock on the primary cluster)

testutil.getExclusiveLock(session1, lock_cluster_set, lock_name);
testutil.getExclusiveLock(session1, lock_cluster, lock_name);

EXPECT_THROWS(function () {
    cset.setOption("replicationAllowedHost", "%");
}, `Failed to acquire ClusterSet lock through global primary member '${hostname}:${__mysql_sandbox_port1}'`);
EXPECT_OUTPUT_CONTAINS(`The operation cannot be executed because it failed to acquire the ClusterSet lock through global primary member '${hostname}:${__mysql_sandbox_port1}'. Another operation requiring access to the member is still in progress, please wait for it to finish and try again.`);

testutil.releaseLocks(session1, lock_cluster_set);

EXPECT_THROWS(function () {
    cset.setOption("replicationAllowedHost", "%");
}, `Failed to acquire Cluster lock through primary member '${hostname}:${__mysql_sandbox_port1}'`);
EXPECT_OUTPUT_CONTAINS(`The operation cannot be executed because it failed to acquire the Cluster lock through primary member '${hostname}:${__mysql_sandbox_port1}'. Another operation requiring access to the member is still in progress, please wait for it to finish and try again.`);

testutil.releaseLocks(session1, lock_cluster);

EXPECT_NO_THROWS(function () {
    cset.setOption("replicationAllowedHost", "%");
});

//@<> exclusive lock (on clusterset, cluster and instance) on dba.upgradeMetadata()
shell.connect(__sandbox_uri1);

testutil.getExclusiveLock(session, lock_cluster_set, lock_name);
testutil.getExclusiveLock(session, lock_cluster, lock_name);
testutil.getExclusiveLock(session, lock_instance, lock_name);

EXPECT_THROWS(function () {
    dba.upgradeMetadata();
}, `Failed to acquire ClusterSet lock through global primary member '${hostname}:${__mysql_sandbox_port1}'`);
EXPECT_OUTPUT_CONTAINS(`The operation cannot be executed because it failed to acquire the ClusterSet lock through global primary member '${hostname}:${__mysql_sandbox_port1}'. Another operation requiring access to the member is still in progress, please wait for it to finish and try again.`);

testutil.releaseLocks(session, lock_cluster_set);

EXPECT_THROWS(function () {
    dba.upgradeMetadata();
}, `Failed to acquire Cluster lock through primary member '${hostname}:${__mysql_sandbox_port1}'`);
EXPECT_OUTPUT_CONTAINS(`The operation cannot be executed because it failed to acquire the Cluster lock through primary member '${hostname}:${__mysql_sandbox_port1}'. Another operation requiring access to the member is still in progress, please wait for it to finish and try again.`);

testutil.releaseLocks(session, lock_cluster);

EXPECT_THROWS(function () {
    dba.upgradeMetadata();
}, `Failed to acquire lock on instance '${hostname}:${__mysql_sandbox_port1}'`);
EXPECT_OUTPUT_CONTAINS(`The operation cannot be executed because it failed to acquire the lock on instance '${hostname}:${__mysql_sandbox_port1}'. Another operation requiring access to the instance is still in progress, please wait for it to finish and try again.`);

testutil.releaseLocks(session, lock_instance);

EXPECT_NO_THROWS(function () {
    dba.upgradeMetadata();
});

//@<> in a primary instance of a replica cluster, the lock service UDFs can't be installed

shell.connect(__sandbox_uri3);
rcluster1 = dba.getCluster();

session.runSql("set global super_read_only=0");
session.runSql("DROP FUNCTION IF EXISTS service_get_read_locks");
session.runSql("DROP FUNCTION IF EXISTS service_get_write_locks");
session.runSql("DROP FUNCTION IF EXISTS service_release_locks");
session.runSql("set global super_read_only=1");

EXPECT_NO_THROWS(function () {
    rcluster1.resetRecoveryAccountsPassword();
});

EXPECT_EQ(0, session.runSql("SELECT count(*) FROM mysql.func WHERE name IN ('service_get_read_locks', 'service_get_write_locks', 'service_release_locks')").fetchOne()[0]);

//@<> in instances where the lock service isn't installed and can't be, check if the lock requests turn into no-ops

//test no-op on cluster lock
shell.connect(__sandbox_uri3);
rcluster1 = dba.getCluster();

EXPECT_EQ(0, session.runSql("SELECT count(*) FROM mysql.func WHERE name IN ('service_get_read_locks', 'service_get_write_locks', 'service_release_locks')").fetchOne()[0]);

WIPE_SHELL_LOG();
EXPECT_NO_THROWS(function () {
    rcluster1.resetRecoveryAccountsPassword();
});
EXPECT_SHELL_LOG_CONTAINS(`The required MySQL Locking Service isn't installed on instance '${hostname}:${__mysql_sandbox_port3}'. The operation will continue without concurrent execution protection.`);

//test no-op on instance lock
shell.connect(__sandbox_uri2);

session.runSql("set global super_read_only=0");
session.runSql("DROP FUNCTION IF EXISTS service_get_read_locks");
session.runSql("DROP FUNCTION IF EXISTS service_get_write_locks");
session.runSql("DROP FUNCTION IF EXISTS service_release_locks");
session.runSql("set global super_read_only=1");

WIPE_SHELL_LOG();
EXPECT_NO_THROWS(function () {
    dba.configureInstance()
});
EXPECT_SHELL_LOG_CONTAINS(`The required MySQL Locking Service isn't installed on instance '${hostname}:${__mysql_sandbox_port2}'. The operation will continue without concurrent execution protection.`);

//@<> Cleanup
session4.close();
session3.close();
session1.close();

scene.destroy();

testutil.destroySandbox(__mysql_sandbox_port3);
testutil.destroySandbox(__mysql_sandbox_port4);
