// Assumptions: smart deployment rountines available
//@ Initialization
var localhost = "localhost";
var deployed_here = reset_or_deploy_sandboxes();
var uri1 = localhost + ":" + __mysql_sandbox_port1;
var uri2 = localhost + ":" + __mysql_sandbox_port2;
var uri3 = localhost + ":" + __mysql_sandbox_port3;

shell.connect({host: localhost, port: __mysql_sandbox_port1, user: 'root', password: 'root'});

//@<OUT> create cluster
if (__have_ssl)
  var cluster = dba.createCluster('dev');
else
  var cluster = dba.createCluster('dev', {memberSsl:false});

cluster.status();

//@ Add instance 2
if (__have_ssl)
  cluster.addInstance({host:localhost, port: __mysql_sandbox_port2, password:'root'});
else
  cluster.addInstance({host:localhost, port: __mysql_sandbox_port2, password:'root', memberSsl:false});

// Waiting for the second added instance to become online
wait_slave_state(cluster, uri2, "ONLINE");

//@ Add instance 3
if (__have_ssl)
  cluster.addInstance({host:localhost, port: __mysql_sandbox_port3, password:'root'});
else
  cluster.addInstance({host:localhost, port: __mysql_sandbox_port3, password:'root', memberSsl:false});

// Waiting for the third added instance to become online
wait_slave_state(cluster, uri3, "ONLINE");

// Kill instance 2
if (__sandbox_dir)
  dba.killSandboxInstance(__mysql_sandbox_port2, {sandboxDir:__sandbox_dir});
else
  dba.killSandboxInstance(__mysql_sandbox_port2);

// Waiting for the second added instance to become unreachable
wait_slave_state(cluster, uri2, "UNREACHABLE");

// Kill instance 3
if (__sandbox_dir)
  dba.killSandboxInstance(__mysql_sandbox_port3, {sandboxDir:__sandbox_dir});
else
  dba.killSandboxInstance(__mysql_sandbox_port3);

// Waiting for the third added instance to become unreachable
wait_slave_state(cluster, uri3, "UNREACHABLE");

// Start instance 2
if (__sandbox_dir)
  dba.startSandboxInstance(__mysql_sandbox_port2, {sandboxDir:__sandbox_dir});
else
  dba.startSandboxInstance(__mysql_sandbox_port2);

// Start instance 3
if (__sandbox_dir)
  dba.startSandboxInstance(__mysql_sandbox_port3, {sandboxDir:__sandbox_dir});
else
  dba.startSandboxInstance(__mysql_sandbox_port3);

//@<OUT> Cluster status
cluster.status();

//@ Cluster.forceQuorumUsingPartitionOf errors
cluster.forceQuorumUsingPartitionOf();
cluster.forceQuorumUsingPartitionOf(1);
cluster.forceQuorumUsingPartitionOf("");
cluster.forceQuorumUsingPartitionOf(1, "");
cluster.forceQuorumUsingPartitionOf({host:localhost, port: __mysql_sandbox_port2, password:'root'});

//@ Cluster.forceQuorumUsingPartitionOf success
if (__have_ssl)
  cluster.forceQuorumUsingPartitionOf({host:localhost, port: __mysql_sandbox_port1, password:'root'});
else
  cluster.forceQuorumUsingPartitionOf({host:localhost, port: __mysql_sandbox_port1, password:'root', memberSsl:false});

//@<OUT> Cluster status after force quorum
cluster.status();

//@ Rejoin instance 2
if (__have_ssl)
  cluster.rejoinInstance({host:localhost, port: __mysql_sandbox_port2, password:'root'});
else
  cluster.rejoinInstance({host:localhost, port: __mysql_sandbox_port2, password:'root', memberSsl:false});

// Waiting for the second rejoined instance to become online
wait_slave_state(cluster, uri2, "ONLINE");

//@ Rejoin instance 3
if (__have_ssl)
  cluster.rejoinInstance({host:localhost, port: __mysql_sandbox_port3, password:'root'});
else
  cluster.rejoinInstance({host:localhost, port: __mysql_sandbox_port3, password:'root', memberSsl:false});

// Waiting for the third rejoined instance to become online
wait_slave_state(cluster, uri3, "ONLINE");

//@<OUT> Cluster status after rejoins
cluster.status();

session.close();

//@ Finalization
// Will delete the sandboxes ONLY if this test was executed standalone
if (deployed_here)
  cleanup_sandboxes(true);
else
  reset_or_deploy_sandboxes();
