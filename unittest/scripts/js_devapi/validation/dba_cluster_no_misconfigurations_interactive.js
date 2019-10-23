//@ Initialization
||

//@ connect
|Creating a Classic session to 'root@localhost:<<<__mysql_sandbox_port1>>>'|
|Your MySQL connection id is|
|No default schema selected; type \use <schema> to set one.|

//@<OUT> create cluster no misconfiguration: ok
A new InnoDB cluster will be created on instance 'localhost:<<<__mysql_sandbox_port1>>>'.

Validating instance at localhost:<<<__mysql_sandbox_port1>>>...
NOTE: Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as <<<hostname>>>:<<<__mysql_sandbox_port1>>>

Instance configuration is suitable.

NOTE: Group Replication will communicate with other members using '<<<hostname>>>:<<<__mysql_sandbox_gr_port1>>>'. Use the localAddress option to override.

<<<(__version_num<80011)?"WARNING: Instance 'localhost:"+__mysql_sandbox_port1+"' cannot persist Group Replication configuration since MySQL version "+__version+" does not support the SET PERSIST command (MySQL version >= 8.0.11 required). Please use the <Dba>.configureLocalInstance() command locally to persist the changes.\n":""\>>>
Creating InnoDB cluster 'dev' on 'localhost:<<<__mysql_sandbox_port1>>>'...

Adding Seed Instance...
Cluster successfully created. Use Cluster.addInstance() to add MySQL instances.
At least 3 instances are needed for the cluster to be able to withstand up to
one server failure.

//@ Finalization
||
