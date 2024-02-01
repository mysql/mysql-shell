// Connect to sandbox1
//FR1 - On a successful dba.createCluster() call, the group replication sysvars must be updated and persisted at the seed instance.
//@ FR1-TS-01 SETUP {VER(>=8.0.12)}
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: hostname});
testutil.snapshotSandboxConf(__mysql_sandbox_port1);
shell.connect(__sandbox_uri1);
var cluster = dba.createCluster("C", {groupName: "ca94447b-e6fc-11e7-b69d-4485005154dc", gtidSetIsComplete: true});
var __gr_view_change_uuid = session.runSql("SELECT @@group_replication_view_change_uuid").fetchOne()[0];
cluster.disconnect();

//@ FR1-TS-01 Check persisted variables after create cluster {VER(>=8.0.12)}
print(get_persisted_gr_sysvars(__mysql_sandbox_port1));

//@<> Reset gr_start_on_boot on instance 1 {VER(>=8.0.12)}
disable_auto_rejoin(__mysql_sandbox_port1);

//@ FR1-TS-01 reboot instance {VER(>=8.0.12)}
session.close();
testutil.stopSandbox(__mysql_sandbox_port1);
testutil.startSandbox(__mysql_sandbox_port1);
shell.connect(__sandbox_uri1);

//@ FR1-TS-01 reboot cluster {VER(>=8.0.12)}
cluster = dba.rebootClusterFromCompleteOutage("C");

var persisted_sysvars = get_persisted_gr_sysvars(__mysql_sandbox_port1);
//@ FR1-TS-01 check persisted variables {VER(>=8.0.12)}
print(persisted_sysvars + "\n");
cluster.status();

//@ FR1-TS-01 TEARDOWN {VER(>=8.0.12)}
cluster.disconnect();
session.close();
testutil.destroySandbox(__mysql_sandbox_port1);

//@ FR1-TS-03 SETUP {VER(>=8.0.12)}
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: hostname});
testutil.snapshotSandboxConf(__mysql_sandbox_port1);
testutil.stopSandbox(__mysql_sandbox_port1);
testutil.changeSandboxConf(__mysql_sandbox_port1, "persisted-globals-load",
    "OFF");
testutil.startSandbox(__mysql_sandbox_port1);
shell.connect(__sandbox_uri1);

//@ FR1-TS-03 {VER(>=8.0.12)}
WIPE_SHELL_LOG();
// there should be a warning message asking to use configureInstance locally since persisted_globals_load is OFF
cluster = dba.createCluster("C", {groupName: "ca94447b-e6fc-11e7-b69d-4485005154dc", gtidSetIsComplete: true});
__gr_view_change_uuid = session.runSql("SELECT @@group_replication_view_change_uuid").fetchOne()[0];

//@ FR1-TS-03 TEARDOWN {VER(>=8.0.12)}
cluster.disconnect();
session.close();
testutil.destroySandbox(__mysql_sandbox_port1);

//@ FR1-TS-04/05 SETUP {VER(>=8.0.12)}
testutil.deployRawSandbox(__mysql_sandbox_port1, 'root', {report_host: hostname});
testutil.deployRawSandbox(__mysql_sandbox_port2, 'root', {report_host: hostname});

// Remove 'root'@'%' user to allow configureInstance() to create it.
shell.connect(__sandbox_uri1);
session.runSql("SET sql_log_bin = 0");
session.runSql("DROP USER IF EXISTS 'root'@'%'");
session.runSql("SET sql_log_bin = 1");
session.close();

//answer to create a new root@% account
testutil.expectPrompt("Please select an option [1]: ", "1");
testutil.expectPrompt("Account Host:", "%");
//accept configuration changes
testutil.expectPrompt("Do you want to perform the required configuration changes? [y/n]:", "Y");
testutil.expectPrompt("Do you want to restart the instance after configuring it?", "n");
dba.configureInstance(__sandbox_uri1);
testutil.stopSandbox(__mysql_sandbox_port1);
testutil.startSandbox(__mysql_sandbox_port1);
testutil.snapshotSandboxConf(__mysql_sandbox_port1);
shell.connect(__hostname_uri1);

var __local_address_1 = (__mysql_sandbox_port2 * 10 + 1).toString();

// due to the usage of ports, we must disable connectivity checks, otherwise the command would fail
shell.options["dba.connectivityChecks"] = false;

