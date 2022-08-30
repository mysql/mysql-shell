// Tests for Group Replication's transaction size limit management.
// The tests cover the main features and changes for InnoDB ClusterSet
//
//  - New option 'transactionSizeLimit' in clusterset.createReplicaCluster()
//  - Automatic handling of the option value in Replica Cluster: set to maximum
//    by default
//  - Automatic restore of the option's original value in
//    ClusterSet.setPrimaryInstance(), ClusterSet.forcePrimaryCluster() and
//    ClusterSet.removeCluster()

//@ {VER(>=8.0.27)}

//@<> Setup
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: hostname});
testutil.deploySandbox(__mysql_sandbox_port2, "root", {report_host: hostname});
testutil.deploySandbox(__mysql_sandbox_port3, "root", {report_host: hostname});
testutil.deploySandbox(__mysql_sandbox_port4, "root", {report_host: hostname});

shell.connect(__sandbox_uri1);

var transaction_size_limit_set_cluster = 123456789;

EXPECT_NO_THROWS(function() { cluster = dba.createCluster("primary_cluster", {gtidSetIsComplete: true, transactionSizeLimit: transaction_size_limit_set_cluster}) });

EXPECT_NO_THROWS(function() { cluster.addInstance(__sandbox_uri2); });

var clusterset;
EXPECT_NO_THROWS(function() { clusterset = cluster.createClusterSet("cs"); });

//@<> create a Replica Cluster
var replica_cluster;
EXPECT_NO_THROWS(function() { replica_cluster = clusterset.createReplicaCluster(__sandbox_uri3, "replica_cluster"); });

var session3 = mysql.getSession(__sandbox_uri3);

// The transaction_size_limit must be set to the maximum in the replica cluster
var transaction_size_limit = session3.runSql("select @@group_replication_transaction_size_limit").fetchOne()[0];
EXPECT_EQ(0, transaction_size_limit);

// transaction_size_limit must be saved in the Metadata and its inherited from the Primary Cluster
var transaction_size_limit_md = parseInt(session.runSql("select attributes->>'$.opt_transactionSizeLimit' from mysql_innodb_cluster_metadata.clusters where cluster_name='replica_cluster'").fetchOne()[0]);
EXPECT_EQ(transaction_size_limit_set_cluster, transaction_size_limit_md);

EXPECT_EQ(undefined, replica_cluster.status()["defaultReplicaSet"]["topology"][__endpoint3]["instanceErrors"]);

//@<> Change transactionSizeLimit with .setOption() must be forbidden in Replica Clusters
EXPECT_THROWS_TYPE(function() { replica_cluster.setOption("transactionSizeLimit", 123456789) }, "Option 'transactionSizeLimit' not supported on Replica Clusters", "RuntimeError");

//@<> Switchovers must ensure Replica Clusters have the maximum value set for transaction_size_limit
EXPECT_NO_THROWS(function() { clusterset.setPrimaryCluster("replica_cluster"); });

// The transaction_size_limit must remain unchanged in the cluster that was promoted
var transaction_size_limit = session3.runSql("select @@group_replication_transaction_size_limit").fetchOne()[0];
EXPECT_EQ(transaction_size_limit_set_cluster, transaction_size_limit);

// The transaction_size_limit must be set to the maximum in the demoted cluster
var transaction_size_limit = session.runSql("select @@group_replication_transaction_size_limit").fetchOne()[0];
EXPECT_EQ(0, transaction_size_limit);

//@<> Change transactionSizeLimit on the Primary Cluster
var transaction_size_limit_set_option = 200000000;
EXPECT_NO_THROWS(function() { replica_cluster.setOption("transactionSizeLimit", transaction_size_limit_set_option) });

// The value must be changed in the Cluster and the metadata for all ClusterSet clusters
var transaction_size_limit = session3.runSql("select @@group_replication_transaction_size_limit").fetchOne()[0];
EXPECT_EQ(transaction_size_limit_set_option, transaction_size_limit);

var transaction_size_limit_md = parseInt(session3.runSql("select attributes->>'$.opt_transactionSizeLimit' from mysql_innodb_cluster_metadata.clusters where cluster_name='replica_cluster'").fetchOne()[0]);
EXPECT_EQ(transaction_size_limit_set_option, transaction_size_limit_md);

transaction_size_limit_md = parseInt(session3.runSql("select attributes->>'$.opt_transactionSizeLimit' from mysql_innodb_cluster_metadata.clusters where cluster_name='primary_cluster'").fetchOne()[0]);
EXPECT_EQ(transaction_size_limit_set_option, transaction_size_limit_md);

//@<> Switchovers must ensure Replica Clusters have the value restored
EXPECT_NO_THROWS(function() { clusterset.setPrimaryCluster("primary_cluster"); });

var transaction_size_limit = session.runSql("select @@group_replication_transaction_size_limit").fetchOne()[0];
EXPECT_EQ(transaction_size_limit_set_option, transaction_size_limit);

