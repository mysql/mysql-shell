// Assumptions: smart deployment functions available

//@ Initialization
testutil.deploySandbox(__mysql_sandbox_port1, "root");
testutil.snapshotSandboxConf(__mysql_sandbox_port1);
testutil.deploySandbox(__mysql_sandbox_port2, "root");
testutil.snapshotSandboxConf(__mysql_sandbox_port2);
testutil.deploySandbox(__mysql_sandbox_port3, "root");
testutil.snapshotSandboxConf(__mysql_sandbox_port3);

function print_gr_local_address() {
  var result = session.runSql('SELECT @@GLOBAL.group_replication_local_address');
  var row = result.fetchOne();
  print(row[0] + "\n");
}

function print_other_gr_local_address(cnx_opts) {
  shell.connect(cnx_opts);
  var result = session.runSql('SELECT @@GLOBAL.group_replication_local_address');
  var row = result.fetchOne();
  print(row[0] + "\n");
  session.close();
}

function print_gr_group_seeds() {
    var result = session.runSql('SELECT @@GLOBAL.group_replication_group_seeds');
    var row = result.fetchOne();
    print(row[0] + "\n");
}

function print_other_gr_group_seeds(cnx_opts) {
    shell.connect(cnx_opts);
    var result = session.runSql('SELECT @@GLOBAL.group_replication_group_seeds');
    var row = result.fetchOne();
    print(row[0] + "\n");
    session.close();
}

function print_gr_group_name() {
    var result = session.runSql('SELECT @@GLOBAL.group_replication_group_name');
    var row = result.fetchOne();
    print(row[0] + "\n");
}

function stop_sandbox(port) {
    testutil.stopSandbox(port);
}

shell.connect({scheme: "mysql", host: localhost, port: __mysql_sandbox_port1, user: 'root', password: 'root'});

//BUG#27758041: SHELL PICKS RANDOM PORTS IF GR LOCAL ADDRESS PORT IS BUSY
//@ Create cluster fails because port default GR local address port is already in use. {!__replaying && !__recording}
var __busy_port = __mysql_sandbox_port1 * 10 + 1;
var __valid_portx = (__mysql_sandbox_port1 * 10 + 9).toString();
testutil.deploySandbox(__busy_port, "root", {"loose_mysqlx_port": __valid_portx});
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
// FR1-TS-1-1
// Note: this test will try to connect to the port in localAddress to see if
// its free. Thus, the port must be busy even in replayed test runs.
var __local_address_1 = localhost + ":" + __port;
var c = dba.createCluster('test', {localAddress: __local_address_1});

//@ Create cluster errors using groupSeeds option
// FR2-TS-1-3
var c = dba.createCluster('test', {groupSeeds: ""});
// FR2-TS-1-4
var c = dba.createCluster('test', {groupSeeds: "abc"});
// Clear invalid group seed value (to avoid further GR errors for next tests).
session.runSql('SET @@GLOBAL.group_replication_group_seeds = ""');

//@ Create cluster errors using groupName option
// FR3-TS-1-2
var c = dba.createCluster('test', {groupName: ""});
// FR3-TS-1-3
var c = dba.createCluster('test', {groupName: "abc"});

//@ Create cluster specifying :<valid_port> for localAddress (FR1-TS-1-2)
var valid_port = __mysql_sandbox_port1 + 20000;
var __local_address_2 = ":" + valid_port;
var __result_local_address_2 = localhost + __local_address_2;
var c = dba.createCluster('test', {clearReadOnly: true, localAddress: __local_address_2});

//@<OUT> Confirm local address is set correctly (FR1-TS-1-2)
print_gr_local_address();

//@ Dissolve cluster (FR1-TS-1-2)
c.dissolve({force: true});

//@ Create cluster specifying <valid_host>: for localAddress (FR1-TS-1-3)
var __local_address_3 = localhost + ":";
var result_port = __mysql_sandbox_port1 * 10 + 1;
var __result_local_address_3 = __local_address_3 + result_port;
var c = dba.createCluster('test', {clearReadOnly: true, localAddress: __local_address_3});

//@<OUT> Confirm local address is set correctly (FR1-TS-1-3)
print_gr_local_address();

//@ Dissolve cluster (FR1-TS-1-3)
c.dissolve({force: true});

//@ Create cluster specifying <valid_port> for localAddress (FR1-TS-1-4)
var __local_address_4 = "12345";
var __result_local_address_4 = localhost + ":" + __local_address_4;
var c = dba.createCluster('test', {clearReadOnly: true, localAddress: __local_address_4});

