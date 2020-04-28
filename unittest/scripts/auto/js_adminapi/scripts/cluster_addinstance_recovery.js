//@{!__dbug_off}

// Tests recovery method selection for addInstance()

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
// - disableClone: true/false
// - clone supported/not


function mark_gtid_set_complete(flag) {
    if (flag)
        session.runSql("UPDATE mysql_innodb_cluster_metadata.clusters SET attributes = JSON_SET(attributes, '$.opt_gtidSetIsComplete', true)");
    else
        session.runSql("UPDATE mysql_innodb_cluster_metadata.clusters SET attributes = JSON_REMOVE(attributes, '$.opt_gtidSetIsComplete')");
}

function mark_clone_disabled(flag) {
    if (flag)
        session.runSql("UPDATE mysql_innodb_cluster_metadata.clusters SET attributes = JSON_SET(attributes, '$.opt_disableClone', true)");
    else
        session.runSql("UPDATE mysql_innodb_cluster_metadata.clusters SET attributes = JSON_REMOVE(attributes, '$.opt_disableClone')");
}

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

function clone_threshold() {
    var row = session2.runSql("SELECT variable_value FROM performance_schema.global_variables WHERE variable_name = 'group_replication_clone_threshold'").fetchOne();
    if (row)
        return row[0];
    return null;
}

//@ Setup
testutil.deploySandbox(__mysql_sandbox_port1, "root");
testutil.deploySandbox(__mysql_sandbox_port2, "root");

session1 = mysql.getSession(__sandbox_uri1);
session2 = mysql.getSession(__sandbox_uri2);

// Tests for recovery method selection logic
//---------------------------------------------

// | recovery- | clone    | regular recovery | regular recovery           |                                                       |
// | Method    | possible | POSSIBLE         | SAFE                       | Action                                                |
// |-----------|----------|------------------|----------------------------|-------------------------------------------------------|
// | !auto     | -        | true             | true or gtidSetComplete=1  | Continue without clone (regular distributed recovery) |
// | auto      | true     | true             | false                      | PROMPT: Clone, Recovery, Abort                        |
// | auto      | false    | true             | false                      | PROMPT: Recovery, Abort                               |
// | auto      | true     | false            | -                          | GTID-SET DIVERGED: PROMPT: Clone, Abort               |
// | auto      | true     | false            | -                          | GTID-SET COMPATIBLE: Continue with clone              |
// | auto      | false    | false            | -                          | ERROR: Manual provisioning required                   |

// | regular   | -        | true             | -                          | Continue without clone (regular distributed recovery) |
// | regular   | -        | false            | -                          | ERROR: Recovery not possible                          |
// | clone     | false    | -                | -                          | ERROR: Clone disabled/not supported                   |
// | clone     | true     | -                | -                          | Continue with clone


//@ prepare
shell.connect(__sandbox_uri1);
c = dba.createCluster("mycluster");

var gtid_executed = session.runSql("SELECT @@global.gtid_executed").fetchOne()[0];

testutil.clearTraps();
testutil.setTrap("mysql", ["sql regex start group_replication"], {msg: "debug"});

//@ bogus recoveryMethod (should fail)
c.addInstance(__sandbox_uri2, {interactive:false, recoveryMethod:"foobar"});

//@ bogus recoveryMethod (should fail if target instance does not support it) {VER(<8.0.17)}
c.addInstance(__sandbox_uri2, {interactive:false, recoveryMethod:"clone"});

//@ Ensure clone disabled on incremental recovery {VER(>=8.0.17)}
session2.runSql("RESET MASTER");
install_clone(session2);

c.addInstance(__sandbox_uri2, {interactive:false, recoveryMethod:"incremental"});

EXPECT_FALSE(clone_installed(session1));
EXPECT_FALSE(clone_installed(session2));
EXPECT_NE(1, clone_threshold());

// Regression test for BUG#29954085 and BUG#29960838:
//
// Regardless if:
//  - The cluster was set up using Incremental recovery only, clone usage was
//    not disabled.
//  - The cluster was created on an instance supporting clone but using a Shell
//    older than 8.0.16
//
// The clone plugin must be installed on all cluster members.
// The same logic applies when disableClone was set: the plugin must be
// uninstalled on all cluster members

