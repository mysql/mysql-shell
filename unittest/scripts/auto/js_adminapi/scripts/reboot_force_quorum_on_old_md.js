//@<> INCLUDE metadata_schema_utils.inc

//@<> Initialization and supporting code
metadata_1_0_1_file = "metadata_1_0_1.sql";

testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: hostname});
dba.configureInstance(__sandbox_uri1, {clusterAdmin: 'tst_admin', clusterAdminPassword: 'tst_pwd'});
testutil.snapshotSandboxConf(__mysql_sandbox_port1)
testutil.deploySandbox(__mysql_sandbox_port2, "root", {report_host: hostname});
dba.configureInstance(__sandbox_uri2, {clusterAdmin: 'tst_admin', clusterAdminPassword: 'tst_pwd'});
testutil.snapshotSandboxConf(__mysql_sandbox_port2)
cluster_admin_uri= "mysql://tst_admin:tst_pwd@" + __host + ":" + __mysql_sandbox_port1;


// Setup cluster and get required data to use on the 1.0.1 metadata
shell.connect(__sandbox_uri1)
var cluster = dba.createCluster('sample');
cluster.addInstance(__sandbox_uri2, {recoveryMethod:'incremental'});
var server_uuid1 = session.runSql("SELECT @@server_uuid").fetchOne()[0];
var group_name = session.runSql("SELECT @@group_replication_group_name").fetchOne()[0];

shell.connect(__sandbox_uri2)
var server_uuid2 = session.runSql("SELECT @@server_uuid").fetchOne()[0];

// Replace the metadata with one of version 1.0.1
prepare_1_0_1_metadata_from_template(metadata_1_0_1_file, group_name, [server_uuid1,server_uuid2]);
shell.connect(__sandbox_uri1)
load_metadata(__sandbox_uri1, metadata_1_0_1_file);
session.runSql("DELETE FROM mysql_innodb_cluster_metadata.routers");

// Simulate Total Lost
testutil.stopSandbox(__mysql_sandbox_port2, {wait:1})
testutil.stopSandbox(__mysql_sandbox_port1, {wait:1})

testutil.startSandbox(__mysql_sandbox_port1)
testutil.startSandbox(__mysql_sandbox_port2)

//@ Testing upgrade metadata on total lost
shell.connect(__sandbox_uri1);
dba.upgradeMetadata()

//@ Testing rebootClusterFromCompleteOutage
var cluster = dba.rebootClusterFromCompleteOutage()
var state = testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");
cluster.status()

//@ Testing upgrade metadata on rebooted cluster
dba.upgradeMetadata()

// Go back to 1.0.1 Metadata and Simulate No Quorum
load_metadata(__sandbox_uri1, metadata_1_0_1_file);
testutil.killSandbox(__mysql_sandbox_port3);
var state = testutil.waitMemberState(__mysql_sandbox_port3, "UNREACHABLE,(MISSING)");
testutil.killSandbox(__mysql_sandbox_port2);
var state = testutil.waitMemberState(__mysql_sandbox_port2, "UNREACHABLE,(MISSING)");


//@ Testing upgrade metadata with no quorum
dba.upgradeMetadata()

//@ Getting cluster with no quorum
var cluster = dba.getCluster()
cluster.forceQuorumUsingPartitionOf(__sandbox_uri1);

//@ Getting cluster with quorum
var cluster = dba.getCluster()

//@ Metadata continues failing...
dba.upgradeMetadata()

session.close();
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
testutil.rmfile(metadata_1_0_1_file);
