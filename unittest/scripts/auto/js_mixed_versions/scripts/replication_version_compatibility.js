//@ {DEF(MYSQLD_SECONDARY_SERVER_A) && VER(>=8.0.27) && testutil.versionCheck(MYSQLD_SECONDARY_SERVER_A.version, ">=", "8.0.27")}

shell.options.logLevel = "debug"

function check_output(primary_version, secondary_version, primary_port=null, target_port=null, potential=false, clear=true) {
    let port = primary_port ? primary_port : __mysql_sandbox_port1;
    let port_replica = target_port ? target_port : __mysql_sandbox_port2;
    let potential_str = potential ? " potential " : " ";

    if (testutil.versionCheck(primary_version, '>', secondary_version)) {
      EXPECT_OUTPUT_CONTAINS("The" + potential_str + "replication source ('" + hostname + ":" + port + "') is running version " + primary_version + ", which has limited compatibility with the target instance's ('" + hostname + ":" + port_replica + "') version " + secondary_version + ". This setup should only be used for rollback purposes, where new functionality from version " + primary_version + " is not yet utilized. It is not suitable for regular continuous production deployment.");
    } else {
      EXPECT_SHELL_LOG_CONTAINS("The" + potential_str + "replication source's ('" + hostname + ":" + port_replica + "') version (" + primary_version + ") is compatible with the replica instance's ('" + hostname + ":" + port + "') version (" + secondary_version + ").");
    }

  if (clear) {
    WIPE_OUTPUT()
    WIPE_SHELL_LOG()
  }
}

if (testutil.versionCheck(MYSQLD_SECONDARY_SERVER_A.version, '>', __version)) {
    testutil.deploySandbox(__mysql_sandbox_port1, "root", { report_host: hostname }, { mysqldPath: MYSQLD_SECONDARY_SERVER_A.path });
    testutil.deploySandbox(__mysql_sandbox_port2, "root", { report_host: hostname });
    testutil.deploySandbox(__mysql_sandbox_port3, "root", { report_host: hostname });
} else {
    testutil.deploySandbox(__mysql_sandbox_port1, "root", { report_host: hostname });
    testutil.deploySandbox(__mysql_sandbox_port3, "root", { report_host: hostname });
    testutil.deploySandbox(__mysql_sandbox_port2, "root", { report_host: hostname }, { mysqldPath: MYSQLD_SECONDARY_SERVER_A.path });
}

//Verify the replication compatibility warnings in ReplicaSet

//@<> Verify the replication compatibility warning in addInstance()
shell.connect(__sandbox_uri1);
session2 = mysql.getSession(__sandbox_uri2);

var r;
EXPECT_NO_THROWS(function() { r = dba.createReplicaSet("rs"); });
EXPECT_NO_THROWS(function() { r.addInstance(__sandbox_uri2, {recoveryMethod: "incremental"}); });

var primary_version = session.runSql('SELECT @@version').fetchOne()[0];
primary_version = primary_version.split('-')[0];

var secondary_version = session2.runSql('SELECT @@version').fetchOne()[0];
secondary_version = secondary_version.split('-')[0];

check_output(primary_version, secondary_version);

//@<> Verify the replication compatibility warning in rejoinInstance()
session2.runSql("STOP replica");

EXPECT_NO_THROWS(function() { r.rejoinInstance(__sandbox_uri2, {recoveryMethod: "incremental"}); });

check_output(primary_version, secondary_version);

//@<> Verify the warning is ignored and just logged if dba.versionCompatibilityChecks is disabled
session2.runSql("STOP replica");

shell.options["dba.versionCompatibilityChecks"] = false;

EXPECT_NO_THROWS(function() { r.rejoinInstance(__sandbox_uri2, {recoveryMethod: "incremental"}); });

EXPECT_SHELL_LOG_CONTAINS("The replication source ('" + hostname + ":" + __mysql_sandbox_port1 + "') is running version " + primary_version + ", which has limited compatibility with the target instance's ('" + hostname + ":" + __mysql_sandbox_port2 + "') version " + secondary_version + ". This setup should only be used for rollback purposes, where new functionality from version " + primary_version + " is not yet utilized. It is not suitable for regular continuous production deployment. NOTE: Check ignored due to 'dba.versionCompatibilityChecks' being disabled.");

