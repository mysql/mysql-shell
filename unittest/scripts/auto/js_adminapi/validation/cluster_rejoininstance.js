//@ BUG#29305551: Initialization
||

//@<OUT> rejoinInstance async replication error
ERROR: Cannot rejoin instance '<<<localhost>>>:<<<__mysql_sandbox_port2>>>' to the cluster because it has asynchronous (master-slave) replication configured and running. Please stop the slave threads by executing the query: 'STOP SLAVE;'

//@<ERR> rejoinInstance async replication error
Cluster.rejoinInstance: The instance '<<<localhost>>>:<<<__mysql_sandbox_port2>>>' is running asynchronous (master-slave) replication. (RuntimeError)

//@ BUG#29305551: Finalization
||