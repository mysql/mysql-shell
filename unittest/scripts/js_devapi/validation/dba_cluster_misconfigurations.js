//@ Initialization
||

//@ create cluster admin
|Cluster admin user 'ca'@'%' created.|

//@ Dba.createCluster (fail because of bad configuration)
||Instance check failed

//@ Dba.createCluster (fail because of bad configuration of parallel-appliers) {VER(>=8.0.23)}
||Instance check failed

//@ Dba.createCluster (succeeds with right configuration of parallel-appliers) {VER(>=8.0.23)}
||

//@ Cluster.addInstance (fail because of bad configuration of parallel-appliers) {VER(>=8.0.23)}
||Instance check failed

//@ Cluster.addInstance (succeeds with right configuration of parallel-appliers) {VER(>=8.0.23)}
||

//@ Setup next test
||

//@ Create cluster fails (one table is not compatible)
|WARNING: The following tables do not have a Primary Key or equivalent column:|
||Instance check failed (RuntimeError)

//@ Enable verbose
||

//@Create cluster fails (one table is not compatible) - verbose mode
|WARNING: The following tables do not have a Primary Key or equivalent column:|
||Instance check failed
//@ Disable verbose
||

//@ Create cluster succeeds (no incompatible table)
||

//@ Dissolve cluster at the end (clean-up)
||

//@ Finalization
||
