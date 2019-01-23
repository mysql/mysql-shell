//@<OUT> checkInstanceConfiguration with bind_address set no error
The instance 'localhost:<<<__mysql_sandbox_port1>>>' is valid for InnoDB cluster usage.

{
    "status": "ok"
}

//@<OUT> createCluster from instance with bind_address set
Validating instance at localhost:<<<__mysql_sandbox_port1>>>...
NOTE: Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as <<<hostname>>>

Instance configuration is suitable.
Creating InnoDB cluster 'testCluster' on 'root@localhost:<<<__mysql_sandbox_port1>>>'...

Adding Seed Instance...
<<<(__version_num<80011) ? "WARNING: Instance 'localhost:"+__mysql_sandbox_port1+"' cannot persist Group Replication configuration since MySQL version "+__version+" does not support the SET PERSIST command (MySQL version >= 8.0.11 required). Please use the <Dba>.configureLocalInstance() command locally to persist the changes.\n":""\>>>
Cluster successfully created. Use Cluster.addInstance() to add MySQL instances.
At least 3 instances are needed for the cluster to be able to withstand up to
one server failure.

<Cluster:testCluster>