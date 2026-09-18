//@ {DEF(MYSQLD_SECONDARY_SERVER_A) && VER(>=8.0.27) && testutil.versionCheck(MYSQLD_SECONDARY_SERVER_A.version, ">=", "8.0.27")}

shell.options.logLevel = "debug"

function get_base_version(version) {
    return version.split('-')[0].split('.').slice(0, 3).join('.');
}

function status_replication_compatibility_warning(source_version, target_version, source_port, target_port, potential=false) {
    let source_type = potential ? "potential replication source" : "replication source";

    return "WARNING: The " + source_type + " ('" + hostname + ":" + source_port + "') is running version " + source_version + ", which has limited compatibility with the target instance's ('" + hostname + ":" + target_port + "') version " + target_version + ". This setup should only be used for rollback purposes, where new functionality from version " + get_base_version(source_version) + " is not yet utilized. It is not suitable for regular continuous production deployment.";
}

//@<> INCLUDE replication_version_compatibility_utils.inc

//@<> Determine default/secondary replication compatibility
determine_replication_version_compatibility();

if (current_to_secondary_replication_is_incompatible()) {
    testutil.deploySandbox(__mysql_sandbox_port1, "root", { report_host: hostname }, { mysqldPath: MYSQLD_SECONDARY_SERVER_A.path });
    testutil.deploySandbox(__mysql_sandbox_port2, "root", { report_host: hostname });
    testutil.deploySandbox(__mysql_sandbox_port3, "root", { report_host: hostname });
} else if (higher_to_lower_replication_is_downgrade_only()) {
    deploy_high_version_sandbox(__mysql_sandbox_port1);
    deploy_low_version_sandbox(__mysql_sandbox_port2);
    deploy_high_version_sandbox(__mysql_sandbox_port3);
} else {
    testutil.skip("Unsupported mixed-version replication compatibility scenario.");
}

//<>@ Verify the replication compatibility checks in ReplicaSet.status()
shell.connect(__sandbox_uri1);
session2 = mysql.getSession(__sandbox_uri2);

var r;
EXPECT_NO_THROWS(function() { r = dba.createReplicaSet("rs"); });
EXPECT_NO_THROWS(function() { r.addInstance(__sandbox_uri2, {recoveryMethod: "incremental"}); });
EXPECT_NO_THROWS(function() { r.addInstance(__sandbox_uri3, {recoveryMethod: "incremental"}); });

var primary_version = get_server_version_with_update(session);

var secondary_version = get_server_version_with_update(session2);

var status = r.status();

