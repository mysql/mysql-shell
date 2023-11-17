//@ Initialization
||

//@<OUT> Configures the instance, answers 'yes' on the read only prompt
Configuring local MySQL instance listening at port <<<__mysql_sandbox_port1>>> for use in an InnoDB Cluster...
NOTE: Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as <<<hostname>>>:<<<__mysql_sandbox_port1>>>
Assuming full account name 'testUser'@'%' for testUser
?{VER(>=8.0.23)}

applierWorkerThreads will be set to the default value of 4.
?{}
Disabled super_read_only on the instance '<<<hostname>>>:<<<__mysql_sandbox_port1>>>'

Creating user testUser@%.
Account testUser@% was successfully created.

Enabling super_read_only on the instance '<<<hostname>>>:<<<__mysql_sandbox_port1>>>'

The instance '<<<hostname>>>:<<<__mysql_sandbox_port1>>>' is valid for InnoDB Cluster usage.

//@<OUT> Configures the instance, read only set, no prompt
Configuring local MySQL instance listening at port <<<__mysql_sandbox_port2>>> for use in an InnoDB Cluster...
NOTE: Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as <<<hostname>>>:<<<__mysql_sandbox_port2>>>
Assuming full account name 'testUser'@'%' for testUser
?{VER(>=8.0.23)}

applierWorkerThreads will be set to the default value of 4.
?{}
Disabled super_read_only on the instance '<<<hostname>>>:<<<__mysql_sandbox_port2>>>'

Creating user testUser@%.
Account testUser@% was successfully created.

Enabling super_read_only on the instance '<<<hostname>>>:<<<__mysql_sandbox_port2>>>'

The instance '<<<hostname>>>:<<<__mysql_sandbox_port2>>>' is valid for InnoDB Cluster usage.

//@<OUT> Configures the instance, no prompt
Configuring local MySQL instance listening at port <<<__mysql_sandbox_port3>>> for use in an InnoDB Cluster...
NOTE: Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as <<<hostname>>>:<<<__mysql_sandbox_port3>>>
Assuming full account name 'testUser'@'%' for testUser
?{VER(>=8.0.23)}

applierWorkerThreads will be set to the default value of 4.
?{}

Creating user testUser@%.
Account testUser@% was successfully created.


The instance '<<<hostname>>>:<<<__mysql_sandbox_port3>>>' is valid for InnoDB Cluster usage.

//@ Creates Cluster succeeds (should auto-clear)
|Disabling super_read_only mode on instance '<<<hostname>>>:<<<__mysql_sandbox_port1>>>'.|

//@ Adds a read only instance
|The instance '<<<hostname>>>:<<<__mysql_sandbox_port2>>>' was successfully added to the cluster.|

//@ Adds other instance
|The instance '<<<hostname>>>:<<<__mysql_sandbox_port3>>>' was successfully added to the cluster.|

//@ Rejoins an instance
|The instance '<<<hostname>>>:<<<__mysql_sandbox_port3>>>' was successfully rejoined to the cluster.|

//@ persist GR configuration settings for 5.7 servers {VER(<8.0.11)}
||

//@ Reset gr_start_on_boot on all instances
||

//@# Reboot the cluster
|Restoring the Cluster 'sample' from complete outage...|

|The Cluster was successfully rebooted.|

//@ Cleanup
||
