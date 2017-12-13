// Assumptions: smart deployment rountines available

// This does basic, high-level validation checks for WL11251
// Basically, ensuring that cluster change operations are possible even
// while connected to a secondary
// Related lower-level and more detailed tests are in cluster_session_sp
// cluster_session_mp, cluster_session_noquorum

//@ Init
testutil.deploySandbox(__mysql_sandbox_port1, "root");
testutil.deploySandbox(__mysql_sandbox_port2, "root");
testutil.deploySandbox(__mysql_sandbox_port3, "root");

shell.connect(__sandbox_uri1);
var cluster = dba.createCluster("clus");
testutil.waitMemberState(__mysql_sandbox_port1, "ONLINE");
cluster.addInstance(__sandbox_uri2);
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");

session.close();
cluster.disconnect();

//@ Connect to secondary, add instance, remove instance (should work)
shell.connect(__sandbox_uri2);
var cluster = dba.getCluster("clus");
cluster.addInstance(__sandbox_uri3);
testutil.waitMemberState(__mysql_sandbox_port3, "ONLINE");
cluster.removeInstance(__sandbox_uri3);

session.close();
cluster.disconnect();

// No warnings expected about read only
EXPECT_OUTPUT_NOT_CONTAINS("WARNING: You are connected to an instance in state 'Read Only'");

//@ Connect to secondary without redirect, add instance, remove instance (fail)
shell.connect(__sandbox_uri2);
var cluster = dba.getCluster("clus", {connectToPrimary: false});

// warning expected about read only
EXPECT_OUTPUT_CONTAINS("WARNING: You are connected to an instance in state 'Read Only'");

//@ addInstance should fail
cluster.addInstance(__sandbox_uri3);

//@ removeInstance should fail
cluster.removeInstance(__sandbox_uri3);

//@ status should work
cluster.status();

session.close();
cluster.disconnect();

//@ Fini
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
testutil.destroySandbox(__mysql_sandbox_port3);
