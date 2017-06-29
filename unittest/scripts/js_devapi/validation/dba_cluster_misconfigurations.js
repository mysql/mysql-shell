//@ Initialization
||

//@ Dba.createCluster
||

//@ Dissolve cluster (to re-create again)
||

//@ Create cluster fails (one table is not compatible)
||Dba.createCluster: ERROR: 1 table(s) do not have a Primary Key or Primary Key Equivalent (non-null unique key).

//@ Create cluster succeeds (no incompatible table)
||

//@ Dissolve cluster at the end (clean-up)
||

//@ Finalization
||