//@ Ensure clone enabled on all cluster members on clone recovery {VER(>=8.0.17)}
c.addInstance(__sandbox_uri2, {interactive:false, recoveryMethod:"clone"});

EXPECT_TRUE(clone_installed(session1));
EXPECT_TRUE(clone_installed(session2));
//EXPECT_EQ(1, clone_threshold()); this check doesn't work because addInstance will revert the threshold when it finishes up

//@ Ensure clone enabled on all cluster members on clone recovery when cluster created with old shell {VER(>=8.0.17)}
// NOTE: To simulate a cluster created with an old shell, on an instance
// supporting clone, we simply ensure the plugin is uninstalled
uninstall_clone(session1);
EXPECT_FALSE(clone_installed(session1));
c.addInstance(__sandbox_uri2, {interactive:false, recoveryMethod:"clone"});

EXPECT_TRUE(clone_installed(session1));
EXPECT_TRUE(clone_installed(session2));

//@ recoveryMethod:auto, interactive, empty GTID -> prompt c/i/a {VER(>=8.0.17)}
testutil.expectPrompt("Please select a recovery method [C]lone/[I]ncremental recovery/[A]bort (default Clone): ", "i");
mark_gtid_set_complete(false);

c.addInstance(__sandbox_uri2, {interactive:true});

EXPECT_FALSE(clone_installed(session1));
EXPECT_FALSE(clone_installed(session2));
EXPECT_NE(1, clone_threshold());

//@ recoveryMethod:auto, interactive, cloneDisabled, empty GTID -> prompt i/a
testutil.expectPrompt("Please select a recovery method [I]ncremental recovery/[A]bort (default Incremental recovery): ", "a");
mark_gtid_set_complete(false);
mark_clone_disabled(true);

c.addInstance(__sandbox_uri2, {interactive:true});

EXPECT_FALSE(clone_installed(session1));
EXPECT_FALSE(clone_installed(session2));
EXPECT_NE(1, clone_threshold());
mark_clone_disabled(false);

//@ recoveryMethod:auto, interactive, empty GTIDs + gtidSetIsComplete -> incr
mark_gtid_set_complete(true);

c.addInstance(__sandbox_uri2, {interactive:true});

EXPECT_FALSE(clone_installed(session1));
EXPECT_FALSE(clone_installed(session2));
EXPECT_NE(1, clone_threshold());

mark_gtid_set_complete(false);

//@ recoveryMethod:auto, interactive, subset GTIDs -> incr {VER(>=8.0.17)}
session2.runSql("RESET MASTER");
session2.runSql("SET GLOBAL gtid_purged=?", [gtid_executed]);

c.addInstance(__sandbox_uri2, {interactive:true});

EXPECT_FALSE(clone_installed(session1));
EXPECT_FALSE(clone_installed(session2));
EXPECT_NE(1, clone_threshold());

//@ recoveryMethod:auto, interactive, errant GTIDs -> prompt c/a {VER(>=8.0.17)}
session2.runSql("RESET MASTER");
session2.runSql("SET GLOBAL gtid_purged=?", [gtid_executed+",00025721-1111-1111-1111-111111111111:1"]);
testutil.expectPrompt("Please select a recovery method [C]lone/[A]bort (default Abort): ", "a");

c.addInstance(__sandbox_uri2, {interactive:true});

EXPECT_FALSE(clone_installed(session1));
EXPECT_FALSE(clone_installed(session2));
EXPECT_NE(1, clone_threshold());

//@ recoveryMethod:auto, interactive, errant GTIDs -> error 5.7 {VER(<8.0.17)}
session2.runSql("RESET MASTER");
session2.runSql("SET GLOBAL gtid_purged=?", [gtid_executed+",00025721-1111-1111-1111-111111111111:1"]);

c.addInstance(__sandbox_uri2, {interactive:true});

EXPECT_FALSE(clone_installed(session1));
EXPECT_FALSE(clone_installed(session2));
EXPECT_NE(1, clone_threshold());

