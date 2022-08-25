// Tests for Group Replication's transaction size limit management.
// The tests cover the main features and changes for InnoDB Cluster
//
//  - New option 'transactionSizeLimit' in dba.createCluster()
//  - Option value stored in the Metadata schema
//  - Automatic settings management in addInstance()/rejoinInstance()
//  - New option changeable via cluster.setOption()
//  - Information about the new option in cluster.options()
//  - Information about the option value missing from the Metadata in .status()
//  - Information about mismatch of the value of the option of Cluster members
//    with the Cluster's option
//  - Fixing of option missing from metadata and mismatch in cluster.rescan()

//@<> Setup
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: hostname});
testutil.snapshotSandboxConf(__mysql_sandbox_port1);
testutil.deploySandbox(__mysql_sandbox_port2, "root", {report_host: hostname});
testutil.snapshotSandboxConf(__mysql_sandbox_port2);

shell.connect(__sandbox_uri1);

//@<> New option in createCluster(): transactionSizeLimit
var cluster;
var transaction_size_limit_set_cluster = 123456789;
EXPECT_NO_THROWS(function() { cluster = dba.createCluster("test", {gtidSetIsComplete: true, transactionSizeLimit: transaction_size_limit_set_cluster}) });

// The option should be set in the target instance
var transaction_size_limit = session.runSql("select @@group_replication_transaction_size_limit").fetchOne()[0];
EXPECT_EQ(transaction_size_limit_set_cluster, transaction_size_limit);

// The option should be saved in the Metadata too
var transaction_size_limit_md = parseInt(session.runSql("select attributes->>'$.opt_transactionSizeLimit' from mysql_innodb_cluster_metadata.clusters where cluster_name='test'").fetchOne()[0]);
EXPECT_EQ(transaction_size_limit_set_cluster, transaction_size_limit_md);

//@<> Option set in the Cluster should be automatically used in addInstance()
EXPECT_NO_THROWS(function() { cluster.addInstance(__sandbox_uri2); });

var session2 = mysql.getSession(__sandbox_uri2);

// The option should be set in the target instance
var transaction_size_limit = session2.runSql("select @@group_replication_transaction_size_limit").fetchOne()[0];

EXPECT_EQ(transaction_size_limit_set_cluster, transaction_size_limit);

//@<> Option set in the Cluster should be automatically used in rejoinInstance()
session2.runSql("SET GLOBAL group_replication_transaction_size_limit = 0");
session2.runSql("STOP group_replication");

EXPECT_NO_THROWS(function() { cluster.rejoinInstance(__sandbox_uri2); });

testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");

var transaction_size_limit = session2.runSql("select @@group_replication_transaction_size_limit").fetchOne()[0];

EXPECT_EQ(transaction_size_limit_set_cluster, transaction_size_limit);

//@<> Option must be changeable in the Cluster via setOption()
var new_transaction_size_limit_set_cluster = 123456;

EXPECT_NO_THROWS(function() { cluster.setOption("transactionSizeLimit", new_transaction_size_limit_set_cluster); });

// The option should be set in all cluster members
var transaction_size_limit = session.runSql("select @@group_replication_transaction_size_limit").fetchOne()[0];
EXPECT_EQ(new_transaction_size_limit_set_cluster, transaction_size_limit);

var transaction_size_limit = session2.runSql("select @@group_replication_transaction_size_limit").fetchOne()[0];
EXPECT_EQ(new_transaction_size_limit_set_cluster, transaction_size_limit);

// The option should be saved in the Metadata too
var transaction_size_limit_md = parseInt(session.runSql("select attributes->>'$.opt_transactionSizeLimit' from mysql_innodb_cluster_metadata.clusters where cluster_name='test'").fetchOne()[0]);
EXPECT_EQ(new_transaction_size_limit_set_cluster, transaction_size_limit_md);

//@<> Option must be included in cluster.options()
var global_options = cluster.options()["defaultReplicaSet"]["globalOptions"];
var found = false;

for (option of global_options) {
  if (option["option"] == "transactionSizeLimit") {
    EXPECT_EQ(new_transaction_size_limit_set_cluster, parseInt(option["value"]));
    found = true;
  }
}

EXPECT_TRUE(found, "transactionSizeLimit option not found");

//@<> Option missing from the metadata must be reported in .status()
session.runSql("UPDATE mysql_innodb_cluster_metadata.clusters SET attributes = JSON_REMOVE(attributes, '$.opt_transactionSizeLimit')");

