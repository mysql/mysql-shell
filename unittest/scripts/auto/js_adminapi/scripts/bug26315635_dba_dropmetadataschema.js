// Bug #26315635 UNABLE TO EXECUTE DBA.DROPMETADATASCHEMA() ON STAND-ALONE INSTANCE WITH METADATA

// It's not possible to drop the Metadata schema using dba.dropMetadataSchema(),
// on an instance which has Metadata schema.

// deploy sandbox, create a cluster
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: hostname});
shell.connect({scheme:'mysql', user:'root', password: 'root', host:'localhost', port:__mysql_sandbox_port1});
dba.createCluster("tempCluster", {gtidSetIsComplete: true});

// stop GR manually
session.runSql("stop group_replication;")

//@# drop metadata: standalone instance with metadata, force false
dba.dropMetadataSchema({clearReadOnly: true})

//@# drop metadata: standalone instance with metadata, force true
dba.dropMetadataSchema({force: true, clearReadOnly: true})

// create new cluster
var cluster = dba.createCluster("tempCluster", {gtidSetIsComplete: true});

// dissolve the cluster
cluster.dissolve({force: true})

//@# drop metadata: dissolved cluster, force false
dba.dropMetadataSchema({force: false, clearReadOnly: true})

//@# drop metadata: dissolved cluster, force true
dba.dropMetadataSchema({force: true, clearReadOnly: true})

// Smart deployment cleanup
session.close();
testutil.destroySandbox(__mysql_sandbox_port1);
