function get_all_users(session) {
  var result = session.runSql(
    "SELECT concat(user,'@',host) FROM mysql.user " +
    "WHERE user NOT REGEXP 'mysql_innodb_cluster_r[0-9]{10}.*'");
  return result.fetchAll();
}

function get_recovery_account(session) {
  var server_uuid = session.runSql("SELECT @@server_uuid").fetchOne()[0];

  var recovery_account = session.runSql("select user from performance_schema.replication_connection_status s join performance_schema.replication_connection_configuration c on s.channel_name = c.channel_name where s.channel_name='group_replication_recovery'").fetchOne()[0];

  var md_recovery_account = session.runSql("select (attributes->>'$.recoveryAccountUser') FROM mysql_innodb_cluster_metadata.instances WHERE mysql_server_uuid = '" + server_uuid + "'").fetchOne()[0];

  return recovery_account;
}

function get_all_recovery_accounts(session) {
  var all_users = session.runSql("select user, host from mysql.user where user like 'mysql_innodb_cluster_%' order by user").fetchAll();

  var just_users = [];
  for (row of all_users) just_users.push(row[0]);

  return just_users;
}

// BUG#29617572: DBA.REMOVEINSTANCE() WITH EMPTY MYSQL.SLAVE_MASTER_INFO DROPS ALL USERS
//
// Removing an instance from a cluster when the instance to be removed has no
// user defined for channel group_replication_recovery in
// mysql.slave_master_info results in dropping ALL users from mysql.user on the
// remaining instances of the cluster.

//@<> Initialization
var scene = new ClusterScenario([__mysql_sandbox_port1]);
var session = scene.session
var c = scene.cluster
var all_users_instance1 = get_all_users(session);

//@<> Add instance to the cluster
testutil.deploySandbox(__mysql_sandbox_port2, 'root', {report_host: hostname});
c.addInstance(__sandbox_uri2);
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");

//@<> Remove instance should always update group_replication_group_seeds
// Bug #31531704	REMOVING UNREACHABLE MEMBER DOESN'T UPDATE GROUP_REPLICATION_GROUP_SEEDS

s2 = mysql.getSession(__sandbox_uri2);
if (__version_num >= 80017)
  s2.runSql("set persist group_replication_start_on_boot = 0");

testutil.stopSandbox(__mysql_sandbox_port2);

EXPECT_EQ(hostname+":"+__mysql_sandbox_port2+"1", session.runSql("SELECT @@group_replication_group_seeds").fetchOne()[0]);

c.removeInstance(hostname+":"+__mysql_sandbox_port2, {force:1});
EXPECT_EQ("", session.runSql("SELECT @@group_replication_group_seeds").fetchOne()[0]);

testutil.startSandbox(__mysql_sandbox_port2);

c.addInstance(__sandbox_uri2);

//@<> Remove instance with an unresolvable hostname
// Bug #31632606  REMOVEINSTANCE() WITH FORCE DOESN'T WORK IF ADDRESS IS NO LONGER RESOLVABLE

// hack the metadata adding an invalid hostname to simulate an instance that was added
// but is no longer resolvable at the time it's removed
var cluster_id = session.runSql("select cluster_id from mysql_innodb_cluster_metadata.clusters limit 1").fetchOne()[0];
session.runSql("insert into mysql_innodb_cluster_metadata.instances (cluster_id, mysql_server_uuid, address, instance_name, addresses, attributes) values (?, 'aaaaaaaa-aaaa-aaaa-aaaa-569f1ba7f3cf', 'badhost.baddomain:1234', 'badhost.baddomain:1234', json_object('mysqlClassic', 'badhost.baddomain:1234', 'mysqlX', 'badhost.baddomain:12340', 'grLocal', 'badhost.baddomain:12341'), '{}')", [cluster_id]);

EXPECT_EQ(3, session.runSql("select * from mysql_innodb_cluster_metadata.instances").fetchAll().length);

c.removeInstance("badhost.baddomain:1234", {force:true});

EXPECT_OUTPUT_CONTAINS("NOTE: The instance 'badhost.baddomain:1234' is not reachable and it will only be removed from the metadata. Please take any necessary actions to ensure the instance will not rejoin the cluster if brought back online.");
EXPECT_OUTPUT_CONTAINS("The instance 'badhost.baddomain:1234' was successfully removed from the cluster.");

EXPECT_EQ(2, session.runSql("select * from mysql_innodb_cluster_metadata.instances").fetchAll().length);

