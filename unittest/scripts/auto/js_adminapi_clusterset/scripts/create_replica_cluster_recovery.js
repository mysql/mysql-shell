//@{!__dbug_off && VER(>=8.0.27)}

// Tests recovery method selection for createReplicaCluster() in ClusterSet

// Variables to combine and test:
// - (target instance has) empty GTIDs
// - (target instance has) GTIDs subset
// - (target instance has) errant GTIDs
// - missing GTIDs from 1 peer, all peers
//
// - recoveryMethod: incremental/clone/auto
// - interactive: true/false
//
// - gtidSetIsComplete: true/false
// - clone supported/not
//
// Recovery selection algorithm:
//
// InnoDB ClusterSet .createReplicaCluster()
// -----------------------------------------
// IF recoveryMethod = clone THEN
//     CONTINUE WITH CLONE
// ELSE IF recoveryMethod = incremental THEN
//     IF incremental_recovery_possible THEN
//         CONTINUE WITHOUT CLONE
//     ELSE
//         ERROR "Incremental recovery is not possible (GTID state not
//               compatible)"
// ELSE
//     IF incremental_recovery_possible THEN
//         IF incremental_recovery_safe OR gtidSetComplete THEN
//             CONTINUE WITHOUT CLONE
//         ELSE IF clone_supported THEN
//             PROMPT Clone, Recovery, Abort
//         ELSE
//             PROMPT Recovery, Abort
//    ELSE
//         IF clone_supported THEN
//             IF gtid-set empty THEN
//                 PROMPT Clone, Abort
//             ELSE
//                 CONTINUE WITH CLONE
//         ELSE
//             ABORT
//

function install_clone(session) {
    ensure_plugin_enabled("clone", session, "mysql_clone");
}

function uninstall_clone(session) {
    ensure_plugin_disabled("clone", session);
}

function clone_installed(session) {
    var row = session.runSql("select plugin_status from information_schema.plugins where plugin_name='clone'").fetchOne();
    if (row)
        return row[0] == "ACTIVE";
    return false;
}

function mark_gtid_set_complete(flag) {
    if (flag)
        session.runSql("UPDATE mysql_innodb_cluster_metadata.clusters SET attributes = JSON_SET(attributes, '$.opt_gtidSetIsComplete', true)");
    else
        session.runSql("UPDATE mysql_innodb_cluster_metadata.clusters SET attributes = JSON_REMOVE(attributes, '$.opt_gtidSetIsComplete')");
}


//@<> Setup + Create primary cluster
var scene = new ClusterScenario([__mysql_sandbox_port1, __mysql_sandbox_port2, __mysql_sandbox_port3]);
var session = scene.session
var cluster = scene.cluster
testutil.deploySandbox(__mysql_sandbox_port4, "root", {report_host: hostname});
var session4 = mysql.getSession(__sandbox_uri4);

//@<> createClusterSet()
var cluster_set = cluster.createClusterSet("myClusterSet");
var gtid_executed = session.runSql("SELECT @@global.gtid_executed").fetchOne()[0];
testutil.dbugSet("+d,dba_abort_create_replica_cluster");

//@<> invalid recoveryMethod (should fail)
EXPECT_THROWS_TYPE(function(){cluster_set.createReplicaCluster(__sandbox_uri4, "myReplicaCluster", {interactive: false, recoveryMethod: "foobar"})}, "Invalid value for option recoveryMethod: foobar", "ArgumentError");
EXPECT_THROWS_TYPE(function(){cluster_set.createReplicaCluster(__sandbox_uri4, "myReplicaCluster", {interactive: false, recoveryMethod: 1})}, "Option 'recoveryMethod' is expected to be of type String, but is Integer", "TypeError");

