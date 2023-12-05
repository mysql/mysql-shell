function get_rpl_user_auth_string(server_id) {
    var result = session.runSql(
        "SELECT authentication_string FROM mysql.user " +
        "WHERE USER=\"mysql_innodb_cluster_" + server_id.toString()+"\"");
    var row = result.fetchOne();
    if (row == null) {
        testutil.fail("Query to get authentication_string returned no results");
    }
    return row[0];
}

function get_read_replica_user_auth_string(server_id) {
    var result = session.runSql(
        "SELECT authentication_string FROM mysql.user " +
        "WHERE USER=\"mysql_innodb_replica_" + server_id.toString()+"\"");
    var row = result.fetchOne();
    if (row == null) {
        testutil.fail("Query to get authentication_string returned no results");
    }
    return row[0];
}

var server_id1 = 11111;
var server_id2 = 22222;
var server_id3 = 33333;

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

//@<> Validate resetRecoveryAccountsPassword changes the passwords of the recovery_accounts when all instances online
// get the auth_strings before running the reset operation
var old_auth_string_1 = get_rpl_user_auth_string(server_id1);
var old_auth_string_2 = get_rpl_user_auth_string(server_id2);
var old_auth_string_3 = get_rpl_user_auth_string(server_id3);

c.resetRecoveryAccountsPassword();

var new_auth_string_1 = get_rpl_user_auth_string(server_id1);
var new_auth_string_2 = get_rpl_user_auth_string(server_id2);
var new_auth_string_3 = get_rpl_user_auth_string(server_id3);

EXPECT_NE(old_auth_string_1, new_auth_string_1);
EXPECT_NE(old_auth_string_2, new_auth_string_2);
EXPECT_NE(old_auth_string_3, new_auth_string_3);

// make sure the recovery credentials that were reset work correctly.
// Note: by restarting the gr plugin on the instances, if they are able to join the
// group again and become online we know the new recovery credentials work.
restart_gr_plugin(__mysql_sandbox_port1);
testutil.waitMemberState(__mysql_sandbox_port1, "ONLINE");
restart_gr_plugin(__mysql_sandbox_port2);
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");
restart_gr_plugin(__mysql_sandbox_port3);
testutil.waitMemberState(__mysql_sandbox_port3, "ONLINE");

//@<> Simulate the instance2 dropping the GR group
session.close();
shell.connect(__sandbox_uri2);
session.runSql("STOP group_replication");
session.close();
shell.connect(__sandbox_uri1);
testutil.waitMemberState(__mysql_sandbox_port2, "(MISSING)");

//@WL#12776 An error is thrown if instance not online and the force option is not used and we are not in interactive mode
c.resetRecoveryAccountsPassword();

//@WL#12776 An error is thrown if instance not online and the force option is not used and we we reply no to the interactive prompt
shell.options.useWizards=1;
testutil.expectPrompt("Do you want to continue anyway (the recovery password for the instance will not be reset)? [y/N]: ", "n");
c.resetRecoveryAccountsPassword();

//@WL#12776 An error is thrown if instance not online and the force option is false (no prompts are shown in interactive mode because force option was already set).
c.resetRecoveryAccountsPassword({force:false});
shell.options.useWizards=0;

//@<> Simulate the instance3 dropping the GR group
session.close();
shell.connect(__sandbox_uri3);
session.runSql("STOP group_replication");
session.close();
shell.connect(__sandbox_uri1);
testutil.waitMemberState(__mysql_sandbox_port3, "(MISSING)");

//@WL#12776 Warning is printed for all instances not online but no error is thrown if force option is used (no prompts are shown even in interactive mode because the force option was already set)
var old_auth_string_1 = get_rpl_user_auth_string(server_id1);
var old_auth_string_2 = get_rpl_user_auth_string(server_id2);
var old_auth_string_3 = get_rpl_user_auth_string(server_id3);

shell.options.useWizards=1;
c.resetRecoveryAccountsPassword({force:true});
shell.options.useWizards=0;

var new_auth_string_1 = get_rpl_user_auth_string(server_id1);
var new_auth_string_2 = get_rpl_user_auth_string(server_id2);
var new_auth_string_3 = get_rpl_user_auth_string(server_id3);

//@<> Validate resetRecoveryAccountsPassword changed only passwords of the recovery_accounts of the online instances (instance1)
EXPECT_NE(old_auth_string_1, new_auth_string_1);
EXPECT_EQ(old_auth_string_2, new_auth_string_2);
EXPECT_EQ(old_auth_string_3, new_auth_string_3);

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

