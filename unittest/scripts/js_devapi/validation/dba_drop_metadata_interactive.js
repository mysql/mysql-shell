|<Cluster:tempCluster>|
//@# Invalid dropMetadataSchema call
||Dba.dropMetadataSchema: Invalid number of arguments, expected 0 to 1 but got 5 (ArgumentError)
||Dba.dropMetadataSchema: Invalid options: not_valid (ArgumentError)

//@# drop metadata: no user response
|No changes made to the Metadata Schema.|

//@# drop metadata: user response no
|No changes made to the Metadata Schema.|

//@# drop metadata: expect prompt, user response no
|No changes made to the Metadata Schema.|

//@# drop metadata: force option
|No changes made to the Metadata Schema.|
|Metadata Schema successfully removed.|

//@# drop metadata: user response yes
|<Cluster:tempCluster>|
|Metadata Schema successfully removed.|