//@<> invalid cloneDonor (should fail)
EXPECT_THROWS_TYPE(function(){cluster_set.createReplicaCluster(__sandbox_uri4, "myReplicaCluster", {interactive: false, cloneDonor: "127.0.0.1:3306"})}, "Option cloneDonor only allowed if option recoveryMethod is used and set to 'clone'.", "ArgumentError");
EXPECT_THROWS_TYPE(function(){cluster_set.createReplicaCluster(__sandbox_uri4, "myReplicaCluster", {interactive: false, recoveryMethod: "incremental", cloneDonor: "127.0.0.1:3306"})}, "Option cloneDonor only allowed if option recoveryMethod is set to 'clone'.", "ArgumentError");
EXPECT_THROWS_TYPE(function(){cluster_set.createReplicaCluster(__sandbox_uri4, "myReplicaCluster", {interactive: false, recoveryMethod: "clone", cloneDonor: ":"})}, "Invalid value for cloneDonor: Invalid address format in ':'. Must be <host>:<port> or [<ip>]:<port> for IPv6 addresses", "ArgumentError");
EXPECT_THROWS_TYPE(function(){cluster_set.createReplicaCluster(__sandbox_uri4, "myReplicaCluster", {interactive: false, recoveryMethod: "clone", cloneDonor: ""})}, "Invalid value for cloneDonor, string value cannot be empty.", "ArgumentError");
EXPECT_THROWS_TYPE(function(){cluster_set.createReplicaCluster(__sandbox_uri4, "myReplicaCluster", {interactive: false, recoveryMethod: "clone", cloneDonor: "localhost:"})}, "Invalid value for cloneDonor: Invalid address format in 'localhost:'. Must be <host>:<port> or [<ip>]:<port> for IPv6 addresses", "ArgumentError");
EXPECT_THROWS_TYPE(function(){cluster_set.createReplicaCluster(__sandbox_uri4, "myReplicaCluster", {interactive: false, recoveryMethod: "clone", cloneDonor: ":3306"})}, "Invalid value for cloneDonor: Invalid address format in ':3306'. Must be <host>:<port> or [<ip>]:<port> for IPv6 addresses", "ArgumentError");

// Tests for recoveryMethod = clone
//
//   IF recoveryMethod = clone THEN
//      CONTINUE WITH CLONE

//@<> createReplicaCluster: recoveryMethod:clone, interactive, make sure no prompts
session4.runSql("RESET MASTER");
mark_gtid_set_complete(false);
EXPECT_THROWS_TYPE(function(){cluster_set.createReplicaCluster(__sandbox_uri4, "myReplicaCluster", {interactive: true, recoveryMethod: "clone"})}, "debug", "LogicError");

//@<> createReplicaCluster: recoveryMethod:clone, empty GTID -> clone
mark_gtid_set_complete(false);
EXPECT_THROWS_TYPE(function(){cluster_set.createReplicaCluster(__sandbox_uri4, "myReplicaCluster", {recoveryMethod: "clone"})}, "debug", "LogicError");
EXPECT_OUTPUT_CONTAINS("Clone based recovery selected through the recoveryMethod option");

//@<> createReplicaCluster: recoveryMethod:clone, empty GTIDs + gtidSetIsComplete -> clone
mark_gtid_set_complete(true);
EXPECT_THROWS_TYPE(function(){cluster_set.createReplicaCluster(__sandbox_uri4, "myReplicaCluster", {recoveryMethod: "clone"})}, "debug", "LogicError");
EXPECT_OUTPUT_CONTAINS("Clone based recovery selected through the recoveryMethod option");
mark_gtid_set_complete(false);

//@<> createReplicaCluster: recoveryMethod:clone, subset GTIDs -> clone
session4.runSql("RESET MASTER");
session4.runSql("SET GLOBAL gtid_purged=?", [gtid_executed]);
EXPECT_THROWS_TYPE(function(){cluster_set.createReplicaCluster(__sandbox_uri4, "myReplicaCluster", {recoveryMethod: "clone"})}, "debug", "LogicError");
EXPECT_OUTPUT_CONTAINS("Clone based recovery selected through the recoveryMethod option");

// Tests for recoveryMethod = incremental
//
// createReplicaCluster:
//
// ELSE IF recoveryMethod = incremental THEN
//     IF incremental_recovery_possible THEN
//         CONTINUE WITHOUT CLONE
//     ELSE
//         ERROR "Incremental recovery is not possible (GTID state not compatible)"
//

