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

//@<> addInstance on primary cluster
EXPECT_NO_THROWS(function() { cluster.addInstance(__sandbox_uri2, {recoveryMethod: "incremental"}); });
EXPECT_NO_THROWS(function() { cluster.addInstance(__sandbox_uri3, {recoveryMethod: "clone"}); });
// Enable with WL#13815
//CHECK_PRIMARY_CLUSTER([__sandbox_uri1, __sandbox_uri2, __sandbox_uri3], cluster)

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
