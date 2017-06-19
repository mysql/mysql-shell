//@ Initialization
||

//@ connect and change vars
|Creating a Classic Session to 'root@localhost:<<<__mysql_sandbox_port1>>>'|
|Session successfully established. No default schema selected.|
|Query OK, 0 rows affected|
|Query OK, 0 rows affected|

//@<OUT> Dba.createCluster: cancel
A new InnoDB cluster will be created on instance 'root@localhost:<<<__mysql_sandbox_port1>>>'.

Warning: The instance configuration needs to be changed in order to
create an InnoDB cluster. To see which changes will be made, please
use the dba.checkInstanceConfiguration() function before confirming
to change the configuration.

Should the configuration be changed accordingly? [Y|n]:
Cancelled

//@<OUT> Dba.createCluster: ok
A new InnoDB cluster will be created on instance 'root@localhost:<<<__mysql_sandbox_port1>>>'.

Warning: The instance configuration needs to be changed in order to
create an InnoDB cluster. To see which changes will be made, please
use the dba.checkInstanceConfiguration() function before confirming
to change the configuration.

Should the configuration be changed accordingly? [Y|n]:
Creating InnoDB cluster 'dev' on 'root@localhost:<<<__mysql_sandbox_port1>>>'...
Adding Seed Instance...

Cluster successfully created. Use Cluster.addInstance() to add MySQL instances.
At least 3 instances are needed for the cluster to be able to withstand up to
one server failure.

//@ Finalization
||
