var server_id1 = 11111;
var server_id2 = 22222;
var server_id3 = 33333;

//@<> INCLUDE replication_accounts_utils.inc
//@<> INCLUDE read_replicas_utils.inc

//@<> WL#12776 Deploy instances.
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: hostname, server_id: server_id1});
testutil.deploySandbox(__mysql_sandbox_port2, "root", {report_host: hostname, server_id: server_id2});
testutil.deploySandbox(__mysql_sandbox_port3, "root", {report_host: hostname, server_id: server_id3});


//@<> WL#12776 create cluster
shell.connect(__sandbox_uri1);
var c = dba.createCluster("Cluster", {gtidSetIsComplete: true});
c.addInstance(__sandbox_uri2);
c.addInstance(__sandbox_uri3);
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");
testutil.waitMemberState(__mysql_sandbox_port3, "ONLINE");

//@<> Validate resetReplicationAccountsPassword changes the passwords of the recovery_accounts when all instances online
var before = snapshot_cluster_accounts([server_id1, server_id2, server_id3]);

WIPE_OUTPUT();
EXPECT_NO_THROWS(function() { c.resetReplicationAccountsPassword(); });

EXPECT_OUTPUT_CONTAINS(
  "The replication account passwords of all the Cluster instances were successfully reset.");

var after = snapshot_cluster_accounts([server_id1, server_id2, server_id3]);
expect_all_changed(before, after);

//@<> Validate resetReplicationAccountsPassword(recreate:true) recreates Cluster recovery accounts
shell.connect(__sandbox_uri1);
var expected_users = session.runSql(
  "select group_concat(concat(user,'@',host) order by user) " +
  "from mysql.user where user like 'mysql_innodb_cluster_%'"
).fetchOne()[0];

session.runSql(
  "DROP USER " +
  "'mysql_innodb_cluster_11111'@'%', " +
  "'mysql_innodb_cluster_22222'@'%', " +
  "'mysql_innodb_cluster_33333'@'%'"
);

EXPECT_EQ(null, session.runSql(
  "select group_concat(concat(user,'@',host) order by user) " +
  "from mysql.user where user like 'mysql_innodb_cluster_%'"
).fetchOne()[0]);

WIPE_OUTPUT();
EXPECT_NO_THROWS(function() { c.resetReplicationAccountsPassword({recreate:true}); });

EXPECT_OUTPUT_CONTAINS(
  "The replication account passwords of all the Cluster instances were successfully recreated.");

shell.connect(__sandbox_uri1);
EXPECT_EQ(expected_users, session.runSql(
  "select group_concat(concat(user,'@',host) order by user) " +
  "from mysql.user where user like 'mysql_innodb_cluster_%'"
).fetchOne()[0]);
EXPECT_EQ("caching_sha2_password", session.runSql(
  "select plugin from mysql.user where user='mysql_innodb_cluster_11111' and host='%'"
).fetchOne()[0]);

shell.connect(__sandbox_uri2);
EXPECT_EQ("mysql_innodb_cluster_22222", session.runSql(
  "select user_name from mysql.slave_master_info where channel_name='group_replication_recovery'"
).fetchOne()[0]);

shell.connect(__sandbox_uri3);
EXPECT_EQ("mysql_innodb_cluster_33333", session.runSql(
  "select user_name from mysql.slave_master_info where channel_name='group_replication_recovery'"
).fetchOne()[0]);

shell.connect(__sandbox_uri1);

// make sure the recovery credentials that were reset work correctly.
// Note: by restarting the gr plugin on the instances, if they are able to join the
// group again and become online we know the new recovery credentials work.
restart_gr_plugin(__mysql_sandbox_port1);
testutil.waitMemberState(__mysql_sandbox_port1, "ONLINE");
restart_gr_plugin(__mysql_sandbox_port2);
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");
restart_gr_plugin(__mysql_sandbox_port3);
testutil.waitMemberState(__mysql_sandbox_port3, "ONLINE");

shell.connectToPrimary();
//@<> Deprecated resetRecoveryAccountsPassword behaves the same (but prints deprecation message)
var before = snapshot_cluster_accounts([server_id1, server_id2, server_id3]);

WIPE_OUTPUT();
EXPECT_NO_THROWS(function() { c.resetRecoveryAccountsPassword(); });

// Deprecation message
EXPECT_OUTPUT_CONTAINS(
  "WARNING: This function is deprecated and will be removed in a future release of MySQL Shell. Use <Cluster>.resetReplicationAccountsPassword() instead.");

