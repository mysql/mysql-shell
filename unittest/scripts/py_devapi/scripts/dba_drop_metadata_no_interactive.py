# Assumptions: smart deployment functions available

# Smart deployment
deployed_here = reset_or_deploy_sandbox(__mysql_sandbox_port1)

shell.connect({'user':'root', 'password': 'root', 'host':'localhost', 'port':__mysql_sandbox_port1});

#@# Invalid drop_metadata_schema call
dba.drop_metadata_schema(1,2,3,4,5);

#@# drop metadata: no arguments
dba.drop_metadata_schema()

#@# create cluster
if __have_ssl:
  dba.create_cluster("tempCluster")
else:
  dba.create_cluster("tempCluster", {'memberSsl': False})

session.get_schema('mysql_innodb_cluster_metadata')

#@# drop metadata: force false
dba.drop_metadata_schema({'force':False});

#@# drop metadata: force true
dba.drop_metadata_schema({'force':True});
ensure_schema_does_not_exist(session, 'mysql_innodb_cluster_metadata')

session.close()

if deployed_here:
  cleanup_sandbox(__mysql_sandbox_port1)
else:
  reset_or_deploy_sandbox(__mysql_sandbox_port1)

