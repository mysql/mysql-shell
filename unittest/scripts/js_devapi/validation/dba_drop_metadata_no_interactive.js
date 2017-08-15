//@# Invalid dropMetadataSchema call
||Invalid number of arguments in Dba.dropMetadataSchema, expected 1 but got 5 (ArgumentError)

//@# drop metadata: no arguments
||Invalid number of arguments in Dba.dropMetadataSchema, expected 1 but got 0 (ArgumentError)

//@# create cluster
|<Cluster:tempCluster>|

//@# drop metadata: force false
||Dba.dropMetadataSchema: No operation executed, use the 'force' option (RuntimeError)

//@# drop metadata: force true
||
