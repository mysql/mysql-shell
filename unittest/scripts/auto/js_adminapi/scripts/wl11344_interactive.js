function print_persisted_variables(session) {
    var res = session.runSql("SELECT * from performance_schema.persisted_variables WHERE Variable_name like '%group_replication%'").fetchAll();
    for (var i = 0; i < res.length; i++) {
        print(res[i][0] + " = " + res[i][1] + "\n");
    }
    print("\n");
}

// Connect to sandbox1
//FR1 - On a successful dba.createCluster() call, the group replication sysvars must be updated and persisted at the seed instance.
//@ FR1-TS-01 SETUP {VER(>=8.0.11)}
testutil.deploySandbox(__mysql_sandbox_port1, "root");
testutil.snapshotSandboxConf(__mysql_sandbox_port1);
shell.connect(__sandbox_uri1);
var cluster = dba.createCluster("C", {groupName: "ca94447b-e6fc-11e7-b69d-4485005154dc"});
cluster.disconnect();

//@ FR1-TS-01 Check persisted variables after create cluster {VER(>=8.0.11)}
print_persisted_variables(session);

//@ FR1-TS-01 reboot instance {VER(>=8.0.11)}
session.close();
testutil.stopSandbox(__mysql_sandbox_port1);
testutil.startSandbox(__mysql_sandbox_port1);
shell.connect(__sandbox_uri1);

//@ FR1-TS-01 reboot cluster and check persisted variables {VER(>=8.0.11)}
cluster = dba.rebootClusterFromCompleteOutage("C");
print_persisted_variables(session);
cluster.status();

//@ FR1-TS-01 TEARDOWN {VER(>=8.0.11)}
cluster.disconnect();
session.close();
testutil.destroySandbox(__mysql_sandbox_port1);

//@ FR1-TS-03 SETUP {VER(>=8.0.11)}
testutil.deploySandbox(__mysql_sandbox_port1, "root");
testutil.snapshotSandboxConf(__mysql_sandbox_port1);
testutil.stopSandbox(__mysql_sandbox_port1);
testutil.changeSandboxConf(__mysql_sandbox_port1, "persisted-globals-load",
    "OFF");
testutil.startSandbox(__mysql_sandbox_port1);
shell.connect(__sandbox_uri1);

//@ FR1-TS-03 {VER(>=8.0.11)}
WIPE_SHELL_LOG();
// there should be a warning message asking to use configureInstance locally since persisted_globals_load is OFF
var msg = "Warning: On instance '" + localhost + ":" + __mysql_sandbox_port1 + "' the persisted cluster configuration will not be loaded upon reboot since 'persisted-globals-load' is set to 'OFF'. Please use the <Dba>.configureLocalInstance command locally to persist the changes or set 'persisted-globals-load' to 'ON' on the configuration file.";
cluster = dba.createCluster("C", {groupName: "ca94447b-e6fc-11e7-b69d-4485005154dc"});
EXPECT_SHELL_LOG_CONTAINS(msg);

//@ FR1-TS-03 TEARDOWN {VER(>=8.0.11)}
cluster.disconnect();
session.close();
testutil.destroySandbox(__mysql_sandbox_port1);

//@ FR1-TS-04/05 SETUP {VER(>=8.0.11)}
testutil.deployRawSandbox(__mysql_sandbox_port1, 'root', {'report_host': hostname});
//answer to create a new root@% account
testutil.expectPrompt("Please select an option [1]: ", "1");
testutil.expectPrompt("Account Host:", "%");
//accept configuration changes
testutil.expectPrompt("Do you want to perform the required configuration changes? [y/n]:", "Y");
testutil.expectPrompt("Do you want to restart the instance after configuring it?", "y");
dba.configureLocalInstance(__sandbox_uri1);
testutil.stopSandbox(__mysql_sandbox_port1);
testutil.startSandbox(__mysql_sandbox_port1);
testutil.snapshotSandboxConf(__mysql_sandbox_port1);
shell.connect(__hostname_uri1);
cluster = dba.createCluster("ClusterName", {localAddress: "localhost:" + __mysql_sandbox_port2, groupName: "62d73bbd-b830-11e7-a7b7-34e6d72fbd80", ipWhitelist:"255.255.255.255/32,127.0.0.1," + hostname_ip});
//@ FR1-TS-04/05 {VER(>=8.0.11)}
print_persisted_variables(session);
var sandbox_cnf1 = testutil.getSandboxConfPath(__mysql_sandbox_port1);
dba.configureLocalInstance(__sandbox_uri1, {interactive: true, mycnfPath: sandbox_cnf1});

