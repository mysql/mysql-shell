//@<OUT> Dba: createCluster multiPrimary with interaction, cancel
A new InnoDB cluster will be created on instance 'root@localhost:<<<__mysql_sandbox_port1>>>'.

The MySQL InnoDB cluster is going to be setup in advanced Multi-Primary Mode.
Before continuing you have to confirm that you understand the requirements and
limitations of Multi-Primary Mode. For more information see
https://dev.mysql.com/doc/refman/en/group-replication-limitations.html
before proceeding.


I have read the MySQL InnoDB cluster manual and I understand the requirements
and limitations of advanced Multi-Primary Mode.
Confirm [y/N]:
Dba.createCluster: Cancelled

//@<OUT> Dba: createCluster multiPrimary with interaction, ok
A new InnoDB cluster will be created on instance 'root@localhost:<<<__mysql_sandbox_port1>>>'.

The MySQL InnoDB cluster is going to be setup in advanced Multi-Primary Mode.
Before continuing you have to confirm that you understand the requirements and
limitations of Multi-Primary Mode. For more information see
https://dev.mysql.com/doc/refman/en/group-replication-limitations.html
before proceeding.


I have read the MySQL InnoDB cluster manual and I understand the requirements
and limitations of advanced Multi-Primary Mode.
Confirm [y/N]:
Validating instance at localhost:<<<__mysql_sandbox_port1>>>...
NOTE: Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as <<<hostname>>>

Instance configuration is suitable.
Creating InnoDB cluster 'devCluster' on 'root@localhost:<<<__mysql_sandbox_port1>>>'...

Adding Seed Instance...
<<<(__version_num<80011)?"WARNING: On instance '"+localhost+":"+__mysql_sandbox_port1+"' membership change cannot be persisted since MySQL version "+__version+" does not support the SET PERSIST command (MySQL version >= 8.0.11 required). Please use the <Dba>.configureLocalInstance() command locally to persist the changes.\n":""\>>>
Cluster successfully created. Use Cluster.addInstance() to add MySQL instances.
At least 3 instances are needed for the cluster to be able to withstand up to
one server failure.

<Cluster:devCluster>

//@ Dissolve cluster
|The cluster was successfully dissolved.|

//@<OUT> Dba: createCluster multiMaster with interaction, regression for BUG#25926603
A new InnoDB cluster will be created on instance 'root@localhost:<<<__mysql_sandbox_port1>>>'.

WARNING: The multiMaster option is deprecated. Please use the multiPrimary option instead.
The MySQL InnoDB cluster is going to be setup in advanced Multi-Primary Mode.
Before continuing you have to confirm that you understand the requirements and
limitations of Multi-Primary Mode. For more information see
https://dev.mysql.com/doc/refman/en/group-replication-limitations.html
before proceeding.


I have read the MySQL InnoDB cluster manual and I understand the requirements
and limitations of advanced Multi-Primary Mode.
Confirm [y/N]:
Validating instance at localhost:<<<__mysql_sandbox_port1>>>...
NOTE: Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as <<<hostname>>>

Instance configuration is suitable.
Creating InnoDB cluster 'devCluster' on 'root@localhost:<<<__mysql_sandbox_port1>>>'...

Adding Seed Instance...
<<<(__version_num<80011)?"WARNING: On instance '"+localhost+":"+__mysql_sandbox_port1+"' membership change cannot be persisted since MySQL version "+__version+" does not support the SET PERSIST command (MySQL version >= 8.0.11 required). Please use the <Dba>.configureLocalInstance() command locally to persist the changes.\n":""\>>>
Cluster successfully created. Use Cluster.addInstance() to add MySQL instances.
At least 3 instances are needed for the cluster to be able to withstand up to
one server failure.

<Cluster:devCluster>

//@ Cluster: addInstance with interaction, error
||Cluster.addInstance: The instance 'localhost:<<<__mysql_sandbox_port1>>>' is already part of this InnoDB cluster

//@<OUT> Cluster: addInstance with interaction, ok
A new instance will be added to the InnoDB cluster. Depending on the amount of
data on the cluster this might take from a few seconds to several hours.

Adding instance to the cluster ...

Validating instance at localhost:<<<__mysql_sandbox_port2>>>...
NOTE: Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as <<<hostname>>>

Instance configuration is suitable.
<<<(__version_num<80011)?"WARNING: On instance '"+localhost+":"+__mysql_sandbox_port2+"' membership change cannot be persisted since MySQL version "+__version+" does not support the SET PERSIST command (MySQL version >= 8.0.11 required). Please use the <Dba>.configureLocalInstance() command locally to persist the changes.\n":""\>>>
<<<(__version_num<80011)?"WARNING: On instance '"+hostname+":"+__mysql_sandbox_port1+"' membership change cannot be persisted since MySQL version "+__version+" does not support the SET PERSIST command (MySQL version >= 8.0.11 required). Please use the <Dba>.configureLocalInstance() command locally to persist the changes.\n":""\>>>
The instance 'root@localhost:<<<__mysql_sandbox_port2>>>' was successfully added to the cluster.

