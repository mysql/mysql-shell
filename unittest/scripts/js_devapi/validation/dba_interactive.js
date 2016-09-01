//@ Initialization
|Are you sure you want to remove the Metadata? [y/N]:|*

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
||Invalid number of arguments in Dba.createCluster, expected 1 to 3 but got 0
||Dba.createCluster: Argument #1 is expected to be a string
||Dba.createCluster: The Cluster name cannot be empty

//@# Dba: createCluster with interaction
|A new InnoDB cluster will be created on instance |
|When setting up a new InnoDB cluster it is required to define an administrative|
|MASTER key for the cluster.This MASTER key needs to be re - entered when making|
|changes to the cluster later on, e.g.adding new MySQL instances or configuring|
|MySQL Routers.Losing this MASTER key will require the configuration of all|
|InnoDB cluster entities to be changed.|
|Please specify an administrative MASTER key for the cluster 'devCluster':|
|Creating InnoDB cluster 'devCluster' on |
|Cluster successfully created. Use Cluster.addInstance() to add MySQL instances.|
|At least 3 instances are needed for the cluster to be able to withstand up to|
|one server failure.|

|<Cluster:devCluster>|

//@# Dba: getCluster errors
||ArgumentError: Unexpected parameter received expected either the InnoDB cluster name or a Dictionary with options
||ArgumentError: Unexpected parameter received expected either the InnoDB cluster name or a Dictionary with options
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