//@ recoveryMethod:auto, interactive, cloneDisabled, errant GTIDs -> error
session2.runSql("RESET MASTER");
session2.runSql("SET GLOBAL gtid_purged=?", [gtid_executed+",00025721-1111-1111-1111-111111111111:1"]);

mark_clone_disabled(true);

c.addInstance(__sandbox_uri2, {interactive:true});

EXPECT_FALSE(clone_installed(session1));
EXPECT_FALSE(clone_installed(session2));
EXPECT_NE(1, clone_threshold());
mark_clone_disabled(false);

// ====

//@ recoveryMethod:auto, non-interactive, empty GTID -> error {VER(>=8.0.17)}
session2.runSql("RESET MASTER");
mark_gtid_set_complete(false);

c.addInstance(__sandbox_uri2, {interactive:false});

EXPECT_FALSE(clone_installed(session1));
EXPECT_FALSE(clone_installed(session2));
EXPECT_NE(1, clone_threshold());

//@ recoveryMethod:auto, non-interactive, cloneDisabled, empty GTID -> error {VER(>=8.0.17)}
mark_gtid_set_complete(false);
mark_clone_disabled(true);

c.addInstance(__sandbox_uri2, {interactive:false});

EXPECT_FALSE(clone_installed(session1));
EXPECT_FALSE(clone_installed(session2));
EXPECT_NE(1, clone_threshold());
mark_clone_disabled(false);

//@ recoveryMethod:auto, non-interactive, empty GTIDs + gtidSetIsComplete -> incr {VER(>=8.0.17)}
mark_gtid_set_complete(true);

c.addInstance(__sandbox_uri2, {interactive:false});

EXPECT_FALSE(clone_installed(session1));
EXPECT_FALSE(clone_installed(session2));
EXPECT_NE(1, clone_threshold());

mark_gtid_set_complete(false);

//@ recoveryMethod:auto, non-interactive, subset GTIDs -> incr {VER(>=8.0.17)}
session2.runSql("RESET MASTER");
session2.runSql("SET GLOBAL gtid_purged=?", [gtid_executed]);

c.addInstance(__sandbox_uri2, {interactive:false});

EXPECT_FALSE(clone_installed(session1));
EXPECT_FALSE(clone_installed(session2));
EXPECT_NE(1, clone_threshold());

//@ recoveryMethod:auto, non-interactive, errant GTIDs -> error
session2.runSql("RESET MASTER");
session2.runSql("SET GLOBAL gtid_purged=?", [gtid_executed+",00025721-1111-1111-1111-111111111111:1"]);

c.addInstance(__sandbox_uri2, {interactive:false});

EXPECT_FALSE(clone_installed(session1));
EXPECT_FALSE(clone_installed(session2));
EXPECT_NE(1, clone_threshold());

//@ recoveryMethod:auto, non-interactive, cloneDisabled, errant GTIDs -> error
session2.runSql("RESET MASTER");
session2.runSql("SET GLOBAL gtid_purged=?", [gtid_executed+",00025721-1111-1111-1111-111111111111:1"]);

mark_clone_disabled(true);

c.addInstance(__sandbox_uri2, {interactive:false});

EXPECT_FALSE(clone_installed(session1));
EXPECT_FALSE(clone_installed(session2));
EXPECT_NE(1, clone_threshold());
mark_clone_disabled(false);



// ====

//@ recoveryMethod:incremental, interactive, make sure no prompts
session2.runSql("RESET MASTER");
mark_gtid_set_complete(false);

c.addInstance(__sandbox_uri2, {interactive:true, recoveryMethod:"incremental"});

//@ recoveryMethod:incremental, empty GTID -> incr
mark_gtid_set_complete(false);

c.addInstance(__sandbox_uri2, {recoveryMethod:"incremental"});

EXPECT_FALSE(clone_installed(session1));
EXPECT_FALSE(clone_installed(session2));

//@ recoveryMethod:incremental, cloneDisabled, empty GTID -> incr
mark_gtid_set_complete(false);
mark_clone_disabled(true);

c.addInstance(__sandbox_uri2, {recoveryMethod:"incremental"});

