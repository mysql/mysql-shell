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
//             IF gtid-set empty THEN
//                 PROMPT Clone, Abort
//             ELSE
//                 CONTINUE WITH CLONE
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
//              IF gtid-set empty THEN
//                  PROMPT Clone, Abort
//              ELSE
//                  CONTINUE WITH CLONE
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

//@<> Setup
testutil.deploySandbox(__mysql_sandbox_port1, "root", {"report_host": hostname_ip});
testutil.deploySandbox(__mysql_sandbox_port2, "root", {"report_host": hostname_ip});
testutil.deploySandbox(__mysql_sandbox_port3, "root", {"report_host": hostname_ip});

session1 = mysql.getSession(__sandbox_uri1);
session2 = mysql.getSession(__sandbox_uri2);
session3 = mysql.getSession(__sandbox_uri3);

//@ prepare
shell.connect(__sandbox_uri1);
rs = dba.createReplicaSet("myrs");
rs.addInstance(__sandbox_uri3, {recoveryMethod: "incremental"});

// STOP SLAVE on instance 3 to simulate the rejoins
session3.runSql("STOP " + get_replica_keyword());

var gtid_executed = session.runSql("SELECT @@global.gtid_executed").fetchOne()[0];

testutil.dbugSet("+d,dba_abort_async_add_replica");
testutil.dbugSet("+d,dba_abort_async_rejoin_replica");

//@ invalid recoveryMethod (should fail)
rs.addInstance(__sandbox_uri2, {recoveryMethod: "foobar"});
rs.addInstance(__sandbox_uri2, {recoveryMethod: 1});
rs.rejoinInstance(__sandbox_uri3, {recoveryMethod: "foobar"});
rs.rejoinInstance(__sandbox_uri3, {recoveryMethod: 1});

//@ invalid cloneDonor (should fail) {VER(>= 8.0.19)}
rs.addInstance(__sandbox_uri2, {cloneDonor: "127.0.0.1:3306"});
rs.addInstance(__sandbox_uri2, {recoveryMethod: "incremental", cloneDonor: "127.0.0.1:3306"});
rs.addInstance(__sandbox_uri2, {recoveryMethod: "clone", cloneDonor: ":"});
rs.addInstance(__sandbox_uri2, {recoveryMethod: "clone", cloneDonor: ""});
rs.addInstance(__sandbox_uri2, {recoveryMethod: "clone", cloneDonor: "localhost:"});
rs.addInstance(__sandbox_uri2, {recoveryMethod: "clone", cloneDonor: ":3306"});
rs.rejoinInstance(__sandbox_uri3, {cloneDonor: "127.0.0.1:3306"});
rs.rejoinInstance(__sandbox_uri3, {recoveryMethod: "incremental", cloneDonor: "127.0.0.1:3306"});
rs.rejoinInstance(__sandbox_uri3, {recoveryMethod: "clone", cloneDonor: ":"});
rs.rejoinInstance(__sandbox_uri3, {recoveryMethod: "clone", cloneDonor: ""});
rs.rejoinInstance(__sandbox_uri3, {recoveryMethod: "clone", cloneDonor: "localhost:"});
rs.rejoinInstance(__sandbox_uri3, {recoveryMethod: "clone", cloneDonor: ":3306"});

//@ invalid recoveryMethod (should fail if target instance does not support it) {VER(< 8.0.17)}
rs.addInstance(__sandbox_uri2, {recoveryMethod: "clone"});
rs.rejoinInstance(__sandbox_uri3, {recoveryMethod: "clone"});

// Tests for recoveryMethod = clone
//
//   IF recoveryMethod = clone THEN
//      CONTINUE WITH CLONE

//@ addInstance: recoveryMethod:clone, interactive, make sure no prompts {VER(>=8.0.17)}
shell.options.useWizards=1;

session2.runSql("RESET " + get_reset_binary_logs_keyword());
mark_gtid_set_complete(false);
rs.addInstance(__sandbox_uri2, {recoveryMethod:"clone"});

shell.options.useWizards=0;

