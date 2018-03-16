//@ Initialization
||

//@ create cluster admin
|Cluster admin user 'ca'@'%' created.|

//@ Dba.createCluster (fail because of bad configuration)
||Instance check failed

//@ Setup next test
||

//@ Create cluster fails (one table is not compatible)
|WARNING: The following tables do not have a Primary Key or equivalent column:|
||Dba.createCluster: Instance check failed (RuntimeError)

//@ Enable verbose
||

//@Create cluster fails (one table is not compatible) - verbose mode
|WARNING: The following tables do not have a Primary Key or equivalent column:|
||Dba.createCluster: Instance check failed
//@ Disable verbose
||

//@ Create cluster succeeds (no incompatible table)
||

//@ Dissolve cluster at the end (clean-up)
||

//@ Finalization
||
