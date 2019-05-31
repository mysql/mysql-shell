//@ Initialization
||

//@<OUT> Configures the instance, answers 'yes' on the read only prompt
Configuring local MySQL instance listening at port <<<__mysql_sandbox_port1>>> for use in an InnoDB cluster...
NOTE: Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as <<<hostname>>>:<<<__mysql_sandbox_port1>>>
Assuming full account name 'testUser'@'%' for testUser

The instance 'localhost:<<<__mysql_sandbox_port1>>>' is valid for InnoDB cluster usage.

The MySQL instance at 'localhost:<<<__mysql_sandbox_port1>>>' currently has the super_read_only system
variable set to protect it from inadvertent updates from applications. You must
first unset it to be able to perform any changes to this instance.
For more information see:
https://dev.mysql.com/doc/refman/en/server-system-variables.html#sysvar_super_read_only.

Do you want to disable super_read_only and continue? [y/N]: Disabled super_read_only on the instance 'localhost:<<<__mysql_sandbox_port1>>>'
Cluster admin user 'testUser'@'%' created.
Enabling super_read_only on the instance 'localhost:<<<__mysql_sandbox_port1>>>'

//@<OUT> Configures the instance, read only set, no prompt
Configuring local MySQL instance listening at port <<<__mysql_sandbox_port2>>> for use in an InnoDB cluster...
NOTE: Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as <<<hostname>>>:<<<__mysql_sandbox_port2>>>
Assuming full account name 'testUser'@'%' for testUser

The instance 'localhost:<<<__mysql_sandbox_port2>>>' is valid for InnoDB cluster usage.
Disabled super_read_only on the instance 'localhost:<<<__mysql_sandbox_port2>>>'
Cluster admin user 'testUser'@'%' created.
Enabling super_read_only on the instance 'localhost:<<<__mysql_sandbox_port2>>>'

//@<OUT> Configures the instance, no prompt
Configuring local MySQL instance listening at port <<<__mysql_sandbox_port3>>> for use in an InnoDB cluster...
NOTE: Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as <<<hostname>>>:<<<__mysql_sandbox_port3>>>
Assuming full account name 'testUser'@'%' for testUser

The instance 'localhost:<<<__mysql_sandbox_port3>>>' is valid for InnoDB cluster usage.

Cluster admin user 'testUser'@'%' created.

//@ Creates Cluster succeeds (should auto-clear)
|Disabling super_read_only mode on instance 'localhost:<<<__mysql_sandbox_port1>>>'.|

//@ Adds a read only instance
|The instance 'localhost:<<<__mysql_sandbox_port2>>>' was successfully added to the cluster.|

//@ Adds other instance
|The instance 'localhost:<<<__mysql_sandbox_port3>>>' was successfully added to the cluster.|

//@ Rejoins an instance
|The instance 'localhost:<<<__mysql_sandbox_port3>>>' was successfully rejoined on the cluster.|

//@ persist GR configuration settings for 5.7 servers {VER(<8.0.11)}
||

//@ Reset gr_start_on_boot on all instances
||

//@<OUT> Reboot the cluster
Reconfiguring the cluster 'sample' from complete outage...

The instance '<<<hostname>>>:<<<__mysql_sandbox_port2>>>' was part of the cluster configuration.
Would you like to rejoin it to the cluster? [y/N]: 
The instance '<<<hostname>>>:<<<__mysql_sandbox_port3>>>' was part of the cluster configuration.
Would you like to rejoin it to the cluster? [y/N]: 
Disabling super_read_only mode on instance 'localhost:<<<__mysql_sandbox_port1>>>'.
The safest and most convenient way to provision a new instance is through
automatic clone provisioning, which will completely overwrite the state of
'localhost:<<<__mysql_sandbox_port1>>>' with a physical snapshot from an existing cluster member. To
use this method by default, set the 'recoveryMethod' option to 'clone'.

The incremental distributed state recovery may be safely used if you are sure
all updates ever executed in the cluster were done with GTIDs enabled, there
are no purged transactions and the new instance contains the same GTID set as
the cluster or a subset of it. To use this method by default, set the
'recoveryMethod' option to 'incremental'.

Incremental distributed state recovery was selected because it seems to be safely usable.
?{VER(<8.0.11)}
WARNING: Instance 'localhost:<<<__mysql_sandbox_port1>>>' cannot persist Group Replication configuration since MySQL version <<<__version>>> does not support the SET PERSIST command (MySQL version >= 8.0.11 required). Please use the <Dba>.configureLocalInstance() command locally to persist the changes.
WARNING: Instance '<<<hostname>>>:<<<__mysql_sandbox_port2>>>' cannot persist Group Replication configuration since MySQL version <<<__version>>> does not support the SET PERSIST command (MySQL version >= 8.0.11 required). Please use the <Dba>.configureLocalInstance() command locally to persist the changes.
WARNING: Instance '<<<hostname>>>:<<<__mysql_sandbox_port3>>>' cannot persist Group Replication configuration since MySQL version <<<__version>>> does not support the SET PERSIST command (MySQL version >= 8.0.11 required). Please use the <Dba>.configureLocalInstance() command locally to persist the changes.
?{}

The cluster was successfully rebooted.

//@ Cleanup
||
