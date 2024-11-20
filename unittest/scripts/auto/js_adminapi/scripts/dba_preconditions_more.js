//@<> INCLUDE metadata_schema_utils.inc

//@<> Utils
function check_exception(callback, expect_exp, expect_output) {
    WIPE_SHELL_LOG();
    testutil.wipeAllOutput();

    EXPECT_THROWS(callback, expect_exp);
    if (expect_output !== undefined)
        EXPECT_OUTPUT_CONTAINS(expect_output);
}

function check_no_exception(callback, expect_output) {
    WIPE_SHELL_LOG();
    testutil.wipeAllOutput();

    EXPECT_NO_THROWS(callback);
    if (expect_output !== undefined)
        EXPECT_OUTPUT_CONTAINS(expect_output);
}

//@<> Check UPGRADING
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: hostname});
shell.connect(__sandbox_uri1);

session.runSql("CREATE SCHEMA mysql_innodb_cluster_metadata_bkp");
session.runSql("CREATE SQL SECURITY INVOKER VIEW mysql_innodb_cluster_metadata_bkp.backup_stage (stage) AS SELECT 'UPGRADING'");
session.runSql("CREATE VIEW mysql_innodb_cluster_metadata_bkp.schema_version (major, minor, patch) AS SELECT 1, 0, 1");
session.runSql("SELECT GET_LOCK('mysql_innodb_cluster_metadata.upgrade_in_progress', 1)");

var expect_exp = "Metadata is being upgraded";
var expect_output = "ERROR: The metadata is being upgraded. Wait until the upgrade process completes and then retry the operation.";

check_exception(function(){ dba.createCluster("test"); }, expect_exp, expect_output);
check_exception(function(){ dba.getCluster(); }, expect_exp, expect_output);
check_exception(function(){ dba.dropMetadataSchema(); }, expect_exp, expect_output);
check_exception(function(){ dba.rebootClusterFromCompleteOutage("test"); }, expect_exp, expect_output);
check_exception(function(){ dba.checkInstanceConfiguration(); }, expect_exp, expect_output);
check_exception(function(){ dba.configureInstance(); }, expect_exp, expect_output);
check_exception(function(){ dba.upgradeMetadata(); }, expect_exp, expect_output);

if (__version_num >= 80011) {
    check_exception(function(){ dba.configureReplicaSetInstance(); }, expect_exp, expect_output);
    check_exception(function(){ dba.createReplicaSet("test"); }, expect_exp, expect_output);
    check_exception(function(){ dba.getReplicaSet(); }, expect_exp, expect_output);
}

if (__version_num >= 80027) {
    check_exception(function(){ dba.getClusterSet(); }, expect_exp, expect_output);
}

session.close();

//@<> Check FAILED_UPGRADE
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: hostname});
shell.connect(__sandbox_uri1);

session.runSql("CREATE SCHEMA mysql_innodb_cluster_metadata_bkp");
session.runSql("CREATE SQL SECURITY INVOKER VIEW mysql_innodb_cluster_metadata_bkp.backup_stage (stage) AS SELECT 'UPGRADING'");
session.runSql("CREATE VIEW mysql_innodb_cluster_metadata_bkp.schema_version (major, minor, patch) AS SELECT 1, 0, 1");

var expect_exp = "Metadata upgrade failed";
var expect_output = "ERROR: An unfinished metadata upgrade was detected, which may have left it in an invalid state. Execute dba.upgradeMetadata again to repair it.";

check_exception(function(){ dba.createCluster("test"); }, expect_exp, expect_output);
check_exception(function(){ dba.getCluster(); }, expect_exp, expect_output);
check_exception(function(){ dba.dropMetadataSchema(); }, expect_exp, expect_output);
check_exception(function(){ dba.rebootClusterFromCompleteOutage("test"); }, expect_exp, expect_output);
check_exception(function(){ dba.checkInstanceConfiguration(); }, expect_exp, expect_output);
check_exception(function(){ dba.configureInstance(); }, expect_exp, expect_output);
check_exception(function(){ dba.upgradeMetadata(); }, "This function is not available through a session to a standalone instance");

