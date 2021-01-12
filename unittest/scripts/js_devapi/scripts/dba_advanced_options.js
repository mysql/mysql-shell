// Assumptions: smart deployment functions available

//@ Initialization
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: hostname});
testutil.snapshotSandboxConf(__mysql_sandbox_port1);
testutil.deploySandbox(__mysql_sandbox_port2, "root", {report_host: hostname});
testutil.snapshotSandboxConf(__mysql_sandbox_port2);
testutil.deploySandbox(__mysql_sandbox_port3, "root", {report_host: hostname});
testutil.snapshotSandboxConf(__mysql_sandbox_port3);

shell.connect({scheme: "mysql", host: localhost, port: __mysql_sandbox_port1, user: 'root', password: 'root'});

//BUG#27758041: SHELL PICKS RANDOM PORTS IF GR LOCAL ADDRESS PORT IS BUSY
//@ Create cluster fails because port default GR local address port is already in use. {!__replaying && !__recording}
var __busy_port = __mysql_sandbox_port1 * 10 + 1;
var __valid_portx = (__mysql_sandbox_port1 * 10 + 9).toString();
testutil.deploySandbox(__busy_port, "root", {"loose_mysqlx_port": __valid_portx, report_host: hostname});
var cluster = dba.createCluster('test');
// Free "busy" port before continuing with the test (otherwise it will fail).
testutil.destroySandbox(__busy_port);

//@ Create cluster errors using localAddress option
// FR1-TS-1-5 (GR issues an error if the hostname or IP address is invalid)
var c = dba.createCluster('test', {localAddress: "1a"});
// FR1-TS-1-6
var c = dba.createCluster('test', {localAddress: ":"});
// FR1-TS-1-7
var c = dba.createCluster('test', {localAddress: ""});
// FR1-TS-1-8
var c = dba.createCluster('test', {localAddress: ":123456"});

//@ Create cluster errors using localAddress option on busy port {!__replaying && !__recording}
// FR1-TS-1-1
// Note: this test will try to connect to the port in localAddress to see if
// its free. Since report_host is now used (instead of localhost) internally
// this test cannot run in replay mode.
var __local_address_1 = __host + ":" + __mysql_port;
var c = dba.createCluster('test', {localAddress: __local_address_1});

shell.connect(__sandbox_uri1);

//@ Create cluster errors using groupSeeds option
// FR2-TS-1-3
var c = dba.createCluster('test', {groupSeeds: ""});
// FR2-TS-1-4
var c = dba.createCluster('test', {groupSeeds: "abc"});
// Clear invalid group seed value (to avoid further GR errors for next tests).
session.runSql('SET @@GLOBAL.group_replication_group_seeds = ""');

//@<> Create cluster errors using groupName option
// FR3-TS-1-2
EXPECT_THROWS_TYPE(function() { dba.createCluster('test', {groupName: ""}); }, "Invalid value for groupName, string value cannot be empty", "ArgumentError");

// FR3-TS-1-3
if (__version_num >= 80021) {
  EXPECT_THROWS_TYPE(function() { dba.createCluster('test', {groupName: "abc"}); }, "Dba.createCluster: Unable to set value 'abc' for 'groupName': The group_replication_group_name is not a valid UUID", "RuntimeError");
} else {
  EXPECT_THROWS_TYPE(function() { dba.createCluster('test', {groupName: "abc"}); }, "Dba.createCluster: Unable to set value 'abc' for 'groupName': The group name is not a valid UUID", "RuntimeError");
}

//@ Create cluster specifying :<valid_port> for localAddress (FR1-TS-1-2)
var valid_port = __mysql_sandbox_port1 + 20000;
var __local_address_2 = ":" + valid_port;
var __result_local_address_2 = hostname + __local_address_2;
var c = dba.createCluster('test', {clearReadOnly: true, localAddress: __local_address_2});

//@<> Confirm local address is set correctly (FR1-TS-1-2)
EXPECT_EQ(__result_local_address_2, get_sysvar(session, "group_replication_local_address"));

//@ Dissolve cluster (FR1-TS-1-2)
c.dissolve({force: true});

