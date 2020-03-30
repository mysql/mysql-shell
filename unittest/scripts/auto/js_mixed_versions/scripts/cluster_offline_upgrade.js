//@ {DEF(MYSQLD57_PATH) && VER(>=8.0.16)}
// Restricting to versions >= 8.0.16 since as of MySQL 8.0.16, the server performs all tasks previously handled by
// mysql_upgrade.

// This test is to simulate an offline in-place upgrade of 5.7 cluster to 8.0.
// https://dev.mysql.com/doc/refman/8.0/en/group-replication-offline-upgrade.html
// https://dev.mysql.com/doc/refman/8.0/en/upgrading.html

//@<> Deploy raw sandbox 1 using 5.7 server
testutil.deployRawSandbox(__mysql_sandbox_port1, "root", { report_host: hostname }, {mysqldPath: MYSQLD57_PATH});
testutil.snapshotSandboxConf(__mysql_sandbox_port1);
var mycnf_path1 = testutil.getSandboxConfPath(__mysql_sandbox_port1);

//@<> Deploy sandbox 2 using 5.7 server
testutil.deploySandbox(__mysql_sandbox_port2, "root", { report_host: hostname }, {mysqldPath: MYSQLD57_PATH});
testutil.snapshotSandboxConf(__mysql_sandbox_port2);
var mycnf_path2 = testutil.getSandboxConfPath(__mysql_sandbox_port2);

//@<> Setup
shell.options.useWizards = false;

//@<> configureLocalInstance
dba.configureLocalInstance(__sandbox_uri1, {mycnfPath: testutil.getSandboxConfPath(__mysql_sandbox_port1), clusterAdmin:CLUSTER_ADMIN, clusterAdminPassword:CLUSTER_ADMIN_PWD});
testutil.restartSandbox(__mysql_sandbox_port1);

//@<> configureInstance
dba.configureInstance(__sandbox_uri2, {mycnfPath: testutil.getSandboxConfPath(__mysql_sandbox_port2), clusterAdmin:CLUSTER_ADMIN, clusterAdminPassword:CLUSTER_ADMIN_PWD});

//@<> createCluster on 5.7 instance
var cluster;
shell.connect(__sandbox_admin_uri1);
cluster = dba.createCluster("mycluster");

//@<> status
status = cluster.status();

//@<> checkInstanceState
cluster.checkInstanceState(__sandbox_admin_uri2);

//@<> describe
cluster.describe();

//@<> disconnect
cluster.disconnect();

//@<> getCluster
cluster = dba.getCluster();

//@<> addInstance
cluster.addInstance(__sandbox_admin_uri2, {recoveryMethod:'incremental'});

// persist configuration after instances were added to cluster
dba.configureLocalInstance(__sandbox_uri1, {mycnfPath: testutil.getSandboxConfPath(__mysql_sandbox_port1)});
dba.configureLocalInstance(__sandbox_uri2, {mycnfPath: testutil.getSandboxConfPath(__mysql_sandbox_port2)});

//NOTE: Required workaround until configureLocalInstance is fixed otherwise the upgrade would fail because of all
// the group_replication variables persisted without the loose prefix.
// get list of group_replication vars

session1 = mysql.getSession(__sandbox_admin_uri1);
session2 = mysql.getSession(__sandbox_admin_uri2);

gr_vars = [];
var gr_vars_with_vals = session1.runSql("show variables like 'group_replication%'").fetchAll();

for (var index = 0; index < gr_vars_with_vals.length; ++index){
    gr_vars.push(gr_vars_with_vals[index][0]);
}

for (var index = 0; index < gr_vars.length; ++index){
    var gr_var = gr_vars[index];
    // get current values
    vals1 = get_sysvar(session1, gr_var);
    vals2 = get_sysvar(session2, gr_var);
    // drop variable from config file
    testutil.removeFromSandboxConf(__mysql_sandbox_port1, gr_var);
    testutil.removeFromSandboxConf(__mysql_sandbox_port2, gr_var);
    // add it back with loose_prefix
    testutil.changeSandboxConf(__mysql_sandbox_port1, "loose_" + gr_var, String(vals1));
    testutil.changeSandboxConf(__mysql_sandbox_port2, "loose_" + gr_var, String(vals2));
}

session1.close();
session2.close();

//@<> Shutdown the cluster and upgrade the mysqld on both
testutil.changeSandboxConf(__mysql_sandbox_port1, 'loose_group_replication_start_on_boot', 'OFF');
testutil.changeSandboxConf(__mysql_sandbox_port2, 'loose_group_replication_start_on_boot', 'OFF');

testutil.stopSandbox(__mysql_sandbox_port1, {wait:1});
testutil.stopSandbox(__mysql_sandbox_port2, {wait:1});
testutil.upgradeSandbox(__mysql_sandbox_port1);
testutil.upgradeSandbox(__mysql_sandbox_port2);

//@<> Restart the sandboxes and wait for them to come alive
testutil.startSandbox(__mysql_sandbox_port1, {timeout: 120});
testutil.startSandbox(__mysql_sandbox_port2, {timeout: 120});
testutil.waitSandboxAlive(__mysql_sandbox_port1);
testutil.waitSandboxAlive(__mysql_sandbox_port2);

//@<> Restore the cluster object
shell.connect(__sandbox_admin_uri1);
cluster = dba.rebootClusterFromCompleteOutage();
// TODO(alfredo) - the reboot should auto-rejoin all members
cluster.rejoinInstance(__sandbox_admin_uri2);
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");

//@<> Deploy sandbox 3 using base server
testutil.deploySandbox(__mysql_sandbox_port3, "root", { report_host: hostname });
testutil.snapshotSandboxConf(__mysql_sandbox_port3);
dba.configureLocalInstance(__sandbox_uri3, {clusterAdmin:CLUSTER_ADMIN, clusterAdminPassword:CLUSTER_ADMIN_PWD});

