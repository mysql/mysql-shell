//@<> INCLUDE metadata_schema_utils.inc

//@<> Initialization and supporting code
metadata_1_0_1_file = get_1_0_1_snapshot_file();
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

function format_messages(installed, current) {
    return {
        MAJOR_HIGHER_ERROR:
        `Operation not allowed. The installed metadata version ${installed} is higher than the supported by the Shell which is version ${current}. Use a Shell version that supports this metadata version execute this operation.`,
        MAJOR_LOWER_ERROR:
        `Operation not allowed. The installed metadata version ${installed} is lower than the version required by Shell which is version ${current}. Upgrade the metadata to execute this operation. See \\? dba.upgradeMetadata for additional details.`,
        MAJOR_HIGHER_WARNING:
        `No cluster change operations can be executed because the installed metadata version ${installed} is higher than the supported by the Shell which is version ${current}. Use a Shell version that supports this metadata version to remove this restriction.`,
        MAJOR_LOWER_WARNING:
        `No cluster change operations can be executed because the installed metadata version ${installed} is lower than the version required by Shell which is version ${current}. Upgrade the metadata to remove this restriction. See \\? dba.upgradeMetadata for additional details.`,
        MINOR_LOWER_NOTE:
        `The installed metadata version ${installed} is lower than the version required by Shell which is version ${current}. It is recommended to upgrade the metadata. See \\? dba.upgradeMetadata for additional details.`,
        PATCH_LOWER_NOTE:
        `The installed metadata version ${installed} is lower than the version required by Shell which is version ${current}. It is recommended to upgrade the metadata. See \\? dba.upgradeMetadata for additional details.`,
        CLUSTER_MAJOR_HIGHER_ERROR:
        `Operation not allowed. No cluster change operations can be executed because the installed metadata version ${installed} is higher than the supported by the Shell which is version ${current}. Use a Shell version that supports this metadata version to remove this restriction.`,
        CLUSTER_MAJOR_LOWER_ERROR:
        `Operation not allowed. No cluster change operations can be executed because the installed metadata version ${installed} is lower than the version required by Shell which is version ${current}. Upgrade the metadata to remove this restriction. See \\? dba.upgradeMetadata for additional details.`,
        FAILED_UPGRADE_ERROR:
        "An unfinished metadata upgrade was detected, which may have left it in an invalid state. Execute dba.upgradeMetadata again to repair it.",
        UPGRADING_ERROR:
        "The metadata is being upgraded. Wait until the upgrade process completes and then retry the operation."
    }
}


//@<> Initialize to the point we have an instance with a metadata but not a cluster
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: hostname});
shell.connect(__sandbox_uri1);


var cluster = dba.createCluster("sample");
backup_metadata(__sandbox_uri1, metadata_current_file);

cluster.dissolve({ force: true });
session.runSql("SET GLOBAL super_read_only=0");

// Calls an API function that should succeed and verifies the expected WARNING/NOTE if any
function call_and_validate(callback, expect_tag) {
    testutil.wipeAllOutput();
    EXPECT_NO_THROWS(callback);

    if (expect_tag.endsWith('WARNING')) {
        EXPECT_NEXT_OUTPUT('WARNING: ' + messages[expect_tag]);
    } else if (expect_tag.endsWith('NOTE')) {
        EXPECT_NEXT_OUTPUT('NOTE: ' + messages[expect_tag]);
    }
}

var current = testutil.getCurrentMetadataVersion();
var installed = testutil.getInstalledMetadataVersion();
print_debug("Base MD Version: " + installed)
var version = installed.split('.');

var major = parseInt(version[0]);
var minor = parseInt(version[1]);
var patch = parseInt(version[2]);

//@<> createCluster test: incompatible errors
//*** CREATE CLUSTER TESTS ***//
var tests = [
    [major, minor, patch - 1, "PATCH_LOWER_NOTE"],
    [major, minor, patch + 1, ""],
    [major, minor - 1, patch, "MINOR_LOWER_NOTE"],
    [major, minor + 1, patch, ""],
    [major + 1, minor, patch, "MAJOR_HIGHER_ERROR"],
    [major - 1, minor, patch, "MAJOR_LOWER_ERROR"],
    [0, 0, 0, "FAILED_UPGRADE_ERROR"],
    [0, 0, 0, "UPGRADING_ERROR"],
];

