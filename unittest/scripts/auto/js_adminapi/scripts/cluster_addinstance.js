// Assumptions: smart deployment functions available

function print_metadata_instance_addresses(session) {
    var res = session.runSql("select instance_name, addresses from mysql_innodb_cluster_metadata.instances").fetchAll();
    for (var i = 0; i < res.length; i++) {
        print(res[i][0] + " = " + res[i][1] + "\n");
    }
    print("\n");
}

function get_recovery_user(session) {
    var res = session.runSql(
        "SELECT user_name FROM mysql.slave_master_info " +
        "WHERE Channel_name = 'group_replication_recovery'");
    var row = res.fetchOne();
    return row[0];
}

function number_of_non_expiring_pwd_accounts(session, user) {
    // Account with non expiring password have password_lifetime = 0.
    var res = session.runSql(
        "SELECT COUNT(*)  FROM mysql.user u WHERE u.user = '" + user +
        "' AND password_lifetime = 0");
    var row = res.fetchOne();
    return row[0];
}




// BUG#27084767: CREATECLUSTER()/ ADDINSTANCE() DOES NOT CORRECTLY SET AUTO_INCREMENT_OFFSET
//
// dba.createCluster() and addInstance() in single-primary mode, must set the following values:
//
// auto_increment_offset = 2
// auto_increment_increment = 1
//
// And in multi-primary mode:
//
// auto_increment_offset = 1 + server_id % 7
// auto_increment_increment = 7
//
// The value setting should not differ if the target instance is a sandbox or not

// Test with a sandbox

// Test in single-primary mode

//@ BUG#27084767: Initialize new instances
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: hostname});
testutil.deploySandbox(__mysql_sandbox_port2, "root", {report_host: hostname});

shell.connect(__sandbox_uri1);

// Enable offline_mode (BUG#33396423)
var session2 = mysql.getSession(__sandbox_uri2);
session.runSql("SET GLOBAL offline_mode=1");
session2.runSql("SET GLOBAL offline_mode=1");

//@ BUG#27084767: Create a cluster in single-primary mode
var c = dba.createCluster('test', {gtidSetIsComplete: true});

//@<> BUG#27084767: Verify the values of auto_increment_% in the seed instance
EXPECT_EQ(1, get_sysvar(session, "auto_increment_increment"));
EXPECT_EQ(2, get_sysvar(session, "auto_increment_offset"));

//@ BUG#27084767: Add instance to cluster in single-primary mode
c.addInstance(__sandbox_uri2)
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");

//@<> BUG#27084767: Verify the values of auto_increment_%
EXPECT_EQ(1, get_sysvar(__mysql_sandbox_port2, "auto_increment_increment"));
EXPECT_EQ(2, get_sysvar(__mysql_sandbox_port2, "auto_increment_offset"));

// ensure offline_mode was disabled (BUG#33396423)
EXPECT_EQ(0, get_sysvar(__mysql_sandbox_port1, "offline_mode"));
EXPECT_EQ(0, get_sysvar(__mysql_sandbox_port2, "offline_mode"));

// Test in multi-primary mode

//@ BUG#27084767: Dissolve the cluster
c.dissolve({force: true})
session.close();

shell.connect(__sandbox_uri1);

//@ BUG#27084767: Create a cluster in multi-primary mode
var c = dba.createCluster('test', {multiPrimary: true, force: true, clearReadOnly: true, gtidSetIsComplete: true});

// Reconnect the session before validating the values of auto_increment_%
// This must be done because 'SET PERSIST' changes the values globally (GLOBAL) and not per session
session.close();
shell.connect(__sandbox_uri1);

// Get the server_id to calculate the expected value of auto_increment_offset
var result = session.runSql("SELECT @@server_id");
var row = result.fetchOne();
var server_id = row[0];

var __expected_auto_inc_offset = 1 + server_id%7

//@<> BUG#27084767: Verify the values of auto_increment_% in the seed instance in multi-primary mode
EXPECT_EQ(7, get_sysvar(session, "auto_increment_increment"));
EXPECT_EQ(__expected_auto_inc_offset, get_sysvar(session, "auto_increment_offset"));