if (__version_num < 80027) {
    cluster = dba.createCluster("ClusterName", {localAddress: "localhost:" + __local_address_1, groupName: "62d73bbd-b830-11e7-a7b7-34e6d72fbd80", ipAllowlist:"255.255.255.255/32,127.0.0.1," + hostname_ip + "," + hostname, gtidSetIsComplete: true});
} else {
    cluster = dba.createCluster("ClusterName", {localAddress: "localhost:" + __local_address_1, groupName: "62d73bbd-b830-11e7-a7b7-34e6d72fbd80", ipAllowlist:"255.255.255.255/32,127.0.0.1," + hostname_ip + "," + hostname, gtidSetIsComplete: true, communicationStack: "XCOM"});
}
__gr_view_change_uuid = session.runSql("SELECT @@group_replication_view_change_uuid").fetchOne()[0];

var persisted_sysvars = get_persisted_gr_sysvars(__mysql_sandbox_port1);

//@ FR1-TS-04/05 {VER(>=8.0.12)}
print(persisted_sysvars);
print("\n\n");
dba.configureInstance(__sandbox_uri1);

shell.options["dba.connectivityChecks"] = true;

//@ FR1-TS-04/05 TEARDOWN {VER(>=8.0.12)}
cluster.disconnect();
session.close();
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);

//@ FR1-TS-06 SETUP {VER(<8.0.12)}
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: hostname});
testutil.snapshotSandboxConf(__mysql_sandbox_port1);
shell.connect(__sandbox_uri1);

//@ FR1-TS-06 {VER(<8.0.12)}
WIPE_SHELL_LOG();
cluster = dba.createCluster("ClusterName", {groupName: "ca94447b-e6fc-11e7-b69d-4485005154dc", gtidSetIsComplete: true});
EXPECT_SHELL_LOG_CONTAINS(`WARNING: Instance '${hostname}:${__mysql_sandbox_port1}' will not load the persisted cluster configuration upon reboot since 'persisted-globals-load' is set to 'OFF'. Please set 'persisted-globals-load' to 'ON' on the configuration file.`);

//@ FR1-TS-06 TEARDOWN {VER(<8.0.12)}
cluster.disconnect();
session.close();
testutil.destroySandbox(__mysql_sandbox_port1);

//@ FR1-TS-7 SETUP {VER(>=8.0.12)}
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: hostname});
shell.connect(__sandbox_uri1);
dba.createCluster("ClusterName",  {multiPrimary: true, force: true, groupName: "ca94447b-e6fc-11e7-b69d-4485005154dc", gtidSetIsComplete: true});
__gr_view_change_uuid = session.runSql("SELECT @@group_replication_view_change_uuid").fetchOne()[0];

var persisted_sysvars = get_persisted_gr_sysvars(__mysql_sandbox_port1);
//@ FR1-TS-7 show persisted cluster variables {VER(>=8.0.12)}
print(persisted_sysvars);

//@<> Reset gr_start_on_boot on instance 1 again {VER(>=8.0.12)}
disable_auto_rejoin(__mysql_sandbox_port1);

//@ FR1-TS-7 reboot instance 1 {VER(>=8.0.12)}
session.close();
testutil.stopSandbox(__mysql_sandbox_port1);
testutil.startSandbox(__mysql_sandbox_port1);
shell.connect(__sandbox_uri1);

//@ FR1-TS-7 reboot cluster {VER(>=8.0.12)}
cluster = dba.rebootClusterFromCompleteOutage("ClusterName");

var persisted_sysvars = get_persisted_gr_sysvars(__mysql_sandbox_port1);
//@ FR1-TS-7 check persisted variables {VER(>=8.0.12)}
print(persisted_sysvars + "\n");
cluster.status();

//@ FR1-TS-7 TEARDOWN {VER(>=8.0.12)}
cluster.disconnect();
session.close();
testutil.destroySandbox(__mysql_sandbox_port1);

//FR2 -On a successful .addInstance() call, the group replication sysvars must
// be updated and persisted at the joining instance.
//@FR2-TS-1 SETUP {VER(>=8.0.12)}
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: hostname});
testutil.deploySandbox(__mysql_sandbox_port2, "root", {report_host: hostname});