//@ rejoinInstance: recoveryMethod:clone, interactive, make sure no prompts {VER(>=8.0.17)}
shell.options.useWizards=1;

session3.runSql("RESET " + get_reset_binary_logs_keyword());
mark_gtid_set_complete(false);
rs.rejoinInstance(__sandbox_uri3, {recoveryMethod:"clone"});

shell.options.useWizards=0;

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
session2.runSql("RESET " + get_reset_binary_logs_keyword());
session2.runSql("SET GLOBAL gtid_purged=?", [gtid_executed]);
rs.addInstance(__sandbox_uri2, {recoveryMethod: "clone"});

//@ rejoinInstance: recoveryMethod:clone, subset GTIDs -> clone {VER(>=8.0.17)}
session3.runSql("RESET " + get_reset_binary_logs_keyword());
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

//@ addInstance: recoveryMethod:incremental, interactive, make sure no prompts
shell.options.useWizards=1;

session2.runSql("RESET " + get_reset_binary_logs_keyword());
mark_gtid_set_complete(false);
rs.addInstance(__sandbox_uri2, {recoveryMethod:"incremental"});
EXPECT_FALSE(clone_installed(session1));
EXPECT_FALSE(clone_installed(session2));

shell.options.useWizards=0;

//@ rejoinInstance: recoveryMethod:incremental, interactive, error
shell.options.useWizards=1;

session3.runSql("RESET " + get_reset_binary_logs_keyword());
mark_gtid_set_complete(false);
rs.rejoinInstance(__sandbox_uri3, {recoveryMethod:"incremental"});

shell.options.useWizards=0;

//@ addInstance: recoveryMethod:incremental, empty GTIDs + gtidSetIsComplete -> incr
mark_gtid_set_complete(true);
rs.addInstance(__sandbox_uri2, {recoveryMethod: "incremental"});
EXPECT_FALSE(clone_installed(session1));
EXPECT_FALSE(clone_installed(session2));

//@ rejoinInstance: recoveryMethod:incremental, empty GTIDs + gtidSetIsComplete -> incr
mark_gtid_set_complete(true);
rs.rejoinInstance(__sandbox_uri3, {recoveryMethod: "incremental"});
EXPECT_FALSE(clone_installed(session1));
EXPECT_FALSE(clone_installed(session2));
EXPECT_FALSE(clone_installed(session3));

//@ addInstance: recoveryMethod:incremental, subset GTIDs -> incr
session2.runSql("RESET " + get_reset_binary_logs_keyword());
session2.runSql("SET GLOBAL gtid_purged=?", [gtid_executed]);
rs.addInstance(__sandbox_uri2, {recoveryMethod:"incremental"});
EXPECT_FALSE(clone_installed(session1));
EXPECT_FALSE(clone_installed(session2));

//@ rejoinInstance: recoveryMethod:incremental, subset GTIDs -> incr
session3.runSql("RESET " + get_reset_binary_logs_keyword());
session3.runSql("SET GLOBAL gtid_purged=?", [gtid_executed]);
rs.rejoinInstance(__sandbox_uri3, {recoveryMethod:"incremental"});
EXPECT_FALSE(clone_installed(session1));
EXPECT_FALSE(clone_installed(session2));
EXPECT_FALSE(clone_installed(session3));

//@ addInstance: recoveryMethod:incremental, errant GTIDs -> error
session2.runSql("RESET " + get_reset_binary_logs_keyword());
session2.runSql("SET GLOBAL gtid_purged=?", [gtid_executed+",00025721-1111-1111-1111-111111111111:1"]);
rs.addInstance(__sandbox_uri2, {recoveryMethod:"incremental"});
EXPECT_FALSE(clone_installed(session1));
EXPECT_FALSE(clone_installed(session2));

//@ rejoinInstance: recoveryMethod:incremental, errant GTIDs -> error
session3.runSql("RESET " + get_reset_binary_logs_keyword());
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

//@ addInstance: recoveryMethod:auto, interactive, clone unavailable, empty GTID -> prompt i/a {VER(< 8.0.17)}
shell.options.useWizards=1;