//@<> createReplicaCluster: recoveryMethod:incremental, interactive, make sure no prompts
session4.runSql("RESET MASTER");
mark_gtid_set_complete(false);
EXPECT_THROWS_TYPE(function(){cluster_set.createReplicaCluster(__sandbox_uri4, "myReplicaCluster", {recoveryMethod: "incremental"})}, "debug", "LogicError");

EXPECT_OUTPUT_CONTAINS(`Setting up replica 'myReplicaCluster' of cluster 'cluster' at instance '${__endpoint4}'.`);
EXPECT_OUTPUT_CONTAINS("* Checking transaction state of the instance...");
EXPECT_OUTPUT_CONTAINS(`NOTE: The target instance '${__endpoint4}' has not been pre-provisioned (GTID set is empty). The Shell is unable to decide whether replication can completely recover its state.`);
EXPECT_OUTPUT_CONTAINS("Incremental state recovery selected through the recoveryMethod option");

EXPECT_FALSE(clone_installed(session));
EXPECT_FALSE(clone_installed(session4));

//@<> createReplicaCluster: recoveryMethod:incremental, empty GTIDs + gtidSetIsComplete -> incr
mark_gtid_set_complete(true);
EXPECT_THROWS_TYPE(function(){cluster_set.createReplicaCluster(__sandbox_uri4, "myReplicaCluster", {recoveryMethod: "incremental"})}, "debug", "LogicError");

EXPECT_OUTPUT_CONTAINS(`Setting up replica 'myReplicaCluster' of cluster 'cluster' at instance '${__endpoint4}'.`);
EXPECT_OUTPUT_CONTAINS("* Checking transaction state of the instance...");
EXPECT_OUTPUT_CONTAINS(`NOTE: The target instance '${__endpoint4}' has not been pre-provisioned (GTID set is empty), but the clusterset was configured to assume that replication can completely recover the state of new instances.`);
EXPECT_OUTPUT_CONTAINS("Incremental state recovery selected through the recoveryMethod option");

EXPECT_FALSE(clone_installed(session));
EXPECT_FALSE(clone_installed(session4));

//@<> createReplicaCluster: recoveryMethod:incremental, subset GTIDs -> incr
session4.runSql("RESET MASTER");
session4.runSql("SET GLOBAL gtid_purged=?", [gtid_executed]);
EXPECT_THROWS_TYPE(function(){cluster_set.createReplicaCluster(__sandbox_uri4, "myReplicaCluster", {recoveryMethod: "incremental"})}, "debug", "LogicError");

EXPECT_OUTPUT_CONTAINS(`Setting up replica 'myReplicaCluster' of cluster 'cluster' at instance '${__endpoint4}'.`);
EXPECT_OUTPUT_CONTAINS("* Checking transaction state of the instance...");
EXPECT_OUTPUT_CONTAINS("Incremental state recovery selected through the recoveryMethod option");

EXPECT_FALSE(clone_installed(session));
EXPECT_FALSE(clone_installed(session4));

//@<> createReplicaCluster: recoveryMethod:incremental, errant GTIDs -> error
session4.runSql("RESET MASTER");
session4.runSql("SET GLOBAL gtid_purged=?", [gtid_executed+",00025721-1111-1111-1111-111111111111:1"]);
EXPECT_THROWS_TYPE(function(){cluster_set.createReplicaCluster(__sandbox_uri4, "myReplicaCluster", {recoveryMethod: "incremental"})}, "Cannot use recoveryMethod=incremental option because the GTID state is not compatible or cannot be recovered.", "RuntimeError");

EXPECT_OUTPUT_CONTAINS(`WARNING: A GTID set check of the MySQL instance at '${__endpoint4}' determined that it contains transactions that do not originate from the clusterset, which must be discarded before it can join the clusterset.`);

EXPECT_FALSE(clone_installed(session));
EXPECT_FALSE(clone_installed(session4));

// Tests for recoveryMethod = auto
//
// ELSE
//      IF incremental_recovery_possible THEN
//          IF incremental_recovery_safe OR gtidSetComplete THEN
//              CONTINUE WITHOUT CLONE
//          ELSE IF clone_supported THEN
//              PROMPT Clone, Recovery, Abort
//          ELSE
//              PROMPT Recovery, Abort
//      ELSE
//          IF clone_supported THEN
//              PROMPT Clone, Abort
//          ELSE
//              ABORT

