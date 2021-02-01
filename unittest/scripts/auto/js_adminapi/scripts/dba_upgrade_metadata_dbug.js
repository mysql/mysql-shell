//@{!__dbug_off}

//@<> INCLUDE metadata_schema_utils.inc

//@<> Initialization and supporting code
metadata_1_0_1_file = "metadata_1_0_1.sql";

function ensure_upgrade_locks_are_free(session) {
    var result = session.runSql("SELECT GET_LOCK('mysql_innodb_cluster_metadata.upgrade_in_progress', 1)")
    var row = result.fetchOne();
    EXPECT_EQ(1, row[0]);
    session.runSql("DO RELEASE_LOCK('mysql_innodb_cluster_metadata.upgrade_in_progress')");
}

//@<> Creates the initial cluster
testutil.deploySandbox(__mysql_sandbox_port1, "root", { report_host: hostname });
testutil.snapshotSandboxConf(__mysql_sandbox_port1);

//@<> upgradeMetadata without connection
EXPECT_THROWS(function(){dba.upgradeMetadata()}, "An open session is required to perform this operation")

//@<> upgradeMetadata on a standalone instance
shell.connect(__sandbox_uri1)
EXPECT_THROWS(function(){dba.upgradeMetadata()}, "This function is not available through a session to a standalone instance")

var current_version = testutil.getCurrentMetadataVersion();
var version = current_version.split('.');

var major = parseInt(version[0]);
var minor = parseInt(version[1]);
var patch = parseInt(version[2]);

var cluster = dba.createCluster('sample')
var group_name = session.runSql("SELECT @@group_replication_group_name").fetchOne()[0];
var server_uuid = session.runSql("SELECT @@server_uuid").fetchOne()[0];
var server_id = session.runSql("SELECT @@server_id").fetchOne()[0];

//NOTE: This template schema is a 1.0.1 schema with registered outdated routers.
//      The following tests tweak this schema to match the required conditions
//      for each scenario being tested.
prepare_1_0_1_metadata_from_template(metadata_1_0_1_file, group_name, [[server_uuid, server_id]]);

//@<> persist GR configuration settings for 5.7 servers {VER(<8.0.11)}
var mycnf1 = testutil.getSandboxConfPath(__mysql_sandbox_port1);
dba.configureLocalInstance('root:root@localhost:' + __mysql_sandbox_port1, {mycnfPath: mycnf1});

//@<> Preparation for the following tests
other_session = mysql.getSession(__sandbox_uri1);

//@<> WL13386-TSFR1_1-A upgradeMetadata patch upgrade no interactive
testutil.dbugSet("+d,dba_EMULATE_CURRENT_MD_1_0_1");
load_metadata(__sandbox_uri1, metadata_1_0_1_file);
dba.upgradeMetadata();
testutil.dbugSet("");

//@<> WL13386-TSFR1_1-B upgradeMetadata patch upgrade interactive
testutil.dbugSet("+d,dba_EMULATE_CURRENT_MD_1_0_1");
load_metadata(__sandbox_uri1, metadata_1_0_1_file);
dba.upgradeMetadata({interactive:true});
testutil.dbugSet("");

//@<> WL13386-TSFR1_2 upgradeMetadata minor upgrade no interactive
testutil.dbugSet("+d,dba_EMULATE_CURRENT_MD_1_0_1");
load_metadata(__sandbox_uri1, metadata_1_0_1_file);
dba.upgradeMetadata();
testutil.dbugSet("");

//@<> WL13386-TSFR1_3-A upgradeMetadata minor interactive, skip upgrade
testutil.dbugSet("+d,dba_EMULATE_CURRENT_MD_1_0_1,dba_EMULATE_UNEXISTING_MD");
load_metadata(__sandbox_uri1, metadata_1_0_1_file);
set_metadata_version(1, -1, 0);
testutil.expectPrompt("Do you want to proceed with the upgrade (Yes/No) [y/N]: ", "n");
dba.upgradeMetadata({interactive:true});
testutil.dbugSet("");