//@ Create cluster specifying <valid_host>: for localAddress (FR1-TS-1-3)
var __local_address_3 = localhost + ":";
var result_port = __mysql_sandbox_port1 * 10 + 1;
var __result_local_address_3 = __local_address_3 + result_port;
var c = dba.createCluster('test', {clearReadOnly: true, localAddress: __local_address_3});

//@<> Confirm local address is set correctly (FR1-TS-1-3)
EXPECT_EQ(__result_local_address_3, get_sysvar(session, "group_replication_local_address"));

//@ Dissolve cluster (FR1-TS-1-3)
c.dissolve({force: true});

//@ Create cluster specifying <valid_port> for localAddress (FR1-TS-1-4)
var __local_address_4 = (__mysql_sandbox_port2 * 10 + 1).toString();
var __result_local_address_4 = hostname + ":" + __local_address_4;
var c = dba.createCluster('test', {clearReadOnly: true, localAddress: __local_address_4});

//@<> Confirm local address is set correctly (FR1-TS-1-4)
EXPECT_EQ(__result_local_address_4, get_sysvar(session, "group_replication_local_address"));

//@ Dissolve cluster (FR1-TS-1-4)
c.dissolve({force: true});

//@ Create cluster specifying <valid_host> for localAddress (FR1-TS-1-9)
var __local_address_9 = localhost;
var __result_local_address_9 = __local_address_9 + ":" + result_port;
var c = dba.createCluster('test', {clearReadOnly: true, localAddress: __local_address_9});

//@<> Confirm local address is set correctly (FR1-TS-1-9)
EXPECT_EQ(__result_local_address_9, get_sysvar(session, "group_replication_local_address"));

//@ Dissolve cluster (FR1-TS-1-9)
c.dissolve({force: true});

//@ Create cluster specifying <valid_host>:<valid_port> for localAddress (FR1-TS-1-10)
var __local_address_port_10 = (__mysql_sandbox_port2 * 10 + 1).toString();
var __local_address_10 = localhost + ":" + __local_address_port_10;
var __result_local_address_10 = __local_address_10;
var c = dba.createCluster('test', {clearReadOnly: true, localAddress: __local_address_10});

//@<> Confirm local address is set correctly (FR1-TS-1-10)
EXPECT_EQ(__result_local_address_10, get_sysvar(session, "group_replication_local_address"));

//@ Dissolve cluster (FR1-TS-1-10)
c.dissolve({force: true});

//@ Create cluster specifying 127.0.0.1:<valid_port> for groupSeeds (FR2-TS-1-1)
var default_valid_port1 = __mysql_sandbox_port1 * 10 + 1;
var __group_seeds_1 = "127.0.0.1:" + default_valid_port1;
var __result_group_seeds_1 = __group_seeds_1;
var c = dba.createCluster('test', {clearReadOnly: true, groupSeeds: __group_seeds_1});

//@<> Confirm group seeds is set correctly (FR2-TS-1-1)
EXPECT_EQ(__result_group_seeds_1, get_sysvar(session, "group_replication_group_seeds"));

//@ Dissolve cluster (FR2-TS-1-1)
c.dissolve({force: true});

//@ Create cluster specifying 127.0.0.1:<valid_port>,127.0.0.1:<valid_port2> for groupSeeds (FR2-TS-1-2)
var default_valid_port1 = __mysql_sandbox_port1 * 10 + 1;
var default_valid_port2 = __mysql_sandbox_port2 * 10 + 1;
var __group_seeds_2 = "127.0.0.1:" + default_valid_port1 + ",127.0.0.1:" + default_valid_port2;
var __result_group_seeds_2 = __group_seeds_2;
var c = dba.createCluster('test', {clearReadOnly: true, groupSeeds: __group_seeds_2});

//@<> Confirm group seeds is set correctly (FR2-TS-1-2)
EXPECT_EQ(__result_group_seeds_2, get_sysvar(session, "group_replication_group_seeds"));

//@ Dissolve cluster (FR2-TS-1-2)
c.dissolve({force: true});