//@<> createReplicaCluster: recoveryMethod:auto, interactive, empty GTID -> prompt c/i/a
testutil.expectPrompt("Please select a recovery method [C]lone/[I]ncremental recovery/[A]bort (default Clone): ", "i");
session4.runSql("RESET MASTER");
mark_gtid_set_complete(false);
EXPECT_THROWS_TYPE(function(){cluster_set.createReplicaCluster(__sandbox_uri4, "myReplicaCluster", {interactive: true})}, "debug", "LogicError");

EXPECT_OUTPUT_CONTAINS(`Setting up replica 'myReplicaCluster' of cluster 'cluster' at instance '${__endpoint4}'.`);
EXPECT_OUTPUT_CONTAINS("* Checking transaction state of the instance...");
EXPECT_OUTPUT_CONTAINS(`NOTE: The target instance '${__endpoint4}' has not been pre-provisioned (GTID set is empty). The Shell is unable to decide whether replication can completely recover its state.`);

EXPECT_FALSE(clone_installed(session));
EXPECT_FALSE(clone_installed(session4));

//@<> createReplicaCluster: recoveryMethod:auto, interactive, empty GTIDs + gtidSetIsComplete -> incr
mark_gtid_set_complete(true);
EXPECT_THROWS_TYPE(function(){cluster_set.createReplicaCluster(__sandbox_uri4, "myReplicaCluster", {interactive: true})}, "debug", "LogicError");

EXPECT_OUTPUT_CONTAINS(`Setting up replica 'myReplicaCluster' of cluster 'cluster' at instance '${__endpoint4}'.`);
EXPECT_OUTPUT_CONTAINS("* Checking transaction state of the instance...");
EXPECT_OUTPUT_CONTAINS("Incremental state recovery was selected because it seems to be safely usable.");

EXPECT_FALSE(clone_installed(session));
EXPECT_FALSE(clone_installed(session4));
mark_gtid_set_complete(false);

//@<> createReplicaCluster: recoveryMethod:auto, interactive, errant GTIDs -> prompt c/a
session4.runSql("RESET MASTER");
session4.runSql("SET GLOBAL gtid_purged=?", [gtid_executed+",00025721-1111-1111-1111-111111111111:1"]);
testutil.expectPrompt("Please select a recovery method [C]lone/[A]bort (default Abort): ", "a");

EXPECT_THROWS_TYPE(function(){cluster_set.createReplicaCluster(__sandbox_uri4, "myReplicaCluster", {interactive: true})}, "Cancelled", "RuntimeError");

EXPECT_OUTPUT_CONTAINS(`Setting up replica 'myReplicaCluster' of cluster 'cluster' at instance '${__endpoint4}'.`);
EXPECT_OUTPUT_CONTAINS("* Checking transaction state of the instance...");
EXPECT_OUTPUT_CONTAINS(`WARNING: A GTID set check of the MySQL instance at '${__endpoint4}' determined that it contains transactions that do not originate from the clusterset, which must be discarded before it can join the clusterset.`);
EXPECT_OUTPUT_CONTAINS(`${__endpoint4} has the following errant GTIDs that do not exist in the clusterset:`);
EXPECT_OUTPUT_CONTAINS("00025721-1111-1111-1111-111111111111:1");
EXPECT_OUTPUT_CONTAINS(`WARNING: Discarding these extra GTID events can either be done manually or by completely overwriting the state of ${__endpoint4} with a physical snapshot from an existing clusterset member. To use this method by default, set the 'recoveryMethod' option to 'clone'.`);
EXPECT_OUTPUT_CONTAINS("Having extra GTID events is not expected, and it is recommended to investigate this further and ensure that the data can be removed prior to choosing the clone recovery method.");

EXPECT_FALSE(clone_installed(session));
EXPECT_FALSE(clone_installed(session4));


// Non-interactive tests

//@<> createReplicaCluster: recoveryMethod:auto, non-interactive, empty GTID -> error
session4.runSql("RESET MASTER");
mark_gtid_set_complete(false);

EXPECT_THROWS_TYPE(function(){cluster_set.createReplicaCluster(__sandbox_uri4, "myReplicaCluster", {interactive: false})}, "'recoveryMethod' option must be set to 'clone' or 'incremental'", "RuntimeError");