// It still performs the same operation (passwords change)
var after = snapshot_cluster_accounts([server_id1, server_id2, server_id3]);
expect_all_changed(before, after);

// same success line
EXPECT_OUTPUT_CONTAINS(
  "The replication account passwords of all the Cluster instances were successfully reset.");

//@<> Simulate the instance2 dropping the GR group
session.close();
shell.connect(__sandbox_uri2);
session.runSql("STOP group_replication");
session.close();
shell.connect(__sandbox_uri1);
testutil.waitMemberState(__mysql_sandbox_port2, "(MISSING)");

//@<> WL#12776 An error is thrown if instance not online and the force option is not used and we are not in interactive mode
EXPECT_THROWS(function() { c.resetReplicationAccountsPassword(); },
  `The instance '${hostname}:${__mysql_sandbox_port2}' is '(MISSING)' (it must be ONLINE).`);

EXPECT_OUTPUT_CONTAINS(`ERROR: The replication account passwords for instance '${hostname}:${__mysql_sandbox_port2}' cannot be reset because it is in a '(MISSING)' state. Replication account passwords can only be rotated for ONLINE instances. Ensure the instance is reachable and use <Cluster>.rejoinInstance() to rejoin it and refresh its internal replication account credentials. You can also run <Cluster>.resetReplicationAccountsPassword() with the force option enabled to skip instances that are not ONLINE.`);

//@<> WL#12776 An error is thrown if instance not online and the force option is not used and we reply no to the interactive prompt
shell.options.useWizards=1;
testutil.expectPrompt("Do you want to continue anyway (the replication account passwords for the instance will not be reset)? [y/N]: ", "n");
EXPECT_THROWS(function() { c.resetReplicationAccountsPassword(); },
  `The instance '${hostname}:${__mysql_sandbox_port2}' is '(MISSING)' (it must be ONLINE).`);

EXPECT_OUTPUT_CONTAINS(`ERROR: The replication account passwords for instance '${hostname}:${__mysql_sandbox_port2}' cannot be reset because it is in a '(MISSING)' state. Replication account passwords can only be rotated for ONLINE instances. Ensure the instance is reachable and use <Cluster>.rejoinInstance() to rejoin it and refresh its internal replication account credentials. You can choose to proceed with the operation and skip resetting this instance's replication account passwords.`);

//@<> WL#12776 An error is thrown if instance not online and the force option is false (no prompts are shown in interactive mode because force option was already set).
EXPECT_THROWS(function() { c.resetReplicationAccountsPassword({force:false}); },
  `The instance '${hostname}:${__mysql_sandbox_port2}' is '(MISSING)' (it must be ONLINE).`);
EXPECT_OUTPUT_CONTAINS(`ERROR: The replication account passwords for instance '${hostname}:${__mysql_sandbox_port2}' cannot be reset because it is in a '(MISSING)' state. Replication account passwords can only be rotated for ONLINE instances. Ensure the instance is reachable and use <Cluster>.rejoinInstance() to rejoin it and refresh its internal replication account credentials. You can also run <Cluster>.resetReplicationAccountsPassword() with the force option enabled to skip instances that are not ONLINE.`);

shell.options.useWizards=0;

//@<> Simulate the instance3 dropping the GR group
session.close();
shell.connect(__sandbox_uri3);
session.runSql("STOP group_replication");
session.close();
shell.connect(__sandbox_uri1);
testutil.waitMemberState(__mysql_sandbox_port3, "(MISSING)");

//@<> WL#12776 Warning is printed for all instances not online but no error is thrown if force option is used
var before = snapshot_cluster_accounts([server_id1, server_id2, server_id3]);

shell.options.useWizards=1;
EXPECT_NO_THROWS(function() { c.resetReplicationAccountsPassword({force:true});});
shell.options.useWizards=0;

var after = snapshot_cluster_accounts([server_id1, server_id2, server_id3]);

EXPECT_OUTPUT_CONTAINS(`NOTE: Skipping reset of the replication account passwords for instance '${hostname}:${__mysql_sandbox_port2}' because it is '(MISSING)'.`);
EXPECT_OUTPUT_CONTAINS(`NOTE: Skipping reset of the replication account passwords for instance '${hostname}:${__mysql_sandbox_port3}' because it is '(MISSING)'.`);
EXPECT_OUTPUT_CONTAINS(`WARNING: Not all replication account passwords were successfully reset, the following instances were skipped: '${hostname}:${__mysql_sandbox_port2}', '${hostname}:${__mysql_sandbox_port3}'. Ensure these instances are reachable and use <Cluster>.rejoinInstance() to rejoin them and refresh their internal replication account credentials.`);