var s = cluster.status();

EXPECT_EQ(["WARNING: Cluster's transaction size limit is not registered in the metadata. Use cluster.rescan() to update the metadata."], s["defaultReplicaSet"]["clusterErrors"]);

//@<> Option missing from the metadata must be fixed with .rescan()
EXPECT_NO_THROWS(function() { cluster.rescan(); });

EXPECT_OUTPUT_CONTAINS("Updating group_replication_transaction_size_limit in the Cluster's metadata...");

var transaction_size_limit_md = parseInt(session.runSql("select attributes->>'$.opt_transactionSizeLimit' from mysql_innodb_cluster_metadata.clusters where cluster_name='test'").fetchOne()[0]);
EXPECT_EQ(new_transaction_size_limit_set_cluster, transaction_size_limit_md);

EXPECT_EQ(undefined, cluster.status()["defaultReplicaSet"]["clusterErrors"]);

//@<> Mismatch of the value of the option of Cluster members must be reported in .status()
session2.runSql("SET GLOBAL group_replication_transaction_size_limit=12345678");

EXPECT_EQ(cluster.status()["defaultReplicaSet"]["topology"][__endpoint2]["instanceErrors"][0],
                "WARNING: The value of 'group_replication_transaction_size_limit' does not match the Cluster\'s configured value. Use Cluster.rescan() to repair.");

//@<> Mismatch of the value of the option of Cluster members must be fixed with .rescan()
EXPECT_NO_THROWS(function() { cluster.rescan(); });

EXPECT_OUTPUT_CONTAINS(`NOTE: 'group_replication_transaction_size_limit' value at '${__endpoint2}' does not match the Cluster's value: '123456'`);

EXPECT_OUTPUT_CONTAINS("Updating the member's value...");

EXPECT_EQ(undefined, cluster.status()["defaultReplicaSet"]["topology"][__endpoint2]["instanceErrors"]);

var transaction_size_limit = session2.runSql("select @@group_replication_transaction_size_limit").fetchOne()[0];
EXPECT_EQ(new_transaction_size_limit_set_cluster, transaction_size_limit);

//@<> If the option is not used, the value of the instance is kept and stored in the metadata equally
EXPECT_NO_THROWS(function() { cluster.dissolve(); });

// Reset to the default
session.runSql("SET GLOBAL group_replication_transaction_size_limit=DEFAULT");

var default_value = session.runSql("select @@group_replication_transaction_size_limit").fetchOne()[0];

EXPECT_NO_THROWS(function() { cluster = dba.createCluster("test", {gtidSetIsComplete: true}) });

// The option should be unchanged in the target instance
var transaction_size_limit = session.runSql("select @@group_replication_transaction_size_limit").fetchOne()[0];
EXPECT_EQ(default_value, transaction_size_limit);

// The option should be saved in the Metadata too
var transaction_size_limit_md = parseInt(session.runSql("select attributes->>'$.opt_transactionSizeLimit' from mysql_innodb_cluster_metadata.clusters where cluster_name='test'").fetchOne()[0]);

EXPECT_EQ(default_value, transaction_size_limit_md);

//@<> Rebooting a cluster from complete outage should keep the value that was stored in the metadata
var transaction_size_limit_reboot = 123123123;

EXPECT_NO_THROWS(function() { cluster.setOption("transactionSizeLimit", transaction_size_limit_reboot); });

testutil.killSandbox(__mysql_sandbox_port1);
testutil.startSandbox(__mysql_sandbox_port1);

shell.connect(__sandbox_uri1);
session.runSql("SET GLOBAL group_replication_transaction_size_limit=DEFAULT");

EXPECT_NO_THROWS(function() { cluster = dba.rebootClusterFromCompleteOutage(); });

// The option should be kept as it was set in the metadata
var transaction_size_limit_md = parseInt(session.runSql("select attributes->>'$.opt_transactionSizeLimit' from mysql_innodb_cluster_metadata.clusters where cluster_name='test'").fetchOne()[0]);
EXPECT_EQ(transaction_size_limit_reboot, transaction_size_limit_md);

// The option should be kept too in the instance
var transaction_size_limit = session.runSql("select @@group_replication_transaction_size_limit").fetchOne()[0];
EXPECT_EQ(transaction_size_limit_reboot, transaction_size_limit);

//@<> Cleanup
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);

