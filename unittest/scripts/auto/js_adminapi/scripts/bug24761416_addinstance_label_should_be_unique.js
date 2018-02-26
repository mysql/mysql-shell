// deploy sandboxes
testutil.deploySandbox(__mysql_sandbox_port1, 'root');
testutil.deploySandbox(__mysql_sandbox_port2, 'root');
testutil.deploySandbox(__mysql_sandbox_port3, 'root');

// connect to first instance
shell.connect(__sandbox_uri1);

//@ create cluster on first instance
var cluster = dba.createCluster('c');

//@ add second instance with label
cluster.addInstance(__sandbox_uri2, {label: 'node1'});
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");

//@<OUT> check status (1)
cluster.status();

//@ third instance is valid for cluster usage
dba.checkInstanceConfiguration(__sandbox_uri3);

//@ add third instance with duplicated label
cluster.addInstance(__sandbox_uri3, {label: 'node1'});

//@<OUT> check status (2)
cluster.status();

//@ third instance is still valid for cluster usage
dba.checkInstanceConfiguration(__sandbox_uri3);

//@ add third instance with unique label
cluster.addInstance(__sandbox_uri3, {label: 'node2'});
testutil.waitMemberState(__mysql_sandbox_port3, "ONLINE");

//@<OUT> check status (3)
cluster.status();

// Cleanup
cluster.disconnect();
session.close();

testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
testutil.destroySandbox(__mysql_sandbox_port3);