// Validate only ONLINE instance changed (instance1)
expect_changed_mask(before, after, [true, false, false]);

// make sure all members whose recovery account password was changed remain online
testutil.waitMemberState(__mysql_sandbox_port1, "ONLINE");

//@<> Bring Instance 3 online
if (__version_num < 80027) {
  session.close();
  shell.connect(__sandbox_uri3);
  session.runSql("START group_replication");
  session.close();
  shell.connect(__sandbox_uri1);
} else {
  c.rejoinInstance(__sandbox_uri3);
}

testutil.waitMemberState(__mysql_sandbox_port3, "ONLINE");

//@<> WL#12776 Warning is printed for the instance not online but no error is thrown if force option is used
var before = snapshot_cluster_accounts([server_id1, server_id2, server_id3]);

shell.options.useWizards=1;
EXPECT_NO_THROWS(function() { c.resetReplicationAccountsPassword({force:true});});
shell.options.useWizards=0;

var after = snapshot_cluster_accounts([server_id1, server_id2, server_id3]);

EXPECT_OUTPUT_CONTAINS(`NOTE: Skipping reset of the replication account passwords for instance '${hostname}:${__mysql_sandbox_port2}' because it is '(MISSING)'.`);
EXPECT_OUTPUT_CONTAINS(`WARNING: Not all replication account passwords were successfully reset, the following instance was skipped: '${hostname}:${__mysql_sandbox_port2}'. Ensure this instance is reachable and use <Cluster>.rejoinInstance() to rejoin it and refresh its internal replication account credentials.`);

// Validate only ONLINE instances changed (instance1 and 3)
expect_changed_mask(before, after, [true, false, true]);

// make sure the recovery credentials that were reset work correctly.
restart_gr_plugin(__mysql_sandbox_port1);
testutil.waitMemberState(__mysql_sandbox_port1, "ONLINE");
restart_gr_plugin(__mysql_sandbox_port3);
testutil.waitMemberState(__mysql_sandbox_port3, "ONLINE");

//@<> Bring Instance 2 online
if (__version_num < 80027) {
  session.close();
  shell.connect(__sandbox_uri2);
  session.runSql("START group_replication");
  session.close();
  shell.connect(__sandbox_uri1);
} else {
  c.rejoinInstance(__sandbox_uri2);
}

testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");

//@<> Validate resetReplicationAccountsPassword works after the instances are brought back online
var before = snapshot_cluster_accounts([server_id1, server_id2, server_id3]);

EXPECT_NO_THROWS(function() { c.resetReplicationAccountsPassword(); });

var after = snapshot_cluster_accounts([server_id1, server_id2, server_id3]);
expect_all_changed(before, after);

// make sure the recovery credentials that were reset work correctly.
restart_gr_plugin(__mysql_sandbox_port1);
testutil.waitMemberState(__mysql_sandbox_port1, "ONLINE");
restart_gr_plugin(__mysql_sandbox_port2);
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");
restart_gr_plugin(__mysql_sandbox_port3);
testutil.waitMemberState(__mysql_sandbox_port3, "ONLINE");

//@<> The recovery credentials must also work with read-replicas {VER(>=8.0.23)}
WIPE_SHELL_LOG();

c.removeInstance(__sandbox_uri3);
c.addReplicaInstance(__sandbox_uri3);

//@<> Validate resetReplicationAccountsPassword(recreate:true) recreates read-replica accounts {VER(>=8.0.23)}
shell.connect(__sandbox_uri1);
var expected_rr_users = session.runSql(
  "select group_concat(concat(user,'@',host) order by user) " +
  "from mysql.user where user like 'mysql_innodb_replica_%'"
).fetchOne()[0];

session.runSql("DROP USER 'mysql_innodb_replica_33333'@'%'");

EXPECT_EQ(null, session.runSql(
  "select group_concat(concat(user,'@',host) order by user) " +
  "from mysql.user where user like 'mysql_innodb_replica_%'"
).fetchOne()[0]);

WIPE_OUTPUT();
EXPECT_NO_THROWS(function(){ c.resetReplicationAccountsPassword({recreate:true}); });

EXPECT_OUTPUT_CONTAINS("The replication account passwords of all the Cluster instances were successfully recreated.");
EXPECT_OUTPUT_CONTAINS(`* Updating replication credentials on '${hostname}:${__mysql_sandbox_port3}' (channel: read_replica_replication). The replication receiver will be temporarily stopped and restarted.`);

