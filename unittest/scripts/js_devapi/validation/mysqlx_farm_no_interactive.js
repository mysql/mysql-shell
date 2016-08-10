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

//@ Farm: addSeedInstance
||

//@# Farm: addInstance errors
||Invalid number of arguments in Farm.addInstance, expected 1 but got 0
||Invalid number of arguments in Farm.addInstance, expected 1 but got 2
||Farm.addInstance: Unexpected argument on connection data
||Farm.addInstance: Unexpected argument 'schema' on connection data
||Farm.addInstance: Unexpected argument 'user' on connection data
||Farm.addInstance: Unexpected argument 'password' on connection data
||Farm.addInstance: Unexpected argument 'authMethod' on connection data
||Farm.addInstance: Missing required value for hostname
||Farm.addInstance: Connection data empty

//@# Farm: addInstance
||
