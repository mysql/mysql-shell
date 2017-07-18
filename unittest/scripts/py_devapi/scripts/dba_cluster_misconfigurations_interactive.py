# Assumptions: smart deployment rountines available
#@ Initialization
deployed_here = reset_or_deploy_sandboxes()

#@ connect and change vars
shell.connect({'scheme': 'mysql', 'scheme': 'mysql', 'host': localhost, 'port': __mysql_sandbox_port1, 'user': 'root', 'password': 'root'})
session.run_sql('SET GLOBAL binlog_checksum=CRC32');
session.run_sql('SET GLOBAL binlog_format=MIXED');

#@<OUT> Dba.createCluster: cancel
if __have_ssl:
  cluster = dba.create_cluster('dev', {'memberSslMode':'REQUIRED'})
else:
  cluster = dba.create_cluster('dev', {'memberSslMode':'DISABLED'})

#@<OUT> Dba.createCluster: ok
if __have_ssl:
  cluster = dba.create_cluster('dev', {'memberSslMode':'REQUIRED'})
else:
  cluster = dba.create_cluster('dev', {'memberSslMode':'DISABLED'})

#@ Finalization
# Will delete the sandboxes ONLY if this test was executed standalone
if (deployed_here):
  cleanup_sandboxes(True)
