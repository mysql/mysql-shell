//@ Session: validating members
|Session Members: 11|
|createCluster: OK|
|deleteLocalInstance: OK|
|deployLocalInstance: OK|
|dropCluster: OK|
|getCluster: OK|
|help: OK|
|killLocalInstance: OK|
|resetSession: OK|
|startLocalInstance: OK|
|validateInstance: OK|

//@# Dba: createCluster errors
||Invalid number of arguments in Dba.createCluster, expected 2 to 3 but got 0
||Invalid number of arguments in Dba.createCluster, expected 2 to 3 but got 1
||Dba.createCluster: The Cluster name cannot be empty
||Invalid number of arguments in Dba.createCluster, expected 2 to 3 but got 1
||Dba.createCluster: Cluster is already initialized. Use getCluster() to access it.

//@ Dba: createCluster
|<Cluster:devCluster>|

//@# Dba: getCluster errors
||Dba.getCluster: Unexpected parameter received expected either the InnoDB cluster name or a Dictionary with options
||Dba.getCluster: Unexpected parameter received expected either the InnoDB cluster name or a Dictionary with options
||Dba.getCluster: The Cluster name cannot be empty

//@ Dba: getCluster
|<Cluster:devCluster>|

//@ Dba: addInstance
||already belongs to the ReplicaSet: 'default'.||

//@ Dba: removeInstance
||

//@# Dba: dropCluster errors
||Invalid number of arguments in Dba.dropCluster, expected 1 to 2 but got 0
||Dba.dropCluster: Argument #1 is expected to be a string
||Dba.dropCluster: The Cluster name cannot be empty
||Dba.dropCluster: Argument #2 is expected to be a map
||Invalid number of arguments in Dba.dropCluster, expected 1 to 2 but got 3
||Dba.dropCluster: The Cluster with the name 'sample' does not exist.
||Dba.dropCluster: The Cluster with the name 'devCluster' is not empty.

//@ Dba: dropCluster
||
