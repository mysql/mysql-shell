#@ Session: validating members
|Session Members: 9|
|default_farm: OK|
|get_default_farm: OK|
|create_farm: OK|
|drop_farm: OK|
|get_farm: OK|
|drop_metadata_schema: OK|
|reset_session: OK|
|validate_instance: OK|
|deploy_local_instance: OK|

#@# Dba: create_farm errors
||Invalid number of arguments in Dba.create_farm, expected 2 to 3 but got 0
||Invalid number of arguments in Dba.create_farm, expected 2 to 3 but got 1
||Dba.create_farm: The Farm name cannot be empty
||Invalid number of arguments in Dba.create_farm, expected 2 to 3 but got 1
||Dba.create_farm: There is already one Farm initialized. Only one Farm is supported.

#@ Dba: create_farm
|<Farm:devFarm>|

#@# Dba: get_farm errors
||Invalid number of arguments in Dba.get_farm, expected 1 but got 0
||Dba.get_farm: Argument #1 is expected to be a string
||Invalid number of arguments in Dba.get_farm, expected 1 but got 2
||Dba.get_farm: The Farm name cannot be empty

#@ Dba: get_farm
|<Farm:devFarm>|

#@ Dba: add_seed_instance
||


#@# Dba: drop_farm errors
||Invalid number of arguments in Dba.drop_farm, expected 1 to 2 but got 0
||Dba.drop_farm: Argument #1 is expected to be a string
||Dba.drop_farm: The Farm name cannot be empty
||Dba.drop_farm: Argument #2 is expected to be a map
||Invalid number of arguments in Dba.drop_farm, expected 1 to 2 but got 3
||Dba.drop_farm: The farm with the name 'sample' does not exist.
||Dba.drop_farm: The farm with the name 'devFarm' is not empty.

#@ Dba: drop_farm
||
