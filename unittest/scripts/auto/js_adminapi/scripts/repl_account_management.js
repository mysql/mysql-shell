//@<> Setup

// use the IP instead of the hostname, because we want to test authentication and IP will not reverse into hostname in many test environments
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host:hostname_ip, server_id:"1111"});
testutil.deploySandbox(__mysql_sandbox_port2, "root", {report_host:hostname_ip, server_id:"2222"});
testutil.deploySandbox(__mysql_sandbox_port3, "root", {report_host:hostname_ip, server_id:"3333"});

session1 = mysql.getSession(__sandbox_uri1);
session2 = mysql.getSession(__sandbox_uri2);
session3 = mysql.getSession(__sandbox_uri3);

var sb1 = hostname_ip+":"+__mysql_sandbox_port1;
var sb2 = hostname_ip+":"+__mysql_sandbox_port2;
var sb3 = hostname_ip+":"+__mysql_sandbox_port3;

function CHECK_REPL_USERS(session, server_ids, host, ensure_format) {
  var users = session.runSql("select user, host from mysql.user where user like 'mysql_innodb_%' order by user").fetchAll();
  EXPECT_EQ(server_ids.length, users.length);

  for (i in server_ids) {
    if (ensure_format === undefined || ensure_format)
      EXPECT_EQ("mysql_innodb_cluster_"+server_ids[i], users[i][0]);
    EXPECT_EQ(host, users[i][1]);

    row = session.runSql("select i.attributes->>'$.recoveryAccountUser', i.attributes->>'$.recoveryAccountHost' from mysql_innodb_cluster_metadata.instances i where i.attributes->>'$.server_id'=?", [server_ids[i]]).fetchOne();
    
    EXPECT_EQ(users[i][0], row[0]);
    EXPECT_EQ(host, row[1]);
  }
}

function CHECK_RECOVERY_USER(session) {
  var uuid = session.runSql("select @@server_uuid").fetchOne()[0];
  var meta_user = session.runSql("select i.attributes->>'$.recoveryAccountUser' from mysql_innodb_cluster_metadata.instances i where i.mysql_server_uuid=?", [uuid]).fetchOne()[0];

  var user = session.runSql("select user_name from mysql.slave_master_info where channel_name='group_replication_recovery'").fetchOne()[0];
  
  EXPECT_EQ(meta_user, user);
}

function get_global_option(options, name) {
  for (var i in options["defaultReplicaSet"]["globalOptions"]) {
    opt = options["defaultReplicaSet"]["globalOptions"][i];
    if (opt["option"] == name)
      return opt["value"];
  }
  return undefined;
}

//@<> Test accounts created by default

shell.connect(__sandbox_uri1);
c = dba.createCluster("cluster");
c.addInstance(__sandbox_uri2, {recoveryMethod:"incremental"});

//@<> add by clone {VER(>=8.0.17)}
c.addInstance(__sandbox_uri3, {recoveryMethod:"clone"});
session3 = mysql.getSession(__sandbox_uri3);
//@<> add by clone {VER(<8.0.17)}
c.addInstance(__sandbox_uri3, {recoveryMethod:"incremental"});

//@<> continue

s = c.status();

EXPECT_EQ("ONLINE", s.defaultReplicaSet.topology[sb1]["status"]);
EXPECT_EQ("ONLINE", s.defaultReplicaSet.topology[sb2]["status"]);
EXPECT_EQ("ONLINE", s.defaultReplicaSet.topology[sb3]["status"]);

CHECK_REPL_USERS(session1, [1111, 2222, 3333], "%");

CHECK_RECOVERY_USER(session1);
CHECK_RECOVERY_USER(session2);
CHECK_RECOVERY_USER(session3);

// check account deletion
c.removeInstance(__sandbox_uri2);

CHECK_REPL_USERS(session1, [1111, 3333], "%");

// check account deletion while the target is offline
session3.runSql("stop group_replication");
c.removeInstance(__sandbox_uri3, {force:1});

CHECK_REPL_USERS(session1, [1111], "%");

//@<> check setOption() on old style account creation where nothing was stored in the MD
c.addInstance(__sandbox_uri2, {recoveryMethod:"incremental"});
c.addInstance(__sandbox_uri3, {recoveryMethod:"incremental"});

