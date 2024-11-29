function parseBoolOrInt(val)
{
    if (typeof val === 'string' && (val.toLowerCase() === 'true'))
        return 1;
    if (typeof val === 'string' && (val.toLowerCase() === 'false'))
        return 0;
    return parseInt(val);
}

//@<> INCLUDE metadata_schema_utils.inc

//@<> Initialization and supporting code
metadata_1_0_1_file = "metadata_1_0_1.sql";

//@<> Configures custom user on the instance
// Use ANSI_QUOTES and other special modes in sql_mode. The Upgrade should not fail. (BUG#31428813)
var sql_mode = "ANSI_QUOTES,NO_AUTO_VALUE_ON_ZERO,NO_BACKSLASH_ESCAPES,NO_UNSIGNED_SUBTRACTION,PIPES_AS_CONCAT,IGNORE_SPACE,STRICT_TRANS_TABLES,STRICT_ALL_TABLES,NO_ZERO_IN_DATE,NO_ZERO_DATE,ERROR_FOR_DIVISION_BY_ZERO,TRADITIONAL,NO_ENGINE_SUBSTITUTION";
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: hostname, sql_mode: sql_mode});
dba.configureInstance(__sandbox_uri1, {clusterAdmin: 'tst_admin', clusterAdminPassword: 'tst_pwd'});
testutil.snapshotSandboxConf(__mysql_sandbox_port1)
testutil.deploySandbox(__mysql_sandbox_port2, "root", {report_host: hostname, sql_mode: sql_mode});
dba.configureInstance(__sandbox_uri2, {clusterAdmin: 'tst_admin', clusterAdminPassword: 'tst_pwd'});
testutil.snapshotSandboxConf(__mysql_sandbox_port2)
testutil.deploySandbox(__mysql_sandbox_port3, "root", {report_host: hostname, sql_mode: sql_mode});
dba.configureInstance(__sandbox_uri3, {clusterAdmin: 'tst_admin', clusterAdminPassword: 'tst_pwd'});
testutil.snapshotSandboxConf(__mysql_sandbox_port3)
cluster_admin_uri= "mysql://tst_admin:tst_pwd@" + __host + ":" + __mysql_sandbox_port1;

//@<> upgradeMetadata without connection
EXPECT_THROWS(function(){dba.upgradeMetadata()}, "An open session is required to perform this operation")

// Session to be used through all the API calls
shell.connect(__sandbox_uri2)
var server_uuid2 = session.runSql("SELECT @@server_uuid").fetchOne()[0];
var server_id2 = session.runSql("SELECT @@server_id").fetchOne()[0];
shell.connect(__sandbox_uri3)
var server_uuid3 = session.runSql("SELECT @@server_uuid").fetchOne()[0];
var server_id3 = session.runSql("SELECT @@server_id").fetchOne()[0];

//@<> upgradeMetadata on a standalone instance
shell.connect(__sandbox_uri1)
var server_uuid1 = session.runSql("SELECT @@server_uuid").fetchOne()[0];
var server_id1 = session.runSql("SELECT @@server_id").fetchOne()[0];
EXPECT_THROWS(function(){dba.upgradeMetadata()}, "Metadata Schema not found.");
EXPECT_OUTPUT_CONTAINS("Command not available on an unmanaged standalone instance.");

//@<> Creates the sample cluster
shell.connect(cluster_admin_uri);
var cluster = dba.createCluster('sample');
cluster.addInstance(__sandbox_uri2, {recoveryMethod:'incremental'});
cluster.addInstance(__sandbox_uri3, {recoveryMethod:'incremental'});

shell.connect(__sandbox_uri1);
var group_name = session.runSql("SELECT @@group_replication_group_name").fetchOne()[0];

var current_version = testutil.getCurrentMetadataVersion();
var version = current_version.split('.');

var major = parseInt(version[0]);
var minor = parseInt(version[1]);
var patch = parseInt(version[2]);

prepare_1_0_1_metadata_from_template(metadata_1_0_1_file, group_name, [[server_uuid1, server_id1], [server_uuid2, server_id2], [server_uuid3, server_id3]]);

//@<> upgradeMetadata, installed version is greater than current version
set_metadata_version(major, minor, patch + 1)
var installed_version = testutil.getInstalledMetadataVersion();
EXPECT_THROWS(function () { dba.upgradeMetadata() }, "Metadata version is not compatible");
EXPECT_OUTPUT_CONTAINS(`Incompatible Metadata version. No operations are allowed on a Metadata version higher than Shell supports ('${current_version}'), the installed Metadata version '${installed_version}' is not supported. Please upgrade MySQL Shell.`);

