//@ Initialization
||

//@ connect
|Creating a Classic session to 'root@localhost:<<<__mysql_sandbox_port1>>>'|
|Your MySQL connection id is|
|No default schema selected; type \use <schema> to set one.|

//@<OUT> create cluster no misconfiguration: ok
A new InnoDB cluster will be created on instance 'root@localhost:<<<__mysql_sandbox_port1>>>'.

Validating instance at localhost:<<<__mysql_sandbox_port1>>>...
Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as <<<real_hostname>>>

Instance configuration is suitable.
Creating InnoDB cluster 'dev' on 'root@localhost:<<<__mysql_sandbox_port1>>>'...
<<<(__version_num<80005)?"WARNING: On instance 'localhost:"+__mysql_sandbox_port1+"' membership change cannot be persisted since MySQL version " + __version + " does not support the SET PERSIST command (MySQL version >= 8.0.5 required). Please use the <Dba>.configureLocalInstance command locally to persist the changes.\n":""\>>>
Adding Seed Instance...

Cluster successfully created. Use Cluster.addInstance() to add MySQL instances.
At least 3 instances are needed for the cluster to be able to withstand up to
one server failure.

//@ Finalization
||
