//@ Initialization
|Are you sure you want to remove the Metadata? [y/N]:|*

//@ Session: validating members
|Session Members: 7|
|defaultFarm: OK|
|getDefaultFarm: OK|
|createFarm: OK|
|dropFarm: OK|
|getFarm: OK|
|dropMetadataSchema: OK|
|resetSession: OK|

//@# AdminSession: createFarm errors
||Invalid number of arguments in AdminSession.createFarm, expected 1 to 3 but got 0
||AdminSession.createFarm: Argument #1 is expected to be a string
||AdminSession.createFarm: The Farm name cannot be empty

//@# AdminSession: createFarm with interaction
|Please enter an administration password to be used for the Farm|
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

//@ AdminSession: dropFarm interaction no options, cancel
|To remove the Farm 'sample' the default replica set needs to be removed.|
|Do you want to remove the default replica set? [y/n]:|

//@ AdminSession: dropFarm interaction missing option, ok error
|To remove the Farm 'sample' the default replica set needs to be removed.|
|Do you want to remove the default replica set? [y/n]:|
||||AdminSession.dropFarm: The farm with the name 'sample' does not exist.

//@ AdminSession: dropFarm interaction no options, ok success
|To remove the Farm 'devFarm' the default replica set needs to be removed.|
|Do you want to remove the default replica set? [y/n]:|
