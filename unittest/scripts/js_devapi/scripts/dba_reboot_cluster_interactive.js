// Assumptions: smart deployment routines available

//@<> Skip tests in 8.0.4 to not trigger GR plugin deadlock {VER(==8.0.4)}
testutil.skip("Reboot tests freeze in 8.0.4 because of bug in GR");

//@ Initialization
testutil.deploySandbox(__mysql_sandbox_port1, 'root', {'report_host': hostname});
testutil.deploySandbox(__mysql_sandbox_port2, 'root', {'report_host': hostname});
testutil.deploySandbox(__mysql_sandbox_port3, 'root', {'report_host': hostname});

// Update __have_ssl and other with the real instance SSL support.
// NOTE: Workaround BUG#25503817 to display the right ssl info for status()
update_have_ssl(__mysql_sandbox_port1);

shell.connect(__sandbox_uri1);
var clusterSession = session;

//@<OUT> create cluster
if (__have_ssl)
  var cluster = dba.createCluster('dev', {memberSslMode:'REQUIRED'});
else
  var cluster = dba.createCluster('dev', {memberSslMode:'DISABLED'});

testutil.waitMemberState(__mysql_sandbox_port1, "ONLINE");

session.close();
// session is stored on the cluster object so changing the global session should not affect cluster operations
shell.connect(__sandbox_uri2);

cluster.status();

//@ Add instance 2
cluster.addInstance(__sandbox_uri2);

// Waiting for the second added instance to become online
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");

//@ Add instance 3
cluster.addInstance(__sandbox_uri3);

// Waiting for the third added instance to become online
testutil.waitMemberState(__mysql_sandbox_port3, "ONLINE");

//@ Dba.rebootClusterFromCompleteOutage errors
dba.rebootClusterFromCompleteOutage("dev");
dba.rebootClusterFromCompleteOutage("dev", {invalidOpt: "foobar"});

// Kill all the instances

// Kill instance 2
testutil.killSandbox(__mysql_sandbox_port2);

// Since the cluster has quorum, the instance will be kicked off the
// Cluster going OFFLINE->UNREACHABLE->(MISSING)
testutil.waitMemberState(__mysql_sandbox_port2, "(MISSING)");

// Kill instance 3
testutil.killSandbox(__mysql_sandbox_port3);

// Waiting for the third added instance to become unreachable
// Will remain unreachable since there's no quorum to kick it off
testutil.waitMemberState(__mysql_sandbox_port3, "UNREACHABLE");

// Kill instance 1
testutil.killSandbox(__mysql_sandbox_port1);

// Re-start all the instances except instance 3

// Start instance 2
testutil.startSandbox(__mysql_sandbox_port2);
//the timeout for GR plugin to install a new view is 60s, so it should be at
// least that value the parameter for the timeout for the waitForDelayedGRStart
testutil.waitForDelayedGRStart(__mysql_sandbox_port2, 'root', 0);

// Start instance 1
testutil.startSandbox(__mysql_sandbox_port1);
testutil.waitForDelayedGRStart(__mysql_sandbox_port1, 'root', 0);

session.close();
cluster.disconnect();

// Re-establish the connection to instance 1
shell.connect(__sandbox_uri1);

var instance2 = localhost + ':' + __mysql_sandbox_port2;
var instance3 = localhost + ':' + __mysql_sandbox_port3;

//@ Dba.rebootClusterFromCompleteOutage error unreachable server cannot be on the rejoinInstances list
cluster = dba.rebootClusterFromCompleteOutage("dev", {rejoinInstances: [instance3]});

//@ Dba.rebootClusterFromCompleteOutage error cannot use same server on both rejoinInstances and removeInstances list
cluster = dba.rebootClusterFromCompleteOutage("dev", {rejoinInstances: [instance2], removeInstances: [instance2]});

//@ Dba.rebootClusterFromCompleteOutage success
// The answers to the prompts of the rebootCluster command
testutil.expectPrompt("Would you like to rejoin it to the cluster? [y/N]: ", "y");
testutil.expectPrompt("Would you like to remove it from the cluster's metadata? [y/N]: ", "y");

cluster = dba.rebootClusterFromCompleteOutage("dev", {clearReadOnly: true});

// Waiting for the second added instance to become online
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");

//@<OUT> cluster status after reboot
cluster.status();

// Start instance 3
testutil.startSandbox(__mysql_sandbox_port3);

// Add instance 3 back to the cluster
testutil.waitForDelayedGRStart(__mysql_sandbox_port3, 'root', 0);

//@ Rescan cluster to add instance 3 back to metadata {VER(>=8.0.11)}
// if server version is greater than 8.0.11 then the GR settings will be
// persisted on instance 3 and it will rejoin the cluster that has been
// rebooted. We just need to add it back to the metadata.
testutil.expectPrompt("Would you like to add it to the cluster metadata? [Y/n]: ", "y");
testutil.expectPassword("Please provide the password for 'root@" + hostname + ":" + __mysql_sandbox_port3 + "': ", "root");
// TODO Remove this user creation as soon as issue with cluster.rescan is fixed
// Create root@% user since the rescan wizard will try to authenticate using the
// hostname read from Group Replication as there is no metadata, i.e.
// root@<hostname> but that user does not exist, so the cluster.rescan will fail
var s3 = mysql.getSession("mysql://root:root@localhost:"+__mysql_sandbox_port3);
create_root_from_anywhere(s3, true);
s3.close();
uri3 = hostname + ":"  + __mysql_sandbox_port3;
cluster.rescan();

//@ Add instance 3 back to the cluster {VER(<8.0.11)}
// if server version is smaller than 8.0.11 then no GR settings will be persisted
// on instance 3, such as gr_start_on_boot and gr_group_seeds so it will not
// automatically rejoin the cluster. We need to manually add it back.
cluster.addInstance(__sandbox_uri3);

// Waiting for the third added instance to become online
testutil.waitMemberState(__mysql_sandbox_port3, "ONLINE");

//@ Dba.rebootClusterFromCompleteOutage regression test for BUG#25516390

// Kill all the instances

// Kill instance 2
testutil.killSandbox(__mysql_sandbox_port2);

// Since the cluster has quorum, the instance will be kicked off the
// Cluster going OFFLINE->UNREACHABLE->(MISSING)
testutil.waitMemberState(__mysql_sandbox_port2, "(MISSING)");

// Kill instance 3
testutil.killSandbox(__mysql_sandbox_port3);

// Waiting for the third added instance to become unreachable
// Will remain unreachable since there's no quorum to kick it off
testutil.waitMemberState(__mysql_sandbox_port3, "UNREACHABLE");
session.close();

// Kill instance 1
testutil.killSandbox(__mysql_sandbox_port1);

// Re-start all the instances

// Start instance 2
testutil.startSandbox(__mysql_sandbox_port2);
testutil.waitForDelayedGRStart(__mysql_sandbox_port2, 'root', 0);

// Start instance 1
testutil.startSandbox(__mysql_sandbox_port1);
testutil.waitForDelayedGRStart(__mysql_sandbox_port1, 'root', 0);

// Start instance 3
testutil.startSandbox(__mysql_sandbox_port3);
testutil.waitForDelayedGRStart(__mysql_sandbox_port3, 'root', 0);

// Re-establish the connection to instance 1
shell.connect(__sandbox_uri1);

cluster.disconnect();
cluster = dba.rebootClusterFromCompleteOutage("dev", {removeInstances: [uri2, uri3]});

session.close();

//@ Finalization
clusterSession.close();
cluster.disconnect();
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
testutil.destroySandbox(__mysql_sandbox_port3);
