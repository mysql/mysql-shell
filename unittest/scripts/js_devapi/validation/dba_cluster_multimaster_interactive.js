//@<OUT> Dba: createCluster multiPrimary with interaction, cancel
A new InnoDB cluster will be created on instance 'localhost:<<<__mysql_sandbox_port1>>>'.

The MySQL InnoDB cluster is going to be setup in advanced Multi-Primary Mode.
Before continuing you have to confirm that you understand the requirements and
limitations of Multi-Primary Mode. For more information see
https://dev.mysql.com/doc/refman/en/group-replication-limitations.html before
proceeding.

I have read the MySQL InnoDB cluster manual and I understand the requirements
and limitations of advanced Multi-Primary Mode.

Confirm [y/N]:

//@<ERR> Dba: createCluster multiPrimary with interaction, cancel
Dba.createCluster: Cancelled (RuntimeError)

//@<OUT> Dba: createCluster multiPrimary with interaction, ok
A new InnoDB cluster will be created on instance 'localhost:<<<__mysql_sandbox_port1>>>'.

The MySQL InnoDB cluster is going to be setup in advanced Multi-Primary Mode.
Before continuing you have to confirm that you understand the requirements and
limitations of Multi-Primary Mode. For more information see
https://dev.mysql.com/doc/refman/en/group-replication-limitations.html before
proceeding.

I have read the MySQL InnoDB cluster manual and I understand the requirements
and limitations of advanced Multi-Primary Mode.

Confirm [y/N]:
Validating instance configuration at localhost:<<<__mysql_sandbox_port1>>>...
NOTE: Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as <<<hostname>>>:<<<__mysql_sandbox_port1>>>

Instance configuration is suitable.
NOTE: Group Replication will communicate with other members using '<<<hostname>>>:<<<__mysql_sandbox_gr_port1>>>'. Use the localAddress option to override.

<<<(__version_num<80011)?"WARNING: Instance '"+hostname+":"+__mysql_sandbox_port1+"' cannot persist Group Replication configuration since MySQL version "+__version+" does not support the SET PERSIST command (MySQL version >= 8.0.11 required). Please use the dba.configureLocalInstance() command locally to persist the changes.\n":""\>>>
Creating InnoDB cluster 'devCluster' on '<<<hostname>>>:<<<__mysql_sandbox_port1>>>'...

The MySQL InnoDB cluster is going to be setup in advanced Multi-Primary Mode. Consult its requirements and limitations in https://dev.mysql.com/doc/refman/en/group-replication-limitations.html
Adding Seed Instance...
Cluster successfully created. Use Cluster.addInstance() to add MySQL instances.
At least 3 instances are needed for the cluster to be able to withstand up to
one server failure.

<Cluster:devCluster>

//@ Dissolve cluster
|The cluster was successfully dissolved.|

//@<OUT> Dba: createCluster multiMaster with interaction, regression for BUG#25926603
WARNING: The multiMaster option is deprecated. Please use the multiPrimary option instead.

A new InnoDB cluster will be created on instance 'localhost:<<<__mysql_sandbox_port1>>>'.

The MySQL InnoDB cluster is going to be setup in advanced Multi-Primary Mode.
Before continuing you have to confirm that you understand the requirements and
limitations of Multi-Primary Mode. For more information see
https://dev.mysql.com/doc/refman/en/group-replication-limitations.html before
proceeding.

I have read the MySQL InnoDB cluster manual and I understand the requirements
and limitations of advanced Multi-Primary Mode.

Confirm [y/N]:
Disabling super_read_only mode on instance '<<<hostname>>>:<<<__mysql_sandbox_port1>>>'.
Validating instance configuration at localhost:<<<__mysql_sandbox_port1>>>...
NOTE: Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as <<<hostname>>>:<<<__mysql_sandbox_port1>>>

Instance configuration is suitable.
NOTE: Group Replication will communicate with other members using '<<<hostname>>>:<<<__mysql_sandbox_gr_port1>>>'. Use the localAddress option to override.

<<<(__version_num<80011)?"WARNING: Instance '"+hostname+":"+__mysql_sandbox_port1+"' cannot persist Group Replication configuration since MySQL version "+__version+" does not support the SET PERSIST command (MySQL version >= 8.0.11 required). Please use the dba.configureLocalInstance() command locally to persist the changes.\n":""\>>>
Creating InnoDB cluster 'devCluster' on '<<<hostname>>>:<<<__mysql_sandbox_port1>>>'...

