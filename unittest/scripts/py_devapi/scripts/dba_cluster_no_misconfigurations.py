# Assumptions: smart deployment routines available
#@ Initialization
deployed_here = reset_or_deploy_sandboxes();

shell.connect({'scheme': 'mysql', 'scheme': 'mysql', 'host': localhost, 'port': __mysql_sandbox_port1, 'user': 'root', 'password': 'root'})

#@ Dba.createCluster
if __have_ssl:
  cluster = dba.create_cluster('dev', {'memberSslMode':'REQUIRED'})
else:
  cluster = dba.create_cluster('dev', {'memberSslMode':'DISABLED'})

#@ Finalization
# Will delete the sandboxes ONLY if this test was executed standalone
if (deployed_here):
  cleanup_sandboxes(True)
