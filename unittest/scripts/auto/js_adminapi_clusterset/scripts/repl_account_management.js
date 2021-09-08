//@ {VER(>=8.0.27)}

//@<> INCLUDE clusterset_utils.inc

//@<> Setup
// use the IP instead of the hostname, because we want to test authentication and IP will not reverse into hostname in many test environments
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host:hostname_ip, server_id:"1111"});
testutil.deploySandbox(__mysql_sandbox_port2, "root", {report_host:hostname_ip, server_id:"2222"});
testutil.deploySandbox(__mysql_sandbox_port3, "root", {report_host:hostname_ip, server_id:"3333"});
testutil.deploySandbox(__mysql_sandbox_port4, "root", {report_host:hostname_ip, server_id:"4444"});

session1 = mysql.getSession(__sandbox_uri1);
session2 = mysql.getSession(__sandbox_uri2);
session3 = mysql.getSession(__sandbox_uri3);
session4 = mysql.getSession(__sandbox_uri4);

var sb1 = hostname+":"+__mysql_sandbox_port1;
var sb2 = hostname+":"+__mysql_sandbox_port2;
var sb3 = hostname+":"+__mysql_sandbox_port3;
var sb4 = hostname+":"+__mysql_sandbox_port4;

function CHECK_RECOVERY_USERS(session, server_ids, host, adjust_num_users) {
  if (!adjust_num_users)
    adjust_num_users = 0;
  var users = session.runSql("select user, host from mysql.user where user like 'mysql_innodb_cluster_%' order by user").fetchAll();
  EXPECT_EQ(server_ids.length, users.length - adjust_num_users);

  users = session.runSql("select user, host from mysql.user where user like 'mysql_innodb_cluster_%' and host = ? order by user", [host]).fetchAll();

  for (i in server_ids) {
    EXPECT_EQ("mysql_innodb_cluster_"+server_ids[i], users[i][0]);
    EXPECT_EQ(host, users[i][1]);
  }
}

function CHECK_CLUSTER_USERS(session, cluster_names, host, adjust_num_users) {
  var users = session.runSql("select user, host from mysql.user where user like 'mysql_innodb_cs_%' order by user").fetchAll();
  EXPECT_EQ(cluster_names.length, users.length - adjust_num_users);

  for (i in cluster_names.length) {
    user = session.runSql("select attributes->>'$.replicationAccountUser' from mysql_innodb_cluster_metadata.clusters where cluster_name=?", [cluster_names[i]]).fetchOne()[0];
    EXPECT_TRUE(user);

    account_host = session.runSql("select host from mysql.user where user=?", [user]).fetchOne()[0];
    EXPECT_EQ(host, account_host);
  }
}

