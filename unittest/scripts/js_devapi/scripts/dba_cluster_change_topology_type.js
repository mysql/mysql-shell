// Assumptions: smart deployment routines available

//@ Initialization
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: hostname});
testutil.deploySandbox(__mysql_sandbox_port2, "root", {report_host: hostname});
testutil.deploySandbox(__mysql_sandbox_port3, "root", {report_host: hostname});

shell.connect(__sandbox_uri1);

//@ Create cluster (topology type: primary master by default)
if (__have_ssl)
  var cluster = dba.createCluster('pmCluster', {memberSslMode: 'REQUIRED'});
else
  var cluster = dba.createCluster('pmCluster', {memberSslMode: 'DISABLED'});

//@ Adding instances to cluster
testutil.waitMemberState(__mysql_sandbox_port1, "ONLINE");
cluster.addInstance(__sandbox_uri2);
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");

cluster.addInstance(__sandbox_uri3);
testutil.waitMemberState(__mysql_sandbox_port3, "ONLINE");

//@ Check topology type
var md_address1 = hostname + ":" + __mysql_sandbox_port1;
var res = session.runSql("SELECT topology_type " +
    "FROM mysql_innodb_cluster_metadata.replicasets r, mysql_innodb_cluster_metadata.instances i " +
    "WHERE r.replicaset_id = i.replicaset_id AND i.instance_name = '"+ md_address1 +"'");
var row = res.fetchOne();
print (row[0]);

//@<OUT> Check cluster status
cluster.status();

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

//@ Error checking cluster status
cluster.status();

//@ Update the topology type to 'mm'
session.runSql("UPDATE mysql_innodb_cluster_metadata.replicasets SET topology_type = 'mm'");
session.close();

cluster.disconnect();
// Reconnect to cluster
shell.connect({scheme:'mysql', host: localhost, port: __mysql_sandbox_port1, user: 'root', password: 'root'});
var cluster = dba.getCluster();
testutil.waitMemberState(__mysql_sandbox_port3, "ONLINE");

//@<OUT> Check cluster status is updated
cluster.status();

// Change topology type manually back to primary master (on all instances)
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

//@ Error checking cluster status again
cluster.status();

// Close session
session.close();
cluster.disconnect();

//@ Finalization
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
testutil.destroySandbox(__mysql_sandbox_port3);
