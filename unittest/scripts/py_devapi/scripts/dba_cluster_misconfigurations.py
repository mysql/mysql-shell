# Assumptions: smart deployment rountines available
#@ Initialization
deployed_here = reset_or_deploy_sandboxes();

shell.connect({'scheme': 'mysql', 'scheme': 'mysql', 'host': localhost, 'port': __mysql_sandbox_port1, 'user': 'root', 'password': 'root'})

# Change dynamic variables manually
session.run_sql('SET GLOBAL binlog_checksum=CRC32');
session.run_sql('SET GLOBAL binlog_format=MIXED');

#@ Dba.createCluster
if __have_ssl:
  cluster = dba.create_cluster('dev', {'memberSslMode':'REQUIRED'})
else:
  cluster = dba.create_cluster('dev')

#@ Finalization
# Will delete the sandboxes ONLY if this test was executed standalone
if (deployed_here):
  cleanup_sandboxes(True)
