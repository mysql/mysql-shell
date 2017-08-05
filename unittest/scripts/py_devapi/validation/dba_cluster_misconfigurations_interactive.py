#@ Initialization
||

#@ connect and change vars
|Creating a Classic Session to 'mysql://root@localhost:<<<__mysql_sandbox_port1>>>'|
|Your MySQL connection id is|
|No default schema selected; type \use <schema> to set one.|
|Query OK, 0 rows affected|
|Query OK, 0 rows affected|

#@<OUT> Dba.createCluster: cancel
A new InnoDB cluster will be created on instance 'mysql://root@localhost:<<<__mysql_sandbox_port1>>>'.

Warning: The instance configuration needs to be changed in order to
create an InnoDB cluster. To see which changes will be made, please
use the dba.check_instance_configuration() function before confirming
to change the configuration.

Should the configuration be changed accordingly? [y|N]:
Cancelled

#@<OUT> Dba.createCluster: ok
A new InnoDB cluster will be created on instance 'mysql://root@localhost:<<<__mysql_sandbox_port1>>>'.

Warning: The instance configuration needs to be changed in order to
create an InnoDB cluster. To see which changes will be made, please
use the dba.check_instance_configuration() function before confirming
to change the configuration.

Should the configuration be changed accordingly? [y|N]:
Creating InnoDB cluster 'dev' on 'mysql://root@localhost:<<<__mysql_sandbox_port1>>>'...
Adding Seed Instance...

Cluster successfully created. Use Cluster.add_instance() to add MySQL instances.
At least 3 instances are needed for the cluster to be able to withstand up to
one server failure.

#@ Finalization
||
