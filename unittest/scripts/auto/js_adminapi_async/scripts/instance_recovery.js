//@{!__dbug_off}

// Tests recovery method selection for addInstance() and rejoinInstance() in ReplicaSets

// Variables to combine and test:
// - (joiner has) empty GTIDs
// - (joiner has) GTIDs subset
// - (joiner has) errant GTIDs
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
// InnoDB ReplicaSet .addInstance()
// --------------------------------
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
//             PROMPT Clone, Abort
//         ELSE
//             ABORT
//
// InnoDB ReplicaSet .rejoinInstance()
// -----------------------------------
// IF recoveryMethod = clone THEN
//      CONTINUE WITH CLONE
//  ELSE IF recoveryMethod = incremental THEN
//      IF incremental_recovery_possible AND incremental_recovery_safe THEN
//          CONTINUE WITHOUT CLONE
//      ELSE
//          ERROR "Recovery is not possible (GTID state not compatible)"
//  ELSE
//      IF incremental_recovery_possible THEN
//          IF incremental_recovery_safe OR gtidSetComplete THEN
//              CONTINUE WITHOUT CLONE
//          ELSE
//              IF clone_supported THEN
//                  PROMPT Clone, Abort
//              ELSE
//                  Abort
//      ELSE
//          IF clone_supported and GTID-set is not Diverged THEN
//              PROMPT Clone, Abort
//          ELSE IF clone_supported and GTID-set Diverged THEN
//              ERROR: The instance has a diverged GTID-set. (indicate that
//                     something went out of our control and show the gtid-set
//                     that makes it incompatible)
//          ELSE ABORT

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

//@<> Setup {VER(>= 8.0.0)
testutil.deploySandbox(__mysql_sandbox_port1, "root", {"report_host": hostname_ip});
testutil.deploySandbox(__mysql_sandbox_port2, "root", {"report_host": hostname_ip});
testutil.deploySandbox(__mysql_sandbox_port3, "root", {"report_host": hostname_ip});

session1 = mysql.getSession(__sandbox_uri1);
session2 = mysql.getSession(__sandbox_uri2);
session3 = mysql.getSession(__sandbox_uri3);

//@ prepare {VER(>= 8.0.0)}
shell.connect(__sandbox_uri1);
rs = dba.createReplicaSet("myrs");
rs.addInstance(__sandbox_uri3, {recoveryMethod: "incremental"});

// STOP SLAVE on instance 3 to simulate the rejoins
session3.runSql("STOP SLAVE");

var gtid_executed = session.runSql("SELECT @@global.gtid_executed").fetchOne()[0];

testutil.dbugSet("+d,dba_abort_async_add_replica");
testutil.dbugSet("+d,dba_abort_async_rejoin_replica");

//@ invalid recoveryMethod (should fail) {VER(>= 8.0.0)}
rs.addInstance(__sandbox_uri2, {interactive: false, recoveryMethod: "foobar"});
rs.addInstance(__sandbox_uri2, {interactive: false, recoveryMethod: 1});
rs.rejoinInstance(__sandbox_uri3, {interactive: false, recoveryMethod: "foobar"});
rs.rejoinInstance(__sandbox_uri3, {interactive: false, recoveryMethod: 1});

//@ invalid cloneDonor (should fail) {VER(>= 8.0.19)}
rs.addInstance(__sandbox_uri2, {interactive: false, cloneDonor: "127.0.0.1:3306"});
rs.addInstance(__sandbox_uri2, {interactive: false, recoveryMethod: "incremental", cloneDonor: "127.0.0.1:3306"});
rs.addInstance(__sandbox_uri2, {interactive: false, recoveryMethod: "clone", cloneDonor: ":"});
rs.addInstance(__sandbox_uri2, {interactive: false, recoveryMethod: "clone", cloneDonor: ""});
rs.addInstance(__sandbox_uri2, {interactive: false, recoveryMethod: "clone", cloneDonor: "localhost:"});
rs.addInstance(__sandbox_uri2, {interactive: false, recoveryMethod: "clone", cloneDonor: ":3306"});
rs.rejoinInstance(__sandbox_uri3, {interactive: false, cloneDonor: "127.0.0.1:3306"});
rs.rejoinInstance(__sandbox_uri3, {interactive: false, recoveryMethod: "incremental", cloneDonor: "127.0.0.1:3306"});
rs.rejoinInstance(__sandbox_uri3, {interactive: false, recoveryMethod: "clone", cloneDonor: ":"});
rs.rejoinInstance(__sandbox_uri3, {interactive: false, recoveryMethod: "clone", cloneDonor: ""});
rs.rejoinInstance(__sandbox_uri3, {interactive: false, recoveryMethod: "clone", cloneDonor: "localhost:"});
rs.rejoinInstance(__sandbox_uri3, {interactive: false, recoveryMethod: "clone", cloneDonor: ":3306"});

