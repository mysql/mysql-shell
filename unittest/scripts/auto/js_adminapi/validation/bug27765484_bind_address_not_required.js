//@<OUT> checkInstanceConfiguration with bind_address set no error
The instance 'localhost:<<<__mysql_sandbox_port1>>>' is valid for InnoDB cluster usage.

{
    "status": "ok"
}

//@<OUT> createCluster from instance with bind_address set
Validating instance at localhost:<<<__mysql_sandbox_port1>>>...
Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as <<<hostname>>>

Instance configuration is suitable.
Creating InnoDB cluster 'testCluster' on 'root@localhost:<<<__mysql_sandbox_port1>>>'...
<Cluster:testCluster>
