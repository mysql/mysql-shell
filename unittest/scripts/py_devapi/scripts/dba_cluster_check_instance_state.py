# Assumptions: smart deployment rountines available
#@ Initialization
deployed_here = reset_or_deploy_sandboxes()

#@ Connect
shell.connect({'scheme': 'mysql', 'host': localhost, 'port': __mysql_sandbox_port1, 'user': 'root', 'password': 'root'})

#@ create cluster
if __have_ssl:
  cluster = dba.create_cluster('dev', {'memberSslMode': 'REQUIRED'})
else:
  cluster = dba.create_cluster('dev', {'memberSslMode': 'DISABLED'})

#@<OUT> check_instance_state: two arguments
cluster.check_instance_state('root@localhost:%d' % __mysql_sandbox_port1, 'root')

#@<OUT> check_instance_state: single argument
cluster.check_instance_state('root:root@localhost:%d' % __mysql_sandbox_port1)

#@ Failure: no arguments
cluster.check_instance_state()

#@ Failure: more than two arguments
cluster.check_instance_state('root@localhost:%d' % __mysql_sandbox_port1, 'root', '')

#@ Adding instance
add_instance_to_cluster(cluster, __mysql_sandbox_port2);

# Waiting for the second added instance to become online
wait_slave_state(cluster, uri2, "ONLINE");

#@<OUT> check_instance_state: two arguments - added instance
cluster.check_instance_state('root@localhost:%d' % __mysql_sandbox_port2, 'root')

#@<OUT> check_instance_state: single argument - added instance
cluster.check_instance_state('root:root@localhost:%d' % __mysql_sandbox_port2)

#@ Failure: no arguments - added instance
cluster.check_instance_state()

#@ Failure: more than two arguments - added instance
cluster.check_instance_state('root@localhost:%d' % __mysql_sandbox_port2, 'root', '')

session.close()

#@ Finalization
# Will delete the sandboxes ONLY if this test was executed standalone
if (deployed_here):
  cleanup_sandboxes(True)