var s2 = mysql.getSession(__sandbox_uri2);
shell.connect(__sandbox_uri1);
dba.createCluster("ClusterName", {groupName: "ca94447b-e6fc-11e7-b69d-4485005154dc", gtidSetIsComplete: true});
__gr_view_change_uuid = session.runSql("SELECT @@group_replication_view_change_uuid").fetchOne()[0];
cluster = dba.getCluster("ClusterName");
cluster.addInstance(__sandbox_uri2);
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");

var persisted_sysvars = get_persisted_gr_sysvars(s2);
//@ FR2-TS-1 check persisted variables on instance 1 {VER(>=8.0.12)}
print(persisted_sysvars);

//@ FR2-TS-1 stop instance 2 {VER(>=8.0.12)}
testutil.stopSandbox(__mysql_sandbox_port2);
testutil.waitMemberState(__mysql_sandbox_port2, "(MISSING)");

//@ FR2-TS-1 cluster status showing instance 2 is missing {VER(>=8.0.12)}
cluster.status();

//@ FR2-TS-1 start instance 1 {VER(>=8.0.12)}
testutil.startSandbox(__mysql_sandbox_port2);
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");

//@ FR2-TS-1 cluster status showing instance 2 is back online {VER(>=8.0.12)}
cluster.status();

//@ FR2-TS-1 TEARDOWN {VER(>=8.0.12)}
cluster.disconnect();
s2.close();
session.close();
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);

//@ FR2-TS-3 SETUP {VER(>=8.0.12)}
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: hostname});
testutil.deploySandbox(__mysql_sandbox_port2, "root", {report_host: hostname});
testutil.snapshotSandboxConf(__mysql_sandbox_port2);
testutil.stopSandbox(__mysql_sandbox_port2);
testutil.changeSandboxConf(__mysql_sandbox_port2, "persisted-globals-load",
    "OFF");
testutil.startSandbox(__mysql_sandbox_port2);
shell.connect(__sandbox_uri1);
dba.createCluster("ClusterName", {groupName: "ca94447b-e6fc-11e7-b69d-4485005154dc", gtidSetIsComplete: true});
__gr_view_change_uuid = session.runSql("SELECT @@group_replication_view_change_uuid").fetchOne()[0];
cluster = dba.getCluster("ClusterName");

//@ FR2-TS-3 check that warning is displayed when adding instance with persisted-globals-load=OFF {VER(>=8.0.12)}
WIPE_SHELL_LOG();
cluster.addInstance(__sandbox_uri2);
cluster.status();

//@ FR2-TS-3 TEARDOWN {VER(>=8.0.12)}
session.close();
cluster.disconnect();
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);

//@ FR2-TS-4 SETUP {VER(>=8.0.12)}
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: hostname});
testutil.deploySandbox(__mysql_sandbox_port2, "root", {report_host: hostname});
var s1 = mysql.getSession(__sandbox_uri1);
s2 = mysql.getSession(__sandbox_uri2);
shell.connect(__sandbox_uri1);
if (__version_num < 80027) {
    dba.createCluster("ClusterName", {groupName: "ca94447b-e6fc-11e7-b69d-4485005154dc", ipAllowlist:"255.255.255.255/32,127.0.0.1," + hostname_ip + "," + hostname, gtidSetIsComplete: true});
} else {
    dba.createCluster("ClusterName", {groupName: "ca94447b-e6fc-11e7-b69d-4485005154dc", ipAllowlist:"255.255.255.255/32,127.0.0.1," + hostname_ip + "," + hostname, gtidSetIsComplete: true, communicationStack: "XCOM"});
}
__gr_view_change_uuid = session.runSql("SELECT @@group_replication_view_change_uuid").fetchOne()[0];

cluster = dba.getCluster("ClusterName");

// due to the usage of ports, we must disable connectivity checks, otherwise the command would fail
shell.options["dba.connectivityChecks"] = false;

var __local_address_2 = "15679";
cluster.addInstance(__sandbox_uri2, {localAddress: "localhost:" + __local_address_2, ipAllowlist:"255.255.255.255/32,127.0.0.1," + hostname_ip + "," + hostname});
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");

var persisted_sysvars1 = get_persisted_gr_sysvars(__mysql_sandbox_port1);
var persisted_sysvars2 = get_persisted_gr_sysvars(__mysql_sandbox_port2);

shell.options["dba.connectivityChecks"] = true;

