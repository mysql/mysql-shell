#@ Farm: validating members
|Farm Members: 8|
|name: OK|
|get_name: OK|
|admin_type: OK|
|get_admin_type: OK|
|add_seed_instance: OK|
|add_instance: OK|
|remove_instance: OK|
|get_replica_set: OK|

#@ Farm: add_seed_instance
||

#@# Farm: add_instance errors
||Invalid number of arguments in Farm.add_instance, expected 1 but got 0
||Invalid number of arguments in Farm.add_instance, expected 1 but got 2
||Farm.add_instance: Unexpected argument on connection data
||Farm.add_instance: Unexpected argument 'schema' on connection data
||Farm.add_instance: Unexpected argument 'user' on connection data
||Farm.add_instance: Unexpected argument 'password' on connection data
||Farm.add_instance: Unexpected argument 'authMethod' on connection data
||Farm.add_instance: Missing required value for hostname
||Farm.add_instance: Connection data empty

#@# Farm: add_instance
||
