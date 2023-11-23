//@ Initialize
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: hostname});

shell.connect(__sandbox_uri1);

//@<> AR Prepare {VER(>=8.0.4)}
rs = dba.createReplicaSet("rs");

//@# GR functions in an AR instance {VER(>=8.0.4)}
dba.createCluster("x");

dba.getCluster();

dba.configureInstance(__sandbox_uri1);

dba.configureLocalInstance(__sandbox_uri1);

dba.rebootClusterFromCompleteOutage();

//@<> AR Cleanup {VER(>=8.0.4)}
session.runSql("DROP SCHEMA mysql_innodb_cluster_metadata");
session.runSql("RESET " + get_reset_binary_logs_keyword());

//@# Dba_preconditions_standalone, get_cluster_fails
dba.getCluster("");

//@# Dba_preconditions_standalone, create_cluster_succeeds
// Create Cluster is allowed on standalone instance, the precondition
// validation passes
dba.createCluster("1nvalidName");

//@# Dba_preconditions_standalone, drop_metadata_schema_fails
dba.dropMetadataSchema({});

//@# Dba_preconditions_standalone, reboot_cluster_from_complete_outage_succeeds
dba.rebootClusterFromCompleteOutage("");

//@ Unmanaged GR
var cluster = dba.createCluster("dev");
dba.dropMetadataSchema({force:true});

//@# Dba_preconditions_standalone, configureLocalInstance allowed
EXPECT_NO_THROWS(function(){dba.configureLocalInstance(__sandbox_uri1)});

//@# Dba_preconditions_standalone, configureInstance allowed
EXPECT_NO_THROWS(function(){dba.configureInstance(__sandbox_uri1)});

//@# Dba_preconditions_unmanaged_gr, get_cluster_fails
dba.getCluster("");

//@# Dba_preconditions_unmanaged_gr, create_cluster_fails
// Create Cluster is allowed on unmanaged gr instance
// validation passes
dba.createCluster("1nvalidName");

//@# Dba_preconditions_unmanaged_gr, create_cluster_adopt_needed
dba.createCluster("bla");

//@# Dba_preconditions_unmanaged_gr, drop_metadata_schema_fails
dba.dropMetadataSchema({});

//@# Dba_preconditions_unmanaged_gr, reboot_cluster_from_complete_outage
dba.rebootClusterFromCompleteOutage("");

//@# Dba_preconditions_unmanaged_gr, create_cluster_adopt
dba.createCluster("bla", {adoptFromGR:true});

//@# Dba_preconditions_innodb, create_cluster_fails
dba.createCluster("duplicate");

//@# Dba_preconditions_innodb, drop_metadata_schema_fails
dba.dropMetadataSchema({});

//@<> Dba_preconditions_innodb, reboot_cluster_from_complete_outage_fails
//Using adoptFromGR option when we create a cluster, means that the hostname
//of the machine will be added to the metadata, instead of localhost, so we
//must use that hostname on the session
session.close();
var uri = "mysql://root:root@"+ hostname + ":" + __mysql_sandbox_port1;
shell.connect(uri);

EXPECT_THROWS(function() {
    dba.rebootClusterFromCompleteOutage("bla");
}, "The Cluster is ONLINE");
EXPECT_OUTPUT_CONTAINS(`Cluster instances: '${hostname}:${__mysql_sandbox_port1}' (ONLINE)`);

session.close();

//@<> remove the cluster's metadata
shell.connect(__sandbox_uri1);

var cluster_id = session.runSql("SELECT cluster_id FROM mysql_innodb_cluster_metadata.clusters WHERE cluster_name = 'bla'").fetchOne()[0];

session.runSql("DELETE FROM mysql_innodb_cluster_metadata.instances WHERE cluster_id = '" + cluster_id +"'");
session.runSql("DELETE from mysql_innodb_cluster_metadata.clusters WHERE cluster_id = '" + cluster_id + "'");
session.runSql("STOP group_replication");

//@ Dba_preconditions_standalone_with_metadata, get_cluster_fails
dba.getCluster("bla");

//@ Dba_preconditions_standalone_with_metadata, create_cluster_succeeds
// Create Cluster is allowed on standalone instance with metadata, the precondition validation passes
dba.createCluster("1nvalidName");

//@ Dba_preconditions_standalone_with_metadata, reboot_cluster_from_complete_outage_fails
dba.rebootClusterFromCompleteOutage("bla");

//@ Dba_preconditions_standalone_with_metadata, drop_metadata_schema_succeeds
dba.dropMetadataSchema({ 'force': true, 'clearReadOnly': true });

//@ create new cluster
dba.createCluster("dev");

//@ stop group replication
session.runSql("stop group_replication;");

//@ Dba_preconditions_standalone_in_metadata, get_cluster_fails
dba.getCluster("dev");

//@ Dba_preconditions_standalone_in_metadata, create_cluster_fails
dba.createCluster("dev2");

//@ Dba_preconditions_standalone_in_metadata, reboot_cluster_from_complete_outage_succeeds
dba.rebootClusterFromCompleteOutage("dev", { 'clearReadOnly': true });

//@ stop group replication once again
session.runSql("stop group_replication;");

//@ Dba_preconditions_standalone_in_metadata, drop_metadata_schema_succeeds
dba.dropMetadataSchema({ 'force': true, 'clearReadOnly': true });

//@ Cleanup
session.close();
testutil.destroySandbox(__mysql_sandbox_port1);

