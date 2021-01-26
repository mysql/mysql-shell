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

//@ Dba.rebootClusterFromCompleteOutage errors
||Dba.rebootClusterFromCompleteOutage: The MySQL instance '<<<hostname>>>:<<<__mysql_sandbox_port2>>>' belongs to an InnoDB Cluster and is reachable.
||Dba.rebootClusterFromCompleteOutage: Argument #2: Invalid options: invalidOpt

//@ Dba.rebootClusterFromCompleteOutage error unreachable server cannot be on the rejoinInstances list
||Dba.rebootClusterFromCompleteOutage: The following instances: '<<<hostname>>>:<<<__mysql_sandbox_port3>>>' were specified in the rejoinInstances list but are not reachable.

//@ Dba.rebootClusterFromCompleteOutage error cannot use same server on both rejoinInstances and removeInstances list
||Dba.rebootClusterFromCompleteOutage: The following instances: 'localhost:<<<__mysql_sandbox_port2>>>' belong to both 'rejoinInstances' and 'removeInstances' lists.

//@ Dba.rebootClusterFromCompleteOutage success
|The cluster was successfully rebooted.|

//@ Rescan cluster to add instance 3 back to metadata {VER(>=8.0.11)}
||

//@ Add instance 3 back to the cluster {VER(<=8.0.11)}
||

//@ Dba.rebootClusterFromCompleteOutage regression test for BUG#25516390
|The cluster was successfully rebooted.|

//@ Finalization
||