//@ FR1-TS-04/05 TEARDOWN {VER(>=8.0.11)}
cluster.disconnect();
session.close();
testutil.destroySandbox(__mysql_sandbox_port1);

//@ FR1-TS-06 SETUP {VER(<8.0.11)}
testutil.deploySandbox(__mysql_sandbox_port1, "root");
testutil.snapshotSandboxConf(__mysql_sandbox_port1);
shell.connect(__sandbox_uri1);

//@ FR1-TS-06 {VER(<8.0.11)}
WIPE_SHELL_LOG();
var msg = "Warning: On instance '" + localhost + ":" + __mysql_sandbox_port1 + "' membership change cannot be persisted since MySQL version " + __version + " does not support the SET PERSIST command (MySQL version >= 8.0.11 required). Please use the <Dba>.configureLocalInstance command locally to persist the changes.";
cluster = dba.createCluster("ClusterName", {groupName: "ca94447b-e6fc-11e7-b69d-4485005154dc"});
EXPECT_SHELL_LOG_CONTAINS(msg);

//@ FR1-TS-06 TEARDOWN {VER(<8.0.11)}
cluster.disconnect();
session.close();
testutil.destroySandbox(__mysql_sandbox_port1);


//@ FR1-TS-7 SETUP {VER(>=8.0.11)}
testutil.deploySandbox(__mysql_sandbox_port1, "root");
shell.connect(__sandbox_uri1);
dba.createCluster("ClusterName",  {multiPrimary: true, force: true, groupName: "ca94447b-e6fc-11e7-b69d-4485005154dc"});

//@ FR1-TS-7 show persisted cluster variables {VER(>=8.0.11)}
print_persisted_variables(session);

//@ FR1-TS-7 reboot instance 1 {VER(>=8.0.11)}
session.close();
testutil.stopSandbox(__mysql_sandbox_port1);
testutil.startSandbox(__mysql_sandbox_port1);
shell.connect(__sandbox_uri1);

//@ FR1-TS-7 reboot cluster and check persisted variables {VER(>=8.0.11)}
cluster = dba.rebootClusterFromCompleteOutage("ClusterName");
print_persisted_variables(session);
cluster.status();

//@ FR1-TS-7 TEARDOWN {VER(>=8.0.11)}
cluster.disconnect();
session.close();
testutil.destroySandbox(__mysql_sandbox_port1);

//FR2 -On a successful .addInstance() call, the group replication sysvars must
// be updated and persisted at the joining instance.
//@FR2-TS-1 SETUP {VER(>=8.0.11)}
testutil.deploySandbox(__mysql_sandbox_port1, "root");
testutil.deploySandbox(__mysql_sandbox_port2, "root");
var s2 = mysql.getSession(__sandbox_uri2);
shell.connect(__sandbox_uri1);
dba.createCluster("ClusterName", {groupName: "ca94447b-e6fc-11e7-b69d-4485005154dc"});
cluster = dba.getCluster("ClusterName");
cluster.addInstance(__sandbox_uri2);
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");

//@ FR2-TS-1 check persisted variables on instance 1 {VER(>=8.0.11)}
print_persisted_variables(s2);

//@ FR2-TS-1 stop instance 2 {VER(>=8.0.11)}
testutil.stopSandbox(__mysql_sandbox_port2);
testutil.waitMemberState(__mysql_sandbox_port2, "(MISSING)");

//@ FR2-TS-1 cluster status showing instance 2 is missing {VER(>=8.0.11)}
cluster.status();

//@ FR2-TS-1 start instance 1 {VER(>=8.0.11)}
testutil.startSandbox(__mysql_sandbox_port2);
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");

