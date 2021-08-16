//@ Initialization
||

//@ Add instance 2
||

//@ Add instance 3
||

//@<OUT> persist GR configuration settings for 5.7 servers {VER(<8.0.11)}
The instance '<<<hostname>>>:<<<__mysql_sandbox_port1>>>' belongs to an InnoDB cluster.
Persisting the cluster settings...
The instance '<<<hostname>>>:<<<__mysql_sandbox_port1>>>' was configured for use in an InnoDB cluster.

The instance cluster settings were successfully persisted.
The instance '<<<hostname>>>:<<<__mysql_sandbox_port2>>>' belongs to an InnoDB cluster.
Persisting the cluster settings...
The instance '<<<hostname>>>:<<<__mysql_sandbox_port2>>>' was configured for use in an InnoDB cluster.

The instance cluster settings were successfully persisted.
The instance '<<<hostname>>>:<<<__mysql_sandbox_port3>>>' belongs to an InnoDB cluster.
Persisting the cluster settings...
The instance '<<<hostname>>>:<<<__mysql_sandbox_port3>>>' was configured for use in an InnoDB cluster.

The instance cluster settings were successfully persisted.

//@ Dba.rebootClusterFromCompleteOutage errors
||The cluster with the name '' does not exist.
||Invalid options: invalidOpt
||The cluster with the name 'dev2' does not exist.
||The MySQL instance '<<<hostname>>>:<<<__mysql_sandbox_port2>>>' belongs to an InnoDB Cluster and is reachable.

//@ Get data about existing replication users before reboot.
||

//@ Reboot cluster fails because instance is online and there is no quorum.
||The MySQL instance '<<<hostname>>>:<<<__mysql_sandbox_port1>>>' belongs to an InnoDB Cluster and is reachable. Please use <Cluster>.forceQuorumUsingPartitionOf() to restore from the quorum loss.

//@ Dba.rebootClusterFromCompleteOutage error unreachable server cannot be on the rejoinInstances list
||The following instances: '<<<hostname>>>:<<<__mysql_sandbox_port3>>>' were specified in the rejoinInstances list but are not reachable.

//@ Dba.rebootClusterFromCompleteOutage error cannot use same server on both rejoinInstances and removeInstances list
||The following instances: 'localhost:<<<__mysql_sandbox_port2>>>' belong to both 'rejoinInstances' and 'removeInstances' lists.

//@ Dba.rebootClusterFromCompleteOutage success
|WARNING: The user option is deprecated and will be removed in a future release. If not specified, the user name is taken from the active session.|
|WARNING: The password option is deprecated and will be removed in a future release. If not specified, the password is taken from the active session.|
||

//@<OUT> Confirm no new replication user was created on bootstrap member.
false

//@<OUT> Confirm no new replication user was created on other rejoining member.
false

//@ Finalization
||
