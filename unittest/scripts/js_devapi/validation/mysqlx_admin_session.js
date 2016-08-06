//@ Session: validating members
|Session Members: 10|
|uri: OK|
|defaultFarm: OK|
|getUri: OK|
|getDefaultFarm: OK|
|isOpen: OK|
|createFarm: OK|
|dropFarm: OK|
|getFarm: OK|
|close: OK|

//@# AdminSession: createFarm errors
||Invalid number of arguments in AdminSession.createFarm, expected 2 to 3 but got 0
||Invalid number of arguments in AdminSession.createFarm, expected 2 to 3 but got 1
||AdminSession.createFarm: The Farm name cannot be empty
||Invalid number of arguments in AdminSession.createFarm, expected 2 to 3 but got 1
||AdminSession.createFarm: There is already one Farm initialized. Only one Farm is supported.

//@ AdminSession: createFarm
|<Farm:devFarm>|

//@# AdminSession: getFarm errors
||Invalid number of arguments in AdminSession.getFarm, expected 1 but got 0
||AdminSession.getFarm: Argument #1 is expected to be a string
||Invalid number of arguments in AdminSession.getFarm, expected 1 but got 2
||AdminSession.getFarm: The Farm name cannot be empty

//@ AdminSession: getFarm
|<Farm:devFarm>|


//@# AdminSession: dropFarm errors
||Invalid number of arguments in AdminSession.dropFarm, expected 1 to 2 but got 0
||AdminSession.dropFarm: Argument #1 is expected to be a string
||AdminSession.dropFarm: The Farm name cannot be empty
||AdminSession.dropFarm: Argument #2 is expected to be a map
||Invalid number of arguments in AdminSession.dropFarm, expected 1 to 2 but got 3
||AdminSession.dropFarm: The farm with the name 'sample' does not exist.
||AdminSession.dropFarm: The farm with the name 'devFarm' is not empty.

//@ AdminSession: dropFarm
||
