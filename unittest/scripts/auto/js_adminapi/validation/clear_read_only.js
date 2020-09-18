
//@ Dba_create_cluster.clear_read_only_invalid
||Dba.createCluster: Option 'clearReadOnly' Bool expected, but value is String (TypeError)

//@ Dba_create_cluster.clear_read_only automatically disabled and clearReadOnly deprecated
|WARNING: The clearReadOnly option is deprecated. The super_read_only mode is now automatically cleared.|
|Cluster successfully created. Use Cluster.addInstance() to add MySQL instances.|

//@ Check super read only was disabled after createCluster
||

//@ Dba_configure_local_instance.clear_read_only_invalid
||Dba.configureLocalInstance: Option 'clearReadOnly' Bool expected, but value is String


//@<OUT> Dba_configure_local_instance.clear_read_only_unset
Configuring local MySQL instance listening at port <<<__mysql_sandbox_port1>>> for use in an InnoDB cluster...
NOTE: Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as <<<hostname>>>:<<<__mysql_sandbox_port1>>>
Assuming full account name 'admin'@'%' for admin

?{VER(>=8.0.23)}
applierWorkerThreads will be set to the default value of 4.

?{}
The instance '<<<hostname>>>:<<<__mysql_sandbox_port1>>>' is valid to be used in an InnoDB cluster.
ERROR: The MySQL instance at '<<<hostname>>>:<<<__mysql_sandbox_port1>>>' currently has[[*]]

//@<ERR> Dba_configure_local_instance.clear_read_only_unset
Dba.configureLocalInstance: Server in SUPER_READ_ONLY mode (RuntimeError)

//@<OUT> Dba_configure_local_instance.clear_read_only_false
Configuring local MySQL instance listening at port <<<__mysql_sandbox_port1>>> for use in an InnoDB cluster...
NOTE: Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as <<<hostname>>>:<<<__mysql_sandbox_port1>>>
Assuming full account name 'admin'@'%' for admin

?{VER(>=8.0.23)}
applierWorkerThreads will be set to the default value of 4.

?{}
The instance '<<<hostname>>>:<<<__mysql_sandbox_port1>>>' is valid to be used in an InnoDB cluster.
ERROR: The MySQL instance at '<<<hostname>>>:<<<__mysql_sandbox_port1>>>' currently has[[*]]

//@<ERR> Dba_configure_local_instance.clear_read_only_false
Dba.configureLocalInstance: Server in SUPER_READ_ONLY mode (RuntimeError)

//@ Dba_drop_metadata.clear_read_only_invalid
||Dba.dropMetadataSchema: Option 'clearReadOnly' Bool expected, but value is String (TypeError)


//@<OUT> Dba_drop_metadata.clear_read_only_unset
ERROR: The MySQL instance at '<<<hostname>>>:<<<__mysql_sandbox_port1>>>' currently has[[*]]

//@<ERR> Dba_drop_metadata.clear_read_only_unset
Dba.dropMetadataSchema: Server in SUPER_READ_ONLY mode (RuntimeError)

//@<OUT> Dba_drop_metadata.clear_read_only_false
ERROR: The MySQL instance at '<<<hostname>>>:<<<__mysql_sandbox_port1>>>' currently has[[*]]

//@<ERR> Dba_drop_metadata.clear_read_only_false
Dba.dropMetadataSchema: Server in SUPER_READ_ONLY mode (RuntimeError)

//@ Dba_reboot_cluster.clear_read_only automatically disabled and clearReadOnly deprecated
|WARNING: The clearReadOnly option is deprecated. The super_read_only mode is now automatically cleared.|

//@ Check super read only was disabled after rebootClusterFromCompleteOutage
||
