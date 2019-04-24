//@ Initialization
||

//@ prepare create_cluster.read_only_no_prompts
||

//@<OUT> create_cluster.read_only_no_prompts
A new InnoDB cluster will be created on instance 'localhost:<<<__mysql_sandbox_port1>>>'.

Validating instance at localhost:<<<__mysql_sandbox_port1>>>...
NOTE: Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as <<<hostname>>>:<<<__mysql_sandbox_port1>>>

Instance configuration is suitable.

//@<OUT> create_cluster.read_only_no_prompts {VER(>=8.0.11)}
Creating InnoDB cluster 'dev' on 'localhost:<<<__mysql_sandbox_port1>>>'...

Adding Seed Instance...
Cluster successfully created. Use Cluster.addInstance() to add MySQL instances.
At least 3 instances are needed for the cluster to be able to withstand up to
one server failure.

//@<OUT> create_cluster.read_only_no_prompts {VER(<8.0.11)}
WARNING: Instance 'localhost:<<<__mysql_sandbox_port1>>>' cannot persist Group Replication configuration since MySQL version <<<__version>>> does not support the SET PERSIST command (MySQL version >= 8.0.11 required). Please use the <Dba>.configureLocalInstance() command locally to persist the changes.
Creating InnoDB cluster 'dev' on 'localhost:<<<__mysql_sandbox_port1>>>'...

Adding Seed Instance...
Cluster successfully created. Use Cluster.addInstance() to add MySQL instances.
At least 3 instances are needed for the cluster to be able to withstand up to
one server failure.

//@ prepare create_cluster.read_only_no_flag_prompt_yes
||

//@<OUT> create_cluster.read_only_no_flag_prompt_yes
Instance configuration is suitable.

The MySQL instance at 'localhost:<<<__mysql_sandbox_port1>>>' currently has the super_read_only system
variable set to protect it from inadvertent updates from applications. You must
first unset it to be able to perform any changes to this instance.
For more information see:
https://dev.mysql.com/doc/refman/en/server-system-variables.html#sysvar_super_read_only.

NOTE: There are open sessions to 'localhost:<<<__mysql_sandbox_port1>>>'.
You may want to kill these sessions to prevent them from performing unexpected updates:

1 open session(s) of 'root@localhost'.

Do you want to disable super_read_only and continue? [y/N]:


//@ prepare create_cluster.read_only_no_flag_prompt_no
||

//@ create_cluster.read_only_no_flag_prompt_no
|A new InnoDB cluster will be created on instance 'localhost:<<<__mysql_sandbox_port1>>>'.|
||
|The MySQL instance at 'localhost:<<<__mysql_sandbox_port1>>>' currently has the super_read_only system|
|variable set to protect it from inadvertent updates from applications. You must|
|first unset it to be able to perform any changes to this instance.|
|For more information see:|
|https://dev.mysql.com/doc/refman/en/server-system-variables.html#sysvar_super_read_only.|
||
|NOTE: There are open sessions to 'localhost:<<<__mysql_sandbox_port1>>>'.|
|You may want to kill these sessions to prevent them from performing unexpected updates:|
||
|1 open session(s) of 'root@localhost'.|
||
|Do you want to disable super_read_only and continue? [y/N]:|
||Dba.createCluster: Cancelled (RuntimeError)

//@ prepare create_cluster.read_only_invalid_flag_value
||

//@# create_cluster.read_only_invalid_flag_value
||Dba.createCluster: Option 'clearReadOnly' Bool expected, but value is String (TypeError)

//@ prepare create_cluster.read_only_flag_true
||

//@<OUT> create_cluster.read_only_flag_true
A new InnoDB cluster will be created on instance 'localhost:<<<__mysql_sandbox_port1>>>'.

Validating instance at localhost:<<<__mysql_sandbox_port1>>>...
NOTE: Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as <<<hostname>>>:<<<__mysql_sandbox_port1>>>

Instance configuration is suitable.

//@<OUT> create_cluster.read_only_flag_true {VER(>=8.0.11)}
Creating InnoDB cluster 'dev' on 'localhost:<<<__mysql_sandbox_port1>>>'...

Adding Seed Instance...
Cluster successfully created. Use Cluster.addInstance() to add MySQL instances.
At least 3 instances are needed for the cluster to be able to withstand up to
one server failure.

//@<OUT> create_cluster.read_only_flag_true {VER(<8.0.11)}
WARNING: Instance 'localhost:<<<__mysql_sandbox_port1>>>' cannot persist Group Replication configuration since MySQL version <<<__version>>> does not support the SET PERSIST command (MySQL version >= 8.0.11 required). Please use the <Dba>.configureLocalInstance() command locally to persist the changes.
Creating InnoDB cluster 'dev' on 'localhost:<<<__mysql_sandbox_port1>>>'...

Adding Seed Instance...
Cluster successfully created. Use Cluster.addInstance() to add MySQL instances.
At least 3 instances are needed for the cluster to be able to withstand up to
one server failure.

//@ prepare create_cluster.read_only_flag_false
||

//@# create_cluster.read_only_flag_false
||Server in SUPER_READ_ONLY mode


//@ Cleanup
||
