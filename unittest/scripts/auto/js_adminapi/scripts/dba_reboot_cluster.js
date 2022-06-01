// Assumptions: smart deployment rountines available

//@<> Skip tests in 8.0.4 to not trigger GR plugin deadlock {VER(==8.0.4)}
testutil.skip("Reboot tests freeze in 8.0.4 because of bug in GR");

//@ Initialization
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: hostname});
testutil.snapshotSandboxConf(__mysql_sandbox_port1);
testutil.deploySandbox(__mysql_sandbox_port2, "root", {report_host: hostname});
testutil.snapshotSandboxConf(__mysql_sandbox_port2);
testutil.deploySandbox(__mysql_sandbox_port3, "root", {report_host: hostname});
testutil.snapshotSandboxConf(__mysql_sandbox_port3);

function get_rpl_users() {
    var result = session.runSql(
        "SELECT GRANTEE FROM INFORMATION_SCHEMA.USER_PRIVILEGES " +
        "WHERE GRANTEE REGEXP \"mysql_innodb_cluster_[0-9]+\"");
    return result.fetchAll();
}

function has_new_rpl_users(rows) {
    var sql =
        "SELECT GRANTEE FROM INFORMATION_SCHEMA.USER_PRIVILEGES " +
        "WHERE GRANTEE REGEXP \"mysql_innodb_cluster_[0-9]+\" " +
        "AND GRANTEE NOT IN (";
    for (i = 0; i < rows.length; i++) {
        sql += "\"" + rows[i][0] + "\"";
        if (i != rows.length-1) {
            sql += ", ";
        }
    }
    sql += ")";
    var result = session.runSql(sql);
    var new_user_rows = result.fetchAll();
    if (new_user_rows.length == 0) {
        return false;
    } else {
        return true;
    }
}

shell.connect(__sandbox_uri1);

// Create cluster
var cluster = dba.createCluster('dev', {memberSslMode:'REQUIRED', gtidSetIsComplete: true});

testutil.waitMemberState(__mysql_sandbox_port1, "ONLINE");