//@<OUT> Confirm local address is set correctly (FR1-TS-1-4)
print_gr_local_address();

//@ Dissolve cluster (FR1-TS-1-4)
c.dissolve({force: true});

//@ Create cluster specifying <valid_host> for localAddress (FR1-TS-1-9)
var __local_address_9 = localhost;
var __result_local_address_9 = __local_address_9 + ":" + result_port;
var c = dba.createCluster('test', {clearReadOnly: true, localAddress: __local_address_9});

//@<OUT> Confirm local address is set correctly (FR1-TS-1-9)
print_gr_local_address();

//@ Dissolve cluster (FR1-TS-1-9)
c.dissolve({force: true});

//@ Create cluster specifying <valid_host>:<valid_port> for localAddress (FR1-TS-1-10)
var __local_address_10 = localhost + ":12346";
var __result_local_address_10 = __local_address_10;
var c = dba.createCluster('test', {clearReadOnly: true, localAddress: __local_address_10});

//@<OUT> Confirm local address is set correctly (FR1-TS-1-10)
print_gr_local_address();

//@ Dissolve cluster (FR1-TS-1-10)
c.dissolve({force: true});

//@ Create cluster specifying 127.0.0.1:<valid_port> for groupSeeds (FR2-TS-1-1)
var default_valid_port1 = __mysql_sandbox_port1 * 10 + 1;
var __group_seeds_1 = "127.0.0.1:" + default_valid_port1;
var __result_group_seeds_1 = __group_seeds_1;
var c = dba.createCluster('test', {clearReadOnly: true, groupSeeds: __group_seeds_1});

//@<OUT> Confirm group seeds is set correctly (FR2-TS-1-1)
print_gr_group_seeds();

//@ Dissolve cluster (FR2-TS-1-1)
c.dissolve({force: true});

//@ Create cluster specifying 127.0.0.1:<valid_port>,127.0.0.1:<valid_port2> for groupSeeds (FR2-TS-1-2)
var default_valid_port1 = __mysql_sandbox_port1 * 10 + 1;
var default_valid_port2 = __mysql_sandbox_port2 * 10 + 1;
var __group_seeds_2 = "127.0.0.1:" + default_valid_port1 + ",127.0.0.1:" + default_valid_port2;
var __result_group_seeds_2 = __group_seeds_2;
var c = dba.createCluster('test', {clearReadOnly: true, groupSeeds: __group_seeds_2});

//@<OUT> Confirm group seeds is set correctly (FR2-TS-1-2)
print_gr_group_seeds();

//@ Dissolve cluster (FR2-TS-1-2)
c.dissolve({force: true});

//@ Create cluster specifying aaaaaaaa-aaaa-aaaa-aaaa-aaaaaaaaaaaa for groupName (FR3-TS-1-1)
var __group_name_1 = "aaaaaaaa-aaaa-aaaa-aaaa-aaaaaaaaaaaa";
var __result_group_name_1 = __group_name_1;
var c = dba.createCluster('test', {clearReadOnly: true, groupName: __group_name_1});

//@<OUT> Confirm group name is set correctly (FR3-TS-1-1)
print_gr_group_name();

//@ Dissolve cluster (FR3-TS-1-1)
c.dissolve({force: true});

//@ Create cluster
var c = dba.createCluster('test', {clearReadOnly: true});

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
// FR1-TS-2-1
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
var __result_local_address_add_2 = localhost + __local_address_add_2;
c.addInstance(add_instance_options, {localAddress: __local_address_add_2});

//@<OUT> Confirm local address is set correctly (FR1-TS-2-2)
session.close();
print_other_gr_local_address(add_instance_options);

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

//@<OUT> Confirm local address is set correctly (FR1-TS-2-3)
session.close();
print_other_gr_local_address(add_instance_options);

c.disconnect();

//@ Remove instance (FR1-TS-2-3)
shell.connect({scheme: "mysql", host: localhost, port: __mysql_sandbox_port1, user: 'root', password: 'root'});
var c = dba.getCluster();
c.removeInstance(add_instance_options);

//@ Add instance specifying <valid_port> for localAddress (FR1-TS-2-4)
var __local_address_add_4 = "12347";
var __result_local_address_add_4 = localhost + ":" + __local_address_add_4;
c.addInstance(add_instance_options, {localAddress: __local_address_add_4});

//@<OUT> Confirm local address is set correctly (FR1-TS-2-4)
session.close();
print_other_gr_local_address(add_instance_options);