// simulate accounts created at 8.0.11
session1.runSql("create user mysql_innodb_cluster_r3294023486@'%'");
session1.runSql("create user mysql_innodb_cluster_r3294023487@'%'");
session1.runSql("create user mysql_innodb_cluster_r3294023488@'%'");
session1.runSql("create user mysql_innodb_cluster_r3294023486@localhost");
session1.runSql("create user mysql_innodb_cluster_r3294023487@localhost");
session1.runSql("create user mysql_innodb_cluster_r3294023488@localhost");

session1.runSql("change master to master_user='mysql_innodb_cluster_r3294023486' for channel 'group_replication_recovery'");
session2.runSql("change master to master_user='mysql_innodb_cluster_r3294023487' for channel 'group_replication_recovery'");
session3.runSql("change master to master_user='mysql_innodb_cluster_r3294023488' for channel 'group_replication_recovery'");

session1.runSql("drop user mysql_innodb_cluster_1111@'%'");
session1.runSql("drop user mysql_innodb_cluster_2222@'%'");
session1.runSql("drop user mysql_innodb_cluster_3333@'%'");
session1.runSql("select user,host from mysql.user");

// simulate MD from 8.0.11
session.runSql("update mysql_innodb_cluster_metadata.clusters set attributes=json_object('default', true)");
session.runSql("update mysql_innodb_cluster_metadata.instances set attributes=json_object('mysqlX', attributes->'$.mysqlX', 'grLocal', attributes->'$.grLocal', 'mysqlClassic', attributes->'$.mysqlClassic')");

EXPECT_EQ(null, get_global_option(c.options(), "replicationAllowedHost"));

c.setOption("replicationAllowedHost", hostname_ip);

EXPECT_EQ(hostname_ip, get_global_option(c.options(), "replicationAllowedHost"));

// put back server_id in MD so that CHECK_REPL_USERS can do its checks
c.rescan();

CHECK_REPL_USERS(session1, [1111, 2222, 3333], hostname_ip, false);

CHECK_RECOVERY_USER(session1);
CHECK_RECOVERY_USER(session2);
CHECK_RECOVERY_USER(session3);

c.setOption("replicationAllowedHost", "%");

EXPECT_EQ("%", get_global_option(c.options(), "replicationAllowedHost"));

CHECK_REPL_USERS(session1, [1111, 2222, 3333], "%", false);

CHECK_RECOVERY_USER(session1);
CHECK_RECOVERY_USER(session2);
CHECK_RECOVERY_USER(session3);

//@<> Check users on adopt
session1.runSql("drop schema mysql_innodb_cluster_metadata");

session1.runSql("drop user mysql_innodb_cluster_r3294023486@'%'");
session1.runSql("drop user mysql_innodb_cluster_r3294023487@'%'");
session1.runSql("drop user mysql_innodb_cluster_r3294023488@'%'");

session1.runSql("change master to master_user='root' for channel 'group_replication_recovery'");
session2.runSql("change master to master_user='root' for channel 'group_replication_recovery'");
session3.runSql("change master to master_user='root' for channel 'group_replication_recovery'");

c = dba.createCluster("clus", {adoptFromGR:1});
c.status();

// check root user didn't get dropped
EXPECT_EQ("%,localhost", session1.runSql("select group_concat(host) from mysql.user where user='root' order by host").fetchOne()[0]);

CHECK_REPL_USERS(session1, [1111, 2222, 3333], "%");

//@<> Check replicationAllowedHost
reset_instance(session1);
reset_instance(session2);
reset_instance(session3);

shell.connect(__sandbox_uri1);
c = dba.createCluster("cluster", {replicationAllowedHost:hostname_ip});
c.addInstance(__sandbox_uri2, {recoveryMethod:"incremental"});
//@<> add by clone2 {VER(>=8.0.17)}
c.addInstance(__sandbox_uri3, {recoveryMethod:"clone"});
session3 = mysql.getSession(__sandbox_uri3);
//@<> add by clone2 {VER(<8.0.17)}
c.addInstance(__sandbox_uri3, {recoveryMethod:"incremental"});

//@<> continue2
var s = c.status();

EXPECT_EQ("ONLINE", s.defaultReplicaSet.topology[sb1]["status"]);
EXPECT_EQ("ONLINE", s.defaultReplicaSet.topology[sb2]["status"]);
EXPECT_EQ("ONLINE", s.defaultReplicaSet.topology[sb3]["status"]);

