// Assumptions: smart deployment functions available

// Smart deployment
var deployed_here = reset_or_deploy_sandbox(__mysql_sandbox_port1);

shell.connect({scheme: 'mysql', user:'root', password: 'root', host:'localhost', port:__mysql_sandbox_port1});

// Assumptions: reset_or_deploy_sandboxes available
if (__have_ssl)
  dba.createCluster("tempCluster", {memberSslMode: "REQUIRED"});
else
  dba.createCluster("tempCluster");

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

//@# drop metadata: user response yes
reset_or_deploy_sandbox(__mysql_sandbox_port1);
shell.connect({scheme: 'mysql', user:'root', password: 'root', host:'localhost', port:__mysql_sandbox_port1});
if (__have_ssl)
  dba.createCluster("tempCluster", {memberSslMode: "REQUIRED"})
else
  dba.createCluster("tempCluster")

dba.dropMetadataSchema()

session.close();

// Smart deployment cleanup
if (deployed_here)
  cleanup_sandbox(__mysql_sandbox_port1);