//@<> Add Instance 3
cluster.addInstance(__sandbox_admin_uri3, {recoveryMethod:'incremental'});
testutil.waitMemberState(__mysql_sandbox_port3, "ONLINE");

//@<> Upgrade admin Account
shell.connect(__sandbox_uri1);
cluster = dba.getCluster();

// Hack because of BUG#30783149
if (__version_num <= 80022) {
  session1 = mysql.getSession(__sandbox_admin_uri1);
  session1.runSql("GRANT REPLICATION_APPLIER ON *.* TO 'root'@'localhost' WITH GRANT OPTION");
  session1.close();
}

cluster.setupAdminAccount(CLUSTER_ADMIN, {update:true, password:CLUSTER_ADMIN_PWD});
shell.connect(__sandbox_admin_uri1);
cluster = dba.getCluster();

//@<> Enable clone support on cluster
cluster.setOption("disableClone", false);

//@<> Deploy sandbox 4 using base server
testutil.deploySandbox(__mysql_sandbox_port4, "root", { report_host: hostname });
testutil.snapshotSandboxConf(__mysql_sandbox_port4);
dba.configureLocalInstance(__sandbox_uri4, {clusterAdmin:CLUSTER_ADMIN, clusterAdminPassword:CLUSTER_ADMIN_PWD});

//@<> Add Instance 4
cluster.addInstance(__sandbox_admin_uri4, {recoveryMethod:'clone'});
testutil.waitMemberState(__mysql_sandbox_port4, "ONLINE");


//@<> removeInstance
cluster.removeInstance(__sandbox_admin_uri2);
cluster.addInstance(__sandbox_admin_uri2, {recoveryMethod:'incremental'});

//@<> setPrimaryInstance
cluster.setPrimaryInstance(__sandbox_admin_uri2);
cluster.setPrimaryInstance(__sandbox_admin_uri3);
cluster.setPrimaryInstance(__sandbox_admin_uri1);

//@<> rejoinInstance
session2 = mysql.getSession(__sandbox_admin_uri2);
session2.runSql("STOP group_replication");

cluster.rejoinInstance(__sandbox_admin_uri2);

//@<> forceQuorumUsingPartitionOf
testutil.killSandbox(__mysql_sandbox_port2);
testutil.killSandbox(__mysql_sandbox_port3);
testutil.waitMemberState(__mysql_sandbox_port2, "(MISSING),UNREACHABLE");
testutil.waitMemberState(__mysql_sandbox_port3, "(MISSING),UNREACHABLE");

cluster.forceQuorumUsingPartitionOf(__sandbox_admin_uri1);

testutil.startSandbox(__mysql_sandbox_port2);
testutil.waitSandboxAlive(__mysql_sandbox_port2);
session2 = mysql.getSession(__sandbox_admin_uri2);

cluster.rejoinInstance(__sandbox_admin_uri2);
testutil.waitMemberState(__mysql_sandbox_port3, "(MISSING)");

testutil.startSandbox(__mysql_sandbox_port3);
testutil.waitSandboxAlive(__mysql_sandbox_port3);
session3 = mysql.getSession(__sandbox_admin_uri3);
cluster.rejoinInstance(__sandbox_admin_uri3);
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");
testutil.waitMemberState(__mysql_sandbox_port3, "ONLINE");

//@<> setOption
cluster.setOption("clusterName", "clooster");

custom_weigth=50;

//@<> setInstanceOption
custom_weigth=20;
cluster.setInstanceOption(__sandbox_admin_uri1, "memberWeight", custom_weigth);

//@<> options
cluster.options();

//@<> switchToMultiPrimaryMode
cluster.switchToMultiPrimaryMode();

//@<> switchToSinglePrimaryMode
cluster.switchToSinglePrimaryMode();

cluster.setPrimaryInstance(__sandbox_admin_uri1);

shell.connect(__sandbox_admin_uri1);
cluster = dba.getCluster(); // shouldn't be necessary

//@<> rescan
// delete sb2 from the metadata so that rescan picks it up
session.runSql("DELETE FROM mysql_innodb_cluster_metadata.instances WHERE instance_name LIKE ?", ["%:"+__mysql_sandbox_port2]);

cluster.rescan();

//@<> listRouters
cluster_id = session.runSql("SELECT cluster_id FROM mysql_innodb_cluster_metadata.clusters").fetchOne()[0];
session.runSql("INSERT mysql_innodb_cluster_metadata.routers VALUES (DEFAULT, 'system', 'mysqlrouter', 'routerhost1', '8.0.18', '2019-01-01 11:22:33', NULL, ?, NULL)", [cluster_id]);
session.runSql("INSERT mysql_innodb_cluster_metadata.routers VALUES (DEFAULT, 'system', 'mysqlrouter', 'routerhost2', '8.0.18', '2019-01-01 11:22:33', NULL, ?, NULL)", [cluster_id]);

cluster.listRouters();

//@<> removeRouterMetadata
cluster.removeRouterMetadata("routerhost1::system");

cluster.listRouters();

//@<> createCluster(adopt)
session.runSql("DROP SCHEMA mysql_innodb_cluster_metadata");
cluster = dba.createCluster("adopted", {adoptFromGR:true});

cluster.status();

//@<> dissolve
cluster.dissolve();

//@<> Finalize
EXPECT_NO_THROWS(function () {
    cluster.disconnect();
    session.close();
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
testutil.destroySandbox(__mysql_sandbox_port3);
testutil.destroySandbox(__mysql_sandbox_port4);
}, "Cleaning Up Test Case");
