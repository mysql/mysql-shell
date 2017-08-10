// Assumptions: smart deployment rountines available
//@ Initialization
var deployed_here = reset_or_deploy_sandboxes();

// Update __have_ssl and other with the real instance SSL support.
// NOTE: Workaround BUG#25503817 to display the right ssl info for status()
update_have_ssl(__mysql_sandbox_port1);

shell.connect({scheme: 'mysql', host: localhost, port: __mysql_sandbox_port1, user: 'root', password: 'root'});
var clusterSession = session;

//@<OUT> create cluster
if (__have_ssl)
  var cluster = dba.createCluster('dev', {memberSslMode:'REQUIRED'});
else
  var cluster = dba.createCluster('dev', {memberSslMode:'DISABLED'});

// session is stored on the cluster object so changing the global session should not affect cluster operations
shell.connect({scheme: 'mysql', host: "localhost", port: __mysql_sandbox_port2, user: 'root', password: 'root'})

cluster.status();

//@ Add instance 2
add_instance_to_cluster(cluster, __mysql_sandbox_port2);

// Waiting for the second added instance to become online
wait_slave_state(cluster, uri2, "ONLINE");

//@ Add instance 3
add_instance_to_cluster(cluster, __mysql_sandbox_port3);

// Waiting for the third added instance to become online
wait_slave_state(cluster, uri3, "ONLINE");

//@ Dba.rebootClusterFromCompleteOutage errors
dba.rebootClusterFromCompleteOutage("");
dba.rebootClusterFromCompleteOutage("dev", {invalidOpt: "foobar"});
dba.rebootClusterFromCompleteOutage("dev2");
dba.rebootClusterFromCompleteOutage("dev");

// Kill all the instances

// Kill instance 2
if (__sandbox_dir)
  dba.killSandboxInstance(__mysql_sandbox_port2, {sandboxDir:__sandbox_dir});
else
  dba.killSandboxInstance(__mysql_sandbox_port2);

// Since the cluster has quorum, the instance will be kicked off the
// Cluster going OFFLINE->UNREACHABLE->(MISSING)
wait_slave_state(cluster, uri2, "(MISSING)");

// Kill instance 3
if (__sandbox_dir)
  dba.killSandboxInstance(__mysql_sandbox_port3, {sandboxDir:__sandbox_dir});
else
  dba.killSandboxInstance(__mysql_sandbox_port3);

// Waiting for the third added instance to become unreachable
// Will remain unreachable since there's no quorum to kick it off
wait_slave_state(cluster, uri3, "UNREACHABLE");

// Kill instance 1
if (__sandbox_dir)
  dba.killSandboxInstance(__mysql_sandbox_port1, {sandboxDir:__sandbox_dir});
else
  dba.killSandboxInstance(__mysql_sandbox_port1);

// Re-start all the instances except instance 3

// Start instance 2
if (__sandbox_dir)
  dba.startSandboxInstance(__mysql_sandbox_port2, {sandboxDir:__sandbox_dir});
else
  dba.startSandboxInstance(__mysql_sandbox_port2);

// Start instance 1
if (__sandbox_dir)
  dba.startSandboxInstance(__mysql_sandbox_port1, {sandboxDir:__sandbox_dir});
else
  dba.startSandboxInstance(__mysql_sandbox_port1);

session.close();

// Re-establish the connection to instance 1
shell.connect({scheme: 'mysql', host: localhost, port: __mysql_sandbox_port1, user: 'root', password: 'root'});

var instance2 = localhost + ':' + __mysql_sandbox_port2;
var instance3 = localhost + ':' + __mysql_sandbox_port3;

//@ Dba.rebootClusterFromCompleteOutage error unreachable server cannot be on the rejoinInstances list
cluster = dba.rebootClusterFromCompleteOutage("dev", {rejoinInstances: [instance3]});

//@ Dba.rebootClusterFromCompleteOutage error cannot use same server on both rejoinInstances and removeInstances list
cluster = dba.rebootClusterFromCompleteOutage("dev", {rejoinInstances: [instance2], removeInstances: [instance2]});

//@ Dba.rebootClusterFromCompleteOutage: super-read-only error (BUG#26422638)
// Since we killed the instance, we have to enable super_read_only to test this scenario
session.runSql('SET GLOBAL super_read_only = 1');
cluster = dba.rebootClusterFromCompleteOutage("dev", {rejoinInstances: [instance2], removeInstances: [instance3]});

//@ Dba.rebootClusterFromCompleteOutage success
cluster = dba.rebootClusterFromCompleteOutage("dev", {rejoinInstances: [instance2], removeInstances: [instance3], clearReadOnly: true});

// Waiting for the second added instance to become online
wait_slave_state(cluster, uri2, "ONLINE");

//@<OUT> cluster status after reboot
cluster.status();
session.close();

//@ Finalization
// Will close opened sessions and delete the sandboxes ONLY if this test was executed standalone
clusterSession.close();
if (deployed_here)
  cleanup_sandboxes(true);
