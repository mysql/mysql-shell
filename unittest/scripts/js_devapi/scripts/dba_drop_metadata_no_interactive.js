// Assumptions: smart deployment functions available

// Smart deployment
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: hostname});

shell.connect({scheme:'mysql', user:'root', password: 'root', host:'localhost', port:__mysql_sandbox_port1});

//@# Invalid dropMetadataSchema call
dba.dropMetadataSchema(1,2,3,4,5);

//@# create cluster
if (__have_ssl)
  dba.createCluster("tempCluster", {memberSslMode: "REQUIRED", clearReadOnly: true});
else
  dba.createCluster("tempCluster", {memberSslMode: "DISABLED", clearReadOnly: true});

//@# drop metadata: force false
dba.dropMetadataSchema({force:false});

//@# drop metadata: force true
dba.dropMetadataSchema({force:true});

session.runSql('drop schema if exists mysql_innodb_cluster_metadata');

session.close();

// Smart deployment cleanup
testutil.destroySandbox(__mysql_sandbox_port1);
