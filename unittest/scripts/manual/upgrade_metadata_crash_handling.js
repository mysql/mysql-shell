// Tests handling of crashes and DB errors during a MD schema upgrade.
// This will load a MD 1.0.1, try to upgrade it, fail after X DB queries and
// try to restore. The final state should be either a reverted or completed
// upgrade.

// Run with mysqlshrec -f <thisfile>, from the build dir.
// To customize whatthe script will do, go to the CUSTOMIZE section below
function prepare() {
    println("\033[1;31mPREPARING", "\033[0m");
    session.runSql("DROP SCHEMA IF EXISTS mysql_innodb_cluster_metadata");
    session.runSql("DROP SCHEMA IF EXISTS mysql_innodb_cluster_metadata_bkp");
    session.runSql("DROP SCHEMA IF EXISTS mysql_innodb_cluster_metadata_previous");
    session.runSql("CREATE SCHEMA mysql_innodb_cluster_metadata");
    testutil.importData(url, "metadata.sql", "mysql_innodb_cluster_metadata");
}


function upgrade_and_fail(num_queries, test_crash) {
    var failure = ""
    if (test_crash)
        failure = "sql_test_abort";
    else
        failure = "sql_test_error";

    println("\033[1;31mUPGRADING", "\033[0m");
    // perform upgrade and ensure it crashes after X queries
    r=testutil.callMysqlsh([url, "--js", "--verbose=3", "--dba-log-sql=2",
                    "--debug=+d,"+failure, 
                    "--", "dba", "upgrade-metadata"], 
                    "", ["TEST_SQL_UNTIL_CRASH="+num_queries], mysqlshrec);
    println("\033[1;31mUPGRADE RC=",r, "\033[0m");
    return r;
}

function recover() {
    println("\033[1;31mRECOVERING", "\033[0m");

    r = testutil.callMysqlsh([url, "--js", "--verbose=2", "--dba-log-sql=2",
        "--", "dba", "upgrade-metadata"]);

    println("\033[1;31mRECOVERY RC=",r, "\033[0m");

    if (r==1) {
        shell.prompt("Failed recovery scenario, time to debug...");
    }
}

function recover_and_fail(num_queries, test_crash) {
    println("\033[1;31mRECOVERING", "\033[0m");
    var failure = ""
    if (test_crash)
        failure = "sql_test_abort";
    else
        failure = "sql_test_error";

    r = testutil.callMysqlsh([url, "--js", "--verbose=2", "--dba-log-sql=2",
        "--debug=+d,"+failure, 
        "--", "dba", "upgrade-metadata"],
        "", ["TEST_SQL_UNTIL_CRASH="+num_queries], mysqlshrec);

    println("\033[1;31mRECOVERY RC=",r, "\033[0m");

    return r;
}


function check_state() {
    println("\033[1;31mCHECKING CONSISTENCY", "\033[0m");
    // ensure that the MD schema is consistent
    try {
        c = dba.getCluster();
        c.status();
        return 0;
    } catch (error) {
        println("\033[1;31mERROR:", error, "\033[0m");
        return -1;
    }
}

//=========== CUSTOMIZE ===============
// This script could test both failures (exceptions) or crashes, this flag is used for that.
var testing_crash = true;

// ~180 SQL Statements are required for complete testing of the upgrade process
// ~100 SQL Statements are required for complete testing of the rollback process
// ~60 SQL Statements are required for complete testing of the rollforward process
var testing_upgrade = true;
var start_at = 101;
var end_at = 200;

// It is possible to test the restore process from 2 points
// Rollback, data has been copied but upgrade state did not change to complete
// Rollforward, upgrade process reached the DONE state but did not complete
var test_rollback = false;

// Holds the numbers of queries required to reach the DONE state, this constant
// needs to be updated if the MD upgrade logic changes
var DONE_STATE_QUERIES=172

