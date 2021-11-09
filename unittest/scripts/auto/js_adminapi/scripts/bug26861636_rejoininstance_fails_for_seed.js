// Deploy instances.
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: hostname});
testutil.deploySandbox(__mysql_sandbox_port2, "root", {report_host: hostname});
testutil.deploySandbox(__mysql_sandbox_port3, "root", {report_host: hostname});

//@<> Create a cluster with 3 members.
shell.connect(__sandbox_uri1);
var cluster = dba.createCluster("test_cluster", {gtidSetIsComplete: true});

cluster.addInstance(__sandbox_uri2);
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");

cluster.addInstance(__sandbox_uri3);
testutil.waitMemberState(__mysql_sandbox_port3, "ONLINE");

//@<> Stop GR on seed instance.
session.runSql('STOP GROUP_REPLICATION');

//@<> Connect to another member and get the cluster.
session.close();
cluster.disconnect();
shell.connect(__sandbox_uri2);
var cluster = dba.getCluster("test_cluster");

//@<> Rejoin the seed instance.
cluster.rejoinInstance(__sandbox_uri1);

//@<> Connect to first instance to check GR group seeds.
session.close();
cluster.disconnect();
shell.connect(__sandbox_uri1);

var result = session.runSql('SELECT @@group_replication_group_seeds');
var row = result.fetchOne();
EXPECT_EQ(`${hostname}:${__mysql_sandbox_gr_port2},${hostname}:${__mysql_sandbox_gr_port3}`, row[0]);

//@<> Clean-up deployed instances.
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
testutil.destroySandbox(__mysql_sandbox_port3);
