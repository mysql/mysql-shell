# Assumptions: wait_slave_state is defined

#@ Dba: create_cluster multiMaster, ok
if __have_ssl:
    dba.create_cluster('devCluster', {'multiMaster': True, 'force': True, 'memberSslMode': 'REQUIRED', 'clearReadOnly': True})
else:
    dba.create_cluster('devCluster', {'multiMaster': True, 'force': True, 'memberSslMode': 'DISABLED', 'clearReadOnly': True})

cluster = dba.get_cluster('devCluster')

#@ Cluster: add_instance 2
add_instance_to_cluster(cluster, __mysql_sandbox_port2)

wait_slave_state(Cluster, uri2, "ONLINE")

#@ Cluster: add_instance 3
add_instance_to_cluster(cluster, __mysql_sandbox_port3)

wait_slave_state(cluster, uri3, "ONLINE")

#@<OUT> Cluster: describe cluster with instance
cluster.describe()

#@<OUT> Cluster: status cluster with instance
cluster.status()

#@ Cluster: remove_instance 2
cluster.remove_instance({'host': 'localhost', 'port': __mysql_sandbox_port2})

#@<OUT> Cluster: describe removed member
cluster.describe()

#@<OUT> Cluster: status removed member
cluster.status()

#@ Cluster: remove_instance 3
cluster.remove_instance(uri3)

#@ Cluster: Error cannot remove last instance
cluster.remove_instance(uri1)

#@ Dissolve cluster with success
cluster.dissolve({'force': True})

#@ Dba: create_cluster multiMaster 2, ok
if __have_ssl:
    dba.create_cluster('devCluster', {'multiMaster': True, 'force': True, 'memberSslMode': 'REQUIRED', 'clearReadOnly': True})
else:
    dba.create_cluster('devCluster', {'multiMaster': True, 'force': True, 'memberSslMode': 'DISABLED', 'clearReadOnly': True})

cluster = dba.get_cluster('devCluster')

#@ Cluster: add_instance 2
add_instance_to_cluster(cluster, __mysql_sandbox_port2)

wait_slave_state(cluster, uri2, "ONLINE")

#@ Cluster: add_instance 3
add_instance_to_cluster(cluster, __mysql_sandbox_port3)

wait_slave_state(cluster, uri3, "ONLINE")

#@<OUT> Cluster: status: success
cluster.status()

# Rejoin tests

#@# Dba: stop instance 3
if __sandbox_dir:
  dba.stop_sandbox_instance(__mysql_sandbox_port3, {'sandboxDir': __sandbox_dir, 'password': 'root'})
else:
  dba.stop_sandbox_instance(__mysql_sandbox_port3, {'password': 'root'})

wait_slave_state(cluster, uri3, ["(MISSING)"])

# start instance 3
try_restart_sandbox(__mysql_sandbox_port3)

#@ Cluster: rejoin_instance errors
cluster.rejoin_instance()
cluster.rejoin_instance(1,2,3)
cluster.rejoin_instance(1)
cluster.rejoin_instance({'host': 'localhost'})
cluster.rejoin_instance("somehost:3306")

#@#: Dba: rejoin instance 3 ok
if __have_ssl:
  cluster.rejoin_instance({'dbUser': 'root', 'host': 'localhost', 'port': __mysql_sandbox_port3, 'password': 'root'}, {'memberSslMode': 'REQUIRED'})
else:
  cluster.rejoin_instance({'dbUser': 'root', 'host': 'localhost', 'port': __mysql_sandbox_port3, 'password': 'root'}, {'memberSslMode': 'DISABLED'})

wait_slave_state(cluster, uri3, "ONLINE")

# Verify if the cluster is OK

#@<OUT> Cluster: status for rejoin: success
cluster.status()

cluster.dissolve({'force': True})
