//@ Session: validating members
|Session Members: 12|
|createCluster: OK|
|deleteLocalInstance: OK|
|deployLocalInstance: OK|
|getCluster: OK|
|help: OK|
|killLocalInstance: OK|
|resetSession: OK|
|startLocalInstance: OK|
|validateInstance: OK|
|stopLocalInstance: OK|
|dropMetadataSchema: OK|
|verbose: OK|

//@# Dba: createCluster errors
||Invalid number of arguments in Dba.createCluster, expected 2 to 3 but got 0
||Invalid number of arguments in Dba.createCluster, expected 2 to 3 but got 1
||Dba.createCluster: The Cluster name cannot be empty
||Invalid number of arguments in Dba.createCluster, expected 2 to 3 but got 1

//@# Dba: createCluster succeed
|<Cluster:devCluster>|

//@# Dba: createCluster already exist
||Dba.createCluster: Cluster is already initialized. Use getCluster() to access it.

//@# Dba: getCluster errors
||Dba.getCluster: Unexpected parameter received expected either the InnoDB cluster name or a Dictionary with options
||Dba.getCluster: Unexpected parameter received expected either the InnoDB cluster name or a Dictionary with options
||Dba.getCluster: The Cluster name cannot be empty
||Dba.getCluster: Authentication failure: wrong MASTER key.

//@ Dba: getCluster
|<Cluster:devCluster>|