shell.options["dba.versionCompatibilityChecks"] = true;

//@<> Verify the replication compatibility warning in setPrimaryInstance()

// Add another instance to verify that the checks will check all potential sources

EXPECT_NO_THROWS(function() { r.addInstance(__sandbox_uri3, {recoveryMethod: "incremental"}); });

// Set the lowest version as primary
EXPECT_NO_THROWS(function() { r.setPrimaryInstance(__sandbox_uri2); });

check_output(secondary_version, primary_version);

// Set the highest version as primary
EXPECT_NO_THROWS(function() { r.setPrimaryInstance(__sandbox_uri1); });

check_output(primary_version, secondary_version, __mysql_sandbox_port1, __mysql_sandbox_port2, false, false);
check_output(primary_version, secondary_version, __mysql_sandbox_port3, __mysql_sandbox_port2, true, true);

// Forced failover: add a new member, kill the primary and failover

testutil.deploySandbox(__mysql_sandbox_port4, "root", { report_host: hostname });
EXPECT_NO_THROWS(function() { r.addInstance(__sandbox_uri4, {recoveryMethod: "incremental"}); });

// Kill primary
testutil.killSandbox(__mysql_sandbox_port1);
shell.connect(__sandbox_uri2);
r = dba.getReplicaSet();

//@<> Verify the replication compatibility warning in forcePrimaryInstance()
EXPECT_NO_THROWS(function() { r.forcePrimaryInstance(__sandbox_uri2); });

check_output(primary_version, secondary_version, __mysql_sandbox_port3, __mysql_sandbox_port2, true, false);
check_output(primary_version, secondary_version, __mysql_sandbox_port4, __mysql_sandbox_port2, true, true);

EXPECT_NO_THROWS(function() { r.removeInstance(__sandbox_uri4); });

EXPECT_NO_THROWS(function() { r.setPrimaryInstance(__sandbox_uri3); });
check_output(primary_version, secondary_version, __mysql_sandbox_port3, __mysql_sandbox_port2);

//@<> Verify the replication compatibility warning in rescan({addUnmanaged: true})
testutil.startSandbox(__mysql_sandbox_port1);
EXPECT_NO_THROWS(function() { r.rejoinInstance(__sandbox_uri1); });

shell.connect(__sandbox_uri3);
session.runSql("DELETE FROM mysql_innodb_cluster_metadata.instances WHERE (address = ?)", [`${hostname}:${__mysql_sandbox_port2}`]);
session.runSql("DELETE FROM mysql_innodb_cluster_metadata.instances WHERE (address = ?)", [`${hostname}:${__mysql_sandbox_port1}`]);

EXPECT_NO_THROWS(function() { r.rescan({addUnmanaged: true}); });

check_output(primary_version, secondary_version, __mysql_sandbox_port3);

//@<> Verify the replication compatibility warning in  createReplicaSet({adoptFromAR: true})
session.runSql("DROP SCHEMA mysql_innodb_cluster_metadata");

EXPECT_NO_THROWS(function() { r = dba.createReplicaSet("rs", {adoptFromAr: true}); });
check_output(primary_version, secondary_version, __mysql_sandbox_port3);

// Cleanup the ReplicaSet
EXPECT_NO_THROWS(function() { r.dissolve(); });

// Verify the replication compatibility warnings in Read-Replicas

//@<> Verify the replication compatibility warning in addReplicaInstance()
shell.connect(__sandbox_uri1);
var c;
EXPECT_NO_THROWS(function() { c = dba.createCluster("c"); });
EXPECT_NO_THROWS(function() { c.addReplicaInstance(__sandbox_uri2, {recoveryMethod: "incremental"}); });

check_output(primary_version, secondary_version);

session2.runSql("STOP replica");

