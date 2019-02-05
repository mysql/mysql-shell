// Deploy instances.
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: hostname});
testutil.deploySandbox(__mysql_sandbox_port2, "root", {report_host: hostname});
testutil.deploySandbox(__mysql_sandbox_port3, "root", {report_host: hostname});

//@ Create a cluster with 3 members.
shell.connect(__sandbox_uri1);
var cluster = dba.createCluster("test_cluster");

cluster.addInstance(__sandbox_uri2);
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");

cluster.addInstance(__sandbox_uri3);
testutil.waitMemberState(__mysql_sandbox_port3, "ONLINE");

//@ Stop GR on seed instance.
session.runSql('STOP GROUP_REPLICATION');

//@ Set GR seed with own GR local address
// NOTE: Used to ensure existing values are not overwritten by rejoinInstance.
//       A valid peer address will still be needed for GR seeds.
var result = session.runSql('SELECT @@group_replication_local_address');
var row = result.fetchOne();
var __seed_init_gr_seed = row[0];
print(row[0] + "\n");
session.runSql("SET GLOBAL group_replication_group_seeds = '"+__seed_init_gr_seed+"'");

//@ Connect to another member and get the cluster.
session.close();
cluster.disconnect();
shell.connect(__sandbox_uri2);
var cluster = dba.getCluster("test_cluster");

//@ Rejoin the seed instance.
cluster.rejoinInstance(__sandbox_uri1);

//@ Connect to first instance to check GR group seeds.
session.close();
cluster.disconnect();
shell.connect(__sandbox_uri1);

//@ Confirm previous GR seed value was not overwritten.
var result = session.runSql('SELECT @@group_replication_group_seeds');
var row = result.fetchOne();
print(row[0] + "\n");

//@ Dissolve cluster.
session.close();
shell.connect(__sandbox_uri2);
var cluster = dba.getCluster("test_cluster");
cluster.dissolve({force: true});
session.close();
cluster.disconnect();

// Clean-up deployed instances.
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
testutil.destroySandbox(__mysql_sandbox_port3);