//@<OUT> Cluster: addInstance 3 with interaction, ok
A new instance will be added to the InnoDB cluster. Depending on the amount of
data on the cluster this might take from a few seconds to several hours.

Adding instance to the cluster ...

Validating instance at localhost:<<<__mysql_sandbox_port3>>>...
NOTE: Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as <<<hostname>>>

Instance configuration is suitable.
<<<(__version_num<80011)?"WARNING: On instance '"+localhost+":"+__mysql_sandbox_port3+"' membership change cannot be persisted since MySQL version "+__version+" does not support the SET PERSIST command (MySQL version >= 8.0.11 required). Please use the <Dba>.configureLocalInstance() command locally to persist the changes.\n":""\>>>
<<<(__version_num<80011)?"WARNING: On instance '"+hostname+":"+__mysql_sandbox_port1+"' membership change cannot be persisted since MySQL version "+__version+" does not support the SET PERSIST command (MySQL version >= 8.0.11 required). Please use the <Dba>.configureLocalInstance() command locally to persist the changes.\n":""\>>>
<<<(__version_num<80011)?"WARNING: On instance '"+hostname+":"+__mysql_sandbox_port2+"' membership change cannot be persisted since MySQL version "+__version+" does not support the SET PERSIST command (MySQL version >= 8.0.11 required). Please use the <Dba>.configureLocalInstance() command locally to persist the changes.\n":""\>>>
The instance 'root@localhost:<<<__mysql_sandbox_port3>>>' was successfully added to the cluster.

//@<OUT> Cluster: describe1
{
    "clusterName": "devCluster",
    "defaultReplicaSet": {
        "name": "default",
        "topology": [
            {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>",
                "label": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>",
                "role": "HA"<<<(__version_num>=80011)?",\n[[*]]\"version\": \"" + __version + "\"":"">>>
            },
            {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port2>>>",
                "label": "<<<hostname>>>:<<<__mysql_sandbox_port2>>>",
                "role": "HA"<<<(__version_num>=80011)?",\n[[*]]\"version\": \"" + __version + "\"":"">>>
            },
            {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port3>>>",
                "label": "<<<hostname>>>:<<<__mysql_sandbox_port3>>>",
                "role": "HA"<<<(__version_num>=80011)?",\n[[*]]\"version\": \"" + __version + "\"":"">>>
            }
        ],
        "topologyMode": "Multi-Primary"
    }
}

//@<OUT> Cluster: status1
{
    "clusterName": "devCluster",
    "defaultReplicaSet": {
        "name": "default",
        "ssl": "<<<__ssl_mode>>>",
        "status": "OK",
        "statusText": "Cluster is ONLINE and can tolerate up to ONE failure.",
        "topology": {
            "<<<hostname>>>:<<<__mysql_sandbox_port1>>>": {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>",
                "mode": "R/W",
                "readReplicas": {},
                "role": "HA",
                "status": "ONLINE"<<<(__version_num>=80011)?",\n[[*]]\"version\": \"" + __version + "\"":"">>>
            },
            "<<<hostname>>>:<<<__mysql_sandbox_port2>>>": {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port2>>>",
                "mode": "R/W",
                "readReplicas": {},
                "role": "HA",
                "status": "ONLINE"<<<(__version_num>=80011)?",\n[[*]]\"version\": \"" + __version + "\"":"">>>
            },
            "<<<hostname>>>:<<<__mysql_sandbox_port3>>>": {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port3>>>",
                "mode": "R/W",
                "readReplicas": {},
                "role": "HA",
                "status": "ONLINE"<<<(__version_num>=80011)?",\n[[*]]\"version\": \"" + __version + "\"":"">>>
            }
        },
        "topologyMode": "Multi-Primary"
    },
    "groupInformationSourceMember": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>"
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
                "role": "HA"<<<(__version_num>=80011)?",\n[[*]]\"version\": \"" + __version + "\"":"">>>
            },
            {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port3>>>",
                "label": "<<<hostname>>>:<<<__mysql_sandbox_port3>>>",
                "role": "HA"<<<(__version_num>=80011)?",\n[[*]]\"version\": \"" + __version + "\"":"">>>
            }
        ],
        "topologyMode": "Multi-Primary"
    }
}

