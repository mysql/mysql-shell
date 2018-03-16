//@ Initialize
||

//@# Dba_preconditions_standalone, get_cluster_fails
||Dba.getCluster: This function is not available through a session to a standalone instance

//@# Dba_preconditions_standalone, create_cluster_succeeds
// Create Cluster is allowed on standalone instance, the precondition
// validation passes
||Dba.createCluster: The Cluster name can only start with an alphabetic or the '_' character.


//@# Dba_preconditions_standalone, drop_metadata_schema_fails
// getCluster is not allowed on standalone instances
||Dba.dropMetadataSchema: This function is not available through a session to a standalone instance


//@# Dba_preconditions_standalone, reboot_cluster_from_complete_outage_succeeds
||Dba.rebootClusterFromCompleteOutage: This function is not available through a session to a standalone instance

//@ Unmanaged GR
||

//@# Dba_preconditions_unmanaged_gr, get_cluster_fails
// getCluster is not allowed on standalone instances
||Dba.getCluster: This function is not available through a session to an instance belonging to an unmanaged replication group

//@# Dba_preconditions_unmanaged_gr, create_cluster_fails
// Create Cluster is allowed on standalone instance, the precondition
// validation passes
||Dba.createCluster: The Cluster name can only start with an alphabetic or the '_' character.

//@# Dba_preconditions_unmanaged_gr, create_cluster_adopt_needed
||Creating a cluster on an unmanaged replication group requires adoptFromGR option to be true (ArgumentError)

//@# Dba_preconditions_unmanaged_gr, drop_metadata_schema_fails
||Dba.dropMetadataSchema: This function is not available through a session to an instance belonging to an unmanaged replication group

//@# Dba_preconditions_unmanaged_gr, reboot_cluster_from_complete_outage
||Dba.rebootClusterFromCompleteOutage: This function is not available through a session to an instance belonging to an unmanaged replication group

//@# Dba_preconditions_unmanaged_gr, create_cluster_adopt
||

//@# Dba_preconditions_innodb, create_cluster_fails
||Dba.createCluster: Unable to create cluster. The instance 'localhost:<<<__mysql_sandbox_port1>>>' already belongs to an InnoDB cluster. Use <Dba>.getCluster() to access it.

//@# Dba_preconditions_innodb, drop_metadata_schema_fails
||Dba.dropMetadataSchema: No operation executed, use the 'force' option

//@# Dba_preconditions_innodb, reboot_cluster_from_complete_outage_fails
||Dba.rebootClusterFromCompleteOutage: The MySQL instance '<<<real_hostname>>>:<<<__mysql_sandbox_port1>>>' belongs to an InnoDB Cluster and is reachable. Please use <Cluster>.forceQuorumUsingPartitionOf() to restore from the quorum loss.

//@ Cleanup
||