The MySQL InnoDB cluster is going to be setup in advanced Multi-Primary Mode. Consult its requirements and limitations in https://dev.mysql.com/doc/refman/en/group-replication-limitations.html
Adding Seed Instance...
NOTE: Metadata schema found in target instance
Cluster successfully created. Use Cluster.addInstance() to add MySQL instances.
At least 3 instances are needed for the cluster to be able to withstand up to
one server failure.

<Cluster:devCluster>

//@ Cluster: addInstance with interaction, error
||The instance '<<<hostname>>>:<<<__mysql_sandbox_port1>>>' is already part of this InnoDB cluster

//@<OUT> Cluster: addInstance with interaction, ok
Validating instance configuration at localhost:<<<__mysql_sandbox_port2>>>...
NOTE: Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as <<<hostname>>>:<<<__mysql_sandbox_port2>>>

Instance configuration is suitable.
NOTE: Group Replication will communicate with other members using '<<<hostname>>>:<<<__mysql_sandbox_port2>>>1'. Use the localAddress option to override.

A new instance will be added to the InnoDB cluster. Depending on the amount of
data on the cluster this might take from a few seconds to several hours.

<<<(__version_num<80011)?"WARNING: Instance '"+hostname+":"+__mysql_sandbox_port2+"' cannot persist Group Replication configuration since MySQL version "+__version+" does not support the SET PERSIST command (MySQL version >= 8.0.11 required). Please use the dba.configureLocalInstance() command locally to persist the changes.\n":""\>>>
Adding instance to the cluster...

Monitoring recovery process of the new cluster member. Press ^C to stop monitoring and let it continue in background.

//@<OUT> Cluster: addInstance with interaction, ok
{{State recovery already finished for '<<<hostname>>>:<<<__mysql_sandbox_port2>>>'|Incremental state recovery is now in progress.}}

//@<OUT> Cluster: addInstance with interaction, ok
<<<(__version_num<80011)?"WARNING: Instance '"+hostname+":"+__mysql_sandbox_port1+"' cannot persist configuration since MySQL version "+__version+" does not support the SET PERSIST command (MySQL version >= 8.0.11 required). Please use the dba.configureLocalInstance() command locally to persist the changes.\n":""\>>>
The instance '<<<hostname>>>:<<<__mysql_sandbox_port2>>>' was successfully added to the cluster.

//@<OUT> Cluster: addInstance 3 with interaction, ok
Validating instance configuration at localhost:<<<__mysql_sandbox_port3>>>...
NOTE: Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as <<<hostname>>>:<<<__mysql_sandbox_port3>>>

Instance configuration is suitable.
NOTE: Group Replication will communicate with other members using '<<<hostname>>>:<<<__mysql_sandbox_port3>>>1'. Use the localAddress option to override.

A new instance will be added to the InnoDB cluster. Depending on the amount of
data on the cluster this might take from a few seconds to several hours.

<<<(__version_num<80011)?"WARNING: Instance '"+hostname+":"+__mysql_sandbox_port3+"' cannot persist Group Replication configuration since MySQL version "+__version+" does not support the SET PERSIST command (MySQL version >= 8.0.11 required). Please use the dba.configureLocalInstance() command locally to persist the changes.\n":""\>>>
Adding instance to the cluster...

Monitoring recovery process of the new cluster member. Press ^C to stop monitoring and let it continue in background.

//@<OUT> Cluster: addInstance 3 with interaction, ok
{{State recovery already finished for '<<<hostname>>>:<<<__mysql_sandbox_port3>>>'|Incremental state recovery is now in progress.}}

//@<OUT> Cluster: addInstance 3 with interaction, ok
<<<(__version_num<80011)?"WARNING: Instance '"+hostname+":"+__mysql_sandbox_port1+"' cannot persist configuration since MySQL version "+__version+" does not support the SET PERSIST command (MySQL version >= 8.0.11 required). Please use the dba.configureLocalInstance() command locally to persist the changes.\n":""\>>>
<<<(__version_num<80011)?"WARNING: Instance '"+hostname+":"+__mysql_sandbox_port2+"' cannot persist configuration since MySQL version "+__version+" does not support the SET PERSIST command (MySQL version >= 8.0.11 required). Please use the dba.configureLocalInstance() command locally to persist the changes.\n":""\>>>
The instance '<<<hostname>>>:<<<__mysql_sandbox_port3>>>' was successfully added to the cluster.