if (__version_num >= 80011) {
    check_exception(function(){ dba.configureReplicaSetInstance(); }, expect_exp, expect_output);
    check_exception(function(){ dba.createReplicaSet("test"); }, expect_exp, expect_output);
    check_exception(function(){ dba.getReplicaSet(); }, expect_exp, expect_output);
}

if (__version_num >= 80027) {
    check_exception(function(){ dba.getClusterSet(); }, expect_exp, expect_output);
}

session.close();

//@<> Check MAJOR_HIGHER or MINOR_HIGHER or PATCH_HIGHER
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: hostname});
shell.connect(__sandbox_uri1);

//need a cluster
var cluster = dba.createCluster("sample");

var md_version_current = testutil.getCurrentMetadataVersion();
var md_version_installed = testutil.getInstalledMetadataVersion();

println("Base MD Version: " + md_version_installed)

version = md_version_installed.split('.');
var major = parseInt(version[0]);
var minor = parseInt(version[1]);
var patch = parseInt(version[2]);

// perform the tests with major + 1

set_metadata_version(major + 1, minor, patch);

var expect_exp = "Metadata version is not compatible";
var expect_output = `ERROR: Incompatible Metadata version. No operations are allowed on a Metadata version higher than Shell supports ('${major}.${minor}.${patch}'), the installed Metadata version '${major + 1}.${minor}.${patch}' is not supported. Please upgrade MySQL Shell.`;

check_exception(function(){ dba.createCluster("test"); }, expect_exp, expect_output);
check_exception(function(){ dba.getCluster(); }, expect_exp, expect_output);
check_exception(function(){ dba.dropMetadataSchema(); }, expect_exp, expect_output);
check_exception(function(){ dba.rebootClusterFromCompleteOutage("test"); }, expect_exp, expect_output);
check_exception(function(){ dba.checkInstanceConfiguration(); }, expect_exp, expect_output);
check_exception(function(){ dba.configureInstance(); }, expect_exp, expect_output);
check_exception(function(){ dba.upgradeMetadata(); }, expect_exp, expect_output);

if (__version_num >= 80011) {
    check_exception(function(){ dba.configureReplicaSetInstance(); }, expect_exp, expect_output);
    check_exception(function(){ dba.createReplicaSet("test"); }, expect_exp, expect_output);
    check_exception(function(){ dba.getReplicaSet(); }, expect_exp, expect_output);
}

if (__version_num >= 80027) {
    check_exception(function(){ dba.getClusterSet(); }, expect_exp, expect_output);
}

// perform the tests with minor + 1

set_metadata_version(major, minor + 1, patch);

var expect_exp = "Metadata version is not compatible";
var expect_output = `ERROR: Incompatible Metadata version. No operations are allowed on a Metadata version higher than Shell supports ('${major}.${minor}.${patch}'), the installed Metadata version '${major}.${minor + 1}.${patch}' is not supported. Please upgrade MySQL Shell.`;

check_exception(function(){ dba.createCluster("test"); }, expect_exp, expect_output);
check_exception(function(){ dba.getCluster(); }, expect_exp, expect_output);
check_exception(function(){ dba.dropMetadataSchema(); }, expect_exp, expect_output);
check_exception(function(){ dba.rebootClusterFromCompleteOutage("test"); }, expect_exp, expect_output);
check_exception(function(){ dba.checkInstanceConfiguration(); }, expect_exp, expect_output);
check_exception(function(){ dba.configureInstance(); }, expect_exp, expect_output);
check_exception(function(){ dba.upgradeMetadata(); }, expect_exp, expect_output);