//@<OUT> Cluster: status2
{
    "clusterName": "devCluster",
    "defaultReplicaSet": {
        "name": "default",
        "ssl": "<<<__ssl_mode>>>",
        "status": "OK_NO_TOLERANCE",
        "statusText": "Cluster is NOT tolerant to any failures.",
        "topology": {
            "<<<hostname>>>:<<<__mysql_sandbox_port1>>>": {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>",
                "mode": "R/W",
                "readReplicas": {},
                "role": "HA",
                "status": "ONLINE"<<<(__version_num>=80011)?",\n[[*]]\"version\": \"" + __version + "\"":"">>>
            },
            "<<<hostname>>>:<<<__mysql_sandbox_port3>>>": {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port3>>>",
                "mode": "R/W",
                "readReplicas": {},
                "role": "HA",
                "status": "ONLINE"<<<(__version_num>=80011)?",\n[[*]]\"version\": \"" + __version + "\"":"">>>
            }
        },
        "topologyMode": "Multi-Primary"
    },
    "groupInformationSourceMember": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>"
}

//@ Cluster: remove_instance 3
||

//@ Cluster: Error cannot remove last instance
||Cluster.removeInstance: The instance '<<<localhost>>>:<<<__mysql_sandbox_port1>>>' is the last member of the ReplicaSet (LogicError)

//@ Dissolve cluster with success
|The cluster was successfully dissolved.|

//@<OUT> Dba: createCluster multiPrimary with interaction 2, ok
A new InnoDB cluster will be created on instance 'root@localhost:<<<__mysql_sandbox_port1>>>'.

The MySQL InnoDB cluster is going to be setup in advanced Multi-Primary Mode.
Before continuing you have to confirm that you understand the requirements and
limitations of Multi-Primary Mode. For more information see
https://dev.mysql.com/doc/refman/en/group-replication-limitations.html
before proceeding.


I have read the MySQL InnoDB cluster manual and I understand the requirements
and limitations of advanced Multi-Primary Mode.
Confirm [y/N]:
Validating instance at localhost:<<<__mysql_sandbox_port1>>>...
NOTE: Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as <<<hostname>>>

Instance configuration is suitable.
Creating InnoDB cluster 'devCluster' on 'root@localhost:<<<__mysql_sandbox_port1>>>'...

Adding Seed Instance...
<<<(__version_num<80011)?"WARNING: On instance '"+localhost+":"+__mysql_sandbox_port1+"' membership change cannot be persisted since MySQL version "+__version+" does not support the SET PERSIST command (MySQL version >= 8.0.11 required). Please use the <Dba>.configureLocalInstance() command locally to persist the changes.\n":""\>>>
Cluster successfully created. Use Cluster.addInstance() to add MySQL instances.
At least 3 instances are needed for the cluster to be able to withstand up to
one server failure.

<Cluster:devCluster>

//@<OUT> Cluster: addInstance with interaction, ok 2
A new instance will be added to the InnoDB cluster. Depending on the amount of
data on the cluster this might take from a few seconds to several hours.

Adding instance to the cluster ...

Validating instance at localhost:<<<__mysql_sandbox_port2>>>...
NOTE: Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as <<<hostname>>>

Instance configuration is suitable.
<<<(__version_num<80011)?"WARNING: On instance '"+localhost+":"+__mysql_sandbox_port2+"' membership change cannot be persisted since MySQL version "+__version+" does not support the SET PERSIST command (MySQL version >= 8.0.11 required). Please use the <Dba>.configureLocalInstance() command locally to persist the changes.\n":""\>>>
<<<(__version_num<80011)?"WARNING: On instance '"+hostname+":"+__mysql_sandbox_port1+"' membership change cannot be persisted since MySQL version "+__version+" does not support the SET PERSIST command (MySQL version >= 8.0.11 required). Please use the <Dba>.configureLocalInstance() command locally to persist the changes.\n":""\>>>
The instance 'root@localhost:<<<__mysql_sandbox_port2>>>' was successfully added to the cluster.

ONLINE

//@<OUT> Cluster: addInstance with interaction, ok 3
A new instance will be added to the InnoDB cluster. Depending on the amount of
data on the cluster this might take from a few seconds to several hours.

Adding instance to the cluster ...

Validating instance at localhost:<<<__mysql_sandbox_port3>>>...
NOTE: Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as <<<hostname>>>

Instance configuration is suitable.
<<<(__version_num<80011)?"WARNING: On instance '"+localhost+":"+__mysql_sandbox_port3+"' membership change cannot be persisted since MySQL version "+__version+" does not support the SET PERSIST command (MySQL version >= 8.0.11 required). Please use the <Dba>.configureLocalInstance() command locally to persist the changes.\n":""\>>>
<<<(__version_num<80011)?"WARNING: On instance '"+hostname+":"+__mysql_sandbox_port1+"' membership change cannot be persisted since MySQL version "+__version+" does not support the SET PERSIST command (MySQL version >= 8.0.11 required). Please use the <Dba>.configureLocalInstance() command locally to persist the changes.\n":""\>>>
<<<(__version_num<80011)?"WARNING: On instance '"+hostname+":"+__mysql_sandbox_port2+"' membership change cannot be persisted since MySQL version "+__version+" does not support the SET PERSIST command (MySQL version >= 8.0.11 required). Please use the <Dba>.configureLocalInstance() command locally to persist the changes.\n":""\>>>
The instance 'root@localhost:<<<__mysql_sandbox_port3>>>' was successfully added to the cluster.