//@ FR2-TS-1 cluster status showing instance 2 is back online {VER(>=8.0.11)}
cluster.status();

//@ FR2-TS-1 TEARDOWN {VER(>=8.0.11)}
cluster.disconnect();
s2.close();
session.close();
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);

//@ FR2-TS-3 SETUP {VER(>=8.0.11)}
testutil.deploySandbox(__mysql_sandbox_port1, "root");
testutil.deploySandbox(__mysql_sandbox_port2, "root");
testutil.snapshotSandboxConf(__mysql_sandbox_port2);
testutil.stopSandbox(__mysql_sandbox_port2);
testutil.changeSandboxConf(__mysql_sandbox_port2, "persisted-globals-load",
    "OFF");
testutil.startSandbox(__mysql_sandbox_port2);
shell.connect(__sandbox_uri1);
dba.createCluster("ClusterName", {groupName: "ca94447b-e6fc-11e7-b69d-4485005154dc"});
cluster = dba.getCluster("ClusterName");

//@ FR2-TS-3 check that warning is displayed when adding instance with persisted-globals-load=OFF {VER(>=8.0.11)}
WIPE_SHELL_LOG();
var msg = "Warning: On instance '" + localhost + ":" + __mysql_sandbox_port2 + "' the persisted cluster configuration will not be loaded upon reboot since 'persisted-globals-load' is set to 'OFF'. Please use the <Dba>.configureLocalInstance command locally to persist the changes or set 'persisted-globals-load' to 'ON' on the configuration file.";
cluster.addInstance(__sandbox_uri2);
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");
cluster.status();
EXPECT_SHELL_LOG_CONTAINS(msg);

//@ FR2-TS-3 TEARDOWN {VER(>=8.0.11)}
session.close();
cluster.disconnect();
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);

//@ FR2-TS-4 SETUP {VER(>=8.0.11)}
testutil.deploySandbox(__mysql_sandbox_port1, "root");
testutil.deploySandbox(__mysql_sandbox_port2, "root");
var s1 = mysql.getSession(__sandbox_uri1);
s2 = mysql.getSession(__sandbox_uri2);
shell.connect(__sandbox_uri1);
dba.createCluster("ClusterName", {groupName: "ca94447b-e6fc-11e7-b69d-4485005154dc", ipWhitelist:"255.255.255.255/32,127.0.0.1," + hostname_ip});
cluster = dba.getCluster("ClusterName");
cluster.addInstance(__sandbox_uri2, {localAddress: "localhost:" + __mysql_sandbox_port3, ipWhitelist:"255.255.255.255/32,127.0.0.1," + hostname_ip});
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");

//@ FR2-TS-4 Check that persisted variables match the ones passed on the arguments to create cluster and addInstance {VER(>=8.0.11)}
print_persisted_variables(s1);
print_persisted_variables(s2);

//@ FR2-TS-4 TEARDOWN {VER(>=8.0.11)}
session.close();
cluster.disconnect();
s1.close();
s2.close();
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);

//@ FR2-TS-5 SETUP {VER(>=8.0.11)}
testutil.deployRawSandbox(__mysql_sandbox_port1, 'root', {'report_host': hostname});
//answer to create a new root@% account
testutil.expectPrompt("Please select an option [1]: ", "1");
testutil.expectPrompt("Account Host:", "%");
//accept configuration changes
testutil.expectPrompt("Do you want to perform the required configuration changes? [y/n]:", "Y");
testutil.expectPrompt("Do you want to restart the instance after configuring it?", "n");
dba.configureLocalInstance(__sandbox_uri1);
testutil.stopSandbox(__mysql_sandbox_port1);
testutil.startSandbox(__mysql_sandbox_port1);

