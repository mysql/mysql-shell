//@ Session: validating members
|Session Members: 9|
|defaultFarm: OK|
|getDefaultFarm: OK|
|createFarm: OK|
|dropFarm: OK|
|getFarm: OK|
|dropMetadataSchema: OK|
|resetSession: OK|
|validateInstance: OK|
|deployLocalInstance: OK|

//@# Dba: createFarm errors
||Invalid number of arguments in Dba.createFarm, expected 2 to 3 but got 0
||Invalid number of arguments in Dba.createFarm, expected 2 to 3 but got 1
||Dba.createFarm: The Farm name cannot be empty
||Invalid number of arguments in Dba.createFarm, expected 2 to 3 but got 1
||Dba.createFarm: There is already one Farm initialized. Only one Farm is supported.

//@ Dba: createFarm
|<Farm:devFarm>|

//@# Dba: getFarm errors
||Invalid number of arguments in Dba.getFarm, expected 1 but got 0
||Dba.getFarm: Argument #1 is expected to be a string
||Invalid number of arguments in Dba.getFarm, expected 1 but got 2
||Dba.getFarm: The Farm name cannot be empty

//@ Dba: getFarm
|<Farm:devFarm>|

//@ Dba: addSeedInstance
||


//@# Dba: dropFarm errors
||Invalid number of arguments in Dba.dropFarm, expected 1 to 2 but got 0
||Dba.dropFarm: Argument #1 is expected to be a string
||Dba.dropFarm: The Farm name cannot be empty
||Dba.dropFarm: Argument #2 is expected to be a map
||Invalid number of arguments in Dba.dropFarm, expected 1 to 2 but got 3
||Dba.dropFarm: The farm with the name 'sample' does not exist.
||Dba.dropFarm: The farm with the name 'devFarm' is not empty.

//@ Dba: dropFarm
||
