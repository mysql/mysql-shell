// Assumptions: smart deployment routines available
var scene = new ClusterScenario([__mysql_sandbox_port1, __mysql_sandbox_port2, __mysql_sandbox_port3], {memberSslMode: 'REQUIRED', gtidSetIsComplete: true});
var cluster = scene.cluster;

//@<> Check topology type
var md_address1 = hostname + ":" + __mysql_sandbox_port1;
var res = session.runSql("SELECT primary_mode " +
    "FROM mysql_innodb_cluster_metadata.clusters r, mysql_innodb_cluster_metadata.instances i " +
    "WHERE r.cluster_id = i.cluster_id AND i.instance_name = '"+ md_address1 +"'");
var row = res.fetchOne();
EXPECT_EQ("pm", row[0]);

// Change topology type manually to multiprimary (on all instances)
session.runSql('STOP GROUP_REPLICATION');
session.runSql('SET GLOBAL group_replication_single_primary_mode=OFF');
session.close();
shell.connect({scheme:'mysql', host: localhost, port: __mysql_sandbox_port2, user: 'root', password: 'root'});
session.runSql('STOP GROUP_REPLICATION');
session.runSql('SET GLOBAL group_replication_single_primary_mode=OFF');
session.close();
shell.connect({scheme:'mysql', host: localhost, port: __mysql_sandbox_port3, user: 'root', password: 'root'});
session.runSql('STOP GROUP_REPLICATION');
session.runSql('SET GLOBAL group_replication_single_primary_mode=OFF');
session.close();

shell.connect({scheme:'mysql', host: localhost, port: __mysql_sandbox_port1, user: 'root', password: 'root'});
session.runSql('SET GLOBAL group_replication_bootstrap_group=ON');
session.runSql('START GROUP_REPLICATION');
session.runSql('SET GLOBAL group_replication_bootstrap_group=OFF');
session.close();
shell.connect({scheme:'mysql', host: localhost, port: __mysql_sandbox_port2, user: 'root', password: 'root'});
session.runSql('START GROUP_REPLICATION');
session.close();
shell.connect({scheme:'mysql', host: localhost, port: __mysql_sandbox_port3, user: 'root', password: 'root'});
session.runSql('START GROUP_REPLICATION');
session.close();

cluster.disconnect();
shell.connect({scheme:'mysql', host: localhost, port: __mysql_sandbox_port1, user: 'root', password: 'root'});
var cluster = dba.getCluster();

testutil.waitMemberTransactions(__mysql_sandbox_port2, __mysql_sandbox_port1);
testutil.waitMemberTransactions(__mysql_sandbox_port3, __mysql_sandbox_port1);

//@<> Error checking cluster status
EXPECT_THROWS(function() { cluster.status(); }, "The InnoDB Cluster topology type (Single-Primary) does not match the current Group Replication configuration (Multi-Primary).")

//@<> Update the topology type to 'mm'
session.runSql("UPDATE mysql_innodb_cluster_metadata.clusters SET primary_mode = 'mm'");
session.close();

cluster.disconnect();
// Reconnect to cluster
shell.connect({scheme:'mysql', host: localhost, port: __mysql_sandbox_port1, user: 'root', password: 'root'});
var cluster = dba.getCluster();
testutil.waitMemberState(__mysql_sandbox_port3, "ONLINE");

//@<> Check cluster status is updated
var status = cluster.status();
EXPECT_EQ("Multi-Primary", status["defaultReplicaSet"]["topologyMode"])
EXPECT_EQ("ONLINE", status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port1}`]["status"])
EXPECT_EQ("ONLINE", status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port2}`]["status"])
EXPECT_EQ("ONLINE", status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port3}`]["status"])
EXPECT_EQ("PRIMARY", status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port1}`]["memberRole"])
EXPECT_EQ("PRIMARY", status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port2}`]["memberRole"])
EXPECT_EQ("PRIMARY", status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port3}`]["memberRole"])
EXPECT_EQ("R/W", status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port1}`]["mode"])
EXPECT_EQ("R/W", status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port2}`]["mode"])
EXPECT_EQ("R/W", status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port3}`]["mode"])

//@<> Change topology type manually back to primary master (on all instances)
session.runSql('STOP GROUP_REPLICATION');
session.runSql('SET GLOBAL group_replication_single_primary_mode=ON');
session.close();
shell.connect({scheme:'mysql', host: localhost, port: __mysql_sandbox_port2, user: 'root', password: 'root'});
session.runSql('STOP GROUP_REPLICATION');
session.runSql('SET GLOBAL group_replication_single_primary_mode=ON');
session.close();
shell.connect({scheme:'mysql', host: localhost, port: __mysql_sandbox_port3, user: 'root', password: 'root'});
session.runSql('STOP GROUP_REPLICATION');
session.runSql('SET GLOBAL group_replication_single_primary_mode=ON');
session.close();

shell.connect({scheme:'mysql', host: localhost, port: __mysql_sandbox_port1, user: 'root', password: 'root'});
session.runSql('SET GLOBAL group_replication_bootstrap_group=ON');
session.runSql('START GROUP_REPLICATION');
session.runSql('SET GLOBAL group_replication_bootstrap_group=OFF');
session.close();
shell.connect({scheme:'mysql', host: localhost, port: __mysql_sandbox_port2, user: 'root', password: 'root'});
session.runSql('START GROUP_REPLICATION');
session.close();
shell.connect({scheme:'mysql', host: localhost, port: __mysql_sandbox_port3, user: 'root', password: 'root'});
session.runSql('START GROUP_REPLICATION');
session.close();

cluster.disconnect();
shell.connect({scheme:'mysql', host: localhost, port: __mysql_sandbox_port1, user: 'root', password: 'root'});
var cluster = dba.getCluster();

testutil.waitMemberTransactions(__mysql_sandbox_port2, __mysql_sandbox_port1);
testutil.waitMemberTransactions(__mysql_sandbox_port3, __mysql_sandbox_port1);

//@<> Error checking cluster status again
EXPECT_THROWS(function() { cluster.status(); }, "The InnoDB Cluster topology type (Multi-Primary) does not match the current Group Replication configuration (Single-Primary).")

//@<> Finalization
session.close();
cluster.disconnect();

testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
testutil.destroySandbox(__mysql_sandbox_port3);