//@<> WL13386-TSFR1_3-B upgradeMetadata minor interactive, do upgrade
testutil.dbugSet("+d,dba_EMULATE_ROUTER_UPGRADE");
testutil.dbugSet("+d,dba_EMULATE_CURRENT_MD_1_0_1");
testutil.dbugSet("+d,dba_EMULATE_UNEXISTING_MD");
load_metadata(__sandbox_uri1, metadata_1_0_1_file);
set_metadata_version(1, -1, 0);
// Router upgrade emulation requires router to have ID = 2
session.runSql("UPDATE mysql_innodb_cluster_metadata.routers SET router_id = 2 WHERE router_id=1");
testutil.expectPrompt("Do you want to proceed with the upgrade (Yes/No) [y/N]: ", "y")
testutil.expectPrompt("Please select an option: ", "1")
dba.upgradeMetadata({interactive:true});
testutil.dbugSet("");

//@<> WL13386-TSET_1 upgradeMetadata, failed at BACKING_UP_METADATA state after backup schema was created
load_metadata(__sandbox_uri1, metadata_1_0_1_file);
var installed_before = testutil.getInstalledMetadataVersion();
session.runSql("delete from mysql_innodb_cluster_metadata.routers");
testutil.dbugSet("+d,dba_FAIL_metadata_upgrade_at_BACKING_UP_METADATA");
EXPECT_THROWS(function () { dba.upgradeMetadata() }, "Dba.upgradeMetadata: Upgrade process failed, metadata was not modified.");
EXPECT_OUTPUT_CONTAINS(`Upgrading metadata at '${hostname}:${__mysql_sandbox_port1}' from version ${installed_before} to version ${current_version}.`);
EXPECT_OUTPUT_CONTAINS("Creating backup of the metadata schema...");
EXPECT_OUTPUT_CONTAINS("ERROR: Debug emulation of failed metadata upgrade BACKING_UP_METADATA.");
EXPECT_OUTPUT_CONTAINS("Removing metadata backup...");
ensure_upgrade_locks_are_free(other_session);
var installed_after = testutil.getInstalledMetadataVersion();
EXPECT_EQ(installed_before, installed_after);
testutil.dbugSet("");


//@<> WL13386-TSET_1 upgradeMetadata, failed at SETTING_UPGRADE_VERSION state
// Emulating the case where the router locked the schema_version and we try
// an upgrade at that very specific moment
testutil.dbugSet("+d,dba_limit_lock_wait_timeout");
other_session.runSql("START TRANSACTION");
other_session.runSql("SELECT * FROM mysql_innodb_cluster_metadata.schema_version");
EXPECT_THROWS(function () { dba.upgradeMetadata() }, "Dba.upgradeMetadata: Upgrade process failed, metadata was not modified.");
EXPECT_OUTPUT_CONTAINS(`Upgrading metadata at '${hostname}:${__mysql_sandbox_port1}' from version ${installed_before} to version ${current_version}.`);
EXPECT_OUTPUT_CONTAINS("Creating backup of the metadata schema...");
EXPECT_OUTPUT_CONTAINS(`ERROR: ${hostname}:${__mysql_sandbox_port1}: Lock wait timeout exceeded; try restarting transaction`);
EXPECT_OUTPUT_CONTAINS("Removing metadata backup...");

other_session.runSql("ROLLBACK");
ensure_upgrade_locks_are_free(other_session);

var installed_after = testutil.getInstalledMetadataVersion();
EXPECT_EQ(installed_before, installed_after);
testutil.dbugSet("");

//@<> WL13386-TSET_1 upgradeMetadata, failed upgrade, automatic restore
var installed_before = testutil.getInstalledMetadataVersion();
testutil.dbugSet("+d,dba_FAIL_metadata_upgrade_at_UPGRADING");
EXPECT_THROWS(function () { dba.upgradeMetadata() }, "Dba.upgradeMetadata: Upgrade process failed, metadata has been restored to version " + installed_before + ".");
EXPECT_OUTPUT_CONTAINS(`Upgrading metadata at '${hostname}:${__mysql_sandbox_port1}' from version ${installed_before} to version ${current_version}.`);
EXPECT_OUTPUT_CONTAINS("Creating backup of the metadata schema...");
EXPECT_OUTPUT_CONTAINS("upgrading from 1.0.1 to 2.0.0...");
EXPECT_OUTPUT_CONTAINS("ERROR: Debug emulation of failed metadata upgrade UPGRADING.");
EXPECT_OUTPUT_CONTAINS("Restoring metadata to version " + installed_before);
EXPECT_OUTPUT_CONTAINS("Removing metadata backup...");