//@<> After being removed, the instance has no information regarding the group_replication_applier channel BUG#30878446
var session2 = mysql.getSession(__sandbox_uri2);
EXPECT_NE(0, session2.runSql("select COUNT(*) from performance_schema.replication_applier_status").fetchOne()[0]);
c.removeInstance(__sandbox_uri2);
EXPECT_EQ(0, session2.runSql("select COUNT(*) from performance_schema.replication_applier_status").fetchOne()[0]);
session2.close();

//@<> Add instance back to the cluster
c.addInstance(__sandbox_uri2);
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");

//@<> BUG#29617572: Clear the mysql.slave_master_info table on instance 2
session.close();
shell.connect(__sandbox_uri2);
session.runSql('STOP GROUP_REPLICATION');
session.runSql('RESET SLAVE ALL');

c.removeInstance(__sandbox_uri2, {force:true});

session1 = mysql.getSession(__sandbox_uri1);
EXPECT_EQ(1, session1.runSql("SELECT count(*) FROM mysql_innodb_cluster_metadata.instances").fetchOne()[0]);

//@<> BUG#29617572: Verify that no extra users were removed from instance 1
session.close();
shell.connect(__sandbox_uri1);
EXPECT_EQ(all_users_instance1, get_all_users(session));

// ------------------------

//@<> removeInstance() while instance is up through hostname (same as in MD and report_host)
c = dba.getCluster();
c.addInstance(__sandbox_uri2);

EXPECT_NO_THROWS(function() {c.removeInstance(hostname+":"+__mysql_sandbox_port2); });
EXPECT_EQ(1, session1.runSql("SELECT count(*) FROM mysql_innodb_cluster_metadata.instances").fetchOne()[0]);

//@<> removeInstance() while instance is up through hostname_ip
c.addInstance(__sandbox_uri2);

EXPECT_NO_THROWS(function() {c.removeInstance(hostname_ip+":"+__mysql_sandbox_port2); });
EXPECT_EQ(1, session1.runSql("SELECT count(*) FROM mysql_innodb_cluster_metadata.instances").fetchOne()[0]);

//@<> removeInstance() while instance is up through localhost
c.addInstance(__sandbox_uri2);

EXPECT_NO_THROWS(function() {c.removeInstance("localhost:"+__mysql_sandbox_port2); });
EXPECT_EQ(1, session1.runSql("SELECT count(*) FROM mysql_innodb_cluster_metadata.instances").fetchOne()[0]);

// ------------------------

// Note:
// "wrong address" = address that can reach the instance but is not what's in the metadata
// "correct address" = the address that the instance is known by in the metadata

//@<> removeInstance() while the instance is down - no force and wrong address (should fail)
// covers Bug #30625424	REMOVEINSTANCE() FORCE:TRUE SAYS INSTANCE REMOVED, BUT NOT REALLY
c.addInstance(__sandbox_uri2);

testutil.stopSandbox(__mysql_sandbox_port2);
testutil.waitMemberState(__mysql_sandbox_port2, "UNREACHABLE,(MISSING)");
EXPECT_THROWS(function() { c.removeInstance(__sandbox_uri2); }, "Metadata for instance <<<__sandbox2>>> not found", "MYSQLSH");
EXPECT_OUTPUT_CONTAINS_ONE_OF(["WARNING: MySQL Error 2003 (HY000): Can't connect to MySQL server on '<<<libmysql_host_description('localhost', __mysql_sandbox_port2)>>>'","WARNING: MySQL Error 2013 (HY000): Lost connection to MySQL server at 'reading initial communication packet', system error:"]);
EXPECT_OUTPUT_CONTAINS("ERROR: The instance <<<__sandbox2>>> is not reachable and does not belong to the cluster either. Please ensure the member is either connectable or remove it through the exact address as shown in the cluster status output.");

//@<> removeInstance() while the instance is down - force and wrong address (should fail)
EXPECT_THROWS(function() { c.removeInstance(__sandbox_uri2, {force:1}); }, "Metadata for instance <<<__sandbox2>>> not found", "MYSQLSH");
EXPECT_OUTPUT_CONTAINS("NOTE: MySQL Error 20");
EXPECT_OUTPUT_CONTAINS("ERROR: The instance localhost:<<<__mysql_sandbox_port2>>> is not reachable and does not belong to the cluster either. Please ensure the member is either connectable or remove it through the exact address as shown in the cluster status output.");

EXPECT_EQ(2, session1.runSql("SELECT count(*) FROM mysql_innodb_cluster_metadata.instances").fetchOne()[0]);

