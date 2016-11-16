// Assumptions: ensure_schema_does_not_exist available
//@# Invalid dropMetadataSchema call
dba.dropMetadataSchema(1,2,3,4,5);

//@# drop metadata: no arguments
dba.dropMetadataSchema()

//@# drop metadata: enforce false
dba.dropMetadataSchema({enforce:false});

//@# create cluster
if (__have_ssl)
  dba.createCluster("tempCluster")
else
  dba.createCluster("tempCluster", {ssl: false})

session.getSchema('mysql_innodb_cluster_metadata');

//@# drop metadata: enforce true
dba.dropMetadataSchema({enforce:true});
ensure_schema_does_not_exist(session, 'mysql_innodb_cluster_metadata')
