//@ Initialization
|||

//@ Cluster: validating members
|Cluster Members: 8|
|name: OK|
|getName: OK|
|adminType: OK|
|getAdminType: OK|
|addSeedInstance: OK|
|addInstance: OK|
|removeInstance: OK|
|getReplicaSet: OK|

//@ Cluster: addInstance, no seed instance answer no
|The default ReplicaSet is not initialized. Do you want to initialize it adding a seed instance?|

//@ Cluster: addInstance, no seed instance answer yes
|The default ReplicaSet is not initialized. Do you want to initialize it adding a seed instance?|Invalid connection options, expected either a URI or a Dictionary.

//@ Cluster: addInstance, ignore invalid attributes no ignore
|The connection data contains the next invalid attributes: data, whatever|
|Do you want to ignore these attributes and continue?|

//@ Cluster: addInstance, ignore invalid attributes ignore
Cluster.addInstance({host:"localhost", data:'sample', port:3304, whatever:5});
|The connection data contains the next invalid attributes: data, whatever|
|Do you want to ignore these attributes and continue?|

//@ Cluster: addSeedInstance, it already initialized, answer no
|The default ReplicaSet is already initialized. Do you want to add a new instance?|

//@ Cluster: addSeedInstance, it already initialized, answer yes
|The default ReplicaSet is already initialized. Do you want to add a new instance?|

//@# Cluster: addInstance errors
||Invalid number of arguments in Cluster.addInstance, expected 1 to 2 but got 0
||Invalid number of arguments in Cluster.addInstance, expected 1 to 2 but got 3
||Cluster.addInstance: Invalid connection options, expected either a URI or a Dictionary
||Cluster.addInstance: The connection data contains the next invalid attributes: authMethod, schema
||Cluster.addInstance: Missing required attribute: host

//@# Cluster: addInstance
||
