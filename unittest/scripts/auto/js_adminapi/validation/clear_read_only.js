
//@ Dba_create_cluster.clear_read_only_invalid
||Dba.createCluster: Option 'clearReadOnly' Bool expected, but value is String (TypeError)

//@<OUT> Dba_create_cluster.clear_read_only_unset
Validating instance at localhost:<<<__mysql_sandbox_port1>>>...
NOTE: Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as <<<hostname>>>

Instance configuration is suitable.
ERROR: The MySQL instance at 'localhost:<<<__mysql_sandbox_port1>>>' currently has the super_read_only system
variable set to protect it from inadvertent updates from applications. You must
first unset it to be able to perform any changes to this instance.
For more information see:
https://dev.mysql.com/doc/refman/en/server-system-variables.html#sysvar_super_read_only.

//@<ERR> Dba_create_cluster.clear_read_only_unset
Dba.createCluster: Server in SUPER_READ_ONLY mode (RuntimeError)

//@<OUT> Dba_create_cluster.clear_read_only_false
Validating instance at localhost:<<<__mysql_sandbox_port1>>>...
NOTE: Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as <<<hostname>>>

Instance configuration is suitable.
ERROR: The MySQL instance at 'localhost:<<<__mysql_sandbox_port1>>>' currently has the super_read_only system
variable set to protect it from inadvertent updates from applications. You must
first unset it to be able to perform any changes to this instance.

//@<ERR> Dba_create_cluster.clear_read_only_false
Dba.createCluster: Server in SUPER_READ_ONLY mode (RuntimeError)

//@ Check unchanged
||

//@ Dba_configure_local_instance.clear_read_only_invalid
||Dba.configureLocalInstance: Option 'clearReadOnly' Bool expected, but value is String


//@<OUT> Dba_configure_local_instance.clear_read_only_unset
Configuring local MySQL instance listening at port <<<__mysql_sandbox_port1>>> for use in an InnoDB cluster...
NOTE: Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as <<<hostname>>>
Assuming full account name 'admin'@'%' for admin

The instance 'localhost:<<<__mysql_sandbox_port1>>>' is valid for InnoDB cluster usage.
ERROR: The MySQL instance at 'localhost:<<<__mysql_sandbox_port1>>>' currently has the super_read_only system
variable set to protect it from inadvertent updates from applications. You must
first unset it to be able to perform any changes to this instance.

//@<ERR> Dba_configure_local_instance.clear_read_only_unset
Dba.configureLocalInstance: Server in SUPER_READ_ONLY mode (RuntimeError)

//@<OUT> Dba_configure_local_instance.clear_read_only_false
Configuring local MySQL instance listening at port <<<__mysql_sandbox_port1>>> for use in an InnoDB cluster...
NOTE: Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as <<<hostname>>>
Assuming full account name 'admin'@'%' for admin

The instance 'localhost:<<<__mysql_sandbox_port1>>>' is valid for InnoDB cluster usage.
ERROR: The MySQL instance at 'localhost:<<<__mysql_sandbox_port1>>>' currently has the super_read_only system
variable set to protect it from inadvertent updates from applications. You must
first unset it to be able to perform any changes to this instance.

//@<ERR> Dba_configure_local_instance.clear_read_only_false
Dba.configureLocalInstance: Server in SUPER_READ_ONLY mode (RuntimeError)

//@ Dba_drop_metadata.clear_read_only_invalid
||Dba.dropMetadataSchema: Argument 'clearReadOnly' is expected to be a bool


//@<OUT> Dba_drop_metadata.clear_read_only_unset
ERROR: The MySQL instance at 'localhost:<<<__mysql_sandbox_port1>>>' currently has the super_read_only system
variable set to protect it from inadvertent updates from applications. You must
first unset it to be able to perform any changes to this instance.

//@<ERR> Dba_drop_metadata.clear_read_only_unset
Dba.dropMetadataSchema: Server in SUPER_READ_ONLY mode (RuntimeError)

//@<OUT> Dba_drop_metadata.clear_read_only_false
ERROR: The MySQL instance at 'localhost:<<<__mysql_sandbox_port1>>>' currently has the super_read_only system
variable set to protect it from inadvertent updates from applications. You must
first unset it to be able to perform any changes to this instance.

//@<ERR> Dba_drop_metadata.clear_read_only_false
Dba.dropMetadataSchema: Server in SUPER_READ_ONLY mode (RuntimeError)

//@ Dba_reboot_cluster.clear_read_only_invalid
||Dba.rebootClusterFromCompleteOutage: Argument 'clearReadOnly' is expected to be a bool

//@<OUT> Dba_reboot_cluster.clear_read_only_unset
ERROR: The MySQL instance at 'localhost:<<<__mysql_sandbox_port1>>>' currently has the super_read_only system
variable set to protect it from inadvertent updates from applications. You must
first unset it to be able to perform any changes to this instance.

//@<ERR> Dba_reboot_cluster.clear_read_only_unset
Dba.rebootClusterFromCompleteOutage: Server in SUPER_READ_ONLY mode (RuntimeError)