//@ invalid recoveryMethod (should fail if target instance does not support it) {VER(>= 8.0.0) && VER(< 8.0.17)}
rs.addInstance(__sandbox_uri2, {interactive: false, recoveryMethod: "clone"});
rs.rejoinInstance(__sandbox_uri3, {interactive: false, recoveryMethod: "clone"});

// Tests for recoveryMethod = clone
//
//   IF recoveryMethod = clone THEN
//      CONTINUE WITH CLONE

//@ addInstance: recoveryMethod:clone, interactive, make sure no prompts {VER(>=8.0.17)}
session2.runSql("RESET MASTER");
mark_gtid_set_complete(false);
rs.addInstance(__sandbox_uri2, {interactive:true, recoveryMethod:"clone"});

//@ rejoinInstance: recoveryMethod:clone, interactive, make sure no prompts {VER(>=8.0.17)}
session3.runSql("RESET MASTER");
mark_gtid_set_complete(false);
rs.rejoinInstance(__sandbox_uri3, {interactive:true, recoveryMethod:"clone"});

//@ addInstance: recoveryMethod:clone, empty GTID -> clone {VER(>=8.0.17)}
mark_gtid_set_complete(false);
rs.addInstance(__sandbox_uri2, {recoveryMethod:"clone"});

//@ rejoinInstance: recoveryMethod:clone, empty GTID -> clone {VER(>=8.0.17)}
mark_gtid_set_complete(false);
rs.rejoinInstance(__sandbox_uri3, {recoveryMethod:"clone"});

//@ addInstance: recoveryMethod:clone, empty GTIDs + gtidSetIsComplete -> clone {VER(>=8.0.17)}
mark_gtid_set_complete(true);
rs.addInstance(__sandbox_uri2, {recoveryMethod: "clone"});
mark_gtid_set_complete(false);

//@ rejoinInstance: recoveryMethod:clone, empty GTIDs + gtidSetIsComplete -> clone {VER(>=8.0.17)}
mark_gtid_set_complete(true);
rs.rejoinInstance(__sandbox_uri3, {recoveryMethod: "clone"});
mark_gtid_set_complete(false);

//@ addInstance: recoveryMethod:clone, subset GTIDs -> clone {VER(>=8.0.17)}
session2.runSql("RESET MASTER");
session2.runSql("SET GLOBAL gtid_purged=?", [gtid_executed]);
rs.addInstance(__sandbox_uri2, {recoveryMethod: "clone"});

//@ rejoinInstance: recoveryMethod:clone, subset GTIDs -> clone {VER(>=8.0.17)}
session3.runSql("RESET MASTER");
session3.runSql("SET GLOBAL gtid_purged=?", [gtid_executed]);
rs.rejoinInstance(__sandbox_uri3 , {recoveryMethod: "clone"});

// Tests for recoveryMethod = incremental
//
// addInstance:
//
// ELSE IF recoveryMethod = incremental THEN
//     IF incremental_recovery_possible THEN
//         CONTINUE WITHOUT CLONE
//     ELSE
//         ERROR "Incremental recovery is not possible (GTID state not compatible)"
//
// rejoinInstance:
//
// ELSE IF recoveryMethod = incremental THEN
//     IF incremental_recovery_possible AND incremental_recovery_safe THEN
//         CONTINUE WITHOUT CLONE
//     ELSE
//         ERROR "Incremental Recovery is not possible (GTID state not compatible)"

//@ addInstance: recoveryMethod:incremental, interactive, make sure no prompts {VER(>= 8.0.0)}
session2.runSql("RESET MASTER");
mark_gtid_set_complete(false);
rs.addInstance(__sandbox_uri2, {interactive:true, recoveryMethod:"incremental"});
EXPECT_FALSE(clone_installed(session1));
EXPECT_FALSE(clone_installed(session2));

//@ rejoinInstance: recoveryMethod:incremental, interactive, error {VER(>= 8.0.0)}
session3.runSql("RESET MASTER");
mark_gtid_set_complete(false);
rs.rejoinInstance(__sandbox_uri3, {interactive:true, recoveryMethod:"incremental"});