testutil.expectPrompt("Please select a recovery method [I]ncremental recovery/[A]bort (default Incremental recovery): ", "a");
session2.runSql("RESET " + get_reset_binary_logs_keyword());
mark_gtid_set_complete(false);
rs.addInstance(__sandbox_uri2);
EXPECT_FALSE(clone_installed(session1));
EXPECT_FALSE(clone_installed(session2));

shell.options.useWizards=0;

//@ rejoinInstance: recoveryMethod:auto, interactive, clone unavailable, empty GTID -> error {VER(< 8.0.17)}
shell.options.useWizards=1;

session3.runSql("RESET " + get_reset_binary_logs_keyword());
mark_gtid_set_complete(false);
rs.rejoinInstance(__sandbox_uri3);
EXPECT_FALSE(clone_installed(session1));
EXPECT_FALSE(clone_installed(session2));
EXPECT_FALSE(clone_installed(session3));

shell.options.useWizards=0;

//@ addInstance: recoveryMethod:auto, interactive, empty GTID -> prompt c/i/a {VER(>= 8.0.19)}
shell.options.useWizards=1;

testutil.expectPrompt("Please select a recovery method [C]lone/[I]ncremental recovery/[A]bort (default Clone): ", "i");
session2.runSql("RESET " + get_reset_binary_logs_keyword());
mark_gtid_set_complete(false);
rs.addInstance(__sandbox_uri2);
EXPECT_FALSE(clone_installed(session1));
EXPECT_FALSE(clone_installed(session2));

shell.options.useWizards=0;

//@ rejoinInstance: recoveryMethod:auto, interactive, empty GTID -> prompt c/a {VER(>= 8.0.19)}
shell.options.useWizards=1;

testutil.expectPrompt("Please select a recovery method [C]lone/[A]bort (default Abort): ", "c");
session3.runSql("RESET " + get_reset_binary_logs_keyword());
mark_gtid_set_complete(false);
rs.rejoinInstance(__sandbox_uri3);
EXPECT_FALSE(clone_installed(session1));
EXPECT_FALSE(clone_installed(session2));
EXPECT_FALSE(clone_installed(session3));

shell.options.useWizards=0;

//@ addInstance: recoveryMethod:auto, interactive, empty GTIDs + gtidSetIsComplete -> incr {VER(>= 8.0.19)}
shell.options.useWizards=1;

mark_gtid_set_complete(true);
rs.addInstance(__sandbox_uri2);
EXPECT_FALSE(clone_installed(session1));
EXPECT_FALSE(clone_installed(session2));
mark_gtid_set_complete(false);

shell.options.useWizards=0;

//@ rejoinInstance: recoveryMethod:auto, interactive, empty GTIDs + gtidSetIsComplete -> incr {VER(>= 8.0.19)}
shell.options.useWizards=1;

mark_gtid_set_complete(true);
rs.rejoinInstance(__sandbox_uri3);
EXPECT_FALSE(clone_installed(session1));
EXPECT_FALSE(clone_installed(session2));
EXPECT_FALSE(clone_installed(session3));
mark_gtid_set_complete(false);

shell.options.useWizards=0;

//@ addInstance: recoveryMethod:auto, interactive, errant GTIDs -> prompt c/a {VER(>= 8.0.19)}
shell.options.useWizards=1;

session2.runSql("RESET " + get_reset_binary_logs_keyword());
session2.runSql("SET GLOBAL gtid_purged=?", [gtid_executed+",00025721-1111-1111-1111-111111111111:1"]);
testutil.expectPrompt("Please select a recovery method [C]lone/[A]bort (default Abort): ", "a");
rs.addInstance(__sandbox_uri2);
EXPECT_FALSE(clone_installed(session1));
EXPECT_FALSE(clone_installed(session2));

shell.options.useWizards=0;

//@ rejoinInstance: recoveryMethod:auto, interactive, errant GTIDs -> error {VER(>= 8.0.19)}
shell.options.useWizards=1;

session3.runSql("RESET " + get_reset_binary_logs_keyword());
session3.runSql("SET GLOBAL gtid_purged=?", [gtid_executed+",00025721-1111-1111-1111-111111111111:1"]);
rs.rejoinInstance(__sandbox_uri3);
EXPECT_FALSE(clone_installed(session1));
EXPECT_FALSE(clone_installed(session2));
EXPECT_FALSE(clone_installed(session3));

