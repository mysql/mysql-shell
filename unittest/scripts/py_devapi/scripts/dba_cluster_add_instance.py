# Assumptions: smart deployment rountines available
#@ Initialization
deployed_here = reset_or_deploy_sandboxes()

#@ create first cluster
shell.connect({'host': localhost, 'port': __mysql_sandbox_port1, 'user': 'root', 'password': 'root'})

if __have_ssl:
  single = dba.create_cluster('single', {'memberSslMode':'REQUIRED'})
else:
  single = dba.create_cluster('single')

#@ Success adding instance
add_instance_to_cluster(single, __mysql_sandbox_port2)

# Waiting for the second added instance to become online
wait_slave_state(single, uri2, "ONLINE")

session.close()

#@ create second cluster
shell.connect({'host': localhost, 'port': __mysql_sandbox_port3, 'user': 'root', 'password': 'root'})

if __have_ssl:
  multi = dba.create_cluster('multi', {'memberSslMode':'REQUIRED', 'multiMaster':True, 'force':True})
else:
  multi = dba.create_cluster('multi', {'multiMaster':True, 'force':True})

session.close()

#@ Failure adding instance from multi cluster into single
shell.connect({'host': localhost, 'port': __mysql_sandbox_port1, 'user': 'root', 'password': 'root'})
cluster = dba.get_cluster()
add_instance_options['port'] = __mysql_sandbox_port3
cluster.add_instance(add_instance_options)
session.close()

# Drops the metadata on the multi cluster letting a non managed replication group
shell.connect({'host': localhost, 'port': __mysql_sandbox_port3, 'user': 'root', 'password': 'root'})
dba.drop_metadata_schema({'force':True})
session.close()

#@ Failure adding instance from an unmanaged replication group into single
shell.connect({'host': localhost, 'port': __mysql_sandbox_port1, 'user': 'root', 'password': 'root'})
cluster = dba.get_cluster()
add_instance_options['port'] = __mysql_sandbox_port3
cluster.add_instance(add_instance_options)

#@ Failre adding instance already in the InnoDB cluster
add_instance_options['port'] = __mysql_sandbox_port2
cluster.add_instance(add_instance_options)

session.close()

#@ Finalization
# Will delete the sandboxes ONLY if this test was executed standalone
if deployed_here:
  cleanup_sandboxes(True)
