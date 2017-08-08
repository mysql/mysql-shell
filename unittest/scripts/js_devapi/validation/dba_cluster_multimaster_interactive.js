//@<OUT> Dba: createCluster multiMaster with interaction, cancel
A new InnoDB cluster will be created on instance 'root@localhost:<<<__mysql_sandbox_port1>>>'.

The MySQL InnoDB cluster is going to be setup in advanced Multi-Master Mode.
Before continuing you have to confirm that you understand the requirements and
limitations of Multi-Master Mode. For more information see
https://dev.mysql.com/doc/refman/en/group-replication-limitations.html
before proceeding.


I have read the MySQL InnoDB cluster manual and I understand the requirements
and limitations of advanced Multi-Master Mode.
Confirm [y|N]:
Cancelled

//@<OUT> Dba: createCluster multiMaster with interaction, ok
A new InnoDB cluster will be created on instance 'root@localhost:<<<__mysql_sandbox_port1>>>'.

The MySQL InnoDB cluster is going to be setup in advanced Multi-Master Mode.
Before continuing you have to confirm that you understand the requirements and
limitations of Multi-Master Mode. For more information see
https://dev.mysql.com/doc/refman/en/group-replication-limitations.html
before proceeding.


I have read the MySQL InnoDB cluster manual and I understand the requirements
and limitations of advanced Multi-Master Mode.
Confirm [y|N]:
Creating InnoDB cluster 'devCluster' on 'root@localhost:<<<__mysql_sandbox_port1>>>'...
Adding Seed Instance...

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

The instance 'root@localhost:<<<__mysql_sandbox_port2>>>' was successfully added to the cluster.

//@<OUT> Cluster: addInstance 3 with interaction, ok
A new instance will be added to the InnoDB cluster. Depending on the amount of
data on the cluster this might take from a few seconds to several hours.

Adding instance to the cluster ...

The instance 'root@localhost:<<<__mysql_sandbox_port3>>>' was successfully added to the cluster.

//@<OUT> Cluster: describe1
{
    "clusterName": "devCluster",
    "defaultReplicaSet": {
        "instances": [
            {
                "host": "localhost:<<<__mysql_sandbox_port1>>>",
                "label": "localhost:<<<__mysql_sandbox_port1>>>",
                "role": "HA"
            },
            {
                "host": "localhost:<<<__mysql_sandbox_port2>>>",
                "label": "localhost:<<<__mysql_sandbox_port2>>>",
                "role": "HA"
            },
            {
                "host": "localhost:<<<__mysql_sandbox_port3>>>",
                "label": "localhost:<<<__mysql_sandbox_port3>>>",
                "role": "HA"
            }
        ],
        "name": "default"
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
            "localhost:<<<__mysql_sandbox_port1>>>": {
                "address": "localhost:<<<__mysql_sandbox_port1>>>",
                "mode": "R/W",
                "readReplicas": {},
                "role": "HA",
                "status": "ONLINE"
            },
            "localhost:<<<__mysql_sandbox_port2>>>": {
                "address": "localhost:<<<__mysql_sandbox_port2>>>",
                "mode": "R/W",
                "readReplicas": {},
                "role": "HA",
                "status": "ONLINE"
            },
            "localhost:<<<__mysql_sandbox_port3>>>": {
                "address": "localhost:<<<__mysql_sandbox_port3>>>",
                "mode": "R/W",
                "readReplicas": {},
                "role": "HA",
                "status": "ONLINE"
            }
        }
    }
}

//@ Cluster: removeInstance
||

//@<OUT> Cluster: describe2
{
    "clusterName": "devCluster",
    "defaultReplicaSet": {
        "instances": [
            {
                "host": "localhost:<<<__mysql_sandbox_port1>>>",
                "label": "localhost:<<<__mysql_sandbox_port1>>>",
                "role": "HA"
            },
            {
                "host": "localhost:<<<__mysql_sandbox_port3>>>",
                "label": "localhost:<<<__mysql_sandbox_port3>>>",
                "role": "HA"
            }
        ],
        "name": "default"
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
            "localhost:<<<__mysql_sandbox_port1>>>": {
                "address": "localhost:<<<__mysql_sandbox_port1>>>",
                "mode": "R/W",
                "readReplicas": {},
                "role": "HA",
                "status": "ONLINE"
            },
            "localhost:<<<__mysql_sandbox_port3>>>": {
                "address": "localhost:<<<__mysql_sandbox_port3>>>",
                "mode": "R/W",
                "readReplicas": {},
                "role": "HA",
                "status": "ONLINE"
            }
        }
    }
}

//@ Cluster: remove_instance 3
||

//@ Cluster: Error cannot remove last instance
||Cluster.removeInstance: The instance '<<<localhost>>>:<<<__mysql_sandbox_port1>>>' cannot be removed because it is the only member of the Cluster. Please use <Cluster>.dissolve() instead to remove the last instance and dissolve the Cluster. (LogicError)