EXPECT_FALSE(clone_installed(session1));
EXPECT_FALSE(clone_installed(session2));
mark_clone_disabled(false);

//@ recoveryMethod:incremental, empty GTIDs + gtidSetIsComplete -> incr
mark_gtid_set_complete(true);

c.addInstance(__sandbox_uri2, {recoveryMethod:"incremental"});

EXPECT_FALSE(clone_installed(session1));
EXPECT_FALSE(clone_installed(session2));
mark_gtid_set_complete(false);

//@ recoveryMethod:incremental, subset GTIDs -> incr
session2.runSql("RESET MASTER");
session2.runSql("SET GLOBAL gtid_purged=?", [gtid_executed]);

c.addInstance(__sandbox_uri2, {recoveryMethod:"incremental"});

EXPECT_FALSE(clone_installed(session1));
EXPECT_FALSE(clone_installed(session2));

//@ recoveryMethod:incremental, errant GTIDs -> error
session2.runSql("RESET MASTER");
session2.runSql("SET GLOBAL gtid_purged=?", [gtid_executed+",00025721-1111-1111-1111-111111111111:1"]);

c.addInstance(__sandbox_uri2, {recoveryMethod:"incremental"});

EXPECT_FALSE(clone_installed(session1));
EXPECT_FALSE(clone_installed(session2));

//@ recoveryMethod:incremental, cloneDisabled, errant GTIDs -> error
session2.runSql("RESET MASTER");
session2.runSql("SET GLOBAL gtid_purged=?", [gtid_executed+",00025721-1111-1111-1111-111111111111:1"]);

mark_clone_disabled(true);

c.addInstance(__sandbox_uri2, {recoveryMethod:"incremental"});

EXPECT_FALSE(clone_installed(session1));
EXPECT_FALSE(clone_installed(session2));
mark_clone_disabled(false);
session2.runSql("RESET MASTER");


// ====

//@ recoveryMethod:clone, interactive, make sure no prompts {VER(>=8.0.17)}
session2.runSql("RESET MASTER");
mark_gtid_set_complete(false);

c.addInstance(__sandbox_uri2, {interactive:true, recoveryMethod:"clone"});

//@ recoveryMethod:clone, empty GTID -> clone {VER(>=8.0.17)}
mark_gtid_set_complete(false);

c.addInstance(__sandbox_uri2, {recoveryMethod:"clone"});

EXPECT_TRUE(clone_installed(session1));
EXPECT_TRUE(clone_installed(session2));

//@ recoveryMethod:clone, cloneDisabled, empty GTID -> err {VER(>=8.0.17)}
mark_gtid_set_complete(false);
mark_clone_disabled(true);

c.addInstance(__sandbox_uri2, {recoveryMethod:"clone"});

mark_clone_disabled(false);

//@ recoveryMethod:clone, empty GTIDs + gtidSetIsComplete -> clone {VER(>=8.0.17)}
mark_gtid_set_complete(true);

c.addInstance(__sandbox_uri2, {recoveryMethod:"clone"});

EXPECT_TRUE(clone_installed(session1));
EXPECT_TRUE(clone_installed(session2));
mark_gtid_set_complete(false);

//@ recoveryMethod:clone, subset GTIDs -> clone {VER(>=8.0.17)}
session2.runSql("RESET MASTER");
session2.runSql("SET GLOBAL gtid_purged=?", [gtid_executed]);

c.addInstance(__sandbox_uri2, {recoveryMethod:"clone"});

EXPECT_TRUE(clone_installed(session1));
EXPECT_TRUE(clone_installed(session2));

//@ recoveryMethod:clone, errant GTIDs -> clone {VER(>=8.0.17)}
session2.runSql("RESET MASTER");
session2.runSql("SET GLOBAL gtid_purged=?", [gtid_executed+",00025721-1111-1111-1111-111111111111:1"]);

c.addInstance(__sandbox_uri2, {recoveryMethod:"clone"});

EXPECT_TRUE(clone_installed(session1));
EXPECT_TRUE(clone_installed(session2));