//@ addInstance: recoveryMethod:incremental, empty GTIDs + gtidSetIsComplete -> incr {VER(>= 8.0.0)}
mark_gtid_set_complete(true);
rs.addInstance(__sandbox_uri2, {recoveryMethod: "incremental"});
EXPECT_FALSE(clone_installed(session1));
EXPECT_FALSE(clone_installed(session2));

//@ rejoinInstance: recoveryMethod:incremental, empty GTIDs + gtidSetIsComplete -> incr {VER(>= 8.0.0)}
mark_gtid_set_complete(true);
rs.rejoinInstance(__sandbox_uri3, {recoveryMethod: "incremental"});
EXPECT_FALSE(clone_installed(session1));
EXPECT_FALSE(clone_installed(session2));
EXPECT_FALSE(clone_installed(session3));

//@ addInstance: recoveryMethod:incremental, subset GTIDs -> incr {VER(>= 8.0.0)}
session2.runSql("RESET MASTER");
session2.runSql("SET GLOBAL gtid_purged=?", [gtid_executed]);
rs.addInstance(__sandbox_uri2, {recoveryMethod:"incremental"});
EXPECT_FALSE(clone_installed(session1));
EXPECT_FALSE(clone_installed(session2));

//@ rejoinInstance: recoveryMethod:incremental, subset GTIDs -> incr {VER(>= 8.0.0)}
session3.runSql("RESET MASTER");
session3.runSql("SET GLOBAL gtid_purged=?", [gtid_executed]);
rs.rejoinInstance(__sandbox_uri3, {recoveryMethod:"incremental"});
EXPECT_FALSE(clone_installed(session1));
EXPECT_FALSE(clone_installed(session2));
EXPECT_FALSE(clone_installed(session3));

//@ addInstance: recoveryMethod:incremental, errant GTIDs -> error {VER(>= 8.0.0)}
session2.runSql("RESET MASTER");
session2.runSql("SET GLOBAL gtid_purged=?", [gtid_executed+",00025721-1111-1111-1111-111111111111:1"]);
rs.addInstance(__sandbox_uri2, {recoveryMethod:"incremental"});
EXPECT_FALSE(clone_installed(session1));
EXPECT_FALSE(clone_installed(session2));

//@ rejoinInstance: recoveryMethod:incremental, errant GTIDs -> error {VER(>= 8.0.0)}
session3.runSql("RESET MASTER");
session3.runSql("SET GLOBAL gtid_purged=?", [gtid_executed+",00025721-1111-1111-1111-111111111111:1"]);
rs.rejoinInstance(__sandbox_uri3, {recoveryMethod:"incremental"});
EXPECT_FALSE(clone_installed(session1));
EXPECT_FALSE(clone_installed(session2));
EXPECT_FALSE(clone_installed(session3));


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

shell.connect(__sandbox_uri1);

//@ addInstance: recoveryMethod:auto, interactive, clone unavailable, empty GTID -> prompt i/a {VER(>= 8.0.0) && VER(< 8.0.17)}
testutil.expectPrompt("Please select a recovery method [I]ncremental recovery/[A]bort (default Incremental recovery): ", "a");
session2.runSql("RESET MASTER");
mark_gtid_set_complete(false);
rs.addInstance(__sandbox_uri2, {interactive: true});
EXPECT_FALSE(clone_installed(session1));
EXPECT_FALSE(clone_installed(session2));

//@ rejoinInstance: recoveryMethod:auto, interactive, clone unavailable, empty GTID -> error {VER(>= 8.0.0) && VER(< 8.0.17)}
session3.runSql("RESET MASTER");
mark_gtid_set_complete(false);
rs.rejoinInstance(__sandbox_uri3, {interactive: true});
EXPECT_FALSE(clone_installed(session1));
EXPECT_FALSE(clone_installed(session2));
EXPECT_FALSE(clone_installed(session3));

//@ addInstance: recoveryMethod:auto, interactive, empty GTID -> prompt c/i/a {VER(>= 8.0.19)}
testutil.expectPrompt("Please select a recovery method [C]lone/[I]ncremental recovery/[A]bort (default Clone): ", "i");
session2.runSql("RESET MASTER");
mark_gtid_set_complete(false);
rs.addInstance(__sandbox_uri2, {interactive: true});
EXPECT_FALSE(clone_installed(session1));
EXPECT_FALSE(clone_installed(session2));

