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
  dba.createCluster("tempCluster", {memberSslMode: "REQUIRED"});
else
  dba.createCluster("tempCluster", {memberSslMode: "DISABLED"});

session.getSchema('mysql_innodb_cluster_metadata');

//@# drop metadata: force false
dba.dropMetadataSchema({force:false});

// Enable super_read_only to test this scenario
session.runSql('SET GLOBAL super_read_only = 1');

//@ Dba.dropMetadataSchema: super-read-only error (BUG#26422638)
dba.dropMetadataSchema({force:true});

//@# drop metadata: force true and clearReadOnly
dba.dropMetadataSchema({force:true, clearReadOnly: true});
ensure_schema_does_not_exist(session, 'mysql_innodb_cluster_metadata')

// Smart deployment cleanup
if (deployed_here)
  cleanup_sandbox(__mysql_sandbox_port1);