for (index in tests) {
    var M = tests[index][0];
    var m = tests[index][1];
    var p = tests[index][2];

    var messages = format_messages(`${M}.${m}.${p}`, current);

    load_metadata_version(M, m, p, true);

    var other_session;
    if (tests[index][3] == "UPGRADING_ERROR") {
        other_session = mysql.getSession(__sandbox_uri1);
        // The upgrading state requires that a backup of the MD exists, otherwise is
        // considered failed upgrade
        other_session.runSql("CREATE SCHEMA mysql_innodb_cluster_metadata_bkp");
        other_session.runSql("CREATE SQL SECURITY INVOKER VIEW mysql_innodb_cluster_metadata_bkp.backup_stage (stage) AS SELECT 'UPGRADING'");
        other_session.runSql("SELECT GET_LOCK('mysql_innodb_cluster_metadata.upgrade_in_progress', 1)");
    }

    print_debug(tests[index]);
    if (tests[index][3].endsWith('ERROR')) {
        EXPECT_THROWS(function () {
            dba.createCluster('sample')
        }, messages[tests[index][3]]);
    } else {
        call_and_validate(function () {
            var cluster = dba.createCluster('sample');
            cluster.dissolve({ force: true });
            session.runSql("SET GLOBAL super_read_only=0");
        }, tests[index][3]);
    }

    if (tests[index][3] == "UPGRADING_ERROR") {
        other_session.runSql("DROP SCHEMA mysql_innodb_cluster_metadata_bkp")
        other_session.close();
    }
}

//@<> createCluster test: compatible metadata
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: hostname});
shell.connect(__sandbox_uri1);

var gr_group_name = recreate_cluster("sample", "Creating cluster with compatible metadata.")
var cluster = dba.getCluster()

//*** GET CLUSTER TESTS ***//
//@<> getCluster test: non COMPATIBLE metadata
var tests = [
    [major, minor, patch - 1, "PATCH_LOWER_NOTE"],
    [major, minor, patch + 1, ""],
    [major, minor - 1, patch, "MINOR_LOWER_NOTE"],
    [major, minor + 1, patch, ""],
    [major + 1, minor, patch, "MAJOR_HIGHER_WARNING"],
    [major - 1, minor, patch, "MAJOR_LOWER_WARNING"],
    [0, 0, 0, "FAILED_UPGRADE_ERROR"],
    [0, 0, 0, "UPGRADING_ERROR"],
];

for (index in tests) {
    var M = tests[index][0];
    var m = tests[index][1];
    var p = tests[index][2];

    print_debug(`MD Version: ${M}.${m}.${p} Shell Version: ${current}`);

    var messages = format_messages(`${M}.${m}.${p}`, current);

    load_metadata_version(M, m, p, false, gr_group_name);

    var other_session;
    if (tests[index][3] == "UPGRADING_ERROR") {
        other_session = mysql.getSession(__sandbox_uri1);
        // The upgrading state requires that a backup of the MD exists, otherwise is
        // considered failed upgrade
        other_session.runSql("CREATE SCHEMA mysql_innodb_cluster_metadata_bkp")
        other_session.runSql("CREATE SQL SECURITY INVOKER VIEW mysql_innodb_cluster_metadata_bkp.backup_stage (stage) AS SELECT 'UPGRADING'");
        other_session.runSql("SELECT GET_LOCK('mysql_innodb_cluster_metadata.upgrade_in_progress', 1)")
    }

    if (tests[index][3].endsWith("ERROR")) {
        EXPECT_THROWS(function () {
            var c = dba.getCluster('sample')
        }, messages[tests[index][3]]);
    } else {
        call_and_validate(function() {var c = dba.getCluster('sample')}, tests[index][3]);
    }

    if (tests[index][4] == "UPGRADING_ERROR") {
        other_session.runSql("DROP SCHEMA mysql_innodb_cluster_metadata_bkp")
        other_session.close();
    }
}

//@<> getCluster test: compatible metadata
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: hostname});
// Sandbox to be used in Cluster.checkInstanceState
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
    [major, minor, patch + 1, ""],
    [major, minor - 1, patch, ""],
    [major, minor + 1, patch, ""],
    [major + 1, minor, patch, "CLUSTER_MAJOR_HIGHER_ERROR"],
    [major - 1, minor, patch, "CLUSTER_MAJOR_LOWER_ERROR"],
    [0, 0, 0, "FAILED_UPGRADE_ERROR"],
    [0, 0, 0, "UPGRADING_ERROR"],
];