//@ recoveryMethod:clone, cloneDisabled, errant GTIDs -> error {VER(>=8.0.17)}
session2.runSql("RESET MASTER");
session2.runSql("SET GLOBAL gtid_purged=?", [gtid_executed+",00025721-1111-1111-1111-111111111111:1"]);

mark_clone_disabled(true);

c.addInstance(__sandbox_uri2, {recoveryMethod:"clone"});

mark_clone_disabled(false);

//@<> Ensure clone is installed on all cluster members when failover happens {VER(>=8.0.17)}
//
// Regression test for BUG#29954085
//
// NOTE: When a cluster is created with a shell version < 8.0.17, the cluster
// plugin won't be installed even if the target mysql instance supports it.
// If an instance is added using incremental recovery and then the seed is
// removed for some reason, whenever adding another new instance using
// clone recovery the "new" seed must have the clone plugin installed.
//
// To simulate a cluster created with a shell version < 8.0.17 we simply uninstall
// the clone plugin on the seed if installed.

// Disable debug
testutil.clearTraps();

testutil.deploySandbox(__mysql_sandbox_port3, "root");
session3 = mysql.getSession(__sandbox_uri3);

ensure_plugin_disabled("clone", session1);
EXPECT_FALSE(clone_installed(session1));
EXPECT_TRUE(clone_installed(session2));

session2.runSql("RESET MASTER");
mark_gtid_set_complete(true);

// Add the second instance to the cluster using incremental recovery
c.addInstance(__sandbox_uri2, {interactive:true, recoveryMethod:"incremental"});

// Remove the seed
c.removeInstance(__sandbox_uri1);

session.close();
c.disconnect();
shell.connect(__sandbox_uri2);

var c = dba.getCluster();

// Add a third instance using clone recovery
c.addInstance(__sandbox_uri3, {recoveryMethod:"clone"});

c.removeInstance(__sandbox_uri2);

// Re-enable debug
testutil.setTrap("mysql", ["sql regex start group_replication"], {msg: "debug"});

session.close();
c.disconnect();
shell.connect(__sandbox_uri3);
var c = dba.getCluster();

set_sysvar(session, "super_read_only", 0);

session1 = mysql.getSession(__sandbox_uri3);
session2 = mysql.getSession(__sandbox_uri2);

// Tests with purged GTIDs
// ----------------------------------------

//@ purge GTIDs from cluster
session.runSql("FLUSH BINARY LOGS");
session.runSql("PURGE BINARY LOGS BEFORE DATE_ADD(NOW(6), INTERVAL 1 DAY)");

//@ recoveryMethod:auto, interactive, purged GTID, new -> prompt c/a {VER(>=8.0.17)}
testutil.expectPrompt("Please select a recovery method [C]lone/[A]bort (default Abort): ", "a");
mark_gtid_set_complete(false);
session2.runSql("RESET MASTER");

c.addInstance(__sandbox_uri2, {interactive:true});

//@ recoveryMethod:auto, no-interactive, purged GTID, new -> error {VER(>=8.0.17)}
mark_gtid_set_complete(false);
session2.runSql("RESET MASTER");

c.addInstance(__sandbox_uri2, {interactive:false});

// BUG#30884590: ADDING AN INSTANCE WITH COMPATIBLE GTID SET SHOULDN'T PROMPT FOR CLONE
//@ recoveryMethod:auto, interactive, purged GTID, subset gtid -> clone, no prompt {VER(>=8.0.17)}
mark_gtid_set_complete(false);
session2.runSql("RESET MASTER");
session2.runSql("SET GLOBAL gtid_purged=?", [gtid_executed]);

c.addInstance(__sandbox_uri2, {interactive:true});

// BUG#30884590: ADDING AN INSTANCE WITH COMPATIBLE GTID SET SHOULDN'T PROMPT FOR CLONE
//@ recoveryMethod:auto, no-interactive, purged GTID, subset gtid -> clone, no prompt {VER(>=8.0.17)}
mark_gtid_set_complete(false);
session2.runSql("RESET MASTER");
session2.runSql("SET GLOBAL gtid_purged=?", [gtid_executed]);

c.addInstance(__sandbox_uri2, {interactive:true});

