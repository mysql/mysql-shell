// deploy sandbox, create a cluster
testutil.deploySandbox(__mysql_sandbox_port1, "root");
shell.connect({scheme:'mysql', user:'root', password: 'root', host:'localhost', port:__mysql_sandbox_port1});
dba.createCluster("tempCluster");

// stop GR manually
session.runSql("stop group_replication;")

//@# drop metadata: standalone instance with metadata, force false
dba.dropMetadataSchema({clearReadOnly: true})

//@# drop metadata: standalone instance with metadata, force true
dba.dropMetadataSchema({force: true, clearReadOnly: true})

// create new cluster
var cluster = dba.createCluster("tempCluster");

// dissolve the cluster
cluster.dissolve({force: true})

//@# drop metadata: dissolved cluster, force false
dba.dropMetadataSchema({force: false, clearReadOnly: true})

//@# drop metadata: dissolved cluster, force true
dba.dropMetadataSchema({force: true, clearReadOnly: true})

// Smart deployment cleanup
session.close();
testutil.destroySandbox(__mysql_sandbox_port1);