//@<> Check status
var status = cluster.status();
EXPECT_EQ(1, Object.keys(status["defaultReplicaSet"]["topology"]).length)
EXPECT_EQ("ONLINE", status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port1}`]["status"])

//@ Add instance 2
cluster.addInstance(__sandbox_uri2);

// Waiting for the second added instance to become online
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");

//@<> Persist the configuration on the cnf file {VER(<8.0.11)}
var cnfPath1 = testutil.getSandboxConfPath(__mysql_sandbox_port1);
var cnfPath2 = testutil.getSandboxConfPath(__mysql_sandbox_port2);

// Test that even if a group_replication variable already exists in the config file, it gets
// the loose_ prefix after the configureInstance
testutil.changeSandboxConf(__mysql_sandbox_port1, "group_replication_ssl_mode", "DISABLED");
testutil.changeSandboxConf(__mysql_sandbox_port2, "group_replication_ssl_mode", "DISABLED");

dba.configureLocalInstance(__sandbox_uri1, {mycnfPath: cnfPath1});
dba.configureLocalInstance(__sandbox_uri2, {mycnfPath: cnfPath2});

//@<> reboot with GR plugin uninstalled
// covers Bug#30531848 RESTARTED INNODB CLUSTER NOT FINDING GR PLUGIN
// and BUG#30768504 REBOOTCLUSTER PARTIAL FAILURE IF MYSQL 5.7 IF GR PLUGIN UNINSTALLED

session1 = mysql.getSession(__sandbox_uri1);
session2 = mysql.getSession(__sandbox_uri2);
session3 = mysql.getSession(__sandbox_uri3);

var vars1 = session1.runSql("show variables like 'group_replication%'").fetchAll();
var vars2 = session2.runSql("show variables like 'group_replication%'").fetchAll();

session1.runSql("STOP group_replication");
session1.runSql("set global super_read_only=0");
session1.runSql("set sql_log_bin=0");
session1.runSql("/*!80000 set persist group_replication_start_on_boot=0 */");
session1.runSql("uninstall plugin group_replication");

session2.runSql("STOP group_replication");
session2.runSql("set global super_read_only=0");
session2.runSql("set sql_log_bin=0");
session2.runSql("/*!80000 set persist group_replication_start_on_boot=0 */");
session2.runSql("uninstall plugin group_replication");

disable_auto_rejoin(__mysql_sandbox_port1);
disable_auto_rejoin(__mysql_sandbox_port2);

shell.connect(__sandbox_uri1);

// Enable offline_mode (BUG#33396423)
session.runSql("SET GLOBAL offline_mode=1");

cluster = dba.rebootClusterFromCompleteOutage("dev");

session2.runSql("/*!80000 set persist group_replication_start_on_boot=1 */");

// ensure configs after reboot are the same as before
EXPECT_EQ(vars1, session1.runSql("show variables like 'group_replication%'").fetchAll());
EXPECT_EQ(vars2, session2.runSql("show variables like 'group_replication%'").fetchAll());

// ensure offline_mode was disabled (BUG#33396423)
EXPECT_EQ(0, session.runSql("select @@offline_mode").fetchOne()[0]);

//@ Add instance 3
session.close();
// session is stored on the cluster object so changing the global session should not affect cluster operations
shell.connect(__sandbox_uri2);

cluster.addInstance(__sandbox_uri3);

// Waiting for the third added instance to become online
testutil.waitMemberState(__mysql_sandbox_port3, "ONLINE");

//@<OUT> persist GR configuration settings for 5.7 servers {VER(<8.0.11)}
var mycnf1 = testutil.getSandboxConfPath(__mysql_sandbox_port1);
var mycnf2 = testutil.getSandboxConfPath(__mysql_sandbox_port2);
var mycnf3 = testutil.getSandboxConfPath(__mysql_sandbox_port3);
dba.configureLocalInstance('root:root@localhost:' + __mysql_sandbox_port1, {mycnfPath: mycnf1});
dba.configureLocalInstance('root:root@localhost:' + __mysql_sandbox_port2, {mycnfPath: mycnf2});
dba.configureLocalInstance('root:root@localhost:' + __mysql_sandbox_port3, {mycnfPath: mycnf3});

//@<> check group_seeds correctly persisted {VER(<8.0.11)}
EXPECT_EQ(`${hostname}:${__mysql_sandbox_gr_port2},${hostname}:${__mysql_sandbox_gr_port3}`, testutil.getSandboxConf(__mysql_sandbox_port1, "loose_group_replication_group_seeds"));
EXPECT_EQ(`${hostname}:${__mysql_sandbox_gr_port1},${hostname}:${__mysql_sandbox_gr_port3}`, testutil.getSandboxConf(__mysql_sandbox_port2, "loose_group_replication_group_seeds"));
EXPECT_EQ(`${hostname}:${__mysql_sandbox_gr_port1},${hostname}:${__mysql_sandbox_gr_port2}`, testutil.getSandboxConf(__mysql_sandbox_port3, "loose_group_replication_group_seeds"));

//@<> check group_seeds correctly set
EXPECT_EQ(`${hostname}:${__mysql_sandbox_gr_port2},${hostname}:${__mysql_sandbox_gr_port3}`, session1.runSql("select @@group_replication_group_seeds").fetchOne()[0]);
EXPECT_EQ(`${hostname}:${__mysql_sandbox_gr_port1},${hostname}:${__mysql_sandbox_gr_port3}`, session2.runSql("select @@group_replication_group_seeds").fetchOne()[0]);
EXPECT_EQ(`${hostname}:${__mysql_sandbox_gr_port1},${hostname}:${__mysql_sandbox_gr_port2}`, session3.runSql("select @@group_replication_group_seeds").fetchOne()[0]);

//@<> Dba.rebootClusterFromCompleteOutage errors
EXPECT_THROWS(function() {
    dba.rebootClusterFromCompleteOutage("");
}, "The cluster with the name '' does not exist.");
EXPECT_THROWS(function() {
    dba.rebootClusterFromCompleteOutage("dev", {invalidOpt: "foobar"});
}, "Invalid options: invalidOpt");
EXPECT_THROWS(function() {
    dba.rebootClusterFromCompleteOutage("dev2");
}, "The cluster with the name 'dev2' does not exist.");

// Regression for BUG#27508627: rebootClusterFromCompleteOutage should not point to use forceQuorumUsingPartitionOf
testutil.wipeAllOutput();
EXPECT_THROWS(function() {
    dba.rebootClusterFromCompleteOutage("dev");
}, "The Cluster is ONLINE");
EXPECT_OUTPUT_CONTAINS(`Cluster instances: '${hostname}:${__mysql_sandbox_port1}' (ONLINE), '${hostname}:${__mysql_sandbox_port2}' (ONLINE), '${hostname}:${__mysql_sandbox_port3}' (ONLINE)`);

// Connect to instance 1 to properly check status of other killed instances.
session.close();
shell.connect(__sandbox_uri1);

//@ Get data about existing replication users before reboot.
//Regression for BUG#27344040: dba.rebootClusterFromCompleteOutage() should not create new user
var rpl_users_rows = get_rpl_users();
session.close();

//@<> Reset gr_start_on_boot on all instances
disable_auto_rejoin(__mysql_sandbox_port1);
disable_auto_rejoin(__mysql_sandbox_port2);
disable_auto_rejoin(__mysql_sandbox_port3);

// Kill all the instances
// Connect to instance 1 to properly check status of other killed instances.
shell.connect(__sandbox_uri1);

// Kill instance 2 (we don't need a real kill b/c there's still quorum anyway)
testutil.stopSandbox(__mysql_sandbox_port2);

// Since the cluster has quorum, the instance will be kicked off the
// Cluster going OFFLINE->UNREACHABLE->(MISSING)
testutil.waitMemberState(__mysql_sandbox_port2, "(MISSING)");

// Kill instance 3
testutil.killSandbox(__mysql_sandbox_port3);

// Waiting for the third added instance to become unreachable
// Will remain unreachable since there's no quorum to kick it off
testutil.waitMemberState(__mysql_sandbox_port3, "UNREACHABLE");

//@<> Reboot cluster fails because instance is online and there is no quorum.
// Regression for BUG#27508627: rebootClusterFromCompleteOutage should not point to use forceQuorumUsingPartitionOf
EXPECT_THROWS(function() {
    dba.rebootClusterFromCompleteOutage();
}, `The MySQL instance '${hostname}:${__mysql_sandbox_port1}' belongs to an InnoDB Cluster and is reachable. Please use <Cluster>.forceQuorumUsingPartitionOf() to restore from the quorum loss.`);

// Kill instance 1
testutil.killSandbox(__mysql_sandbox_port1);

// Re-start all the instances except instance 3

// Start instance 2
testutil.startSandbox(__mysql_sandbox_port2);

// Start instance 1
testutil.startSandbox(__mysql_sandbox_port1);

session.close();

cluster.disconnect();

// Re-establish the connection to instance 1
shell.connect(__sandbox_uri1);

//@<> Dba.rebootClusterFromCompleteOutage error unreachable server
EXPECT_THROWS(function () {
    cluster = dba.rebootClusterFromCompleteOutage("dev");
}, "Could not determine if Cluster is completely OFFLINE");
EXPECT_OUTPUT_CONTAINS(`WARNING: One or more instances of the Cluster could not be reached and cannot be rejoined nor ensured to be OFFLINE: '${hostname}:${__mysql_sandbox_port3}'. Cluster may diverge and become inconsistent unless all instances are either reachable or certain to be OFFLINE and not accepting new transactions. You may use the 'force' option to bypass this check and proceed anyway.`);

//@<> Dba.rebootClusterFromCompleteOutage success
EXPECT_NO_THROWS(function () { cluster = dba.rebootClusterFromCompleteOutage("dev", {force: true, user:'root', password:'root', clearReadOnly: true}); });
EXPECT_OUTPUT_CONTAINS("The 'clearReadOnly' option is no longer used (it's deprecated): super_read_only is automatically cleared.");
EXPECT_OUTPUT_CONTAINS("The 'user' option is no longer used (it's deprecated): the connection data is taken from the active shell session.");
EXPECT_OUTPUT_CONTAINS("The 'password' option is no longer used (it's deprecated): the connection data is taken from the active shell session.");
EXPECT_OUTPUT_CONTAINS(`The instance '${hostname}:${__mysql_sandbox_port2}' was successfully rejoined to the cluster.`);
EXPECT_OUTPUT_CONTAINS("The Cluster was successfully rebooted.");
// |WARNING: The user option is deprecated and will be removed in a future release. If not specified, the user name is taken from the active session.|
// |WARNING: The password option is deprecated and will be removed in a future release. If not specified, the password is taken from the active session.|

// Waiting for the second added instance to become online
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");

//@<> Confirm no new replication user was created on bootstrap member.
//Regression for BUG#27344040: dba.rebootClusterFromCompleteOutage() should not create new user
EXPECT_EQ(false, has_new_rpl_users(rpl_users_rows));

//@<> cluster status after reboot
var status = cluster.status();
status
EXPECT_EQ(3, Object.keys(status["defaultReplicaSet"]["topology"]).length)
EXPECT_EQ("ONLINE", status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port1}`]["status"])
EXPECT_EQ("ONLINE", status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port2}`]["status"])
EXPECT_EQ("(MISSING)", status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port3}`]["status"])
EXPECT_EQ("R/W", status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port1}`]["mode"])
EXPECT_EQ("R/O", status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port2}`]["mode"])

