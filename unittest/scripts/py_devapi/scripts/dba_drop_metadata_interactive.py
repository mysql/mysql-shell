if __have_ssl:
  dba.create_cluster("tempCluster")
else:
  dba.create_cluster("tempCluster", {'ssl': False})

@# Invalid drop_metadata_schema call
dba.drop_metadata_schema(1,2,3,4,5);
dba.drop_metadata_schema({not_valid:true});

@# drop metadata: no user response
dba.drop_metadata_schema()

@# drop metadata: user response no
dba.drop_metadata_schema()

@# drop metadata: enforce option
dba.drop_metadata_schema({enforce:false});
session.get_schema('mysql_innodb_cluster_metadata');
dba.drop_metadata_schema({enforce:true});

@# drop metadata: user response yes
if __have_ssl:
  dba.create_cluster("tempCluster")
else:
  dba.create_cluster("tempCluster", {'ssl': False})

dba.drop_metadata_schema()
