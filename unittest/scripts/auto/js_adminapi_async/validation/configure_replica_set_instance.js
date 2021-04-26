//@<OUT> configure and restart:0 {VER(>8.0.0)}
NOTE: MySQL server needs to be restarted for configuration changes to take effect.

//@# configure and restart:1 {VER(>8.0.0)}
|Configuring local MySQL instance listening at port <<<__mysql_sandbox_port2>>> for use in an InnoDB ReplicaSet...|
|Some variables need to be changed, but cannot be done dynamically on the server.|
|NOTE: MySQL server at <<<__address2>>> was restarted.|
|Configuring local MySQL instance listening at port <<<__mysql_sandbox_port2>>> for use in an InnoDB ReplicaSet...|
|The instance '<<<__address2>>>' is valid to be used in an InnoDB ReplicaSet.|

//@# Verify that configureInstance() detects and fixes the wrong settings {VER(>=8.0.23) && VER(<8.0.26)}
@The instance '<<<__address2>>>' belongs to an InnoDB ReplicaSet.@
@Configuring local MySQL instance listening at port <<<__mysql_sandbox_port2>>> for use in an InnoDB ReplicaSet...@
@applierWorkerThreads will be set to the default value of 4.@
@NOTE: Some configuration options need to be fixed:@
@+-----------------------------+---------------+----------------+----------------------------+@
@| Variable                    | Current Value | Required Value | Note                       |@
@+-----------------------------+---------------+----------------+----------------------------+@
@| slave_preserve_commit_order | OFF           | ON             | Update the server variable |@
@+-----------------------------+---------------+----------------+----------------------------+@
@The instance '<<<__address2>>>' was configured to be used in an InnoDB ReplicaSet.@

//@# Verify that configureInstance() detects and fixes the wrong settings {VER(>=8.0.26)}
@The instance '<<<__address2>>>' belongs to an InnoDB ReplicaSet.@
@Configuring local MySQL instance listening at port <<<__mysql_sandbox_port2>>> for use in an InnoDB ReplicaSet...@
@applierWorkerThreads will be set to the default value of 4.@
@NOTE: Some configuration options need to be fixed:@
@+-------------------------------+---------------+----------------+----------------------------+@
@| Variable                      | Current Value | Required Value | Note                       |@
@+-------------------------------+---------------+----------------+----------------------------+@
@| replica_preserve_commit_order | OFF           | ON             | Update the server variable |@
@+-------------------------------+---------------+----------------+----------------------------+@
@The instance '<<<__address2>>>' was configured to be used in an InnoDB ReplicaSet.@

//@# configure and check admin user {VER(>8.0.0)}
||User 'root' can only connect from 'localhost'. (RuntimeError)

//@<OUT> configure and check admin user interactive {VER(>8.0.0)}
Configuring local MySQL instance listening at port <<<__mysql_sandbox_port1>>> for use in an InnoDB ReplicaSet...

This instance reports its own address as <<<__address1>>>

ERROR: User 'root' can only connect from 'localhost'. New account(s) with proper source address specification to allow remote connection from all instances must be created to manage the cluster.

1) Create remotely usable account for 'root' with same grants and password
2) Create a new admin account for InnoDB ReplicaSet with minimal required grants
3) Ignore and continue
4) Cancel

Please select an option [1]: Please provide a source address filter for the account (e.g: 192.168.% or % etc) or leave empty and press Enter to cancel.
Account Host:
?{VER(>=8.0.23)}
applierWorkerThreads will be set to the default value of 4.

?{}
NOTE: Some configuration options need to be fixed:
