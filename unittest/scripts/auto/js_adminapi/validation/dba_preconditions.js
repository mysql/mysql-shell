//@ Initialize
||

//@# GR functions in an AR instance {VER(>=8.0.4)}
||This function is not available through a session to an instance that is a member of an InnoDB ReplicaSet (MYSQLSH 51306)
|No InnoDB Cluster found, did you meant to call dba.getReplicaSet()?|
||This function is not available through a session to an instance that is a member of an InnoDB ReplicaSet (MYSQLSH 51306)
||This function is not available through a session to an instance that is a member of an InnoDB ReplicaSet (MYSQLSH 51306)
||This function is not available through a session to an instance that is a member of an InnoDB ReplicaSet (MYSQLSH 51306)
||This function is not available through a session to an instance that is a member of an InnoDB ReplicaSet (MYSQLSH 51306)

//@# Dba_preconditions_standalone, get_cluster_fails
||This function is not available through a session to a standalone instance

//@# Dba_preconditions_standalone, create_cluster_succeeds
// Create Cluster is allowed on standalone instance, the precondition
// validation passes
||Cluster name may only contain alphanumeric characters, '_', '-', or '.' and may not start with a number (1nvalidName)


//@# Dba_preconditions_standalone, drop_metadata_schema_fails
// getCluster is not allowed on standalone instances
||This function is not available through a session to a standalone instance


//@# Dba_preconditions_standalone, reboot_cluster_from_complete_outage_succeeds
||This function is not available through a session to a standalone instance

//@ Unmanaged GR
||

//@# Dba_preconditions_standalone, configureLocalInstance allowed
|The instance '<<<hostname>>>:<<<__mysql_sandbox_port1>>>' is valid to be used in an InnoDB cluster.|
|The instance '<<<hostname>>>:<<<__mysql_sandbox_port1>>>' is already ready to be used in an InnoDB cluster.|

//@# Dba_preconditions_standalone, configureInstance allowed
|The instance '<<<hostname>>>:<<<__mysql_sandbox_port1>>>' is valid to be used in an InnoDB cluster.|
|The instance '<<<hostname>>>:<<<__mysql_sandbox_port1>>>' is already ready to be used in an InnoDB cluster.|

//@# Dba_preconditions_unmanaged_gr, get_cluster_fails
// getCluster is not allowed on standalone instances
||This function is not available through a session to an instance belonging to an unmanaged replication group

//@# Dba_preconditions_unmanaged_gr, create_cluster_fails
// Create Cluster is allowed on standalone instance, the precondition
// validation passes
||Cluster name may only contain alphanumeric characters, '_', '-', or '.' and may not start with a number (1nvalidName)

//@# Dba_preconditions_unmanaged_gr, create_cluster_adopt_needed
||Creating a cluster on an unmanaged replication group requires adoptFromGR option to be true (MYSQLSH 51315)

//@# Dba_preconditions_unmanaged_gr, drop_metadata_schema_fails
||This function is not available through a session to an instance belonging to an unmanaged replication group

//@# Dba_preconditions_unmanaged_gr, reboot_cluster_from_complete_outage
||This function is not available through a session to an instance belonging to an unmanaged replication group

//@# Dba_preconditions_unmanaged_gr, create_cluster_adopt
||

//@# Dba_preconditions_innodb, create_cluster_fails
||Unable to create cluster. The instance '<<<hostname>>>:<<<__mysql_sandbox_port1>>>' already belongs to an InnoDB cluster. Use dba.getCluster() to access it.

//@# Dba_preconditions_innodb, drop_metadata_schema_fails
||No operation executed, use the 'force' option

//@# Dba_preconditions_innodb, reboot_cluster_from_complete_outage_fails
||The MySQL instance '<<<hostname>>>:<<<__mysql_sandbox_port1>>>' belongs to an InnoDB Cluster and is reachable.

//@ dissolve the cluster
||

//@ Dba_preconditions_standalone_with_metadata, get_cluster_fails
||This function is not available through a session to a standalone instance (metadata exists, instance does not belong to that metadata, and GR is not active)

//@ Dba_preconditions_standalone_with_metadata, create_cluster_succeeds
||Cluster name may only contain alphanumeric characters, '_', '-', or '.' and may not start with a number (1nvalidName)

//@ Dba_preconditions_standalone_with_metadata, reboot_cluster_from_complete_outage_fails
||This function is not available through a session to a standalone instance (metadata exists, instance does not belong to that metadata, and GR is not active)

//@ Dba_preconditions_standalone_with_metadata, drop_metadata_schema_succeeds
||

//@ create new cluster
||

//@ stop group replication
||

//@ Dba_preconditions_standalone_in_metadata, get_cluster_fails
||This function is not available through a session to a standalone instance (metadata exists, instance belongs to that metadata, but GR is not active)

//@ Dba_preconditions_standalone_in_metadata, create_cluster_fails
||dba.createCluster: Unable to create cluster. The instance '<<<hostname>>>:<<<__mysql_sandbox_port1>>>' has a populated Metadata schema and belongs to that Metadata. Use either dba.dropMetadataSchema() to drop the schema, or dba.rebootClusterFromCompleteOutage() to reboot the cluster from complete outage.

//@ Dba_preconditions_standalone_in_metadata, reboot_cluster_from_complete_outage_succeeds
||

//@ stop group replication once again
||

//@ Dba_preconditions_standalone_in_metadata, drop_metadata_schema_succeeds
||

//@ Cleanup
||