//@<> removeInstance() while the instance is down - no force and correct address (should fail)
EXPECT_THROWS(function(){ c.removeInstance(hostname+":"+__mysql_sandbox_port2); }, `Can't connect to MySQL server on '${libmysql_host_description(hostname, __mysql_sandbox_port2)}'`);
EXPECT_OUTPUT_CONTAINS("ERROR: The instance '<<<hostname>>>:<<<__mysql_sandbox_port2>>>' is not reachable and cannot be safely removed from the cluster.");
EXPECT_OUTPUT_CONTAINS("To safely remove the instance from the cluster, make sure the instance is back ONLINE and try again. If you are sure the instance is permanently unable to rejoin the group and no longer connectable, use the 'force' option to remove it from the metadata.");

EXPECT_EQ(2, session1.runSql("SELECT count(*) FROM mysql_innodb_cluster_metadata.instances").fetchOne()[0]);

//@<> removeInstance() while the instance is down - force and correct address
EXPECT_NO_THROWS(function() { c.removeInstance(hostname+":"+__mysql_sandbox_port2, {force:1});});
EXPECT_OUTPUT_CONTAINS("NOTE: MySQL Error 20");
EXPECT_OUTPUT_CONTAINS("NOTE: The instance '<<<hostname>>>:<<<__mysql_sandbox_port2>>>' is not reachable and it will only be removed from the metadata. Please take any necessary actions to ensure the instance will not rejoin the cluster if brought back online.");
EXPECT_OUTPUT_CONTAINS("The instance '<<<hostname>>>:<<<__mysql_sandbox_port2>>>' was successfully removed from the cluster.");

EXPECT_EQ(1, session1.runSql("SELECT count(*) FROM mysql_innodb_cluster_metadata.instances").fetchOne()[0]);

//@<> bring back the stopped instance for the remaining tests
testutil.startSandbox(__mysql_sandbox_port2);
session2 = mysql.getSession(__sandbox_uri2);
// abort start_on_boot from the previous test if still active
session2.runSql("STOP GROUP_REPLICATION");

// ------------------------

//@<> removeInstance() while the instance is up but OFFLINE - no force and wrong address (should fail)
// covers Bug #30625424	REMOVEINSTANCE() FORCE:TRUE SAYS INSTANCE REMOVED, BUT NOT REALLY (should fail)
c.addInstance(__sandbox_uri2);
session2.runSql("STOP GROUP_REPLICATION");

EXPECT_THROWS(function(){ c.removeInstance(__sandbox_uri2); }, "Instance is not ONLINE and cannot be safely removed", "MYSQLSH");
EXPECT_OUTPUT_CONTAINS("ERROR: <<<hostname>>>:<<<__mysql_sandbox_port2>>> is reachable but has state OFFLINE");
EXPECT_OUTPUT_CONTAINS("To safely remove it from the cluster, it must be brought back ONLINE. If not possible, use the 'force' option to remove it anyway.");

EXPECT_EQ(2, session1.runSql("SELECT count(*) FROM mysql_innodb_cluster_metadata.instances").fetchOne()[0]);

//@<> removeInstance() while the instance is up but OFFLINE - no force and correct address (should fail)
EXPECT_THROWS(function(){ c.removeInstance(hostname+":"+__mysql_sandbox_port2); }, "Instance is not ONLINE and cannot be safely removed", "MYSQLSH");
EXPECT_OUTPUT_CONTAINS("ERROR: <<<hostname>>>:<<<__mysql_sandbox_port2>>> is reachable but has state OFFLINE");
EXPECT_OUTPUT_CONTAINS("To safely remove it from the cluster, it must be brought back ONLINE. If not possible, use the 'force' option to remove it anyway.");

EXPECT_EQ(2, session1.runSql("SELECT count(*) FROM mysql_innodb_cluster_metadata.instances").fetchOne()[0]);

//@<> removeInstance() while the instance is up but OFFLINE - force and wrong address
EXPECT_NO_THROWS(function() { c.removeInstance(__sandbox_uri2, {force:1}); });
EXPECT_OUTPUT_CONTAINS("NOTE: <<<hostname>>>:<<<__mysql_sandbox_port2>>> is reachable but has state OFFLINE");
EXPECT_OUTPUT_CONTAINS("The instance 'localhost:<<<__mysql_sandbox_port2>>>' was successfully removed from the cluster.");

EXPECT_EQ(1, session1.runSql("SELECT count(*) FROM mysql_innodb_cluster_metadata.instances").fetchOne()[0]);

//@<> removeInstance() while the instance is up but OFFLINE - force and correct address
c.addInstance(__sandbox_uri2);
session2.runSql("STOP GROUP_REPLICATION");