if (current_to_secondary_replication_is_incompatible()) {
    EXPECT_FALSE("instanceErrors" in status["replicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port2}`]);
} else {
    EXPECT_EQ(status_replication_compatibility_warning(primary_version, secondary_version, __mysql_sandbox_port1, __mysql_sandbox_port2), status["replicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port2}`]["instanceErrors"][0])
}

// Instance3 is compatible
EXPECT_FALSE("instanceErrors" in status["replicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port3}`]);

//<>@ Verify the replication compatibility checks in Cluster.status() (read-replicas)
EXPECT_NO_THROWS(function() { r.dissolve(); });
EXPECT_NO_THROWS(function() { c = dba.createCluster("c"); });
EXPECT_NO_THROWS(function() { c.addInstance(__sandbox_uri3, {recoveryMethod: "incremental"}); });

// Add a read-replica using the default sources (primary)
EXPECT_NO_THROWS(function() { c.addReplicaInstance(__sandbox_uri2, {recoveryMethod: "incremental"}); });

var status = c.status();

if (current_to_secondary_replication_is_incompatible()) {
    EXPECT_FALSE("instanceErrors" in status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port1}`]["readReplicas"][`${hostname}:${__mysql_sandbox_port2}`]);
} else {
    EXPECT_EQ(status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port1}`]["readReplicas"][`${hostname}:${__mysql_sandbox_port2}`]["instanceErrors"].length, 2);

    EXPECT_EQ(status_replication_compatibility_warning(primary_version, secondary_version, __mysql_sandbox_port1, __mysql_sandbox_port2), status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port1}`]["readReplicas"][`${hostname}:${__mysql_sandbox_port2}`]["instanceErrors"][0])

    EXPECT_EQ(status_replication_compatibility_warning(primary_version, secondary_version, __mysql_sandbox_port3, __mysql_sandbox_port2, true), status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port1}`]["readReplicas"][`${hostname}:${__mysql_sandbox_port2}`]["instanceErrors"][1])
}

// Add a read-replica using the a fixed set of sources
EXPECT_NO_THROWS(function() { c.removeInstance(__sandbox_uri2); });

EXPECT_NO_THROWS(function() { c.addReplicaInstance(__sandbox_uri2, {recoveryMethod: "incremental", replicationSources: [__endpoint3]}); });

var status = c.status();

if (current_to_secondary_replication_is_incompatible()) {
    EXPECT_FALSE("instanceErrors" in status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port3}`]["readReplicas"][`${hostname}:${__mysql_sandbox_port2}`]);
} else {
    EXPECT_EQ(status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port3}`]["readReplicas"][`${hostname}:${__mysql_sandbox_port2}`]["instanceErrors"].length, 1);

    EXPECT_EQ(status_replication_compatibility_warning(primary_version, secondary_version, __mysql_sandbox_port3, __mysql_sandbox_port2), status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port3}`]["readReplicas"][`${hostname}:${__mysql_sandbox_port2}`]["instanceErrors"][0])
}

//<>@ Verify the replication compatibility checks in ClusterSet.status()
EXPECT_NO_THROWS(function() { c.removeInstance(__sandbox_uri2); });
EXPECT_NO_THROWS(function() { cs = c.createClusterSet("cs"); });
WIPE_SHELL_LOG()
EXPECT_NO_THROWS(function() { cs.createReplicaCluster(__sandbox_uri2, "replica_cluster", {recoveryMethod: "incremental"}); });

var status = cs.status({extended: 1});

if (current_to_secondary_replication_is_incompatible()) {
    EXPECT_FALSE("instanceErrors" in status["clusters"]["replica_cluster"]["topology"][`${hostname}:${__mysql_sandbox_port2}`]);
} else {
    EXPECT_EQ(status["clusters"]["replica_cluster"]["topology"][`${hostname}:${__mysql_sandbox_port2}`]["instanceErrors"].length, 2);

    EXPECT_EQ(status_replication_compatibility_warning(primary_version, secondary_version, __mysql_sandbox_port1, __mysql_sandbox_port2), status["clusters"]["replica_cluster"]["topology"][`${hostname}:${__mysql_sandbox_port2}`]["instanceErrors"][0])

    EXPECT_EQ(status_replication_compatibility_warning(primary_version, secondary_version, __mysql_sandbox_port3, __mysql_sandbox_port2, true), status["clusters"]["replica_cluster"]["topology"][`${hostname}:${__mysql_sandbox_port2}`]["instanceErrors"][1])

    EXPECT_SHELL_LOG_CONTAINS("Warning: The replication source ('" + hostname + ":" + __mysql_sandbox_port1 + "') is running version " + primary_version + ", which has limited compatibility with the target instance's ('" + hostname + ":" + __mysql_sandbox_port2 + "') version " + secondary_version + ". This setup should only be used for rollback purposes, where new functionality from version " + get_base_version(primary_version) + " is not yet utilized. It is not suitable for regular continuous production deployment.");
}

// Positive tests

//@<> Add a compatible read-replica - no issues but log entry
EXPECT_NO_THROWS(function() { c.removeInstance(__sandbox_uri3); });
WIPE_SHELL_LOG()

EXPECT_NO_THROWS(function() { c.addReplicaInstance(__sandbox_uri3, {recoveryMethod: "incremental"}); })

var compatible_read_replica_version = current_to_secondary_replication_is_incompatible() ? secondary_version : primary_version;

var status = c.status();
EXPECT_FALSE("instanceErrors" in status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port1}`]);

EXPECT_SHELL_LOG_CONTAINS("The replication source's ('" + hostname + ":" + __mysql_sandbox_port1 + "') version (" + primary_version + ") is compatible with the replica instance's ('" + hostname + ":" + __mysql_sandbox_port3 + "') version (" + compatible_read_replica_version + ").");

//@<> Add a compatible read-replica - no issues but log entry (log-level < debug)
EXPECT_NO_THROWS(function() { c.removeInstance(__sandbox_uri3); })
WIPE_SHELL_LOG()

shell.options.logLevel = "info"

EXPECT_NO_THROWS(function() { c.addReplicaInstance(__sandbox_uri3, {recoveryMethod: "incremental"}); })

var status = c.status();
EXPECT_FALSE("instanceErrors" in status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port1}`]);

EXPECT_SHELL_LOG_NOT_CONTAINS("The replication source's ('" + hostname + ":" + __mysql_sandbox_port1 + "') version (" + primary_version + ") is compatible with the replica instance's ('" + hostname + ":" + __mysql_sandbox_port3 + "') version (" + compatible_read_replica_version + ").");

//@<> Cleanup
session.close();
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
testutil.destroySandbox(__mysql_sandbox_port3);