if (__version_num >= 80011) {
    check_exception(function(){ dba.configureReplicaSetInstance(); }, expect_exp, expect_output);
    check_exception(function(){ dba.createReplicaSet("test"); }, expect_exp, expect_output);
    check_exception(function(){ dba.getReplicaSet(); }, expect_exp, expect_output);
}

if (__version_num >= 80027) {
    check_exception(function(){ dba.getClusterSet(); }, expect_exp, expect_output);
}

// perform the tests with patch + 1

set_metadata_version(major, minor, patch + 1);

var expect_exp = "Metadata version is not compatible";
var expect_output = `ERROR: Incompatible Metadata version. No operations are allowed on a Metadata version higher than Shell supports ('${major}.${minor}.${patch}'), the installed Metadata version '${major}.${minor}.${patch + 1}' is not supported. Please upgrade MySQL Shell.`;

check_exception(function(){ dba.createCluster("test"); }, expect_exp, expect_output);
check_exception(function(){ dba.getCluster(); }, expect_exp, expect_output);
check_exception(function(){ dba.dropMetadataSchema(); }, expect_exp, expect_output);
check_exception(function(){ dba.rebootClusterFromCompleteOutage("test"); }, expect_exp, expect_output);
check_exception(function(){ dba.checkInstanceConfiguration(); }, expect_exp, expect_output);
check_exception(function(){ dba.configureInstance(); }, expect_exp, expect_output);
check_exception(function(){ dba.upgradeMetadata(); }, expect_exp, expect_output);

if (__version_num >= 80011) {
    check_exception(function(){ dba.configureReplicaSetInstance(); }, expect_exp, expect_output);
    check_exception(function(){ dba.createReplicaSet("test"); }, expect_exp, expect_output);
    check_exception(function(){ dba.getReplicaSet(); }, expect_exp, expect_output);
}

if (__version_num >= 80027) {
    check_exception(function(){ dba.getClusterSet(); }, expect_exp, expect_output);
}

session.close();

//@<> Check compatibility warning
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: hostname});
shell.connect(__sandbox_uri1);

//need a cluster
var cluster = dba.createCluster("sample");

var md_version_current = testutil.getCurrentMetadataVersion();
var md_version_installed = testutil.getInstalledMetadataVersion();

println("Base MD Version: " + md_version_installed)

version = md_version_installed.split('.');
var major = parseInt(version[0]);
var minor = parseInt(version[1]);
var patch = parseInt(version[2]);

// we can't check for (major - 1) because that would create a MD version 1, which isn't compatible

// perform the tests with minor - 1
set_metadata_version(major, minor - 1, patch);

var expect_output = `WARNING: The installed metadata version '${major}.${minor - 1}.${patch}' is lower than the version supported by Shell, version '${major}.${minor}.${patch}'. It is recommended to upgrade the Metadata. See \\? dba.upgradeMetadata for additional details.`;

check_no_exception(function(){ dba.getCluster(); }, expect_output);
check_exception(function(){ dba.rebootClusterFromCompleteOutage("test"); }, "The cluster with the name 'test' does not exist.", expect_output);

if (__version_num >= 80011) {
    check_exception(function(){ dba.getReplicaSet(); }, "This function is not available through a session to an instance already in an InnoDB Cluster", expect_output);
}

if (__version_num >= 80027) {
    check_exception(function(){ dba.getClusterSet(); }, `No metadata found for the ClusterSet that ${hostname}:${__mysql_sandbox_port1} belongs to.`, expect_output);
}

// perform the tests with patch - 1
set_metadata_version(major, minor, patch - 1);

var expect_output = `WARNING: The installed metadata version '${major}.${minor}.${patch - 1}' is lower than the version supported by Shell, version '${major}.${minor}.${patch}'. It is recommended to upgrade the Metadata. See \\? dba.upgradeMetadata for additional details.`;

check_no_exception(function(){ dba.getCluster(); }, expect_output);
check_exception(function(){ dba.rebootClusterFromCompleteOutage("test"); }, "The cluster with the name 'test' does not exist.", expect_output);

