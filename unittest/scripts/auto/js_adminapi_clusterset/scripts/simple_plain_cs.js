//@ {VER(>=8.0.27)}

// Plain ClusterSet test, use as a template for other tests that check
// a specific feature/aspect across the whole API

//@<> INCLUDE clusterset_utils.inc

//@<> Setup
testutil.deployRawSandbox(__mysql_sandbox_port1, "root", {report_host: hostname});
testutil.snapshotSandboxConf(__mysql_sandbox_port1);
testutil.deploySandbox(__mysql_sandbox_port2, "root", {report_host: hostname});
testutil.snapshotSandboxConf(__mysql_sandbox_port2);
testutil.deploySandbox(__mysql_sandbox_port3, "root", {report_host: hostname});
testutil.snapshotSandboxConf(__mysql_sandbox_port3);
testutil.deployRawSandbox(__mysql_sandbox_port4, "root", {report_host: hostname});
testutil.snapshotSandboxConf(__mysql_sandbox_port4);

shell.options.useWizards = false;

//@<> configureInstances
EXPECT_NO_THROWS(function() { dba.configureInstance(__sandbox_uri1, {clusterAdmin:"admin", clusterAdminPassword:"bla"}); });
EXPECT_NO_THROWS(function() { dba.configureInstance(__sandbox_uri2, {clusterAdmin:"admin", clusterAdminPassword:"bla"}); });
EXPECT_NO_THROWS(function() { dba.configureInstance(__sandbox_uri3, {clusterAdmin:"admin", clusterAdminPassword:"bla"}); });
EXPECT_NO_THROWS(function() { dba.configureInstance(__sandbox_uri4, {clusterAdmin:"admin", clusterAdminPassword:"bla"}); });

testutil.restartSandbox(__mysql_sandbox_port1);
testutil.restartSandbox(__mysql_sandbox_port2);
testutil.restartSandbox(__mysql_sandbox_port3);
testutil.restartSandbox(__mysql_sandbox_port4);

//@<> create Primary Cluster
shell.connect(__sandbox_uri1);

var cluster;
EXPECT_NO_THROWS(function() { cluster = dba.createCluster("cluster"); });

//@ createClusterSet
EXPECT_NO_THROWS(function() {cluster.createClusterSet("clusterset"); });

//@<> validate primary cluster
CHECK_PRIMARY_CLUSTER([__sandbox_uri1], cluster)

//@<> dba.getClusterSet()
var clusterset;
EXPECT_NO_THROWS(function() {clusterset = dba.getClusterSet(); });
EXPECT_NE(clusterset, null);

//@<> cluster.getClusterSet()
var cs;
EXPECT_NO_THROWS(function() {cs = cluster.getClusterSet(); });
EXPECT_NE(cs, null);

// status()
// TODO

// describe()
// TODO

//@ createReplicaCluster() - incremental recovery
// SRO might be set in the instance so we must ensure the proper handling of it in createReplicaCluster.
var session4 = mysql.getSession(__sandbox_uri4);
session4.runSql("SET PERSIST super_read_only=true");
var replicacluster;
EXPECT_NO_THROWS(function() {replicacluster = clusterset.createReplicaCluster(__sandbox_uri4, "replicacluster", {recoveryMethod: "incremental"}); });

//@<> validate replica cluster - incremental recovery
CHECK_REPLICA_CLUSTER([__sandbox_uri4], cluster, replicacluster);

// Test regular InnoDB Cluster operations on the PRIMARY Cluster:
//  - Add instance
//  - Remove instance
//  - Rejoin instance
//  - Rescan
//  - Reset recovery account
//  - Set instance option
//  - Set option
//  - Set primary instance
//  - Setup admin account
//  - Setup router account

shell.connect(__sandbox_uri1);
cluster = dba.getCluster();

//@<> addInstance on primary cluster
EXPECT_NO_THROWS(function() { cluster.addInstance(__sandbox_uri2, {recoveryMethod: "incremental"}); });
EXPECT_NO_THROWS(function() { cluster.addInstance(__sandbox_uri3, {recoveryMethod: "clone"}); });
CHECK_PRIMARY_CLUSTER([__sandbox_uri1, __sandbox_uri2, __sandbox_uri3], cluster);

//@<> rejoinInstance on a primary cluster
session3 = mysql.getSession(__sandbox_uri3);
session3.runSql("STOP group_replication");
EXPECT_NO_THROWS(function() { cluster.rejoinInstance(__sandbox_uri3); });
testutil.waitMemberState(__mysql_sandbox_port3, "ONLINE");
CHECK_PRIMARY_CLUSTER([__sandbox_uri1, __sandbox_uri2, __sandbox_uri3], cluster);

//@<> rescan() on a primary cluster
// delete sb3 from the metadata so that rescan picks it up
session.runSql("DELETE FROM mysql_innodb_cluster_metadata.instances WHERE instance_name LIKE ?", ["%:"+__mysql_sandbox_port3]);

EXPECT_NO_THROWS(function() { cluster.rescan({addInstances: "auto"}); });
testutil.waitMemberState(__mysql_sandbox_port3, "ONLINE");
CHECK_PRIMARY_CLUSTER([__sandbox_uri1, __sandbox_uri2, __sandbox_uri3], cluster);