EXPECT_NO_THROWS(function() { c.removeInstance(hostname+":"+__mysql_sandbox_port2, {force:1}); });
EXPECT_OUTPUT_CONTAINS("NOTE: <<<hostname>>>:<<<__mysql_sandbox_port2>>> is reachable but has state OFFLINE");
EXPECT_OUTPUT_CONTAINS("The instance '<<<hostname>>>:<<<__mysql_sandbox_port2>>>' was successfully removed from the cluster.");

EXPECT_EQ(1, session1.runSql("SELECT count(*) FROM mysql_innodb_cluster_metadata.instances").fetchOne()[0]);

//@<> removeInstance() - OFFLINE, no force, interactive
c.addInstance(__sandbox_uri2);
session2.runSql("STOP GROUP_REPLICATION");

// prompt whether to remove without sync should appear
testutil.expectPrompt("Do you want to continue anyway (only the instance metadata will be removed)?", "n");

EXPECT_THROWS(function() { c.removeInstance(hostname+":"+__mysql_sandbox_port2, {interactive:1}); }, "Instance is not ONLINE and cannot be safely removed", "MYSQLSH");
EXPECT_OUTPUT_CONTAINS("ERROR: <<<hostname>>>:<<<__mysql_sandbox_port2>>> is reachable but has state OFFLINE");

//@<> removeInstance() - OFFLINE, force:false, interactive
EXPECT_THROWS(function() { c.removeInstance(hostname+":"+__mysql_sandbox_port2, {interactive:1, force:false}); }, "Instance is not ONLINE and cannot be safely removed", "MYSQLSH");
EXPECT_OUTPUT_CONTAINS("ERROR: <<<hostname>>>:<<<__mysql_sandbox_port2>>> is reachable but has state OFFLINE");

//@<> removeInstance() - OFFLINE, force:true, interactive
EXPECT_NO_THROWS(function() { c.removeInstance(hostname+":"+__mysql_sandbox_port2, {interactive:1, force:true}); });
EXPECT_OUTPUT_CONTAINS("NOTE: <<<hostname>>>:<<<__mysql_sandbox_port2>>> is reachable but has state OFFLINE");
EXPECT_OUTPUT_CONTAINS("The instance '<<<hostname>>>:<<<__mysql_sandbox_port2>>>' was successfully removed from the cluster.");

// ------------------------

// Check removeInstance() with values that don't match what's in the MD

// Bug #30501628	REMOVEINSTANCE, SETPRIMARY ETC SHOULD WORK WITH ANY FORM OF ADDRESS
// NOTE: this test must come last before sandbox destruction because it changes the sandbox configs
//@<> add instance back for next test
c.addInstance(__sandbox_uri2);

//@<> change instance address stored in the MD so that it doesn't match report_host
session1 = mysql.getSession(__sandbox_uri1);
session2 = mysql.getSession(__sandbox_uri2);

shell.dumpRows(session1.runSql("SELECT * FROM mysql_innodb_cluster_metadata.instances"));

session1.runSql("UPDATE mysql_innodb_cluster_metadata.instances"+
          " SET instance_name=concat(?,':',substring_index(instance_name, ':', -1)), "+
          " address=concat(?,':',substring_index(address, ':', -1)), "+
          " addresses=JSON_SET(addresses,"+
          " '$.mysqlX', concat(?,':',substring_index(addresses->>'$.mysqlX', ':', -1)),"+
          " '$.grLocal', concat(?,':',substring_index(addresses->>'$.grLocal', ':', -1)),"+
          " '$.mysqlClassic', concat(?,':',substring_index(addresses->>'$.mysqlClassic', ':', -1)))",
          [hostname_ip, hostname_ip, hostname_ip, hostname_ip, hostname_ip]);
shell.dumpRows(session1.runSql("SELECT * FROM mysql_innodb_cluster_metadata.instances"));

// now, MD has hostname_ip but report_host is hostname
shell.dumpRows(session2.runSql("SELECT @@report_host"));

c.status();

//@<> removeInstance() using hostname - hostname is not in MD, but uuid is
EXPECT_NO_THROWS(function() { c.removeInstance(hostname+":"+__mysql_sandbox_port2); });
EXPECT_OUTPUT_CONTAINS("The instance '<<<hostname>>>:<<<__mysql_sandbox_port2>>>' was successfully removed from the cluster.");

// add back for next test
c.addInstance(__sandbox_uri2);

//@<> removeInstance() using localhost - localhost is not in MD, but uuid is
EXPECT_NO_THROWS(function() { c.removeInstance("localhost:"+__mysql_sandbox_port2); });
EXPECT_OUTPUT_CONTAINS("The instance 'localhost:<<<__mysql_sandbox_port2>>>' was successfully removed from the cluster.");

