# Assumptions: ensure_schema_does_not_exist available
@# Invalid drop_metadata_schema call
dba.drop_metadata_schema(1,2,3,4,5);

@# drop metadata: no arguments
dba.drop_metadata_schema()

@# drop metadata: enforce false
dba.drop_metadata_schema({enforce:false});

@# create cluster
dba.create_cluster("tempCluster");
session.get_schema('mysql_innodb_cluster_metadata');

@# drop metadata: enforce true
dba.drop_metadata_schema({enforce:true});
ensure_schema_does_not_exist(session, 'mysql_innodb_cluster_metadata')