//@ BUG#27084767: Add instance to cluster in multi-primary mode
c.addInstance(__sandbox_uri2)
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");

// Connect to the instance 2 to perform the auto_increment_% validations
session.close();
shell.connect(__sandbox_uri2);

// Get the server_id to calculate the expected value of auto_increment_offset
var result = session.runSql("SELECT @@server_id");
var row = result.fetchOne();
var server_id = row[0];

var __expected_auto_inc_offset = 1 + server_id%7

//@<> BUG#27084767: Verify the values of auto_increment_% multi-primary
EXPECT_EQ(7, get_sysvar(session, "auto_increment_increment"));
EXPECT_EQ(__expected_auto_inc_offset, get_sysvar(session, "auto_increment_offset"));

//@ BUG#27084767: Finalization
c.disconnect()
session.close();
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);

// Test with non-sandbox instance

// Test in single-primary mode

//@ BUG#27084767: Initialize new non-sandbox instance
testutil.deployRawSandbox(__mysql_sandbox_port1, 'root', {report_host: hostname});
testutil.snapshotSandboxConf(__mysql_sandbox_port1);
testutil.deployRawSandbox(__mysql_sandbox_port2, 'root', {report_host: hostname});
testutil.snapshotSandboxConf(__mysql_sandbox_port2);
var sandbox_cnf1 = testutil.getSandboxConfPath(__mysql_sandbox_port1);
dba.configureInstance(__sandbox_uri1, {clusterAdmin:'root', mycnfPath: sandbox_cnf1});
testutil.stopSandbox(__mysql_sandbox_port1);
testutil.startSandbox(__mysql_sandbox_port1);
var sandbox_cnf2 = testutil.getSandboxConfPath(__mysql_sandbox_port2);
dba.configureInstance(__sandbox_uri2, {clusterAdmin:'root', mycnfPath: sandbox_cnf2});
testutil.stopSandbox(__mysql_sandbox_port2);
testutil.startSandbox(__mysql_sandbox_port2);

// Connect to instance1 to create the cluster
shell.connect(__hostname_uri1);

//@ BUG#27084767: Create a cluster in single-primary mode non-sandbox
var c = dba.createCluster('test', {gtidSetIsComplete: true});

//@<> BUG#27084767: Verify the values of auto_increment_% in the seed instance non-sandbox
EXPECT_EQ(1, get_sysvar(session, "auto_increment_increment"));
EXPECT_EQ(2, get_sysvar(session, "auto_increment_offset"));

//@ BUG#27084767: Add instance to cluster in single-primary mode non-sandbox
c.addInstance(__hostname_uri2)
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");

//@<> BUG#27084767: Verify the values of auto_increment_% non-sandbox
EXPECT_EQ(1, get_sysvar(__mysql_sandbox_port2, "auto_increment_increment"));
EXPECT_EQ(2, get_sysvar(__mysql_sandbox_port2, "auto_increment_offset"));

// Test in multi-primary mode

//@ BUG#27084767: Dissolve the cluster non-sandbox
c.dissolve({force: true})

// Connect to instance1 to create the cluster
shell.connect(__hostname_uri1);

//@ BUG#27084767: Create a cluster in multi-primary mode non-sandbox
var c = dba.createCluster('test', {multiPrimary: true, force: true, clearReadOnly: true, gtidSetIsComplete: true});

// Reconnect the session before validating the values of auto_increment_%
// This must be done because 'SET PERSIST' changes the values globally (GLOBAL) and not per session
session.close();
shell.connect(__sandbox_uri1);

// Get the server_id to calculate the expected value of auto_increment_offset
var result = session.runSql("SELECT @@server_id");
var row = result.fetchOne();
var server_id = row[0];

var __expected_auto_inc_offset = 1 + server_id%7

//@<> BUG#27084767: Verify the values of auto_increment_% in multi-primary non-sandbox
EXPECT_EQ(7, get_sysvar(session, "auto_increment_increment"));
EXPECT_EQ(__expected_auto_inc_offset, get_sysvar(session, "auto_increment_offset"));

//@ BUG#27084767: Add instance to cluster in multi-primary mode non-sandbox
c.addInstance(__hostname_uri2)
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");