//@<> resetRecoveryAccountsPassword() on a primary cluster
EXPECT_NO_THROWS(function() { cluster.resetRecoveryAccountsPassword(); });
CHECK_PRIMARY_CLUSTER([__sandbox_uri1, __sandbox_uri2, __sandbox_uri3], cluster);

//@<> setInstanceOption() on a primary cluster
EXPECT_NO_THROWS(function() { cluster.setInstanceOption(__sandbox_uri2, "memberWeight", 25); });

//@<> setOption() on a primary cluster
EXPECT_NO_THROWS(function() { cluster.setOption("memberWeight", 50); });

//@<> setupAdminAccount() on a primary cluster
EXPECT_NO_THROWS(function() { cluster.setupAdminAccount("cadmin@'%'", {password:"boo"}); });

//@<> setupRouterAccount() on a primary cluster
EXPECT_NO_THROWS(function() { cluster.setupRouterAccount("router@'%'", {password:"boo"}); });

//@<> removeInstance on primary cluster
EXPECT_NO_THROWS(function() { cluster.removeInstance(__sandbox_uri3); });
CHECK_PRIMARY_CLUSTER([__sandbox_uri1, __sandbox_uri2], cluster)

//@<> addInstance on replica cluster
EXPECT_NO_THROWS(function() { replicacluster.addInstance(__sandbox_uri3, {recoveryMethod: "clone"}); });
CHECK_REPLICA_CLUSTER([__sandbox_uri4, __sandbox_uri3], cluster, replicacluster);

//@<> rejoinInstance on a replica cluster
session3 = mysql.getSession(__sandbox_uri3);
session3.runSql("STOP group_replication");
shell.connect(__sandbox_uri4);
EXPECT_NO_THROWS(function() { replicacluster.rejoinInstance(__sandbox_uri3); });
testutil.waitMemberState(__mysql_sandbox_port3, "ONLINE");
CHECK_REPLICA_CLUSTER([__sandbox_uri4, __sandbox_uri3], cluster, replicacluster);

//@<> rescan() on a replica cluster
// delete sb3 from the metadata so that rescan picks it up
shell.connect(__sandbox_uri1);
session.runSql("DELETE FROM mysql_innodb_cluster_metadata.instances WHERE instance_name LIKE ?", ["%:"+__mysql_sandbox_port3]);

EXPECT_NO_THROWS(function() { replicacluster.rescan({addInstances: "auto"}); });
shell.connect(__sandbox_uri4);
testutil.waitMemberState(__mysql_sandbox_port3, "ONLINE");
CHECK_REPLICA_CLUSTER([__sandbox_uri4, __sandbox_uri3], cluster, replicacluster);

//@<> resetRecoveryAccountsPassword() on a replica cluster
EXPECT_NO_THROWS(function() { replicacluster.resetRecoveryAccountsPassword(); });
CHECK_REPLICA_CLUSTER([__sandbox_uri4, __sandbox_uri3], cluster, replicacluster);

//@<> setInstanceOption() on a replica cluster
EXPECT_NO_THROWS(function() { replicacluster.setInstanceOption(__sandbox_uri3, "memberWeight", 25); });

//@<> setOption() on a replica cluster
EXPECT_NO_THROWS(function() { replicacluster.setOption("memberWeight", 50); });

//@<> setupAdminAccount() on a replica cluster
EXPECT_NO_THROWS(function() { replicacluster.setupAdminAccount("cadminreplica@'%'", {password:"boo"}); });

//@<> setupRouterAccount() on a replica cluster
EXPECT_NO_THROWS(function() { replicacluster.setupRouterAccount("routerreplica@'%'", {password:"boo"}); });

//@<> removeInstance on replica cluster
EXPECT_NO_THROWS(function() { replicacluster.removeInstance(__sandbox_uri3); });
CHECK_REPLICA_CLUSTER([__sandbox_uri4], cluster, replicacluster);

//@ removeCluster()
EXPECT_NO_THROWS(function() {clusterset.removeCluster("replicacluster"); });

//@<> validate remove cluster
CHECK_REMOVED_CLUSTER([__sandbox_uri4], cluster, "replicacluster");

//@<> dissolve cluster
shell.connect(__sandbox_uri4);
replicacluster = dba.getCluster();
EXPECT_NO_THROWS(function() {replicacluster.dissolve({force: true}); });
session.runSql("SET GLOBAL super_read_only=OFF");
dba.dropMetadataSchema({force: true});

//@<> createReplicaCluster() - clone recovery
EXPECT_NO_THROWS(function() {replicacluster = clusterset.createReplicaCluster(__sandbox_uri4, "replicacluster", {recoveryMethod: "clone"}); });

//@<> validate replica cluster - clone recovery
CHECK_REPLICA_CLUSTER([__sandbox_uri4], cluster, replicacluster);

//@<> Cleanup
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
testutil.destroySandbox(__mysql_sandbox_port3);
testutil.destroySandbox(__mysql_sandbox_port4);
