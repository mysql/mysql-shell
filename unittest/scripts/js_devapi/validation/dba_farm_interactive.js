//@ Initialization
|||

//@ Farm: validating members
|Farm Members: 8|
|name: OK|
|getName: OK|
|adminType: OK|
|getAdminType: OK|
|addSeedInstance: OK|
|addInstance: OK|
|removeInstance: OK|
|getReplicaSet: OK|

//@ Farm: addInstance, no seed instance answer no
|The default ReplicaSet is not initialized. Do you want to initialize it adding a seed instance?|

//@ Farm: addInstance, no seed instance answer yes
|The default ReplicaSet is not initialized. Do you want to initialize it adding a seed instance?|Invalid connection options, expected either a URI or a Dictionary.

//@ Farm: addInstance, ignore invalid attributes no ignore
|The connection data contains the next invalid attributes: data, whatever|
|Do you want to ignore these attributes and continue?|

//@ Farm: addInstance, ignore invalid attributes ignore
farm.addInstance({host:"localhost", data:'sample', port:3304, whatever:5});
|The connection data contains the next invalid attributes: data, whatever|
|Do you want to ignore these attributes and continue?|

//@ Farm: addSeedInstance, it already initialized, answer no
|The default ReplicaSet is already initialized. Do you want to add a new instance?|

//@ Farm: addSeedInstance, it already initialized, answer yes
|The default ReplicaSet is already initialized. Do you want to add a new instance?|

//@# Farm: addInstance errors
||Invalid number of arguments in Farm.addInstance, expected 1 to 2 but got 0
||Invalid number of arguments in Farm.addInstance, expected 1 to 2 but got 3
||Farm.addInstance: Invalid connection options, expected either a URI or a Dictionary
||Farm.addInstance: The connection data contains the next invalid attributes: authMethod, schema
||Farm.addInstance: Missing required attribute: host

//@# Farm: addInstance
||
