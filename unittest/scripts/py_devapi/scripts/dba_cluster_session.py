# Assumptions: smart deployment rountines available
#@ Initialization
deployed_here = reset_or_deploy_sandboxes()

shell.connect({'scheme': 'mysql', 'host': localhost, 'port': __mysql_sandbox_port1, 'user': 'root', 'password': 'root'})
cluster_session = session

#@<OUT> cluster status
if __have_ssl:
    cluster = dba.create_cluster('dev', {'memberSslMode': 'REQUIRED'})
else:
    cluster = dba.create_cluster('dev', {'memberSslMode': 'DISABLED'})

# session is stored on the cluster object so changing the global session should not affect cluster operations
shell.connect({'scheme': 'mysql', 'host': localhost, 'port': __mysql_sandbox_port2, 'user': 'root', 'password': 'root'})
session.close()

cluster.status()

#@ cluster session closed: error
shell.connect({'scheme': 'mysql', 'host': localhost, 'port': __mysql_sandbox_port1, 'user': 'root', 'password': 'root'})
cluster = dba.get_cluster()
session.close()

cluster.status()

#@ Finalization
#  Will close opened sessions and delete the sandboxes ONLY if this test was executed standalone
cluster_session.close()
if (deployed_here):
    cleanup_sandboxes(True)