shell.connect(__sandbox_uri1);
EXPECT_EQ(expected_rr_users, session.runSql(
  "select group_concat(concat(user,'@',host) order by user) " +
  "from mysql.user where user like 'mysql_innodb_replica_%'"
).fetchOne()[0]);
EXPECT_EQ("caching_sha2_password", session.runSql(
  "select plugin from mysql.user where user='mysql_innodb_replica_33333' and host='%'"
).fetchOne()[0]);

shell.connect(__sandbox_uri3);
EXPECT_EQ("mysql_innodb_replica_33333", session.runSql(
  "select user_name from mysql.slave_master_info where channel_name='read_replica_replication'"
).fetchOne()[0]);

shell.connect(__sandbox_uri1);
testutil.waitReadReplicaState(__mysql_sandbox_port3, "ONLINE");
testutil.waitMemberTransactions(__mysql_sandbox_port3, __mysql_sandbox_port1);

CHECK_READ_REPLICA(__sandbox_uri3, c, "primary", __endpoint1);

var old_auth_string_3 = snapshot_read_replica_accounts([server_id3])[0];

EXPECT_NO_THROWS(function(){ c.resetReplicationAccountsPassword(); });
EXPECT_SHELL_LOG_CONTAINS(`Updating replication credentials on '${hostname}:${__mysql_sandbox_port3}' (channel: read_replica_replication)`);
EXPECT_OUTPUT_CONTAINS(`* Updating replication credentials on '${hostname}:${__mysql_sandbox_port3}' (channel: read_replica_replication). The replication receiver will be temporarily stopped and restarted.`);

testutil.waitReadReplicaState(__mysql_sandbox_port3, "ONLINE");
testutil.waitMemberTransactions(__mysql_sandbox_port3, __mysql_sandbox_port1);

CHECK_READ_REPLICA(__sandbox_uri3, c, "primary", __endpoint1);
EXPECT_NE(old_auth_string_3, snapshot_read_replica_accounts([server_id3])[0]);

// password should change even if the replica is OFFLINE
var session3 = mysql.getSession(__sandbox_uri3);
session3.runSql("STOP REPLICA FOR CHANNEL 'read_replica_replication'");
testutil.waitReadReplicaState(__mysql_sandbox_port3, "OFFLINE");

old_auth_string_3 = snapshot_read_replica_accounts([server_id3])[0];
EXPECT_NO_THROWS(function(){ c.resetReplicationAccountsPassword(); });

session3.runSql("START REPLICA FOR CHANNEL 'read_replica_replication'");
testutil.waitReadReplicaState(__mysql_sandbox_port3, "ONLINE");
testutil.waitMemberTransactions(__mysql_sandbox_port3, __mysql_sandbox_port1);

CHECK_READ_REPLICA(__sandbox_uri3, c, "primary", __endpoint1);
EXPECT_NE(old_auth_string_3, snapshot_read_replica_accounts([server_id3])[0]);

session3.close();

// an error is thrown if instance not reachable and force not used (non-interactive)
testutil.killSandbox(__mysql_sandbox_port3);

WIPE_OUTPUT();
EXPECT_THROWS(function() {
    c.resetReplicationAccountsPassword();
}, `Can't connect to MySQL server on '${hostname}:${__mysql_sandbox_port3}'`);

EXPECT_OUTPUT_CONTAINS(`ERROR: Unable to connect to instance '${hostname}:${__mysql_sandbox_port3}'. Please verify connection credentials and make sure the instance is available.`);

// an error must be thrown if the force option is not used and we we reply no to the interactive prompt
shell.options.useWizards=1;

testutil.expectPrompt("Do you want to continue anyway (the replication account passwords for the instance will not be reset)? [y/N]: ", "n");
EXPECT_THROWS(function() {
    c.resetReplicationAccountsPassword();
}, `Can't connect to MySQL server on '${hostname}:${__mysql_sandbox_port3}'`);

// an error must be thrown if the force option is false (no prompts are shown in interactive mode because force option was already set).
EXPECT_THROWS(function() {
    c.resetReplicationAccountsPassword({force:false});
}, `Can't connect to MySQL server on '${hostname}:${__mysql_sandbox_port3}'`);

// force:true -> skip
WIPE_STDOUT();
WIPE_SHELL_LOG();

EXPECT_NO_THROWS(function(){ c.resetReplicationAccountsPassword({force:true}); });

EXPECT_SHELL_LOG_NOT_CONTAINS(`Updating replication credentials on '${hostname}:${__mysql_sandbox_port3}' (channel: group_replication_recovery)`);

EXPECT_OUTPUT_CONTAINS(`NOTE: The replication account passwords for instance '${hostname}:${__mysql_sandbox_port3}' will not be reset because the instance is not reachable.`);