//@<> Verify the replication compatibility warning in rejoinInstance() (read-replicas)
EXPECT_NO_THROWS(function() { c.rejoinInstance(__sandbox_uri2, {recoveryMethod: "incremental"}); });

check_output(primary_version, secondary_version);

//@<> Verify the replication compatibility warning in force_quorum (read-replicas)
EXPECT_NO_THROWS(function() { c.addInstance(__sandbox_uri3); });
testutil.killSandbox(__mysql_sandbox_port3);
testutil.waitMemberState(__mysql_sandbox_port3, "(MISSING),UNREACHABLE");
testutil.startSandbox(__mysql_sandbox_port3);
testutil.waitMemberState(__mysql_sandbox_port3, "UNREACHABLE");

EXPECT_NO_THROWS(function() { c.forceQuorumUsingPartitionOf(__sandbox_uri1); });

check_output(primary_version, secondary_version, __mysql_sandbox_port1, __mysql_sandbox_port2);

//<> Verify that if the read-replica has a custom list of sources and the source is no long part of the cluster or unreachable, it won't be verified
EXPECT_NO_THROWS(function() { c.rejoinInstance(__sandbox_uri3); });

EXPECT_NO_THROWS(function() { c.setInstanceOption(__sandbox_uri2, "replicationSources", [__endpoint3]); });

testutil.killSandbox(__mysql_sandbox_port3);
testutil.waitMemberState(__mysql_sandbox_port3, "(MISSING),UNREACHABLE");
testutil.startSandbox(__mysql_sandbox_port3);
testutil.waitMemberState(__mysql_sandbox_port3, "UNREACHABLE");

EXPECT_NO_THROWS(function() { c.forceQuorumUsingPartitionOf(__sandbox_uri1); });

EXPECT_OUTPUT_NOT_CONTAINS("The replication source ('" + hostname + ":" + __mysql_sandbox_port1 + "') is running version " + primary_version + ", which has limited compatibility with the target instance's ('" + hostname + ":" + __mysql_sandbox_port3 + "') version " + secondary_version + ". This setup should only be used for rollback purposes, where new functionality from version " + primary_version + " is not yet utilized. It is not suitable for regular continuous production deployment.");

//@<> Verify the replication compatibility warning in setInstanceOption() when changing the replicationSources for a Read-Replica
EXPECT_NO_THROWS(function() { c.setInstanceOption(__sandbox_uri2, "replicationSources", [__endpoint1]); });

check_output(primary_version, secondary_version, __mysql_sandbox_port1, __mysql_sandbox_port2);

//Verify no warning then the versions are compatible
EXPECT_NO_THROWS(function() { c.rejoinInstance(__sandbox_uri2); });
EXPECT_NO_THROWS(function() { c.rejoinInstance(__sandbox_uri3); });

//@<> Verify the replication compatibility warning for all potential sources in Read-Replicas

// Add a Read-Replica using the default options (primary source), ensure the check is performed on all potential sources (all cluster members)

EXPECT_NO_THROWS(function() { c.removeInstance(__sandbox_uri2); });
EXPECT_NO_THROWS(function() { c.addReplicaInstance(__sandbox_uri2, {recoveryMethod: "incremental"}); });

check_output(primary_version, secondary_version, __mysql_sandbox_port1, __mysql_sandbox_port2, false, false);
check_output(primary_version, secondary_version, __mysql_sandbox_port3, __mysql_sandbox_port2, true);

// Add a Read-Replica using a specific list of sources, ensure the check is performed on all potential sources
EXPECT_NO_THROWS(function() { c.removeInstance(__sandbox_uri2); });
EXPECT_NO_THROWS(function() { c.addReplicaInstance(__sandbox_uri2, {recoveryMethod: "incremental", replicationSources: [__endpoint3, __endpoint1]}); });

check_output(primary_version, secondary_version, __mysql_sandbox_port3, __mysql_sandbox_port2, false, false);
check_output(primary_version, secondary_version, __mysql_sandbox_port1, __mysql_sandbox_port2, true);