//@<OUT> Cluster: describe1
{
    "clusterName": "devCluster",
    "defaultReplicaSet": {
        "name": "default",
        "topology": [
            {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>",
                "label": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>",
                "role": "HA"
            },
            {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port2>>>",
                "label": "<<<hostname>>>:<<<__mysql_sandbox_port2>>>",
                "role": "HA"
            },
            {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port3>>>",
                "label": "<<<hostname>>>:<<<__mysql_sandbox_port3>>>",
                "role": "HA"
            }
        ],
        "topologyMode": "Multi-Primary"
    }
}

//@ Cluster: removeInstance
||

//@<OUT> Cluster: describe2
{
    "clusterName": "devCluster",
    "defaultReplicaSet": {
        "name": "default",
        "topology": [
            {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>",
                "label": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>",
                "role": "HA"
            },
            {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port3>>>",
                "label": "<<<hostname>>>:<<<__mysql_sandbox_port3>>>",
                "role": "HA"
            }
        ],
        "topologyMode": "Multi-Primary"
    }
}

//@ Cluster: remove_instance 3
||

//@ Cluster: Error cannot remove last instance
||The instance 'localhost:<<<__mysql_sandbox_port1>>>' is the last member of the cluster (RuntimeError)

//@ Dissolve cluster with success
|The cluster was successfully dissolved.|

//@<OUT> Dba: createCluster multiPrimary with interaction 2, ok
A new InnoDB cluster will be created on instance 'localhost:<<<__mysql_sandbox_port1>>>'.

The MySQL InnoDB cluster is going to be setup in advanced Multi-Primary Mode.
Before continuing you have to confirm that you understand the requirements and
limitations of Multi-Primary Mode. For more information see
https://dev.mysql.com/doc/refman/en/group-replication-limitations.html before
proceeding.

I have read the MySQL InnoDB cluster manual and I understand the requirements
and limitations of advanced Multi-Primary Mode.

Confirm [y/N]:
Disabling super_read_only mode on instance '<<<hostname>>>:<<<__mysql_sandbox_port1>>>'.
Validating instance configuration at localhost:<<<__mysql_sandbox_port1>>>...
NOTE: Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as <<<hostname>>>:<<<__mysql_sandbox_port1>>>

Instance configuration is suitable.
NOTE: Group Replication will communicate with other members using '<<<hostname>>>:<<<__mysql_sandbox_gr_port1>>>'. Use the localAddress option to override.

<<<(__version_num<80011)?"WARNING: Instance '"+hostname+":"+__mysql_sandbox_port1+"' cannot persist Group Replication configuration since MySQL version "+__version+" does not support the SET PERSIST command (MySQL version >= 8.0.11 required). Please use the dba.configureLocalInstance() command locally to persist the changes.\n":""\>>>
Creating InnoDB cluster 'devCluster' on '<<<hostname>>>:<<<__mysql_sandbox_port1>>>'...

The MySQL InnoDB cluster is going to be setup in advanced Multi-Primary Mode. Consult its requirements and limitations in https://dev.mysql.com/doc/refman/en/group-replication-limitations.html
Adding Seed Instance...
NOTE: Metadata schema found in target instance
Cluster successfully created. Use Cluster.addInstance() to add MySQL instances.
At least 3 instances are needed for the cluster to be able to withstand up to
one server failure.

<Cluster:devCluster>

//@<OUT> Cluster: addInstance with interaction, ok 2
Validating instance configuration at localhost:<<<__mysql_sandbox_port2>>>...
NOTE: Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as <<<hostname>>>:<<<__mysql_sandbox_port2>>>

Instance configuration is suitable.
NOTE: Group Replication will communicate with other members using '<<<hostname>>>:<<<__mysql_sandbox_gr_port2>>>'. Use the localAddress option to override.
<<<(__version_num<80011)?"WARNING: Instance '"+hostname+":"+__mysql_sandbox_port2+"' cannot persist Group Replication configuration since MySQL version "+__version+" does not support the SET PERSIST command (MySQL version >= 8.0.11 required). Please use the dba.configureLocalInstance() command locally to persist the changes.\n":""\>>>

//@<OUT> Cluster: addInstance with interaction, ok 2
{{State recovery already finished for '<<<hostname>>>:<<<__mysql_sandbox_port2>>>'|Incremental state recovery is now in progress.}}

//@<OUT> Cluster: addInstance with interaction, ok 2
<<<(__version_num<80011)?"WARNING: Instance '"+hostname+":"+__mysql_sandbox_port1+"' cannot persist configuration since MySQL version "+__version+" does not support the SET PERSIST command (MySQL version >= 8.0.11 required). Please use the dba.configureLocalInstance() command locally to persist the changes.\n":""\>>>
The instance '<<<hostname>>>:<<<__mysql_sandbox_port2>>>' was successfully added to the cluster.

