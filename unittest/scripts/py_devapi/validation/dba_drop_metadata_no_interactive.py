#@# Invalid drop_metadata_schema call
||Invalid number of arguments in Dba.drop_metadata_schema, expected 1 but got 5

#@# drop metadata: no arguments
||Invalid number of arguments in Dba.drop_metadata_schema, expected 1 but got 0

#@# create cluster
|<Cluster:tempCluster>|

#@# drop metadata: force false
||Dba.drop_metadata_schema: No operation executed, use the 'force' option

#@ Dba.drop_metadata_schema: super-read-only error (BUG#26422638)
||Dba.drop_metadata_schema: The MySQL instance at 'localhost:<<<__mysql_sandbox_port1>>>' currently has the super_read_only system variable set to protect it from inadvertent updates from applications. You must first unset it to be able to perform any changes to this instance. For more information see: https://dev.mysql.com/doc/refman/en/server-system-variables.html#sysvar_super_read_only. If you unset super_read_only you should consider closing the following: 3 open session(s) of 'root@localhost'.

#@# drop metadata: force true and clearReadOnly
||
