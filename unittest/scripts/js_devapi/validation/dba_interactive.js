//@ Initialization
|Are you sure you want to remove the Metadata? [y/N]:|*

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
||Invalid number of arguments in Dba.createFarm, expected 1 to 3 but got 0
||Dba.createFarm: Argument #1 is expected to be a string
||Dba.createFarm: The Farm name cannot be empty

//@# Dba: createFarm with interaction
|Please enter an administration password to be used for the Farm|
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

  //@ Dba: dropFarm interaction no options, cancel
|To remove the Farm 'sample' the default replica set needs to be removed.|
|Do you want to remove the default replica set? [y/n]:|

  //@ Dba: dropFarm interaction missing option, ok error
|To remove the Farm 'sample' the default replica set needs to be removed.|
|Do you want to remove the default replica set? [y/n]:|
||||Dba.dropFarm: The farm with the name 'sample' does not exist.

  //@ Dba: dropFarm interaction no options, ok success
|To remove the Farm 'devFarm' the default replica set needs to be removed.|
|Do you want to remove the default replica set? [y/n]:|
