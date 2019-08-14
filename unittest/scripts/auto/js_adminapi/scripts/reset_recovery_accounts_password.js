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

var server_id1 = 11111;
var server_id2 = 22222;
var server_id3 = 33333;
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
c.resetRecoveryAccountsPassword({interactive:false});

//@WL#12776 An error is thrown if instance not online and the force option is not used and we we reply no to the interactive prompt
testutil.expectPrompt("Do you want to continue anyway (the recovery password for the instance will not be reset)? [y/N]: ", "n");
c.resetRecoveryAccountsPassword({interactive:true});

//@WL#12776 An error is thrown if instance not online and the force option is false (no prompts are shown in interactive mode because force option was already set).
c.resetRecoveryAccountsPassword({interactive:true, force:false});

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

c.resetRecoveryAccountsPassword({interactive:true, force:true});

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
session.close();
shell.connect(__sandbox_uri3);
session.runSql("START group_replication");
session.close();
shell.connect(__sandbox_uri1);
testutil.waitMemberState(__mysql_sandbox_port3, "ONLINE");

//@WL#12776 Warning is printed for the instance not online but no error is thrown if force option is used (no prompts are shown even in interactive mode because the force option was already set)
var old_auth_string_1 = get_rpl_user_auth_string(server_id1);
var old_auth_string_2 = get_rpl_user_auth_string(server_id2);
var old_auth_string_3 = get_rpl_user_auth_string(server_id3);

c.resetRecoveryAccountsPassword({interactive:true, force:true});

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
session.close();
shell.connect(__sandbox_uri2);
session.runSql("START group_replication");
session.close();
shell.connect(__sandbox_uri1);
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
session.runSql("CHANGE MASTER TO master_user='nonstandart', master_password='password123' FOR CHANNEL 'group_replication_recovery'");
session.close();
shell.connect(__sandbox_uri1);

// validate non standard Innodb cluster recovery user is working correctly
restart_gr_plugin(__mysql_sandbox_port2);
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");

//@WL#12776 An error is thrown if the any of the instances' recovery user was not created by InnoDB cluster.
c.resetRecoveryAccountsPassword();

//@<> WL#12776: Cleanup
c.disconnect();
session.close();
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
testutil.destroySandbox(__mysql_sandbox_port3);
