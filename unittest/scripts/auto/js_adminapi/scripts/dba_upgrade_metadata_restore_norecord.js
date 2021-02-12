//@{!__dbug_off}

//@<> INCLUDE metadata_schema_utils.inc

//@<> Initialization and supporting code
metadata_1_0_1_file = "metadata_1_0_1.sql";

//@<> Generic Functions
function set_upgrading_state(state) {
    // enum class State { NONE, SETTING_UPGRADE_VERSION, UPGRADING, DONE };
    session.runSql(
        "CREATE OR REPLACE SQL SECURITY INVOKER VIEW mysql_innodb_cluster_metadata.backup_state (state) AS SELECT ?",
        [state]);
}

/**
 * This function creates a scenario where an upgrade is interrupted in the middle,
 * this means the following is true:
 *
 * - MD schema_version is set to upgrading version: 0.0.0
 * - There is a MD schema backup
 */
function create_failed_upgrade_scenario() {
    load_metadata(__sandbox_uri1, metadata_1_0_1_file)
    session.runSql("delete from mysql_innodb_cluster_metadata.routers")
    testutil.dbugSet("+d,dba_CRASH_metadata_upgrade_at_UPGRADING");
    dba.upgradeMetadata();
    testutil.dbugSet("");
    // We create this schema to emulate the crash occurred after the step backup was created
    session.runSql("CREATE SCHEMA mysql_innodb_cluster_metadata_previous")
}

function verify_successful_restore_output(restore_data, drop_backups) {
    EXPECT_OUTPUT_CONTAINS("WARNING: An unfinished metadata upgrade was detected, which may have left it in an invalid state.");
    if (restore_data) {
        EXPECT_OUTPUT_CONTAINS("The metadata will be restored to version " + installed_before + ".");
        EXPECT_OUTPUT_CONTAINS("Restoring metadata to version " + installed_before + "...");
    } else {
        EXPECT_OUTPUT_CONTAINS("The metadata upgrade to version " + testutil.getCurrentMetadataVersion() + " will be completed");
    }

    if (drop_backups) {
        EXPECT_OUTPUT_CONTAINS("Removing metadata backup...");
    }
}

function verify_no_backups() {
    var res = session.runSql("SHOW DATABASES LIKE 'mysql_innodb_cluster_metadata_bkp'");
    var row = res.fetchOne();
    EXPECT_EQ(null, row, "Unexpected metadata general backup found");
    var res = session.runSql("SHOW DATABASES LIKE 'mysql_innodb_cluster_metadata_previous'");
    var row = res.fetchOne();
    EXPECT_EQ(null, row, "Unexpected metadata step backup found");
}

//@<> Creates the initial cluster
testutil.deploySandbox(__mysql_sandbox_port1, "root", { report_host: hostname });
testutil.snapshotSandboxConf(__mysql_sandbox_port1);

shell.connect(__sandbox_uri1);
dba.createCluster('sample');
var group_name = session.runSql("SELECT @@group_replication_group_name").fetchOne()[0];
var server_uuid = session.runSql("SELECT @@server_uuid").fetchOne()[0];
var server_id = session.runSql("SELECT @@server_id").fetchOne()[0];

prepare_1_0_1_metadata_from_template(metadata_1_0_1_file, group_name, [[server_uuid, server_id]]);

//@<> persist GR configuration settings for 5.7 servers {VER(<8.0.11)}
var mycnf1 = testutil.getSandboxConfPath(__mysql_sandbox_port1);
dba.configureLocalInstance('root:root@localhost:' + __mysql_sandbox_port1, {mycnfPath: mycnf1});

//@<> Verify Cluster Status
load_metadata(__sandbox_uri1, metadata_1_0_1_file, false, group_name)
session.runSql("delete from mysql_innodb_cluster_metadata.routers")
var cluster = dba.getCluster()
var installed_before = testutil.getInstalledMetadataVersion();
var version = installed_before.split('.');

var major = parseInt(version[0]);
var minor = parseInt(version[1]);
var patch = parseInt(version[2]);