// Connect to the instance 2 to perform the auto_increment_% validations
session.close();
shell.connect(__sandbox_uri2);

// Get the server_id to calculate the expected value of auto_increment_offset
var result = session.runSql("SELECT @@server_id");
var row = result.fetchOne();
var server_id = row[0];

var __expected_auto_inc_offset = 1 + server_id%7

//@<> BUG#27084767: Verify the values of auto_increment_% multi-primary non-sandbox
EXPECT_EQ(7, get_sysvar(__mysql_sandbox_port2, "auto_increment_increment"));
EXPECT_EQ(__expected_auto_inc_offset, get_sysvar(__mysql_sandbox_port2, "auto_increment_offset"));

//@ BUG#27084767: Finalization non-sandbox
c.disconnect();
session.close();
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);

//@ BUG#27677227 cluster with x protocol disabled setup
WIPE_SHELL_LOG();
testutil.deploySandbox(__mysql_sandbox_port1, "root", {"mysqlx":"0", report_host: hostname});
testutil.deploySandbox(__mysql_sandbox_port2, "root", {"mysqlx":"0", report_host: hostname});
testutil.deploySandbox(__mysql_sandbox_port3, "root", {report_host: hostname});

shell.connect(__sandbox_uri1);
c = dba.createCluster('noxplugin', {gtidSetIsComplete: true});
c.addInstance(__sandbox_uri2);
c.addInstance(__sandbox_uri3);

var msg1 = "The X plugin is not enabled on instance '" + hostname + ":" + __mysql_sandbox_port1 + "'. No value will be assumed for the X protocol address.";
var msg2 = "The X plugin is not enabled on instance '" + hostname + ":" + __mysql_sandbox_port2 + "'. No value will be assumed for the X protocol address.";
EXPECT_SHELL_LOG_CONTAINS(msg1);
EXPECT_SHELL_LOG_CONTAINS(msg2);

//@<OUT> BUG#27677227 cluster with x protocol disabled, mysqlx should be NULL
print_metadata_instance_addresses(session);

//@ BUG#27677227 cluster with x protocol disabled cleanup
c.disconnect();
session.close();
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
testutil.destroySandbox(__mysql_sandbox_port3);

// BUG28056944: cannot add instance after removing its metadata
//@ BUG28056944 deploy sandboxes.
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: hostname});
testutil.deploySandbox(__mysql_sandbox_port2, "root", {report_host: hostname});

//@ BUG28056944 create cluster.
shell.connect(__hostname_uri1);
var c = dba.createCluster('test_cluster', {gtidSetIsComplete: true});
c.addInstance(__hostname_uri2);
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");

//@<> BUG28056944 remove instance with wrong password and force = true.
var wrong_pwd_uri = "root:wrongpawd@" + hostname + ":" + __mysql_sandbox_port2;
c.removeInstance(wrong_pwd_uri, {force: true});

//@<> BUG28056944 Error adding instance already in group but not in Metadata.
// This should pass by just adding the instance to the metadata
c.addInstance(__hostname_uri2);

//@ BUG28056944 clean-up.
c.disconnect();
session.close();
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);

// BUG#29305551: ADMINAPI FAILS TO DETECT INSTANCE IS RUNNING ASYNCHRONOUS REPLICATION
//
// dba.checkInstance() reports that a target instance which is running the Slave
// SQL and IO threads is valid to be used in an InnoDB cluster.
//
// As a consequence, the AdminAPI fails to detects that an instance has
// asynchronous replication running and both addInstance() and rejoinInstance()
// fail with useless/unfriendly errors on this scenario. There's not even
// information on how to overcome the issue.

//@<> BUG#29305551: Initialization
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: hostname});
testutil.snapshotSandboxConf(__mysql_sandbox_port1);
testutil.deploySandbox(__mysql_sandbox_port2, "root", {report_host: hostname});
testutil.snapshotSandboxConf(__mysql_sandbox_port2);
shell.connect(__sandbox_uri1);
var c = dba.createCluster('test', {gtidSetIsComplete: true});
session.close();