//@<OUT> Cluster: status: success
{
    "clusterName": "devCluster",
    "defaultReplicaSet": {
        "name": "default",
        "ssl": "<<<__ssl_mode>>>",
        "status": "OK",
        "statusText": "Cluster is ONLINE and can tolerate up to ONE failure.",
        "topology": {
            "<<<hostname>>>:<<<__mysql_sandbox_port1>>>": {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>",
                "mode": "R/W",
                "readReplicas": {},
                "role": "HA",
                "status": "ONLINE"<<<(__version_num>=80011)?",\n[[*]]\"version\": \"" + __version + "\"":"">>>
            },
            "<<<hostname>>>:<<<__mysql_sandbox_port2>>>": {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port2>>>",
                "mode": "R/W",
                "readReplicas": {},
                "role": "HA",
                "status": "ONLINE"<<<(__version_num>=80011)?",\n[[*]]\"version\": \"" + __version + "\"":"">>>
            },
            "<<<hostname>>>:<<<__mysql_sandbox_port3>>>": {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port3>>>",
                "mode": "R/W",
                "readReplicas": {},
                "role": "HA",
                "status": "ONLINE"<<<(__version_num>=80011)?",\n[[*]]\"version\": \"" + __version + "\"":"">>>
            }
        },
        "topologyMode": "Multi-Primary"
    },
    "groupInformationSourceMember": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>"
}

//@ Disable group_replication_start_on_boot if version >= 8.0.11 {VER(>=8.0.11)}
||

//@# Dba: stop instance 3
||

//@: Cluster: rejoinInstance errors
||Cluster.rejoinInstance: Invalid number of arguments, expected 1 to 2 but got 0
||Cluster.rejoinInstance: Invalid number of arguments, expected 1 to 2 but got 3
||Invalid connection options, expected either a URI or a Dictionary
||Cluster.rejoinInstance: Could not open connection to 'localhost:3306'
||Cluster.rejoinInstance: Invalid values in connection options: authMethod
||Cluster.rejoinInstance: Could not open connection to 'localhost:3306'

//@<OUT> Cluster: rejoinInstance with interaction, ok
Rejoining the instance to the InnoDB cluster. Depending on the original
problem that made the instance unavailable, the rejoin operation might not be
successful and further manual steps will be needed to fix the underlying
problem.

Please monitor the output of the rejoin operation and take necessary action if
the instance cannot rejoin.

Rejoining instance to the cluster ...

WARNING: Option 'memberSslMode' is deprecated for this operation and it will be removed in a future release. This option is not needed because the SSL mode is automatically obtained from the cluster. Please do not use it here.

Please provide the password for 'root@localhost:<<<__mysql_sandbox_port3>>>': The instance 'localhost:<<<__mysql_sandbox_port3>>>' was successfully rejoined on the cluster.

//@<OUT> Cluster: status for rejoin: success
{
    "clusterName": "devCluster",
    "defaultReplicaSet": {
        "name": "default",
        "ssl": "<<<__ssl_mode>>>",
        "status": "OK",
        "statusText": "Cluster is ONLINE and can tolerate up to ONE failure.",
        "topology": {
            "<<<hostname>>>:<<<__mysql_sandbox_port1>>>": {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>",
                "mode": "R/W",
                "readReplicas": {},
                "role": "HA",
                "status": "ONLINE"<<<(__version_num>=80011)?",\n[[*]]\"version\": \"" + __version + "\"":"">>>
            },
            "<<<hostname>>>:<<<__mysql_sandbox_port2>>>": {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port2>>>",
                "mode": "R/W",
                "readReplicas": {},
                "role": "HA",
                "status": "ONLINE"<<<(__version_num>=80011)?",\n[[*]]\"version\": \"" + __version + "\"":"">>>
            },
            "<<<hostname>>>:<<<__mysql_sandbox_port3>>>": {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port3>>>",
                "mode": "R/W",
                "readReplicas": {},
                "role": "HA",
                "status": "ONLINE"<<<(__version_num>=80011)?",\n[[*]]\"version\": \"" + __version + "\"":"">>>
            }
        },
        "topologyMode": "Multi-Primary"
    },
    "groupInformationSourceMember": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>"
}
