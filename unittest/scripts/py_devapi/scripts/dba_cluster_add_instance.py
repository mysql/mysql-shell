# Assumptions: smart deployment rountines available
#@ Initialization
deployed_here = reset_or_deploy_sandboxes()

#@ create first cluster
shell.connect({'scheme': 'mysql', 'host': localhost, 'port': __mysql_sandbox_port1, 'user': 'root', 'password': 'root'})
single_session = session

if __have_ssl:
  single = dba.create_cluster('single', {'memberSslMode':'REQUIRED'})
else:
  single = dba.create_cluster('single')

#@ Success adding instance
add_instance_to_cluster(single, __mysql_sandbox_port2)

# Waiting for the second added instance to become online
wait_slave_state(single, uri2, "ONLINE")

#@ create second cluster
shell.connect({'scheme': 'mysql', 'host': localhost, 'port': __mysql_sandbox_port3, 'user': 'root', 'password': 'root'})
multi_session = session

if __have_ssl:
  multi = dba.create_cluster('multi', {'memberSslMode':'REQUIRED', 'multiMaster':True, 'force':True})
else:
  multi = dba.create_cluster('multi', {'multiMaster':True, 'force':True})

#@ Failure adding instance from multi cluster into single
add_instance_options['port'] = __mysql_sandbox_port3
single.add_instance(add_instance_options)

# Drops the metadata on the multi cluster letting a non managed replication group
dba.drop_metadata_schema({'force':True})

#@ Failure adding instance from an unmanaged replication group into single
add_instance_options['port'] = __mysql_sandbox_port3
single.add_instance(add_instance_options)

#@ Failure adding instance already in the InnoDB cluster
add_instance_options['port'] = __mysql_sandbox_port2
single.add_instance(add_instance_options)

#@ Finalization
#  Will close opened sessions and delete the sandboxes ONLY if this test was executed standalone
single_session.close()
multi_session.close()
if deployed_here:
  cleanup_sandboxes(True)
