//@ Remove the persisted group_replication_start_on_boot and group_replication_group_name {VER(>=8.0.11)}
||

//@ Take third sandbox down, change group_name, start it back
||

//@ Rescan
|Rescanning the cluster...|
|The instance '<<<hostname>>>:<<<__mysql_sandbox_port3>>>' is no longer part of the cluster.|

//@# Try to rejoin it (error)
||Cluster.rejoinInstance: The instance '<<<hostname>>>:<<<__mysql_sandbox_port3>>>' may belong to a different cluster as the one registered in the Metadata since the value of 'group_replication_group_name' does not match the one registered in the cluster's Metadata: possible split-brain scenario. Please remove the instance from the cluster.

//@ getCluster() where the member we're connected to has a mismatched group_name vs metadata
||

//@# check error
||Dba.getCluster: Unable to get an InnoDB cluster handle. The instance '<<<hostname>>>:<<<__mysql_sandbox_port1>>>' may belong to a different cluster from the one registered in the Metadata since the value of 'group_replication_group_name' does not match the one registered in the Metadata: possible split-brain scenario. Please retry while connected to another member of the cluster.

//@ Cleanup
||