// Add a new instance can affect a Read-Replica potential sources: ensure the check in that case
EXPECT_NO_THROWS(function() { c.setInstanceOption(__sandbox_uri2, "replicationSources", "primary"); });
EXPECT_NO_THROWS(function() { c.removeInstance(__sandbox_uri3); });
EXPECT_NO_THROWS(function() { c.addInstance(__sandbox_uri3); });

check_output(primary_version, secondary_version, __mysql_sandbox_port3, __mysql_sandbox_port2, true);

// Test rejoinInstance() too
session3 = mysql.getSession(__sandbox_uri3);
session3.runSql("STOP GROUP_REPLICATION");

EXPECT_NO_THROWS(function() { c.rejoinInstance(__sandbox_uri3); });

check_output(primary_version, secondary_version, __mysql_sandbox_port3, __mysql_sandbox_port2, true);

// Same check applies when using .rescan({addUnmanaged: true})

EXPECT_NO_THROWS(function() { c.rejoinInstance(__sandbox_uri2); });

session.runSql("DELETE FROM mysql_innodb_cluster_metadata.instances WHERE (address = ?)", [`${hostname}:${__mysql_sandbox_port3}`]);

EXPECT_NO_THROWS(function() { c.rescan({addUnmanaged: true}); });

check_output(primary_version, secondary_version, __mysql_sandbox_port3, __mysql_sandbox_port2, true);

//@<> Verify that adding a read-replica without any incompatible source does not throw or print warnings

// Clean up the cluster
EXPECT_NO_THROWS(function() { c.dissolve(); });

shell.connect(__sandbox_uri2);

EXPECT_NO_THROWS(function() { c = dba.createCluster("c"); });
EXPECT_NO_THROWS(function() { c.addInstance(__sandbox_uri1, {recoveryMethod: "incremental"}); });

EXPECT_NO_THROWS(function() { c.addReplicaInstance(__sandbox_uri3, {recoveryMethod: "incremental", replicationSources: [__endpoint2]}); });

EXPECT_OUTPUT_NOT_CONTAINS("The replication source ('" + hostname + ":" + __mysql_sandbox_port2 + "') is running version " + primary_version + ", which has limited compatibility with the target instance's ('" + hostname + ":" + __mysql_sandbox_port3 + "') version " + secondary_version + ". This setup should only be used for rollback purposes, where new functionality from version " + primary_version + " is not yet utilized. It is not suitable for regular continuous production deployment.");

// Verify the replication compatibility warnings in ClusterSet

//@<> Verify the replication compatibility warning in createCluster()
EXPECT_NO_THROWS(function() { c.removeInstance(__sandbox_uri2); });
EXPECT_NO_THROWS(function() { c.addInstance(__sandbox_uri4, {recoveryMethod: "incremental"}); });

var cs;
EXPECT_NO_THROWS(function() { cs = c.createClusterSet("cs"); });
EXPECT_NO_THROWS(function() { cs.createReplicaCluster(__sandbox_uri2, "replica_cluster", {recoveryMethod: "incremental"}); });

check_output(primary_version, secondary_version, __mysql_sandbox_port1, __mysql_sandbox_port2, false, false);
check_output(primary_version, secondary_version, __mysql_sandbox_port4, __mysql_sandbox_port2, true, true);

// Fix the read-replica sources since instance2 was removed
EXPECT_NO_THROWS(function() { c.setInstanceOption(__sandbox_uri3, "replicationSources", "primary"); });
EXPECT_NO_THROWS(function() { c.rejoinInstance(__sandbox_uri3); });

//@<> Verify the replication compatibility warning in setPrimaryCluster()
EXPECT_NO_THROWS(function() { cs.setPrimaryCluster("replica_cluster"); });

check_output(secondary_version, primary_version);

EXPECT_NO_THROWS(function() { cs.setPrimaryCluster("c"); });

check_output(primary_version, secondary_version);

//@<> Verify the replication compatibility warning in rejoinCluster()
session2.runSql("STOP replica");

EXPECT_NO_THROWS(function() { cs.rejoinCluster("replica_cluster"); });