// To explore the state at a specific number of queries on the upgrade process
// turn on this flag. You will be prompted for the number of queries to be executed
// before stopping so you can explore the state, when done enter a negative number
var explore_state = false;


// Port in which the sandbox will be created
port = 3000;

// Path to the mysqlsh binary to create schema 1.0.1
mysqlsh_old = "/usr/bin/mysqlsh";

// Path to the mysqlshrec executable
mysqlshrec = "bin/mysqlshrec";

//=========== EXECUTION CONFIGURATION ===============
if (explore_state) {
    println("MODE: Upgrade State EXplore");
    println("TODO: Provide the number of queries to execute on the prompt, enter negative number to finish.");
} else {
    if (testing_upgrade) {
        println("MODE: Testing Metadata Upgrade");
    } else {
        if (test_rollback) {
            println("MODE: Testing Upgrade Recovery: Rollback");
        } else {
            println("MODE: Testing Upgrade Recovery: Rollforward");
        }
    }
    println(`Start At Query: ${start_at}`);
    println(`End At Query: ${end_at}`);
}

shell.prompt("Hit ENTER to continue or Ctrl+C to cancel...")

//=========== EXECUTION STARTS ===============
url = "root:root@localhost:"+port;

println("Deploying test sandbox...");
var query_log = testutil.getSandboxLogPath(port).replace("error", "query");
testutil.deploySandbox(port, "root", {general_log:1});
shell.connect(url);

// create a cluster with the old MD
println("Creating test cluster...");
testutil.callMysqlsh([url, "--js", "--", "dba", "create-cluster", "mycluster"], "", [], mysqlsh_old);

println("Backing up metadata schema...");
testutil.dumpData(url, "metadata.sql", ["mysql_innodb_cluster_metadata"]);


if (explore_state) {
    var queries = 0;
    while(queries >= 0) {
        queries = parseInt(shell.prompt("Number of queries? "))
        if (queries > 0) {
            prepare();
            upgrade_and_fail(queries, true)
        }
    }
} else {
    var test_description = ""
    if (testing_crash)
        test_description = "CRASH"
    else
        test_description = "FAILURE"

    if (testing_upgrade) {
        done = 0;
        for (i = start_at; i <= end_at && !done; i++) {
            println("\033[1;31m#### TEST UPGRADE", test_description, "AFTER", i, "\033[0m");
            prepare();

            if (upgrade_and_fail(i, testing_crash) == 0) {
                done = 1;
            } else {
                recover();
            }
            if (check_state() != 0) {
                println("\033[1;31mFAILED AT", i, "\033[0m");
                break;
            } else {
                println("\033[1;31mOK", "\033[0m");
            }
            println("\033[1;31m#########################", "\033[0m");
        }
    } else {
        done = 0;
        for (i = start_at; i <= end_at && !done; i++) {
            println("\033[1;31m#### TEST RESTORE", test_description, "AFTER", i, "\033[0m");
            // DONE state is set on query #155 so to test full rollback we use 154
            // to test rollforward we use 155
            var queries = 0;
            if (test_rollback) {
                println("\033[1;31m#### ROLL BACK\033[0m");
                queries = DONE_STATE_QUERIES-1;
            } else {
                println("\033[1;31m#### ROLL FORWARD\033[0m");
                queries = DONE_STATE_QUERIES;
            }
            prepare();

            // This will let the upgrade in the desired state
            upgrade_and_fail(queries, testing_crash);

            // This will cause the recovery to fail after i statements
            if (recover_and_fail(i, testing_crash) == 0 ) {
                done = 1;
            } else {
                // This should succeed
                recover();
            }
            if (check_state() != 0) {
                println("\033[1;31mFAILED AT", i, "\033[0m");
                break;
            } else {
                println("\033[1;31mOK", "\033[0m");
            }
            println("\033[1;31m#########################", "\033[0m");
        }
    }
}


testutil.destroySandbox(port);