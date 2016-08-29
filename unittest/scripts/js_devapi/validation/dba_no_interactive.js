//@ Session: validating members
|Session Members: 9|
|defaultCluster: OK|
|getDefaultCluster: OK|
|createCluster: OK|
|dropCluster: OK|
|getCluster: OK|
|dropMetadataSchema: OK|
|resetSession: OK|
|validateInstance: OK|
|deployLocalInstance: OK|

//@# Dba: createCluster errors
||Invalid number of arguments in Dba.createCluster, expected 2 to 3 but got 0
||Invalid number of arguments in Dba.createCluster, expected 2 to 3 but got 1
||Dba.createCluster: The Cluster name cannot be empty
||Invalid number of arguments in Dba.createCluster, expected 2 to 3 but got 1
||Dba.createCluster: There is already one Cluster initialized. Only one Cluster is supported.

//@ Dba: createCluster
|<Cluster:devCluster>|

//@# Dba: getCluster errors
||Invalid number of arguments in Dba.getCluster, expected 1 but got 0
||Dba.getCluster: Argument #1 is expected to be a string
||Invalid number of arguments in Dba.getCluster, expected 1 but got 2
||Dba.getCluster: The Cluster name cannot be empty

//@ Dba: getCluster
|<Cluster:devCluster>|

//@ Dba: addSeedInstance
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
