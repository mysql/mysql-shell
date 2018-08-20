//@ Initialize
testutil.deploySandbox(__mysql_sandbox_port1, "root");
testutil.deploySandbox(__mysql_sandbox_port2, "root");

shell.connect(__sandbox_uri1);
var cluster = dba.createCluster("c");
cluster.addInstance(__sandbox_uri2);
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");
cluster.status();

//@ No-op
cluster.rescan();

//@ Instance in group but not MD
session.runSql("delete from mysql_innodb_cluster_metadata.instances where instance_name=?", ["localhost:"+__mysql_sandbox_port2]);
cluster.status();

//@ addInstance should fail and suggest a rescan
cluster.addInstance(__sandbox_uri2);

// rescan should re-add the instance (but it doesn't: Bug #28529362)
//cluster.rescan();
//cluster.status();

//@ Finalize
cluster.disconnect();
session.close();
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
