//@<> INCLUDE metadata_schema_utils.inc

//@<> Initialization and supporting code
metadata_1_0_1_file = "metadata_1_0_1.sql";
metadata_current_file = "current_metadata.sql"

var debug = true;
function print_debug(text) {
    if (debug) {
        println(text);
    }
}

function recreate_cluster(name, context) {
    EXPECT_NO_THROWS(function () {
        dba.createCluster(name)
    }, context);

    var result = session.runSql("SELECT attributes->>'$.group_replication_group_name' FROM mysql_innodb_cluster_metadata.clusters");
    var row = result.fetchOne()
    return row[0];
}

function load_metadata_version(major, minor, patch, clear_data, replication_group_name) {
    print_debug("Loading metadata version " + major + "." + minor + "." + patch);

    if (major == 1 || major == 0) {
        file = metadata_1_0_1_file;
    }else {
        file = metadata_current_file;
    }

    load_metadata(__sandbox_uri1, file, clear_data, replication_group_name);

    set_metadata_version(major, minor, patch);
}

function expected_exception(tag) {
    switch(tag) {
        case "MAJOR_HIGHER":
        case "MINOR_HIGHER":
        case "PATCH_HIGHER":
            return "Metadata version is not compatible";
        case "FAILED_UPGRADE":
            return "Metadata upgrade failed";
        case "UPGRADING":
            return "Metadata is being upgraded";
    }

    return "!unknown!";
}

function format_messages(installed, current) {
    return {
        MAJOR_HIGHER:
        `Incompatible Metadata version. No operations are allowed on a Metadata version higher than Shell supports ('${current}'), the installed Metadata version '${installed}' is not supported. Please upgrade MySQL Shell.`,
        MINOR_HIGHER:
        `Incompatible Metadata version. No operations are allowed on a Metadata version higher than Shell supports ('${current}'), the installed Metadata version '${installed}' is not supported. Please upgrade MySQL Shell.`,
        PATCH_HIGHER:
        `Incompatible Metadata version. No operations are allowed on a Metadata version higher than Shell supports ('${current}'), the installed Metadata version '${installed}' is not supported. Please upgrade MySQL Shell.`,
        MAJOR_LOWER:
        `The installed metadata version '${installed}' is lower than the version supported by Shell, version '${current}'. It is recommended to upgrade the Metadata. See \\? dba.upgradeMetadata for additional details.`,
        MINOR_LOWER:
        `The installed metadata version '${installed}' is lower than the version supported by Shell, version '${current}'. It is recommended to upgrade the Metadata. See \\? dba.upgradeMetadata for additional details.`,
        PATCH_LOWER:
        `The installed metadata version '${installed}' is lower than the version supported by Shell, version '${current}'. It is recommended to upgrade the Metadata. See \\? dba.upgradeMetadata for additional details.`,
        FAILED_UPGRADE:
        "An unfinished metadata upgrade was detected, which may have left it in an invalid state. Execute dba.upgradeMetadata again to repair it.",
        UPGRADING:
        "The metadata is being upgraded. Wait until the upgrade process completes and then retry the operation."
    }
}

//@<> Initialize to the point we have an instance with a metadata but not a cluster
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: hostname});
shell.connect(__sandbox_uri1);

// Prepare metadata schema 1.0.1
var server_uuid1 = session.runSql("SELECT @@server_uuid").fetchOne()[0];
var server_id1 = session.runSql("SELECT @@server_id").fetchOne()[0];
prepare_1_0_1_metadata_from_template(metadata_1_0_1_file, "", [[server_uuid1, server_id1]]);

var cluster = dba.createCluster("sample");
backup_metadata(__sandbox_uri1, metadata_current_file);

var cluster_id = session.runSql("SELECT cluster_id FROM mysql_innodb_cluster_metadata.clusters WHERE cluster_name = 'sample'").fetchOne()[0];

session.runSql("DELETE FROM mysql_innodb_cluster_metadata.instances WHERE cluster_id = '" + cluster_id +"'");
session.runSql("DELETE from mysql_innodb_cluster_metadata.clusters WHERE cluster_id = '" + cluster_id + "'");
session.runSql("STOP group_replication");
session.runSql("SET GLOBAL super_read_only=0");