//@WL#12776 Warning is printed for the instance not online but no error is thrown if force option is used (no prompts are shown even in interactive mode because the force option was already set)
var old_auth_string_1 = get_rpl_user_auth_string(server_id1);
var old_auth_string_2 = get_rpl_user_auth_string(server_id2);
var old_auth_string_3 = get_rpl_user_auth_string(server_id3);

shell.options.useWizards=1;
c.resetRecoveryAccountsPassword({force:true});
shell.options.useWizards=0;

var new_auth_string_1 = get_rpl_user_auth_string(server_id1);
var new_auth_string_2 = get_rpl_user_auth_string(server_id2);
var new_auth_string_3 = get_rpl_user_auth_string(server_id3);

//@<> Validate resetRecoveryAccountsPassword changed only passwords of the recovery_accounts of the online instances (instance1 and 3)
EXPECT_NE(old_auth_string_1, new_auth_string_1);
EXPECT_EQ(old_auth_string_2, new_auth_string_2);
EXPECT_NE(old_auth_string_3, new_auth_string_3);

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

//@<> Validate resetRecoveryAccountsPassword works after the instances are brought back online
var old_auth_string_1 = get_rpl_user_auth_string(server_id1);
var old_auth_string_2 = get_rpl_user_auth_string(server_id2);
var old_auth_string_3 = get_rpl_user_auth_string(server_id3);

c.resetRecoveryAccountsPassword();

var new_auth_string_1 = get_rpl_user_auth_string(server_id1);
var new_auth_string_2 = get_rpl_user_auth_string(server_id2);
var new_auth_string_3 = get_rpl_user_auth_string(server_id3);

EXPECT_NE(old_auth_string_1, new_auth_string_1);
EXPECT_NE(old_auth_string_2, new_auth_string_2);
EXPECT_NE(old_auth_string_3, new_auth_string_3);

// make sure the recovery credentials that were reset work correctly.
restart_gr_plugin(__mysql_sandbox_port1);
testutil.waitMemberState(__mysql_sandbox_port1, "ONLINE");
restart_gr_plugin(__mysql_sandbox_port2);
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");
restart_gr_plugin(__mysql_sandbox_port3);
testutil.waitMemberState(__mysql_sandbox_port3, "ONLINE");

//@<> The recovery credentials must also work with read-replicas {VER(>=8.0.23)}

// check if password for the replica is changed
WIPE_SHELL_LOG();

c.removeInstance(__sandbox_uri3);
c.addReplicaInstance(__sandbox_uri3);

var old_auth_string_3 = get_read_replica_user_auth_string(server_id3);

EXPECT_NO_THROWS(function(){ c.resetRecoveryAccountsPassword(); });
EXPECT_SHELL_LOG_CONTAINS(`Changing '${hostname}:${__mysql_sandbox_port3}''s recovery credentials`);

testutil.waitReadReplicaState(__mysql_sandbox_port3, "ONLINE");
testutil.waitMemberTransactions(__mysql_sandbox_port3, __mysql_sandbox_port1);

CHECK_READ_REPLICA(__sandbox_uri3, c, "primary", __endpoint1);
EXPECT_NE(old_auth_string_3, get_read_replica_user_auth_string(server_id3));

// password should change even of the replica is OFFLINE

var session3 = mysql.getSession(__sandbox_uri3);
session3.runSql("STOP REPLICA FOR CHANNEL 'read_replica_replication'");
testutil.waitReadReplicaState(__mysql_sandbox_port3, "OFFLINE");

old_auth_string_3 = get_read_replica_user_auth_string(server_id3);
EXPECT_NO_THROWS(function(){ c.resetRecoveryAccountsPassword(); });

session3.runSql("START REPLICA FOR CHANNEL 'read_replica_replication'");
testutil.waitReadReplicaState(__mysql_sandbox_port3, "ONLINE");
testutil.waitMemberTransactions(__mysql_sandbox_port3, __mysql_sandbox_port1);

CHECK_READ_REPLICA(__sandbox_uri3, c, "primary", __endpoint1);
EXPECT_NE(old_auth_string_3, get_read_replica_user_auth_string(server_id3));

session3.close();

// an error is thrown if instance not online and the force option is not used and we are not in interactive mode
testutil.killSandbox(__mysql_sandbox_port3);

WIPE_OUTPUT();
EXPECT_THROWS(function() {
    c.resetRecoveryAccountsPassword();
}, `Can't connect to MySQL server on '${hostname}:${__mysql_sandbox_port3}'`);

EXPECT_OUTPUT_CONTAINS(`Unable to connect to instance '${hostname}:${__mysql_sandbox_port3}'. Please, verify connection credentials and make sure the instance is available.`);

// an error must be thrown if the force option is not used and we we reply no to the interactive prompt
shell.options.useWizards=1;