//@ recoveryMethod:auto, interactive, cloneDisabled, purged GTID -> error
mark_gtid_set_complete(false);
mark_clone_disabled(true);

c.addInstance(__sandbox_uri2, {interactive:true});

mark_clone_disabled(false);

//@ recoveryMethod:auto, interactive, errant GTIDs + purged GTIDs -> prompt c/a {VER(>=8.0.17)}
session2.runSql("RESET MASTER");
session2.runSql("SET GLOBAL gtid_purged=?", ["00025721-1111-1111-1111-111111111111:1"]);
testutil.expectPrompt("Please select a recovery method [C]lone/[A]bort (default Abort): ", "a");

c.addInstance(__sandbox_uri2, {interactive:true});

//@ recoveryMethod:auto, interactive, cloneDisabled, errant GTIDs + purged GTIDs -> error
session2.runSql("RESET MASTER");
session2.runSql("SET GLOBAL gtid_purged=?", ["00025721-1111-1111-1111-111111111111:1"]);

mark_clone_disabled(true);

c.addInstance(__sandbox_uri2, {interactive:true});

mark_clone_disabled(false);

// ====

//@ recoveryMethod:incremental, purged GTID -> error
session2.runSql("RESET MASTER");
mark_gtid_set_complete(false);

c.addInstance(__sandbox_uri2, {recoveryMethod:"incremental"});


//@ recoveryMethod:incremental, cloneDisabled, purged GTID -> error
mark_gtid_set_complete(false);
mark_clone_disabled(true);

c.addInstance(__sandbox_uri2, {recoveryMethod:"incremental"});

mark_clone_disabled(false);

//@ recoveryMethod:incremental, errant GTIDs + purged GTIDs -> error
session2.runSql("RESET MASTER");
session2.runSql("SET GLOBAL gtid_purged=?", ["00025721-1111-1111-1111-111111111111:1"]);

c.addInstance(__sandbox_uri2, {recoveryMethod:"incremental"});

//@ recoveryMethod:incremental, cloneDisabled, errant GTIDs + purged GTIDs -> error
session2.runSql("RESET MASTER");
session2.runSql("SET GLOBAL gtid_purged=?", ["00025721-1111-1111-1111-111111111111:1"]);

mark_clone_disabled(true);

c.addInstance(__sandbox_uri2, {recoveryMethod:"incremental"});

mark_clone_disabled(false);


// ====

//@ recoveryMethod:clone, purged GTID -> clone {VER(>=8.0.17)}
session2.runSql("RESET MASTER");
mark_gtid_set_complete(false);

c.addInstance(__sandbox_uri2, {recoveryMethod:"clone"});

EXPECT_TRUE(clone_installed(session1));
EXPECT_TRUE(clone_installed(session2));

//@ recoveryMethod:clone, cloneDisabled, purged GTID -> err {VER(>=8.0.17)}
mark_gtid_set_complete(false);
mark_clone_disabled(true);

c.addInstance(__sandbox_uri2, {recoveryMethod:"clone"});

mark_clone_disabled(false);

//@ recoveryMethod:clone, errant GTIDs + purged GTIDs -> clone {VER(>=8.0.17)}
session2.runSql("RESET MASTER");
session2.runSql("SET GLOBAL gtid_purged=?", [gtid_executed+",00025721-1111-1111-1111-111111111111:1"]);

c.addInstance(__sandbox_uri2, {recoveryMethod:"clone"});

EXPECT_TRUE(clone_installed(session1));
EXPECT_TRUE(clone_installed(session2));

//@ recoveryMethod:clone, cloneDisabled, errant GTIDs + purged GTIDs -> error {VER(>=8.0.17)}
session2.runSql("RESET MASTER");
session2.runSql("SET GLOBAL gtid_purged=?", [gtid_executed+",00025721-1111-1111-1111-111111111111:1"]);

mark_clone_disabled(true);

c.addInstance(__sandbox_uri2, {recoveryMethod:"clone"});

mark_clone_disabled(false);


//@ Cleanup
testutil.clearTraps();

session.close();
session1.close();
session2.close();
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
testutil.destroySandbox(__mysql_sandbox_port3);
