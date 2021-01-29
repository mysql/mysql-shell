//@ {VER(>=8.0.27)}

//@<> INCLUDE clusterset_utils.inc

//@<> Setup
testutil.deploySandbox(__mysql_sandbox_port1, "root");
testutil.deploySandbox(__mysql_sandbox_port2, "root");
testutil.deploySandbox(__mysql_sandbox_port4, "root");
testutil.deploySandbox(__mysql_sandbox_port5, "root");

session1 = mysql.getSession(__sandbox_uri1);
session2 = mysql.getSession(__sandbox_uri2);
session4 = mysql.getSession(__sandbox_uri4);
session5 = mysql.getSession(__sandbox_uri5);


function CHECK_ASYNC_CHANNEL_SSL(session, ssl_mode) {
  var row = session.runSql("select * from mysql.slave_master_info where channel_name='clusterset_replication'").fetchOne();
  EXPECT_NE(null, row, "channel configured");

  if (ssl_mode == "REQUIRED") {
    EXPECT_EQ(1, row["Enabled_ssl"], "Enabled_SSL");
  } else if (ssl_mode == "DISABLED") {
    EXPECT_EQ(0, row["Enabled_ssl"], "Enabled_SSL");
  } else {
    EXPECT_FALSE("bad ssl_mode "+ssl_mode);
  }
}

function CHECK_ASYNC_CHANNEL_SSL_AT_SOURCE(session, cluster_name, ssl_mode) {
  var repl_user = session.runSql("select attributes->>'$.replicationAccountUser' from mysql_innodb_cluster_metadata.clusters where cluster_name=?", [cluster_name]).fetchOne()[0];

  var row = session.runSql("select * from performance_schema.threads where processlist_command='Binlog Dump GTID' and processlist_user=?", [repl_user]).fetchOne();
  println(row);
  EXPECT_NE(null, row, "binlog dump thread exists");
  thread_id = row[0];

  var status = session.runSql("select * from performance_schema.status_by_thread where thread_id=? and variable_name='Ssl_cipher'", [thread_id]).fetchOne();
  EXPECT_NE(null, status, "thread status exists");
  println("SSL status for ", cluster_name, "=", status);

  if (ssl_mode == "REQUIRED") {
    EXPECT_NE("", status[2], "session SSL status");
  } else if (ssl_mode == "DISABLED") {
    EXPECT_EQ("", status[2], "session SSL status");
  } else {
    EXPECT_FALSE("bad ssl_mode "+ssl_mode);
  }
}


//@<> Create cluster
shell.connect(__sandbox_uri1);
c = dba.createCluster("cluster");
c.addInstance(__sandbox_uri2, {recoveryMethod:"incremental"});

//@<> Create clusterset with SSL, check all is REQUIRED by default (since server has SSL)

cs = c.createClusterSet("cs");
c2 = cs.createReplicaCluster(__sandbox_uri4, "cluster2", {recoveryMethod:"incremental"});
c3 = cs.createReplicaCluster(__sandbox_uri5, "cluster3", {recoveryMethod:"clone"});
session5 = mysql.getSession(__sandbox_uri5);

CHECK_ASYNC_CHANNEL_SSL_AT_SOURCE(session1, "cluster2", "REQUIRED");
CHECK_ASYNC_CHANNEL_SSL_AT_SOURCE(session1, "cluster3", "REQUIRED");

CHECK_ASYNC_CHANNEL_SSL(session4, "REQUIRED");
CHECK_ASYNC_CHANNEL_SSL(session5, "REQUIRED");

//@<> replicas should follow primary after promoting instance2 in cluster
c.setPrimaryInstance(__sandbox_uri2);
uuid = session2.runSql("select @@server_uuid").fetchOne()[0];

// wait for replicas to reconnect
wait_channel_ready(session4, "clusterset_replication", uuid);
wait_channel_ready(session5, "clusterset_replication", uuid);

CHECK_ASYNC_CHANNEL_SSL_AT_SOURCE(session2, "cluster2", "REQUIRED");
CHECK_ASYNC_CHANNEL_SSL_AT_SOURCE(session2, "cluster3", "REQUIRED");

