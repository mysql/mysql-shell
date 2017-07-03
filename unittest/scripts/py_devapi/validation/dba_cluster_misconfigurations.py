#@ Initialization
||

#@ Dba.createCluster
||

#@ Dissolve cluster (to re-create again)
||

#@ Create cluster fails (one table is not compatible)
||SystemError: RuntimeError: Dba.create_cluster: ERROR: 1 table(s) do not have a Primary Key or Primary Key Equivalent (non-null unique key).

#@ Enable verbose
||

#@<ERR> Create cluster fails (one table is not compatible) - verbose mode
* Checking compliance of existing tables... FAIL
ERROR: 1 table(s) do not have a Primary Key or Primary Key Equivalent (non-null unique key).
	pke_test.t3

Group Replication requires tables to use InnoDB and have a PRIMARY KEY or PRIMARY KEY Equivalent (non-null unique key). Tables that do not follow these requirements will be readable but not updateable when used with Group Replication. If your applications make updates (INSERT, UPDATE or DELETE) to these tables, ensure they use the InnoDB storage engine and have a PRIMARY KEY or PRIMARY KEY Equivalent.


ERROR: Error starting cluster: The operation could not continue due to the following requirements not being met:
Non-compatible tables found in database.
==============================================================================

#@ Disable verbose
||

#@ Create cluster succeeds (no incompatible table)
||

#@ Dissolve cluster at the end (clean-up)
||

#@ Finalization
||
