// Assumptions: smart deployment functions available

// Smart deployment
var deployed_here = reset_or_deploy_sandbox(__mysql_sandbox_port1);

shell.connect({scheme: 'mysql', user:'root', password: 'root', host:'localhost', port:__mysql_sandbox_port1});

// Assumptions: reset_or_deploy_sandboxes available
if (__have_ssl)
  dba.createCluster("tempCluster", {memberSslMode: "REQUIRED"});
else
  dba.createCluster("tempCluster", {memberSslMode: "DISABLED"});

//@# Invalid dropMetadataSchema call
dba.dropMetadataSchema(1,2,3,4,5);
dba.dropMetadataSchema({not_valid:true});

//@# drop metadata: no user response
dba.dropMetadataSchema()

//@# drop metadata: user response no
dba.dropMetadataSchema()

//@# drop metadata: force option
dba.dropMetadataSchema({force:false});
session.getSchema('mysql_innodb_cluster_metadata');
dba.dropMetadataSchema({force:true});

session.close();

reset_or_deploy_sandbox(__mysql_sandbox_port1);
shell.connect({scheme: 'mysql', user:'root', password: 'root', host:'localhost', port:__mysql_sandbox_port1});
if (__have_ssl)
  dba.createCluster("tempCluster", {memberSslMode: "REQUIRED"});
else
  dba.createCluster("tempCluster", {memberSslMode: "DISABLED"});

// Enable super_read_only to test this scenario
session.runSql('SET GLOBAL super_read_only = 1');

//@<OUT> Dba.dropMetadataSchema: super-read-only error (BUG#26422638)
dba.dropMetadataSchema()

//@<OUT> drop metadata: user response yes to force and clearReadOnly
dba.dropMetadataSchema()

session.close();

// Smart deployment cleanup
if (deployed_here)
  cleanup_sandbox(__mysql_sandbox_port1);