// Calls an API function that should succeed and verifies the expected WARNING/NOTE if any
function call_and_validate(callback, expect_tag, is_warning) {
    WIPE_SHELL_LOG();
    testutil.wipeAllOutput();

    if (is_warning) {
        EXPECT_NO_THROWS(callback);
        EXPECT_OUTPUT_CONTAINS('WARNING: ' + messages[expect_tag]);
        // EXPECT_SHELL_LOG_NOT_CONTAINS("Info: Detected state of MD schema as");
    } else {
        EXPECT_THROWS(callback, expected_exception(expect_tag));
        EXPECT_OUTPUT_CONTAINS('ERROR: ' + messages[expect_tag]);
        // EXPECT_SHELL_LOG_CONTAINS("Info: Detected state of MD schema as");
    }
}

var current = testutil.getCurrentMetadataVersion();
var installed = testutil.getInstalledMetadataVersion();
print_debug("Base MD Version: " + installed)
var version = installed.split('.');

var major = parseInt(version[0]);
var minor = parseInt(version[1]);
var patch = parseInt(version[2]);

//@<> createCluster test: compatible metadata
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: hostname});
shell.connect(__sandbox_uri1);

var gr_group_name = recreate_cluster("sample", "Creating cluster with compatible metadata.")
var cluster = dba.getCluster()

//*** GET CLUSTER TESTS ***//
//@<> getCluster test: non COMPATIBLE metadata
var tests = [
    [major, minor, patch - 1, "PATCH_LOWER", true],
    [major, minor, patch + 1, "PATCH_HIGHER", false],
    [major, minor - 1, patch, "MINOR_LOWER", true],
    [major, minor + 1, patch, "MINOR_HIGHER", false],
    [major - 1, minor, patch, "MAJOR_LOWER", true],
    [major + 1, minor, patch, "MAJOR_HIGHER", false],
    [0, 0, 0, "FAILED_UPGRADE", false],
    [0, 0, 0, "UPGRADING", false],
];

for (index in tests) {
    var M = tests[index][0];
    var m = tests[index][1];
    var p = tests[index][2];

    print_debug(`MD Version: ${M}.${m}.${p} Shell Version: ${current} MD State: "${tests[index][3]}"`);

    var messages = format_messages(`${M}.${m}.${p}`, current);

    load_metadata_version(M, m, p, false, gr_group_name);

    var other_session;
    if (tests[index][3] == "FAILED_UPGRADE" || tests[index][3] == "UPGRADING") {
        other_session = mysql.getSession(__sandbox_uri1);
        // The upgrading state requires that a backup of the MD exists, otherwise is
        // considered failed upgrade
        other_session.runSql("CREATE SCHEMA mysql_innodb_cluster_metadata_bkp")
        other_session.runSql("CREATE SQL SECURITY INVOKER VIEW mysql_innodb_cluster_metadata_bkp.backup_stage (stage) AS SELECT 'UPGRADING'");
        other_session.runSql("CREATE VIEW mysql_innodb_cluster_metadata_bkp.schema_version (major, minor, patch) AS SELECT 1, 0, 1");
        if (tests[index][3] == "UPGRADING") {
            other_session.runSql("SELECT GET_LOCK('mysql_innodb_cluster_metadata.upgrade_in_progress', 1)")
        }
    }

    call_and_validate(function(){ var c = dba.getCluster('sample'); }, tests[index][3], tests[index][4]);

    if (tests[index][3] == "FAILED_UPGRADE" || tests[index][3] == "UPGRADING") {
        other_session.runSql("DROP SCHEMA mysql_innodb_cluster_metadata_bkp")
        other_session.close();
    }
}

//@<> getCluster test: compatible metadata
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: hostname});
testutil.deploySandbox(__mysql_sandbox_port2, "root", {report_host: hostname});
shell.connect(__sandbox_uri1);

var gr_group_name = recreate_cluster("sample", "Creating cluster with compatible metadata.");

load_metadata_version(major, minor, patch, false, gr_group_name);

EXPECT_NO_THROWS(function () {
    dba.getCluster('sample')
}, "Retrieving cluster with compatible metadata.");

