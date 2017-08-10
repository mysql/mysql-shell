# Assumptions: smart deployment functions available

# Smart deployment
deployed_here = reset_or_deploy_sandbox(__mysql_sandbox_port1)

shell.connect({'scheme': 'mysql', 'user':'root', 'password': 'root', 'host':'localhost', 'port':__mysql_sandbox_port1});

if __have_ssl:
  dba.create_cluster("tempCluster", {"memberSslMode": "REQUIRED"})
else:
  dba.create_cluster("tempCluster", {"memberSslMode": "DISABLED"})

#@# Invalid drop_metadata_schema call
dba.drop_metadata_schema(1,2,3,4,5);
dba.drop_metadata_schema({'not_valid':True});

#@# drop metadata: no user response
dba.drop_metadata_schema()

#@# drop metadata: user response no
dba.drop_metadata_schema()

#@# drop metadata: force option
dba.drop_metadata_schema({'force':False});
session.get_schema('mysql_innodb_cluster_metadata');
dba.drop_metadata_schema({'force':True});

session.close()

reset_or_deploy_sandbox(__mysql_sandbox_port1)
shell.connect({'scheme': 'mysql', 'user':'root', 'password': 'root', 'host':'localhost', 'port':__mysql_sandbox_port1});
if __have_ssl:
  dba.create_cluster("tempCluster", {"memberSslMode": "REQUIRED"})
else:
  dba.create_cluster("tempCluster", {"memberSslMode": "DISABLED"})

# Enable super_read_only to test this scenario
session.run_sql('SET GLOBAL super_read_only = 1');

#@<OUT> Dba.drop_metadata_schema: super-read-only error (BUG#26422638)
dba.drop_metadata_schema()

#@<OUT> drop metadata: user response yes to force and clearReadOnly
dba.drop_metadata_schema()

session.close()

if deployed_here:
  cleanup_sandbox(__mysql_sandbox_port1)
else:
  reset_or_deploy_sandbox(__mysql_sandbox_port1)