//@ rejoinInstance: recoveryMethod:auto, interactive, empty GTID -> prompt c/a {VER(>= 8.0.19)}
testutil.expectPrompt("Please select a recovery method [C]lone/[A]bort (default Abort): ", "c");
session3.runSql("RESET MASTER");
mark_gtid_set_complete(false);
rs.rejoinInstance(__sandbox_uri3, {interactive: true});
EXPECT_FALSE(clone_installed(session1));
EXPECT_FALSE(clone_installed(session2));
EXPECT_FALSE(clone_installed(session3));

//@ addInstance: recoveryMethod:auto, interactive, empty GTIDs + gtidSetIsComplete -> incr {VER(>= 8.0.19)}
mark_gtid_set_complete(true);
rs.addInstance(__sandbox_uri2, {interactive: true});
EXPECT_FALSE(clone_installed(session1));
EXPECT_FALSE(clone_installed(session2));
mark_gtid_set_complete(false);

//@ rejoinInstance: recoveryMethod:auto, interactive, empty GTIDs + gtidSetIsComplete -> incr {VER(>= 8.0.19)}
mark_gtid_set_complete(true);
rs.rejoinInstance(__sandbox_uri3, {interactive: true});
EXPECT_FALSE(clone_installed(session1));
EXPECT_FALSE(clone_installed(session2));
EXPECT_FALSE(clone_installed(session3));
mark_gtid_set_complete(false);

//@ addInstance: recoveryMethod:auto, interactive, errant GTIDs -> prompt c/a {VER(>= 8.0.19)}
session2.runSql("RESET MASTER");
session2.runSql("SET GLOBAL gtid_purged=?", [gtid_executed+",00025721-1111-1111-1111-111111111111:1"]);
testutil.expectPrompt("Please select a recovery method [C]lone/[A]bort (default Abort): ", "a");
rs.addInstance(__sandbox_uri2, {interactive:true});
EXPECT_FALSE(clone_installed(session1));
EXPECT_FALSE(clone_installed(session2));

//@ rejoinInstance: recoveryMethod:auto, interactive, errant GTIDs -> error {VER(>= 8.0.19)}
session3.runSql("RESET MASTER");
session3.runSql("SET GLOBAL gtid_purged=?", [gtid_executed+",00025721-1111-1111-1111-111111111111:1"]);
rs.rejoinInstance(__sandbox_uri3, {interactive:true});
EXPECT_FALSE(clone_installed(session1));
EXPECT_FALSE(clone_installed(session2));
EXPECT_FALSE(clone_installed(session3));

//@ addInstance: recoveryMethod:auto, interactive, errant GTIDs -> error clone not supported {VER(>= 8.0.0) && VER(< 8.0.17)}
session2.runSql("RESET MASTER");
session2.runSql("SET GLOBAL gtid_purged=?", [gtid_executed+",00025721-1111-1111-1111-111111111111:1"]);
rs.addInstance(__sandbox_uri2, {interactive:true});
EXPECT_FALSE(clone_installed(session1));
EXPECT_FALSE(clone_installed(session2));

//@ rejoinInstance: recoveryMethod:auto, interactive, errant GTIDs -> error clone not supported {VER(>= 8.0.0) && VER(< 8.0.17)}
session3.runSql("RESET MASTER");
session3.runSql("SET GLOBAL gtid_purged=?", [gtid_executed+",00025721-1111-1111-1111-111111111111:1"]);
rs.rejoinInstance(__sandbox_uri3, {interactive:true});
EXPECT_FALSE(clone_installed(session1));
EXPECT_FALSE(clone_installed(session2));
EXPECT_FALSE(clone_installed(session3));

// Non-interactive tests

//@ addInstance: recoveryMethod:auto, non-interactive, empty GTID -> error {VER(>= 8.0.19)}
session2.runSql("RESET MASTER");
mark_gtid_set_complete(false);
rs.addInstance(__sandbox_uri2, {interactive: false});
EXPECT_FALSE(clone_installed(session1));
EXPECT_FALSE(clone_installed(session2));

//@ rejoinInstance: recoveryMethod:auto, non-interactive, empty GTID -> error {VER(>= 8.0.19)}
session3.runSql("RESET MASTER");
mark_gtid_set_complete(false);
rs.rejoinInstance(__sandbox_uri3, {interactive: false});
EXPECT_FALSE(clone_installed(session1));
EXPECT_FALSE(clone_installed(session2));
EXPECT_FALSE(clone_installed(session3));