// The transaction_size_limit must be set to the maximum in the demoted cluster
var transaction_size_limit = session3.runSql("select @@group_replication_transaction_size_limit").fetchOne()[0];
EXPECT_EQ(0, transaction_size_limit);


//@<> removeCluster must ensure the removed Cluster gets the value restored
EXPECT_NO_THROWS(function() { clusterset.setPrimaryCluster("replica_cluster"); });

EXPECT_NO_THROWS(function() { clusterset.removeCluster("primary_cluster"); });

var transaction_size_limit = session.runSql("select @@group_replication_transaction_size_limit").fetchOne()[0];
EXPECT_EQ(transaction_size_limit_set_option, transaction_size_limit);

var session2 = mysql.getSession(__sandbox_uri2);

transaction_size_limit = session2.runSql("select @@group_replication_transaction_size_limit").fetchOne()[0];
EXPECT_EQ(transaction_size_limit_set_option, transaction_size_limit);

//@<> Replica Clusters get the value set to the maximum by default
var original_transaction_size_limit = session.runSql("select @@group_replication_transaction_size_limit").fetchOne()[0];

shell.connect(__sandbox_uri3);
clusterset = dba.getClusterSet();

EXPECT_NO_THROWS(function() { replica_cluster = clusterset.createReplicaCluster(__sandbox_uri1, "new_replica_cluster"); });

EXPECT_NO_THROWS(function() { replica_cluster.addInstance(__sandbox_uri2, {recoveryMethod: "clone"}); });

var session1 = mysql.getSession(__sandbox_uri1);
var session2 = mysql.getSession(__sandbox_uri2);

// The value stored in the metadata must be the original one
var transaction_size_limit_md = parseInt(session1.runSql("select attributes->>'$.opt_transactionSizeLimit' from mysql_innodb_cluster_metadata.clusters where cluster_name='new_replica_cluster'").fetchOne()[0]);
EXPECT_EQ(original_transaction_size_limit, transaction_size_limit_md);

// The value in use by every instance must be zero (maximum)
transaction_size_limit = session1.runSql("select @@group_replication_transaction_size_limit").fetchOne()[0];
EXPECT_EQ(0, transaction_size_limit);

transaction_size_limit = session2.runSql("select @@group_replication_transaction_size_limit").fetchOne()[0];
EXPECT_EQ(0, transaction_size_limit);

EXPECT_EQ(undefined, replica_cluster.status()["defaultReplicaSet"]["topology"][__endpoint1]["instanceErrors"]);
EXPECT_EQ(undefined, replica_cluster.status()["defaultReplicaSet"]["topology"][__endpoint2]["instanceErrors"]);

//@<> Failover must ensure the elected Cluster has the value restored

// Add another replica cluster
EXPECT_NO_THROWS(function() { clusterset.createReplicaCluster(__sandbox_uri4, "another_replica_cluster"); });

testutil.killSandbox(__mysql_sandbox_port3);

shell.connect(__sandbox_uri1);
clusterset = dba.getClusterSet();

EXPECT_NO_THROWS(function() { clusterset.forcePrimaryCluster("new_replica_cluster"); });

// The value in use by every instance must be the original one
transaction_size_limit = session1.runSql("select @@group_replication_transaction_size_limit").fetchOne()[0];
EXPECT_EQ(original_transaction_size_limit, transaction_size_limit);

transaction_size_limit = session2.runSql("select @@group_replication_transaction_size_limit").fetchOne()[0];
EXPECT_EQ(original_transaction_size_limit, transaction_size_limit);

// The remaining Replica Clusters must be using the maximum value
shell.connect(__sandbox_uri4);
replica_cluster2 = dba.getCluster();

transaction_size_limit = session.runSql("select @@group_replication_transaction_size_limit").fetchOne()[0];
EXPECT_EQ(0, transaction_size_limit);

EXPECT_EQ(undefined, replica_cluster2.status()["defaultReplicaSet"]["topology"][__endpoint4]["instanceErrors"]);

//@<> rejoinCluster() must restore the value of transaction_size_limit to zero
testutil.startSandbox(__mysql_sandbox_port3);
shell.connect(__sandbox_uri3);

EXPECT_NO_THROWS(function() { rebooted_replica = dba.rebootClusterFromCompleteOutage(); });

EXPECT_NO_THROWS(function() { clusterset.rejoinCluster("replica_cluster") });

transaction_size_limit = session.runSql("select @@group_replication_transaction_size_limit").fetchOne()[0];
EXPECT_EQ(0, transaction_size_limit);

//@<> removeCluster must ensure the removed Cluster gets the value restored even if GR is stopped
session.runSql("STOP group_replication");

EXPECT_NO_THROWS(function() { clusterset.removeCluster("replica_cluster", {force: true}); });

var transaction_size_limit = session.runSql("select @@group_replication_transaction_size_limit").fetchOne()[0];
EXPECT_EQ(transaction_size_limit_set_option, transaction_size_limit);

//@<> Cleanup
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
testutil.destroySandbox(__mysql_sandbox_port3);
testutil.destroySandbox(__mysql_sandbox_port4);