//@ FR2-TS-4 Check that persisted variables match the ones passed on the arguments to create cluster and addInstance {VER(>=8.0.12)}
__mysql_sandbox_gr_port1_xcom = __mysql_sandbox_port1 * 10 + 1
print(persisted_sysvars1);
print("\n");
print(persisted_sysvars2);

//@ FR2-TS-4 TEARDOWN {VER(>=8.0.12)}
session.close();
cluster.disconnect();
s1.close();
s2.close();
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);

//@ FR2-TS-5 SETUP {VER(>=8.0.12)}
testutil.deployRawSandbox(__mysql_sandbox_port1, 'root', {report_host: hostname});

// Remove 'root'@'%' user to allow configureInstance() to create it.
shell.connect(__sandbox_uri1);
session.runSql("SET sql_log_bin = 0");
session.runSql("DROP USER IF EXISTS 'root'@'%'");
session.runSql("SET sql_log_bin = 1");
session.close();

//answer to create a new root@% account
testutil.expectPrompt("Please select an option [1]: ", "1");
testutil.expectPrompt("Account Host:", "%");
//accept configuration changes
testutil.expectPrompt("Do you want to perform the required configuration changes? [y/n]:", "Y");
testutil.expectPrompt("Do you want to restart the instance after configuring it?", "n");
dba.configureInstance(__sandbox_uri1);
testutil.stopSandbox(__mysql_sandbox_port1);
testutil.startSandbox(__mysql_sandbox_port1);

testutil.deployRawSandbox(__mysql_sandbox_port2, 'root', {report_host: hostname});
testutil.deployRawSandbox(__mysql_sandbox_port3, 'root', {report_host: hostname});

// Remove 'root'@'%' user to allow configureInstance() to create it.
shell.connect(__sandbox_uri2);
session.runSql("SET sql_log_bin = 0");
session.runSql("DROP USER IF EXISTS 'root'@'%'");
session.runSql("SET sql_log_bin = 1");
session.close();

//answer to create a new root@% account
testutil.expectPrompt("Please select an option [1]: ", "1");
testutil.expectPrompt("Account Host:", "%");
//accept configuration changes
testutil.expectPrompt("Do you want to perform the required configuration changes? [y/n]:", "Y");
testutil.expectPrompt("Do you want to restart the instance after configuring it?", "n");
dba.configureInstance(__sandbox_uri2);
testutil.stopSandbox(__mysql_sandbox_port2);
testutil.startSandbox(__mysql_sandbox_port2);
testutil.snapshotSandboxConf(__mysql_sandbox_port2);
s1 = mysql.getSession(__sandbox_uri1);
s2 = mysql.getSession(__sandbox_uri2);
shell.connect(__hostname_uri1);
if (__version_num < 80027) {
    dba.createCluster("ClusterName", {groupName: "ca94447b-e6fc-11e7-b69d-4485005154dc", gtidSetIsComplete: true});
} else {
    dba.createCluster("ClusterName", {groupName: "ca94447b-e6fc-11e7-b69d-4485005154dc", gtidSetIsComplete: true, communicationStack: "XCOM"});
}
__gr_view_change_uuid = session.runSql("SELECT @@group_replication_view_change_uuid").fetchOne()[0];

cluster = dba.getCluster("ClusterName");

// due to the usage of ports, we must disable connectivity checks, otherwise the command would fail
shell.options["dba.connectivityChecks"] = false;

var __local_address_3 = (__mysql_sandbox_port3 * 10 + 1).toString();
cluster.addInstance(__hostname_uri2, {localAddress: "localhost:" + __local_address_3, ipAllowlist:"255.255.255.255/32,127.0.0.1," + hostname_ip + "," + hostname});

session.close();
shell.connect(__hostname_uri2);

testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");

var persisted_sysvars1 = get_persisted_gr_sysvars(__mysql_sandbox_port1);
var persisted_sysvars2 = get_persisted_gr_sysvars(__mysql_sandbox_port2);

shell.options["dba.connectivityChecks"] = true;

//@ FR2-TS-5 {VER(>=8.0.12)}
print(persisted_sysvars1);
print("\n");

print(persisted_sysvars2);
print("\n\n");

dba.configureInstance(__sandbox_uri2);

//@ FR2-TS-5 TEARDOWN {VER(>=8.0.12)}
session.close();
cluster.disconnect();
s1.close();
s2.close();
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
testutil.destroySandbox(__mysql_sandbox_port3);