check_output(primary_version, secondary_version);

EXPECT_NO_THROWS(function() { c.removeInstance(__sandbox_uri3); });

//@<> Verify the replication compatibility warning in forcePrimaryCluster()

// "replica_cluster" is running the older version, primary the newer. Add another Replica Cluster running the newer
EXPECT_NO_THROWS(function() { cs.createReplicaCluster(__sandbox_uri3, "replica_cluster2", {recoveryMethod: "incremental"}); });

// Kill the primary cluster
EXPECT_NO_THROWS(function() { c.removeInstance(__sandbox_uri4); });
testutil.killSandbox(__mysql_sandbox_port1);
shell.connect(__sandbox_uri2);
cs = dba.getClusterSet();

// Elect the replica running the higher version so that "replica_cluster" running the older is rejoined and the check for compatibility warns
EXPECT_NO_THROWS(function() { cs.forcePrimaryCluster("replica_cluster2"); });

check_output(primary_version, secondary_version, __mysql_sandbox_port3);

//@<> Verify the replication compatibility warning in rebootCluster() when restoring a Replica Cluster
testutil.startSandbox(__mysql_sandbox_port1);
shell.connect(__sandbox_uri1);

// Restore the cluster, it won't be rejoined because it is invalidated
EXPECT_NO_THROWS(function() { dba.rebootClusterFromCompleteOutage(); });

EXPECT_OUTPUT_NOT_CONTAINS("The replication source ('" + hostname + ":" + __mysql_sandbox_port1 + "') is running version " + primary_version + ", which has limited compatibility with the target instance's ('" + hostname + ":" + __mysql_sandbox_port3 + "') version " + secondary_version + ". This setup should only be used for rollback purposes, where new functionality from version " + primary_version + " is not yet utilized. It is not suitable for regular continuous production deployment.");

EXPECT_NO_THROWS(function() { cs.rejoinCluster("c"); });

testutil.killSandbox(__mysql_sandbox_port2);
testutil.startSandbox(__mysql_sandbox_port2);
shell.connect(__sandbox_uri2);

// Restore the cluster, it will be auto-rejoined and the check must warn
EXPECT_NO_THROWS(function() { dba.rebootClusterFromCompleteOutage(); });

check_output(primary_version, secondary_version, __mysql_sandbox_port3, __mysql_sandbox_port2);

// Disable the checks and verify no warnings are shown, just a log entry
testutil.killSandbox(__mysql_sandbox_port2);
testutil.startSandbox(__mysql_sandbox_port2);
shell.connect(__sandbox_uri2);

shell.options["dba.versionCompatibilityChecks"] = false;

EXPECT_NO_THROWS(function() { dba.rebootClusterFromCompleteOutage(); });

EXPECT_OUTPUT_NOT_CONTAINS("The replication source ('" + hostname + ":" + __mysql_sandbox_port3 + "') is running version " + primary_version + ", which has limited compatibility with the target instance's ('" + hostname + ":" + __mysql_sandbox_port2 + "') version " + secondary_version + ". This setup should only be used for rollback purposes, where new functionality from version " + primary_version + " is not yet utilized. It is not suitable for regular continuous production deployment.");

EXPECT_SHELL_LOG_CONTAINS("The replication source ('" + hostname + ":" + __mysql_sandbox_port3 + "') is running version " + primary_version + ", which has limited compatibility with the target instance's ('" + hostname + ":" + __mysql_sandbox_port2 + "') version " + secondary_version + ". This setup should only be used for rollback purposes, where new functionality from version " + primary_version + " is not yet utilized. It is not suitable for regular continuous production deployment. NOTE: Check ignored due to 'dba.versionCompatibilityChecks' being disabled.");

shell.options["dba.versionCompatibilityChecks"] = true;

//@<> Verify the replication compatibility warning when adding an instance to a PRIMARY Cluster that is incompatible with the REPLICA Clusters

// primary cluster (replica_cluster2): sandbox3 (higher version)
// replica cluster (replica_cluster): sandbox2 (lower version)
// replica cluster 2 (c): sandbox1 (higher version)