//@<> upgradeMetadata, installed version is unknown
set_metadata_version(major, minor, -1)
var installed_version = testutil.getInstalledMetadataVersion();
EXPECT_THROWS(function () { dba.upgradeMetadata() }, `Installed metadata at '${hostname}:${__mysql_sandbox_port1}' has an unknown version (2.3.-1), upgrading this version of the metadata is not supported.`);

//@<> Downgrading to version 1.0.1 for the following tests
other_session = mysql.getSession(__sandbox_uri1);
load_metadata(__sandbox_uri1, metadata_1_0_1_file);
var installed_before = testutil.getInstalledMetadataVersion();

//@<> upgradeMetadata, not the right credentials
session.runSql("CREATE USER myguest@'localhost' IDENTIFIED BY 'mypassword'");
session.runSql("GRANT SELECT on *.* to myguest@'localhost'");
shell.connect("myguest:mypassword@localhost:" + __mysql_sandbox_port1);
EXPECT_THROWS(function () { dba.upgradeMetadata() }, "The account 'myguest'@'localhost' is missing privileges required for this operation.");

//@ upgradeMetadata, dryRrun upgrade required
shell.connect(__sandbox_uri1)
var installed_version = testutil.getInstalledMetadataVersion();
EXPECT_NO_THROWS(function () { dba.upgradeMetadata({dryRun: true}) })

//@<> WL13386-TSET_1 Trying upgrade but a mysql_innodb_cluster_metadata_previous schema exists
load_metadata(__sandbox_uri1, metadata_1_0_1_file);
session.runSql("delete from mysql_innodb_cluster_metadata.routers")
session.runSql("CREATE SCHEMA mysql_innodb_cluster_metadata_previous");
EXPECT_THROWS(function () { dba.upgradeMetadata() }, "Unable to create the step backup because a schema named 'mysql_innodb_cluster_metadata_previous' already exists.");
session.runSql("DROP SCHEMA mysql_innodb_cluster_metadata_previous");

//@ Upgrades the metadata, no registered routers
shell.options.useWizards=1;

// Gets the cluster while MD is 1.0.1
var c = dba.getCluster();
dba.upgradeMetadata();

//@ Upgrades the metadata, up to date
dba.upgradeMetadata();

shell.options.useWizards=0;

//@<> Tests accessing the cluster after the metadata migration
c.status();
c.describe();
c.options();
EXPECT_STDERR_EMPTY();

//@<> Tests accessing the cluster after dropping the metadata after an upgrade
dba.dropMetadataSchema({force:true});

EXPECT_THROWS(function(){ c.status(); }, "Metadata Schema not found.");
EXPECT_OUTPUT_CONTAINS("Command not available on an unmanaged standalone instance.");

//@ Upgrades the metadata from a slave instance
load_metadata(__sandbox_uri1, metadata_1_0_1_file);
session.runSql("delete from mysql_innodb_cluster_metadata.routers")
shell.connect(__sandbox_uri2);
testutil.waitMemberTransactions(__mysql_sandbox_port2, __mysql_sandbox_port1);
dba.upgradeMetadata()

//@<> WL13386-TSFR1_4 Upgrades the metadata, interactive off, error
shell.connect(__sandbox_uri1);
load_metadata(__sandbox_uri1, metadata_1_0_1_file);
EXPECT_THROWS(function () { dba.upgradeMetadata(); }, "Outdated Routers found. Please upgrade the Routers before upgrading the Metadata schema");

//@ WL13386-TSFR1_5 Upgrades the metadata, upgrade done by unregistering 10 routers and no router accounts
session.runSql("INSERT INTO mysql_innodb_cluster_metadata.routers VALUES (2, 'second', 2, NULL)")
session.runSql("INSERT INTO mysql_innodb_cluster_metadata.routers VALUES (3, 'third', 2, NULL)")
session.runSql("INSERT INTO mysql_innodb_cluster_metadata.routers VALUES (4, 'fourth', 2, NULL)")
session.runSql("INSERT INTO mysql_innodb_cluster_metadata.routers VALUES (5, 'fifth', 2, NULL)")
session.runSql("INSERT INTO mysql_innodb_cluster_metadata.routers VALUES (6, 'sixth', 2, NULL)")
session.runSql("INSERT INTO mysql_innodb_cluster_metadata.routers VALUES (7, 'seventh', 2, NULL)")
session.runSql("INSERT INTO mysql_innodb_cluster_metadata.routers VALUES (8, 'eighth', 2, NULL)")
session.runSql("INSERT INTO mysql_innodb_cluster_metadata.routers VALUES (9, 'nineth', 2, NULL)")
session.runSql("INSERT INTO mysql_innodb_cluster_metadata.routers VALUES (10, 'tenth', 2, NULL)")