// add back for next test
c.addInstance(__sandbox_uri2);

//@<> removeInstance() using hostname_ip - MD has hostname_ip (and UUID), although report_host is different
EXPECT_NO_THROWS(function() { c.removeInstance(hostname_ip+":"+__mysql_sandbox_port2); });
EXPECT_OUTPUT_CONTAINS("The instance '<<<hostname_ip>>>:<<<__mysql_sandbox_port2>>>' was successfully removed from the cluster.");

//@<> Finalization
session.close();
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);

// WL#13208 AdminAPI: Automatic handling of Clone GR integration

//Since clone copies all the data, including mysql.slave_master_info (where the CHANGE MASTER credentials are stored),
// the following issue may happen:
//
//1) Group is bootstrapped on server1.
//    user_account:  mysql_innodb_cluster_1
//2) server2 is added to the group
//    user_account:  mysql_innodb_cluster_2
//    But server2 is cloned from server1, which means its recovery account
//    will be mysql_innodb_cluster_1
//3) server3 is added to the group
//    user_account:  mysql_innodb_cluster_3
//    But server3 is cloned from server1, which means its recovery account
//    will be mysql_innodb_cluster_1
//4) server1 is removed from the group
//    mysql_innodb_cluster_1 account is dropped on all group servers.
//
//To avoid such situation, we will re-issue the CHANGE MASTER query after clone to ensure the right account is used.
//On top of that, we will make sure that the account is not dropped if any other cluster member is using it.
//
// To verify this fix, we do the following:
// 1 - create a cluster
// 2 - add an instance using clone
// 3 - add another instance using clone
// 4 - Remove the seed instance
// 5 - Re-add seed instance
// 6 - Remove seed instance
// 7 - Dissolve cluster

//@<> WL#13208: Initialization {VER(>=8.0.17)}
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: hostname, server_id:"1111"});
testutil.deploySandbox(__mysql_sandbox_port2, "root", {report_host: hostname, server_id:"2222"});
testutil.deploySandbox(__mysql_sandbox_port3, "root", {report_host: hostname, server_id:"3333"});
shell.connect(__sandbox_uri1);
var c = dba.createCluster('test', {expelTimeout: 0});

var comm_stack = c.status({extended: 1})["defaultReplicaSet"]["communicationStack"];

//@<> WL#13208: Add instance 2 using clone {VER(>=8.0.17)}
c.addInstance(__sandbox_uri2, {recoveryMethod: "clone"});

//@<> WL#13208: Add instance 3 using clone {VER(>=8.0.17)}
c.addInstance(__sandbox_uri3, {recoveryMethod: "clone"});

//@<> WL#13208: Remove the seed instance {VER(>=8.0.17)}
c.removeInstance(__sandbox_uri1);

//@<> WL#13208: Re-add seed instance {VER(>=8.0.17)}
session.close();
shell.connect(__sandbox_uri2);
var c = dba.getCluster();
c.addInstance(__sandbox_uri1, {recoveryMethod: "incremental"});

//@<> WL#13208: Cluster status {VER(>=8.0.17)}
c.status();
if (__version_num >= 80011) {
EXPECT_STDOUT_CONTAINS_MULTILINE(`
{
    "clusterName": "test",
    "defaultReplicaSet": {
        "name": "default",
        "primary": "${hostname}:${__mysql_sandbox_port2}",
        "ssl": "REQUIRED",
        "status": "OK",
        "statusText": "Cluster is ONLINE and can tolerate up to ONE failure.",
        "topology": {
            "${hostname}:${__mysql_sandbox_port1}": {
                "address": "${hostname}:${__mysql_sandbox_port1}",
                "memberRole": "SECONDARY",
                "mode": "R/O",
                "readReplicas": {},
                "replicationLag": [[*]],
                "role": "HA",
                "status": "ONLINE",
                "version": "${__version}"
            },
            "${hostname}:${__mysql_sandbox_port2}": {
                "address": "${hostname}:${__mysql_sandbox_port2}",
                "memberRole": "PRIMARY",
                "mode": "R/W",
                "readReplicas": {},
                "replicationLag": [[*]],
                "role": "HA",
                "status": "ONLINE",
                "version": "${__version}"
            },
            "${hostname}:${__mysql_sandbox_port3}": {
                "address": "${hostname}:${__mysql_sandbox_port3}",
                "memberRole": "SECONDARY",
                "mode": "R/O",
                "readReplicas": {},
                "replicationLag": [[*]],
                "role": "HA",
                "status": "ONLINE",
                "version": "${__version}"
            }
        },
        "topologyMode": "Single-Primary"
    },
    "groupInformationSourceMember": "${hostname}:${__mysql_sandbox_port2}"
}
`);
} else {
EXPECT_STDOUT_CONTAINS_MULTILINE(`
{
    "clusterName": "test",
    "defaultReplicaSet": {
        "name": "default",
        "primary": "${hostname}:${__mysql_sandbox_port2}",
        "ssl": "REQUIRED",
        "status": "OK",
        "statusText": "Cluster is ONLINE and can tolerate up to ONE failure.",
        "topology": {
            "${hostname}:${__mysql_sandbox_port1}": {
                "address": "${hostname}:${__mysql_sandbox_port1}",
                "memberRole": "SECONDARY",
                "mode": "R/O",
                "readReplicas": {},
                "role": "HA",
                "status": "ONLINE",
                "version": "${__version}"
            },
            "${hostname}:${__mysql_sandbox_port2}": {
                "address": "${hostname}:${__mysql_sandbox_port2}",
                "memberRole": "PRIMARY",
                "mode": "R/W",
                "readReplicas": {},
                "role": "HA",
                "status": "ONLINE",
                "version": "${__version}"
            },
            "${hostname}:${__mysql_sandbox_port3}": {
                "address": "${hostname}:${__mysql_sandbox_port3}",
                "memberRole": "SECONDARY",
                "mode": "R/O",
                "readReplicas": {},
                "role": "HA",
                "status": "ONLINE",
                "version": "${__version}"
            }
        },
        "topologyMode": "Single-Primary"
    },
    "groupInformationSourceMember": "${hostname}:${__mysql_sandbox_port2}"
}
`);
}