testutil.deployRawSandbox(__mysql_sandbox_port2, 'root', {'report_host': hostname});
//answer to create a new root@% account
testutil.expectPrompt("Please select an option [1]: ", "1");
testutil.expectPrompt("Account Host:", "%");
//accept configuration changes
testutil.expectPrompt("Do you want to perform the required configuration changes? [y/n]:", "Y");
testutil.expectPrompt("Do you want to restart the instance after configuring it?", "n");
dba.configureLocalInstance(__sandbox_uri2);
testutil.stopSandbox(__mysql_sandbox_port2);
testutil.startSandbox(__mysql_sandbox_port2);
testutil.snapshotSandboxConf(__mysql_sandbox_port2);
s1 = mysql.getSession(__sandbox_uri1);
s2 = mysql.getSession(__sandbox_uri2);
shell.connect(__hostname_uri1);
dba.createCluster("ClusterName", {groupName: "ca94447b-e6fc-11e7-b69d-4485005154dc"});
cluster = dba.getCluster("ClusterName");
cluster.addInstance(__hostname_uri2, {localAddress: "localhost:" + __mysql_sandbox_port3, ipWhitelist:"255.255.255.255/32,127.0.0.1," + hostname_ip});
session.close();
shell.connect(__hostname_uri2);
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");

//@ FR2-TS-5 {VER(>=8.0.11)}
print_persisted_variables(s1);
print_persisted_variables(s2);
var sandbox_cnf2 = testutil.getSandboxConfPath(__mysql_sandbox_port2);
dba.configureLocalInstance(__sandbox_uri2, {interactive: true, mycnfPath: sandbox_cnf2, clearReadOnly: true});
print_persisted_variables(s2);

//@ FR2-TS-5 TEARDOWN {VER(>=8.0.11)}
session.close();
cluster.disconnect();
s1.close();
s2.close();
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);

//@ FR2-TS-6 SETUP {VER(<8.0.11)}
WIPE_SHELL_LOG();
testutil.deploySandbox(__mysql_sandbox_port1, "root");
testutil.snapshotSandboxConf(__mysql_sandbox_port1);
testutil.deploySandbox(__mysql_sandbox_port2, "root");
testutil.snapshotSandboxConf(__mysql_sandbox_port2);
shell.connect(__sandbox_uri1);
dba.createCluster("ClusterName", {groupName: "ca94447b-e6fc-11e7-b69d-4485005154dc"});
cluster = dba.getCluster("ClusterName");

//@ FR2-TS-6 Warning is displayed on addInstance {VER(<8.0.11)}
// A warning about server version not supporting set persist syntax and asking
// to use ConfigureLocalInstance should be displaying in both the seed instance
// and the added instance.
var expected_msg1 = "Warning: On instance '" + localhost + ":" + __mysql_sandbox_port1 + "' membership change cannot be persisted since MySQL version " + __version + " does not support the SET PERSIST command (MySQL version >= 8.0.11 required). Please use the <Dba>.configureLocalInstance command locally to persist the changes.";
var expected_msg2 = "Warning: On instance '" + localhost + ":" + __mysql_sandbox_port2 + "' membership change cannot be persisted since MySQL version " + __version + " does not support the SET PERSIST command (MySQL version >= 8.0.11 required). Please use the <Dba>.configureLocalInstance command locally to persist the changes.";
cluster.addInstance(__sandbox_uri2);
EXPECT_SHELL_LOG_CONTAINS(expected_msg1);
EXPECT_SHELL_LOG_CONTAINS(expected_msg2);

//@ FR2-TS-6 TEARDOWN {VER(<8.0.11)}
session.close();
cluster.disconnect();
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);

//@ FR2-TS-8 SETUP {VER(>=8.0.11)}
testutil.deploySandbox(__mysql_sandbox_port1, "root");
testutil.snapshotSandboxConf(__mysql_sandbox_port1);
testutil.deploySandbox(__mysql_sandbox_port2, "root");
testutil.snapshotSandboxConf(__mysql_sandbox_port2);
s1 = mysql.getSession(__sandbox_uri1);
s2 = mysql.getSession(__sandbox_uri2);
shell.connect(__sandbox_uri1);
dba.createCluster("ClusterName", {multiPrimary: true, force: true, groupName: "ca94447b-e6fc-11e7-b69d-4485005154dc"});
cluster = dba.getCluster("ClusterName");
cluster.addInstance(__sandbox_uri2);
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");

