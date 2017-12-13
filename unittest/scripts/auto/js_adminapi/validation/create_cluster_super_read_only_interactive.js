//@ Initialization
||

//@ prepare create_cluster.read_only_no_prompts
||

//@<OUT> create_cluster.read_only_no_prompts
A new InnoDB cluster will be created on instance 'root@localhost:<<<__mysql_sandbox_port1>>>'.

Creating InnoDB cluster 'dev' on 'root@localhost:<<<__mysql_sandbox_port1>>>'...
Adding Seed Instance...

Cluster successfully created. Use Cluster.addInstance() to add MySQL instances.
At least 3 instances are needed for the cluster to be able to withstand up to
one server failure.

//@ prepare create_cluster.read_only_no_flag_prompt_yes
||

//@<OUT> create_cluster.read_only_no_flag_prompt_yes
A new InnoDB cluster will be created on instance 'root@localhost:<<<__mysql_sandbox_port1>>>'.

The MySQL instance at 'localhost:<<<__mysql_sandbox_port1>>>' currently has the super_read_only
system variable set to protect it from inadvertent updates from applications.
You must first unset it to be able to perform any changes to this instance.
For more information see: https://dev.mysql.com/doc/refman/en/server-system-variables.html#sysvar_super_read_only.

Note: there are open sessions to 'localhost:<<<__mysql_sandbox_port1>>>'.
You may want to kill these sessions to prevent them from performing unexpected updates:

1 open session(s) of 'root@localhost'.

Do you want to disable super_read_only and continue? [y|N]:


//@ prepare create_cluster.read_only_no_flag_prompt_no
||

//@<OUT> create_cluster.read_only_no_flag_prompt_no
A new InnoDB cluster will be created on instance 'root@localhost:<<<__mysql_sandbox_port1>>>'.

The MySQL instance at 'localhost:<<<__mysql_sandbox_port1>>>' currently has the super_read_only
system variable set to protect it from inadvertent updates from applications.
You must first unset it to be able to perform any changes to this instance.
For more information see: https://dev.mysql.com/doc/refman/en/server-system-variables.html#sysvar_super_read_only.

Note: there are open sessions to 'localhost:<<<__mysql_sandbox_port1>>>'.
You may want to kill these sessions to prevent them from performing unexpected updates:

1 open session(s) of 'root@localhost'.

Do you want to disable super_read_only and continue? [y|N]:
Cancelled

//@ prepare create_cluster.read_only_invalid_flag_value
||

//@# create_cluster.read_only_invalid_flag_value
||Dba.createCluster: Argument 'clearReadOnly' is expected to be a bool

//@ prepare create_cluster.read_only_flag_true
||

//@<OUT> create_cluster.read_only_flag_true
A new InnoDB cluster will be created on instance 'root@localhost:<<<__mysql_sandbox_port1>>>'.

Creating InnoDB cluster 'dev' on 'root@localhost:<<<__mysql_sandbox_port1>>>'...
Adding Seed Instance...

Cluster successfully created. Use Cluster.addInstance() to add MySQL instances.
At least 3 instances are needed for the cluster to be able to withstand up to
one server failure.

//@ prepare create_cluster.read_only_flag_false
||

//@# create_cluster.read_only_flag_false
||The MySQL instance at 'localhost:<<<__mysql_sandbox_port1>>>' currently has the super_read_only system variable set to protect


//@ Cleanup
||