CHECK_REPL_USERS(session1, [1111, 2222, 3333], hostname_ip);

CHECK_RECOVERY_USER(session1);
CHECK_RECOVERY_USER(session2);
CHECK_RECOVERY_USER(session3);

// check account deletion
c.removeInstance(__sandbox_uri2);

CHECK_REPL_USERS(session1, [1111, 3333], hostname_ip);

// check account deletion while the target is offline
session3.runSql("stop group_replication");
c.removeInstance(__sandbox_uri3, {force:1});

CHECK_REPL_USERS(session1, [1111], hostname_ip);

//@<> change replicationAllowedHost

EXPECT_EQ(hostname_ip, get_global_option(c.options(), "replicationAllowedHost"));

c.setOption("replicationAllowedHost", "%");

EXPECT_EQ("%", get_global_option(c.options(), "replicationAllowedHost"));

c.addInstance(__sandbox_uri2);
session2 = mysql.getSession(__sandbox_uri2);

//@<> add with clone {VER(>=8.0.17)}
c.addInstance(__sandbox_uri3, {recoveryMethod:"clone"});
session3 = mysql.getSession(__sandbox_uri3);

//@<> add with clone {VER(<8.0.17)}
c.addInstance(__sandbox_uri3, {recoveryMethod:"incremental"});

//@<> continue after clone
testutil.waitMemberTransactions(__mysql_sandbox_port2, __mysql_sandbox_port1);
testutil.waitMemberTransactions(__mysql_sandbox_port3, __mysql_sandbox_port1);

session1.runSql("select user,host from mysql.user");
session1.runSql("select * from mysql_innodb_cluster_metadata.instances");

CHECK_REPL_USERS(session1, [1111, 2222, 3333], "%");

CHECK_RECOVERY_USER(session1);
CHECK_RECOVERY_USER(session2);
CHECK_RECOVERY_USER(session3);

//@<> Check users with replicationAllowedHost on adopt
session1.runSql("drop schema mysql_innodb_cluster_metadata");
session1.runSql("drop user mysql_innodb_cluster_1111@?", ["%"]);
session1.runSql("drop user mysql_innodb_cluster_2222@?", ["%"]);
session1.runSql("drop user mysql_innodb_cluster_3333@?", ["%"]);

session1.runSql("change master to master_user='root' for channel 'group_replication_recovery'");
session2.runSql("change master to master_user='root' for channel 'group_replication_recovery'");
session3.runSql("change master to master_user='root' for channel 'group_replication_recovery'");

c = dba.createCluster("clus", {adoptFromGR:1, replicationAllowedHost:hostname_ip});
c.status();

// check root user didn't get dropped
EXPECT_EQ("%,localhost", session1.runSql("select group_concat(host) from mysql.user where user='root' order by host").fetchOne()[0]);

session1.runSql("select user,host from mysql.user");
session1.runSql("select * from mysql_innodb_cluster_metadata.instances");

CHECK_REPL_USERS(session1, [1111, 2222, 3333], hostname_ip);

//@<> change replicationAllowedHost after adopt
EXPECT_EQ(hostname_ip, get_global_option(c.options(), "replicationAllowedHost"));

c.setOption("replicationAllowedHost", "%");

EXPECT_EQ("%", get_global_option(c.options(), "replicationAllowedHost"));

session1.runSql("select user,host from mysql.user");
session1.runSql("select * from mysql_innodb_cluster_metadata.instances");

CHECK_REPL_USERS(session1, [1111, 2222, 3333], "%");

CHECK_RECOVERY_USER(session1);
CHECK_RECOVERY_USER(session2);
CHECK_RECOVERY_USER(session3);

//@<> setPrimaryInstance {VER(>=8.0.17)}
c.setPrimaryInstance(__sandbox_uri2);

CHECK_REPL_USERS(session1, [1111, 2222, 3333], "%");

CHECK_RECOVERY_USER(session1);
CHECK_RECOVERY_USER(session2);
CHECK_RECOVERY_USER(session3);

c.setPrimaryInstance(__sandbox_uri1);

//@<> Cleanup
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
testutil.destroySandbox(__mysql_sandbox_port3);
