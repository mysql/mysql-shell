//@ Initialization
||

//@ create cluster admin
|"status": "ok"|

//@ Dba.createCluster
||

//@ Dissolve cluster (to re-create again)
||

//@ Create cluster fails (one table is not compatible)
||Dba.createCluster: ERROR: 1 table(s) do not have a Primary Key or Primary Key Equivalent (non-null unique key).

//@ Enable verbose
||

//@Create cluster fails (one table is not compatible) - verbose mode
||ERROR: Error starting cluster: The operation could not continue due to the following requirements not being met:
||Non-compatible tables found in database.
//@ Disable verbose
||

//@ Create cluster succeeds (no incompatible table)
||

//@ Dissolve cluster at the end (clean-up)
||

//@ Finalization
||
