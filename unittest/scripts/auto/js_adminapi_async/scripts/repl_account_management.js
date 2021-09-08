//@ {VER(>=8.0.11)}
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

hostmask = hostname_ip.split(".")[0]+".%";

function CHECK_REPL_USERS(session, server_ids, host) {
  var host_ = host ? host : "%";

  var users = session.runSql("select user, host from mysql.user where user like 'mysql_innodb_%' order by user").fetchAll();
  EXPECT_EQ(server_ids.length, users.length);

  for (i in server_ids) {
    EXPECT_EQ("mysql_innodb_rs_"+server_ids[i], users[i][0]);
    EXPECT_EQ(host_, users[i][1]);

    row = session.runSql("select attributes->>'$.replicationAccountUser', attributes->>'$.replicationAccountHost' from mysql_innodb_cluster_metadata.instances where attributes->>'$.server_id' = ?", [server_ids[i]]).fetchOne();
    if (host == null) {
      EXPECT_EQ(null, row[0]);
      EXPECT_EQ(null, row[1]);
    } else {
      EXPECT_EQ(users[i][0], row[0]);
      EXPECT_EQ(host_, row[1]);
    }
  }

  allowed_host = session.runSql("select attributes->>'$.opt_replicationAllowedHost' from mysql_innodb_cluster_metadata.clusters").fetchOne()[0];
  EXPECT_EQ(host, allowed_host, "opt_replicationAllowedHost");
}

function CHECK_RECOVERY_USER(session) {
  var server_id = session.runSql("select @@server_id").fetchOne()[0];
  var user = session.runSql("select user_name from mysql.slave_master_info").fetchOne()[0];
  EXPECT_EQ("mysql_innodb_rs_"+server_id, user);
}

function get_global_option(options, name) {
  for (var i in options["replicaSet"]["globalOptions"]) {
    opt = options["replicaSet"]["globalOptions"][i];
    if (opt["option"] == name)
      return opt["value"];
  }
  return undefined;
}

//@<> Test accounts created by default

shell.connect(__sandbox_uri1);
c = dba.createReplicaSet("cluster");
c.addInstance(__sandbox_uri2, {recoveryMethod:"incremental"});

//@<> add by clone {VER(>=8.0.17)}
c.addInstance(__sandbox_uri3, {recoveryMethod:"clone"});
session3 = mysql.getSession(__sandbox_uri3);
//@<> add by clone {VER(<8.0.17)}
c.addInstance(__sandbox_uri3, {recoveryMethod:"incremental"});

//@<> after add by clone
s = c.status();

EXPECT_EQ("ONLINE", s.replicaSet.topology[sb1]["status"]);
EXPECT_EQ("ONLINE", s.replicaSet.topology[sb2]["status"]);
EXPECT_EQ("ONLINE", s.replicaSet.topology[sb3]["status"]);

CHECK_REPL_USERS(session1, [1111, 2222, 3333], "%");

// CHECK_RECOVERY_USER(session1);
CHECK_RECOVERY_USER(session2);
CHECK_RECOVERY_USER(session3);

//@<> check account deletion with defaults
c.removeInstance(__sandbox_uri2);

CHECK_REPL_USERS(session1, [1111, 3333], "%");

// check account deletion while the target is offline
c.removeInstance(__sandbox_uri3, {force:1});

CHECK_REPL_USERS(session1, [1111], "%");

//@<> Check users on adopt
session1.runSql("drop schema mysql_innodb_cluster_metadata");
session1.runSql("drop user mysql_innodb_rs_1111@'%'");

session2.runSql("change master to master_host=?, master_port=?, master_user='root', master_password='root', MASTER_AUTO_POSITION=1", [hostname_ip, __mysql_sandbox_port1]);
session3.runSql("change master to master_host=?, master_port=?, master_user='root', master_password='root', MASTER_AUTO_POSITION=1", [hostname_ip, __mysql_sandbox_port1]);
session2.runSql("start slave");
session3.runSql("start slave");

shell.connect(__sandbox_uri1);

c = dba.createReplicaSet("clus", {adoptFromAR:1});
c.status();

// check root user didn't get dropped
EXPECT_EQ("%,localhost", session1.runSql("select group_concat(host) from mysql.user where user='root' order by host").fetchOne()[0]);

// MD should also be empty
CHECK_REPL_USERS(session1, [1111, 2222, 3333], null);

//@<> Check options for default
EXPECT_EQ(null, get_global_option(c.options(), "replicationAllowedHost"));

//@<> Check setOption when changing from adopted default
c.setOption("replicationAllowedHost", hostmask);
EXPECT_EQ(hostmask, get_global_option(c.options(), "replicationAllowedHost"));

CHECK_REPL_USERS(session1, [1111, 2222, 3333], hostmask);

CHECK_RECOVERY_USER(session2);
CHECK_RECOVERY_USER(session3);

//@<> Check replicationAllowedHost

reset_instance(session1);
reset_instance(session2);
reset_instance(session3);

shell.connect(__sandbox_uri1);
c = dba.createReplicaSet("cluster", {replicationAllowedHost:hostname_ip});
c.addInstance(__sandbox_uri2, {recoveryMethod:"incremental"});
//@<> add by clone2 {VER(>=8.0.17)}
c.addInstance(__sandbox_uri3, {recoveryMethod:"clone"});
session3 = mysql.getSession(__sandbox_uri3);
//@<> add by clone2 {VER(<8.0.17)}
c.addInstance(__sandbox_uri3, {recoveryMethod:"incremental"});

//@<> after add by clone2
var s = c.status();