shell.options.useWizards=0;

//@ addInstance: recoveryMethod:auto, interactive, errant GTIDs -> error clone not supported {VER(< 8.0.17)}
shell.options.useWizards=1;

session2.runSql("RESET " + get_reset_binary_logs_keyword());
session2.runSql("SET GLOBAL gtid_purged=?", [gtid_executed+",00025721-1111-1111-1111-111111111111:1"]);
rs.addInstance(__sandbox_uri2);
EXPECT_FALSE(clone_installed(session1));
EXPECT_FALSE(clone_installed(session2));

shell.options.useWizards=0;

//@ rejoinInstance: recoveryMethod:auto, interactive, errant GTIDs -> error clone not supported {VER(< 8.0.17)}
shell.options.useWizards=1;

session3.runSql("RESET " + get_reset_binary_logs_keyword());
session3.runSql("SET GLOBAL gtid_purged=?", [gtid_executed+",00025721-1111-1111-1111-111111111111:1"]);
rs.rejoinInstance(__sandbox_uri3);
EXPECT_FALSE(clone_installed(session1));
EXPECT_FALSE(clone_installed(session2));
EXPECT_FALSE(clone_installed(session3));

shell.options.useWizards=0;

// Non-interactive tests

//@ addInstance: recoveryMethod:auto, non-interactive, empty GTID -> error {VER(>= 8.0.19)}
session2.runSql("RESET " + get_reset_binary_logs_keyword());
mark_gtid_set_complete(false);
rs.addInstance(__sandbox_uri2);
EXPECT_FALSE(clone_installed(session1));
EXPECT_FALSE(clone_installed(session2));

//@ rejoinInstance: recoveryMethod:auto, non-interactive, empty GTID -> error {VER(>= 8.0.19)}
session3.runSql("RESET " + get_reset_binary_logs_keyword());
mark_gtid_set_complete(false);
rs.rejoinInstance(__sandbox_uri3);
EXPECT_FALSE(clone_installed(session1));
EXPECT_FALSE(clone_installed(session2));
EXPECT_FALSE(clone_installed(session3));

//@ addInstance: recoveryMethod:auto, non-interactive, clone not supported, empty GTID -> error {VER(< 8.0.17)}
session2.runSql("RESET " + get_reset_binary_logs_keyword());
mark_gtid_set_complete(false);
rs.addInstance(__sandbox_uri2);
EXPECT_FALSE(clone_installed(session1));
EXPECT_FALSE(clone_installed(session2));

//@ rejoinInstance: recoveryMethod:auto, non-interactive, clone not supported, empty GTID -> error {VER(< 8.0.17)}
session3.runSql("RESET " + get_reset_binary_logs_keyword());
mark_gtid_set_complete(false);
rs.rejoinInstance(__sandbox_uri3);
EXPECT_FALSE(clone_installed(session1));
EXPECT_FALSE(clone_installed(session2));
EXPECT_FALSE(clone_installed(session3));

//@ addInstance: recoveryMethod:auto, non-interactive, empty GTIDs + gtidSetIsComplete -> incr {VER(>=8.0.17)}
mark_gtid_set_complete(true);
rs.addInstance(__sandbox_uri2);
EXPECT_FALSE(clone_installed(session1));
EXPECT_FALSE(clone_installed(session2));
mark_gtid_set_complete(false);

//@ rejoinInstance: recoveryMethod:auto, non-interactive, empty GTIDs + gtidSetIsComplete -> incr {VER(>=8.0.17)}
mark_gtid_set_complete(true);
rs.rejoinInstance(__sandbox_uri3);
EXPECT_FALSE(clone_installed(session1));
EXPECT_FALSE(clone_installed(session2));
EXPECT_FALSE(clone_installed(session3));
mark_gtid_set_complete(false);