testutil.expectPrompt("Do you want to continue anyway (the recovery password for the instance will not be reset)? [y/N]: ", "n");
EXPECT_THROWS(function() {
    c.resetRecoveryAccountsPassword();
}, `Can't connect to MySQL server on '${hostname}:${__mysql_sandbox_port3}'`);

// an error must be thrown if the force option is false (no prompts are shown in interactive mode because force option was already set).
EXPECT_THROWS(function() {
    c.resetRecoveryAccountsPassword({force:false});
}, `Can't connect to MySQL server on '${hostname}:${__mysql_sandbox_port3}'`);

// won't throw, but password won't be changed because instance isn't reachable
WIPE_STDOUT();
WIPE_SHELL_LOG();

EXPECT_NO_THROWS(function(){ c.resetRecoveryAccountsPassword({force:true}); });

EXPECT_SHELL_LOG_NOT_CONTAINS(`Changing '${hostname}:${__mysql_sandbox_port3}''s recovery credentials`);

EXPECT_OUTPUT_CONTAINS(`The recovery password of instance '${hostname}:${__mysql_sandbox_port3}' will not be reset because the instance is not reachable.`);
EXPECT_OUTPUT_CONTAINS(`WARNING: Not all recovery or replication account passwords were successfully reset, the following instance was skipped: '${hostname}:${__mysql_sandbox_port3}'. Bring this instance back online and run the <Cluster>.resetRecoveryAccountsPassword() operation again if you want to reset its recovery account password.`);

shell.options.useWizards=0;

// bring instance ONLINE and try again (must work)
testutil.startSandbox(__mysql_sandbox_port3);
testutil.waitReadReplicaState(__mysql_sandbox_port3, "ONLINE");
testutil.waitMemberTransactions(__mysql_sandbox_port3, __mysql_sandbox_port1);

shell.connect(__sandbox_uri1);

old_auth_string_3 = get_read_replica_user_auth_string(server_id3);

WIPE_STDOUT();
EXPECT_NO_THROWS(function(){ c.resetRecoveryAccountsPassword(); });
EXPECT_OUTPUT_CONTAINS("The recovery and replication account passwords of all the cluster instances' were successfully reset.");

EXPECT_NE(old_auth_string_3, get_read_replica_user_auth_string(server_id3));
CHECK_READ_REPLICA(__sandbox_uri3, c, "primary", __endpoint1);

// cleanup
c.removeInstance(__sandbox_uri3);
c.addInstance(__sandbox_uri3);

//@<> Make the recovery user of the MD schema invalid one, exception should be thrown BUG#32157182
shell.connect(__sandbox_uri2);
var uuid_2 = get_sysvar(session, "SERVER_UUID", "GLOBAL");
session.close();
shell.connect(__sandbox_uri1);
session.runSql("UPDATE mysql_innodb_cluster_metadata.instances SET attributes = json_set(COALESCE(attributes, '{}'),'$.recoveryAccountUser', '', '$.recoveryAccountHost', '') WHERE mysql_server_uuid = '" + uuid_2 + "'");
WIPE_STDOUT()
EXPECT_THROWS_TYPE(function() { c.resetRecoveryAccountsPassword(); }, "Cluster.resetRecoveryAccountsPassword: The replication recovery account in use by '<<<hostname>>>:<<<__mysql_sandbox_port2>>>' is not stored in the metadata. Use cluster.rescan() to update the metadata.", "MetadataError");
c.rescan();
var status = c.status();
EXPECT_FALSE("instanceErrors" in status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port2}`]);

//@<> Set up a recovery user on instance 2 whose format is different than the ones created by Innodb Cluster and check that an exception is thrown
// Note must be connected to the primary instance as other instances will be in super-read-only mode.
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

// validate non standard Innodb cluster recovery user is working correctly
restart_gr_plugin(__mysql_sandbox_port2);
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");

//<>@ WL#12776 An error is thrown if the any of the instances' recovery user was not created by InnoDB cluster.
WIPE_STDOUT()
EXPECT_THROWS_TYPE(function() { c.resetRecoveryAccountsPassword(); }, "Cluster.resetRecoveryAccountsPassword: Recovery user 'nonstandart' not created by InnoDB Cluster", "RuntimeError");
EXPECT_STDOUT_CONTAINS("ERROR: The recovery user name for instance '<<<hostname>>>:<<<__mysql_sandbox_port2>>>' does not match the expected format for users created automatically by InnoDB Cluster. Please remove and add the instance back to the Cluster to ensure a supported recovery account is used. Aborting password reset operation.")

//@<> WL#12776: Cleanup
c.disconnect();
session.close();
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
testutil.destroySandbox(__mysql_sandbox_port3);