c.disconnect();

//@ Remove instance (FR1-TS-2-4)
shell.connect({scheme: "mysql", host: localhost, port: __mysql_sandbox_port1, user: 'root', password: 'root'});
var c = dba.getCluster();
c.removeInstance(add_instance_options);

//@ Add instance specifying <valid_host> for localAddress (FR1-TS-2-9)
var __local_address_add_9 = localhost;
var __result_local_address_add_9 = __local_address_add_9 + ":" + result_port2;
c.addInstance(add_instance_options, {localAddress: __local_address_add_9});

//@<OUT> Confirm local address is set correctly (FR1-TS-2-9)
session.close();
print_other_gr_local_address(add_instance_options);

c.disconnect();

//@ Remove instance (FR1-TS-2-9)
shell.connect({scheme: "mysql", host: localhost, port: __mysql_sandbox_port1, user: 'root', password: 'root'});
var c = dba.getCluster();
c.removeInstance(add_instance_options);

//@ Add instance specifying <valid_host>:<valid_port> for localAddress (FR1-TS-2-10)
var __local_address_add_10 = localhost + ":12348";
var __result_local_address_add_10 = __local_address_add_10;
c.addInstance(add_instance_options, {localAddress: __local_address_add_10});

//@<OUT> Confirm local address is set correctly (FR1-TS-2-10)
session.close();
print_other_gr_local_address(add_instance_options);

c.disconnect();

//@ Remove instance (FR1-TS-2-10)
shell.connect({scheme: "mysql", host: localhost, port: __mysql_sandbox_port1, user: 'root', password: 'root'});
var c = dba.getCluster();
c.removeInstance(add_instance_options);

//@ Add instance specifying 127.0.0.1:<valid_port> for groupSeeds (FR2-TS-2-1)
c.addInstance(add_instance_options, {groupSeeds: __group_seeds_1});

//@<OUT> Confirm group seeds is set correctly (FR2-TS-2-1)
session.close();
print_other_gr_group_seeds(add_instance_options);

c.disconnect();

//@ Remove instance (FR2-TS-2-1)
shell.connect({scheme: "mysql", host: localhost, port: __mysql_sandbox_port1, user: 'root', password: 'root'});
var c = dba.getCluster();
c.removeInstance(add_instance_options);

//@ Add instance specifying 127.0.0.1:<valid_port>,127.0.0.1:<valid_port2> for groupSeeds (FR2-TS-2-2)
c.addInstance(add_instance_options, {groupSeeds: __group_seeds_2});

//@<OUT> Confirm group seeds is set correctly (FR2-TS-2-2)
session.close();
print_other_gr_group_seeds(add_instance_options);

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
var c = dba.createCluster('test', {clearReadOnly: true, localAddress: __cfg_local_address1, groupSeeds: __cfg_group_seeds, groupName: __cfg_group_name});

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
shell.connect(add_instance_options);
var cnfPath2 = __sandbox_dir + __mysql_sandbox_port2 + "/my.cnf";
dba.configureLocalInstance({host: localhost, port: __mysql_sandbox_port2, password:'root', user: 'root'}, {mycnfPath: cnfPath2});

//@ Stop seed and added instance with specific options (FR1-TS-4)
stop_sandbox(__mysql_sandbox_port2);
stop_sandbox(__mysql_sandbox_port1);

//@ Restart added instance (FR1-TS-4)
testutil.startSandbox(__mysql_sandbox_port2);

//@<OUT> Confirm localAddress, groupSeeds, and groupName values were persisted for added instance (FR1-TS-4)
shell.connect(add_instance_options);
print_gr_local_address();
print_gr_group_seeds();
print_gr_group_name();

//@ Restart seed instance (FR1-TS-4)
testutil.startSandbox(__mysql_sandbox_port1);

//@<OUT> Confirm localAddress, groupSeeds, and groupName values were persisted for seed instance (FR1-TS-4)
shell.connect({scheme: "mysql", host: localhost, port: __mysql_sandbox_port1, user: 'root', password: 'root'});
print_gr_local_address();
print_gr_group_seeds();
print_gr_group_name();

//@ Dissolve cluster (FR1-TS-4)
shell.connect({scheme: "mysql", host: localhost, port: __mysql_sandbox_port3, user: 'root', password: 'root'});
c = dba.getCluster();
c.dissolve({force: true});

//@ Finalization
session.close();
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
testutil.destroySandbox(__mysql_sandbox_port3);
