//@ Initialization
||

//@ create first cluster
||

//@ Success adding instance
||

//@<OUT> Check auto_increment values for single-primary

//@ Get the cluster back
||

//@ Restore the quorum
||

//@ Success adding instance to the single cluster
||

//@ Remove the instance from the cluster
||

//@ create second cluster
||

//@ Failure adding instance from multi cluster into single
||The instance 'localhost:<<<__mysql_sandbox_port3>>>' is already part of another InnoDB cluster

//@ Failure adding instance from an unmanaged replication group into single
||The instance 'localhost:<<<__mysql_sandbox_port3>>>' is already part of another Replication Group

//@ Failure adding instance already in the InnoDB cluster
||The instance 'localhost:<<<__mysql_sandbox_port2>>>' is already part of this InnoDB cluster

//@ Finalization
||
