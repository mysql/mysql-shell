//@# Invalid dropMetadataSchema call
||Dba.dropMetadataSchema: Invalid number of arguments, expected 0 to 1 but got 5 (ArgumentError)

//@# create cluster
|<Cluster:tempCluster>|

//@# drop metadata: force false
||Dba.dropMetadataSchema: No operation executed, use the 'force' option (RuntimeError)

//@# drop metadata: force true
||
