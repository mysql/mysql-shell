# Assumptions: smart deployment rountines available
#@ Initialization
deployed_here = reset_or_deploy_sandboxes();

#@ connect
shell.connect({'scheme': 'mysql', 'scheme': 'mysql', 'host': localhost, 'port': __mysql_sandbox_port1, 'user': 'root', 'password': 'root'})

#@<OUT> create cluster no misconfiguration: ok
if __have_ssl:
  cluster = dba.create_cluster('dev', {'memberSslMode':'REQUIRED'})
else:
  cluster = dba.create_cluster('dev')

session.close()

#@ Finalization
# Will delete the sandboxes ONLY if this test was executed standalone
if (deployed_here):
  cleanup_sandboxes(True)