//@<> BUG#29305551: Setup asynchronous replication
// Create Replication user
shell.connect(__sandbox_uri1);
session.runSql("CREATE USER 'repl'@'%' IDENTIFIED BY 'password' REQUIRE SSL");
session.runSql("GRANT REPLICATION SLAVE ON *.* TO 'repl'@'%';");

// Set async channel on instance2
session.close();
shell.connect(__sandbox_uri2);

session.runSql("CHANGE MASTER TO MASTER_HOST='" + hostname + "', MASTER_PORT=" + __mysql_sandbox_port1 + ", MASTER_USER='repl', MASTER_PASSWORD='password', MASTER_AUTO_POSITION=1, MASTER_SSL=1");
session.runSql("START SLAVE");

testutil.waitMemberTransactions(__mysql_sandbox_port2, __mysql_sandbox_port1);

//@ AddInstance async replication error
c.addInstance(__sandbox_uri2);

// BUG#32197197: ADMINAPI DOES NOT PROPERLY CHECK FOR PRECONFIGURED REPLICATION CHANNELS
//
// Even if replication is not running but configured, the warning/error has to
// be provided as implemented in BUG#29305551
session.runSql("STOP SLAVE");

//@ AddInstance async replication error with channels stopped
c.addInstance(__sandbox_uri2);

//@ BUG#29305551: Finalization
session.close();
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);

// BUG#29809560: addinstance() does not validate if server_id is unique in the cluster
//@ BUG#29809560: deploy sandboxes.
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: hostname});
testutil.deploySandbox(__mysql_sandbox_port2, "root", {report_host: hostname});

//@ BUG#29809560: set the same server id an all instances.
shell.connect(__hostname_uri1);
session.runSql("SET GLOBAL server_id = 666");
session.close();
shell.connect(__hostname_uri2);
session.runSql("SET GLOBAL server_id = 666");
session.close();

//@ BUG#29809560: create cluster.
shell.connect(__hostname_uri1);
var c = dba.createCluster('test', {gtidSetIsComplete: true});

//@<> BUG#29809560: add instance fails because server_id is not unique.
c.addInstance(__hostname_uri2);

//@ BUG#29809560: clean-up.
c.disconnect();
session.close();
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);

// BUG#28855764: user created by shell expires with default_password_lifetime
//@ BUG#28855764: deploy sandboxes.
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: hostname});
testutil.deploySandbox(__mysql_sandbox_port2, "root", {report_host: hostname});

//@ BUG#28855764: create cluster.
shell.connect(__hostname_uri1);
var c = dba.createCluster('test', {gtidSetIsComplete: true});

//@ BUG#28855764: add instance an instance to the cluster
c.addInstance(__hostname_uri2);

//@ BUG#28855764: get recovery user for instance 2.
session.close();
shell.connect(__hostname_uri2);
var recovery_user_2 = get_recovery_user(session);

//@ BUG#28855764: get recovery user for instance 1.
session.close();
shell.connect(__hostname_uri1);
var recovery_user_1 = get_recovery_user(session);

//@<> BUG#28855764: Passwords for recovery users never expire (password_lifetime=0).
print("Number of accounts for '" + recovery_user_1 + "': " + number_of_non_expiring_pwd_accounts(session, recovery_user_1) + "\n");
print("Number of accounts for '" + recovery_user_2 + "': " + number_of_non_expiring_pwd_accounts(session, recovery_user_2) + "\n");

//@ BUG#28855764: clean-up.
c.disconnect();
session.close();
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);

// WL#12773 AdminAPI: Simplification of internal recovery accounts

//@<> WL#12773: Initialization
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: hostname, server_id: 11111});
testutil.deploySandbox(__mysql_sandbox_port2, "root", {report_host: hostname, server_id: 22222});
testutil.deploySandbox(__mysql_sandbox_port3, "root", {report_host: hostname, server_id: 33333});
shell.connect(__sandbox_uri1);

var c;
if (__version_num < 80027) {
    c = dba.createCluster('test', {gtidSetIsComplete: true});
} else {
    c = dba.createCluster('test', {gtidSetIsComplete: true, communicationStack: "XCOM"});
}