if (__version_num >= 80011) {
    check_exception(function(){ dba.getReplicaSet(); }, "This function is not available through a session to an instance already in an InnoDB Cluster", expect_output);
}

if (__version_num >= 80027) {
    check_exception(function(){ dba.getClusterSet(); }, `No metadata found for the ClusterSet that ${hostname}:${__mysql_sandbox_port1} belongs to.`, expect_output);
}

session.close();

//@<> Check for min MD version requirements for dba commands {!__dbug_off}
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: hostname});
shell.connect(__sandbox_uri1);

// replicaset related tests

EXPECT_NO_THROWS(function(){ dba.configureReplicaSetInstance(); }); //without MD, dba.configureReplicaSetInstance() must succeed

EXPECT_NO_THROWS(function(){ dba.createReplicaSet("test"); });

testutil.dbugSet("+d,md_assume_version_lower");

testutil.dbugSet("+d,md_assume_version_1_0_1");
var expect_exp = "Metadata version is not compatible";
var expect_output = "ERROR: Incompatible Metadata version. This operation is disallowed because the installed Metadata version '1.0.1' is lower than the required version, '2.0.0'. Upgrade the Metadata to remove this restriction. See \\? dba.upgradeMetadata for additional details.";

check_exception(function(){ dba.getReplicaSet(); }, expect_exp, expect_output);
check_exception(function(){ dba.configureReplicaSetInstance(); }, expect_exp, expect_output);

// clusterset related tests

testutil.dbugSet("+d,md_assume_version_2_0_0");
var expect_exp = "Metadata version is not compatible";
var expect_output = "ERROR: Incompatible Metadata version. This operation is disallowed because the installed Metadata version '2.0.0' is lower than the required version, '2.1.0'. Upgrade the Metadata to remove this restriction. See \\? dba.upgradeMetadata for additional details.";

check_exception(function(){ dba.getClusterSet(); }, expect_exp, expect_output);

testutil.dbugSet("");

//@<> Check for min MD version requirements for cluster commands {!__dbug_off}
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: hostname});
shell.connect(__sandbox_uri1);

var cluster;
EXPECT_NO_THROWS(function(){ cluster = dba.createCluster("test"); });

testutil.dbugSet("+d,md_assume_version_lower");

testutil.dbugSet("+d,md_assume_version_1_0_1");
var expect_exp = "Metadata version is not compatible";
var expect_output = `ERROR: Incompatible Metadata version. This operation is disallowed because the installed Metadata version '1.0.1' is lower than the required version, '2.0.0'. Upgrade the Metadata to remove this restriction. See \\? dba.upgradeMetadata for additional details.`;

check_exception(function(){ cluster.addInstance(__sandbox_uri2); }, expect_exp, expect_output);
check_exception(function(){ cluster.removeInstance(__sandbox_uri2); }, expect_exp, expect_output);
check_exception(function(){ cluster.rejoinInstance(__sandbox_uri2); }, expect_exp, expect_output);
check_exception(function(){ cluster.dissolve(); }, expect_exp, expect_output);
check_exception(function(){ cluster.rescan(); }, expect_exp, expect_output);
check_exception(function(){ cluster.forceQuorumUsingPartitionOf(__sandbox_uri1); }, expect_exp, expect_output);
check_exception(function(){ cluster.switchToSinglePrimaryMode(); }, expect_exp, expect_output);
check_exception(function(){ cluster.switchToMultiPrimaryMode(); }, expect_exp, expect_output);
check_exception(function(){ cluster.setRoutingOption("tag:test_tag", 567); }, expect_exp, expect_output);
check_exception(function(){ cluster.routingOptions(); }, expect_exp, expect_output);
check_exception(function(){ cluster.fenceAllTraffic(); }, expect_exp, expect_output);
check_exception(function(){ cluster.fenceWrites(); }, expect_exp, expect_output);
check_exception(function(){ cluster.unfenceWrites(); }, expect_exp, expect_output);