//@<> WL#13208: Re-remove seed instance {VER(>=8.0.17)}
c.removeInstance(__sandbox_uri1);

//@<> WL#13208: Cluster status 2 {VER(>=8.0.17)}
c.status();

if (__version_num >= 80011) {
EXPECT_STDOUT_CONTAINS_MULTILINE(`
{
    "clusterName": "test",
    "defaultReplicaSet": {
        "name": "default",
        "primary": "${hostname}:${__mysql_sandbox_port2}",
        "ssl": "REQUIRED",
        "status": "OK_NO_TOLERANCE",
        "statusText": "Cluster is NOT tolerant to any failures.",
        "topology": {
            "${hostname}:${__mysql_sandbox_port2}": {
                "address": "${hostname}:${__mysql_sandbox_port2}",
                "memberRole": "PRIMARY",
                "mode": "R/W",
                "readReplicas": {},
                "replicationLag": [[*]],
                "role": "HA",
                "status": "ONLINE",
                "version": "${__version}"
            },
            "${hostname}:${__mysql_sandbox_port3}": {
                "address": "${hostname}:${__mysql_sandbox_port3}",
                "memberRole": "SECONDARY",
                "mode": "R/O",
                "readReplicas": {},
                "replicationLag": [[*]],
                "role": "HA",
                "status": "ONLINE",
                "version": "${__version}"
            }
        },
        "topologyMode": "Single-Primary"
    },
    "groupInformationSourceMember": "${hostname}:${__mysql_sandbox_port2}"
}
`);
} else {
EXPECT_STDOUT_CONTAINS_MULTILINE(`
{
    "clusterName": "test",
    "defaultReplicaSet": {
        "name": "default",
        "primary": "${hostname}:${__mysql_sandbox_port2}",
        "ssl": "REQUIRED",
        "status": "OK_NO_TOLERANCE",
        "statusText": "Cluster is NOT tolerant to any failures.",
        "topology": {
            "${hostname}:${__mysql_sandbox_port2}": {
                "address": "${hostname}:${__mysql_sandbox_port2}",
                "memberRole": "PRIMARY",
                "mode": "R/W",
                "readReplicas": {},
                "role": "HA",
                "status": "ONLINE",
                "version": "${__version}"
            },
            "${hostname}:${__mysql_sandbox_port3}": {
                "address": "${hostname}:${__mysql_sandbox_port3}",
                "memberRole": "SECONDARY",
                "mode": "R/O",
                "readReplicas": {},
                "role": "HA",
                "status": "ONLINE",
                "version": "${__version}"
            }
        },
        "topologyMode": "Single-Primary"
    },
    "groupInformationSourceMember": "${hostname}:${__mysql_sandbox_port2}"
}
`);
}

// Verify that recovery accounts of forcefully removed instances are dropped from the cluster {VER(>=8.0.17)}

// BUG#33630808 Instances that are removed using the 'force' option, because
// are either offline or unreachable, won't have its recovery account removed
// from the cluster. The reason is that the AdminAPI couldn't know which was
// the account to be dropped, however, that changed in 8.0.17 since the
// accounts are now stored in the metadata schema