//@ addInstance: recoveryMethod:auto, non-interactive, subset GTIDs -> incr {VER(>=8.0.17)}
session2.runSql("RESET " + get_reset_binary_logs_keyword());
session2.runSql("SET GLOBAL gtid_purged=?", [gtid_executed]);
rs.addInstance(__sandbox_uri2);
EXPECT_FALSE(clone_installed(session1));
EXPECT_FALSE(clone_installed(session2));

//@ rejoinInstance: recoveryMethod:auto, non-interactive, subset GTIDs -> incr {VER(>=8.0.17)}
session3.runSql("RESET " + get_reset_binary_logs_keyword());
session3.runSql("SET GLOBAL gtid_purged=?", [gtid_executed]);
rs.rejoinInstance(__sandbox_uri3);
EXPECT_FALSE(clone_installed(session1));
EXPECT_FALSE(clone_installed(session2));
EXPECT_FALSE(clone_installed(session3));

//@ addInstance: recoveryMethod:auto, non-interactive, errant GTIDs -> error
session2.runSql("RESET " + get_reset_binary_logs_keyword());
session2.runSql("SET GLOBAL gtid_purged=?", [gtid_executed+",00025721-1111-1111-1111-111111111111:1"]);
rs.addInstance(__sandbox_uri2);
EXPECT_FALSE(clone_installed(session1));
EXPECT_FALSE(clone_installed(session2));

//@ rejoinInstance: recoveryMethod:auto, non-interactive, errant GTIDs -> error
session3.runSql("RESET " + get_reset_binary_logs_keyword());
session3.runSql("SET GLOBAL gtid_purged=?", [gtid_executed+",00025721-1111-1111-1111-111111111111:1"]);
rs.rejoinInstance(__sandbox_uri3);
EXPECT_FALSE(clone_installed(session1));
EXPECT_FALSE(clone_installed(session2));
EXPECT_FALSE(clone_installed(session3));

// interactive tests


// Tests with purged GTIDs
// -----------------------

//@<> purge GTIDs from replicaset
session.runSql("FLUSH BINARY LOGS");
session.runSql("PURGE BINARY LOGS BEFORE DATE_ADD(NOW(6), INTERVAL 1 DAY)");

session2.runSql("RESET " + get_reset_binary_logs_keyword());
session3.runSql("RESET " + get_reset_binary_logs_keyword());

//@ addInstance: recoveryMethod:auto, interactive, purged GTID -> prompt c/a {VER(>=8.0.17)}
shell.options.useWizards=1;

testutil.expectPrompt("Please select a recovery method [C]lone/[A]bort (default Clone): ", "a");
mark_gtid_set_complete(false);
rs.addInstance(__sandbox_uri2);

shell.options.useWizards=0;

//@ addInstance: recoveryMethod:auto, no-interactive, purged GTID -> error {VER(>=8.0.17)}
mark_gtid_set_complete(false);
rs.addInstance(__sandbox_uri2);

// BUG#30884590: ADDING AN INSTANCE WITH COMPATIBLE GTID SET SHOULDN'T PROMPT FOR CLONE
//@ addInstance: recoveryMethod:auto, interactive, purged GTID, subset gtid -> clone, no prompt {VER(>=8.0.17)}
shell.options.useWizards=1;

mark_gtid_set_complete(false);
session2.runSql("RESET " + get_reset_binary_logs_keyword());
session2.runSql("SET GLOBAL gtid_purged=?", [gtid_executed]);

rs.addInstance(__sandbox_uri2);

shell.options.useWizards=0;

//@ addInstance: recoveryMethod:auto, no-interactive, purged GTID, subset gtid -> clone, no prompt {VER(>=8.0.17)}
mark_gtid_set_complete(false);
session2.runSql("RESET " + get_reset_binary_logs_keyword());
session2.runSql("SET GLOBAL gtid_purged=?", [gtid_executed]);

rs.addInstance(__sandbox_uri2);

//@ rejoinInstance: recoveryMethod:auto, interactive, purged GTID -> prompt c/a {VER(>=8.0.17)}
shell.options.useWizards=1;

testutil.expectPrompt("Please select a recovery method [C]lone/[A]bort (default Abort): ", "a");
mark_gtid_set_complete(false);
rs.rejoinInstance(__sandbox_uri3);