//@ FR2-TS-6 SETUP {VER(<8.0.12)}
WIPE_SHELL_LOG();

testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: hostname});
testutil.snapshotSandboxConf(__mysql_sandbox_port1);
testutil.deploySandbox(__mysql_sandbox_port2, "root", {report_host: hostname});
testutil.snapshotSandboxConf(__mysql_sandbox_port2);
shell.connect(__sandbox_uri1);

// Create a test dataset so that RECOVERY takes a while and we ensure right monitoring messages for addInstance
session.runSql("SET GLOBAL super_read_only = 0");
session.runSql("create schema test");
session.runSql("create table test.data (a int primary key auto_increment, data longtext)");
for (i = 0; i < 20; i++) {
    session.runSql("insert into test.data values (default, repeat('x', 4*1024*1024))");
}

dba.createCluster("ClusterName", {groupName: "ca94447b-e6fc-11e7-b69d-4485005154dc", gtidSetIsComplete: true});
cluster = dba.getCluster("ClusterName");

//@ FR2-TS-6 Warning is displayed on addInstance {VER(<8.0.12)}
// A warning about server version not supporting set persist syntax and asking
// to use ConfigureLocalInstance should be displaying in both the seed instance
// and the added instance.
var expected_msg1 = "Warning: Instance '" + hostname + ":" + __mysql_sandbox_port1 + "' cannot persist Group Replication configuration since MySQL version " + __version + " does not support the SET PERSIST command (MySQL version >= 8.0.11 required). Please use the dba.configureInstance() command locally, using the 'mycnfPath' option, to persist the changes.";
var expected_msg2 = "Warning: Instance '" + hostname + ":" + __mysql_sandbox_port2 + "' cannot persist Group Replication configuration since MySQL version " + __version + " does not support the SET PERSIST command (MySQL version >= 8.0.11 required). Please use the dba.configureInstance() command locally, using the 'mycnfPath' option, to persist the changes.";
cluster.addInstance(__sandbox_uri2);
EXPECT_SHELL_LOG_CONTAINS(expected_msg1);
EXPECT_SHELL_LOG_CONTAINS(expected_msg2);

//@ FR2-TS-6 TEARDOWN {VER(<8.0.12)}
session.close();
cluster.disconnect();
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);

//@ FR2-TS-8 SETUP {VER(>=8.0.12)}
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: hostname});
testutil.snapshotSandboxConf(__mysql_sandbox_port1);
testutil.deploySandbox(__mysql_sandbox_port2, "root", {report_host: hostname});
testutil.snapshotSandboxConf(__mysql_sandbox_port2);
s1 = mysql.getSession(__sandbox_uri1);
s2 = mysql.getSession(__sandbox_uri2);
shell.connect(__sandbox_uri1);
dba.createCluster("ClusterName", {multiPrimary: true, force: true, groupName: "ca94447b-e6fc-11e7-b69d-4485005154dc", gtidSetIsComplete: true});
__gr_view_change_uuid = session.runSql("SELECT @@group_replication_view_change_uuid").fetchOne()[0];
cluster = dba.getCluster("ClusterName");
cluster.addInstance(__sandbox_uri2);
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");
session.close();

var persisted_sysvars1 = get_persisted_gr_sysvars(__mysql_sandbox_port1);
var persisted_sysvars2 = get_persisted_gr_sysvars(__mysql_sandbox_port2);
shell.connect(__sandbox_uri1);

//@ FR2-TS-8 Check that correct values were persisted and that instance rejoins automatically {VER(>=8.0.12)}
print(persisted_sysvars1);
print("\n");
print(persisted_sysvars2);
print("\n");
testutil.stopSandbox(__mysql_sandbox_port2);
testutil.waitMemberState(__mysql_sandbox_port2, "(MISSING)");
cluster.status();
testutil.startSandbox(__mysql_sandbox_port2);
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");
cluster.status();

//@ FR2-TS-8 TEARDOWN {VER(>=8.0.12)}
cluster.disconnect();
session.close();
s1.close();
s2.close();
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);