EXPECT_OUTPUT_CONTAINS(`Setting up replica 'myReplicaCluster' of cluster 'cluster' at instance '${__endpoint4}'.`);
EXPECT_OUTPUT_CONTAINS("* Checking transaction state of the instance...");
EXPECT_OUTPUT_CONTAINS("WARNING: It should be safe to rely on replication to incrementally recover the state of the new Replica Cluster if you are sure all updates ever executed in the ClusterSet were done with GTIDs enabled, there are no purged transactions and the instance used to create the new Replica Cluster contains the same GTID set as the ClusterSet or a subset of it. To use this method by default, set the 'recoveryMethod' option to 'incremental'.");

EXPECT_FALSE(clone_installed(session));
EXPECT_FALSE(clone_installed(session4));


//@<> createReplicaCluster: recoveryMethod:auto, non-interactive, empty GTIDs + gtidSetIsComplete -> incr
mark_gtid_set_complete(true);

EXPECT_THROWS_TYPE(function(){cluster_set.createReplicaCluster(__sandbox_uri4, "myReplicaCluster", {interactive: false})}, "debug", "LogicError");

EXPECT_OUTPUT_CONTAINS(`Setting up replica 'myReplicaCluster' of cluster 'cluster' at instance '${__endpoint4}'.`);
EXPECT_OUTPUT_CONTAINS("* Checking transaction state of the instance...");
EXPECT_OUTPUT_CONTAINS(`NOTE: The target instance '${__endpoint4}' has not been pre-provisioned (GTID set is empty), but the clusterset was configured to assume that replication can completely recover the state of new instances.`);
EXPECT_OUTPUT_CONTAINS("Incremental state recovery was selected because it seems to be safely usable.");

EXPECT_FALSE(clone_installed(session));
EXPECT_FALSE(clone_installed(session4));
mark_gtid_set_complete(false);

//@<> createReplicaCluster: recoveryMethod:auto, non-interactive, subset GTIDs -> incr
session4.runSql("RESET MASTER");
session4.runSql("SET GLOBAL gtid_purged=?", [gtid_executed]);

EXPECT_THROWS_TYPE(function(){cluster_set.createReplicaCluster(__sandbox_uri4, "myReplicaCluster", {interactive: false})}, "debug", "LogicError");
EXPECT_OUTPUT_CONTAINS("WARNING: It should be safe to rely on replication to incrementally recover the state of the new Replica Cluster if you are sure all updates ever executed in the ClusterSet were done with GTIDs enabled, there are no purged transactions and the instance used to create the new Replica Cluster contains the same GTID set as the ClusterSet or a subset of it. To use this method by default, set the 'recoveryMethod' option to 'incremental'.");
EXPECT_OUTPUT_CONTAINS("Incremental state recovery was selected because it seems to be safely usable.");

EXPECT_FALSE(clone_installed(session));
EXPECT_FALSE(clone_installed(session4));


//@<> createReplicaCluster: recoveryMethod:auto, non-interactive, errant GTIDs -> error
session4.runSql("RESET MASTER");
session4.runSql("SET GLOBAL gtid_purged=?", [gtid_executed+",00025721-1111-1111-1111-111111111111:1"]);

EXPECT_THROWS_TYPE(function(){cluster_set.createReplicaCluster(__sandbox_uri4, "myReplicaCluster", {interactive: false})}, "Instance provisioning required", "MYSQLSH");
EXPECT_OUTPUT_CONTAINS(`${__endpoint4} has the following errant GTIDs that do not exist in the clusterset:`);
EXPECT_OUTPUT_CONTAINS("00025721-1111-1111-1111-111111111111:1");
EXPECT_OUTPUT_CONTAINS(`WARNING: Discarding these extra GTID events can either be done manually or by completely overwriting the state of ${__endpoint4} with a physical snapshot from an existing clusterset member. To use this method by default, set the 'recoveryMethod' option to 'clone'.`);
EXPECT_OUTPUT_CONTAINS("Having extra GTID events is not expected, and it is recommended to investigate this further and ensure that the data can be removed prior to choosing the clone recovery method.");
EXPECT_OUTPUT_CONTAINS("ERROR: The target instance must be either cloned or fully provisioned before it can be added to the target clusterset.");