shell.options.useWizards=0;

// BUG#30884590: ADDING AN INSTANCE WITH COMPATIBLE GTID SET SHOULDN'T PROMPT FOR CLONE
//@ rejoinInstance: recoveryMethod:auto, interactive, purged GTID, subset gtid -> clone, no prompt {VER(>=8.0.17)}
shell.options.useWizards=1;

mark_gtid_set_complete(false);
session3.runSql("RESET " + get_reset_binary_logs_keyword());
session3.runSql("SET GLOBAL gtid_purged=?", [gtid_executed]);

rs.rejoinInstance(__sandbox_uri3);

shell.options.useWizards=0;

// BUG#30884590: ADDING AN INSTANCE WITH COMPATIBLE GTID SET SHOULDN'T PROMPT FOR CLONE
//@ rejoinInstance: recoveryMethod:auto, no-interactive, purged GTID, subset gtid -> clone, no prompt {VER(>=8.0.17)}
mark_gtid_set_complete(false);
session3.runSql("RESET " + get_reset_binary_logs_keyword());
session3.runSql("SET GLOBAL gtid_purged=?", [gtid_executed]);

rs.rejoinInstance(__sandbox_uri3);

//@ addInstance: recoveryMethod:auto, interactive, errant GTIDs + purged GTIDs -> prompt c/a {VER(>=8.0.17)}
shell.options.useWizards=1;

session2.runSql("RESET " + get_reset_binary_logs_keyword());
session2.runSql("SET GLOBAL gtid_purged=?", ["00025721-1111-1111-1111-111111111111:1"]);
testutil.expectPrompt("Please select a recovery method [C]lone/[A]bort (default Abort): ", "a");
rs.addInstance(__sandbox_uri2);

shell.options.useWizards=0;

//@ rejoinInstance: recoveryMethod:auto, interactive, errant GTIDs + purged GTIDs -> error {VER(>=8.0.17)}
shell.options.useWizards=1;

session3.runSql("RESET " + get_reset_binary_logs_keyword());
session3.runSql("SET GLOBAL gtid_purged=?", ["00025721-1111-1111-1111-111111111111:1"]);
rs.rejoinInstance(__sandbox_uri3);

shell.options.useWizards=0;

//@ addInstance: recoveryMethod:incremental, purged GTID -> error
session2.runSql("RESET " + get_reset_binary_logs_keyword());
mark_gtid_set_complete(false);
rs.addInstance(__sandbox_uri2, {recoveryMethod: "incremental"});

//@ rejoinInstance: recoveryMethod:incremental, purged GTID -> error
session3.runSql("RESET " + get_reset_binary_logs_keyword());
mark_gtid_set_complete(false);
rs.rejoinInstance(__sandbox_uri3, {recoveryMethod: "incremental"});

//@ addInstance: recoveryMethod:incremental, errant GTIDs + purged GTIDs -> error
session2.runSql("RESET " + get_reset_binary_logs_keyword());
session2.runSql("SET GLOBAL gtid_purged=?", ["00025721-1111-1111-1111-111111111111:1"]);
rs.addInstance(__sandbox_uri2, {recoveryMethod: "incremental"});

//@ rejoinInstance: recoveryMethod:incremental, errant GTIDs + purged GTIDs -> error
session3.runSql("RESET " + get_reset_binary_logs_keyword());
session3.runSql("SET GLOBAL gtid_purged=?", ["00025721-1111-1111-1111-111111111111:1"]);
rs.rejoinInstance(__sandbox_uri3, {recoveryMethod: "incremental"});

//@ addInstance: recoveryMethod:clone, purged GTID -> clone {VER(>=8.0.17)}
session2.runSql("RESET " + get_reset_binary_logs_keyword());
mark_gtid_set_complete(false);
rs.addInstance(__sandbox_uri2, {recoveryMethod:"clone"});

//@ rejoinInstance: recoveryMethod:clone, purged GTID -> clone {VER(>=8.0.17)}
session3.runSql("RESET " + get_reset_binary_logs_keyword());
mark_gtid_set_complete(false);
rs.rejoinInstance(__sandbox_uri3, {recoveryMethod:"clone"});

