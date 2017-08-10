|<Cluster:tempCluster>|
//@# Invalid dropMetadataSchema call
||Invalid number of arguments in Dba.dropMetadataSchema, expected 0 to 1 but got 5 (ArgumentError)
||Invalid values in drop options: not_valid (ArgumentError)

//@# drop metadata: no user response
|No changes made to the Metadata Schema.|

//@# drop metadata: user response no
|No changes made to the Metadata Schema.|

//@# drop metadata: force option
|No changes made to the Metadata Schema.|
|Metadata Schema successfully removed.|

//@<OUT> Dba.dropMetadataSchema: super-read-only error (BUG#26422638)
Are you sure you want to remove the Metadata? [y|N]:  [y|N]: The MySQL instance at 'localhost:<<<__mysql_sandbox_port1>>>' currently has the super_read_only
system variable set to protect it from inadvertent updates from applications.
You must first unset it to be able to perform any changes to this instance.
For more information see: https://dev.mysql.com/doc/refman/en/server-system-variables.html#sysvar_super_read_only.

Note: there are open sessions to 'localhost:<<<__mysql_sandbox_port1>>>'.
You may want to kill these sessions to prevent them from performing unexpected updates:

4 open session(s) of 'root@localhost'.

Do you want to disable super_read_only and continue? [y|N]:
Cancelled

//@<OUT> drop metadata: user response yes to force and clearReadOnly
Are you sure you want to remove the Metadata? [y|N]:  [y|N]: The MySQL instance at 'localhost:<<<__mysql_sandbox_port1>>>' currently has the super_read_only
system variable set to protect it from inadvertent updates from applications.
You must first unset it to be able to perform any changes to this instance.
For more information see: https://dev.mysql.com/doc/refman/en/server-system-variables.html#sysvar_super_read_only.

Note: there are open sessions to 'localhost:<<<__mysql_sandbox_port1>>>'.
You may want to kill these sessions to prevent them from performing unexpected updates:

4 open session(s) of 'root@localhost'.

Do you want to disable super_read_only and continue? [y|N]:
Metadata Schema successfully removed.
