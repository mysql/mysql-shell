//@<OUT> WL#12052: Create cluster
Creating a Classic session to 'root@localhost:<<<__mysql_sandbox_port1>>>'

//@<ERR> WL#12052: Error when executing switchToMultiPrimaryMode on a cluster with members < 8.0.13 {VER(<8.0.13)}
Cluster.switchToMultiPrimaryMode: Operation not supported on target server version: '<<<__version>>>' (RuntimeError)

//@ WL#12052: Error when executing switchToMultiPrimaryMode on a cluster with 1 or more members not ONLINE
||This operation requires all the cluster members to be ONLINE (RuntimeError)

//@<ERR> WL#12052: Error when executing switchToMultiPrimaryMode on a cluster with no visible quorum < 8.0.13 {VER(>=8.0.13)}
Cluster.switchToMultiPrimaryMode: There is no quorum to perform the operation (MYSQLSH 51011)

//@ WL#12052: Re-create the cluster {VER(>=8.0.13)}
||

//@<OUT> WL#12052: Switch to multi-primary mode {VER(>=8.0.13)}
Switching cluster 'cluster' to Multi-Primary mode...

Instance '<<<hostname>>>:<<<__mysql_sandbox_port1>>>' remains PRIMARY.
Instance '<<<hostname>>>:<<<__mysql_sandbox_port2>>>' was switched from SECONDARY to PRIMARY.
Instance '<<<hostname>>>:<<<__mysql_sandbox_port3>>>' was switched from SECONDARY to PRIMARY.

The cluster successfully switched to Multi-Primary mode.

//@<OUT> WL#12052: Verify the value of replicasets.topology_type {VER(>=8.0.13)}
mm

//@ WL#12052: Finalization
||