//@ addInstance: recoveryMethod:clone, errant GTIDs + purged GTIDs -> clone {VER(>=8.0.17)}
session2.runSql("RESET " + get_reset_binary_logs_keyword());
session2.runSql("SET GLOBAL gtid_purged=?", [gtid_executed+",00025721-1111-1111-1111-111111111111:1"]);
rs.addInstance(__sandbox_uri2, {recoveryMethod:"clone"});

//@ rejoinInstance: recoveryMethod:clone, errant GTIDs + purged GTIDs -> clone {VER(>=8.0.17)}
session3.runSql("RESET " + get_reset_binary_logs_keyword());
session3.runSql("SET GLOBAL gtid_purged=?", [gtid_executed+",00025721-1111-1111-1111-111111111111:1"]);
rs.rejoinInstance(__sandbox_uri3, {recoveryMethod:"clone"});

// cloneDonor tests
testutil.dbugSet("");
testutil.dbugSet("+d,dba_clone_version_check_fail");

//@<> rejoinInstance: recoveryMethod: clone, errant GTIDs + purged GTIDs + automatic donor not valid {VER(>=8.0.17)}
__endpoint_uri1 = hostname_ip+":"+__mysql_sandbox_port1;

EXPECT_THROWS_TYPE(function() { rs.rejoinInstance(__sandbox_uri3, {recoveryMethod: "clone"}); }, "Cannot use recoveryMethod=clone because the selected donor is incompatible or no compatible donors are available due to version/platform incompatibilities.", "MYSQLSH");

EXPECT_OUTPUT_CONTAINS(`WARNING: Clone-based recovery not available: Instance '${__endpoint_uri1}' cannot be a donor because its version (8.0.17) isn't compatible with the recipient's (${__version})`);

//@<> rejoinInstance: recoveryMethod: clone, errant GTIDs + purged GTIDs + cloneDonor not valid {VER(>=8.0.17)}
EXPECT_THROWS_TYPE(function() { rs.rejoinInstance(__sandbox_uri3, {recoveryMethod: "clone", cloneDonor: __endpoint1}); }, "Cannot use recoveryMethod=clone because the selected donor is incompatible or no compatible donors are available due to version/platform incompatibilities.", "MYSQLSH");

EXPECT_OUTPUT_CONTAINS(`WARNING: Clone-based recovery not available: Instance '${__endpoint_uri1}' cannot be a donor because its version (8.0.17) isn't compatible with the recipient's (${__version})`);

//@<> rejoinInstance: recoveryMethod: clone, errant GTIDs + purged GTIDs + cloneDonor valid {VER(>=8.0.17)}

// Disable debug
testutil.dbugSet("");

EXPECT_NO_THROWS(function() { rs.rejoinInstance(__sandbox_uri3, {recoveryMethod: "clone", cloneDonor: __endpoint1}); });

// Tests for the scenario on which clone is unavailable

// addInstance()

//<>@ instance has empty GTID set + gtidSetIsComplete:0 + clone not available prompt-no (should fail) {VER(>=8.0.17)}
shell.options.useWizards = true;

rs.removeInstance(__sandbox3);
session3 = mysql.getSession(__sandbox_uri3);
reset_instance(session3);
reset_instance(session2);
shell.connect(__sandbox_uri2);
rs = dba.createReplicaSet("rs");

testutil.dbugSet("+d,dba_clone_version_check_fail");

__endpoint_uri2 = hostname_ip+":"+__mysql_sandbox_port2;

testutil.expectPrompt("Please select a recovery method [I]ncremental recovery/[A]bort (default Incremental recovery): ", "a");
EXPECT_THROWS(function() { rs.addInstance(__sandbox3); }, "Cancelled");
EXPECT_OUTPUT_CONTAINS(`WARNING: Clone-based recovery not available: Instance '${__endpoint_uri2}' cannot be a donor because its version (8.0.17) isn't compatible with the recipient's (${__version})`);

shell.options.useWizards = false;