//@ FR2-TS-9 SETUP {VER(>=8.0.12)}
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: hostname});
testutil.deploySandbox(__mysql_sandbox_port2, "root", {report_host: hostname});
testutil.deploySandbox(__mysql_sandbox_port3, "root", {report_host: hostname});
s1 = mysql.getSession(__sandbox_uri1);
s2 = mysql.getSession(__sandbox_uri2);
var s3 = mysql.getSession(__sandbox_uri3);
shell.connect(__sandbox_uri1);
dba.createCluster("ClusterName", {groupName: "ca94447b-e6fc-11e7-b69d-4485005154dc", gtidSetIsComplete: true});
__gr_view_change_uuid = session.runSql("SELECT @@group_replication_view_change_uuid").fetchOne()[0];
cluster = dba.getCluster("ClusterName");
cluster.addInstance(__sandbox_uri2);
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");
session.close();

var persisted_sysvars = get_persisted_gr_sysvars(__mysql_sandbox_port2);
//@ FR2-TS-9 Check that correct values were persisted on instance 2 {VER(>=8.0.12)}
print(persisted_sysvars);

//@FR2-TS-9 Add instance 3 and wait for it to be online {VER(>=8.0.12)}
shell.connect(__sandbox_uri1);
cluster.addInstance(__sandbox_uri3);
testutil.waitMemberState(__mysql_sandbox_port3, "ONLINE");
session.close();

var persisted_sysvars3 = get_persisted_gr_sysvars(__mysql_sandbox_port3);
var persisted_sysvars2 = get_persisted_gr_sysvars(__mysql_sandbox_port2);
var persisted_sysvars1 = get_persisted_gr_sysvars(__mysql_sandbox_port1);
shell.connect(__sandbox_uri1);

//@ FR2-TS-9 Check that correct values are persisted and updated when instances are added and that instances rejoin automatically {VER(>=8.0.12)}
print(persisted_sysvars3);
print("\n");
print(persisted_sysvars2);
print("\n");
print(persisted_sysvars1);
print("\n");

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

//@ FR2-TS-9 TEARDOWN {VER(>=8.0.12)}
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
//@ FR5-TS-1 SETUP {VER(>=8.0.12)}
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: hostname});
testutil.deploySandbox(__mysql_sandbox_port2, "root", {report_host: hostname});
s1 = mysql.getSession(__sandbox_uri1);
s2 = mysql.getSession(__sandbox_uri2);
shell.connect(__sandbox_uri1);
dba.createCluster("ClusterName", {groupName: "ca94447b-e6fc-11e7-b69d-4485005154dc", gtidSetIsComplete: true});
__gr_view_change_uuid = session.runSql("SELECT @@group_replication_view_change_uuid").fetchOne()[0];
cluster = dba.getCluster("ClusterName");
cluster.addInstance(__sandbox_uri2);
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");
cluster.removeInstance(__sandbox_uri2);

var persisted_sysvars2 = get_persisted_gr_sysvars(__mysql_sandbox_port2);
var persisted_sysvars1 = get_persisted_gr_sysvars(__mysql_sandbox_port1);

//@ FR5-TS-1 Check that persisted variables are updated/reset after removeCluster operation {VER(>=8.0.12)}
print(persisted_sysvars2);
print("\n");
print(persisted_sysvars1);

//@ FR5-TS-1 TEARDOWN {VER(>=8.0.12)}
cluster.disconnect();
session.close();
s1.close();
s2.close();
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);

//@ FR5-TS-4 SETUP {VER(>=8.0.12)}
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: hostname});
testutil.deploySandbox(__mysql_sandbox_port2, "root", {report_host: hostname});
testutil.deploySandbox(__mysql_sandbox_port3, "root", {report_host: hostname});
s1 = mysql.getSession(__sandbox_uri1);
s2 = mysql.getSession(__sandbox_uri2);
s3 = mysql.getSession(__sandbox_uri3);
shell.connect(__sandbox_uri1);
dba.createCluster("ClusterName", {groupName: "ca94447b-e6fc-11e7-b69d-4485005154dc", gtidSetIsComplete: true});
__gr_view_change_uuid = session.runSql("SELECT @@group_replication_view_change_uuid").fetchOne()[0];
cluster = dba.getCluster("ClusterName");
cluster.addInstance(__sandbox_uri2);
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");
cluster.addInstance(__sandbox_uri3);
testutil.waitMemberState(__mysql_sandbox_port3, "ONLINE");

var persisted_sysvars3 = get_persisted_gr_sysvars(__mysql_sandbox_port3);

//@ FR5-TS-4 Check that persisted variables are updated/reset after removeCluster operation - before {VER(>=8.0.12)}
print(persisted_sysvars3);
print("\n");
cluster.removeInstance(__sandbox_uri2);

