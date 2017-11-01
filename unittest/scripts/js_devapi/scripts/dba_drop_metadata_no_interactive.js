// Assumptions: smart deployment functions available

// Smart deployment
var deployed_here = reset_or_deploy_sandbox(__mysql_sandbox_port1);

shell.connect({scheme: 'mysql', user:'root', password: 'root', host:'localhost', port:__mysql_sandbox_port1});

//@# Invalid dropMetadataSchema call
dba.dropMetadataSchema(1,2,3,4,5);

//@# drop metadata: no arguments
dba.dropMetadataSchema()

//@# create cluster
if (__have_ssl)
  dba.createCluster("tempCluster", {memberSslMode: "REQUIRED", clearReadOnly: true});
else
  dba.createCluster("tempCluster", {memberSslMode: "DISABLED", clearReadOnly: true});

session.getSchema('mysql_innodb_cluster_metadata');

//@# drop metadata: force false
dba.dropMetadataSchema({force:false});

//@# drop metadata: force true
dba.dropMetadataSchema({force:true});

ensure_schema_does_not_exist(session, 'mysql_innodb_cluster_metadata')

session.close();

// Smart deployment cleanup
if (deployed_here)
  cleanup_sandbox(__mysql_sandbox_port1);