//@ Create cluster specifying aaaaaaaa-aaaa-aaaa-aaaa-aaaaaaaaaaaa for groupName (FR3-TS-1-1)
var __group_name_1 = "aaaaaaaa-aaaa-aaaa-aaaa-aaaaaaaaaaaa";
var __result_group_name_1 = __group_name_1;
var c = dba.createCluster('test', {clearReadOnly: true, groupName: __group_name_1});

//@<> Confirm group name is set correctly (FR3-TS-1-1)
EXPECT_EQ(__result_group_name_1, get_sysvar(session, "group_replication_group_name"));

//@ Dissolve cluster (FR3-TS-1-1)
c.dissolve({force: true});

//@ Create cluster
var c = dba.createCluster('test', {clearReadOnly: true, gtidSetIsComplete: true});

//@ Add instance errors using localAddress option
// FR1-TS-2-5 (GR issues an error if the hostname or IP address is invalid)
add_instance_options['port'] = __mysql_sandbox_port2;
add_instance_options['user'] = 'root';
c.addInstance(add_instance_options, {localAddress: "1a"});
// FR1-TS-2-6
c.addInstance(add_instance_options, {localAddress: ":"});
// FR1-TS-2-7
c.addInstance(add_instance_options, {localAddress: ""});
// FR1-TS-2-8
c.addInstance(add_instance_options, {localAddress: ":123456"});

//@ Add instance errors using localAddress option on busy port {!__replaying && !__recording}
// Note: Since report_host is now used (instead of localhost) internally
// this test cannot run in replay mode.
// FR1-TS-2-1
print(__local_address_1+"\n");
c.addInstance(add_instance_options, {localAddress: __local_address_1});

//@ Add instance errors using groupSeeds option
// FR2-TS-2-3
add_instance_options['port'] = __mysql_sandbox_port2;
c.addInstance(add_instance_options, {groupSeeds: ""});
// FR2-TS-2-4
c.addInstance(add_instance_options, {groupSeeds: "abc"});

//@ Add instance error using groupName (not a valid option)
c.addInstance(add_instance_options, {groupName: "aaaaaaaa-aaaa-aaaa-aaaa-aaaaaaaaaaaa"});

//@ Add instance specifying :<valid_port> for localAddress (FR1-TS-2-2)
var valid_port2 = __mysql_sandbox_port2 + 20000;
var __local_address_add_2 = ":" + valid_port2;
var __result_local_address_add_2 = hostname + __local_address_add_2;
c.addInstance(add_instance_options, {localAddress: __local_address_add_2});
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");

//@<> Confirm local address is set correctly (FR1-TS-2-2)
session.close();
EXPECT_EQ(__result_local_address_add_2, get_sysvar(__mysql_sandbox_port2, "group_replication_local_address"));

c.disconnect();

//@ Remove instance (FR1-TS-2-2)
shell.connect({scheme: "mysql", host: localhost, port: __mysql_sandbox_port1, user: 'root', password: 'root'});
var c = dba.getCluster();
c.removeInstance(add_instance_options);

//@ Add instance specifying <valid_host>: for localAddress (FR1-TS-2-3)
var __local_address_add_3 = localhost + ":";
var result_port2 = __mysql_sandbox_port2 * 10 + 1;
var __result_local_address_add_3 = __local_address_add_3 + result_port2;
c.addInstance(add_instance_options, {localAddress: __local_address_add_3});
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");

//@<> Confirm local address is set correctly (FR1-TS-2-3)
session.close();
EXPECT_EQ(__result_local_address_add_3, get_sysvar(__mysql_sandbox_port2, "group_replication_local_address"));

c.disconnect();

//@ Remove instance (FR1-TS-2-3)
shell.connect({scheme: "mysql", host: localhost, port: __mysql_sandbox_port1, user: 'root', password: 'root'});
var c = dba.getCluster();
c.removeInstance(add_instance_options);

//@ Add instance specifying <valid_port> for localAddress (FR1-TS-2-4)
var __local_address_add_4 = (__mysql_sandbox_port3 * 10 + 1).toString();
var __result_local_address_add_4 = hostname + ":" + __local_address_add_4;
c.addInstance(add_instance_options, {localAddress: __local_address_add_4});
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");

