//@ Session: validating members
|Session Members: 13|
|createCluster: OK|
|deleteSandboxInstance: OK|
|deploySandboxInstance: OK|
|getCluster: OK|
|help: OK|
|killSandboxInstance: OK|
|resetSession: OK|
|startSandboxInstance: OK|
|validateInstance: OK|
|stopSandboxInstance: OK|
|dropMetadataSchema: OK|
|prepareInstance: OK|
|verbose: OK|

//@# Dba: createCluster errors
||ArgumentError: Invalid number of arguments in Dba.createCluster, expected 1 to 2 but got 0
||Argument #1 is expected to be a string
||Dba.createCluster: The Cluster name cannot be empty
||Argument #2 is expected to be a map
||Invalid values in the options: another, invalid

//@# Dba: createCluster succeed
|<Cluster:devCluster>|

//@# Dba: createCluster already exist
||Dba.createCluster: Cluster is already initialized. Use getCluster() to access it.

//@# Dba: getCluster errors
||Invalid cluster name: Argument #1 is expected to be a string
||Invalid number of arguments in Dba.getCluster, expected 0 to 1 but got 2
||Dba.getCluster: The Cluster name cannot be empty

//@ Dba: getCluster
|<Cluster:devCluster>|
