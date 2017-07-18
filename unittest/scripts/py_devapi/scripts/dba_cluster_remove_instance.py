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

#@ Adding instance
add_instance_to_cluster(cluster, __mysql_sandbox_port2);

# Waiting for the second added instance to become online
wait_slave_state(cluster, uri2, "ONLINE");

#@ Failure: remove_instance - invalid uri
cluster.remove_instance('@localhost:%d' % __mysql_sandbox_port2)

#@<OUT> Cluster status
cluster.status()

#@ Remove instance failure due to wrong credentials
cluster.remove_instance({'host': 'localhost', 'port': __mysql_sandbox_port2, 'user': 'foo', 'password': 'bar'});

#@<OUT> Cluster status after remove failed
cluster.status()

#@ Removing instance
cluster.remove_instance('root:root@localhost:%d' % __mysql_sandbox_port2)

#@<OUT> Cluster status after removal
cluster.status()

#@ Adding instance on port2 back
# Regression for BUG#24916064 : CAN NOT REMOVE STOPPED SERVER FROM A CLUSTER
add_instance_to_cluster(cluster, __mysql_sandbox_port2)

# Waiting for the instance on port2 to become online
# Regression for BUG#24916064 : CAN NOT REMOVE STOPPED SERVER FROM A CLUSTER
wait_slave_state(cluster, uri2, "ONLINE")

#@ Stop instance on port2
# Regression for BUG#24916064 : CAN NOT REMOVE STOPPED SERVER FROM A CLUSTER
if __sandbox_dir != '':
    dba.stop_sandbox_instance(__mysql_sandbox_port2, {"password": "root", "sandboxDir": __sandbox_dir})
else:
    dba.stop_sandbox_instance(__mysql_sandbox_port2, {"password": "root"})

# Waiting for the instance on port2 to be found missing
# Regression for BUG#24916064 : CAN NOT REMOVE STOPPED SERVER FROM A CLUSTER
wait_slave_state(cluster, uri2, "(MISSING)")

#@<OUT> Cluster status after instance on port2 is stopped
cluster.status()

#@ Error removing stopped instance on port2
# Regression for BUG#24916064 : CAN NOT REMOVE STOPPED SERVER FROM A CLUSTER
cluster.remove_instance('root:root@localhost:' + str(__mysql_sandbox_port2))

#@ Remove stopped instance on port2 with force option
# Regression for BUG#24916064 : CAN NOT REMOVE STOPPED SERVER FROM A CLUSTER
cluster.remove_instance('root@localhost:' + str(__mysql_sandbox_port2), {"force": True, "password": "root"})

#@<OUT> Cluster status after removal of instance on port2
cluster.status()

#@ Error removing last instance
# Regression for BUG#25226130 : REMOVAL OF SEED NODE BREAKS DISSOLVE
cluster.remove_instance('root:root@localhost:' + str(__mysql_sandbox_port1))

#@ Dissolve cluster with success
# Regression for BUG#25226130 : REMOVAL OF SEED NODE BREAKS DISSOLVE
cluster.dissolve({"force": True})

#@ Cluster re-created with success
# Regression for BUG#25226130 : REMOVAL OF SEED NODE BREAKS DISSOLVE
cluster = dba.create_cluster('dev')

session.close()

#@ Finalization
# Will delete the sandboxes ONLY if this test was executed standalone
if (deployed_here):
  cleanup_sandboxes(True)
