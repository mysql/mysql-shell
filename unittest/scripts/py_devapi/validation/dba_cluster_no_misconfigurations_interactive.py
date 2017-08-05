#@ Initialization
||

#@ connect
|Creating a Classic Session to 'mysql://root@localhost:<<<__mysql_sandbox_port1>>>'|
|Your MySQL connection id is|
|No default schema selected; type \use <schema> to set one.|

#@<OUT> create cluster no misconfiguration: ok
A new InnoDB cluster will be created on instance 'mysql://root@localhost:<<<__mysql_sandbox_port1>>>'.

Creating InnoDB cluster 'dev' on 'mysql://root@localhost:<<<__mysql_sandbox_port1>>>'...
Adding Seed Instance...

Cluster successfully created. Use Cluster.add_instance() to add MySQL instances.
At least 3 instances are needed for the cluster to be able to withstand up to
one server failure.

#@ Finalization
||
