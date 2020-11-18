// deploy sandboxes
testutil.deploySandbox(__mysql_sandbox_port1, 'root', {report_host: hostname});
testutil.deploySandbox(__mysql_sandbox_port2, 'root', {report_host: hostname});
testutil.deploySandbox(__mysql_sandbox_port3, 'root', {report_host: hostname});

// connect to first instance
shell.connect(__sandbox_uri1);

//@ create cluster on first instance
var cluster = dba.createCluster('c', {gtidSetIsComplete: true});

//@ add second instance with label
cluster.addInstance(__sandbox_uri2, {label: '1node1'});
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");

//@<> check status (1)
var status = cluster.status();
EXPECT_EQ(status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port1}`]["status"], "ONLINE");
EXPECT_EQ(status["defaultReplicaSet"]["topology"]["1node1"]["status"], "ONLINE");
EXPECT_EQ(status["defaultReplicaSet"]["topology"]["1node1"]["address"], `${hostname}:${__mysql_sandbox_port2}`);

//@ third instance is valid for cluster usage
dba.checkInstanceConfiguration(__sandbox_uri3);

//@ add third instance with duplicated label
cluster.addInstance(__sandbox_uri3, {label: '1node1'});

//@<> check status (2)
var status = cluster.status();
EXPECT_EQ(status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port1}`]["status"], "ONLINE");
EXPECT_EQ(status["defaultReplicaSet"]["topology"]["1node1"]["status"], "ONLINE");
EXPECT_EQ(status["defaultReplicaSet"]["topology"]["1node1"]["address"], `${hostname}:${__mysql_sandbox_port2}`);

//@ third instance is still valid for cluster usage
dba.checkInstanceConfiguration(__sandbox_uri3);

//@ add third instance with unique label
cluster.addInstance(__sandbox_uri3, {label: '1node2'});
testutil.waitMemberState(__mysql_sandbox_port3, "ONLINE");

//@<> check status (3)
var status = cluster.status();
EXPECT_EQ(status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port1}`]["status"], "ONLINE");
EXPECT_EQ(status["defaultReplicaSet"]["topology"]["1node1"]["status"], "ONLINE");
EXPECT_EQ(status["defaultReplicaSet"]["topology"]["1node1"]["address"], `${hostname}:${__mysql_sandbox_port2}`);
EXPECT_EQ(status["defaultReplicaSet"]["topology"]["1node2"]["status"], "ONLINE");
EXPECT_EQ(status["defaultReplicaSet"]["topology"]["1node2"]["address"], `${hostname}:${__mysql_sandbox_port3}`);

// Cleanup
cluster.disconnect();
session.close();

testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
testutil.destroySandbox(__mysql_sandbox_port3);
