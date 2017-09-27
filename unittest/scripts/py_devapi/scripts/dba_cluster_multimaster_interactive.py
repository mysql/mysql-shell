# Assumptions: wait_slave_state is defined

#@<OUT> Dba: create_cluster multiMaster with interaction, cancel
dba.create_cluster('devCluster', {'multiMaster': True, 'clearReadOnly': True})

#@<OUT> Dba: create_cluster multiMaster with interaction, ok
if __have_ssl:
  dba.create_cluster('devCluster', {'multiMaster': True, 'memberSslMode': 'REQUIRED', 'clearReadOnly': True})
else:
  dba.create_cluster('devCluster', {'multiMaster': True, 'memberSslMode': 'DISABLED', 'clearReadOnly': True})

cluster = dba.get_cluster('devCluster')

#@ Cluster: add_instance with interaction, error
add_instance_options['port'] = __mysql_sandbox_port1

cluster.add_instance(add_instance_options, add_instance_extra_opts)

#@<OUT> Cluster: add_instance with interaction, ok
add_instance_to_cluster(cluster, __mysql_sandbox_port2)

wait_slave_state(cluster, uri2, "ONLINE")

#@<OUT> Cluster: add_instance 3 with interaction, ok
add_instance_to_cluster(cluster, __mysql_sandbox_port3)

wait_slave_state(cluster, uri3, "ONLINE")

#@<OUT> Cluster: describe1
cluster.describe()

#@<OUT> Cluster: status1
cluster.status()

#@ Cluster: remove_instance
cluster.remove_instance({'host': 'localhost', 'port': __mysql_sandbox_port2})

#@<OUT> Cluster: describe2
cluster.describe()

#@<OUT> Cluster: status2
cluster.status()

#@ Cluster: remove_instance 3
cluster.remove_instance({'host': 'localhost', 'port': __mysql_sandbox_port3})

#@ Cluster: Error cannot remove last instance
cluster.remove_instance({'host': 'localhost', 'port': __mysql_sandbox_port1})

#@ Dissolve cluster with success
cluster.dissolve({'force': True})

#@<OUT> Dba: create_cluster multiMaster with interaction 2, ok
if __have_ssl:
  dba.create_cluster('devCluster', {'multiMaster': True, 'memberSslMode': 'REQUIRED', 'clearReadOnly': True})
else:
  dba.create_cluster('devCluster', {'multiMaster': True, 'memberSslMode': 'DISABLED', 'clearReadOnly': True})

cluster = dba.get_cluster('devCluster')

#@<OUT> Cluster: add_instance with interaction, ok 2
add_instance_to_cluster(cluster, __mysql_sandbox_port2)

wait_slave_state(cluster, uri2, "ONLINE")

#@<OUT> Cluster: add_instance with interaction, ok 3
add_instance_to_cluster(cluster, __mysql_sandbox_port3)

wait_slave_state(cluster, uri3, "ONLINE")

#@<OUT> Cluster: status: success
cluster.status()

# Rejoin tests

#@# Dba: kill instance 3
if __sandbox_dir:
  dba.kill_sandbox_instance(__mysql_sandbox_port3, {"sandboxDir":__sandbox_dir})
else:
  dba.kill_sandbox_instance(__mysql_sandbox_port3)

# XCOM needs time to kick out the member of the group. The GR team has a patch to fix this
# But won't be available for the GA release. So we need to wait until the instance is reported
# as offline
wait_slave_state(cluster, uri3, ["(MISSING)"])

#@# Dba: start instance 3
if __sandbox_dir:
  dba.start_sandbox_instance(__mysql_sandbox_port3, {"sandboxDir": __sandbox_dir})
else:
  dba.start_sandbox_instance(__mysql_sandbox_port3)

#@: Cluster: rejoin_instance errors
cluster.rejoin_instance();
cluster.rejoin_instance(1,2,3);
cluster.rejoin_instance(1);
cluster.rejoin_instance({'host': "localhost"});
cluster.rejoin_instance({'host': "localhost", 'schema': "abs", 'authMethod':56});
cluster.rejoin_instance("somehost:3306");

#@<OUT> Cluster: rejoin_instance with interaction, ok
if __have_ssl:
  cluster.rejoin_instance({'dbUser': 'root', 'host': 'localhost', 'port': __mysql_sandbox_port3}, {'memberSslMode': 'REQUIRED'})
else:
  cluster.rejoin_instance({'dbUser': 'root', 'host': 'localhost', 'port': __mysql_sandbox_port3}, {'memberSslMode': 'DISABLED'})

wait_slave_state(cluster, uri3, "ONLINE")

# Verify if the cluster is OK

#@<OUT> Cluster: status for rejoin: success
cluster.status()

cluster.dissolve({'force': True})