EXPECT_FALSE(clone_installed(session));
EXPECT_FALSE(clone_installed(session4));


// interactive tests

// Tests with purged GTIDs
// -----------------------

//@<> purge GTIDs from the primary cluster
session.runSql("FLUSH BINARY LOGS");
session.runSql("PURGE BINARY LOGS BEFORE DATE_ADD(NOW(6), INTERVAL 1 DAY)");

session4.runSql("RESET MASTER");
session4.runSql("RESET MASTER");

//@<> createReplicaCluster: recoveryMethod:auto, interactive, purged GTID -> prompt c/a
testutil.expectPrompt("Please select a recovery method [C]lone/[A]bort (default Abort): ", "a");
mark_gtid_set_complete(false);

EXPECT_THROWS_TYPE(function(){cluster_set.createReplicaCluster(__sandbox_uri4, "myReplicaCluster", {interactive: true})}, "Cancelled", "RuntimeError");

EXPECT_OUTPUT_CONTAINS(`NOTE: A GTID set check of the MySQL instance at '${__endpoint4}' determined that it is missing transactions that were purged from all clusterset members.`);
EXPECT_OUTPUT_CONTAINS(`NOTE: The target instance '${__endpoint4}' has not been pre-provisioned (GTID set is empty). The Shell is unable to decide whether clone based recovery is safe to use.`);

//@<> createReplicaCluster: recoveryMethod:auto, no-interactive, purged GTID -> error
mark_gtid_set_complete(false);

EXPECT_THROWS_TYPE(function(){cluster_set.createReplicaCluster(__sandbox_uri4, "myReplicaCluster", {interactive: false})}, "Instance provisioning required", "MYSQLSH");

EXPECT_OUTPUT_CONTAINS(`NOTE: A GTID set check of the MySQL instance at '${__endpoint4}' determined that it is missing transactions that were purged from all clusterset members.`);
EXPECT_OUTPUT_CONTAINS(`NOTE: The target instance '${__endpoint4}' has not been pre-provisioned (GTID set is empty). The Shell is unable to decide whether clone based recovery is safe to use.`);

//@<> createReplicaCluster: recoveryMethod:auto, interactive, purged GTID, subset gtid -> clone, no prompt
mark_gtid_set_complete(false);
session4.runSql("RESET MASTER");
session4.runSql("SET GLOBAL gtid_purged=?", [gtid_executed]);

EXPECT_THROWS_TYPE(function(){cluster_set.createReplicaCluster(__sandbox_uri4, "myReplicaCluster", {interactive: true})}, "debug", "LogicError");

EXPECT_OUTPUT_CONTAINS(`NOTE: A GTID set check of the MySQL instance at '${__endpoint4}' determined that it is missing transactions that were purged from all clusterset members.`);
EXPECT_OUTPUT_CONTAINS("Clone based recovery was selected because it seems to be safely usable.");

//@<> createReplicaCluster: recoveryMethod:auto, no-interactive, purged GTID, subset gtid -> clone, no prompt
mark_gtid_set_complete(false);
session4.runSql("RESET MASTER");
session4.runSql("SET GLOBAL gtid_purged=?", [gtid_executed]);

EXPECT_THROWS_TYPE(function(){cluster_set.createReplicaCluster(__sandbox_uri4, "myReplicaCluster", {interactive: false})}, "debug", "LogicError");

EXPECT_OUTPUT_CONTAINS(`NOTE: A GTID set check of the MySQL instance at '${__endpoint4}' determined that it is missing transactions that were purged from all clusterset members.`);
EXPECT_OUTPUT_CONTAINS("Clone based recovery was selected because it seems to be safely usable.");


//@<> createReplicaCluster: recoveryMethod:auto, interactive, errant GTIDs + purged GTIDs -> prompt c/a
session4.runSql("RESET MASTER");
session4.runSql("SET GLOBAL gtid_purged=?", ["00025721-1111-1111-1111-111111111111:1"]);
testutil.expectPrompt("Please select a recovery method [C]lone/[A]bort (default Abort): ", "a");

