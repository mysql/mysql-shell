//@ Initialization
||

//@<OUT> Interactive_dba_configure_local_instance read_only_no_prompts
Validating instance...

The instance 'localhost:<<<__mysql_sandbox_port1>>>' is valid for Cluster usage
You can now use it in an InnoDB Cluster.

//@<OUT> Interactive_dba_configure_local_instance read_only_no_flag_prompt_yes
The MySQL instance at 'localhost:<<<__mysql_sandbox_port1>>>' currently has the super_read_only
system variable set to protect it from inadvertent updates from applications.
You must first unset it to be able to perform any changes to this instance.
For more information see: https://dev.mysql.com/doc/refman/en/server-system-variables.html#sysvar_super_read_only.

Note: there are open sessions to 'localhost:<<<__mysql_sandbox_port1>>>'.
You may want to kill these sessions to prevent them from performing unexpected updates:

1 open session(s) of 'root@localhost'.

Do you want to disable super_read_only and continue? [y|N]:
Validating instance...

The instance 'localhost:<<<__mysql_sandbox_port1>>>' is valid for Cluster usage
You can now use it in an InnoDB Cluster.

//@<OUT> Interactive_dba_configure_local_instance read_only_no_flag_prompt_no
The MySQL instance at 'localhost:<<<__mysql_sandbox_port1>>>' currently has the super_read_only
system variable set to protect it from inadvertent updates from applications.
You must first unset it to be able to perform any changes to this instance.
For more information see: https://dev.mysql.com/doc/refman/en/server-system-variables.html#sysvar_super_read_only.

Note: there are open sessions to 'localhost:<<<__mysql_sandbox_port1>>>'.
You may want to kill these sessions to prevent them from performing unexpected updates:

1 open session(s) of 'root@localhost'.

Do you want to disable super_read_only and continue? [y|N]:
Cancelled

//@# Interactive_dba_configure_local_instance read_only_invalid_flag_value
||Dba.configureLocalInstance: Argument 'clearReadOnly' is expected to be a bool

//@ Interactive_dba_configure_local_instance read_only_flag_true
|Validating instance...|
|The instance 'localhost:<<<__mysql_sandbox_port1>>>' is valid for Cluster usage|
|You can now use it in an InnoDB Cluster.|

//@# Interactive_dba_configure_local_instance read_only_flag_false
||The MySQL instance at 'localhost:<<<__mysql_sandbox_port1>>>' currently has the super_read_only system variable set to protect it from inadvertent updates from applications. You must first unset it to be able to perform any changes to this instance. For more information see: https://dev.mysql.com/doc/refman/en/server-system-variables.html#sysvar_super_read_only. If you unset super_read_only you should consider closing the following: 1 open session(s) of 'root@localhost'.

//@ Cleanup
||
