//@{false}
// Disabling tests as this code will be refactored
function set_metadata_version(major, minor, patch) {
    session.runSql("DROP VIEW mysql_innodb_cluster_metadata.schema_version");
    session.runSql("CREATE VIEW mysql_innodb_cluster_metadata.schema_version (major, minor, patch) AS SELECT ?, ?, ?", [major, minor, patch]);
}

function ensure_upgrade_locks_are_free(session) {
    var result = session.runSql("SELECT GET_LOCK('mysql_innodb_cluster_metadata.upgrade_in_progress', 1)")
    var row = result.fetchOne();
    EXPECT_EQ(1, row[0]);
    session.runSql("DO RELEASE_LOCK('mysql_innodb_cluster_metadata.upgrade_in_progress')");
}

//@<> Creates the initial cluster
testutil.deploySandbox(__mysql_sandbox_port1, "root", { report_host: hostname });
testutil.snapshotSandboxConf(__mysql_sandbox_port1);
testutil.deploySandbox(__mysql_sandbox_port2, "root", { report_host: hostname });
testutil.snapshotSandboxConf(__mysql_sandbox_port2);

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
cluster.addInstance(__sandbox_uri2, { recoveryMethod: 'incremental' })
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");

//@<> persist GR configuration settings for 5.7 servers {VER(<8.0.11)}
var mycnf1 = testutil.getSandboxConfPath(__mysql_sandbox_port1);
dba.configureLocalInstance('root:root@localhost:' + __mysql_sandbox_port1, {mycnfPath: mycnf1});
var mycnf2 = testutil.getSandboxConfPath(__mysql_sandbox_port2);
dba.configureLocalInstance('root:root@localhost:' + __mysql_sandbox_port2, {mycnfPath: mycnf2});

//@<> upgradeMetadata, installed version is up to date
EXPECT_NO_THROWS(function () { dba.upgradeMetadata() })
EXPECT_OUTPUT_CONTAINS("Installed metadata at 'localhost:" + __mysql_sandbox_port1 + "' is up to date (version " + current_version + ").");

//@<> upgradeMetadata, installed version is greater than current version
set_metadata_version(major, minor, patch + 1)
var installed_version = testutil.getInstalledMetadataVersion();
EXPECT_THROWS(function () { dba.upgradeMetadata() }, "Installed metadata at 'localhost:" + __mysql_sandbox_port1 + "' is newer than the version version supported by this Shell (installed: " + installed_version + ", shell: " + current_version + ")")

//@<> Preparation for the following tests
other_session = mysql.getSession(__sandbox_uri1);
set_metadata_version(1, 0, 0)
var installed_before = testutil.getInstalledMetadataVersion();

//@<> upgradeMetadata, failed at BACKING_UP_METADATA state, no backup was created {!__dbug_off}
session.runSql("CREATE SCHEMA mysql_innodb_cluster_metadata_bkp");
EXPECT_THROWS(function () { dba.upgradeMetadata() }, "Dba.upgradeMetadata: Upgrade process failed, metadata was not modified.");
EXPECT_OUTPUT_CONTAINS("Upgrading metadata at 'localhost:" + __mysql_sandbox_port1 + "' from version " + installed_before + " to version " + current_version + ".");
EXPECT_OUTPUT_CONTAINS("Creating backup of the metadata schema...");
EXPECT_OUTPUT_CONTAINS("ERROR: Can't create database 'mysql_innodb_cluster_metadata_bkp'; database exists.");
EXPECT_OUTPUT_NOT_CONTAINS("Removing metadata backup...");
session.runSql("DROP SCHEMA mysql_innodb_cluster_metadata_bkp");
ensure_upgrade_locks_are_free(other_session);
var installed_after = testutil.getInstalledMetadataVersion();
EXPECT_EQ(installed_before, installed_after);

//@<> upgradeMetadata, failed at BACKING_UP_METADATA state after backup schema was created {!__dbug_off}
testutil.dbugSet("+d,dba_FAIL_metadata_upgrade_at_BACKING_UP_METADATA");
EXPECT_THROWS(function () { dba.upgradeMetadata() }, "Dba.upgradeMetadata: Upgrade process failed, metadata was not modified.");
EXPECT_OUTPUT_CONTAINS("Upgrading metadata at 'localhost:" + __mysql_sandbox_port1 + "' from version " + installed_before + " to version " + current_version + ".");
EXPECT_OUTPUT_CONTAINS("Creating backup of the metadata schema...");
EXPECT_OUTPUT_CONTAINS("ERROR: Debug emulation of failed metadata upgrade BACKING_UP_METADATA.");
EXPECT_OUTPUT_CONTAINS("Removing metadata backup...");
ensure_upgrade_locks_are_free(other_session);
var installed_after = testutil.getInstalledMetadataVersion();
EXPECT_EQ(installed_before, installed_after);
testutil.dbugSet("");


//@<> upgradeMetadata, failed at SETTING_UPGRADE_VERSION state {!__dbug_off}
// Emulating the case where the router locked the schema_version and we try
// an upgrade at that very specific moment
testutil.dbugSet("+d,dba_limit_lock_wait_timeout");
other_session.runSql("START TRANSACTION");
other_session.runSql("SELECT * FROM mysql_innodb_cluster_metadata.schema_version");