EXPECT_THROWS_TYPE(function(){cluster_set.createReplicaCluster(__sandbox_uri4, "myReplicaCluster", {interactive: true})}, "Cancelled", "RuntimeError");
EXPECT_OUTPUT_CONTAINS(`${__endpoint4} has the following errant GTIDs that do not exist in the clusterset:`);
EXPECT_OUTPUT_CONTAINS("00025721-1111-1111-1111-111111111111:1");
EXPECT_OUTPUT_CONTAINS(`WARNING: Discarding these extra GTID events can either be done manually or by completely overwriting the state of ${__endpoint4} with a physical snapshot from an existing clusterset member. To use this method by default, set the 'recoveryMethod' option to 'clone'.`);
EXPECT_OUTPUT_CONTAINS("Having extra GTID events is not expected, and it is recommended to investigate this further and ensure that the data can be removed prior to choosing the clone recovery method.");

//@<> createReplicaCluster: recoveryMethod:incremental, purged GTID -> error
session4.runSql("RESET MASTER");
mark_gtid_set_complete(false);

EXPECT_THROWS_TYPE(function(){cluster_set.createReplicaCluster(__sandbox_uri4, "myReplicaCluster", {recoveryMethod: "incremental"})}, "Cannot use recoveryMethod=incremental option because the GTID state is not compatible or cannot be recovered.", "RuntimeError");
EXPECT_OUTPUT_CONTAINS(`NOTE: A GTID set check of the MySQL instance at '${__endpoint4}' determined that it is missing transactions that were purged from all clusterset members.`);

//@<> createReplicaCluster: recoveryMethod:incremental, errant GTIDs + purged GTIDs -> error
session4.runSql("RESET MASTER");
session4.runSql("SET GLOBAL gtid_purged=?", ["00025721-1111-1111-1111-111111111111:1"]);

EXPECT_THROWS_TYPE(function(){cluster_set.createReplicaCluster(__sandbox_uri4, "myReplicaCluster", {recoveryMethod: "incremental"})}, "Cannot use recoveryMethod=incremental option because the GTID state is not compatible or cannot be recovered.", "RuntimeError");
EXPECT_OUTPUT_CONTAINS(`${__endpoint4} has the following errant GTIDs that do not exist in the clusterset:`);
EXPECT_OUTPUT_CONTAINS("00025721-1111-1111-1111-111111111111:1");

//@<> createReplicaCluster: recoveryMethod:clone, purged GTID -> clone {VER(>=8.0.17)}
session4.runSql("RESET MASTER");
mark_gtid_set_complete(false);
EXPECT_THROWS_TYPE(function(){cluster_set.createReplicaCluster(__sandbox_uri4, "clone", {recoveryMethod: "clone"})}, "debug", "LogicError");

EXPECT_OUTPUT_CONTAINS(`NOTE: A GTID set check of the MySQL instance at '${__endpoint4}' determined that it is missing transactions that were purged from all clusterset members.`);
EXPECT_OUTPUT_CONTAINS("Clone based recovery selected through the recoveryMethod option");


//@<> createReplicaCluster: recoveryMethod:clone, errant GTIDs + purged GTIDs -> clone
session4.runSql("RESET MASTER");
session4.runSql("SET GLOBAL gtid_purged=?", [gtid_executed+",00025721-1111-1111-1111-111111111111:1"]);
EXPECT_THROWS_TYPE(function(){cluster_set.createReplicaCluster(__sandbox_uri4, "clone", {recoveryMethod: "clone"})}, "debug", "LogicError");

EXPECT_OUTPUT_CONTAINS(`${__endpoint4} has the following errant GTIDs that do not exist in the clusterset:`);
EXPECT_OUTPUT_CONTAINS("00025721-1111-1111-1111-111111111111:1");
EXPECT_OUTPUT_CONTAINS("Clone based recovery selected through the recoveryMethod option");

// TODO(miguel): add tests for cloneDonor

//@<> Cleanup
// Disable debug
testutil.dbugSet("");

session.close();
session4.close();
scene.destroy();
testutil.destroySandbox(__mysql_sandbox_port4);
testutil.dbugSet("");
