//@ {VER(>=8.0.11)}

//@<> INCLUDE metadata_schema_utils.inc

//@<> Initialization and supporting code
metadata_1_0_1_file = "metadata_1_0_1.sql";

//@<> Configures custom user on the instance
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: hostname});
dba.configureInstance(__sandbox_uri1, {clusterAdmin: 'tst_admin', clusterAdminPassword: 'tst_pwd'});
testutil.snapshotSandboxConf(__mysql_sandbox_port1)
testutil.deploySandbox(__mysql_sandbox_port2, "root", {report_host: hostname});
dba.configureInstance(__sandbox_uri2, {clusterAdmin: 'tst_admin', clusterAdminPassword: 'tst_pwd'});
testutil.snapshotSandboxConf(__mysql_sandbox_port2)


//@<> Creates the sample cluster
shell.connect(__sandbox_uri1);
var cluster = dba.createCluster('sample');
cluster.addInstance(__sandbox_uri2, {recoveryMethod:'incremental'});
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");
cluster.switchToMultiPrimaryMode()
var server_uuid1 = session.runSql("SELECT @@server_uuid").fetchOne()[0];
var server_id1 = session.runSql("SELECT @@server_id").fetchOne()[0];
var group_name = session.runSql("SELECT @@group_replication_group_name").fetchOne()[0];
shell.connect(__sandbox_uri2)
var server_uuid2 = session.runSql("SELECT @@server_uuid").fetchOne()[0];
var server_id2 = session.runSql("SELECT @@server_id").fetchOne()[0];
shell.connect(__sandbox_uri1)

var current_version = testutil.getCurrentMetadataVersion();
var version = current_version.split('.');

var major = parseInt(version[0]);
var minor = parseInt(version[1]);
var patch = parseInt(version[2]);

prepare_1_0_1_metadata_from_template(metadata_1_0_1_file, group_name, [[server_uuid1, server_id1], [server_uuid2, server_id2]],"mm");

EXPECT_STDERR_EMPTY();

//@<> Tests upgrade works from a multiprimary cluster
load_metadata(__sandbox_uri1, metadata_1_0_1_file);
session.runSql("delete from mysql_innodb_cluster_metadata.routers")
EXPECT_NO_THROWS(function () { dba.upgradeMetadata() });

//@<> Cleanup
session.close();
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
testutil.rmfile(metadata_1_0_1_file);