// FR1 - A new auto-generated recovery account must be created whenever creating a cluster and joining an instance to a cluster
//@<> WL#12773: FR1.1 - The account user-name shall be mysql_innodb_cluster_<server_id>
c.addInstance(__sandbox_uri2);
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");
var result = session.runSql("SELECT COUNT(*) FROM mysql.user WHERE User = 'mysql_innodb_cluster_22222'");
var number = result.fetchOne()[0];
EXPECT_EQ(1, number);

//@<> WL#12773: FR1.2 - The host-name shall be %
result = session.runSql("SELECT Host FROM mysql.user WHERE User = 'mysql_innodb_cluster_22222'");
var user_host = result.fetchOne()[0];
EXPECT_EQ("%", user_host);

//@ WL#12773: FR4 - The ipWhitelist shall not change the behavior defined by FR1
result = session.runSql("SELECT COUNT(*) FROM mysql.user");
var old_account_number = result.fetchOne()[0];
c.addInstance(__sandbox_uri3, {ipWhitelist:"192.168.2.1/15,127.0.0.1," + hostname_ip});
testutil.waitMemberState(__mysql_sandbox_port3, "ONLINE");
print(get_all_gr_recovery_accounts(session));

result = session.runSql("SELECT COUNT(*) FROM mysql.user");
var new_account_number = result.fetchOne()[0];
EXPECT_EQ(old_account_number + 1, new_account_number);

// Tests for deprecation of ipWhitelist in favor of ipAllowlist

//@<> Remove an instance to add it back using ipWhitelist
c.removeInstance(__sandbox_uri2);

//@<> Verify that ipWhitelist sets the right sysvars depending on the version
var ip_white_list = "10.10.10.1/15,127.0.0.1," + hostname_ip;

c.addInstance(__sandbox_uri2, {ipWhitelist: ip_white_list});
shell.connect(__sandbox_uri2);

//@<> Verify that ipWhitelist sets group_replication_ip_allowlist in 8.0.22 {VER(>=8.0.22)}
EXPECT_EQ(ip_white_list, get_sysvar(session, "group_replication_ip_allowlist"));

//@<> Verify that ipWhitelist sets group_replication_ip_whitelist in versions < 8.0.22 {VER(<8.0.22)}
EXPECT_EQ(ip_white_list, get_sysvar(session, "group_replication_ip_whitelist"));

//@<> Remove the instance to add it back using ipAllowlist
c.removeInstance(__sandbox_uri2);

//@<> Verify the new option ipAllowlist sets the right sysvars depending on the version
ip_white_list = "10.1.1.0/15,127.0.0.1," + hostname_ip;

c.addInstance(__sandbox_uri2, {ipAllowlist:ip_white_list});
shell.connect(__sandbox_uri2);

//@<> Verify the new option ipAllowlist sets group_replication_ip_allowlist in 8.0.22 {VER(>=8.0.22)}
EXPECT_EQ(ip_white_list, get_sysvar(session, "group_replication_ip_allowlist"));

//@<> Verify the new option ipAllowlist sets group_replication_ip_whitelist in versions < 8.0.22 {VER(<8.0.22)}
EXPECT_EQ(ip_white_list, get_sysvar(session, "group_replication_ip_whitelist"));

//@<> WL#12773: Cleanup
session.close();
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
testutil.destroySandbox(__mysql_sandbox_port3);

// BUG#25503159: addInstance() does not drop recovery user when it fails
//@<> BUG#25503159: deploy sandboxes.
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: hostname});
testutil.deploySandbox(__mysql_sandbox_port2, "root", {report_host: hostname});

//@<> BUG#25503159: create cluster.
shell.connect(__hostname_uri1);
var c;

if (__version_num >= 80027) {
    c = dba.createCluster('test', {gtidSetIsComplete: true, communicationStack: "XCOM"});
} else {
    c = dba.createCluster('test', {gtidSetIsComplete: true});
}

//@ BUG#25503159: number of recovery users before executing addInstance().
var num_recovery_users = number_of_gr_recovery_accounts(session);
print("Number of recovery user before addInstance(): " + num_recovery_users + "\n");

//@ BUG#25503159: add instance fail (using an invalid localaddress).
c.addInstance(__hostname_uri2, {localAddress: "invalid_host"});