var server_id1 = "1111";
var server_id2 = "2222";
var server_id3 = "3333"

// Add back instance1 to the cluster
shell.connect(__sandbox_uri2);
c = dba.getCluster();
c.addInstance(__sandbox_uri1, {recoveryMethod: "incremental"});
testutil.waitMemberState(__mysql_sandbox_port1, "ONLINE");

//   Recovery Accounts
//   |------------|---------------------------|---------------------------|
//   |////////////| recovery account in use   | recovery account in md    |
//   |____________|___________________________|___________________________|
// RW| instance 2 | mysql_innodb_cluster_2222 | mysql_innodb_cluster_2222 |
// RO| instance 1 | mysql_innodb_cluster_1111 | mysql_innodb_cluster_1111 |
// RO| instance 3 | mysql_innodb_cluster_3333 | mysql_innodb_cluster_3333 |
//   |------------|---------------------------|---------------------------|

// Get the recovery user of instance 3
shell.connect(__sandbox_uri3);

var recovery_account_in_use = get_recovery_account(session);
var recovery_account_generated = "mysql_innodb_cluster_" + server_id3;

// Kill instance 3
testutil.killSandbox(__mysql_sandbox_port3);

shell.connect(__sandbox_uri2);
testutil.waitMemberState(__mysql_sandbox_port3, "UNREACHABLE");

// Force remove instance 3
EXPECT_NO_THROWS(function() { c.removeInstance(__endpoint3, {force: true}); });
testutil.waitMemberTransactions(__mysql_sandbox_port1, __mysql_sandbox_port2);

// Validate that the recovery account (mysql_innodb_cluster_3333) was dropped from the cluster

// Get all users
var users_in_use = get_all_recovery_accounts(session);
var all_users = get_all_users(session);

EXPECT_FALSE(users_in_use.includes(recovery_account_in_use));
EXPECT_FALSE(users_in_use.includes(recovery_account_generated));

testutil.waitMemberTransactions(__mysql_sandbox_port1, __mysql_sandbox_port2);

// Validate that if an account is still in use, it's not dropped

// use clone and waitRecovery:0 so that the recovery account is not updated and
// the one from the seed is used by the target instance
c.removeInstance(__sandbox_uri1);
c.addInstance(__sandbox_uri1, {recoveryMethod: "clone", waitRecovery: 0})
testutil.waitMemberState(__mysql_sandbox_port1, "ONLINE");

//   Recovery Accounts (commStack XCOM)
//   |------------|---------------------------|---------------------------|
//   |////////////| recovery account in use   | recovery account in md    |
//   |____________|___________________________|___________________________|
// RW| instance 2 | mysql_innodb_cluster_2222 | mysql_innodb_cluster_2222 |
// RO| instance 1 | mysql_innodb_cluster_2222 | mysql_innodb_cluster_2222 |
//   |------------|---------------------------|---------------------------|

//   Recovery Accounts (commStack MYSQL)
//   |------------|---------------------------|---------------------------|
//   |////////////| recovery account in use   | recovery account in md    |
//   |____________|___________________________|___________________________|
// RW| instance 2 | mysql_innodb_cluster_1111 | mysql_innodb_cluster_1111 |
// RO| instance 1 | mysql_innodb_cluster_1111 | mysql_innodb_cluster_1111 |
//   |------------|---------------------------|---------------------------|

// Get the recovery user created for instance 1 (mysql_innodb_cluster_1111)
shell.connect(__sandbox_uri1);
var recovery_account = get_recovery_account(session, true);
var recovery_account_generated = "mysql_innodb_cluster_" + server_id1;
session.runSql("SET PERSIST group_replication_start_on_boot=OFF");

// Kill instance 1
shell.connect(__sandbox_uri2);
testutil.killSandbox(__mysql_sandbox_port1);
testutil.waitMemberState(__mysql_sandbox_port1, "UNREACHABLE");

// Quorum is lost, restore it
c.forceQuorumUsingPartitionOf(__sandbox_uri2);

// Force remove instance 1
EXPECT_NO_THROWS(function() { c.removeInstance(__endpoint1, {force: true}); });

// Get all users
var users_in_use = get_all_recovery_accounts(session);
var all_users = get_all_users(session);

// Validate that the recovery account was NOT dropped from the cluster since it's still being used by instance 2
EXPECT_TRUE(users_in_use.includes(recovery_account));

// Validate that the recovery account generated for the instance was dropped from the cluster
if (comm_stack == "XCOM") {
  EXPECT_FALSE(users_in_use.includes(recovery_account_generated));
}

