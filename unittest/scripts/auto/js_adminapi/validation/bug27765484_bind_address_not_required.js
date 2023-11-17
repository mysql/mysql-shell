//@<OUT> checkInstanceConfiguration with bind_address set no error
The instance '<<<hostname>>>:<<<__mysql_sandbox_port1>>>' is valid for InnoDB Cluster usage.

{
    "status": "ok"
}

//@<OUT> createCluster from instance with bind_address set
Validating instance configuration at localhost:<<<__mysql_sandbox_port1>>>...
NOTE: Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as <<<hostname>>>:<<<__mysql_sandbox_port1>>>

Instance configuration is suitable.
NOTE: Group Replication will communicate with other members using '<<<hostname>>>:<<<__mysql_sandbox_gr_port1>>>'. Use the localAddress option to override.

<<<(__version_num<80027)?"NOTE: The 'localAddress' \"" + hostname + "\" is [[*]], which is compatible with the Group Replication automatically generated list of IPs.\n":""\>>>
<<<(__version_num<80027)?"See https://dev.mysql.com/doc/refman/en/group-replication-ip-address-permissions.html for more details.\n":""\>>>
<<<(__version_num<80027)?"NOTE: When adding more instances to the Cluster, be aware that the subnet masks dictate whether the instance's address is automatically added to the allowlist or not. Please specify the 'ipAllowlist' accordingly if needed.\n":""\>>>
* Checking connectivity and SSL configuration...

<<<(__version_num<80011) ? "WARNING: Instance '"+hostname+":"+__mysql_sandbox_port1+"' cannot persist Group Replication configuration since MySQL version "+__version+" does not support the SET PERSIST command (MySQL version >= 8.0.11 required). Please use the dba.configureLocalInstance() command locally to persist the changes.\n":""\>>>
Creating InnoDB Cluster 'testCluster' on '<<<hostname>>>:<<<__mysql_sandbox_port1>>>'...

Adding Seed Instance...
Cluster successfully created. Use Cluster.addInstance() to add MySQL instances.
At least 3 instances are needed for the cluster to be able to withstand up to
one server failure.

<Cluster:testCluster>
