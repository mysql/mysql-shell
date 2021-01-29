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
    EXPECT_EQ(1, row["Enabled_ssl"]);
  } else if (ssl_mode == "DISABLED") {
    EXPECT_EQ(0, row["Enabled_ssl"]);
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

// TODO add another instance to a replica

CHECK_ASYNC_CHANNEL_SSL(session4, "REQUIRED");
CHECK_ASYNC_CHANNEL_SSL(session5, "REQUIRED");

// TODO switch primary and check again

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

CHECK_ASYNC_CHANNEL_SSL(session4, "DISABLED");
CHECK_ASYNC_CHANNEL_SSL(session5, "DISABLED");

// TODO switch primary and check again

//@<> Create replica clusters, check SSL mode also DISABLED

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

CHECK_ASYNC_CHANNEL_SSL(session4, "REQUIRED");
CHECK_ASYNC_CHANNEL_SSL(session5, "REQUIRED");

// TODO switch primary and check again

//@<> Create clusterset with SSL mode REQUIRED

//@<> Create replica clusters, check SSL mode also REQUIRED



//@<> Cleanup
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
testutil.destroySandbox(__mysql_sandbox_port4);
testutil.destroySandbox(__mysql_sandbox_port5);
testutil.destroySandbox(__mysql_sandbox_port6);
