//@<> Initialize to the point we have an instance with a metadata but not a cluster
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: hostname});

shell.connect(__sandbox_uri1);

var cluster = dba.createCluster("myCluster");

cluster.dissolve({ force: true });
session.runSql("SET GLOBAL super_read_only=0");

function set_metadata_version(major, minor, patch) {
    session.runSql("DROP VIEW mysql_innodb_cluster_metadata.schema_version");
    session.runSql("CREATE VIEW mysql_innodb_cluster_metadata.schema_version (major, minor, patch) AS SELECT ?, ?, ?", [major, minor, patch]);
}

var debug = false;
function print_debug(text) {
    if (debug) {
        println(text);
    }
}

function format_messages(installed, current) {
    return {
        MAJOR_HIGHER_ERROR:
        `Operation not allowed. The installed metadata version ${installed} is higher than the supported by the Shell which is version ${current}. Use a Shell version that supports this metadata version execute this operation.`,
        MAJOR_LOWER_ERROR:
        `Operation not allowed. The installed metadata version ${installed} is lower than the supported by the Shell which is version ${current}. Upgrade the metadata to execute this operation. See \\? dba.upgradeMetadata for additional details.`,
        MAJOR_HIGHER_WARNING:
        `The cluster is in READ ONLY mode because the installed metadata version ${installed} is higher than the supported by the Shell which is version ${current}. Use a Shell version that supports this metadata version to remove this restriction.`,
        MAJOR_LOWER_WARNING:
        `The cluster is in READ ONLY mode because the installed metadata version ${installed} is lower than the supported by the Shell which is version ${current}. Upgrade the metadata to remove this restriction. See \\? dba.upgradeMetadata for additional details.`,
        CLUSTER_MAJOR_HIGHER:
        `Operation not allowed. The cluster is in READ ONLY mode because the installed metadata version ${installed} is higher than the supported by the Shell which is version ${current}. Use a Shell version that supports this metadata version to remove this restriction.`,
        CLUSTER_MAJOR_LOWER:
        `Operation not allowed. The cluster is in READ ONLY mode because the installed metadata version ${installed} is lower than the supported by the Shell which is version ${current}. Upgrade the metadata to remove this restriction. See \\? dba.upgradeMetadata for additional details.`,
        FAILED_UPGRADE:
        "The installed metadata is unreliable because of a failed upgrade. It is recommended to restore the metadata by executing dba.upgradeMetadata with the restore option.",
        UPGRADING:
        "The metadata is being upgraded. Wait until the upgrade process completes and then retry the operation."
    }
}

var current = testutil.getCurrentMetadataVersion();
var installed = testutil.getInstalledMetadataVersion();
var version = installed.split('.');

var major = parseInt(version[0]);
var minor = parseInt(version[1]);
var patch = parseInt(version[2]);


//@<> createCluster test: incompatible errors
//*** CREATE CLUSTER TESTS ***//
var tests = [
    [major, minor, patch - 1, ""],
    [major, minor, patch + 1, ""],
    [major, minor - 1, patch, ""],
    [major, minor + 1, patch, ""],
    [major - 1, minor, patch, "MAJOR_LOWER_ERROR"],
    [major + 1, minor, patch, "MAJOR_HIGHER_ERROR"],
    [0, 0, 0, "FAILED_UPGRADE"],
    [0, 0, 0, "UPGRADING"],
];

for (index in tests) {
    var M = tests[index][0];
    var m = tests[index][1];
    var p = tests[index][2];

    var messages = format_messages(`${M}.${m}.${p}`, current);

    set_metadata_version(M, m, p);

    var other_session;
    if (tests[index][3] == "UPGRADING") {
        other_session = mysql.getSession(__sandbox_uri1);
        other_session.runSql("SELECT GET_LOCK('mysql_innodb_cluster_metadata.upgrade_in_progress', 1)")
    }

    print_debug(tests[index]);
    if (tests[index][3] != "") {
        EXPECT_THROWS(function () {
            dba.createCluster('sample')
        }, messages[tests[index][3]]);
    } else {
        EXPECT_NO_THROWS(function () {
            var cluster = dba.createCluster('sample');
            cluster.dissolve({ force: true });
            session.runSql("SET GLOBAL super_read_only=0");
        });
        EXPECT_OUTPUT_NOT_CONTAINS('WARNING: The cluster is in READ ONLY');
    }

    if (tests[index][3] == "UPGRADING") {
        other_session.close();
    }
}

//@<> createCluster test: compatible metadata
set_metadata_version(major, minor, patch);
EXPECT_NO_THROWS(function () {
    dba.createCluster('sample')
}, "Creating cluster with compatible metadata.");