//@<> Confirm local address is set correctly (FR1-TS-2-4)
session.close();
EXPECT_EQ(__result_local_address_add_4, get_sysvar(__mysql_sandbox_port2, "group_replication_local_address"));

c.disconnect();

//@ Remove instance (FR1-TS-2-4)
shell.connect({scheme: "mysql", host: localhost, port: __mysql_sandbox_port1, user: 'root', password: 'root'});
var c = dba.getCluster();
c.removeInstance(add_instance_options);

//@ Add instance specifying <valid_host> for localAddress (FR1-TS-2-9)
var __local_address_add_9 = localhost;
var __result_local_address_add_9 = __local_address_add_9 + ":" + result_port2;
c.addInstance(add_instance_options, {localAddress: __local_address_add_9});
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");

//@<> Confirm local address is set correctly (FR1-TS-2-9)
session.close();
EXPECT_EQ(__result_local_address_add_9, get_sysvar(__mysql_sandbox_port2, "group_replication_local_address"));

c.disconnect();

//@ Remove instance (FR1-TS-2-9)
shell.connect({scheme: "mysql", host: localhost, port: __mysql_sandbox_port1, user: 'root', password: 'root'});
var c = dba.getCluster();
c.removeInstance(add_instance_options);

//@ Add instance specifying <valid_host>:<valid_port> for localAddress (FR1-TS-2-10)
var __local_address_add_10 = localhost + ":" + (__mysql_sandbox_port3 * 10 + 1).toString();
var __result_local_address_add_10 = __local_address_add_10;
c.addInstance(add_instance_options, {localAddress: __local_address_add_10});
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");

//@<> Confirm local address is set correctly (FR1-TS-2-10)
session.close();
EXPECT_EQ(__result_local_address_add_10, get_sysvar(__mysql_sandbox_port2, "group_replication_local_address"));

c.disconnect();

//@ Remove instance (FR1-TS-2-10)
shell.connect({scheme: "mysql", host: localhost, port: __mysql_sandbox_port1, user: 'root', password: 'root'});
var c = dba.getCluster();
c.removeInstance(add_instance_options);

//@ Add instance specifying 127.0.0.1:<valid_port> for groupSeeds (FR2-TS-2-1)
c.addInstance(add_instance_options, {groupSeeds: __group_seeds_1});
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");

//@<> Confirm group seeds is set correctly (FR2-TS-2-1)
session.close();
EXPECT_EQ(__result_group_seeds_1, get_sysvar(__mysql_sandbox_port2, "group_replication_group_seeds"));

c.disconnect();

//@ Remove instance (FR2-TS-2-1)
shell.connect({scheme: "mysql", host: localhost, port: __mysql_sandbox_port1, user: 'root', password: 'root'});
var c = dba.getCluster();
c.removeInstance(add_instance_options);

//@ Add instance specifying 127.0.0.1:<valid_port>,127.0.0.1:<valid_port2> for groupSeeds (FR2-TS-2-2)
c.addInstance(add_instance_options, {groupSeeds: __group_seeds_2});
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");

//@<> Confirm group seeds is set correctly (FR2-TS-2-2)
session.close();
EXPECT_EQ(__result_group_seeds_2, get_sysvar(__mysql_sandbox_port2, "group_replication_group_seeds"));

c.disconnect();

//@ Remove instance (FR2-TS-2-2)
shell.connect({scheme: "mysql", host: localhost, port: __mysql_sandbox_port1, user: 'root', password: 'root'});
var c = dba.getCluster();
c.removeInstance(add_instance_options);

//@ Dissolve cluster
c.dissolve({force: true});

//@ Create cluster with a specific localAddress, groupSeeds and groupName (FR1-TS-4)
var __local_port1 = 20000 + __mysql_sandbox_port1;
var __local_port2 = 20000 + __mysql_sandbox_port2;
var __local_port3 = 20000 + __mysql_sandbox_port3;
var __cfg_local_address1 = localhost + ":" + __local_port1;
var __cfg_local_address2 = localhost + ":" + __local_port2;
var __cfg_local_address3 = localhost + ":" + __local_port3;
var __cfg_group_seeds = __cfg_local_address1 + "," + __cfg_local_address2 + "," + __cfg_local_address3;
var __cfg_group_name = "bbbbbbbb-aaaa-aaaa-aaaa-aaaaaaaaaaaa";
var c = dba.createCluster('test', {clearReadOnly: true, localAddress: __cfg_local_address1, groupSeeds: __cfg_group_seeds, groupName: __cfg_group_name, gtidSetIsComplete: true});