//@ addInstance: recoveryMethod:auto, non-interactive, clone not supported, empty GTID -> error {VER(>= 8.0.0) && VER(< 8.0.17)}
session2.runSql("RESET MASTER");
mark_gtid_set_complete(false);
rs.addInstance(__sandbox_uri2, {interactive:false});
EXPECT_FALSE(clone_installed(session1));
EXPECT_FALSE(clone_installed(session2));

//@ rejoinInstance: recoveryMethod:auto, non-interactive, clone not supported, empty GTID -> error {VER(>= 8.0.0) && VER(< 8.0.17)}
session3.runSql("RESET MASTER");
mark_gtid_set_complete(false);
rs.rejoinInstance(__sandbox_uri3, {interactive:false});
EXPECT_FALSE(clone_installed(session1));
EXPECT_FALSE(clone_installed(session2));
EXPECT_FALSE(clone_installed(session3));

//@ addInstance: recoveryMethod:auto, non-interactive, empty GTIDs + gtidSetIsComplete -> incr {VER(>=8.0.17)}
mark_gtid_set_complete(true);
rs.addInstance(__sandbox_uri2, {interactive:false});
EXPECT_FALSE(clone_installed(session1));
EXPECT_FALSE(clone_installed(session2));
mark_gtid_set_complete(false);

//@ rejoinInstance: recoveryMethod:auto, non-interactive, empty GTIDs + gtidSetIsComplete -> incr {VER(>=8.0.17)}
mark_gtid_set_complete(true);
rs.rejoinInstance(__sandbox_uri3, {interactive:false});
EXPECT_FALSE(clone_installed(session1));
EXPECT_FALSE(clone_installed(session2));
EXPECT_FALSE(clone_installed(session3));
mark_gtid_set_complete(false);

//@ addInstance: recoveryMethod:auto, non-interactive, subset GTIDs -> incr {VER(>=8.0.17)}
session2.runSql("RESET MASTER");
session2.runSql("SET GLOBAL gtid_purged=?", [gtid_executed]);
rs.addInstance(__sandbox_uri2, {interactive:false});
EXPECT_FALSE(clone_installed(session1));
EXPECT_FALSE(clone_installed(session2));

//@ rejoinInstance: recoveryMethod:auto, non-interactive, subset GTIDs -> incr {VER(>=8.0.17)}
session3.runSql("RESET MASTER");
session3.runSql("SET GLOBAL gtid_purged=?", [gtid_executed]);
rs.rejoinInstance(__sandbox_uri3, {interactive:false});
EXPECT_FALSE(clone_installed(session1));
EXPECT_FALSE(clone_installed(session2));
EXPECT_FALSE(clone_installed(session3));

//@ addInstance: recoveryMethod:auto, non-interactive, errant GTIDs -> error {VER(>=8.0.0)}
session2.runSql("RESET MASTER");
session2.runSql("SET GLOBAL gtid_purged=?", [gtid_executed+",00025721-1111-1111-1111-111111111111:1"]);
rs.addInstance(__sandbox_uri2, {interactive:false});
EXPECT_FALSE(clone_installed(session1));
EXPECT_FALSE(clone_installed(session2));

//@ rejoinInstance: recoveryMethod:auto, non-interactive, errant GTIDs -> error {VER(>=8.0.0)}
session3.runSql("RESET MASTER");
session3.runSql("SET GLOBAL gtid_purged=?", [gtid_executed+",00025721-1111-1111-1111-111111111111:1"]);
rs.rejoinInstance(__sandbox_uri3, {interactive:false});
EXPECT_FALSE(clone_installed(session1));
EXPECT_FALSE(clone_installed(session2));
EXPECT_FALSE(clone_installed(session3));

// interactive tests


// Tests with purged GTIDs
// -----------------------

//@<> purge GTIDs from replicaset {VER(>= 8.0.0)}
session.runSql("FLUSH BINARY LOGS");
session.runSql("PURGE BINARY LOGS BEFORE DATE_ADD(NOW(6), INTERVAL 1 DAY)");
session2.runSql("RESET MASTER");
session3.runSql("RESET MASTER");