var status = cluster.status();
EXPECT_EQ("ONLINE", status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port1}`]["status"])
EXPECT_EQ("R/W", status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port1}`]["mode"])
EXPECT_EQ("OK_NO_TOLERANCE", status["defaultReplicaSet"]["status"])


//@<> upgradeMetadata, restore dryRun preparation
create_failed_upgrade_scenario();
EXPECT_STDERR_EMPTY();

//@<> upgradeMetadata, restore dryRun
dba.upgradeMetadata({ dryRun: true });
EXPECT_OUTPUT_CONTAINS("The metadata can be restored to version " + installed_before);

/**
 * Restore Process Steps
 * =====================
 * 1) Drop MD Schema
 * 2) Deploy MD Schema
 * 2.1) Store original version in temporary view
 * 2.2) Set MD to Upgrading Version
 * 3) Restore Data From Backup
 * 4) Drop Step Backup
 * 5) Drop Backup
 * 5.1) Set MD to Original Version
 * 5.2) Remove temporary view with original version
 */

//@<> Defalt Restore process [USE:Verify Cluster Status]
// A failure in Step 1 of the Restore Process falls under the normal restore process
dba.upgradeMetadata();
verify_no_backups();
cluster.status();

//@<> Restore after failure in Restore Process Step 2 preparation
create_failed_upgrade_scenario();
session.runSql("DROP SCHEMA mysql_innodb_cluster_metadata")
EXPECT_STDERR_EMPTY();

//@<> Restore after failure in Restore Process Step 2 [USE:Verify Cluster Status]
// At this point, MD has been dropped, only the backup exists
dba.upgradeMetadata();
verify_successful_restore_output(true, true);
verify_no_backups();
cluster.status();


//@<> Restore after failure in Restore Process Step 2.1 preparation
create_failed_upgrade_scenario();
set_upgrading_state("UPGRADING");
EXPECT_STDERR_EMPTY();

//@<> Restore after failure in Restore Process Step 2.1 [USE:Verify Cluster Status]
// At this point the MD has been re-deployed in upgrading mode so it contains in
// schema_version 0.0.0 and upgrading state is "UPGRADING" and the original version
dba.upgradeMetadata();
verify_successful_restore_output(true, true);
verify_no_backups();
cluster.status();

//@<> Restore after failure in Restore Process Step 3 and 4 preparation
create_failed_upgrade_scenario();
set_upgrading_state("UPGRADING");
EXPECT_STDERR_EMPTY();

//@<> Restore after failure in Restore Process Step 3 and 4 [USE:Verify Cluster Status]
// At this point the MD has been re-deployed, original version is backed up in original_version
// And schema_version has been set to the upgrading version
// The data has not been copied back
// In other words, we fall back in the normal restore scenario
dba.upgradeMetadata();
verify_successful_restore_output(true, true);
verify_no_backups();
cluster.status();

//@<> Restore after failure in Restore Process Step 5 preparation
create_failed_upgrade_scenario();
set_upgrading_state("UPGRADING");
session.runSql("DROP SCHEMA mysql_innodb_cluster_metadata_previous");
EXPECT_STDERR_EMPTY();

//@<> Restore after failure in Restore Process Step 5 [USE:Verify Cluster Status]
// At this point the data has been copied back, and the step backup has been deleted
// But failed dropping the master backup
dba.upgradeMetadata();
verify_successful_restore_output(true, true);
verify_no_backups();
cluster.status();

//@<> Successful restore preparation
create_failed_upgrade_scenario();
EXPECT_STDERR_EMPTY();

//@<> Successful restore
dba.upgradeMetadata();
verify_successful_restore_output(true, true);
verify_no_backups();

//@<> Restore after failure in Restore Process Step 6 preparation
dba.upgradeMetadata();
set_metadata_version(0, 0, 0);
EXPECT_STDERR_EMPTY();

//@<> Restore after failure in Restore Process Step 6 [USE:Verify Cluster Status]
// At this point the data has been copied back and the backups were removed, but
// failed updating the schema version so it is still in 0.0.0
dba.upgradeMetadata();
verify_successful_restore_output(false, false);
verify_no_backups();
cluster.status();

testutil.destroySandbox(__mysql_sandbox_port1);
testutil.rmfile(metadata_1_0_1_file);
