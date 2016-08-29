//@ Initialization
|Are you sure you want to remove the Metadata? [y/N]:|*

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
||Invalid number of arguments in Dba.createCluster, expected 1 to 3 but got 0
||Dba.createCluster: Argument #1 is expected to be a string
||Dba.createCluster: The Cluster name cannot be empty

//@# Dba: createCluster with interaction
|Please enter an administrative MASTER password to be used for the Cluster|
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

  //@ Dba: dropCluster interaction no options, cancel
|To remove the Cluster 'sample' the default replica set needs to be removed.|
|Do you want to remove the default replica set? [y/n]:|

  //@ Dba: dropCluster interaction missing option, ok error
|To remove the Cluster 'sample' the default replica set needs to be removed.|
|Do you want to remove the default replica set? [y/n]:|
||||Dba.dropCluster: The Cluster with the name 'sample' does not exist.

  //@ Dba: dropCluster interaction no options, ok success
|To remove the Cluster 'devCluster' the default replica set needs to be removed.|
|Do you want to remove the default replica set? [y/n]:|