//@ addInstance: recoveryMethod:auto, interactive, purged GTID -> prompt c/a {VER(>=8.0.17)}
testutil.expectPrompt("Please select a recovery method [C]lone/[A]bort (default Abort): ", "a");
mark_gtid_set_complete(false);
rs.addInstance(__sandbox_uri2, {interactive: true});

//@ rejoinInstance: recoveryMethod:auto, interactive, purged GTID -> prompt c/a {VER(>=8.0.17)}
testutil.expectPrompt("Please select a recovery method [C]lone/[A]bort (default Abort): ", "a");
mark_gtid_set_complete(false);
rs.rejoinInstance(__sandbox_uri3, {interactive: true});

//@ addInstance: recoveryMethod:auto, interactive, errant GTIDs + purged GTIDs -> prompt c/a {VER(>=8.0.17)}
session2.runSql("RESET MASTER");
session2.runSql("SET GLOBAL gtid_purged=?", ["00025721-1111-1111-1111-111111111111:1"]);
testutil.expectPrompt("Please select a recovery method [C]lone/[A]bort (default Abort): ", "a");
rs.addInstance(__sandbox_uri2, {interactive: true});

//@ rejoinInstance: recoveryMethod:auto, interactive, errant GTIDs + purged GTIDs -> error {VER(>=8.0.17)}
session3.runSql("RESET MASTER");
session3.runSql("SET GLOBAL gtid_purged=?", ["00025721-1111-1111-1111-111111111111:1"]);
rs.rejoinInstance(__sandbox_uri3, {interactive: true});

//@ addInstance: recoveryMethod:incremental, purged GTID -> error {VER(>= 8.0.0)}
session2.runSql("RESET MASTER");
mark_gtid_set_complete(false);
rs.addInstance(__sandbox_uri2, {recoveryMethod: "incremental"});

//@ rejoinInstance: recoveryMethod:incremental, purged GTID -> error {VER(>= 8.0.0)}
session3.runSql("RESET MASTER");
mark_gtid_set_complete(false);
rs.rejoinInstance(__sandbox_uri3, {recoveryMethod: "incremental"});

//@ addInstance: recoveryMethod:incremental, errant GTIDs + purged GTIDs -> error {VER(>= 8.0.0)}
session2.runSql("RESET MASTER");
session2.runSql("SET GLOBAL gtid_purged=?", ["00025721-1111-1111-1111-111111111111:1"]);
rs.addInstance(__sandbox_uri2, {recoveryMethod: "incremental"});

//@ rejoinInstance: recoveryMethod:incremental, errant GTIDs + purged GTIDs -> error {VER(>= 8.0.0)}
session3.runSql("RESET MASTER");
session3.runSql("SET GLOBAL gtid_purged=?", ["00025721-1111-1111-1111-111111111111:1"]);
rs.rejoinInstance(__sandbox_uri3, {recoveryMethod: "incremental"});

//@ addInstance: recoveryMethod:clone, purged GTID -> clone {VER(>=8.0.17)}
session2.runSql("RESET MASTER");
mark_gtid_set_complete(false);
rs.addInstance(__sandbox_uri2, {recoveryMethod:"clone"});

//@ rejoinInstance: recoveryMethod:clone, purged GTID -> clone {VER(>=8.0.17)}
session3.runSql("RESET MASTER");
mark_gtid_set_complete(false);
rs.rejoinInstance(__sandbox_uri3, {recoveryMethod:"clone"});

//@ addInstance: recoveryMethod:clone, errant GTIDs + purged GTIDs -> clone {VER(>=8.0.17)}
session2.runSql("RESET MASTER");
session2.runSql("SET GLOBAL gtid_purged=?", [gtid_executed+",00025721-1111-1111-1111-111111111111:1"]);
rs.addInstance(__sandbox_uri2, {recoveryMethod:"clone"});

//@ rejoinInstance: recoveryMethod:clone, errant GTIDs + purged GTIDs -> clone {VER(>=8.0.17)}
session3.runSql("RESET MASTER");
session3.runSql("SET GLOBAL gtid_purged=?", [gtid_executed+",00025721-1111-1111-1111-111111111111:1"]);
rs.rejoinInstance(__sandbox_uri3, {recoveryMethod:"clone"});

// TODO: add tests for cloneDonor

//@<> Cleanup {VER(>= 8.0.0)}
// Disable debug
testutil.dbugSet("");

session.close();
session1.close();
session2.close();
session3.close();
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
testutil.destroySandbox(__mysql_sandbox_port3);
testutil.dbugSet("");
