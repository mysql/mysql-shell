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

session1.close();
testutil.restartSandbox(__mysql_sandbox_port1);
session1 = mysql.getSession(__sandbox_uri1);
session2.close();
testutil.restartSandbox(__mysql_sandbox_port2);
session2 = mysql.getSession(__sandbox_uri2);

shell.connect(__sandbox_uri1);

// Enable offline_mode (BUG#33396423)
session.runSql("SET GLOBAL offline_mode=1");

cluster = dba.rebootClusterFromCompleteOutage("dev", {rejoinInstances:[__sandbox_uri2]});

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

//@ Dba.rebootClusterFromCompleteOutage errors
dba.rebootClusterFromCompleteOutage("");
dba.rebootClusterFromCompleteOutage("dev", {invalidOpt: "foobar"});
dba.rebootClusterFromCompleteOutage("dev2");
// Regression for BUG#27508627: rebootClusterFromCompleteOutage should not point to use forceQuorumUsingPartitionOf
dba.rebootClusterFromCompleteOutage("dev");

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

//@ Reboot cluster fails because instance is online and there is no quorum.
// Regression for BUG#27508627: rebootClusterFromCompleteOutage should not point to use forceQuorumUsingPartitionOf
dba.rebootClusterFromCompleteOutage("dev");

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

var instance2 = localhost + ':' + __mysql_sandbox_port2;
var instance3 = hostname + ':' + __mysql_sandbox_port3;

//@ Dba.rebootClusterFromCompleteOutage error unreachable server cannot be on the rejoinInstances list
cluster = dba.rebootClusterFromCompleteOutage("dev", {rejoinInstances: [instance3]});

//@ Dba.rebootClusterFromCompleteOutage error cannot use same server on both rejoinInstances and removeInstances list
cluster = dba.rebootClusterFromCompleteOutage("dev", {rejoinInstances: [instance2], removeInstances: [instance2]});

// Test both rejoinInstances and removeInstances on a single call
//@ Dba.rebootClusterFromCompleteOutage success
cluster = dba.rebootClusterFromCompleteOutage("dev", {user:'root', password:'root', rejoinInstances: [instance2], removeInstances: [instance3], clearReadOnly: true});

// Waiting for the second added instance to become online
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");

//@<OUT> Confirm no new replication user was created on bootstrap member.
//Regression for BUG#27344040: dba.rebootClusterFromCompleteOutage() should not create new user
print(has_new_rpl_users(rpl_users_rows) + "\n");

//@<> cluster status after reboot
var status = cluster.status();
EXPECT_EQ(2, Object.keys(status["defaultReplicaSet"]["topology"]).length)
EXPECT_EQ("ONLINE", status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port1}`]["status"])
EXPECT_EQ("ONLINE", status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port2}`]["status"])
EXPECT_EQ("R/W", status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port1}`]["mode"])
EXPECT_EQ("R/O", status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port2}`]["mode"])

cluster.disconnect();
session.close();

//@<OUT> Confirm no new replication user was created on other rejoining member.
//Regression for BUG#27344040: dba.rebootClusterFromCompleteOutage() should not create new user
shell.connect(__sandbox_uri2);
print(has_new_rpl_users(rpl_users_rows) + "\n");
session.close();

//@ Finalization
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
testutil.destroySandbox(__mysql_sandbox_port3);