for (index in tests) {
    // We get the cluster handle first, the rest of the tests will be done using
    // this handle but tweaking the metadata version

    var M = tests[index][0];
    var m = tests[index][1];
    var p = tests[index][2];

    var upgrading_version = tests[index][0] == 0 && tests[index][1] == 0 && tests[index][2] == 0;

    print_debug(`MD Version: ${M}.${m}.${p} Shell Version: ${current} MD State: "${tests[index][3]}"`);
    var messages = format_messages(`${M}.${m}.${p}`, current);

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
    if (tests[index][3] == "UPGRADING_ERROR") {
        other_session = mysql.getSession(__sandbox_uri1);
        other_session.runSql("CREATE SCHEMA mysql_innodb_cluster_metadata_bkp");
        other_session.runSql("CREATE SQL SECURITY INVOKER VIEW mysql_innodb_cluster_metadata_bkp.backup_stage (stage) AS SELECT 'UPGRADING'");
        other_session.runSql("SELECT GET_LOCK('mysql_innodb_cluster_metadata.upgrade_in_progress', 1)")
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
        },
        function () {
            print_debug("Cluster.checkInstanceState()");
            testutil.wipeAllOutput();
            println(cluster.checkInstanceState(__sandbox_uri2));
        }
    ]

    // Once a cluster is retrieved, the read only functions should work
    // without throwing errors or printing any warning
    for (ro_index in ro_tests) {
        if (upgrading_version) {
            EXPECT_THROWS(ro_tests[ro_index], messages[tests[index][3]]);
        } else {
            EXPECT_NO_THROWS(ro_tests[ro_index]);
        }
    }

    var not_ro_tests = [
        function () {
            print_debug("Cluster.dissolve()");
            cluster.dissolve()
            session.runSql("SET GLOBAL super_read_only=0");
        },
        function () {
            print_debug("Cluster.addInstance()");
            cluster.addInstance(__sandbox_uri2)
        },
        function () {
            print_debug("Cluster.removeInstance()");
            cluster.removeInstance(__sandbox_uri1)
        },
        function () {
            print_debug("Cluster.rejoinInstance()");
            cluster.rejoinInstance(__sandbox_uri1)
        },
        function () {
            print_debug("Cluster.rescan()");
            cluster.rescan()
        },
        function () {
            print_debug("Cluster.forceQuorumUsingPartitionOf()");
            cluster.forceQuorumUsingPartitionOf(__sandbox_uri1)
        },
        function () {
            print_debug("Cluster.switchToSinglePrimaryMode()");
            cluster.switchToSinglePrimaryMode(__sandbox_uri1)
        },
        function () {
            print_debug("Cluster.switchToMultiPrimaryMode()");
            cluster.switchToMultiPrimaryMode()
        },
        function () {
            print_debug("Cluster.setPrimaryInstance()");
            cluster.setPrimaryInstance(__sandbox_uri1)
        },
        function () {
            print_debug("Cluster.setOption()");
            cluster.setOption('sample', 'option')
        },
        function () {
            print_debug("Cluster.setInstanceOption()");
            cluster.setInstanceOption(__sandbox_uri1, 'sample', 'option')
        },
        function () {
            print_debug("Cluster.resetRecoveryAccountsPassword()");
            cluster.resetRecoveryAccountsPassword()
        }
    ]

    // Once a cluster is retrieved, the non read only functions should not
    // be allowed if the schema is incompatible
    for (not_ro_index in not_ro_tests) {
        if (tests[index][3].endsWith("ERROR")) {
            // If a metadata precondition error is expected
            EXPECT_THROWS(not_ro_tests[not_ro_index], messages[tests[index][3]]);
        } else {
            // No metadata precondition error is expected
            if (not_ro_index == 0) {
                // First function should succeed as it is dissolve()
                EXPECT_NO_THROWS(not_ro_tests[not_ro_index]);
            } else {
                // The rest of the functions should fail as the cluster is dissolved, this means
                // the metadata precondition passed the check
                EXPECT_THROWS(not_ro_tests[not_ro_index], "on a dissolved cluster");
            }
        }
    }

    // On the last test releases the locks by closing the other session
    // And also, recreates the cluster for the next round of tests
    // When no failure was expected
    print_debug("Upgrading: " + upgrading_version)
    if (tests[index][3] == "UPGRADING_ERROR") {
        other_session.runSql("DROP SCHEMA mysql_innodb_cluster_metadata_bkp");
        other_session.close();
    } else if (tests[index][3] == "") {
        gr_group_name = recreate_cluster("sample", "Recreating cluster for cluster tests.");
    }
}

print_debug("DONE!");


//@<> Finalization
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);

// The file only exist when not in replay mode, so error is ignored
try {testutil.rmfile(metadata_current_file)} catch (err) {}
