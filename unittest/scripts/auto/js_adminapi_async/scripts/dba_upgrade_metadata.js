//@ {VER(>=8.0.11)}

//@<> Configures custom user on the instance
// Use ANSI_QUOTES and other special modes in sql_mode. The Upgrade should not fail. (BUG#31428813)
var sql_mode = "ANSI_QUOTES,NO_AUTO_VALUE_ON_ZERO,NO_BACKSLASH_ESCAPES,NO_UNSIGNED_SUBTRACTION,PIPES_AS_CONCAT,IGNORE_SPACE,STRICT_TRANS_TABLES,STRICT_ALL_TABLES,NO_ZERO_IN_DATE,NO_ZERO_DATE,ERROR_FOR_DIVISION_BY_ZERO,TRADITIONAL,NO_ENGINE_SUBSTITUTION";
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: hostname, sql_mode: sql_mode});
dba.configureInstance(__sandbox_uri1, {clusterAdmin: 'tst_admin', clusterAdminPassword: 'tst_pwd'});
testutil.snapshotSandboxConf(__mysql_sandbox_port1)
testutil.deploySandbox(__mysql_sandbox_port2, "root", {report_host: hostname, sql_mode: sql_mode});
dba.configureInstance(__sandbox_uri2, {clusterAdmin: 'tst_admin', clusterAdminPassword: 'tst_pwd'});
testutil.snapshotSandboxConf(__mysql_sandbox_port2)
cluster_admin_uri= "mysql://tst_admin:tst_pwd@" + __host + ":" + __mysql_sandbox_port1;

//@<> upgradeMetadata without connection
EXPECT_THROWS(function(){dba.upgradeMetadata()}, "An open session is required to perform this operation")

// Session to be used through all the AAPI calls
shell.connect(__sandbox_uri2)
var server_uuid2 = session.runSql("SELECT @@server_uuid").fetchOne()[0];

//@<> upgradeMetadata on a standalone instance
shell.connect(__sandbox_uri1)
var server_uuid1 = session.runSql("SELECT @@server_uuid").fetchOne()[0];
EXPECT_THROWS(function(){dba.upgradeMetadata()}, "Metadata Schema not found.")
EXPECT_OUTPUT_CONTAINS("Command not available on an unmanaged standalone instance.");

//@<> Creates the sample cluster
var rset = dba.createReplicaSet("myrs", {gtidSetIsComplete:true});
rset.addInstance(__sandbox_uri2);
EXPECT_STDERR_EMPTY()

shell.options.useWizards = true;

//@<> Upgrades the metadata, up to date
dba.upgradeMetadata()
EXPECT_OUTPUT_CONTAINS(`NOTE: Installed metadata at '${hostname}:${__mysql_sandbox_port1}' is up to date (version 2.3.0).`);

//@<> Upgrades the metadata from slave, up to date
shell.connect(__sandbox_uri2)
dba.upgradeMetadata()
EXPECT_OUTPUT_CONTAINS(`NOTE: Installed metadata at '${hostname}:${__mysql_sandbox_port1}' is up to date (version 2.3.0).`);

// Dissolve
rset.dissolve({force: 1});

ports=[__mysql_sandbox_port1]
metadata_2_1_0_file = "metadata_2_1_0.sql";
primary_port = __mysql_sandbox_port1

//@<> INCLUDE metadata_schema_utils.inc
//@<> INCLUDE _prepare_metadata_2_1_0_replicaset.js

//@<> Upgrading a ReplicaSet metadata to 2.2.0 must set the correct instance_type {VER(>=8.1.0)}
shell.connect(__sandbox_uri1)
dba.upgradeMetadata()

var instance_type = session.runSql("SELECT instance_type from mysql_innodb_cluster_metadata.instances where instance_id = 1").fetchOne()[0];

EXPECT_EQ("async-member", instance_type);

//@<> Cleanup
session.close();
testutil.destroySandbox(__mysql_sandbox_port1)
testutil.destroySandbox(__mysql_sandbox_port2)