// Chooses to unregister the existing routers
shell.options.useWizards=1;
testutil.expectPrompt("Please select an option: ", "2")
testutil.expectPrompt("Unregistering a Router implies it will not be used in the Cluster, do you want to continue? [y/N]:", "y")
dba.upgradeMetadata();
shell.options.useWizards=0;

//@ WL13386-TSFR1_5 Upgrades the metadata, upgrade done by unregistering more than 10 routers with router accounts
// Fake router account to get the account upgrade tested
load_metadata(__sandbox_uri1, metadata_1_0_1_file);
session.runSql("CREATE USER mysql_router_test@`%` IDENTIFIED BY 'whatever'")
session.runSql("CREATE USER mysql_router1_bc0e9n9dnfzk@`%` IDENTIFIED BY 'whatever'")
session.runSql("INSERT INTO mysql_innodb_cluster_metadata.routers VALUES (2, 'second', 2, NULL)")
session.runSql("INSERT INTO mysql_innodb_cluster_metadata.routers VALUES (3, 'third', 2, NULL)")
session.runSql("INSERT INTO mysql_innodb_cluster_metadata.routers VALUES (4, 'fourth', 2, NULL)")
session.runSql("INSERT INTO mysql_innodb_cluster_metadata.routers VALUES (5, 'fifth', 2, NULL)")
session.runSql("INSERT INTO mysql_innodb_cluster_metadata.routers VALUES (6, 'sixth', 2, NULL)")
session.runSql("INSERT INTO mysql_innodb_cluster_metadata.routers VALUES (7, 'seventh', 2, NULL)")
session.runSql("INSERT INTO mysql_innodb_cluster_metadata.routers VALUES (8, 'eighth', 2, NULL)")
session.runSql("INSERT INTO mysql_innodb_cluster_metadata.routers VALUES (9, 'nineth', 2, NULL)")
session.runSql("INSERT INTO mysql_innodb_cluster_metadata.routers VALUES (10, 'tenth', 2, NULL)")
session.runSql("INSERT INTO mysql_innodb_cluster_metadata.routers VALUES (11, 'eleventh', 2, NULL)")

// Aborts first attempt, accounts remain updated
shell.options.useWizards=1;
testutil.expectPrompt("Please select an option: ", "3")
EXPECT_NO_THROWS(function () { dba.upgradeMetadata(); });
shell.options.useWizards=0;

//@<> WL13386-TSFR2_1 Account Verification
// Verifying grants for mysql_router_test
session.runSql("SHOW GRANTS FOR mysql_router_test@`%`")
EXPECT_STDOUT_CONTAINS('GRANT USAGE ON *.*')
EXPECT_STDOUT_CONTAINS('GRANT SELECT, EXECUTE ON "mysql_innodb_cluster_metadata".*')
EXPECT_STDOUT_CONTAINS('GRANT INSERT, UPDATE, DELETE ON "mysql_innodb_cluster_metadata"."routers"')
EXPECT_STDOUT_CONTAINS('GRANT INSERT, UPDATE, DELETE ON "mysql_innodb_cluster_metadata"."v2_routers"')
EXPECT_STDOUT_CONTAINS('GRANT SELECT ON "performance_schema"."global_variables"')
EXPECT_STDOUT_CONTAINS('GRANT SELECT ON "performance_schema"."replication_group_member_stats"')
EXPECT_STDOUT_CONTAINS('GRANT SELECT ON "performance_schema"."replication_group_members"')

// Verifying grants for mysql_router1_bc0e9n9dnfzk
session.runSql('SHOW GRANTS FOR mysql_router1_bc0e9n9dnfzk@"%"')
EXPECT_STDOUT_CONTAINS('GRANT USAGE ON *.*')
EXPECT_STDOUT_CONTAINS('GRANT SELECT, EXECUTE ON "mysql_innodb_cluster_metadata".*')
EXPECT_STDOUT_CONTAINS('GRANT INSERT, UPDATE, DELETE ON "mysql_innodb_cluster_metadata"."routers"')
EXPECT_STDOUT_CONTAINS('GRANT INSERT, UPDATE, DELETE ON "mysql_innodb_cluster_metadata"."v2_routers"')
EXPECT_STDOUT_CONTAINS('GRANT SELECT ON "performance_schema"."global_variables"')
EXPECT_STDOUT_CONTAINS('GRANT SELECT ON "performance_schema"."replication_group_member_stats"')
EXPECT_STDOUT_CONTAINS('GRANT SELECT ON "performance_schema"."replication_group_members"')