//@<> BUG#25503159: number of recovery users is the same.
var num_recovery_users_after = number_of_gr_recovery_accounts(session);
EXPECT_EQ(num_recovery_users, num_recovery_users_after);

// BUG#30281908: MYSQL SHELL CRASHED WHILE ADDING A INSTANCE TO INNODB CLUSTER
// This bug was caused by a segmentation fault in the post-clone restart handling
// that did not cover the timeout waiting for the instance to start after clone restarts it.
// To test it, we change the sandbox configuration file to introduce a bogus setting which will
// cause the failure of the instance restart, simulating then a timeout.

//@ BUG#30281908: add instance using clone and simulating a restart timeout {VER(>= 8.0.17)}
testutil.changeSandboxConf(__mysql_sandbox_port2, "foo", "bar");
// Also tests the restartWaitTimeout option
shell.options["dba.restartWaitTimeout"] = 1;
c.addInstance(__sandbox_uri2, {interactive:true, recoveryMethod:"clone"});
shell.options["dba.restartWaitTimeout"] = 60;

//@<> BUG#30281908: restart the instance manually {VER(>= 8.0.17)}
testutil.removeFromSandboxConf(__mysql_sandbox_port2, "foo");
testutil.startSandbox(__mysql_sandbox_port2);
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");

//@ BUG#30281908: complete the process with .rescan() {VER(>= 8.0.17)}
testutil.expectPrompt("Would you like to add it to the cluster metadata? [Y/n]:", "y");
c.rescan({interactive:true});

//@<> BUG#25503159: clean-up.
c.disconnect();
session.close();
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);

//@<> Initialization canonical IPv6 addresses supported WL#12758 {VER(>= 8.0.14)}
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: "::1"});
testutil.deploySandbox(__mysql_sandbox_port2, "root", {report_host: "::1"});
shell.connect(__sandbox_uri1);
var cluster = dba.createCluster("cluster", {gtidSetIsComplete: true});

//@<> canonical IPv6 addresses are supported on addInstance WL#12758 {VER(>= 8.0.14)}
cluster.addInstance(__sandbox_uri2);
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");
// Bug #30548843 Validate that IPv6 values stored on metadata for mysqlx are valid
print_metadata_instance_addresses(session);

//@<> Cleanup canonical IPv6 addresses supported WL#12758 {VER(>= 8.0.14)}
session.close();
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);

//@<> Initialization sandbox WL#12758 IPv6 addresses supported {VER(>= 8.0.14)}
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: "::1"});
testutil.deploySandbox(__mysql_sandbox_port2, "root");
shell.connect(__sandbox_uri1);
var cluster;
if (__version_num < 80027) {
    cluster = dba.createCluster("cluster", {gtidSetIsComplete: true});
} else {
    cluster = dba.createCluster("cluster", {gtidSetIsComplete: true, communicationStack: "xcom"});
}

//@<> IPv6 addresses are supported on localAddress and ipWhitelist WL#12758 {VER(>= 8.0.14)}
var port = __mysql_sandbox_port2*10+1;
var local_address = "[::1]:" + port;
var ip_white_list = "::1, 127.0.0.1";
cluster.addInstance(__sandbox_uri2, {ipWhitelist:ip_white_list, localAddress:local_address});
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");
shell.connect(__sandbox_uri2);
EXPECT_EQ(local_address, get_sysvar(session, "group_replication_local_address"));

//@<> Confirm that ipWhitelist is set {VER(>= 8.0.14) && VER(<8.0.22)}
EXPECT_EQ(ip_white_list, get_sysvar(session, "group_replication_ip_whitelist"));

//@<> Confirm that ipWhitelist is set {VER(>= 8.0.22)}
EXPECT_EQ(ip_white_list, get_sysvar(session, "group_replication_ip_allowlist"));

//@<> Cleanup WL#12758 IPv6 addresses supported {VER(>= 8.0.14)}
session.close();
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);