//@<OUT> Cluster: addInstance with interaction, ok 3
Validating instance configuration at localhost:<<<__mysql_sandbox_port3>>>...
NOTE: Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as <<<hostname>>>:<<<__mysql_sandbox_port3>>>

Instance configuration is suitable.
NOTE: Group Replication will communicate with other members using '<<<hostname>>>:<<<__mysql_sandbox_port3>>>1'. Use the localAddress option to override.

A new instance will be added to the InnoDB cluster. Depending on the amount of
data on the cluster this might take from a few seconds to several hours.

<<<(__version_num<80011)?"WARNING: Instance '"+hostname+":"+__mysql_sandbox_port3+"' cannot persist Group Replication configuration since MySQL version "+__version+" does not support the SET PERSIST command (MySQL version >= 8.0.11 required). Please use the dba.configureLocalInstance() command locally to persist the changes.\n":""\>>>
Adding instance to the cluster...

//@<OUT> Cluster: addInstance with interaction, ok 3
{{State recovery already finished for '<<<hostname>>>:<<<__mysql_sandbox_port3>>>'|Incremental state recovery is now in progress.}}

//@<OUT> Cluster: addInstance with interaction, ok 3
<<<(__version_num<80011)?"WARNING: Instance '"+hostname+":"+__mysql_sandbox_port1+"' cannot persist configuration since MySQL version "+__version+" does not support the SET PERSIST command (MySQL version >= 8.0.11 required). Please use the dba.configureLocalInstance() command locally to persist the changes.\n":""\>>>
<<<(__version_num<80011)?"WARNING: Instance '"+hostname+":"+__mysql_sandbox_port2+"' cannot persist configuration since MySQL version "+__version+" does not support the SET PERSIST command (MySQL version >= 8.0.11 required). Please use the dba.configureLocalInstance() command locally to persist the changes.\n":""\>>>
The instance '<<<hostname>>>:<<<__mysql_sandbox_port3>>>' was successfully added to the cluster.

//@ Disable group_replication_start_on_boot if version >= 8.0.11 {VER(>=8.0.11)}
||

//@# Dba: stop instance 3
||

//@: Cluster: rejoinInstance errors
||Invalid number of arguments, expected 1 to 2 but got 0
||Invalid number of arguments, expected 1 to 2 but got 3
||Invalid connection options, expected either a URI or a Connection Options Dictionary
||Invalid values in connection options: authMethod
||Could not open connection to 'localhost:3306'

//@<OUT> Cluster: rejoinInstance with interaction, ok {VER(>=8.0.11)}
WARNING: Option 'memberSslMode' is deprecated for this operation and it will be removed in a future release. This option is not needed because the SSL mode is automatically obtained from the cluster. Please do not use it here.

Rejoining instance '<<<hostname>>>:<<<__mysql_sandbox_port3>>>' to cluster 'devCluster'...

The instance '<<<hostname>>>:<<<__mysql_sandbox_port3>>>' was successfully rejoined to the cluster.

//@<OUT> Cluster: rejoinInstance with interaction, ok {VER(<8.0.11)}
WARNING: Option 'memberSslMode' is deprecated for this operation and it will be removed in a future release. This option is not needed because the SSL mode is automatically obtained from the cluster. Please do not use it here.

WARNING: Instance '<<<hostname>>>:<<<__mysql_sandbox_port3>>>' cannot persist Group Replication configuration since MySQL version <<<__version>>> does not support the SET PERSIST command (MySQL version >= 8.0.11 required). Please use the dba.configureLocalInstance() command locally to persist the changes.
Rejoining instance '<<<hostname>>>:<<<__mysql_sandbox_port3>>>' to cluster 'devCluster'...

WARNING: Instance '<<<hostname>>>:<<<__mysql_sandbox_port1>>>' cannot persist configuration since MySQL version <<<__version>>> does not support the SET PERSIST command (MySQL version >= 8.0.11 required). Please use the dba.configureLocalInstance() command locally to persist the changes.
WARNING: Instance '<<<hostname>>>:<<<__mysql_sandbox_port2>>>' cannot persist configuration since MySQL version <<<__version>>> does not support the SET PERSIST command (MySQL version >= 8.0.11 required). Please use the dba.configureLocalInstance() command locally to persist the changes.
The instance '<<<hostname>>>:<<<__mysql_sandbox_port3>>>' was successfully rejoined to the cluster.