//*** GET CLUSTER TESTS ***//
//@<> getCluster test: non COMPATIBLE metadata
var expect_nothing = 0;
var expect_error = 1;
var expect_warning = 2;
var tests = [
    [major, minor, patch - 1, expect_nothing, ""],
    [major, minor, patch + 1, expect_nothing, ""],
    [major, minor - 1, patch, expect_nothing, ""],
    [major, minor + 1, patch, expect_nothing, ""],
    [major - 1, minor, patch, expect_warning, "MAJOR_LOWER_WARNING"],
    [major + 1, minor, patch, expect_warning, "MAJOR_HIGHER_WARNING"],
    [0, 0, 0, expect_error, "FAILED_UPGRADE"],
    [0, 0, 0, expect_error, "UPGRADING"],
];

for (index in tests) {
    var M = tests[index][0];
    var m = tests[index][1];
    var p = tests[index][2];

    print_debug(`MD Version: ${M}.${m}.${p} Shell Version: ${current}`);

    var messages = format_messages(`${M}.${m}.${p}`, current);

    set_metadata_version(M, m, p);

    var other_session;
    if (tests[index][4] == "UPGRADING") {
        other_session = mysql.getSession(__sandbox_uri1);
        other_session.runSql("SELECT GET_LOCK('mysql_innodb_cluster_metadata.upgrade_in_progress', 1)")
    }

    if (tests[index][3] == expect_error) {
        EXPECT_THROWS(function () {
            var c = dba.getCluster('sample')
        }, messages[tests[index][4]]);
    } else if (tests[index][3] == expect_warning) {
        testutil.wipeAllOutput();
        var c = dba.getCluster('sample');
        EXPECT_NEXT_OUTPUT('WARNING: ' + messages[tests[index][4]]);
    } else {
        EXPECT_NO_THROWS(function () { var c = dba.getCluster('sample') });
        EXPECT_OUTPUT_NOT_CONTAINS('WARNING: The cluster is in READ ONLY');
    }

    if (tests[index][4] == "UPGRADING") {
        other_session.close();
    }
}

//@<> getCluster test: compatible metadata
set_metadata_version(major, minor, patch);
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
    [major - 1, minor, patch, "CLUSTER_MAJOR_LOWER"],
    [major + 1, minor, patch, "CLUSTER_MAJOR_HIGHER"],
    [0, 0, 0, "FAILED_UPGRADE"],
    [0, 0, 0, "UPGRADING"],
];

for (index in tests) {
    // We get the cluster handle first, the rest of the tests will be done using
    // this handle but tweaking the metadata version

    var M = tests[index][0];
    var m = tests[index][1];
    var p = tests[index][2];

    var upgrading_version = tests[index][0] == 0 && tests[index][1] == 0 && tests[index][2] == 0;

    // Sets the version to the current one to ensure this getCluster succeeds
    set_metadata_version(major, minor, patch);
    var cluster = dba.getCluster();

    print_debug(`MD Version: ${M}.${m}.${p} Shell Version: ${current}`);

    var messages = format_messages(`${M}.${m}.${p}`, current);

    // Nos sets the version to the one being tested
    set_metadata_version(M, m, p);
    testutil.wipeAllOutput();

    var other_session;
    if (tests[index][3] == "UPGRADING") {
        other_session = mysql.getSession(__sandbox_uri1);
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
        }
    ]

    // Once a cluster is retrieved, the read only functions should work 
    // without throwing errors or printing any warning
    for (ro_index in ro_tests) {
        if (upgrading_version) {
            EXPECT_THROWS(ro_tests[ro_index], messages[tests[index][3]]);
        } else {
            EXPECT_NO_THROWS(ro_tests[ro_index]);
            EXPECT_OUTPUT_NOT_CONTAINS('WARNING: The cluster is in READ ONLY');
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
        }
    ]

    // Once a cluster is retrieved, the non read only functions should not
    // be allowed if the schema is incompatible
    for (not_ro_index in not_ro_tests) {
        if (tests[index][3] != "") {
            // If a metadata precondition error is expected
            EXPECT_THROWS(not_ro_tests[not_ro_index], messages[tests[index][3]]);
        } else {
            // No metadata precondition error is expected
            if (not_ro_index == 0) {
                // First function should succeed as it is dissolve()
                EXPECT_NO_THROWS(not_ro_tests[not_ro_index]);
                EXPECT_OUTPUT_NOT_CONTAINS('WARNING: The cluster is in READ ONLY');
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
    if (tests[index][3] == "UPGRADING") {
        other_session.close();
    } else if (tests[index][3] == "") {
        set_metadata_version(major, minor, patch);
        EXPECT_NO_THROWS(function () {
            dba.createCluster('sample')
        }, "Recreating cluster for cluster tests.");
    }
}



testutil.destroySandbox(__mysql_sandbox_port1);