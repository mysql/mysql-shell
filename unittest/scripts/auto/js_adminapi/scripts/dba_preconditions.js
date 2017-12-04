//@ Initialize
testutil.deploySandbox(__mysql_sandbox_port1, "root");

shell.connect(__sandbox_uri1);

//@# Dba_preconditions_standalone, get_cluster_fails
dba.getCluster({});

//@# Dba_preconditions_standalone, get_cluster_fails 2
dba.getCluster("");

//@# Dba_preconditions_standalone, create_cluster_succeeds
// Create Cluster is allowed on standalone instance, the precondition
// validation passes
dba.createCluster("1nvalidName");

//@# Dba_preconditions_standalone, drop_metadata_schema_fails
// getCluster is not allowed on standalone instances
dba.dropMetadataSchema({});

//@# Dba_preconditions_standalone, reboot_cluster_from_complete_outage_succeeds
dba.rebootClusterFromCompleteOutage("");

//@ Unmanaged GR
var cluster = dba.createCluster("dev");
dba.dropMetadataSchema({force:true});

//@# Dba_preconditions_unmanaged_gr, get_cluster_fails
// getCluster is not allowed on standalone instances
dba.getCluster("");

//@# Dba_preconditions_unmanaged_gr, create_cluster_fails
// Create Cluster is allowed on standalone instance, the precondition
// validation passes
dba.createCluster("1nvalidName");

//@# Dba_preconditions_unmanaged_gr, create_cluster_adopt_needed
dba.createCluster("bla");

//@# Dba_preconditions_unmanaged_gr, drop_metadata_schema_fails
// getCluster is not allowed on standalone instances
dba.dropMetadataSchema({});

//@# Dba_preconditions_unmanaged_gr, reboot_cluster_from_complete_outage
dba.rebootClusterFromCompleteOutage("");

//@# Dba_preconditions_unmanaged_gr, create_cluster_adopt
create_root_from_anywhere(session, true);
dba.createCluster("bla", {adoptFromGR:true});

//@# Dba_preconditions_innodb, create_cluster_fails
dba.createCluster("duplicate");

//@# Dba_preconditions_innodb, drop_metadata_schema_fails
dba.dropMetadataSchema({});

//@# Dba_preconditions_innodb, reboot_cluster_from_complete_outage_fails
//Using adoptFromGR option when we create a cluster, means that the hostname
//of the machine will be added to the metadata, instead of localhost, so we
//must use that hostname on the session
session.close();
var uri = "mysql://root:root@"+ hostname + ":" + __mysql_sandbox_port1;
shell.connect(uri);
dba.rebootClusterFromCompleteOutage("bla");

//@ Cleanup
testutil.destroySandbox(__mysql_sandbox_port1);