//@<> Get the persisted vars again {VER(>=8.0.12)}
var persisted_sysvars3 = get_persisted_gr_sysvars(__mysql_sandbox_port3);
var persisted_sysvars2 = get_persisted_gr_sysvars(__mysql_sandbox_port2);
var persisted_sysvars1 = get_persisted_gr_sysvars(__mysql_sandbox_port1);

//@ FR5-TS-4 Check that persisted variables are updated/reset after removeCluster operation - after {VER(>=8.0.12)}
print(persisted_sysvars3);
print("\n");
print(persisted_sysvars2);
print("\n");
print(persisted_sysvars1);

//@ FR5-TS-4 TEARDOWN {VER(>=8.0.12)}
cluster.disconnect();
session.close();
s1.close();
s2.close();
s3.close();
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
testutil.destroySandbox(__mysql_sandbox_port3);

//@ FR5-Extra SETUP {VER(<8.0.12)}
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: hostname});
testutil.deploySandbox(__mysql_sandbox_port2, "root", {report_host: hostname});
testutil.deploySandbox(__mysql_sandbox_port3, "root", {report_host: hostname});
shell.connect(__sandbox_uri1);
dba.createCluster("ClusterName", {groupName: "ca94447b-e6fc-11e7-b69d-4485005154dc", gtidSetIsComplete: true});
cluster = dba.getCluster("ClusterName");
cluster.addInstance(__sandbox_uri2);
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");
cluster.addInstance(__sandbox_uri3);
testutil.waitMemberState(__mysql_sandbox_port3, "ONLINE");

//@ FR5-Extra Check that warning is shown when removeInstance is called {VER(<8.0.12)}
WIPE_SHELL_LOG();
cluster.removeInstance(__sandbox_uri2);
// there should be a warning message for each of the members staying in the group
var expected_msg1 = "Warning: Instance '" + hostname + ":" + __mysql_sandbox_port1 + "' cannot persist configuration since MySQL version " + __version + " does not support the SET PERSIST command (MySQL version >= 8.0.11 required). Please use the dba.configureInstance() command locally, using the 'mycnfPath' option, to persist the changes.";
var expected_msg2 = "Warning: Instance '" + hostname + ":" + __mysql_sandbox_port3 + "' cannot persist configuration since MySQL version " + __version + " does not support the SET PERSIST command (MySQL version >= 8.0.11 required). Please use the dba.configureInstance() command locally, using the 'mycnfPath' option, to persist the changes.";
// and a warning for the member that was removed
var expected_msg3 = "Warning: On instance 'localhost:" + __mysql_sandbox_port2 + "' configuration cannot be persisted since MySQL version " + __version + " does not support the SET PERSIST command (MySQL version >= 8.0.11 required). Please set the 'group_replication_start_on_boot' variable to 'OFF' in the server configuration file, otherwise it might rejoin the cluster upon restart.";
EXPECT_SHELL_LOG_CONTAINS(expected_msg1);
EXPECT_SHELL_LOG_CONTAINS(expected_msg2);
EXPECT_SHELL_LOG_CONTAINS(expected_msg3);

cluster.status();

//@ FR5-Extra TEARDOWN {VER(<8.0.12)}
cluster.disconnect();
session.close();
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
testutil.destroySandbox(__mysql_sandbox_port3);

//@ Check if Cluster dissolve will reset persisted variables SETUP {VER(>=8.0.12)}
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: hostname});
shell.connect(__sandbox_uri1);
dba.createCluster("ClusterName", {groupName: "ca94447b-e6fc-11e7-b69d-4485005154dc", gtidSetIsComplete: true});
__gr_view_change_uuid = session.runSql("SELECT @@group_replication_view_change_uuid").fetchOne()[0];
cluster = dba.getCluster("ClusterName");
testutil.expectPrompt("Are you sure you want to dissolve the cluster?", "y");
cluster.dissolve({force:true});

var persisted_sysvars = get_persisted_gr_sysvars(__mysql_sandbox_port1);
//@ Check if Cluster dissolve will reset persisted variables {VER(>=8.0.12)}
print(persisted_sysvars);

//@ Check if Cluster dissolve will reset persisted variables TEARDOWN {VER(>=8.0.12)}
session.close();
testutil.destroySandbox(__mysql_sandbox_port1);