function CHECK_RECOVERY_USER(session, is_primary) {
  row = session.runSql("select @@server_id, @@server_uuid").fetchOne();
  var server_id = row[0];
  var uuid = row[1];
  var user = session.runSql("select user_name from mysql.slave_master_info where channel_name='group_replication_recovery'").fetchOne()[0];
  EXPECT_EQ("mysql_innodb_cluster_"+server_id, user);

  if (!is_primary) {
    cs_user = session.runSql("select c.attributes->>'$.replicationAccountUser' from mysql_innodb_cluster_metadata.clusters c join mysql_innodb_cluster_metadata.instances i on c.cluster_id = i.cluster_id where i.mysql_server_uuid=?", [uuid]).fetchOne()[0];

    var user = session.runSql("select user_name from mysql.slave_master_info where channel_name='clusterset_replication'").fetchOne()[0];
    EXPECT_EQ(cs_user, user);
  }
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
c1 = dba.createCluster("cluster1");

cs = c1.createClusterSet("cs");

c2 = cs.createReplicaCluster(__sandbox_uri2, "cluster2", {recoveryMethod:"incremental"});
c2.addInstance(__sandbox_uri3, {recoveryMethod:"clone"});
session3 = mysql.getSession(__sandbox_uri3);

c3 = cs.createReplicaCluster(__sandbox_uri4, "cluster3", {recoveryMethod:"clone"});
session4 = mysql.getSession(__sandbox_uri4);

s = cs.status({extended:1});

EXPECT_EQ("OK_NO_TOLERANCE", s.clusters.cluster1.status);
EXPECT_EQ("OK_NO_TOLERANCE", s.clusters.cluster2.status);
EXPECT_EQ("OK_NO_TOLERANCE", s.clusters.cluster3.status);

CHECK_RECOVERY_USERS(session1, [1111, 2222, 3333, 4444], "%");
CHECK_CLUSTER_USERS(session1, ["cluster1", "cluster2", "cluster3"], "%", 0);

CHECK_RECOVERY_USER(session1, 1);
CHECK_RECOVERY_USER(session2, 0);
CHECK_RECOVERY_USER(session3, 0);
CHECK_RECOVERY_USER(session4, 0);

//@<> check options
EXPECT_EQ([{"option": "replicationAllowedHost", "value": "%"}], cs.options()["clusterSet"]["globalOptions"]);

//@<> check primary switch
cs.setPrimaryCluster("cluster2");

CHECK_RECOVERY_USERS(session1, [1111, 2222, 3333, 4444], "%");
CHECK_CLUSTER_USERS(session1, ["cluster1", "cluster2", "cluster3"], "%", 0);

CHECK_RECOVERY_USER(session1, 0);
CHECK_RECOVERY_USER(session2, 1);
CHECK_RECOVERY_USER(session3, 1);
CHECK_RECOVERY_USER(session4, 0);

cs.setPrimaryCluster("cluster1");

CHECK_RECOVERY_USERS(session1, [1111, 2222, 3333, 4444], "%");
CHECK_CLUSTER_USERS(session1, ["cluster1", "cluster2", "cluster3"], "%", 0);

CHECK_RECOVERY_USER(session1, 1);
CHECK_RECOVERY_USER(session2, 0);
CHECK_RECOVERY_USER(session3, 0);
CHECK_RECOVERY_USER(session4, 0);

//@<> check account deletion
c2.removeInstance(__sandbox_uri3);

CHECK_RECOVERY_USERS(session1, [1111, 2222, 4444], "%");
CHECK_CLUSTER_USERS(session1, ["cluster1", "cluster2", "cluster3"], "%", 0);

// check account deletion for cluster
cs.removeCluster("cluster3");
CHECK_RECOVERY_USERS(session1, [1111, 2222], "%");
CHECK_CLUSTER_USERS(session1, ["cluster1", "cluster2"], "%", 0);

// avoid slow wait when sb is started back
session2.runSql("set persist group_replication_start_on_boot=0");

// check account deletion for offline cluster
testutil.stopSandbox(__mysql_sandbox_port2);
cs.removeCluster("cluster2", {force:1});

CHECK_RECOVERY_USERS(session1, [1111], "%");
CHECK_CLUSTER_USERS(session1, ["cluster1"], "%", 0);

testutil.startSandbox(__mysql_sandbox_port2);
session2 = mysql.getSession(__sandbox_uri2);

//@<> change the option and readd replica clusters
reset_instance(session2); 
reset_instance(session3);
reset_instance(session4);

c2 = cs.createReplicaCluster(__sandbox_uri2, "cluster2", {recoveryMethod:"clone"});
session2 = mysql.getSession(__sandbox_uri2);

cs.setOption("replicationAllowedHost", hostname_ip);

EXPECT_EQ([{"option": "replicationAllowedHost", "value": hostname_ip}], cs.options()["clusterSet"]["globalOptions"]);

c2.addInstance(__sandbox_uri3, {recoveryMethod:"clone"});
session3 = mysql.getSession(__sandbox_uri3);

c3 = cs.createReplicaCluster(__sandbox_uri4, "cluster3", {recoveryMethod:"clone"} );
session4 = mysql.getSession(__sandbox_uri4);

CHECK_RECOVERY_USERS(session1, [1111, 2222, 3333, 4444], "%");
CHECK_CLUSTER_USERS(session1, ["cluster1", "cluster2", "cluster3"], hostname_ip, 0);

CHECK_RECOVERY_USER(session1, 1);
CHECK_RECOVERY_USER(session2, 0);
CHECK_RECOVERY_USER(session3, 0);
CHECK_RECOVERY_USER(session4, 0);

cs.removeCluster("cluster2");

CHECK_RECOVERY_USERS(session1, [1111, 4444], "%");
CHECK_CLUSTER_USERS(session1, ["cluster1", "cluster3"], hostname_ip, 0);

CHECK_RECOVERY_USER(session1, 1);
CHECK_RECOVERY_USER(session4, 0);

// ---------------

//@<> Check ClusterSet created with replicationAllowedHost
reset_instance(session1);
reset_instance(session2); 
reset_instance(session3);
reset_instance(session4);

// workaround ClusterSet.createReplicaCluster: asynchronous_connection_failover_delete_managed UDF failed; Error no matching row was found to be deleted. (MySQL Error 3200)
testutil.destroySandbox(__mysql_sandbox_port2);
testutil.deploySandbox(__mysql_sandbox_port2, "root", {report_host:hostname_ip, server_id:"2222"});
session2 = mysql.getSession(__sandbox_uri2);

shell.connect(__sandbox_uri1);
c1 = dba.createCluster("cluster1", {replicationAllowedHost:hostname_ip});

cs = c1.createClusterSet("cs", {replicationAllowedHost:hostname_ip});

c2 = cs.createReplicaCluster(__sandbox_uri2, "cluster2", {recoveryMethod:"incremental"});

c2.addInstance(__sandbox_uri3, {recoveryMethod:"clone"});
session3 = mysql.getSession(__sandbox_uri3);

c3 = cs.createReplicaCluster(__sandbox_uri4, "cluster3", {recoveryMethod:"clone", replicationAllowedHost:hostname_ip});
session4 = mysql.getSession(__sandbox_uri4);

s = cs.status({extended:1});

EXPECT_EQ("OK_NO_TOLERANCE", s.clusters.cluster1.status);
EXPECT_EQ("OK_NO_TOLERANCE", s.clusters.cluster2.status);
EXPECT_EQ("OK_NO_TOLERANCE", s.clusters.cluster3.status);

session1.runSql("select user,host from mysql.user");

CHECK_RECOVERY_USERS(session1, [1111, 4444], hostname_ip, 2);
CHECK_RECOVERY_USERS(session1, [2222, 3333], "%", 2);
CHECK_CLUSTER_USERS(session1, ["cluster1", "cluster3"], hostname_ip, 1);
CHECK_CLUSTER_USERS(session1, ["cluster2"], "%", 2);

CHECK_RECOVERY_USER(session1, 1);
CHECK_RECOVERY_USER(session2, 0);
CHECK_RECOVERY_USER(session3, 0);
CHECK_RECOVERY_USER(session4, 0);


//@<> check options2
EXPECT_EQ([{"option": "replicationAllowedHost", "value": hostname_ip}], cs.options()["clusterSet"]["globalOptions"]);

EXPECT_EQ(hostname_ip, get_global_option(c1.options(), "replicationAllowedHost"));
EXPECT_EQ("%", get_global_option(c2.options(), "replicationAllowedHost"));
EXPECT_EQ(hostname_ip, get_global_option(c3.options(), "replicationAllowedHost"));

//@<> check primary switch2
cs.setPrimaryCluster("cluster2");

CHECK_RECOVERY_USERS(session1, [1111, 4444], hostname_ip, 2);
CHECK_RECOVERY_USERS(session1, [2222, 3333], "%", 2);
CHECK_RECOVERY_USERS(session1, [1111, 4444], hostname_ip, 2);
CHECK_RECOVERY_USERS(session1, [2222, 3333], "%", 2);

CHECK_RECOVERY_USER(session1, 0);
CHECK_RECOVERY_USER(session2, 1);
CHECK_RECOVERY_USER(session3, 1);
CHECK_RECOVERY_USER(session4, 0);

cs.setPrimaryCluster("cluster1");

CHECK_RECOVERY_USERS(session1, [1111, 4444], hostname_ip, 2);
CHECK_RECOVERY_USERS(session1, [2222, 3333], "%", 2);
CHECK_RECOVERY_USERS(session1, [1111, 4444], hostname_ip, 2);
CHECK_RECOVERY_USERS(session1, [2222, 3333], "%", 2);

CHECK_RECOVERY_USER(session1, 1);
CHECK_RECOVERY_USER(session2, 0);
CHECK_RECOVERY_USER(session3, 0);
CHECK_RECOVERY_USER(session4, 0);

//@<> check account deletion2
c2.removeInstance(__sandbox_uri3);

CHECK_RECOVERY_USERS(session1, [1111, 4444], hostname_ip, 1);
CHECK_RECOVERY_USERS(session1, [2222], "%", 2);
CHECK_CLUSTER_USERS(session1, ["cluster1", "cluster3"], hostname_ip, 1);
CHECK_CLUSTER_USERS(session1, ["cluster2"], "%", 2);

// check account deletion for cluster
cs.removeCluster("cluster3");

CHECK_RECOVERY_USERS(session1, [1111], hostname_ip, 1);
CHECK_RECOVERY_USERS(session1, [2222], "%", 1);
CHECK_CLUSTER_USERS(session1, ["cluster1"], hostname_ip, 1);
CHECK_CLUSTER_USERS(session1, ["cluster2"], "%", 1);

// check account deletion for offline cluster
testutil.stopSandbox(__mysql_sandbox_port2, {wait:1});
cs.removeCluster("cluster2", {force:1});

CHECK_RECOVERY_USERS(session1, [1111], hostname_ip);
CHECK_CLUSTER_USERS(session1, ["cluster1"], hostname_ip, 0);

testutil.startSandbox(__mysql_sandbox_port2);
session2 = mysql.getSession(__sandbox_uri2);

//@<> check changing via options
reset_instance(session2); 
reset_instance(session3);
reset_instance(session4);

c2 = cs.createReplicaCluster(__sandbox_uri2, "cluster2", {recoveryMethod:"clone"});
session2 = mysql.getSession(__sandbox_uri2);

EXPECT_EQ([{"option": "replicationAllowedHost", "value": hostname_ip}], cs.options()["clusterSet"]["globalOptions"]);

CHECK_CLUSTER_USERS(session1, ["cluster1", "cluster2"], hostname_ip, 0);

cs.setOption("replicationAllowedHost", "%");

EXPECT_EQ([{"option": "replicationAllowedHost", "value": "%"}], cs.options()["clusterSet"]["globalOptions"]);

c2.addInstance(__sandbox_uri3, {recoveryMethod:"clone"});
session3 = mysql.getSession(__sandbox_uri3);

c3 = cs.createReplicaCluster(__sandbox_uri4, "cluster3", {recoveryMethod:"clone"} );
session4 = mysql.getSession(__sandbox_uri4);

CHECK_CLUSTER_USERS(session1, ["cluster1", "cluster2", "cluster3"], "%", 0);

//<> Cleanup
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
testutil.destroySandbox(__mysql_sandbox_port3);
testutil.destroySandbox(__mysql_sandbox_port4);