EXPECT_EQ("ONLINE", s.replicaSet.topology[sb1]["status"]);
EXPECT_EQ("ONLINE", s.replicaSet.topology[sb2]["status"]);
EXPECT_EQ("ONLINE", s.replicaSet.topology[sb3]["status"]);

CHECK_REPL_USERS(session1, [1111, 2222, 3333], hostname_ip);

CHECK_RECOVERY_USER(session2);
CHECK_RECOVERY_USER(session3);

//@<> Check options
EXPECT_EQ(hostname_ip, get_global_option(c.options(), "replicationAllowedHost"));

//@<> Check setOption
c.setOption("replicationAllowedHost", hostmask);
EXPECT_EQ(hostmask, get_global_option(c.options(), "replicationAllowedHost"));

CHECK_REPL_USERS(session1, [1111, 2222, 3333], hostmask);

CHECK_RECOVERY_USER(session2);
CHECK_RECOVERY_USER(session3);

//@<> check after setPrimary
c.setPrimaryInstance(__sandbox_uri2);

CHECK_REPL_USERS(session1, [1111, 2222, 3333], hostmask);

CHECK_RECOVERY_USER(session1);
CHECK_RECOVERY_USER(session3);

c.setPrimaryInstance(__sandbox_uri1);
CHECK_REPL_USERS(session1, [1111, 2222, 3333], hostmask);

CHECK_RECOVERY_USER(session2);
CHECK_RECOVERY_USER(session3);

//@<> check account deletion
c.removeInstance(__sandbox_uri2);

CHECK_REPL_USERS(session1, [1111, 3333], hostmask);

// check account deletion while the target is offline
c.removeInstance(__sandbox_uri3, {force:1});

CHECK_REPL_USERS(session1, [1111], hostmask);

//@<> Check users with replicationAllowedHost on adopt

session1.runSql("drop schema mysql_innodb_cluster_metadata");
session1.runSql("drop user mysql_innodb_rs_1111@?", [hostmask]);

session2.runSql("change master to master_host=?, master_port=?, master_user='root', master_password='root', MASTER_AUTO_POSITION=1", [hostname_ip, __mysql_sandbox_port1]);
session3.runSql("change master to master_host=?, master_port=?, master_user='root', master_password='root', MASTER_AUTO_POSITION=1", [hostname_ip, __mysql_sandbox_port1]);
session2.runSql("start slave");
session3.runSql("start slave");

c = dba.createReplicaSet("clus", {adoptFromAR:1, replicationAllowedHost:hostname_ip});
c.status();

// check root user didn't get dropped
EXPECT_EQ("%,localhost", session1.runSql("select group_concat(host) from mysql.user where user='root' order by host").fetchOne()[0]);

// cluster is adopted, so MD shouldn't have accounts but the accounts should exist
session1.runSql("select * from mysql_innodb_cluster_metadata.instances");
session1.runSql("select user,host from mysql.user");

EXPECT_EQ(null, session1.runSql("select group_concat(attributes->>'$.replicationAccountUser') u from mysql_innodb_cluster_metadata.instances where attributes->>'$.replicationAccountUser' is not null").fetchOne()[0]);
EXPECT_EQ(`mysql_innodb_rs_1111@${hostname_ip},mysql_innodb_rs_2222@${hostname_ip},mysql_innodb_rs_3333@${hostname_ip}`, session1.runSql("select group_concat(concat(user,'@',host)) from mysql.user where user like 'mysql_innodb%' order by user").fetchOne()[0]);

EXPECT_EQ("root", session2.runSql("select user_name from mysql.slave_master_info").fetchOne()[0]);
EXPECT_EQ("root", session3.runSql("select user_name from mysql.slave_master_info").fetchOne()[0]);

//@<> removeInstance on adopt
c.removeInstance(__sandbox_uri2);

EXPECT_EQ(null, session1.runSql("select group_concat(attributes->>'$.replicationAccountUser') u from mysql_innodb_cluster_metadata.instances where attributes->>'$.replicationAccountUser' is not null").fetchOne()[0]);
EXPECT_EQ(`mysql_innodb_rs_1111@${hostname_ip},mysql_innodb_rs_3333@${hostname_ip}`, session1.runSql("select group_concat(concat(user,'@',host)) from mysql.user where user like 'mysql_innodb%' order by user").fetchOne()[0]);

// check root user didn't get dropped
EXPECT_EQ("%,localhost", session1.runSql("select group_concat(host) from mysql.user where user='root' order by host").fetchOne()[0]);

//@<> addInstance should use the proper hostname_ip
c.addInstance(__sandbox_uri2);

EXPECT_EQ(`mysql_innodb_rs_2222@${hostname_ip}`, session1.runSql("select group_concat(concat(attributes->>'$.replicationAccountUser','@',attributes->>'$.replicationAccountHost')) from mysql_innodb_cluster_metadata.instances where attributes->>'$.replicationAccountUser' is not null").fetchOne()[0]);
EXPECT_EQ(`mysql_innodb_rs_1111@${hostname_ip},mysql_innodb_rs_2222@${hostname_ip},mysql_innodb_rs_3333@${hostname_ip}`, session1.runSql("select group_concat(concat(user,'@',host)) from mysql.user where user like 'mysql_innodb%' order by user").fetchOne()[0]);

EXPECT_EQ("mysql_innodb_rs_2222", session2.runSql("select user_name from mysql.slave_master_info").fetchOne()[0]);
EXPECT_EQ("root", session3.runSql("select user_name from mysql.slave_master_info").fetchOne()[0]);

//<> Cleanup
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
testutil.destroySandbox(__mysql_sandbox_port3);
