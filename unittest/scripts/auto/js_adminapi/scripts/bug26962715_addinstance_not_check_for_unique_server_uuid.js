// Deploy instances (with the same server UUID).
testutil.deploySandbox(__mysql_sandbox_port1, "root", {"server-uuid": "aaaaaaaa-bbbb-cccc-dddd-eeeeeeeeeeee"});
testutil.deploySandbox(__mysql_sandbox_port2, "root", {"server-uuid": "aaaaaaaa-bbbb-cccc-dddd-eeeeeeeeeeee"});

//@ Create a cluster a cluster.
shell.connect(__sandbox_uri1);
var cluster = dba.createCluster("test_cluster");

//@ Adding an instance with the same server UUID fails.
cluster.addInstance(__sandbox_uri2);

// Clean-up deployed instances.
session.close();
cluster.disconnect();
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