//@ Second upgrade attempt should succeed
shell.options.useWizards=1;

testutil.expectPrompt("Please select an option: ", "2")
testutil.expectPrompt("Unregistering a Router implies it will not be used in the Cluster, do you want to continue? [y/N]:", "y")
dba.upgradeMetadata();
EXPECT_STDERR_NOT_CONTAINS("Table 'v2_routers' already exists");

shell.options.useWizards=0;

//@<> Account verification with no outdated routers dry run
shell.options.useWizards=1;

load_metadata(__sandbox_uri1, metadata_1_0_1_file);
session.runSql("delete from mysql_innodb_cluster_metadata.routers")
session.runSql("CREATE USER mysql_router2_bc0e9n9dnfzk@`%` IDENTIFIED BY 'whatever'")
dba.upgradeMetadata({dryRun:true});
EXPECT_STDOUT_CONTAINS("3 Router accounts need to be updated.")

shell.options.useWizards=0;

//@<> Account verification with no outdated routers
shell.options.useWizards=1;

dba.upgradeMetadata();
session.runSql("SHOW GRANTS FOR mysql_router2_bc0e9n9dnfzk@`%`")
EXPECT_STDOUT_CONTAINS("Updating Router accounts...")
EXPECT_STDOUT_CONTAINS("NOTE: 3 Router accounts have been updated.")
EXPECT_STDOUT_CONTAINS('GRANT USAGE ON *.*')
EXPECT_STDOUT_CONTAINS('GRANT SELECT, EXECUTE ON "mysql_innodb_cluster_metadata".*')
EXPECT_STDOUT_CONTAINS('GRANT INSERT, UPDATE, DELETE ON "mysql_innodb_cluster_metadata"."routers"')
EXPECT_STDOUT_CONTAINS('GRANT INSERT, UPDATE, DELETE ON "mysql_innodb_cluster_metadata"."v2_routers"')
EXPECT_STDOUT_CONTAINS('GRANT SELECT ON "performance_schema"."global_variables"')
EXPECT_STDOUT_CONTAINS('GRANT SELECT ON "performance_schema"."replication_group_member_stats"')
EXPECT_STDOUT_CONTAINS('GRANT SELECT ON "performance_schema"."replication_group_members"')

shell.options.useWizards=0;

//@ Test Migration from 1.0.1 to 2.3.0
load_metadata(__sandbox_uri1, metadata_1_0_1_file);
test_session = mysql.getSession(__sandbox_uri1)

// Chooses to unregister the existing router
shell.options.useWizards=1;

testutil.expectPrompt("Please select an option: ", "2")
testutil.expectPrompt("Unregistering a Router implies it will not be used in the Cluster, do you want to continue? [y/N]:", "y")
dba.upgradeMetadata();

shell.options.useWizards=0;

//@<> WL13386-TSNFR1_14 Test Migration from 1.0.1 to 2.3.0, Data Verification
load_metadata(__sandbox_uri1, metadata_1_0_1_file);

// Marks the router as up to date, we are interested in all data migration verification
test_session.runSql("UPDATE mysql_innodb_cluster_metadata.routers SET attributes=JSON_OBJECT('version','8.0.19', 'ROEndpoint', 'mysqlro.sock', 'RWEndpoint', '6446', 'ROXEndpoint', 'mysqlrox.sock', 'RWXEndpoint', '')");
test_session.runSql("UPDATE mysql_innodb_cluster_metadata.clusters SET attributes=JSON_INSERT(attributes, '$.adminType','whatever')");

// Fake one of the instances to ensure the calculated X port falls out of range to test it is removed
test_session.runSql("UPDATE mysql_innodb_cluster_metadata.instances SET addresses=JSON_REPLACE(addresses, '$.mysqlClassic', 'localhost:6554', '$.mysqlX', 'localhost:65540') WHERE instance_id=2");
test_session.runSql("UPDATE mysql_innodb_cluster_metadata.instances SET addresses=JSON_REPLACE(addresses, '$.mysqlClassic', '[::1]:6554', '$.mysqlX', '[::1]:65540') WHERE instance_id=3");

var result = test_session.runSql("select * from mysql_innodb_cluster_metadata.clusters");
var old_cluster = result.fetchOneObject();
var old_cluster_attributes = JSON.parse(old_cluster.attributes)

var result = test_session.runSql("select * from mysql_innodb_cluster_metadata.replicasets");
var old_replicaset = result.fetchOneObject();
var old_replicaset_attributes = JSON.parse(old_replicaset.attributes)

