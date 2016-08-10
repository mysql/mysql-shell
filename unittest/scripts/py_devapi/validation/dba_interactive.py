#@ Initialization
|Are you sure you want to remove the Metadata? [y/N]:|*

#@ Session: validating members
|Session Members: 7|
|default_farm: OK|
|get_default_farm: OK|
|create_farm: OK|
|drop_farm: OK|
|get_farm: OK|
|drop_metadata_schema: OK|
|reset_session: OK|

#@# AdminSession: create_farm errors
||Invalid number of arguments in AdminSession.create_farm, expected 1 to 3 but got 0
||AdminSession.create_farm: Argument #1 is expected to be a string
||AdminSession.create_farm: The Farm name cannot be empty

#@# AdminSession: create_farm with interaction
|Please enter an administration password to be used for the Farm|
|<Farm:devFarm>|

#@# AdminSession: get_farm errors
||Invalid number of arguments in AdminSession.get_farm, expected 1 but got 0
||AdminSession.get_farm: Argument #1 is expected to be a string
||Invalid number of arguments in AdminSession.get_farm, expected 1 but got 2
||AdminSession.get_farm: The Farm name cannot be empty

#@ AdminSession: get_farm
|<Farm:devFarm>|

#@# AdminSession: drop_farm errors
||Invalid number of arguments in AdminSession.drop_farm, expected 1 to 2 but got 0
||AdminSession.drop_farm: Argument #1 is expected to be a string
||AdminSession.drop_farm: The Farm name cannot be empty
||AdminSession.drop_farm: Argument #2 is expected to be a map
||Invalid number of arguments in AdminSession.drop_farm, expected 1 to 2 but got 3

#@ AdminSession: drop_farm interaction no options, cancel
|To remove the Farm 'sample' the default replica set needs to be removed.|
|Do you want to remove the default replica set? [y/n]:|

#@ AdminSession: drop_farm interaction missing option, ok error
|To remove the Farm 'sample' the default replica set needs to be removed.|
|Do you want to remove the default replica set? [y/n]:|
||||AdminSession.drop_farm: The farm with the name 'sample' does not exist.

#@ AdminSession: drop_farm interaction no options, ok success
|To remove the Farm 'devFarm' the default replica set needs to be removed.|
|Do you want to remove the default replica set? [y/n]:|