ensure_upgrade_locks_are_free(other_session);
var installed_after = testutil.getInstalledMetadataVersion();
EXPECT_EQ(installed_before, installed_after);
var res = session.runSql("SHOW DATABASES LIKE 'mysql_innodb_cluster_metadata_bkp'");
var row = res.fetchOne();
EXPECT_EQ(null, row, "Unexpected metadata general backup found");
var res = session.runSql("SHOW DATABASES LIKE 'mysql_innodb_cluster_metadata_previous'");
var row = res.fetchOne();
EXPECT_EQ(null, row, "Unexpected metadata step backup found");
testutil.dbugSet("");

//@<> upgradeMetadata, prepare for upgrading errors
var installed_before = testutil.getInstalledMetadataVersion();
testutil.dbugSet("+d,dba_CRASH_metadata_upgrade_at_UPGRADING");
dba.upgradeMetadata();
EXPECT_OUTPUT_CONTAINS(`Upgrading metadata at '${hostname}:${__mysql_sandbox_port1}' from version ${installed_before} to version ${current_version}.`);
EXPECT_OUTPUT_CONTAINS("Creating backup of the metadata schema...");
testutil.dbugSet("");
other_session.runSql("SELECT GET_LOCK('mysql_innodb_cluster_metadata.upgrade_in_progress', 1)")

//@<> Upgrading: precondition error upgradeMetadata
EXPECT_THROWS(function () { dba.upgradeMetadata() }, "Dba.upgradeMetadata: The metadata is being upgraded. Wait until the upgrade process completes and then retry the operation.");

//@<> Upgrading: precondition error upgradeMetadata + dryRun
EXPECT_THROWS(function () { dba.upgradeMetadata({dryRun:true}) }, "Dba.upgradeMetadata: The metadata is being upgraded. Wait until the upgrade process completes and then retry the operation.");

//@<> upgradeMetadata, prepare for failed upgrade errors
other_session.close();
// Drops the backup schema created on the previous test
session.runSql("DROP DATABASE mysql_innodb_cluster_metadata_bkp");


//@ Upgrades the metadata, interactive options simulating unregister (no upgrade performed)
load_metadata(__sandbox_uri1, metadata_1_0_1_file);
session.runSql("INSERT INTO mysql_innodb_cluster_metadata.routers VALUES (2, 'test-router', 2, NULL)")
testutil.dbugSet("+d,dba_EMULATE_ROUTER_UNREGISTER")
// Chooses to Continue without fixing anything
testutil.expectPrompt("Please select an option: ", "1")
// Chooses to unregister the remaining routers
testutil.expectPrompt("Please select an option: ", "2")
// Aborts the router unregistration
testutil.expectPrompt("Unregistering a Router implies it will not be used in the Cluster, do you want to continue? [y/N]:", "n")
// Chooses to print the help on rolling an upgrade
testutil.expectPrompt("Please select an option: ", "4")
// Chooses to abort the operation
testutil.expectPrompt("Please select an option: ", "3")
EXPECT_NO_THROWS(function(){dba.upgradeMetadata({interactive:true})})
testutil.dbugSet("")

//@ Upgrades the metadata, interactive options simulating upgrade (no upgrade performed)
session.runSql("INSERT INTO mysql_innodb_cluster_metadata.routers VALUES (2, 'second', 2, NULL)")
testutil.dbugSet("+d,dba_EMULATE_ROUTER_UPGRADE")
// Chooses to Continue without fixing anything
testutil.expectPrompt("Please select an option: ", "1")
// Chooses to unregister the remaining routers
testutil.expectPrompt("Please select an option: ", "2")
// Aborts the router unregistration
testutil.expectPrompt("Unregistering a Router implies it will not be used in the Cluster, do you want to continue? [y/N]:", "n")
// Chooses to print the help on rolling an upgrade
testutil.expectPrompt("Please select an option: ", "4")
// Chooses to abort the operation
testutil.expectPrompt("Please select an option: ", "3")
EXPECT_NO_THROWS(function(){dba.upgradeMetadata({interactive:true})})
testutil.dbugSet("")

testutil.destroySandbox(__mysql_sandbox_port1);
testutil.rmfile(metadata_1_0_1_file);