//@ Dissolve cluster with success
|The cluster was successfully dissolved.|

//@<OUT> Dba: createCluster multiMaster with interaction 2, ok
A new InnoDB cluster will be created on instance 'root@localhost:<<<__mysql_sandbox_port1>>>'.

The MySQL InnoDB cluster is going to be setup in advanced Multi-Master Mode.
Before continuing you have to confirm that you understand the requirements and
limitations of Multi-Master Mode. For more information see
https://dev.mysql.com/doc/refman/en/group-replication-limitations.html
before proceeding.


I have read the MySQL InnoDB cluster manual and I understand the requirements
and limitations of advanced Multi-Master Mode.
Confirm [y|N]:
Creating InnoDB cluster 'devCluster' on 'root@localhost:<<<__mysql_sandbox_port1>>>'...
Adding Seed Instance...

Cluster successfully created. Use Cluster.addInstance() to add MySQL instances.
At least 3 instances are needed for the cluster to be able to withstand up to
one server failure.

<Cluster:devCluster>

//@<OUT> Cluster: addInstance with interaction, ok 2
A new instance will be added to the InnoDB cluster. Depending on the amount of
data on the cluster this might take from a few seconds to several hours.

Adding instance to the cluster ...

The instance 'root@localhost:<<<__mysql_sandbox_port2>>>' was successfully added to the cluster.

//@<OUT> Cluster: addInstance with interaction, ok 3
A new instance will be added to the InnoDB cluster. Depending on the amount of
data on the cluster this might take from a few seconds to several hours.

Adding instance to the cluster ...

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
            "localhost:<<<__mysql_sandbox_port1>>>": {
                "address": "localhost:<<<__mysql_sandbox_port1>>>",
                "mode": "R/W",
                "readReplicas": {},
                "role": "HA",
                "status": "ONLINE"
            },
            "localhost:<<<__mysql_sandbox_port2>>>": {
                "address": "localhost:<<<__mysql_sandbox_port2>>>",
                "mode": "R/W",
                "readReplicas": {},
                "role": "HA",
                "status": "ONLINE"
            },
            "localhost:<<<__mysql_sandbox_port3>>>": {
                "address": "localhost:<<<__mysql_sandbox_port3>>>",
                "mode": "R/W",
                "readReplicas": {},
                "role": "HA",
                "status": "ONLINE"
            }
        }
    }
}

//@# Dba: kill instance 3
||

//@# Dba: start instance 3
||

//@: Cluster: rejoinInstance errors
||Invalid number of arguments in Cluster.rejoinInstance, expected 1 to 2 but got 0
||Invalid number of arguments in Cluster.rejoinInstance, expected 1 to 2 but got 3
||Invalid connection options, expected either a URI or a Dictionary
||Cluster.rejoinInstance: The instance 'localhost:3306' does not belong to the ReplicaSet: 'default'
||Cluster.rejoinInstance: Invalid values in connection options: authMethod
||Cluster.rejoinInstance: The instance 'somehost:3306' does not belong to the ReplicaSet: 'default'

//@<OUT> Cluster: rejoinInstance with interaction, ok
Rejoining the instance to the InnoDB cluster. Depending on the original
problem that made the instance unavailable, the rejoin operation might not be
successful and further manual steps will be needed to fix the underlying
problem.

Please monitor the output of the rejoin operation and take necessary action if
the instance cannot rejoin.

Please provide the password for 'root@localhost:<<<__mysql_sandbox_port3>>>': Rejoining instance to the cluster ...

The instance 'root@localhost:<<<__mysql_sandbox_port3>>>' was successfully rejoined on the cluster.

//@<OUT> Cluster: status for rejoin: success
{
    "clusterName": "devCluster",
    "defaultReplicaSet": {
        "name": "default",
        "ssl": "<<<__ssl_mode>>>",
        "status": "OK",
        "statusText": "Cluster is ONLINE and can tolerate up to ONE failure.",
        "topology": {
            "localhost:<<<__mysql_sandbox_port1>>>": {
                "address": "localhost:<<<__mysql_sandbox_port1>>>",
                "mode": "R/W",
                "readReplicas": {},
                "role": "HA",
                "status": "ONLINE"
            },
            "localhost:<<<__mysql_sandbox_port2>>>": {
                "address": "localhost:<<<__mysql_sandbox_port2>>>",
                "mode": "R/W",
                "readReplicas": {},
                "role": "HA",
                "status": "ONLINE"
            },
            "localhost:<<<__mysql_sandbox_port3>>>": {
                "address": "localhost:<<<__mysql_sandbox_port3>>>",
                "mode": "R/W",
                "readReplicas": {},
                "role": "HA",
                "status": "ONLINE"
            }
        }
    }
}