testutil.dbugSet("+d,md_assume_version_2_0_0");
var expect_exp = "Metadata version is not compatible";
var expect_output = `ERROR: Incompatible Metadata version. This operation is disallowed because the installed Metadata version '2.0.0' is lower than the required version, '2.1.0'. Upgrade the Metadata to remove this restriction. See \\? dba.upgradeMetadata for additional details.`;

check_exception(function(){ cluster.routerOptions(); }, expect_exp, expect_output);

testutil.dbugSet("+d,md_assume_version_2_1_0");
var expect_exp = "Metadata version is not compatible";
var expect_output = `ERROR: Incompatible Metadata version. This operation is disallowed because the installed Metadata version '2.1.0' is lower than the required version, '2.2.0'. Upgrade the Metadata to remove this restriction. See \\? dba.upgradeMetadata for additional details.`;

check_exception(function(){ cluster.addReplicaInstance(__sandbox_uri2); }, expect_exp, expect_output);

// Routing Guidelines related tests

testutil.dbugSet("+d,md_assume_version_2_2_0");
var expect_exp = "Metadata version is not compatible";
var expect_output = `ERROR: Incompatible Metadata version. This operation is disallowed because the installed Metadata version '2.2.0' is lower than the required version, '2.3.0'. Upgrade the Metadata to remove this restriction. See \\? dba.upgradeMetadata for additional details.`;

check_exception(function(){ cluster.createRoutingGuideline("foo"); }, expect_exp, expect_output);
check_exception(function(){ cluster.getRoutingGuideline("foo"); }, expect_exp, expect_output);
check_exception(function(){ cluster.routingGuidelines(); }, expect_exp, expect_output);
check_exception(function(){ cluster.removeRoutingGuideline("foo"); }, expect_exp, expect_output);
check_exception(function(){ cluster.importRoutingGuideline("foo"); }, expect_exp, expect_output);

testutil.dbugSet("");

//@<> Check for min MD version requirements for replicaset commands {VER(>=8.0.11) && !__dbug_off}
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: hostname});
shell.connect(__sandbox_uri1);

var rset;
EXPECT_NO_THROWS(function(){ rset = dba.createReplicaSet("test"); });

testutil.dbugSet("+d,md_assume_version_lower");

testutil.dbugSet("+d,md_assume_version_2_0_0");
var expect_exp = "Metadata version is not compatible";
var expect_output = `ERROR: Incompatible Metadata version. This operation is disallowed because the installed Metadata version '2.0.0' is lower than the required version, '2.1.0'. Upgrade the Metadata to remove this restriction. See \\? dba.upgradeMetadata for additional details.`;

check_exception(function(){ rset.routerOptions(); }, expect_exp, expect_output);

testutil.dbugSet("");


//@<> Check for min MD version requirements for clusterset commands {VER(>=8.0.27) && !__dbug_off}
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: hostname});
shell.connect(__sandbox_uri1);

var cluster;
EXPECT_NO_THROWS(function(){ cluster = dba.createCluster("test"); });

var clusterset;
EXPECT_NO_THROWS(function(){ clusterset = cluster.createClusterSet("test"); });

testutil.dbugSet("+d,md_assume_version_lower");

testutil.dbugSet("+d,md_assume_version_2_0_0");
var expect_exp = "Metadata version is not compatible";
var expect_output = `ERROR: Incompatible Metadata version. This operation is disallowed because the installed Metadata version '2.0.0' is lower than the required version, '2.1.0'. Upgrade the Metadata to remove this restriction. See \\? dba.upgradeMetadata for additional details.`;

check_exception(function(){ cluster.getClusterSet(); }, expect_exp, expect_output);
check_exception(function(){ cluster.createClusterSet("test"); }, expect_exp, expect_output);

testutil.dbugSet("");

//@<> Cleanup
testutil.destroySandbox(__mysql_sandbox_port1);