//@ FR2-TS-8 Check that correct values were persisted and that instance rejoins automatically {VER(>=8.0.11)}
print_persisted_variables(s1);
print_persisted_variables(s2);
testutil.stopSandbox(__mysql_sandbox_port2);
testutil.waitMemberState(__mysql_sandbox_port2, "(MISSING)");
cluster.status();
testutil.startSandbox(__mysql_sandbox_port2);
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");
cluster.status();

//@ FR2-TS-8 TEARDOWN {VER(>=8.0.11)}
cluster.disconnect();
session.close();
s1.close();
s2.close();
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);

//@ FR2-TS-9 SETUP {VER(>=8.0.11)}
testutil.deploySandbox(__mysql_sandbox_port1, "root");
testutil.deploySandbox(__mysql_sandbox_port2, "root");
testutil.deploySandbox(__mysql_sandbox_port3, "root");
s1 = mysql.getSession(__sandbox_uri1);
s2 = mysql.getSession(__sandbox_uri2);
var s3 = mysql.getSession(__sandbox_uri3);
shell.connect(__sandbox_uri1);
dba.createCluster("ClusterName", {groupName: "ca94447b-e6fc-11e7-b69d-4485005154dc"});
cluster = dba.getCluster("ClusterName");
cluster.addInstance(__sandbox_uri2);
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");

//@ FR2-TS-9 Check that correct values were persisted on instance 2 {VER(>=8.0.11)}
print_persisted_variables(s2);

//@FR2-TS-9 Add instance 3 and wait for it to be online {VER(>=8.0.11)}
cluster.addInstance(__sandbox_uri3);
testutil.waitMemberState(__mysql_sandbox_port3, "ONLINE");

//@ FR2-TS-9 Check that correct values are persisted and updated when instances are added and that instances rejoin automatically {VER(>=8.0.11)}
print_persisted_variables(s3);
print_persisted_variables(s2);
print_persisted_variables(s1);

testutil.stopSandbox(__mysql_sandbox_port2);
testutil.waitMemberState(__mysql_sandbox_port2, "(MISSING)");
testutil.stopSandbox(__mysql_sandbox_port3);
testutil.waitMemberState(__mysql_sandbox_port3, "(MISSING)");
cluster.status();
testutil.startSandbox(__mysql_sandbox_port2);
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");
testutil.startSandbox(__mysql_sandbox_port3);
testutil.waitMemberState(__mysql_sandbox_port3, "ONLINE");
cluster.status();

//@ FR2-TS-9 TEARDOWN {VER(>=8.0.11)}
cluster.disconnect();
session.close();
s1.close();
s2.close();
s3.close();
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
testutil.destroySandbox(__mysql_sandbox_port3);

//FR5 - On a successful .removeInstance() call, the group replication rejoin
// sysvars must be cleared and persisted at the leaving instance
//@ FR5-TS-1 SETUP {VER(>=8.0.11)}
testutil.deploySandbox(__mysql_sandbox_port1, "root");
testutil.deploySandbox(__mysql_sandbox_port2, "root");
s1 = mysql.getSession(__sandbox_uri1);
s2 = mysql.getSession(__sandbox_uri2);
shell.connect(__sandbox_uri1);
dba.createCluster("ClusterName", {groupName: "ca94447b-e6fc-11e7-b69d-4485005154dc"});
cluster = dba.getCluster("ClusterName");
cluster.addInstance(__sandbox_uri2);
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");
cluster.removeInstance(__sandbox_uri2);

//@ FR5-TS-1 Check that persisted variables are updated/reset after removeCluster operation {VER(>=8.0.11)}
print_persisted_variables(s2);
print_persisted_variables(s1);

//@ FR5-TS-1 TEARDOWN {VER(>=8.0.11)}
cluster.disconnect();
session.close();
s1.close();
s2.close();
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);

//@ FR5-TS-4 SETUP {VER(>=8.0.11)}
testutil.deploySandbox(__mysql_sandbox_port1, "root");
testutil.deploySandbox(__mysql_sandbox_port2, "root");
testutil.deploySandbox(__mysql_sandbox_port3, "root");
s1 = mysql.getSession(__sandbox_uri1);
s2 = mysql.getSession(__sandbox_uri2);
s3 = mysql.getSession(__sandbox_uri3);
shell.connect(__sandbox_uri1);
dba.createCluster("ClusterName", {groupName: "ca94447b-e6fc-11e7-b69d-4485005154dc"});
cluster = dba.getCluster("ClusterName");
cluster.addInstance(__sandbox_uri2);
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");
cluster.addInstance(__sandbox_uri3);
testutil.waitMemberState(__mysql_sandbox_port3, "ONLINE");