// Add instance 1 back with clone and waitRecovery:0 so that the recovery
// account is not updated and the one from the seed is used by the target instance
testutil.startSandbox(__mysql_sandbox_port1);
c.addInstance(__sandbox_uri1, {recoveryMethod: "clone", waitRecovery: 0})
testutil.waitMemberState(__mysql_sandbox_port1, "ONLINE");

//   Recovery Accounts (commStack XCOM)
//   |------------|---------------------------|---------------------------|
//   |////////////| recovery account in use   | recovery account in md    |
//   |____________|___________________________|___________________________|
// RW| instance 2 | mysql_innodb_cluster_2222 | mysql_innodb_cluster_2222 |
// RO| instance 1 | mysql_innodb_cluster_2222 | mysql_innodb_cluster_2222 |
//   |------------|---------------------------|---------------------------|

//   Recovery Accounts (commStack MYSQL)
//   |------------|---------------------------|---------------------------|
//   |////////////| recovery account in use   | recovery account in md    |
//   |____________|___________________________|___________________________|
// RW| instance 2 | mysql_innodb_cluster_1111 | mysql_innodb_cluster_1111 |
// RO| instance 1 | mysql_innodb_cluster_1111 | mysql_innodb_cluster_1111 |
//   |------------|---------------------------|---------------------------|

// Stop GR on instance 1, leaving it reachable
shell.connect(__sandbox_uri1);

var recovery_account = get_recovery_account(session, false);
var recovery_account_generated = "mysql_innodb_cluster_" + server_id1;

session.runSql("STOP group_replication");
shell.connect(__sandbox_uri2);
testutil.waitMemberState(__mysql_sandbox_port1, "(MISSING)");

// Force remove instance 1
EXPECT_NO_THROWS(function() { c.removeInstance(__endpoint1, {force: true}); });

// Get all users
var users_in_use = get_all_recovery_accounts(session);

// Validate that the recovery account was NOT dropped from the cluster since it's still being used by instance 2
EXPECT_TRUE(users_in_use.includes(recovery_account));

// Validate that the recovery account generated for the instance was dropped from the cluster
if (comm_stack == "XCOM") {
  EXPECT_FALSE(users_in_use.includes(recovery_account_generated));
}

// Add instance 1 back to the cluster
c.addInstance(__sandbox_uri1, {recoveryMethod: "clone", waitRecovery: 0})
testutil.waitMemberState(__mysql_sandbox_port1, "ONLINE");

// Change the primary to instance 1
c.setPrimaryInstance(__sandbox_uri1);

//   Recovery Accounts (commStack XCOM)
//   |------------|---------------------------|---------------------------|
//   |////////////| recovery account in use   | recovery account in md    |
//   |____________|___________________________|___________________________|
// RW| instance 1 | mysql_innodb_cluster_2222 | mysql_innodb_cluster_2222 |
// RO| instance 2 | mysql_innodb_cluster_2222 | mysql_innodb_cluster_2222 |
//   |------------|---------------------------|---------------------------|

//   Recovery Accounts (commStack MYSQL)
//   |------------|---------------------------|---------------------------|
//   |////////////| recovery account in use   | recovery account in md    |
//   |____________|___________________________|___________________________|
// RW| instance 1 | mysql_innodb_cluster_1111 | mysql_innodb_cluster_1111 |
// RO| instance 2 | mysql_innodb_cluster_1111 | mysql_innodb_cluster_1111 |
//   |------------|---------------------------|---------------------------|

// Remove instance 2 from the cluster
EXPECT_NO_THROWS(function() { c.removeInstance(__endpoint2, {force: true}); });

// instance's 2 recovery account must be kept since it's in use by instance 1

// Get all users
shell.connect(__sandbox_uri1);
var users_in_use = get_all_recovery_accounts(session);

// Validate that the recovery account was NOT dropped from the cluster since it's still being used by instance 2
EXPECT_TRUE(users_in_use.includes(recovery_account));

// The recovery instance of instance1 is not dropped
//
// removeInstance() attempts to clean-up the accounts of the instance being
// removed in case its unused, by doing a lookup in the metadata. We are
// removing instance2 (mysql_innodb_cluster_2222) so the account is not removed and the account from instance 1 is still in the cluster even though unused.
//
// TODO: BUG#33235502 to handle the cleanup in cluster.rescan()
EXPECT_TRUE(users_in_use.includes(recovery_account_generated));

//@<> WL#13208: Finalization {VER(>=8.0.17)}
session.close();
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
testutil.destroySandbox(__mysql_sandbox_port3);