//@<> getCluster test: incompatible errors
// These error expectations are for cluster functions, the ones for getCluster
// must be adjusted based on them
var tests = [
    [major, minor, patch - 1, ""],
    [major, minor, patch + 1, "PATCH_HIGHER"],
    [major, minor - 1, patch, ""],
    [major, minor + 1, patch, "MINOR_HIGHER"],
    [major - 1, minor, patch, ""],
    [major + 1, minor, patch, "MAJOR_HIGHER"],
    [0, 0, 0, "FAILED_UPGRADE"],
    [0, 0, 0, "UPGRADING"],
];

for (index in tests) {
    // We get the cluster handle first, the rest of the tests will be done using
    // this handle but tweaking the metadata version

    var M = tests[index][0];
    var m = tests[index][1];
    var p = tests[index][2];

    print_debug(`MD Version: ${M}.${m}.${p} Shell Version: ${current} MD State: "${tests[index][3]}"`);

    var upgrading_version = tests[index][0] == 0 && tests[index][1] == 0 && tests[index][2] == 0;

    var messages = format_messages(`${M}.${m}.${p}`, current);

    if (!upgrading_version && tests[index][3].endsWith("_HIGHER")) {
        load_metadata_version(M, m, p, false, gr_group_name);

        EXPECT_THROWS(function(){ dba.getCluster(); }, expected_exception(tests[index][3]));
        EXPECT_OUTPUT_CONTAINS(messages[tests[index][3]]);
        continue;
    }

    // Sets the version to the current one to ensure this getCluster succeeds
    if (upgrading_version) {
        load_metadata_version(1, 0, 1, false, gr_group_name);
        cluster = dba.getCluster();
        set_metadata_version(M,m,p);
    } else {
        load_metadata_version(M, m, p, false, gr_group_name);
        cluster = dba.getCluster();
    }

    testutil.wipeAllOutput();

    var other_session;
    if (tests[index][3] == "FAILED_UPGRADE" || tests[index][3] == "UPGRADING") {
        other_session = mysql.getSession(__sandbox_uri1);
        other_session.runSql("CREATE SCHEMA mysql_innodb_cluster_metadata_bkp");
        other_session.runSql("CREATE SQL SECURITY INVOKER VIEW mysql_innodb_cluster_metadata_bkp.backup_stage (stage) AS SELECT 'UPGRADING'");
        other_session.runSql("CREATE VIEW mysql_innodb_cluster_metadata_bkp.schema_version (major, minor, patch) AS SELECT 1, 0, 1");
        if (tests[index][3] == "UPGRADING") {
            other_session.runSql("SELECT GET_LOCK('mysql_innodb_cluster_metadata.upgrade_in_progress', 1)")
        }
    }

    var ro_tests = [
        function () {
            print_debug("Cluster.status()");
            testutil.wipeAllOutput();
            println(cluster.status())
        },
        function () {
            print_debug("Cluster.options()");
            testutil.wipeAllOutput();
            println(cluster.options())
        },
        function () {
            print_debug("Cluster.describe()");
            testutil.wipeAllOutput();
            println(cluster.describe())
        }
    ]

    // Once a cluster is retrieved, the read only functions should work
    // without throwing errors or printing any warning
    for (ro_index in ro_tests) {
        if (upgrading_version) {
            EXPECT_THROWS(ro_tests[ro_index], expected_exception(tests[index][3]));
            EXPECT_OUTPUT_CONTAINS(messages[tests[index][3]]);
        } else {
            EXPECT_NO_THROWS(ro_tests[ro_index]);
        }
    }

    // On the last test releases the locks by closing the other session
    // And also, recreates the cluster for the next round of tests
    // When no failure was expected
    print_debug("Upgrading: " + upgrading_version)
    if (tests[index][3] == "FAILED_UPGRADE" || tests[index][3] == "UPGRADING") {
        other_session.runSql("DROP SCHEMA mysql_innodb_cluster_metadata_bkp");
        other_session.close();
    }
}

print_debug("DONE!");


//@<> Finalization
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);

// The file only exist when not in replay mode, so error is ignored
try {testutil.rmfile(metadata_current_file)} catch (err) {}
