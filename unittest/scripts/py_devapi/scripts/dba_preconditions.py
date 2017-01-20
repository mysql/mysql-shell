# Assumptions: smart deployment rountines available
#@ Initialization
deployed_here = reset_or_deploy_sandboxes()

#@<OUT> Standalone Instance : check instance config
shell.connect({'host': localhost, 'port': __mysql_sandbox_port1, 'user': 'root', 'password': 'root'})
dba.check_instance_configuration({'host': localhost, 'port': __mysql_sandbox_port1, 'password':'root'})

#@<OUT> Standalone Instance : config local instance
dba.config_local_instance({'host': localhost, 'port': __mysql_sandbox_port1, 'password':'root'}, {'mycnfPath': 'mybad.cnf'})

#@<OUT> Standalone Instance: create cluster
if __have_ssl:
  cluster = dba.create_cluster('dev', {'memberSslMode': 'REQUIRED'})
else:
  cluster = dba.create_cluster('dev')

cluster.status()
session.close()

#@ Standalone Instance: Failed preconditions
shell.connect({'host': localhost, 'port': __mysql_sandbox_port2, 'user': 'root', 'password': 'root'})

dba.get_cluster()
dba.drop_metadata_schema({'force':  True})
cluster.add_instance({'host': localhost, 'port': __mysql_sandbox_port2})
cluster.rejoin_instance({'host': localhost, 'port': __mysql_sandbox_port2})
cluster.remove_instance({'host': localhost, 'port': __mysql_sandbox_port2})
cluster.describe()
cluster.status()
cluster.dissolve()
cluster.check_instance_state({'host': localhost, 'port': __mysql_sandbox_port2, 'password': 'root'})
cluster.rescan()
session.close()

#@ Read Only Instance : get cluster
shell.connect({'host': localhost, 'port': __mysql_sandbox_port1, 'user': 'root', 'password': 'root'})

add_instance_to_cluster(cluster, __mysql_sandbox_port2)

# Waiting for the second added instance to become online
wait_slave_state(cluster, uri2, "ONLINE")
session.close()

shell.connect({'host': localhost, 'port': __mysql_sandbox_port2, 'user': 'root', 'password': 'root'})
cluster = dba.get_cluster()

#@<OUT> Read Only Instance : check instance config
dba.check_instance_configuration({'host': localhost, 'port': __mysql_sandbox_port3, 'password':'root'})

#@<OUT> Read Only Instance : config local instance
dba.config_local_instance({'host': localhost, 'port': __mysql_sandbox_port3, 'password':'root'}, {'mycnfPath': 'mybad.cnf'})

#@<OUT> Read Only Instance : check instance state
cluster.check_instance_state({'host': localhost, 'port': __mysql_sandbox_port3, 'password': 'root'})

#@ Read Only Instance : rejoin instance
cluster.rejoin_instance({'host': localhost, 'port': __mysql_sandbox_port3, 'password':'root'})

#@<OUT> Read Only Instance : describe
cluster.describe()

#@<OUT> Read Only Instance : status
cluster.status()

#@ Read Only: Failed preconditions
dba.create_cluster('sample')
dba.drop_metadata_schema({'force':  True})
cluster.add_instance({'host': localhost, 'port': __mysql_sandbox_port3})
cluster.remove_instance({'host': localhost, 'port': __mysql_sandbox_port3})
cluster.dissolve()
cluster.rescan()
session.close()

#@ Preparation for quorumless cluster tests
shell.connect({'host': localhost, 'port': __mysql_sandbox_port1, 'user': 'root', 'password': 'root'})
cluster = dba.get_cluster()

if __sandbox_dir:
  dba.kill_sandbox_instance(__mysql_sandbox_port2, {'sandboxDir':__sandbox_dir})
else:
  dba.kill_sandbox_instance(__mysql_sandbox_port2)

# Waiting for the second instance to become offline
wait_slave_state(cluster, uri2, ["UNREACHABLE", "OFFLINE"])

#@ Quorumless Cluster: Failed preconditions
dba.create_cluster('failed')
dba.drop_metadata_schema({'force':  True})
cluster.add_instance({'host': localhost, 'port': __mysql_sandbox_port3})
cluster.rejoin_instance({'host': localhost, 'port': __mysql_sandbox_port3})
cluster.remove_instance({'host': localhost, 'port': __mysql_sandbox_port3})
cluster.dissolve()
cluster.check_instance_state({'host': localhost, 'port': __mysql_sandbox_port3, 'password': 'root'})
cluster.rescan()

#@ Quorumless Cluster: get cluster
cluster = dba.get_cluster()

#@<OUT> Quorumless Cluster : check instance config
dba.check_instance_configuration({'host': localhost, 'port': __mysql_sandbox_port3, 'password':'root'})

#@<OUT> Quorumless Cluster : config local instance
dba.config_local_instance({'host': localhost, 'port': __mysql_sandbox_port3, 'password':'root'}, {'mycnfPath': 'mybad.cnf'})

#@<OUT> Quorumless Cluster : describe
cluster.describe()

#@<OUT> Quorumless Cluster : status
cluster.status()
session.close()

#@ Preparation for unmanaged instance tests
reset_or_deploy_sandbox(__mysql_sandbox_port1)
reset_or_deploy_sandbox(__mysql_sandbox_port2)

shell.connect({'host': localhost, 'port': __mysql_sandbox_port1, 'user': 'root', 'password': 'root'})

if __have_ssl:
  cluster = dba.create_cluster('temporal', {'memberSslMode': 'REQUIRED'})
else:
  cluster = dba.create_cluster('temporal')

dba.drop_metadata_schema({'force': True})

#@ Unmanaged Instance: Failed preconditions
dba.create_cluster('failed')
dba.get_cluster()
dba.drop_metadata_schema({'force':  True})
cluster.add_instance({'host': localhost, 'port': __mysql_sandbox_port3})
cluster.rejoin_instance({'host': localhost, 'port': __mysql_sandbox_port3})
cluster.remove_instance({'host': localhost, 'port': __mysql_sandbox_port3})
cluster.describe()
cluster.status()
cluster.dissolve()
cluster.check_instance_state({'host': localhost, 'port': __mysql_sandbox_port3, 'password': 'root'})
cluster.rescan()

#@<OUT> Unmanaged Instance: create cluster
# TODO: Uncomment this test case once the hostname resolution is fixed for adoptFromGR

#cluster = dba.create_cluster('fromGR', {adoptFromGR:True})
#cluster.status()
session.close()


#@ XSession: Failed preconditions
shell.connect({'host': localhost, 'port': __mysql_sandbox_port1*10, 'user': 'root', 'password': 'root'})
dba.create_cluster('failed')
dba.get_cluster()
dba.drop_metadata_schema({'force':  True})
cluster.add_instance({'host': localhost, 'port': __mysql_sandbox_port3})
cluster.rejoin_instance({'host': localhost, 'port': __mysql_sandbox_port3})
cluster.remove_instance({'host': localhost, 'port': __mysql_sandbox_port3})
cluster.describe()
cluster.status()
cluster.dissolve()
cluster.check_instance_state({'host': localhost, 'port': __mysql_sandbox_port3, 'password': 'root'})
cluster.rescan()
session.close()

#@ Finalization
# Will delete the sandboxes ONLY if this test was executed standalone
if (deployed_here):
  cleanup_sandboxes(True)
else:
  reset_or_deploy_sandboxes()