EXPECT_OUTPUT_CONTAINS(`WARNING: Not all replication account passwords were successfully reset, the following instance was skipped: '${hostname}:${__mysql_sandbox_port3}'. Ensure this instance is reachable and use <Cluster>.rejoinInstance() to rejoin it and refresh its internal replication account credentials.`);

shell.options.useWizards=0;

// bring instance ONLINE and try again (must work)
testutil.startSandbox(__mysql_sandbox_port3);
testutil.waitReadReplicaState(__mysql_sandbox_port3, "ONLINE");
testutil.waitMemberTransactions(__mysql_sandbox_port3, __mysql_sandbox_port1);

shell.connect(__sandbox_uri1);

old_auth_string_3 = snapshot_read_replica_accounts([server_id3])[0];

WIPE_STDOUT();
EXPECT_NO_THROWS(function(){ c.resetReplicationAccountsPassword(); });
EXPECT_OUTPUT_CONTAINS("The replication account passwords of all the Cluster instances were successfully reset.");

EXPECT_NE(old_auth_string_3, snapshot_read_replica_accounts([server_id3])[0]);
CHECK_READ_REPLICA(__sandbox_uri3, c, "primary", __endpoint1);

// cleanup read-replica
c.removeInstance(__sandbox_uri3);
c.addInstance(__sandbox_uri3);

//@<> Make the recovery user of the MD schema invalid one, exception should be thrown BUG#32157182
shell.connect(__sandbox_uri2);
var uuid_2 = get_sysvar(session, "SERVER_UUID", "GLOBAL");
session.close();
shell.connect(__sandbox_uri1);
session.runSql("UPDATE mysql_innodb_cluster_metadata.instances SET attributes = json_set(COALESCE(attributes, '{}'),'$.recoveryAccountUser', '', '$.recoveryAccountHost', '') WHERE mysql_server_uuid = '" + uuid_2 + "'");
WIPE_STDOUT()
EXPECT_THROWS_TYPE(function() { c.resetReplicationAccountsPassword(); }, "The replication recovery account in use by '<<<hostname>>>:<<<__mysql_sandbox_port2>>>' is not stored in the metadata. Use cluster.rescan() to update the metadata.", "MetadataError");
c.rescan();
var status = c.status();
EXPECT_FALSE("instanceErrors" in status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port2}`]);

//@<> Set up a recovery user on instance 2 whose format is different and check exception
shell.connect(__sandbox_uri2);
var uuid_2 = get_sysvar(session, "SERVER_UUID", "GLOBAL");
session.close();
shell.connect(__sandbox_uri1);
// delete information from metadata to simulate user created recovery account.
session.runSql("UPDATE mysql_innodb_cluster_metadata.instances SET attributes = json_set(COALESCE(attributes, '{}'),'$.recoveryAccountUser', '', '$.recoveryAccountHost', '') WHERE mysql_server_uuid = '" + uuid_2 + "'");
session.runSql("RENAME USER \'mysql_innodb_cluster_"+ server_id2.toString()+"'@'%' to 'nonstandart'@'%'");
session.runSql("ALTER USER 'nonstandart'@'%' IDENTIFIED BY 'password123'");

shell.connect(__sandbox_uri2);
session.runSql("change " + get_replication_source_keyword() + " TO " + get_replication_option_keyword() + "_USER='nonstandart', " + get_replication_option_keyword() + "_PASSWORD='password123' FOR CHANNEL 'group_replication_recovery'");
session.close();
shell.connect(__sandbox_uri1);

// validate non standard recovery user is working
restart_gr_plugin(__mysql_sandbox_port2);
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");

//<>@ WL#12776 An error is thrown if the any of the instances' recovery user was not created by InnoDB cluster.
WIPE_STDOUT()
EXPECT_THROWS(function() { c.resetReplicationAccountsPassword(); }, "Recovery user 'nonstandart' not created by InnoDB Cluster");
EXPECT_STDOUT_CONTAINS("ERROR: The replication account name for instance '<<<hostname>>>:<<<__mysql_sandbox_port2>>>' does not match the expected format for accounts created automatically by InnoDB Cluster. Please use <Cluster>.rejoinInstance() to ensure a supported replication account is used. Aborting password reset operation.")
EXPECT_SHELL_LOG_CONTAINS("Failed to get replication account for instance '<<<hostname>>>:<<<__mysql_sandbox_port2>>>':")

//@<> WL#12776: Cleanup
c.disconnect();
session.close();
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
testutil.destroySandbox(__mysql_sandbox_port3);