//@ Add instance with a specific localAddress and groupSeeds (FR1-TS-4)
c.addInstance(add_instance_options, {localAddress: __cfg_local_address2, groupSeeds: __cfg_group_seeds});
// Wait for metadata changes to be replicated on added instance
testutil.waitMemberTransactions(__mysql_sandbox_port2);

//@ Add a 3rd instance to ensure it will not affect the persisted group seed values specified on others (FR1-TS-4)
// NOTE: This instance is also used for convenience, to simplify the test
// (allow both other instance to be shutdown at the same time)
add_instance_options['port'] = __mysql_sandbox_port3;
c.addInstance(add_instance_options, {localAddress: __cfg_local_address3});
// Wait for metadata changes to be replicated on added instance
testutil.waitMemberTransactions(__mysql_sandbox_port3);

c.disconnect();

//@ Configure seed instance (FR1-TS-4)
var cnfPath1 = __sandbox_dir + __mysql_sandbox_port1 + "/my.cnf";
dba.configureLocalInstance({host: localhost, port: __mysql_sandbox_port1, password:'root', user: 'root'}, {mycnfPath: cnfPath1});

//@ Configure added instance (FR1-TS-4)
session.close();
add_instance_options['port'] = __mysql_sandbox_port2;
var cnfPath2 = __sandbox_dir + __mysql_sandbox_port2 + "/my.cnf";
dba.configureLocalInstance({host: localhost, port: __mysql_sandbox_port2, password:'root', user: 'root'}, {mycnfPath: cnfPath2});

//@ Stop seed and added instance with specific options (FR1-TS-4)
testutil.stopSandbox(__mysql_sandbox_port2, {wait: 1});
testutil.stopSandbox(__mysql_sandbox_port1, {wait: 1});

//@ Restart added instance (FR1-TS-4)
testutil.startSandbox(__mysql_sandbox_port2);
testutil.waitSandboxAlive(__mysql_sandbox_port2);

//@<> Confirm localAddress, groupSeeds, and groupName values were persisted for added instance (FR1-TS-4)
shell.connect({scheme: "mysql", host: localhost, port: __mysql_sandbox_port2, user: 'root', password: 'root'});
EXPECT_EQ(__cfg_local_address2, get_sysvar(session, "group_replication_local_address"));
EXPECT_EQ(__cfg_group_seeds, get_sysvar(session, "group_replication_group_seeds"));
EXPECT_EQ(__cfg_group_name, get_sysvar(session, "group_replication_group_name"));
session.close();

//@ Restart seed instance (FR1-TS-4)
testutil.startSandbox(__mysql_sandbox_port1);
testutil.waitSandboxAlive(__mysql_sandbox_port1);

//@<> Confirm localAddress, groupSeeds, and groupName values were persisted for seed instance (FR1-TS-4)
shell.connect({scheme: "mysql", host: localhost, port: __mysql_sandbox_port1, user: 'root', password: 'root'});
EXPECT_EQ(__cfg_local_address1, get_sysvar(session, "group_replication_local_address"));
EXPECT_EQ(__cfg_group_seeds, get_sysvar(session, "group_replication_group_seeds"));
EXPECT_EQ(__cfg_group_name, get_sysvar(session, "group_replication_group_name"));
session.close();

//@ Dissolve cluster (FR1-TS-4)
shell.connect({scheme: "mysql", host: localhost, port: __mysql_sandbox_port3, user: 'root', password: 'root'});
testutil.waitMemberState(__mysql_sandbox_port1, "ONLINE");
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");
c = dba.getCluster();
c.dissolve({force: true});

//@ Finalization
session.close();
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
testutil.destroySandbox(__mysql_sandbox_port3);