cluster.disconnect();
session.close();

//@<> Confirm no new replication user was created on other rejoining member.
//Regression for BUG#27344040: dba.rebootClusterFromCompleteOutage() should not create new user
shell.connect(__sandbox_uri2);
EXPECT_EQ(false, has_new_rpl_users(rpl_users_rows));
session.close();

//@<> Reboot cluster resets group_seeds because peer members could be unreachable
// Bug #33389693 Can't reboot after total outage with group_seeds set
shell.connect(__sandbox_uri2);
session.runSql("stop group_replication");
shell.connect(__sandbox_uri1);
session.runSql("stop group_replication");

session.runSql("set global group_replication_group_seeds='127.0.0.1:"+__mysql_sandbox_gr_port2+",unreachable:1234'")

shell.options.useWizards=0;
c = dba.rebootClusterFromCompleteOutage("dev", {force: true});

EXPECT_EQ(`${hostname}:${__mysql_sandbox_gr_port2},${hostname}:${__mysql_sandbox_gr_port3}`, session.runSql("select @@group_replication_group_seeds").fetchOne()[0]);

// ensure group_seeds has the correct value in each member
shell.connect(__sandbox_uri2);
EXPECT_EQ(`${hostname}:${__mysql_sandbox_gr_port1},${hostname}:${__mysql_sandbox_gr_port3}`, session.runSql("select @@group_replication_group_seeds").fetchOne()[0]);

//@<> Finalization
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
testutil.destroySandbox(__mysql_sandbox_port3);
