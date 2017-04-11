// Assumptions: smart deployment rountines available
//@ Initialization
var deployed_here = reset_or_deploy_sandboxes();

//@ Connect
shell.connect({scheme: 'mysql', host: localhost, port: __mysql_sandbox_port1, user: 'root', password: 'root'});

//@ create cluster
if (__have_ssl)
  var cluster = dba.createCluster('dev', {memberSslMode: 'REQUIRED'});
else
  var cluster = dba.createCluster('dev');

//@<OUT> checkInstanceState: two arguments
cluster.checkInstanceState('root@localhost:' + __mysql_sandbox_port1, 'root')

//@<OUT> checkInstanceState: single argument
cluster.checkInstanceState('root:root@localhost:' + __mysql_sandbox_port1)

//@ Failure: no arguments
cluster.checkInstanceState()

//@ Failure: more than two arguments
cluster.checkInstanceState('root@localhost:' + __mysql_sandbox_port1, 'root', '')

//@ Adding instance
add_instance_to_cluster(cluster, __mysql_sandbox_port2);

// Waiting for the second added instance to become online
wait_slave_state(cluster, uri2, "ONLINE");

//@<OUT> checkInstanceState: two arguments - added instance
cluster.checkInstanceState('root@localhost:' + __mysql_sandbox_port2, 'root')

//@<OUT> checkInstanceState: single argument - added instance
cluster.checkInstanceState('root:root@localhost:' + __mysql_sandbox_port2)

//@ Failure: no arguments - added instance
cluster.checkInstanceState()

//@ Failure: more than two arguments - added instance
cluster.checkInstanceState('root@localhost:' + __mysql_sandbox_port2, 'root', '')

session.close();

//@ Finalization
// Will delete the sandboxes ONLY if this test was executed standalone
if (deployed_here)
  cleanup_sandboxes(true);