//@<> instance has empty GTID set + gtidSetIsComplete:0 + recoveryMethod:CLONE + clone unavailable {VER(>=8.0.17)}
EXPECT_THROWS(function() { rs.addInstance(__sandbox3, {recoveryMethod:"CLONE"}); }, "Cannot use recoveryMethod=clone because the selected donor is incompatible or no compatible donors are available due to version/platform incompatibilities.");
EXPECT_OUTPUT_CONTAINS(`WARNING: Clone-based recovery not available: Instance '${__endpoint_uri2}' cannot be a donor because its version (8.0.17) isn't compatible with the recipient's (${__version})`);

//@<> cloneDonor invalid: clone unavailable {VER(>=8.0.17)}
EXPECT_THROWS(function() { rs.addInstance(__sandbox3, {recoveryMethod:"clone", cloneDonor: __sandbox1}); }, "Cannot use recoveryMethod=clone because the selected donor is incompatible or no compatible donors are available due to version/platform incompatibilities.");
EXPECT_OUTPUT_CONTAINS(`WARNING: Clone-based recovery not available: Instance '${__endpoint_uri2}' cannot be a donor because its version (8.0.17) isn't compatible with the recipient's (${__version})`);

// Tests for the scenario on which clone is unavailable

//@<> Try to rejoin instance with purged transactions on PRIMARY + clone unavailable (fail) {VER(>=8.0.17) && !__dbug_off}
testutil.dbugSet("");

EXPECT_NO_THROWS(function() { rs.addInstance(__sandbox_uri3, {recoveryMethod:"clone"}); });

testutil.dbugSet("+d,dba_clone_version_check_fail");

session3 = mysql.getSession(__sandbox_uri3);
session3.runSql("STOP " + get_replica_keyword());
session3.runSql("RESET " + get_reset_binary_logs_keyword());
session3.runSql("FLUSH BINARY LOGS");
session3.runSql("PURGE BINARY LOGS BEFORE DATE_ADD(NOW(), INTERVAL 1 DAY)");

EXPECT_THROWS(function() { rs.rejoinInstance(__sandbox3); }, "Instance provisioning required");
EXPECT_OUTPUT_CONTAINS(`WARNING: Clone-based recovery not available: Instance '${__endpoint_uri2}' cannot be a donor because its version (8.0.17) isn't compatible with the recipient's (${__version})`);
EXPECT_OUTPUT_CONTAINS(`ERROR: The target instance must be either cloned or fully re-provisioned before it can be re-added to the target replicaset.`);

// rejoinInstance()

//@<> Try to rejoin instance with purged transactions on PRIMARY + clone unavailable + recoveryMethod (fail) {VER(>=8.0.17) && !__dbug_off}
session3.runSql("STOP " + get_replica_keyword());
EXPECT_THROWS(function() { rs.rejoinInstance(__sandbox3, {recoveryMethod: "clone"}); }, "Cannot use recoveryMethod=clone because the selected donor is incompatible or no compatible donors are available due to version/platform incompatibilities.");
EXPECT_OUTPUT_CONTAINS(`WARNING: Clone-based recovery not available: Instance '${__endpoint_uri2}' cannot be a donor because its version (8.0.17) isn't compatible with the recipient's (${__version})`);

//@<> Try to rejoin instance with purged transactions on PRIMARY + clone unavailable + recoveryMethod + cloneDonor (fail) {VER(>=8.0.17) && !__dbug_off}
EXPECT_THROWS(function() { rs.rejoinInstance(__sandbox3, {recoveryMethod: "clone", cloneDonor: __sandbox1}); }, "Cannot use recoveryMethod=clone because the selected donor is incompatible or no compatible donors are available due to version/platform incompatibilities.");
EXPECT_OUTPUT_CONTAINS(`WARNING: Clone-based recovery not available: Instance '${__endpoint_uri2}' cannot be a donor because its version (8.0.17) isn't compatible with the recipient's (${__version})`);

testutil.dbugSet("");


//@<> Cleanup
session.close();
session1.close();
session2.close();
session3.close();
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
testutil.destroySandbox(__mysql_sandbox_port3);
testutil.dbugSet("");