var result = test_session.runSql("select * from mysql_innodb_cluster_metadata.instances");
var old_instance = result.fetchOneObject();
var old_instance_addresses = JSON.parse(old_instance.addresses)

var result = test_session.runSql("select * from mysql_innodb_cluster_metadata.routers");
var old_router = result.fetchOneObject();
var old_router_attributes = JSON.parse(old_router.attributes)

var result = test_session.runSql("select * from mysql_innodb_cluster_metadata.hosts where host_id = " + old_router.host_id);
var old_host = result.fetchOneObject();

shell.options.useWizards=1;
dba.upgradeMetadata();
shell.options.useWizards=0;

var result = test_session.runSql("select * from mysql_innodb_cluster_metadata.clusters");
var new_cluster = result.fetchOneObject();
var new_cluster_attributes = JSON.parse(new_cluster.attributes)

var result = test_session.runSql("select * from mysql_innodb_cluster_metadata.instances");
var new_instance = result.fetchOneObject();

var result = test_session.runSql("select json_contains_path(addresses, 'one', '$.mysqlX') AS migrated from mysql_innodb_cluster_metadata.instances WHERE instance_id = 1");
var valid_x_port_instance = result.fetchOneObject();

var result = test_session.runSql("select json_contains_path(addresses, 'one', '$.mysqlX') AS migrated from mysql_innodb_cluster_metadata.instances WHERE instance_id = 2");
var invalid_x_port_instance = result.fetchOneObject();

var result = test_session.runSql("select json_contains_path(addresses, 'one', '$.mysqlX') AS migrated from mysql_innodb_cluster_metadata.instances WHERE instance_id = 3");
var invalid_x_ipv6_port_instance = result.fetchOneObject();


var result = test_session.runSql("select * from mysql_innodb_cluster_metadata.routers");
var new_router = result.fetchOneObject();

// New Cluster Data Validation
EXPECT_EQ(new_cluster.cluster_id, old_cluster.cluster_id.toString());
EXPECT_EQ(new_cluster.cluster_name, old_cluster.cluster_name);
EXPECT_EQ(new_cluster.description, old_cluster.description);
EXPECT_EQ(new_cluster.options, old_cluster.options);
EXPECT_EQ(new_cluster.cluster_type, 'gr');
EXPECT_EQ(new_cluster.primary_mode, old_replicaset.topology_type);

// Old Cluster Attributes
EXPECT_EQ(new_cluster_attributes.default, old_cluster_attributes.default);
EXPECT_EQ(new_cluster_attributes.opt_gtidSetIsComplete, old_cluster_attributes.opt_gtidSetIsComplete);
EXPECT_FALSE('adminType' in new_cluster_attributes);

// Old Replicaset Attributes
EXPECT_EQ(new_cluster_attributes.adopted, parseBoolOrInt(old_replicaset_attributes.adopted));
EXPECT_EQ(new_cluster_attributes.group_replication_group_name, old_replicaset_attributes.group_replication_group_name);

// New Instance Data Validation
EXPECT_EQ(new_instance.instance_id, old_instance.instance_id);
EXPECT_EQ(new_instance.cluster_id, old_cluster.cluster_id.toString());
EXPECT_EQ(new_instance.address, old_instance_addresses.mysqlClassic);
EXPECT_EQ(new_instance.mysql_server_uuid, old_instance.mysql_server_uuid);
EXPECT_EQ(new_instance.instance_name, old_instance.instance_name);
EXPECT_EQ(new_instance.addresses, old_instance.addresses);
EXPECT_EQ(new_instance.attributes, old_instance.attributes);
EXPECT_EQ(new_instance.description, old_instance.description);

// X port Migration Validation
EXPECT_EQ(valid_x_port_instance.migrated, 1);
EXPECT_EQ(invalid_x_port_instance.migrated, 0);
EXPECT_EQ(invalid_x_ipv6_port_instance.migrated, 0);

// New Router Data Validation
EXPECT_EQ(new_router.router_id, old_router.router_id);
EXPECT_EQ(new_router.router_name, old_router.router_name);
EXPECT_EQ(new_router.product_name, "MySQL Router");
EXPECT_EQ(new_router.address, old_host.host_name);
EXPECT_EQ(new_router.version, old_router_attributes.version);
EXPECT_EQ(new_router.last_check_in, null);
EXPECT_EQ(new_router.attributes, old_router.attributes);
EXPECT_EQ(new_router.cluster_id, old_cluster.cluster_id.toString());
EXPECT_EQ(new_router.options, null);

//@<> Cleanup
session.close();
test_session.close();
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
testutil.destroySandbox(__mysql_sandbox_port3);
testutil.rmfile(metadata_1_0_1_file);