//@ FR5-TS-4 Check that persisted variables are updated/reset after removeCluster operation {VER(>=8.0.11)}
print_persisted_variables(s3);
cluster.removeInstance(__sandbox_uri2);
print_persisted_variables(s3);
print_persisted_variables(s2);
print_persisted_variables(s1);

//@ FR5-TS-4 TEARDOWN {VER(>=8.0.11)}
cluster.disconnect();
session.close();
s1.close();
s2.close();
s3.close();
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
testutil.destroySandbox(__mysql_sandbox_port3);

//@ FR5-Extra SETUP {VER(<8.0.11)}
testutil.deploySandbox(__mysql_sandbox_port1, "root");
testutil.deploySandbox(__mysql_sandbox_port2, "root");
testutil.deploySandbox(__mysql_sandbox_port3, "root");
shell.connect(__sandbox_uri1);
dba.createCluster("ClusterName", {groupName: "ca94447b-e6fc-11e7-b69d-4485005154dc"});
cluster = dba.getCluster("ClusterName");
cluster.addInstance(__sandbox_uri2);
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");
cluster.addInstance(__sandbox_uri3);
testutil.waitMemberState(__mysql_sandbox_port3, "ONLINE");

//@ FR5-Extra Check that warning is shown when removeInstance is called {VER(<8.0.11)}
WIPE_SHELL_LOG();
cluster.removeInstance(__sandbox_uri2);
// there should be a warning message for each of the members staying in the group
var expected_msg1 = "Warning: On instance '" + localhost + ":" + __mysql_sandbox_port1 + "' membership change cannot be persisted since MySQL version " + __version + " does not support the SET PERSIST command (MySQL version >= 8.0.11 required). Please use the <Dba>.configureLocalInstance command locally to persist the changes.";
var expected_msg2 = "Warning: On instance '" + localhost + ":" + __mysql_sandbox_port3 + "' membership change cannot be persisted since MySQL version " + __version + " does not support the SET PERSIST command (MySQL version >= 8.0.11 required). Please use the <Dba>.configureLocalInstance command locally to persist the changes.";
// and a warning for the member that was removed
var expected_msg3 = "Warning: On instance '" + localhost + ":" + __mysql_sandbox_port2 + "' configuration cannot be persisted since MySQL version " + __version + " does not support the SET PERSIST command (MySQL version >= 8.0.11 required). Please set the 'group_replication_start_on_boot' variable to 'OFF' in the server configuration file, otherwise it might rejoin the cluster upon restart.";
EXPECT_SHELL_LOG_CONTAINS(expected_msg1);
EXPECT_SHELL_LOG_CONTAINS(expected_msg2);
EXPECT_SHELL_LOG_CONTAINS(expected_msg3);

cluster.status();

//@ FR5-Extra TEARDOWN {VER(<8.0.11)}
cluster.disconnect();
session.close();
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
testutil.destroySandbox(__mysql_sandbox_port3);

//@ Check if Cluster dissolve will reset persisted variables SETUP {VER(>=8.0.11)}
testutil.deploySandbox(__mysql_sandbox_port1, "root");
shell.connect(__sandbox_uri1);
dba.createCluster("ClusterName", {groupName: "ca94447b-e6fc-11e7-b69d-4485005154dc"});
cluster = dba.getCluster("ClusterName");
testutil.expectPrompt("Are you sure you want to dissolve the cluster?", "y");
cluster.dissolve({force:true});

//@ Check if Cluster dissolve will reset persisted variables {VER(>=8.0.11)}
print_persisted_variables(session);

//@ Check if Cluster dissolve will reset persisted variables TEARDOWN {VER(>=8.0.11)}
session.close();
testutil.destroySandbox(__mysql_sandbox_port1);
