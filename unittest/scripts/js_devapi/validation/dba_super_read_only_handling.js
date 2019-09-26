//@ Initialization
||

//@<OUT> Configures the instance, answers 'yes' on the read only prompt
Configuring local MySQL instance listening at port <<<__mysql_sandbox_port1>>> for use in an InnoDB cluster...
NOTE: Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as <<<hostname>>>:<<<__mysql_sandbox_port1>>>
Assuming full account name 'testUser'@'%' for testUser

The instance '<<<hostname>>>:<<<__mysql_sandbox_port1>>>' is valid to be used in an InnoDB cluster.

The MySQL instance at 'localhost:<<<__mysql_sandbox_port1>>>' currently has the super_read_only system
variable set to protect it from inadvertent updates from applications.
You must first unset it to be able to perform any changes to this instance.
For more information see:
https://dev.mysql.com/doc/refman/en/server-system-variables.html#sysvar_super_read_only.

Do you want to disable super_read_only and continue? [y/N]: Disabled super_read_only on the instance '<<<hostname>>>:<<<__mysql_sandbox_port1>>>'
Cluster admin user 'testUser'@'%' created.
Enabling super_read_only on the instance '<<<hostname>>>:<<<__mysql_sandbox_port1>>>'

//@<OUT> Configures the instance, read only set, no prompt
Configuring local MySQL instance listening at port <<<__mysql_sandbox_port2>>> for use in an InnoDB cluster...
NOTE: Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as <<<hostname>>>:<<<__mysql_sandbox_port2>>>
Assuming full account name 'testUser'@'%' for testUser

The instance '<<<hostname>>>:<<<__mysql_sandbox_port2>>>' is valid to be used in an InnoDB cluster.
Disabled super_read_only on the instance '<<<hostname>>>:<<<__mysql_sandbox_port2>>>'
Cluster admin user 'testUser'@'%' created.
Enabling super_read_only on the instance '<<<hostname>>>:<<<__mysql_sandbox_port2>>>'

//@<OUT> Configures the instance, no prompt
Configuring local MySQL instance listening at port <<<__mysql_sandbox_port3>>> for use in an InnoDB cluster...
NOTE: Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as <<<hostname>>>:<<<__mysql_sandbox_port3>>>
Assuming full account name 'testUser'@'%' for testUser

The instance '<<<hostname>>>:<<<__mysql_sandbox_port3>>>' is valid to be used in an InnoDB cluster.

Cluster admin user 'testUser'@'%' created.

//@ Creates Cluster succeeds (should auto-clear)
|Disabling super_read_only mode on instance '<<<hostname>>>:<<<__mysql_sandbox_port1>>>'.|

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

//@# Reboot the cluster
|Reconfiguring the cluster 'sample' from complete outage...|

|The instance '<<<hostname>>>:<<<__mysql_sandbox_port2>>>' was part of the cluster configuration.|
|Would you like to rejoin it to the cluster? [y/N]: |
|The instance '<<<hostname>>>:<<<__mysql_sandbox_port3>>>' was part of the cluster configuration.|
|Would you like to rejoin it to the cluster? [y/N]: |
|Disabling super_read_only mode on instance '<<<hostname>>>:<<<__mysql_sandbox_port1>>>'.|
|The cluster was successfully rebooted.|

//@ Cleanup
||
