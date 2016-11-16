if (__have_ssl)
  dba.createCluster("tempCluster");
else
  dba.createCluster("tempCluster", {ssl: false});

//@# Invalid dropMetadataSchema call
dba.dropMetadataSchema(1,2,3,4,5);
dba.dropMetadataSchema({not_valid:true});

//@# drop metadata: no user response
dba.dropMetadataSchema()

//@# drop metadata: user response no
dba.dropMetadataSchema()

//@# drop metadata: enforce option
dba.dropMetadataSchema({enforce:false});
session.getSchema('mysql_innodb_cluster_metadata');
dba.dropMetadataSchema({enforce:true});

//@# drop metadata: user response yes
if (__have_ssl)
  dba.createCluster("tempCluster")
else
  dba.createCluster("tempCluster", {ssl: false})

dba.dropMetadataSchema()