//@<> Initialization canonical IPv6 addresses are not supported below 8.0.14 WL#12758 {VER(< 8.0.14)}
testutil.deploySandbox(__mysql_sandbox_port1, "root");
testutil.deploySandbox(__mysql_sandbox_port2, "root", {report_host: "::1"});
shell.connect(__sandbox_uri1);
var cluster = dba.createCluster("cluster", {gtidSetIsComplete: true});

//@ canonical IPv6 addresses are not supported below 8.0.14 WL#12758 {VER(< 8.0.14)}
cluster.addInstance(__sandbox_uri2);

//@<> Cleanup canonical IPv6 addresses are not supported below 8.0.14 WL#12758 {VER(< 8.0.14)}
session.close();
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);

//@<> Initialization IPv6 addresses are not supported below 8.0.14 WL#12758 {VER(< 8.0.14)}
testutil.deploySandbox(__mysql_sandbox_port1, "root");
testutil.deploySandbox(__mysql_sandbox_port2, "root");
shell.connect(__sandbox_uri1);
var cluster = dba.createCluster("cluster", {gtidSetIsComplete: true});

//@ IPv6 local_address is not supported below 8.0.14 WL#12758 {VER(< 8.0.14)}
var local_address = "[::1]:" + __mysql_sandbox_gr_port1;
cluster.addInstance(__sandbox_uri2, {localAddress: local_address});

//@ IPv6 on ipWhitelist is not supported below 8.0.14 WL#12758 {VER(< 8.0.14)}
cluster.addInstance(__sandbox_uri2, {ipWhitelist: "::1, 127.0.0.1"});

//@<> Cleanup IPv6 addresses are not supported below 8.0.14 WL#12758 {VER(< 8.0.14)}
session.close();
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);

//@<> cluster.addInstance does not error if using instance with binlog_checksum enabled BUG#31329024 {VER(>= 8.0.21)}
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: hostname, binlog_checksum: "NONE"});
testutil.deploySandbox(__mysql_sandbox_port2, "root", {report_host: hostname, binlog_checksum: "CRC32"});

shell.connect(__sandbox_uri1);
cluster = dba.createCluster("sample", {gtidSetIsComplete: true});
cluster.addInstance(__sandbox_uri2);
EXPECT_STDERR_EMPTY();

//<> Adding an existing replica set should produce an error BUG#31964273 {VER(>=8.0.19)}
cluster.removeInstance(__sandbox_uri2);

shell.connect(__sandbox_uri2);
EXPECT_NO_THROWS(function(){ dba.createReplicaSet("rs"); });

shell.connect(__sandbox_uri1);
cluster = dba.getCluster();
EXPECT_THROWS(function(){
    cluster.addInstance(__sandbox_uri2);
}, `The instance '${hostname}:${__mysql_sandbox_port2}' is already part of an InnoDB ReplicaSet`);

shell.connect(__sandbox_uri2);
dba.dropMetadataSchema({force:true, clearReadOnly:true});
session.runSql("RESET MASTER");

//@<> Check if fails when GR local address isn't properly read {!__dbug_off}
if (testutil.versionCheck(__version, "<", "8.0.21")) {
    testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: hostname});
    testutil.deploySandbox(__mysql_sandbox_port2, "root", {report_host: hostname});

    shell.connect(__sandbox_uri1);
    cluster = dba.createCluster("sample", {gtidSetIsComplete: true});
    EXPECT_STDERR_EMPTY();
}
else {
    shell.connect(__sandbox_uri1);
    cluster = dba.getCluster();
}

testutil.dbugSet("+d,dba_instance_query_gr_local_address");
EXPECT_THROWS(function(){ cluster.addInstance(__sandbox_uri2); }, "Cluster.addInstance: group_replication_local_address is empty");
EXPECT_OUTPUT_CONTAINS("Unable to read Group Replication local address setting for instance '" + hostname + ":" + __mysql_sandbox_port2 + "', probably due to connectivity issues. Please retry the operation.");
testutil.dbugSet("");

EXPECT_NO_THROWS(function(){ cluster.addInstance(__sandbox_uri2); });
EXPECT_OUTPUT_CONTAINS("The instance '" + hostname + ":" + __mysql_sandbox_port2 + "' was successfully added to the cluster.");

//@<> Cleanup 
session.close();
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
