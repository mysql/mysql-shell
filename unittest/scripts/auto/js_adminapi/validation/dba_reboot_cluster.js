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

//@ Get data about existing replication users before reboot.
||

