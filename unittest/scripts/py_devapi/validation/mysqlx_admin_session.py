#@ Session: validating members
|Session Members: 10|
|uri: OK|
|default_farm: OK|
|get_uri: OK|
|get_default_farm: OK|
|is_open: OK|
|create_farm: OK|
|drop_farm: OK|
|get_farm: OK|
|drop_metadata_schema: OK|
|close: OK|

#@# AdminSession: create_farm errors
||Invalid number of arguments in AdminSession.create_farm, expected 2 to 3 but got 0
||Invalid number of arguments in AdminSession.create_farm, expected 2 to 3 but got 1
||AdminSession.create_farm: The Farm name cannot be empty
||Invalid number of arguments in AdminSession.create_farm, expected 2 to 3 but got 1
||AdminSession.create_farm: There is already one Farm initialized. Only one Farm is supported.

#@ AdminSession: create_farm
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
||AdminSession.drop_farm: The farm with the name 'sample' does not exist.
||AdminSession.drop_farm: The farm with the name 'devFarm' is not empty.

#@ AdminSession: drop_farm
||
