# Assumptions: smart deployment functions available

# Smart deployment
deployed_here = reset_or_deploy_sandbox(__mysql_sandbox_port1)

shell.connect({'user':'root', 'password': 'root', 'host':'localhost', 'port':__mysql_sandbox_port1});

if __have_ssl:
  dba.create_cluster("tempCluster")
else:
  dba.create_cluster("tempCluster", {'memberSsl': False})

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

#@# drop metadata: user response yes
reset_or_deploy_sandbox(__mysql_sandbox_port1)

shell.connect({'user':'root', 'password': 'root', 'host':'localhost', 'port':__mysql_sandbox_port1});

if __have_ssl:
  dba.create_cluster("tempCluster")
else:
  dba.create_cluster("tempCluster", {'memberSsl': False})

dba.drop_metadata_schema()

session.close()

if deployed_here:
  cleanup_sandbox(__mysql_sandbox_port1)
else:
  reset_or_deploy_sandbox(__mysql_sandbox_port1)