EXPECT_THROWS(function () { dba.upgradeMetadata() }, "Dba.upgradeMetadata: Upgrade process failed, metadata was not modified.");
EXPECT_OUTPUT_CONTAINS("Upgrading metadata at 'localhost:" + __mysql_sandbox_port1 + "' from version " + installed_before + " to version " + current_version + ".");
EXPECT_OUTPUT_CONTAINS("Creating backup of the metadata schema...");
EXPECT_OUTPUT_CONTAINS("ERROR: localhost:" + __mysql_sandbox_port1 + ": Lock wait timeout exceeded; try restarting transaction");
EXPECT_OUTPUT_CONTAINS("Removing metadata backup...");

other_session.runSql("ROLLBACK");
ensure_upgrade_locks_are_free(other_session);

var installed_after = testutil.getInstalledMetadataVersion();
EXPECT_EQ(installed_before, installed_after);
testutil.dbugSet("");

//@<> upgradeMetadata, failed upgrade, automatic restore {!__dbug_off}
var installed_before = testutil.getInstalledMetadataVersion();
testutil.dbugSet("+d,dba_FAIL_metadata_upgrade_at_UPGRADING");
EXPECT_THROWS(function () { dba.upgradeMetadata() }, "Dba.upgradeMetadata: Upgrade process failed, metadata has been restored to version " + installed_before + ".");
EXPECT_OUTPUT_CONTAINS("Upgrading metadata at 'localhost:" + __mysql_sandbox_port1 + "' from version " + installed_before + " to version " + current_version + ".");
EXPECT_OUTPUT_CONTAINS("Creating backup of the metadata schema...");
EXPECT_OUTPUT_CONTAINS("upgrading from 1.0.0 to 1.0.1...");
EXPECT_OUTPUT_CONTAINS("ERROR: Debug emulation of failed metadata upgrade UPGRADING.");
EXPECT_OUTPUT_CONTAINS("Restoring metadata to version " + installed_before);
EXPECT_OUTPUT_CONTAINS("Removing metadata backup...");

ensure_upgrade_locks_are_free(other_session);
var installed_after = testutil.getInstalledMetadataVersion();
EXPECT_EQ(installed_before, installed_after);
testutil.dbugSet("");

//@<> upgradeMetadata, prepare for upgrading errors {!__dbug_off}
var installed_before = testutil.getInstalledMetadataVersion();
testutil.dbugSet("+d,dba_CRASH_metadata_upgrade_at_UPGRADING");
dba.upgradeMetadata();
EXPECT_OUTPUT_CONTAINS("Upgrading metadata at 'localhost:" + __mysql_sandbox_port1 + "' from version " + installed_before + " to version " + current_version + ".");
EXPECT_OUTPUT_CONTAINS("Creating backup of the metadata schema...");
testutil.dbugSet("");
other_session.runSql("SELECT GET_LOCK('mysql_innodb_cluster_metadata.upgrade_in_progress', 1)")

//@<> Upgrading: precondition error upgradeMetadata {!__dbug_off}
EXPECT_THROWS(function () { dba.upgradeMetadata() }, "Dba.upgradeMetadata: The metadata is being upgraded. Wait until the upgrade process completes and then retry the operation.");

//@<> Upgrading: precondition error upgradeMetadata + dryRun {!__dbug_off}
EXPECT_THROWS(function () { dba.upgradeMetadata({dryRun:true}) }, "Dba.upgradeMetadata: The metadata is being upgraded. Wait until the upgrade process completes and then retry the operation.");

//@<> upgradeMetadata, prepare for failed upgrade errors {!__dbug_off}
other_session.close();

//@<> upgradeMetadata, restore dryRun {!__dbug_off}
dba.upgradeMetadata({ dryRun: true });
EXPECT_OUTPUT_CONTAINS("The metadata can be restored to version " + installed_before);

//@<> upgradeMetadata, restore {!__dbug_off}
dba.upgradeMetadata();
EXPECT_OUTPUT_CONTAINS("Restoring metadata to version " + installed_before + "...");
EXPECT_OUTPUT_CONTAINS("Removing metadata backup...");

//@<> Downgrading to version 1.0.0 for the following tests
session.runSql("DROP VIEW IF EXISTS mysql_innodb_cluster_metadata.schema_version");

//@<> upgradeMetadata, not the right credentials
session.runSql("CREATE USER myguest@'localhost' IDENTIFIED BY 'mypassword'");
session.runSql("GRANT SELECT on *.* to myguest@'localhost'");
shell.connect("myguest:mypassword@localhost:" + __mysql_sandbox_port1);
EXPECT_THROWS(function () { dba.upgradeMetadata() }, "Dba.upgradeMetadata: The account 'myguest'@'localhost' is missing privileges required for this operation.");

//@<> upgradeMetadata, dryRrun upgrade required
shell.connect(__sandbox_uri1)
var installed_version = testutil.getInstalledMetadataVersion();
EXPECT_NO_THROWS(function () { dba.upgradeMetadata({dryRun: true}) })
EXPECT_OUTPUT_CONTAINS("Metadata at 'localhost:" + __mysql_sandbox_port1 + "' is at version " + installed_version + ", for this reason some AdminAPI functionality may be restricted. To fully enable the AdminAPI the metadata is required to be upgraded to version " + current_version + ".");

//@<> upgradeMetadata, upgrade succeeds
EXPECT_NO_THROWS(function () { dba.upgradeMetadata() })
EXPECT_OUTPUT_CONTAINS("Upgrading metadata at 'localhost:" + __mysql_sandbox_port1 + "' from version " + installed_version + " to version " + current_version + ".");
EXPECT_OUTPUT_CONTAINS("Upgrade process successfully finished, metadata schema is now on version " + current_version);

var installed_version = testutil.getInstalledMetadataVersion();
EXPECT_EQ(current_version, installed_version, "Upgrading Metadata");

testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
