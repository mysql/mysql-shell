// Assumptions: smart deployment rountines available
//@ Initialization
testutil.deploySandbox(__mysql_sandbox_port1, "root");
testutil.deploySandbox(__mysql_sandbox_port2, "root");
testutil.deploySandbox(__mysql_sandbox_port3, "root");

//@ Create single-primary cluster
shell.connect(__sandbox_uri1);
var singleSession = session;

if (__have_ssl)
  var single = dba.createCluster('single', {memberSslMode: 'REQUIRED'});
else
  var single = dba.createCluster('single', {memberSslMode: 'DISABLED'});

//@ Success adding instance 2
testutil.waitMemberState(__mysql_sandbox_port1, "ONLINE");
single.addInstance(__sandbox_uri2);

// Waiting for the added instance to become online
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");

// Wait for the second added instance to fetch all the replication data
testutil.waitMemberTransactions(__mysql_sandbox_port2);

//@ Success adding instance 3
single.addInstance(__sandbox_uri3);

// Waiting for the added instance to become online
testutil.waitMemberState(__mysql_sandbox_port3, "ONLINE");

// Wait for the third added instance to fetch all the replication data
testutil.waitMemberTransactions(__mysql_sandbox_port3);

//@ Cluster.dissolve no force error
single.dissolve();

//@ Success dissolving single-primary cluster
single.dissolve({force: true});

//@ Cluster.dissolve already dissolved
single.dissolve();

shell.connect(__sandbox_uri1);
var multiSession = session;

//@ Create multi-primary cluster
if (__have_ssl)
  var multi = dba.createCluster('multi', {multiPrimary: true, memberSslMode: 'REQUIRED', clearReadOnly: true, force: true});
else
  var multi = dba.createCluster('multi', {multiPrimary: true, memberSslMode: 'DISABLED', clearReadOnly: true, force: true});

//@ Success adding instance 2 mp
testutil.waitMemberState(__mysql_sandbox_port1, "ONLINE");
multi.addInstance(__sandbox_uri2)

// Waiting for the added instance to become online
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");

// Wait for the second added instance to fetch all the replication data
testutil.waitMemberTransactions(__mysql_sandbox_port2);

//@ Success adding instance 3 mp
multi.addInstance(__sandbox_uri3)

// Waiting for the added instance to become online
testutil.waitMemberState(__mysql_sandbox_port3, "ONLINE");

// Wait for the third added instance to fetch all the replication data
testutil.waitMemberTransactions(__mysql_sandbox_port3);

//@ Success dissolving multi-primary cluster
multi.dissolve({force: true});

//@ Create single-primary cluster 2
shell.connect(__sandbox_uri1);
var singleSession2 = session;

if (__have_ssl)
  var single2 = dba.createCluster('single2', {memberSslMode: 'REQUIRED', clearReadOnly: true});
else
  var single2 = dba.createCluster('single2', {memberSslMode: 'DISABLED', clearReadOnly: true});

//@ Success adding instance 2 2
testutil.waitMemberState(__mysql_sandbox_port1, "ONLINE");
single2.addInstance(__sandbox_uri2);

// Waiting for the added instance to become online
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");

// Wait for the second added instance to fetch all the replication data
testutil.waitMemberTransactions(__mysql_sandbox_port2);

//@ Success adding instance 3 2
single2.addInstance(__sandbox_uri3);

// Waiting for the added instance to become online
testutil.waitMemberState(__mysql_sandbox_port3, "ONLINE");

// Wait for the third added instance to fetch all the replication data
testutil.waitMemberTransactions(__mysql_sandbox_port3);

// stop instance 3
// Use stop sandbox instance to make sure the instance is gone before restarting it
testutil.stopSandbox(__mysql_sandbox_port3);

testutil.waitMemberState(__mysql_sandbox_port3, "(MISSING)");

// Regression test for BUG#26001653
//@ Success dissolving cluster 2
single2.dissolve({force: true});

// start instance 3
testutil.startSandbox(__mysql_sandbox_port3);
//the timeout for GR plugin to install a new view is 60s, so it should be at
// least that value the parameter for the timeout for the waitForDelayedGRStart
testutil.waitForDelayedGRStart(__mysql_sandbox_port3, 'root', 0);

//@ Create multi-primary cluster 2
shell.connect(__sandbox_uri1);
var multiSession2 = session;

if (__have_ssl)
  var multi2 = dba.createCluster('multi2', {memberSslMode: 'REQUIRED', clearReadOnly: true, multiPrimary: true, force: true});
else
  var multi2 = dba.createCluster('multi2', {memberSslMode: 'DISABLED', clearReadOnly: true, multiPrimary: true, force: true});

//@ Success adding instance 2 mp 2
testutil.waitMemberState(__mysql_sandbox_port1, "ONLINE");
multi2.addInstance(__sandbox_uri2);

// Waiting for the added instance to become online
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");

// Wait for the second added instance to fetch all the replication data
testutil.waitMemberTransactions(__mysql_sandbox_port2);

//@ Success adding instance 3 mp 2
multi2.addInstance(__sandbox_uri3);

// Waiting for the added instance to become online
testutil.waitMemberState(__mysql_sandbox_port3, "ONLINE");

// Wait for the third added instance to fetch all the replication data
testutil.waitMemberTransactions(__mysql_sandbox_port3);

// stop instance 3
// Use stop sandbox instance to make sure the instance is gone before restarting it
testutil.stopSandbox(__mysql_sandbox_port3);

testutil.waitMemberState(__mysql_sandbox_port3, "(MISSING)");

// Regression test for BUG#26001653
//@ Success dissolving multi-primary cluster 2
multi2.dissolve({force: true});

//@ Finalization
// Will close opened sessions and delete the sandboxes ONLY if this test was executed standalone
singleSession.close();
multiSession.close();
singleSession2.close();
multiSession2.close();

testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
testutil.destroySandbox(__mysql_sandbox_port3);