CHECK_ASYNC_CHANNEL_SSL(session4, "REQUIRED");
CHECK_ASYNC_CHANNEL_SSL(session5, "REQUIRED");


//@<> after promoting cluster2 - default required
cs.setPrimaryCluster("cluster2");

CHECK_ASYNC_CHANNEL_SSL_AT_SOURCE(session4, "cluster", "REQUIRED");
CHECK_ASYNC_CHANNEL_SSL_AT_SOURCE(session4, "cluster3", "REQUIRED");

CHECK_ASYNC_CHANNEL_SSL(session1, "REQUIRED");
CHECK_ASYNC_CHANNEL_SSL(session5, "REQUIRED");

//@<> Create clusterset with SSL mode DISABLED
reset_instance(session1);
reset_instance(session2);
reset_instance(session4);
reset_instance(session5);

c = dba.createCluster("cluster");
c.addInstance(__sandbox_uri2, {recoveryMethod:"incremental"});

cs = c.createClusterSet("cs", {clusterSetReplicationSslMode:"DISABLED"});
c2 = cs.createReplicaCluster(__sandbox_uri4, "cluster2", {recoveryMethod:"incremental"});
c3 = cs.createReplicaCluster(__sandbox_uri5, "cluster3", {recoveryMethod:"clone"});
session5 = mysql.getSession(__sandbox_uri5);

CHECK_ASYNC_CHANNEL_SSL_AT_SOURCE(session1, "cluster2", "DISABLED");
CHECK_ASYNC_CHANNEL_SSL_AT_SOURCE(session1, "cluster3", "DISABLED");

CHECK_ASYNC_CHANNEL_SSL(session4, "DISABLED");
CHECK_ASYNC_CHANNEL_SSL(session5, "DISABLED");

//@<> after promoting cluster2 - disabled
cs.setPrimaryCluster("cluster2");

CHECK_ASYNC_CHANNEL_SSL_AT_SOURCE(session4, "cluster", "DISABLED");
CHECK_ASYNC_CHANNEL_SSL_AT_SOURCE(session4, "cluster3", "DISABLED");

CHECK_ASYNC_CHANNEL_SSL(session, "DISABLED");
CHECK_ASYNC_CHANNEL_SSL(session5, "DISABLED");


//@<> explicitly REQUIRED ssl mode

reset_instance(session1);
reset_instance(session2);
reset_instance(session4);
reset_instance(session5);

c = dba.createCluster("cluster");
c.addInstance(__sandbox_uri2, {recoveryMethod:"incremental"});

cs = c.createClusterSet("cs", {clusterSetReplicationSslMode:"REQUIRED"});
c2 = cs.createReplicaCluster(__sandbox_uri4, "cluster2", {recoveryMethod:"incremental"});
c3 = cs.createReplicaCluster(__sandbox_uri5, "cluster3", {recoveryMethod:"clone"});
session5 = mysql.getSession(__sandbox_uri5);

CHECK_ASYNC_CHANNEL_SSL_AT_SOURCE(session1, "cluster2", "REQUIRED");
CHECK_ASYNC_CHANNEL_SSL_AT_SOURCE(session1, "cluster3", "REQUIRED");

CHECK_ASYNC_CHANNEL_SSL(session4, "REQUIRED");
CHECK_ASYNC_CHANNEL_SSL(session5, "REQUIRED");

//@<> after promoting cluster2 - REQUIRED
cs.setPrimaryCluster("cluster2");

CHECK_ASYNC_CHANNEL_SSL_AT_SOURCE(session4, "cluster", "REQUIRED");
CHECK_ASYNC_CHANNEL_SSL_AT_SOURCE(session4, "cluster3", "REQUIRED");

CHECK_ASYNC_CHANNEL_SSL(session, "REQUIRED");
CHECK_ASYNC_CHANNEL_SSL(session5, "REQUIRED");


//@<> Cleanup
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
testutil.destroySandbox(__mysql_sandbox_port4);
testutil.destroySandbox(__mysql_sandbox_port5);
testutil.destroySandbox(__mysql_sandbox_port6);