shell.connect(__sandbox_uri3);
c = dba.getCluster();
cs = dba.getClusterSet();
EXPECT_NO_THROWS(function() { c.addInstance(__sandbox_uri4, {recoveryMethod: "incremental"}); });
check_output(primary_version, secondary_version, __mysql_sandbox_port4, __mysql_sandbox_port2, true);

// Check rejoin too
session4 = mysql.getSession(__sandbox_uri4);
session4.runSql("STOP GROUP_REPLICATION");

EXPECT_NO_THROWS(function() { c.rejoinInstance(__sandbox_uri4); });
check_output(primary_version, secondary_version, __mysql_sandbox_port4, __mysql_sandbox_port2, true);

//@<> Verify the replication compatibility warning when adding an instance to a REPLICA Cluster that is incompatible with the PRIMARY Cluster

// primary cluster (replica_cluster2): sandbox3 (higher version), sandbox4 (higher version)
// replica cluster (replica_cluster): sandbox2 (lower version)
// replica cluster 2 (c): sandbox1 (higher version)

EXPECT_NO_THROWS(function() { c.removeInstance(__sandbox_uri4); });

EXPECT_NO_THROWS(function() { cs.setPrimaryCluster("replica_cluster"); });

EXPECT_NO_THROWS(function() { c.addInstance(__sandbox_uri4, {recoveryMethod: "incremental"}); });

check_output(secondary_version, primary_version, __mysql_sandbox_port4, __mysql_sandbox_port2);

// Check rejoin too
session4.runSql("STOP GROUP_REPLICATION");
EXPECT_NO_THROWS(function() { c.rejoinInstance(__sandbox_uri4); });
check_output(secondary_version, primary_version, __mysql_sandbox_port4, __mysql_sandbox_port2);

// Positive tests

//@<> Add a compatible read-replica
EXPECT_NO_THROWS(function() { cs.removeCluster("c"); });
EXPECT_NO_THROWS(function() { c.addReplicaInstance(__sandbox_uri1, {recoveryMethod: "incremental"}); })

EXPECT_SHELL_LOG_CONTAINS("The replication source's ('" + hostname + ":" + __mysql_sandbox_port3 + "') version (" + primary_version + ") is compatible with the replica instance's ('" + hostname + ":" + __mysql_sandbox_port1 + "') version (" + primary_version + ").");
WIPE_SHELL_LOG()

//@<> Add a compatible read-replica - log disabled
shell.options.logLevel = "info"
EXPECT_NO_THROWS(function() { c.removeInstance(__sandbox_uri1); });
EXPECT_NO_THROWS(function() { c.addReplicaInstance(__sandbox_uri1, {recoveryMethod: "incremental"}); })

EXPECT_SHELL_LOG_NOT_CONTAINS("The replication source's ('" + hostname + ":" + __mysql_sandbox_port3 + "') version (" + primary_version + ") is compatible with the replica instance's ('" + hostname + ":" + __mysql_sandbox_port1 + "') version (" + primary_version + ").");

EXPECT_NO_THROWS(function() { cs.dissolve(); });
WIPE_SHELL_LOG()

//@<> Create a ReplicaSet with 2 instances, compatible
shell.options.logLevel = "debug"
var r;
EXPECT_NO_THROWS(function() { r = dba.createReplicaSet("rs"); });
EXPECT_NO_THROWS(function() { r.addInstance(__sandbox_uri4, {recoveryMethod: "incremental"}); });

EXPECT_SHELL_LOG_CONTAINS("The replication source's ('" + hostname + ":" + __mysql_sandbox_port3 + "') version (" + primary_version + ") is compatible with the replica instance's ('" + hostname + ":" + __mysql_sandbox_port4 + "') version (" + primary_version + ").");

//@<> Cleanup
session.close();
session2.close();
session3.close();
session4.close();
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
testutil.destroySandbox(__mysql_sandbox_port3);
testutil.destroySandbox(__mysql_sandbox_port4);
